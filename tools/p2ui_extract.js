// Copies menu UI files out of a local Portal 2 install into the game data.
// Usage: node p2ui_extract.js <portal2 folder> <output folder> <list file with one vpk path per line>
const fs = require('fs'), path = require('path');
const [root, out, listFile] = process.argv.slice(2);
const dir = fs.readFileSync(path.join(root, 'pak01_dir.vpk'));
let treeSize = 0, p = 0;
if (dir.readUInt32LE(0) === 0x55aa1234) { treeSize = dir.readUInt32LE(8); p = dir.readUInt32LE(4) === 2 ? 28 : 12; }
const treeEnd = p + treeSize, dataBase = treeEnd;
const str = () => { const s = p; while (dir[p] !== 0) p++; const r = dir.toString('latin1', s, p); p++; return r; };
const entries = new Map();
for (;;) {
  const ext = str(); if (!ext) break;
  for (;;) {
    const d = str(); if (!d) break;
    for (;;) {
      const n = str(); if (!n) break;
      p += 4;
      const pre = dir.readUInt16LE(p); p += 2;
      const arc = dir.readUInt16LE(p); p += 2;
      const off = dir.readUInt32LE(p); p += 4;
      const len = dir.readUInt32LE(p); p += 4;
      p += 2;
      const preOff = p; p += pre;
      entries.set((d === ' ' ? '' : d + '/') + n + '.' + ext, { pre, preOff, arc, off, len });
    }
  }
}
const chunks = new Map();
function chunk(a) {
  if (!chunks.has(a)) chunks.set(a, fs.openSync(path.join(root, 'pak01_' + String(a).padStart(3, '0') + '.vpk'), 'r'));
  return chunks.get(a);
}
const want = fs.readFileSync(listFile, 'latin1').split(/\r?\n/).map(s => s.trim().toLowerCase()).filter(Boolean);
let done = 0, missing = [];
for (const name of want) {
  const e = entries.get(name);
  if (!e) { missing.push(name); continue; }
  const buf = Buffer.alloc(e.pre + e.len);
  dir.copy(buf, 0, e.preOff, e.preOff + e.pre);
  if (e.len) {
    if (e.arc === 0x7fff) dir.copy(buf, e.pre, dataBase + e.off, dataBase + e.off + e.len);
    else fs.readSync(chunk(e.arc), buf, e.pre, e.len, e.off);
  }
  const to = path.join(out, name);
  fs.mkdirSync(path.dirname(to), { recursive: true });
  fs.writeFileSync(to, buf);
  done++;
}
console.log('extracted ' + done + ', missing ' + missing.length);
if (missing.length) console.log(missing.slice(0, 40).join('\n'));
