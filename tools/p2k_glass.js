// Project 2008's game code is closed, and its renderer only draws water reflections for world water, not for the func_water_analog seas these maps use. It does draw
// them for func_reflective_glass, so each sea gets an invisible one on the same brush, a little above the surface so the sea stays out of its own reflection. A sea that
// moves takes its glass along as a child. Safe to run again. usage: node tools/p2k_glass.js [map ...]
const fs = require('fs');
const RAISE = 2;
const maps = process.argv.slice(2);
for (const map of maps.length ? maps : ['lab_intro', 'aperture_training', 'aperture_training_island', 'island']) {
  const f = (__dirname + '/../p2k/fstop_fixes/maps/') + map + '.bsp';
  const a = fs.readFileSync(f);
  const o = a.readInt32LE(8), l = a.readInt32LE(12);
  // anything an earlier run added comes out first
  const t = a.toString('latin1', o, o + l).replace(/\0+$/, '')
    .replace(/\{[^{}]*"classname" "func_reflective_glass"[^{}]*\}\s*/g, '')
    .replace(/\{[^{}]*"classname" "func_water_analog"[^{}]*\}/g, e => e.replace('"rendermode" "10"\n', ''));
  let add = '', count = 0;
  for (const m of t.matchAll(/\{[^{}]*\}/g)) {
    const e = m[0];
    if (!/"classname" "func_water_analog"/.test(e)) continue;
    const model = /"model" "(\*\d+)"/.exec(e)[1], origin = /"origin" "([^"]+)"/.exec(e)[1].split(' ').map(Number), name = /"targetname" "([^"]+)"/.exec(e);
    origin[2] += RAISE;
    add += `{\n"classname" "func_reflective_glass"\n"model" "${model}"\n"origin" "${origin.join(' ')}"\n"Solidity" "1"\n"disableshadows" "1"\n"rendermode" "10"\n${name ? `"parentname" "${name[1]}"\n` : ''}}\n`;
    count++;
  }
  if (!count) throw new Error('no sea in ' + map);
  const lump = Buffer.from(t.replace(/\s*$/, '\n') + add + '\0', 'latin1');
  const r = Buffer.concat([a, lump]);
  r.writeInt32LE(a.length, 8); r.writeInt32LE(lump.length - 1, 12);
  fs.writeFileSync(f, r);
  console.log(map, 'sea reflections:', count);
}
