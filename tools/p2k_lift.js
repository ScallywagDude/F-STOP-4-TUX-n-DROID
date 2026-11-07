// Project 2008's game code is closed, so its lift platform floors can't relight themselves the way the F-STOP builds do. This dims them with the
// maps' own inputs, timed from the trigger that starts the ride. Safe to run again.
const fs = require('fs');
const ESC = String.fromCharCode(27);
for (const map of ['lab_intro', 'aperture_training']) {
  const f = (__dirname + '/../p2k/fstop_fixes/maps/') + map + '.bsp';
  const a = fs.readFileSync(f);
  const o = a.readInt32LE(8), l = a.readInt32LE(12);
  const t = a.toString('latin1', o, o + l).replace(/\0+$/, '');
  if (t.includes('fstop_lift_dim')) { console.log(map, 'lift already dimmed'); continue; }
  const start = 'movelinear_platform' + ESC + 'Open' + ESC + ESC;
  const at = t.indexOf(start);
  if (at < 0) throw new Error('ride trigger not found in ' + map);
  const end = t.indexOf('}', at);
  // the platform leaves the bright set about 31 seconds after the trigger: 4 s wait, then 5376 units at 200 per second
  const steps = [[29.5, '210 210 215'], [30.5, '160 160 168'], [31.5, '110 110 118'], [32.5, '70 70 76'], [33.5, '40 40 44']];
  let add = '"fstop_lift_dim" "1"\n';
  for (const [delay, color] of steps) for (const target of ['movelinear_platform', 'platform_door'])
    add += `"OnTrigger" "${[target, 'Color', color, delay, '-1'].join(ESC)}"\n`;
  const n = t.slice(0, end) + add + t.slice(end);
  const lump = Buffer.from(n + '\0', 'latin1');
  const r = Buffer.concat([a, lump]);
  r.writeInt32LE(a.length, 8); r.writeInt32LE(lump.length - 1, 12);
  fs.writeFileSync(f, r);
  console.log(map, 'lift dimming added');
}
