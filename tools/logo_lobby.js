// Replaces the running man lobby icon with the f/ mark, dark on transparent.
const fs = require('fs');
const M = 'fsdata/fstop/materials/vgui/';
const src = fs.readFileSync(M + 'fstop_logo.vtf');
const LW = src.readUInt16LE(16), LH = src.readUInt16LE(18), start = src.length - LW * LH * 4;
const w = 64, h = 64, head = Buffer.from(src.subarray(0, 80));
head.writeUInt16LE(w, 16); head.writeUInt16LE(h, 18);
const out = Buffer.alloc(w * h * 4), step = LW / w;
for (let y = 0; y < h; y++) for (let x = 0; x < w; x++) {
  let s = 0, n = 0;
  for (let yy = Math.floor(y * step); yy < Math.floor((y + 1) * step); yy++) for (let xx = Math.floor(x * step); xx < Math.floor((x + 1) * step); xx++) { s += src[start + (yy * LW + xx) * 4 + 3]; n++; }
  const o = (y * w + x) * 4;
  out[o] = 20; out[o + 1] = 24; out[o + 2] = 28; out[o + 3] = Math.round(s / n);
}
fs.writeFileSync(M + 'icon_lobby.vtf', Buffer.concat([head, out]));
console.log('icon_lobby done');
