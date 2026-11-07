// Appends files to an APK. Large assets and native libraries go in uncompressed; dex is deflated.
// usage: node zipadd.js <apk> <stage-dir> [asset-root]   (asset-root's contents land under assets/)
// Rewrites the zip central directory; stays within plain 32-bit zip limits, as Android requires.
const fs = require('fs'), path = require('path'), zlib = require('zlib');
const [apk, stage, assetRoot] = process.argv.slice(2);
const walk = d => fs.readdirSync(d, { withFileTypes: true }).flatMap(e => e.isDirectory() ? walk(path.join(d, e.name)) : [path.join(d, e.name)]);
const crcTable = (() => { const t = new Int32Array(256); for (let i = 0; i < 256; i++) { let c = i; for (let k = 0; k < 8; k++) c = c & 1 ? 0xedb88320 ^ (c >>> 1) : c >>> 1; t[i] = c; } return t; })();
function crcUpdate(crc, b) { for (let i = 0; i < b.length; i++) crc = crcTable[(crc ^ b[i]) & 255] ^ (crc >>> 8); return crc; }

// read existing entries
const src = fs.readFileSync(apk);
let e = src.length - 22; while (src.readUInt32LE(e) !== 0x06054b50) e--;
const count = src.readUInt16LE(e + 10); let p = src.readUInt32LE(e + 16);
const fd = fs.openSync(apk + '.tmp', 'w');
let off = 0; const cd = [];
const write = b => { fs.writeSync(fd, b); off += b.length; };
function header(name, method, crc, csize, size) {
  const n = Buffer.from(name), h = Buffer.alloc(30);
  h.writeUInt32LE(0x04034b50, 0); h.writeUInt16LE(20, 4); h.writeUInt16LE(0x0800, 6); h.writeUInt16LE(method, 8);
  h.writeUInt16LE(0, 10); h.writeUInt16LE(0x21, 12); h.writeUInt32LE(crc >>> 0, 14); h.writeUInt32LE(csize, 18); h.writeUInt32LE(size, 22); h.writeUInt16LE(n.length, 26); h.writeUInt16LE(0, 28);
  cd.push({ n, method, crc: crc >>> 0, csize, size, off });
  write(h); write(n);
}
for (let i = 0; i < count; i++) {
  const method = src.readUInt16LE(p + 10), crc = src.readUInt32LE(p + 16), cs = src.readUInt32LE(p + 20), us = src.readUInt32LE(p + 24);
  const nl = src.readUInt16LE(p + 28), xl = src.readUInt16LE(p + 30), cl = src.readUInt16LE(p + 32), lo = src.readUInt32LE(p + 42);
  const name = src.toString('utf8', p + 46, p + 46 + nl).split(String.fromCharCode(92)).join('/');
  const lnl = src.readUInt16LE(lo + 26), lxl = src.readUInt16LE(lo + 28), ds = lo + 30 + lnl + lxl;
  header(name, method, crc, cs, us); write(src.subarray(ds, ds + cs));
  p += 46 + nl + xl + cl;
}
function addFile(name, file, store) {
  const size = fs.statSync(file).size;
  if (store) {
    let crc = -1; const f = fs.openSync(file, 'r'), buf = Buffer.alloc(1 << 22); let n;
    while ((n = fs.readSync(f, buf, 0, buf.length)) > 0) crc = crcUpdate(crc, buf.subarray(0, n));
    fs.closeSync(f); crc = (crc ^ -1) >>> 0;
    header(name, 0, crc, size, size);
    const g = fs.openSync(file, 'r'); while ((n = fs.readSync(g, buf, 0, buf.length)) > 0) write(buf.subarray(0, n)); fs.closeSync(g);
  } else {
    const data = fs.readFileSync(file), crc = (crcUpdate(-1, data) ^ -1) >>> 0, z = zlib.deflateRawSync(data, { level: 9 });
    header(name, 8, crc, z.length, data.length); write(z);
  }
}
for (const f of walk(stage)) { const rel = path.relative(stage, f).split(path.sep).join('/'); addFile(rel, f, rel.startsWith('lib/')); }
if (assetRoot) for (const f of walk(assetRoot)) addFile('assets/' + path.relative(assetRoot, f).split(path.sep).join('/'), f, true);
const cdStart = off;
for (const c of cd) {
  const h = Buffer.alloc(46);
  h.writeUInt32LE(0x02014b50, 0); h.writeUInt16LE(20, 4); h.writeUInt16LE(20, 6); h.writeUInt16LE(0x0800, 8); h.writeUInt16LE(c.method, 10);
  h.writeUInt16LE(0, 12); h.writeUInt16LE(0x21, 14); h.writeUInt32LE(c.crc, 16); h.writeUInt32LE(c.csize, 20); h.writeUInt32LE(c.size, 24); h.writeUInt16LE(c.n.length, 28); h.writeUInt32LE(c.off, 42);
  write(h); write(c.n);
}
const end = Buffer.alloc(22); end.writeUInt32LE(0x06054b50, 0); end.writeUInt16LE(cd.length, 8); end.writeUInt16LE(cd.length, 10); end.writeUInt32LE(off - cdStart, 12); end.writeUInt32LE(cdStart, 16);
write(end); fs.closeSync(fd);
fs.renameSync(apk + '.tmp', apk);
console.log('apk entries', cd.length, 'bytes', off);
