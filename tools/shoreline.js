// lab_intro: Lost Coast's shoreline soundscape over the island, ending when the player arrives on the platform and Cave starts talking.
// Adds the soundscape definitions to the game's data, and two env_soundscape entities and the switch between them to the map. usage: node tools/shoreline.js
const fs = require('fs'), path = require('path');
const ROOT = (__dirname + '/../');
const ESC = String.fromCharCode(27);

// ---- definitions: the five coast blocks, as Half-Life 2 has them, and a silent one to switch to
const blocks = fs.readFileSync(ROOT + 'out/shoreline_blocks.txt', 'latin1');
const defs = blocks + '\r\n\r\n"fstop.silence"\r\n{\r\n\t"dsp"\t"1"\r\n}\r\n';

// ---- the manifest the game already has, plus this file
function vpkRead(dirFile, wanted) {
  const a = fs.readFileSync(dirFile); let p = 0, tree = 0;
  if (a.readUInt32LE(0) === 0x55aa1234) { tree = a.readUInt32LE(4) === 1 ? 12 : 28; p = tree; }
  const ds = tree + (tree ? a.readUInt32LE(8) : 0);
  const str = () => { const s = p; while (a[p] !== 0) p++; const r = a.toString('latin1', s, p); p++; return r; };
  for (;;) { const ext = str(); if (!ext) break; for (;;) { const dir = str(); if (!dir) break; for (;;) { const name = str(); if (!name) break;
    p += 4; const pre = a.readUInt16LE(p); p += 2; const arc = a.readUInt16LE(p); p += 2; const off = a.readUInt32LE(p); p += 4; const len = a.readUInt32LE(p); p += 6; const preOff = p; p += pre;
    if ((dir === ' ' ? '' : dir + '/') + name + (ext === ' ' ? '' : '.' + ext) === wanted) {
      const parts = [a.subarray(preOff, preOff + pre)];
      if (len) { if (arc === 0x7fff) parts.push(a.subarray(ds + off, ds + off + len)); else { const af = dirFile.replace(/_dir\.vpk$/i, '_' + String(arc).padStart(3, '0') + '.vpk'); const fd = fs.openSync(af, 'r'); const b = Buffer.alloc(len); fs.readSync(fd, b, 0, len, off); fs.closeSync(fd); parts.push(b); } }
      return Buffer.concat(parts).toString('latin1');
    }
  } } }
  throw new Error('not in vpk: ' + wanted);
}
const manifestPath = ROOT + 'fsdata/fstop/scripts/soundscapes_manifest.txt';
let manifest = fs.existsSync(manifestPath) ? fs.readFileSync(manifestPath, 'latin1') : vpkRead(ROOT + 'fsdata/fstop/fstop_pak_dir.vpk', 'scripts/soundscapes_manifest.txt');
if (!manifest.includes('soundscapes_fstop')) {
  const at = manifest.lastIndexOf('}');
  manifest = manifest.slice(0, at) + '\t"file"\t\t"scripts/soundscapes_fstop.txt"\r\n' + manifest.slice(at);
}
fs.mkdirSync(path.dirname(manifestPath), { recursive: true });
fs.writeFileSync(manifestPath, manifest, 'latin1');
fs.writeFileSync(ROOT + 'fsdata/fstop/scripts/soundscapes_fstop.txt', defs, 'latin1');
console.log(manifest.replace(/\r/g, ''));

// ---- the map
const f = ROOT + 'maps/lab_intro.bsp';
const a = fs.readFileSync(f);
const o = a.readInt32LE(8), l = a.readInt32LE(12);
let t = a.toString('latin1', o, o + l).replace(/\0+$/, '');
if (t.includes('fstop_shoreline')) { console.log('map already done'); process.exit(0); }
const named = /"targetname" "rv_intro_trigger"/.exec(t);
if (!named) throw new Error('rv_intro_trigger not found');
const end = t.indexOf('}', named.index);
const out = (target, input) => `"OnTrigger" "${[target, input, '', 0, '-1'].join(ESC)}"\n`;
t = t.slice(0, end) + out('fstop_shoreline', 'Disable') + out('fstop_silence', 'Enable') + t.slice(end);
t += `{
"classname" "env_soundscape"
"targetname" "fstop_shoreline"
"origin" "9504 7520 10300"
"soundscape" "coast.general_shoreline"
"radius" "6000"
"StartDisabled" "0"
}
{
"classname" "env_soundscape"
"targetname" "fstop_silence"
"origin" "-8416 -3568 -2600"
"soundscape" "fstop.silence"
"radius" "900"
"StartDisabled" "1"
}
`;
const lump = Buffer.from(t + '\0', 'latin1');
const r = Buffer.concat([a, lump]);
r.writeInt32LE(a.length, 8); r.writeInt32LE(lump.length - 1, 12);
fs.writeFileSync(f, r);
fs.copyFileSync(f, ROOT + 'fsdata/fstop/maps/lab_intro.bsp');
console.log('map written', r.length);
