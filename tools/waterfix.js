// A nicer water_2fort: live reflections of the sky and scene, and clear blue-green water instead of TF2's murk.
const fs = require('fs'), path = require('path');
const b = require('../out/bundle.json'); const low = {}; for (const k of Object.keys(b)) low[k.toLowerCase()] = b[k];
function read(e) { if (e.file) return fs.readFileSync(e.file); const parts = []; if (e.pre) parts.push(Buffer.from(e.preData, 'base64')); if (e.len) { const af = e.arc === 0x7fff ? e.vpk : e.vpk.replace('_dir.vpk', '_' + String(e.arc).padStart(3, '0') + '.vpk'); const fd = fs.openSync(af, 'r'); const x = Buffer.alloc(e.len); fs.readSync(fd, x, 0, e.len, e.off); fs.closeSync(fd); parts.push(x); } return Buffer.concat(parts); }
const FOG = process.argv[2] || '{24 70 84}', END = process.argv[3] || '700', AMT = process.argv[4] || '0.7';
let t = read(low['materials/water/water_2fort.vmt']).toString('latin1').replace(/\r/g, '');
t = t.replace(/"\$fogcolor"\s+"[^"]*"/, '"$fogcolor" "' + FOG + '"').replace(/"\$fogend"\s+"[^"]*"/, '"$fogend" "' + END + '"');
if (!t.includes('_rt_WaterReflection')) t = t.replace('"$refracttexture" "_rt_WaterRefraction"', '"$refracttexture" "_rt_WaterRefraction"\n\t"$reflecttexture" "_rt_WaterReflection"\n\t"$reflectamount" "' + AMT + '"\n\t"$reflecttint" "[1 1 1]"\n\t"$reflectentities" 1\n\t"$forceexpensive" 1');
const f = (__dirname + '/../tools/loose_materials/materials/water/water_2fort.vmt');
fs.mkdirSync(path.dirname(f), { recursive: true }); fs.writeFileSync(f, t);
fs.mkdirSync((__dirname + '/../fsdata/fstop/materials/water'), { recursive: true }); fs.writeFileSync((__dirname + '/../fsdata/fstop/materials/water/water_2fort.vmt'), t);
console.log(t.split('\n').filter(l => /fog|reflect|refract|force/i.test(l)).join('\n'));
