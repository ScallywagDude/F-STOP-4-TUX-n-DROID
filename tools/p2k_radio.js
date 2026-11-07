// Project 2008's game code has no song handling, so its radios stay silent. This adds entities to the fixes folder's maps
// that start the radio once the player has loaded and restart it when the song ends. The entity counts itself as playing from
// spawn, before anyone could hear it, so PlaySound alone does nothing: it has to be stopped first. Safe to run again.
const fs = require('fs');
const DIR = (__dirname + '/../p2k/fstop_fixes/maps/');
const SONG_SECONDS = 238;	// music/tiki.mp3 is 237.7 s
const ESC = String.fromCharCode(27);
const out = (target, input, delay) => [target, input, '', delay, '-1'].join(ESC);
for (const f of fs.readdirSync(DIR).filter(x => x.endsWith('.bsp'))) {
  const a = fs.readFileSync(DIR + f);
  const o = a.readInt32LE(8), l = a.readInt32LE(12);
  let t = a.toString('latin1', o, o + l).replace(/\0+$/, '');
  if (!/"targetname" "radio_sound"/.test(t) || t.includes('fstop_radio_start')) continue;
  const add = `{
"classname" "logic_auto"
"targetname" "fstop_radio_start"
"spawnflags" "0"
"origin" "0 0 0"
"OnMapSpawn" "${out('radio_sound', 'StopSound', 15)}"
"OnMapSpawn" "${out('radio_sound', 'PlaySound', 15.1)}"
}
{
"classname" "logic_timer"
"targetname" "fstop_radio_loop"
"StartDisabled" "0"
"UseRandomTime" "0"
"RefireTime" "${SONG_SECONDS}"
"spawnflags" "0"
"origin" "0 0 0"
"OnTimer" "${out('radio_sound', 'StopSound', 0)}"
"OnTimer" "${out('radio_sound', 'PlaySound', 0.1)}"
}
`;
  const lump = Buffer.from(t + add + '\0', 'latin1');
  const r = Buffer.concat([a, lump]);
  r.writeInt32LE(a.length, 8); r.writeInt32LE(lump.length - 1, 12);
  fs.writeFileSync(DIR + f, r);
  console.log('radio', f);
}
