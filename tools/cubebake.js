// Builds real cubemaps from faces captured on the phone (out/cube/<map>/*.pfm) and packs them into the map.
// usage: node tools/cubebake.js <map> [--write] [--compare]
//   --compare  measures the result against the cubemap already in the map (1970s is the only map with a real one)
//   --write    writes maps/<map>.bsp and fsdata/fstop/maps/<map>.bsp, keeping the original in maps_orig/
const fs = require('fs'), zlib = require('zlib'), path = require('path');
const ROOT = (__dirname + '/../');
const map = process.argv[2]; const WRITE = process.argv.includes('--write'), COMPARE = process.argv.includes('--compare');
const FACES = ['rt', 'lf', 'bk', 'ft', 'up', 'dn']; // VTF cube order: +x -x +y -y +z -z

function readPfm(f) {
  const b = fs.readFileSync(f); let p = 0; const line = () => { const s = p; while (p < b.length && b[p] !== 10) p++; const t = b.toString('latin1', s, p); p++; return t; };
  if (line().trim() !== 'PF') throw new Error('not a PFM: ' + f);
  const [w, h] = line().trim().split(/\s+/).map(Number); line();
  const d = new Float32Array(w * h * 3);
  for (let y = 0; y < h; y++) { const row = h - 1 - y; for (let i = 0; i < w * 3; i++) d[(y * w * 3) + i] = b.readFloatLE(p + (row * w * 3 + i) * 4); } // top row first
  return { w, h, d };
}
// the camera each face was taken with, in the engine's own convention
const CAM = { rt: [0, 0], lf: [0, 180], bk: [0, 90], ft: [0, 270], up: [-90, 0], dn: [90, 0] };
function basis(pitch, yaw) { const r = Math.PI / 180, sp = Math.sin(pitch * r), cp = Math.cos(pitch * r), sy = Math.sin(yaw * r), cy = Math.cos(yaw * r); return { f: [cp * cy, cp * sy, -sp], r: [sy, -cy, 0], u: [sp * cy, sp * sy, cp] }; }
const dot = (a, b) => a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
const BASES = FACES.map(n => basis(...CAM[n]));
// direction of a texel on a cube face, in the layout textures are stored
function texelDir(face, s, t) { // s,t in -1..1, t down
  switch (face) { case 0: return [1, -t, -s]; case 1: return [-1, -t, s]; case 2: return [s, 1, t]; case 3: return [s, -1, -t]; case 4: return [s, -t, 1]; default: return [-s, -t, -1]; }
}
function samplePhotos(photos, dir) {
  let best = 0, bv = -9; for (let i = 0; i < 6; i++) { const v = dot(BASES[i].f, dir); if (v > bv) { bv = v; best = i; } }
  const B = BASES[best], P = photos[best]; const x = dot(B.r, dir) / bv, y = dot(B.u, dir) / bv;
  const fx = (x + 1) / 2 * P.w - 0.5, fy = (1 - y) / 2 * P.h - 0.5; const ix = Math.max(0, Math.min(P.w - 1, Math.round(fx))), iy = Math.max(0, Math.min(P.h - 1, Math.round(fy)));
  const o = (iy * P.w + ix) * 3; return [P.d[o], P.d[o + 1], P.d[o + 2]];
}
function buildCube(photos, N) {
  const faces = [];
  for (let f = 0; f < 6; f++) { const d = new Float32Array(N * N * 3); for (let v = 0; v < N; v++) for (let u = 0; u < N; u++) { const c = samplePhotos(photos, texelDir(f, (u + 0.5) / N * 2 - 1, (v + 0.5) / N * 2 - 1)); const o = (v * N + u) * 3; d[o] = c[0]; d[o + 1] = c[1]; d[o + 2] = c[2]; } faces.push(d); }
  return faces;
}
function halve(d, N) { const M = Math.max(1, N >> 1), o = new Float32Array(M * M * 3); for (let y = 0; y < M; y++) for (let x = 0; x < M; x++) for (let c = 0; c < 3; c++) { let s = 0; for (const [a, b] of [[0, 0], [1, 0], [0, 1], [1, 1]]) s += d[(Math.min(N - 1, y * 2 + b) * N + Math.min(N - 1, x * 2 + a)) * 3 + c]; o[(y * M + x) * 3 + c] = s / 4; } return o; }
function toHalf(v) { if (!(v > 0)) return 0; if (v >= 65504) return 0x7bff; if (v < 6.1e-5) return Math.round(v / 5.96e-8); let e = Math.floor(Math.log2(v)); let m = v / Math.pow(2, e) - 1; let he = e + 15, hm = Math.round(m * 1024); if (hm === 1024) { hm = 0; he++; } return (he << 10) | hm; }
function makeVtf(template, faces, N, hdr) {
  let mips = 0; while ((N >> mips) > 0) mips++;
  const levels = []; let cur = faces, n = N; for (let m = 0; m < mips; m++) { levels.push({ n, faces: cur }); if (n > 1) { cur = cur.map(f => halve(f, n)); n >>= 1; } }
  const avg = [0, 0, 0]; const last = levels[levels.length - 1]; for (const f of last.faces) for (let c = 0; c < 3; c++) avg[c] += f[c] / 6;
  const hs = template.readUInt32LE(12); const bpp = hdr ? 8 : 3; let size = 0; for (const l of levels) size += l.n * l.n * bpp * 7;
  const out = Buffer.alloc(hs + size); template.copy(out, 0, 0, hs);
  out.writeUInt16LE(N, 16); out.writeUInt16LE(N, 18); out.writeUInt32LE(hdr ? 0x6000 : 0x4000, 20);
  [0, 1, 2].forEach(c => out.writeFloatLE(Math.min(1, avg[c]), 32 + c * 4));
  out.writeUInt32LE(hdr ? 24 : 2, 52); out[56] = mips;
  let p = hs;
  for (let m = mips - 1; m >= 0; m--) { const l = levels[m]; for (let f = 0; f < 7; f++) { const src = f < 6 ? l.faces[f] : null; for (let i = 0; i < l.n * l.n; i++) { const c = src ? [src[i * 3], src[i * 3 + 1], src[i * 3 + 2]] : avg; if (hdr) { out.writeUInt16LE(toHalf(c[0]), p); out.writeUInt16LE(toHalf(c[1]), p + 2); out.writeUInt16LE(toHalf(c[2]), p + 4); out.writeUInt16LE(toHalf(1), p + 6); p += 8; } else { for (let k = 0; k < 3; k++) out[p + k] = Math.max(0, Math.min(255, Math.round(255 * Math.pow(Math.max(0, c[k]), 1 / 2.2)))); p += 3; } } } }
  return out;
}

