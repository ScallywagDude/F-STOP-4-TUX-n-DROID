// Packs out/bundle.json into a chunked VPK (v1) plus the loose files the game needs.
// usage: node makevpk.js <output-root>     creates <root>/fstop/{gameinfo.txt,fstop_pak_dir.vpk,fstop_pak_NNN.vpk,maps/...} and <root>/platform
const fs = require('fs'), path = require('path');
const root = process.argv[2];
const list = JSON.parse(fs.readFileSync((__dirname + '/../out/bundle.json'), 'latin1'));
const CHUNK = 256 * 1024 * 1024;
const G = root + '/fstop';
fs.mkdirSync(G + '/maps', { recursive: true });

const crcT = new Int32Array(256); for (let i = 0; i < 256; i++) { let c = i; for (let k = 0; k < 8; k++) c = c & 1 ? 0xedb88320 ^ (c >>> 1) : c >>> 1; crcT[i] = c; }
const crc32 = b => { let c = -1; for (let i = 0; i < b.length; i++) c = crcT[(c ^ b[i]) & 255] ^ (c >>> 8); return (c ^ -1) >>> 0; };

const handles = new Map();
function read(e) {
  if (e.file) return fs.readFileSync(e.file);
  const parts = [];
  if (e.pre) parts.push(Buffer.from(e.preData, 'base64'));
  if (e.len) {
    const af = e.arc === 0x7fff ? e.vpk : e.vpk.replace('_dir.vpk', '_' + String(e.arc).padStart(3, '0') + '.vpk');
    let fd = handles.get(af); if (fd === undefined) { fd = fs.openSync(af, 'r'); handles.set(af, fd); }
    const b = Buffer.alloc(e.len); fs.readSync(fd, b, 0, e.len, e.off); parts.push(b);
  }
  return Buffer.concat(parts);
}
// tree: ext -> dir -> [ {name, crc, chunk, off, len} ]
const tree = new Map();
let chunk = 0, chunkFd = null, chunkOff = 0, total = 0, count = 0;
const openChunk = () => { if (chunkFd !== null) fs.closeSync(chunkFd); chunkFd = fs.openSync(`${G}/fstop_pak_${String(chunk).padStart(3, '0')}.vpk`, 'w'); chunkOff = 0; };
openChunk();
for (const p of Object.keys(list).sort()) {
  const data = read(list[p]);
  const dot = p.lastIndexOf('.'), slash = p.lastIndexOf('/');
  const ext = dot > slash ? p.slice(dot + 1) : ' ';
  const dir = slash >= 0 ? p.slice(0, slash) : ' ';
  const name = p.slice(slash + 1, dot > slash ? dot : undefined);
  if (chunkOff + data.length > CHUNK && chunkOff > 0) { chunk++; openChunk(); }
  fs.writeSync(chunkFd, data);
  if (!tree.has(ext)) tree.set(ext, new Map());
  const dm = tree.get(ext); if (!dm.has(dir)) dm.set(dir, []);
  dm.get(dir).push({ name, crc: crc32(data), chunk, off: chunkOff, len: data.length });
  chunkOff += data.length; total += data.length; count++;
  if (count % 4000 === 0) console.log(count, 'files', Math.round(total / 1048576), 'MB');
}
fs.closeSync(chunkFd);
const parts = [];
const cs = s => Buffer.from(s + '\0', 'latin1');
for (const [ext, dm] of tree) {
  parts.push(cs(ext));
  for (const [dir, files] of dm) {
    parts.push(cs(dir));
    for (const f of files) {
      parts.push(cs(f.name));
      const h = Buffer.alloc(18);
      h.writeUInt32LE(f.crc, 0); h.writeUInt16LE(0, 4); h.writeUInt16LE(f.chunk, 6); h.writeUInt32LE(f.off, 8); h.writeUInt32LE(f.len, 12); h.writeUInt16LE(0xffff, 16);
      parts.push(h);
    }
    parts.push(Buffer.from([0]));
  }
  parts.push(Buffer.from([0]));
}
parts.push(Buffer.from([0]));
const body = Buffer.concat(parts);
const hdr = Buffer.alloc(12); hdr.writeUInt32LE(0x55aa1234, 0); hdr.writeUInt32LE(1, 4); hdr.writeUInt32LE(body.length, 8);
fs.writeFileSync(`${G}/fstop_pak_dir.vpk`, Buffer.concat([hdr, body]));
console.log('vpk: files', count, 'MB', Math.round(total / 1048576), 'chunks', chunk + 1, 'dir KB', Math.round(body.length / 1024));
