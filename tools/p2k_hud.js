// Project 2008's game code is closed, so its crosshair brackets go through data: the two icons become blank characters in an overriding copy of hud_textures.txt.
const fs = require('fs'), path = require('path');
const src = (process.env.P2008_DIR + '/game/core/scripts/hud_textures.txt');
const out = (__dirname + '/../p2k/fstop_fixes/scripts/hud_textures.txt');
let t = fs.readFileSync(src, 'latin1');
let n = 0;
t = t.replace(/("crosshair_(?:left|right)"\s*\{\s*"font"\s*"QuickInfo"\s*"character"\s*)"[()]"/g, (m, a) => { n++; return a + '" "'; });
if (n !== 2) throw new Error('expected two bracket icons, found ' + n);
fs.mkdirSync(path.dirname(out), { recursive: true });
fs.writeFileSync(out, t, 'latin1');
console.log('hud_textures written, brackets blanked');