// ---- pak lump
const bspFile = process.env.CUBE_SRC ? process.env.CUBE_SRC + '/' + map + '.bsp' : fs.existsSync(ROOT + 'maps_orig/' + map + '.bsp') ? ROOT + 'maps_orig/' + map + '.bsp' : ROOT + 'maps/' + map + '.bsp';
const bsp = fs.readFileSync(bspFile);
const po = bsp.readInt32LE(8 + 40 * 16), pl = bsp.readInt32LE(12 + 40 * 16); const z = bsp.slice(po, po + pl);
let e = z.length - 22; while (e > 0 && z.readUInt32LE(e) !== 0x06054b50) e--;
const count = z.readUInt16LE(e + 10); let cp = z.readUInt32LE(e + 16); const entries = [];
for (let i = 0; i < count; i++) { const method = z.readUInt16LE(cp + 10), crc = z.readUInt32LE(cp + 16), cs = z.readUInt32LE(cp + 20), us = z.readUInt32LE(cp + 24), nl = z.readUInt16LE(cp + 28), xl = z.readUInt16LE(cp + 30), cm = z.readUInt16LE(cp + 32), lo = z.readUInt32LE(cp + 42); const name = z.toString('latin1', cp + 46, cp + 46 + nl); cp += 46 + nl + xl + cm; const h = lo + 30 + z.readUInt16LE(lo + 26) + z.readUInt16LE(lo + 28); entries.push({ name, method, crc, cs, us, data: z.slice(h, h + cs) }); }
const isCube = n => /\.vtf$/i.test(n) && new RegExp('^materials/maps/' + map + '/(c-?\\d+_-?\\d+_-?\\d+|cubemapdefault)(\\.hdr)?\\.vtf$', 'i').test(n);
const cubes = entries.filter(x => isCube(x.name)); console.log(map, 'pak files', entries.length, 'cube files', cubes.length, 'lump at', po, 'len', pl, 'file', bsp.length);

