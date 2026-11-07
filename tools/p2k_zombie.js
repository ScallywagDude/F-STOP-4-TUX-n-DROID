// Project 2008's game code only ragdolls a model that has a sequence marked ACT_DIERAGDOLL, and its zombie has none, so a dead one
// stands frozen. This marks the zombie's reference sequence with that activity in the fixes folder's copy of the model. Checksums stay as they are.
const fs = require('fs');
const f = (__dirname + '/../p2k/fstop_fixes/models/zombie/new_zombie.mdl');
let h = fs.readFileSync(f);
const cs = p => { let s = ''; while (h[p]) s += String.fromCharCode(h[p++]); return s; };
const ns = h.readInt32LE(188), si = h.readInt32LE(192);
for (let i = 0; i < ns; i++) { const b = si + i * 212; console.log('seq', i, cs(b + h.readInt32LE(b + 4)), 'act', JSON.stringify(cs(b + h.readInt32LE(b + 8))), 'weight', h.readInt32LE(b + 20)); }
if (ns !== 1 || cs(si + h.readInt32LE(si + 8)) === 'ACT_DIERAGDOLL') { console.log('nothing to do'); process.exit(0); }
const at = h.length;
h = Buffer.concat([h, Buffer.from('ACT_DIERAGDOLL\0\0', 'latin1')]);
h.writeInt32LE(at - si, si + 8);
h.writeInt32LE(1, si + 20);
h.writeInt32LE(h.length, 76);
fs.writeFileSync(f, h);
console.log('marked; includes', h.readInt32LE(336), 'length', h.length);
