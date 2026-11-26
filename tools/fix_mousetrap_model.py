"""Separate intersecting folding-wall surfaces without changing their bones/animations.

The original wallbuilder_mousetrap mesh is shared by lab_intro and the mousetrap
maps. Its folded panels have 329 pairs of coplanar, overlapping triangles. The
material/bone graph below colors those intersections; each color moves a rigid
surface 1/8 unit along its normal (at most 1/2 unit). Re-run with the original
MDL and VVD extracted from the release VPK. MDL/VTX identity stays unchanged.
"""
import argparse
import hashlib
from pathlib import Path
import struct

COLORS = {(18,0):0,(25,0):1,(20,0):2,(21,0):2,(23,0):3,(16,0):1,
          (17,0):0,(19,0):4,(22,0):3,(24,0):4,(27,1):0,(29,1):1,
          (26,1):1,(28,1):0,(28,2):2,(27,2):1,(29,2):0,(51,3):0,
          (52,1):1,(53,1):1}

def repair(mdl, vvd):
    assert hashlib.sha256(vvd).hexdigest() == 'cf58245c4fa5e19f973a933b5a9d2733eead5aa7f7ad777261d0d44ce721dc4e', 'Supply the original release VVD'
    assert mdl[:4] == b'IDST' and vvd[:4] == b'IDSV'
    assert struct.unpack_from('<i',mdl,8)[0] == -1541831956
    assert struct.unpack_from('<i',vvd,8)[0] == -1541831956
    assert struct.unpack_from('<i',vvd,48)[0] == 0, 'Unexpected vertex fixups'
    parts, offset = struct.unpack_from('<2i',mdl,232)
    assert parts == 1
    count, model_offset = struct.unpack_from('<i4xi',mdl,offset+4)
    assert count == 1
    model = offset + model_offset
    meshes, mesh_offset, vertices, vertex_offset = struct.unpack_from('<4i',mdl,model+72)
    start = struct.unpack_from('<i',vvd,56)[0]
    result = bytearray(vvd)
    for mesh in range(meshes):
        material, _, count, first = struct.unpack_from('<4i',mdl,model+mesh_offset+mesh*116)
        for index in range(first, first+count):
            at = start + vertex_offset + 48*index
            bone, _, _, bone_count = struct.unpack_from('<4B',vvd,at+12)
            color = COLORS.get((bone,material),0)
            if not color:
                continue
            assert bone_count == 1, 'Only rigid panels are offset'
            pos = struct.unpack_from('<3f',vvd,at+16)
            normal = struct.unpack_from('<3f',vvd,at+28)
            struct.pack_into('<3f',result,at+16,*(pos[i]+normal[i]*color*.125 for i in range(3)))
    return result

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('mdl',type=Path)
    parser.add_argument('vvd',type=Path)
    parser.add_argument('output',type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_bytes(repair(args.mdl.read_bytes(),args.vvd.read_bytes()))
    print(hashlib.sha256(args.output.read_bytes()).hexdigest(), args.output)