const dir = ROOT + 'out/cube/' + map + '/';
const names = fs.readdirSync(dir); const samples = [...new Set(names.filter(n => /\.pfm$/.test(n)).map(n => n.replace(/(rt|lf|bk|ft|up|dn)\.pfm$/, '')))];
console.log('captured samples', samples.length);
const template = cubes.find(x => !/hdr/i.test(x.name)).data;
const built = {}; let firstCube = null;
for (const s of samples) {
  const photos = FACES.map(f => readPfm(dir + s + f + '.pfm')); const N = photos[0].w;
  const faces = buildCube(photos, N); if (!firstCube) firstCube = faces;
  built[s] = { ldr: makeVtf(template, faces, N, false), hdr: makeVtf(template, faces, N, true) };
}
// the fallback everything uses when a map has no samples, and any cube without its own capture
const keys = Object.keys(built); const fallback = built['cubemapdefault'] || built[keys[0]];

if (COMPARE) {
  // compare against the map's existing DXT5 cube, face by face, in gamma space
  const rd = (v) => { const w = v.readUInt16LE(16), fmt = v.readUInt32LE(52); if (fmt !== 15) throw new Error('compare needs DXT5, got ' + fmt); const blocks = Math.ceil(w / 4) ** 2 * 16; const faces = []; const off = v.length - blocks * 7; for (let f = 0; f < 6; f++) { const px = []; for (let i = 0; i < Math.ceil(w / 4) ** 2; i++) { const o = off + f * blocks + i * 16 + 8; const c0 = v.readUInt16LE(o); px.push([(c0 >> 11) * 255 / 31, ((c0 >> 5) & 63) * 255 / 63, (c0 & 31) * 255 / 31]); } faces.push({ px, n: Math.ceil(w / 4) }); } return faces; };
  for (const c of cubes.filter(x => !/hdr/i.test(x.name))) { let real; try { real = rd(c.data); } catch (x) { console.log(c.name, x.message); continue; } const key = c.name.split('/').pop().replace('.vtf', ''); const mine = built[key]; if (!mine) { console.log('no capture for', key); continue; }
    const N = mine.ldr.readUInt16LE(16), hs = mine.ldr.readUInt32LE(12); let sz = 0; for (let n = N; n >= 1; n >>= 1) sz += n * n * 3 * 7; const top = hs + sz - N * N * 3 * 7;
    for (let f = 0; f < 6; f++) { const r = real[f]; let sa = 0, sb = 0, saa = 0, sbb = 0, sab = 0, cnt = 0; for (let y = 0; y < r.n; y++) for (let x = 0; x < r.n; x++) { const a = r.px[y * r.n + x]; const mx = Math.floor((x + 0.5) / r.n * N), my = Math.floor((y + 0.5) / r.n * N); const q = top + f * N * N * 3 + (my * N + mx) * 3; const av = (a[0] + a[1] + a[2]) / 3, bv = (mine.ldr[q] + mine.ldr[q + 1] + mine.ldr[q + 2]) / 3; sa += av; sb += bv; saa += av * av; sbb += bv * bv; sab += av * bv; cnt++; } const corr = (sab - sa * sb / cnt) / Math.sqrt(Math.max(1e-6, (saa - sa * sa / cnt) * (sbb - sb * sb / cnt))); console.log(key, FACES[f], 'correlation', corr.toFixed(3), 'mean shipped', (sa / cnt).toFixed(0), 'mine', (sb / cnt).toFixed(0)); } }
}

