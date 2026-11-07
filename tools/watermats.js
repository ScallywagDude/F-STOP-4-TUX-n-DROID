// lists water materials per map and prints their settings
const fs = require('fs'); const b = require('../out/bundle.json'); const SEP = String.fromCharCode(92);
const low = {}; for (const k of Object.keys(b)) low[k.toLowerCase()] = b[k];
const seen = {};
for (const f of fs.readdirSync('maps').filter(x => x.endsWith('.bsp'))) {
  const a = fs.readFileSync('maps/' + f); const o = a.readInt32LE(8 + 43 * 16), l = a.readInt32LE(12 + 43 * 16);
  for (const t of a.slice(o, o + l).toString('latin1').split('\0')) { if (!/water|ocean|\bsea/i.test(t) || /watermelon/i.test(t)) continue; (seen[t.toLowerCase()] = seen[t.toLowerCase()] || []).push(f.replace('.bsp', '')); }
}
const handles = new Map();
function read(e) { if (e.file) return fs.readFileSync(e.file); const parts = []; if (e.pre) parts.push(Buffer.from(e.preData, 'base64')); if (e.len) { const af = e.arc === 0x7fff ? e.vpk : e.vpk.replace('_dir.vpk', '_' + String(e.arc).padStart(3, '0') + '.vpk'); let fd = handles.get(af); if (fd === undefined) { fd = fs.openSync(af, 'r'); handles.set(af, fd); } const x = Buffer.alloc(e.len); fs.readSync(fd, x, 0, e.len, e.off); parts.push(x); } return Buffer.concat(parts); }
for (const t of Object.keys(seen)) {
  const k = 'materials/' + t.split(SEP).join('/') + '.vmt'; const e = low[k];
  console.log('=== ' + t, 'maps', seen[t].length, seen[t].slice(0, 4).join(','), 'src', e && e.src);
  if (e) console.log(read(e).toString('latin1').replace(/\r/g, '').split('\n').filter(x => x.trim() && !/^\s*\/\//.test(x)).slice(0, 34).join('\n').slice(0, 1100));
}
