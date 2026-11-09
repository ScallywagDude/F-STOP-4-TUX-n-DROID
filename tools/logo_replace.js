// Puts the f/ mark on the loading spinner and the empty save and chapter pictures.
const fs = require('fs');
const M = 'fsdata/fstop/materials/vgui/';
const logoFile = fs.readFileSync(M + 'fstop_logo.vtf');
const LW = logoFile.readUInt16LE(16), LH = logoFile.readUInt16LE(18);
if (logoFile.readInt32LE(52) !== 0) throw new Error('expected RGBA logo');
const logoStart = logoFile.length - LW * LH * 4;
const logoA = (x, y) => logoFile[logoStart + (y * LW + x) * 4 + 3];

let minX = LW, minY = LH, maxX = 0, maxY = 0;
for (let y = 0; y < LH; y++) for (let x = 0; x < LW; x++) if (logoA(x, y) > 16) {
  if (x < minX) minX = x; if (x > maxX) maxX = x; if (y < minY) minY = y; if (y > maxY) maxY = y;
}
const BW = maxX - minX + 1, BH = maxY - minY + 1;
console.log('logo box', minX, minY, BW, BH);

// coverage of the mark at a pixel of a target box, with box filtering
function cover(px, py, boxX, boxY, boxW, boxH) {
  const sx0 = minX + (px - boxX) / boxW * BW, sx1 = minX + (px + 1 - boxX) / boxW * BW;
  const sy0 = minY + (py - boxY) / boxH * BH, sy1 = minY + (py + 1 - boxY) / boxH * BH;
  if (sx1 <= minX || sy1 <= minY || sx0 > maxX || sy0 > maxY) return 0;
  let s = 0, n = 0;
  for (let y = Math.floor(sy0); y < Math.max(Math.ceil(sy1), Math.floor(sy0) + 1); y++)
    for (let x = Math.floor(sx0); x < Math.max(Math.ceil(sx1), Math.floor(sx0) + 1); x++) {
      n++;
      if (x >= 0 && y >= 0 && x < LW && y < LH) s += logoA(x, y);
    }
  return n ? s / n / 255 : 0;
}

// ---- spinner: every frame keeps its turning ring, the middle becomes the mark
{
  const f = M + 'spinner.vtf';
  const b = fs.readFileSync(f);
  const w = b.readUInt16LE(16), h = b.readUInt16LE(18), frames = b.readUInt16LE(24), mips = b[56];
  if (b.readInt32LE(52) !== 12) throw new Error('expected BGRA spinner');
  const mipSize = m => Math.max(1, w >> m) * Math.max(1, h >> m) * 4;
  // mips run smallest to largest, each holding every frame
  const offsets = [];
  let o = b.length;
  for (let m = 0; m < mips; m++) { o -= mipSize(m) * frames; offsets[m] = o; }
  const size = 60 * (w / 128), inner = 44 * (w / 128);
  const boxH = size, boxW = size * BW / BH;
  const boxX = (w - boxW) / 2, boxY = (h - boxH) / 2;
  for (let fr = 0; fr < frames; fr++) {
    const top = offsets[0] + fr * mipSize(0);
    for (let y = 0; y < h; y++) for (let x = 0; x < w; x++) {
      const dx = x + 0.5 - w / 2, dy = y + 0.5 - h / 2;
      if (dx * dx + dy * dy >= inner * inner) continue;
      const p = top + (y * w + x) * 4;
      b[p] = 255; b[p + 1] = 255; b[p + 2] = 255;
      b[p + 3] = Math.round(cover(x, y, boxX, boxY, boxW, boxH) * 255);
    }
    for (let m = 1; m < mips; m++) {
      const mw = Math.max(1, w >> m), mh = Math.max(1, h >> m);
      const pw = Math.max(1, w >> (m - 1)), ph = Math.max(1, h >> (m - 1));
      const src = offsets[m - 1] + fr * mipSize(m - 1), dst = offsets[m] + fr * mipSize(m);
      for (let y = 0; y < mh; y++) for (let x = 0; x < mw; x++) for (let c = 0; c < 4; c++) {
        let s = 0, n = 0;
        for (let yy = y * 2; yy < Math.min(ph, y * 2 + 2); yy++) for (let xx = x * 2; xx < Math.min(pw, x * 2 + 2); xx++) { s += b[src + (yy * pw + xx) * 4 + c]; n++; }
        b[dst + (y * mw + x) * 4 + c] = Math.round(s / n);
      }
    }
  }
  fs.writeFileSync(f, b);
  console.log('spinner done', frames, 'frames', mips, 'mips');
}

// ---- empty save and chapter pictures: drawn stretched to 225x152, so the mark is narrowed to look right on screen
function picture(name, topColor, bottomColor) {
  const w = 256, h = 256;
  const head = Buffer.from(logoFile.subarray(0, 80));
  head.writeUInt16LE(w, 16); head.writeUInt16LE(h, 18);
  const data = Buffer.alloc(w * h * 4);
  const boxH = 150, boxW = boxH * BW / BH * (152 / 225);
  const boxX = (w - boxW) / 2, boxY = (h - boxH) / 2;
  for (let y = 0; y < h; y++) for (let x = 0; x < w; x++) {
    const t = y / (h - 1), c = cover(x, y, boxX, boxY, boxW, boxH), o = (y * w + x) * 4;
    for (let i = 0; i < 3; i++) {
      const bg = topColor[i] + (bottomColor[i] - topColor[i]) * t;
      data[o + i] = Math.round(bg + (240 - bg) * c);
    }
    data[o + 3] = 255;
  }
  fs.writeFileSync(M + name + '.vtf', Buffer.concat([head, data]));
  console.log(name + ' done');
}
picture('no_save_game', [30, 36, 42], [14, 18, 22]);
picture('new_save_game', [52, 62, 72], [26, 32, 38]);

// the same art is on screen in the source repository's data only, so nothing else needs the old files
const java = 'launcher/src/com/fstop/game/MainActivity.java';
const t = fs.readFileSync(java, 'latin1');
fs.writeFileSync(java, t.replace(/DATA_VERSION = (\d+)/, (m, n) => 'DATA_VERSION = ' + (Number(n) + 1)), 'latin1');
console.log('logo_replace ok');