const out = []; const seen = new Set();
for (const x of entries) { seen.add(x.name); if (!isCube(x.name)) { out.push(x); continue; } const m = x.name.split('/').pop().replace(/\.vtf$/i, ''); const hdr = /\.hdr$/i.test(m); const key = m.replace(/\.hdr$/i, ''); const b = built[key] || fallback; const data = hdr ? b.hdr : b.ldr; out.push({ name: x.name, method: 0, crc: zlib.crc32(data), cs: data.length, us: data.length, data }); }
const parts = []; const cd = []; let off = 0;
for (const x of out) { const nm = Buffer.from(x.name, 'latin1'); const lh = Buffer.alloc(30); lh.writeUInt32LE(0x04034b50, 0); lh.writeUInt16LE(10, 4); lh.writeUInt16LE(0, 6); lh.writeUInt16LE(x.method, 8); lh.writeUInt32LE(x.crc, 14); lh.writeUInt32LE(x.cs, 18); lh.writeUInt32LE(x.us, 22); lh.writeUInt16LE(nm.length, 26); parts.push(lh, nm, x.data); const ch = Buffer.alloc(46); ch.writeUInt32LE(0x02014b50, 0); ch.writeUInt16LE(10, 4); ch.writeUInt16LE(10, 6); ch.writeUInt16LE(x.method, 10); ch.writeUInt32LE(x.crc, 16); ch.writeUInt32LE(x.cs, 20); ch.writeUInt32LE(x.us, 24); ch.writeUInt16LE(nm.length, 28); ch.writeUInt32LE(off, 42); cd.push(ch, nm); off += 30 + nm.length + x.data.length; }
const cdBuf = Buffer.concat(cd); const eo = Buffer.alloc(22); eo.writeUInt32LE(0x06054b50, 0); eo.writeUInt16LE(out.length, 8); eo.writeUInt16LE(out.length, 10); eo.writeUInt32LE(cdBuf.length, 12); eo.writeUInt32LE(off, 16);
const newPak = Buffer.concat([...parts, cdBuf, eo]);
const atEnd = po + pl >= bsp.length - 16; const keep = atEnd ? bsp.slice(0, po) : bsp; const pad = (4 - (keep.length % 4)) % 4;
const final = Buffer.concat([keep, Buffer.alloc(pad), newPak]); final.writeInt32LE(keep.length + pad, 8 + 40 * 16); final.writeInt32LE(newPak.length, 12 + 40 * 16);
console.log('new pak', newPak.length, 'new bsp', final.length, atEnd ? '(pak was last)' : '(appended)');
if (process.env.CUBE_OUT) { fs.mkdirSync(process.env.CUBE_OUT, { recursive: true }); fs.writeFileSync(process.env.CUBE_OUT + '/' + map + '.bsp', final); console.log('written to', process.env.CUBE_OUT); }
else if (WRITE) { fs.mkdirSync(ROOT + 'maps_orig', { recursive: true }); if (!fs.existsSync(ROOT + 'maps_orig/' + map + '.bsp')) fs.copyFileSync(ROOT + 'maps/' + map + '.bsp', ROOT + 'maps_orig/' + map + '.bsp'); fs.writeFileSync(ROOT + 'maps/' + map + '.bsp', final); fs.writeFileSync(ROOT + 'fsdata/fstop/maps/' + map + '.bsp', final); console.log('written'); }
