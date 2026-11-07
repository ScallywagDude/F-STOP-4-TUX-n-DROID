// Writes a PNG of a VTF's top mip over dark grey, using its alpha. BGRA and DXT5, one frame. usage: vtf_sheet.js in.vtf out.png [frame]
const fs = require('fs'), zlib = require('zlib');
const [inf, out, frArg] = process.argv.slice(2);
const b = fs.readFileSync(inf);
const w = b.readUInt16LE(16), h = b.readUInt16LE(18), frames = b.readUInt16LE(24), fmt = b.readInt32LE(52), fr = +frArg || 0;
const rgba = Buffer.alloc(w * h * 4);
if (fmt === 12 || fmt === 0) {
  const top = b.length - w * h * 4 * frames + fr * w * h * 4;
  for (let i = 0; i < w * h; i++) {
    rgba[i * 4] = b[top + i * 4 + (fmt === 12 ? 2 : 0)]; rgba[i * 4 + 1] = b[top + i * 4 + 1]; rgba[i * 4 + 2] = b[top + i * 4 + (fmt === 12 ? 0 : 2)]; rgba[i * 4 + 3] = b[top + i * 4 + 3];
  }
} else if (fmt === 15) {
  const size = Math.ceil(w / 4) * Math.ceil(h / 4) * 16;
  const top = b.length - size * frames + fr * size;
  const c5 = u => [(u >> 11) * 255 / 31, ((u >> 5) & 63) * 255 / 63, (u & 31) * 255 / 31];
  for (let by = 0; by < Math.ceil(h / 4); by++) for (let bx = 0; bx < Math.ceil(w / 4); bx++) {
    const o = top + (by * Math.ceil(w / 4) + bx) * 16;
    const a0 = b[o], a1 = b[o + 1];
    const al = [a0, a1];
    if (a0 > a1) for (let i = 1; i < 7; i++) al.push(((7 - i) * a0 + i * a1) / 7);
    else { for (let i = 1; i < 5; i++) al.push(((5 - i) * a0 + i * a1) / 5); al.push(0, 255); }
    let bits = 0n;
    for (let i = 0; i < 6; i++) bits |= BigInt(b[o + 2 + i]) << BigInt(8 * i);
    const u0 = b.readUInt16LE(o + 8), u1 = b.readUInt16LE(o + 10), p0 = c5(u0), p1 = c5(u1);
    const col = [p0, p1, p0.map((v, i) => (2 * v + p1[i]) / 3), p0.map((v, i) => (v + 2 * p1[i]) / 3)];
    const idx = b.readUInt32LE(o + 12);
    for (let i = 0; i < 16; i++) {
      const x = bx * 4 + (i & 3), y = by * 4 + (i >> 2);
      if (x >= w || y >= h) continue;
      const p = (y * w + x) * 4, c = col[(idx >>> (i * 2)) & 3];
      rgba[p] = c[0]; rgba[p + 1] = c[1]; rgba[p + 2] = c[2];
      rgba[p + 3] = al[Number((bits >> BigInt(i * 3)) & 7n)];
    }
  }
} else throw new Error('format ' + fmt);
const raw = Buffer.alloc((w * 3 + 1) * h);
for (let y = 0; y < h; y++) for (let x = 0; x < w; x++) {
  const p = (y * w + x) * 4, a = rgba[p + 3] / 255, q = y * (w * 3 + 1) + 1 + x * 3;
  for (let c = 0; c < 3; c++) raw[q + c] = Math.round(rgba[p + c] * a + 50 * (1 - a));
}
const T = []; for (let n = 0; n < 256; n++) { let c = n; for (let k = 0; k < 8; k++) c = c & 1 ? 0xedb88320 ^ (c >>> 1) : c >>> 1; T[n] = c >>> 0; }
const crc = d => { let c = ~0; for (const x of d) c = T[(c ^ x) & 255] ^ (c >>> 8); return (~c) >>> 0; };
const ch = (t, d) => { const l = Buffer.alloc(4); l.writeUInt32BE(d.length); const td = Buffer.concat([Buffer.from(t), d]); const c = Buffer.alloc(4); c.writeUInt32BE(crc(td)); return Buffer.concat([l, td, c]); };
const ih = Buffer.alloc(13); ih.writeUInt32BE(w, 0); ih.writeUInt32BE(h, 4); ih[8] = 8; ih[9] = 2;
fs.writeFileSync(out, Buffer.concat([Buffer.from([137, 80, 78, 71, 13, 10, 26, 10]), ch('IHDR', ih), ch('IDAT', zlib.deflateSync(raw)), ch('IEND', Buffer.alloc(0))]));
console.log(out, w, h, 'frames', frames);
