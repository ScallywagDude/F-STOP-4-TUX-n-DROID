// Older Source and CSGO VTX files both use version 7. The older layout has
// 25-byte strip groups and 27-byte strips; CSGO appends eight topology bytes
// to each. Keep the file payload and append replacement headers, rebasing only
// byte offsets (strip vertex/index offsets are element indices).
#ifndef FSTOP_LEGACY_VTX_H
#define FSTOP_LEGACY_VTX_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>
#include <vector>

namespace LegacyVtx
{
#pragma pack(push, 1)
struct File { int32_t version, cache; uint16_t stripBones, faceBones; int32_t vertBones, checksum, lods, materials, bodies, bodyOffset; };
struct Body { int32_t models, offset; };
struct Model { int32_t lods, offset; };
struct Lod { int32_t meshes, offset; float distance; };
struct Mesh { int32_t groups, offset; uint8_t flags; };
struct OldGroup { int32_t verts, vertOffset, indices, indexOffset, strips, stripOffset; uint8_t flags; };
struct Group { OldGroup old; int32_t topology, topologyOffset; };
struct OldStrip { int32_t indices, indexOffset, verts, vertOffset; int16_t bones; uint8_t flags; int32_t changes, changeOffset; };
struct Strip { OldStrip old; int32_t topology, topologyOffset; };
struct ReplacementList { int32_t count, offset; };
struct Replacement { int16_t id; int32_t nameOffset; };
#pragma pack(pop)
static_assert(sizeof(File) == 36 && sizeof(Mesh) == 9, "VTX wire layout");
static_assert(sizeof(OldGroup) == 25 && sizeof(Group) == 33, "VTX group layout");
static_assert(sizeof(OldStrip) == 27 && sizeof(Strip) == 35, "VTX strip layout");

struct GroupLocation { size_t mesh, group; int index; };

class Reader
{
public:
    Reader(const void *data, size_t size) : bytes(static_cast<const uint8_t *>(data)), length(size) {}
    bool Range(size_t base, int32_t offset, int32_t count, size_t stride, size_t &at) const
    {
        const int64_t start = static_cast<int64_t>(base) + offset;
        if (count < 0 || start < 0 || static_cast<uint64_t>(start) > length) return false;
        at = static_cast<size_t>(start);
        return stride && static_cast<size_t>(count) <= (length - at) / stride;
    }
    template<class T> bool Read(size_t at, T &value) const
    {
        if (at > length || sizeof(T) > length - at) return false;
        memcpy(&value, bytes + at, sizeof(T));
        return true;
    }
    bool String(size_t base, int32_t offset) const
    {
        size_t at;
        return Range(base, offset, 1, 1, at) && memchr(bytes + at, 0, length - at);
    }
    const uint8_t *bytes;
    size_t length;
};

inline bool Elements(int32_t first, int32_t count, int32_t total)
{
    return first >= 0 && count >= 0 && first <= total && count <= total - first;
}

inline bool Validate(const Reader &r, bool modern, std::vector<GroupLocation> *locations = NULL)
{
    File file;
    size_t bodies, materials;
    if (!r.bytes || !r.Read(0, file) || file.version != 7 || file.lods < 1 || file.lods > 8 ||
        !r.Range(0, file.bodyOffset, file.bodies, sizeof(Body), bodies) ||
        !r.Range(0, file.materials, file.lods, sizeof(ReplacementList), materials)) return false;
    for (int l = 0; l < file.lods; ++l)
    {
        const size_t at = materials + l * sizeof(ReplacementList);
        ReplacementList list;
        size_t replacements;
        if (!r.Read(at, list) || !r.Range(at, list.offset, list.count, sizeof(Replacement), replacements)) return false;
        for (int i = 0; i < list.count; ++i)
        {
            const size_t rp = replacements + i * sizeof(Replacement);
            Replacement item;
            if (!r.Read(rp, item) || !r.String(rp, item.nameOffset)) return false;
        }
    }
    size_t visited = 0;
    for (int b = 0; b < file.bodies; ++b)
    {
        const size_t bp = bodies + b * sizeof(Body);
        Body body;
        size_t models;
        if (!r.Read(bp, body) || !r.Range(bp, body.offset, body.models, sizeof(Model), models)) return false;
        for (int m = 0; m < body.models; ++m)
        {
            const size_t mp = models + m * sizeof(Model);
            Model model;
            size_t lods;
            if (!r.Read(mp, model) || model.lods < 1 || model.lods > file.lods ||
                !r.Range(mp, model.offset, model.lods, sizeof(Lod), lods)) return false;
            for (int l = 0; l < model.lods; ++l)
            {
                const size_t lp = lods + l * sizeof(Lod);
                Lod lod;
                size_t meshes;
                if (!r.Read(lp, lod) || !r.Range(lp, lod.offset, lod.meshes, sizeof(Mesh), meshes)) return false;
                for (int n = 0; n < lod.meshes; ++n)
                {
                    const size_t meshAt = meshes + n * sizeof(Mesh);
                    Mesh mesh;
                    size_t groups;
                    if (!r.Read(meshAt, mesh) || !r.Range(meshAt, mesh.offset, mesh.groups, modern ? sizeof(Group) : sizeof(OldGroup), groups)) return false;
                    for (int g = 0; g < mesh.groups; ++g)
                    {
                        // Bound work and memory for corrupt files with overlapping nested arrays.
                        if (++visited > r.length / sizeof(OldGroup)) return false;
                        const size_t gp = groups + g * (modern ? sizeof(Group) : sizeof(OldGroup));
                        OldGroup group;
                        size_t verts, indices, strips, unused;
                        // Vertex_t has three weights, numBones, uint16 origMeshVertID, three bone IDs.
                        if (!r.Read(gp, group) || !r.Range(gp, group.vertOffset, group.verts, 9, verts) ||
                            !r.Range(gp, group.indexOffset, group.indices, 2, indices) ||
                            !r.Range(gp, group.stripOffset, group.strips, modern ? sizeof(Strip) : sizeof(OldStrip), strips)) return false;
                        Group extended = {};
                        if (modern && (!r.Read(gp, extended) ||
                            !r.Range(gp, extended.topologyOffset, extended.topology, 2, unused))) return false;
                        for (int i = 0; i < group.indices; ++i)
                        {
                            uint16_t index;
                            if (!r.Read(indices + i * 2, index) || index >= group.verts) return false;
                        }
                        for (int s = 0; s < group.strips; ++s)
                        {
                            const size_t sp = strips + s * (modern ? sizeof(Strip) : sizeof(OldStrip));
                            OldStrip strip;
                            if (!r.Read(sp, strip) || !Elements(strip.indexOffset, strip.indices, group.indices) ||
                                !Elements(strip.vertOffset, strip.verts, group.verts) || strip.bones < 0 ||
                                !r.Range(sp, strip.changeOffset, strip.changes, 8, unused)) return false;
                            if (modern)
                            {
                                Strip ex;
                                if (!r.Read(sp, ex) || !Elements(ex.topologyOffset, ex.topology, extended.topology)) return false;
                            }
                        }
                        if (locations)
                        {
                            GroupLocation loc = { meshAt, gp, g };
                            locations->push_back(loc);
                        }
                    }
                }
            }
        }
    }
    return true;
}

enum Result { INVALID, NATIVE, CONVERTED };

inline int32_t Relative(size_t target, size_t origin)
{
    return static_cast<int32_t>(static_cast<int64_t>(target) - static_cast<int64_t>(origin));
}

inline Result Convert(const void *data, size_t size, std::vector<uint8_t> &output)
{
    output.clear();
    Reader r(data, size);
    if (size > INT_MAX) return INVALID;
    if (Validate(r, true)) return NATIVE;
    std::vector<GroupLocation> groups;
    if (!Validate(r, false, &groups)) return INVALID;
    // Compute once before allocating; all relative offsets in the format are signed int32.
    uint64_t total = size + static_cast<uint64_t>(groups.size()) * sizeof(Group);
    for (size_t i = 0; i < groups.size(); ++i)
    {
        OldGroup group;
        r.Read(groups[i].group, group);
        total += static_cast<uint64_t>(group.strips) * sizeof(Strip);
        if (total > INT_MAX) return INVALID;
    }
    output.resize(static_cast<size_t>(total));
    memcpy(&output[0], data, size);
    size_t next = size;
    // Reserve contiguous group arrays first; strips follow them. The mesh headers
    // remain in place, so every body/model/LOD/material replacement offset survives.
    const size_t stripStart = size + groups.size() * sizeof(Group);
    size_t nextStrip = stripStart;
    for (size_t i = 0; i < groups.size(); ++i)
    {
        const GroupLocation &loc = groups[i];
        OldGroup old;
        r.Read(loc.group, old);
        if (loc.index == 0)
        {
            const int32_t offset = Relative(next, loc.mesh);
            memcpy(&output[loc.mesh + offsetof(Mesh, offset)], &offset, sizeof(offset));
        }
        Group group = {};
        group.old = old;
        group.old.vertOffset = Relative(loc.group + old.vertOffset, next);
        group.old.indexOffset = Relative(loc.group + old.indexOffset, next);
        group.old.stripOffset = Relative(nextStrip, next);
        memcpy(&output[next], &group, sizeof(group));
        for (int s = 0; s < old.strips; ++s)
        {
            const size_t sp = loc.group + old.stripOffset + s * sizeof(OldStrip);
            Strip strip = {};
            r.Read(sp, strip.old);
            strip.old.changeOffset = strip.old.changes ? Relative(sp + strip.old.changeOffset, nextStrip) : 0;
            memcpy(&output[nextStrip], &strip, sizeof(strip));
            nextStrip += sizeof(Strip);
        }
        next += sizeof(Group);
    }
    return CONVERTED;
}
}
#endif
