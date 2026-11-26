// Counts the shader named by every .vmt in a VPK (v1 or v2) and in loose material folders.
// usage: node shadercensus.js <pak_dir.vpk> [loose materials dir...]
const fs = require('fs'), path = require('path');
const counts = new Map(), sample = new Map();
function note(name, text) {
  const m = /^\s*(?:\/\/[^\n]*\n\s*)*"?([A-Za-z0-9_]+)"?/.exec(text.replace(/^﻿/, ''));
  const s = m ? m[1].toLowerCase() : '(unparsed)';
  counts.set(s, (counts.get(s) || 0) + 1); if (!sample.has(s)) sample.set(s, name);
}
function readVpk(dirFile) {
  const d = fs.readFileSync(dirFile); const ver = d.readUInt32LE(4), treeLen = d.readUInt32LE(8); const hdr = ver === 2 ? 28 : 12;
  let p = hdr; const str = () => { const s = p; while (d[p]) p++; const t = d.toString('latin1', s, p); p++; return t; };
  const base = dirFile.replace(/_dir\.vpk$/i, ''); const fds = new Map();
  const open = i => { if (!fds.has(i)) fds.set(i, fs.openSync(base + '_' + String(i).padStart(3, '0') + '.vpk', 'r')); return fds.get(i); };
  for (;;) { const ext = str(); if (!ext) break; for (;;) { const dir = str(); if (!dir) break; for (;;) { const name = str(); if (!name) break;
    p += 4; const pre = d.readUInt16LE(p), arc = d.readUInt16LE(p + 2), off = d.readUInt32LE(p + 4), len = d.readUInt32LE(p + 8); p += 14; const preData = d.slice(p, p + pre); p += pre;
    if (ext !== 'vmt') continue;
    let body = Buffer.alloc(len);
    if (len) { if (arc === 0x7fff) d.copy(body, 0, hdr + treeLen + off, hdr + treeLen + off + len); else fs.readSync(open(arc), body, 0, len, off); }
    note(dir + '/' + name + '.vmt', Buffer.concat([preData, body]).toString('latin1'));
  } } }
  for (const fd of fds.values()) fs.closeSync(fd);
}
function walk(dir) { for (const e of fs.readdirSync(dir, { withFileTypes: true })) { const f = path.join(dir, e.name); if (e.isDirectory()) walk(f); else if (/\.vmt$/i.test(e.name)) note(f, fs.readFileSync(f, 'latin1')); } }
readVpk(process.argv[2]);
for (const dir of process.argv.slice(3)) if (fs.existsSync(dir)) walk(dir);
for (const [s, c] of [...counts].sort((a, b) => b[1] - a[1])) console.log(String(c).padStart(6), s.padEnd(34), sample.get(s));
