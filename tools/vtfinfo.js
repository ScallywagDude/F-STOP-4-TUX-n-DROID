// Prints the header of each VTF file given, or of every VTF under the folders given.
const fs = require('fs'), path = require('path');
const FORMATS = { 0: 'RGBA8888', 1: 'ABGR8888', 2: 'RGB888', 3: 'BGR888', 4: 'RGB565', 12: 'BGRA8888', 13: 'DXT1', 14: 'DXT3', 15: 'DXT5', 16: 'BGRX8888' };
function walk(p, out) {
  if (fs.statSync(p).isDirectory()) { for (const e of fs.readdirSync(p)) walk(path.join(p, e), out); }
  else if (p.toLowerCase().endsWith('.vtf')) out.push(p);
  return out;
}
const files = [];
for (const a of process.argv.slice(2)) walk(a, files);
for (const f of files) {
  const fd = fs.openSync(f, 'r');
  const b = Buffer.alloc(80);
  fs.readSync(fd, b, 0, 80, 0);
  fs.closeSync(fd);
  const v = b.readUInt32LE(4) + '.' + b.readUInt32LE(8);
  console.log([f.replace(/\\/g, '/'), 'v' + v, b.readUInt16LE(16) + 'x' + b.readUInt16LE(18), 'flags 0x' + b.readUInt32LE(20).toString(16), 'frames ' + b.readUInt16LE(24), FORMATS[b.readInt32LE(52)] || b.readInt32LE(52), 'mips ' + b[56], fs.statSync(f).size].join('  '));
}
