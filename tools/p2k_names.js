// This engine finds a model's vertex files from the name stored inside the .mdl, not from where the file sits. A model whose internal
// name points somewhere else picks up that other model's files. Rewrites the name in the fixes folder's copies to match their path.
const fs = require('fs');
const R = (__dirname + '/../p2k/fstop_fixes/models/');
let scanned = 0, fixed = 0, tooLong = [];
(function walk(d) {
  for (const e of fs.readdirSync(R + d, { withFileTypes: true })) {
    const r = d + e.name;
    if (e.isDirectory()) { walk(r + '/'); continue; }
    if (!r.toLowerCase().endsWith('.mdl')) continue;
    const h = fs.readFileSync(R + r);
    if (h.length < 80 || h.readInt32LE(0) !== 0x54534449) continue;
    scanned++;
    let s = ''; for (let p = 12; p < 76 && h[p]; p++) s += String.fromCharCode(h[p]);
    if (s.toLowerCase().split(String.fromCharCode(92)).join('/') === r.toLowerCase()) continue;
    if (r.length > 63) { tooLong.push(r); continue; }
    h.fill(0, 12, 76); h.write(r, 12, 'latin1');
    fs.writeFileSync(R + r, h);
    fixed++; if (fixed <= 12) console.log(' ', s, '->', r);
  }
})('');
console.log('models', scanned, 'names corrected', fixed, 'too long', tooLong.join(' '));
