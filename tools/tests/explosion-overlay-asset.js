// Regression: every explosion fireball creates a CExplosionOverlay, which is a
// CWarpOverlay. That overlay draws the material named in warp_overlay.cpp. If the
// material is not shipped, the first frames of each explosion flash the
// missing-texture checkerboard. Usage: node tools/tests/explosion-overlay-asset.js
const fs = require('fs'), path = require('path');
const root = path.join(__dirname, '..', '..');
const BS = String.fromCharCode(92);
let fail = 0;
const bad = msg => { console.error('FAIL: ' + msg); fail++; };

const src = fs.readFileSync(path.join(root, 'engine/game/client/warp_overlay.cpp'), 'latin1');
const names = [...new Set([...src.matchAll(/(?:CLIENTEFFECT_MATERIAL|FindMaterial)\(\s*"([^"]+)"/g)].map(m => m[1]))];
if (!names.length) bad('no overlay material found in warp_overlay.cpp');

const loose = path.join(root, 'tools/loose_materials/materials');
for (const n of names) {
  const vmt = path.join(loose, n + '.vmt');
  if (!fs.existsSync(vmt)) { bad('missing ' + n + '.vmt'); continue; }
  const t = fs.readFileSync(vmt, 'latin1');
  const base = (t.match(/\$basetexture"?\s+"?([^"\r\n]+)/i) || [])[1];
  if (!base) { bad(n + '.vmt has no $basetexture'); continue; }
  const rel = base.trim().split(BS).join('/');
  const vtf = path.join(loose, rel + '.vtf');
  if (!fs.existsSync(vtf)) { bad('missing texture ' + rel + '.vtf'); continue; }
  const b = fs.readFileSync(vtf);
  if (b.toString('latin1', 0, 4) !== 'VTF' + String.fromCharCode(0)) { bad(rel + '.vtf is not a VTF'); continue; }
  console.log('ok ' + n + ' -> ' + rel + '.vtf (' + b.length + ' bytes)');
}

// Phones that already extracted DATA_VERSION 19 only pick up new loose files if it increases.
const java = fs.readFileSync(path.join(root, 'launcher/src/com/fstop/game/MainActivity.java'), 'latin1');
const dv = Number((java.match(/DATA_VERSION = (\d+)/) || [])[1]);
if (!(dv >= 20)) bad('DATA_VERSION ' + dv + ' would not re-extract new loose materials');
else console.log('ok DATA_VERSION ' + dv);

process.exit(fail ? 1 : 0);
