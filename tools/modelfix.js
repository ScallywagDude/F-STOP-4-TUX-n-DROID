// A model's .mdl, .vvd, .vtx and .phy must come from the same compile. Files were picked one by one from
// many games, so some models mix versions and render as the ERROR model. This finds them and puts the
// matching file (from any source) in tools/loose_models, which loosedata.js copies over the packed archive.
const fs = require('fs'), path = require('path');
const C = 'C:/Program Files (x86)/Steam/steamapps/common/';
const P = (process.env.P2008_DIR + '/game/');
const norm = s => s.toLowerCase().split(String.fromCharCode(92)).join('/').replace(/\/+/g, '/');
const b = require('../out/bundle.json');
const OUT = (__dirname + '/../tools/loose_models/');

function readVpkDir(file) {
  const d = fs.readFileSync(file); let p = 0, tree = 0;
  if (d.readUInt32LE(0) === 0x55aa1234) { tree = d.readUInt32LE(4) === 1 ? 12 : 28; p = tree; }
  const dataStart = tree + (tree ? d.readUInt32LE(8) : 0);
  const str = () => { const s = p; while (d[p] !== 0) p++; const r = d.toString('latin1', s, p); p++; return r; };
  const out = [];
  for (;;) { const ext = str(); if (!ext) break; for (;;) { const dir = str(); if (!dir) break; for (;;) { const name = str(); if (!name) break;
    p += 4; const pre = d.readUInt16LE(p); p += 2; const arc = d.readUInt16LE(p); p += 2; const off = d.readUInt32LE(p); p += 4; const len = d.readUInt32LE(p); p += 4; p += 2;
    const preData = pre ? Buffer.from(d.subarray(p, p + pre)).toString('base64') : null; p += pre;
    const ph = norm((dir === ' ' ? '' : dir + '/') + name + (ext === ' ' ? '' : '.' + ext));
    if (ph.startsWith('models/')) out.push({ path: ph, vpk: file, arc, off: arc === 0x7fff ? off + dataStart : off, len, pre, preData, size: len + pre });
  } } }
  return out;
}
function loose(root) {
  const out = []; if (!fs.existsSync(root + '/models')) return out;
  (function w(d) { for (const e of fs.readdirSync(d, { withFileTypes: true })) { const f = d + '/' + e.name; if (e.isDirectory()) w(f); else out.push({ path: norm(path.relative(root, f)), file: f, size: fs.statSync(f).size }); } })(root + '/models');
  return out;
}
const dirVpks = d => fs.existsSync(d) ? fs.readdirSync(d).filter(x => x.endsWith('_dir.vpk')).map(x => d + '/' + x) : [];
const handles = new Map();
function read(e) {
  if (e.file) return fs.readFileSync(e.file);
  const parts = [];
  if (e.pre) parts.push(Buffer.from(e.preData, 'base64'));
  if (e.len) {
    const af = e.arc === 0x7fff ? e.vpk : e.vpk.replace('_dir.vpk', '_' + String(e.arc).padStart(3, '0') + '.vpk');
    let fd = handles.get(af); if (fd === undefined) { fd = fs.openSync(af, 'r'); handles.set(af, fd); }
    const x = Buffer.alloc(e.len); fs.readSync(fd, x, 0, e.len, e.off); parts.push(x);
  }
  return Buffer.concat(parts);
}
const sources = [
  ['overrides', { loose: P + 'overrides' }], ['aperture', { loose: P + 'aperture' }], ['p2008', { loose: P + 'p2008' }],
  ['corehub', { loose: (__dirname + '/../corehub/portal2_tempcontent') }],
  ['portal', { vpks: [C + 'Portal/portal/portal_pak_dir.vpk'], loose: C + 'Portal/portal' }],
  ['tf', { vpks: dirVpks(C + 'Team Fortress 2/tf') }],
  ['ep2', { vpks: dirVpks(C + 'Half-Life 2/ep2'), loose: P + 'mounts/ep2' }],
  ['ep1', { vpks: dirVpks(C + 'Half-Life 2/episodic') }],
  ['hl2', { vpks: dirVpks(C + 'Portal/hl2'), loose: C + 'Portal/hl2' }],
  ['hl2b', { vpks: dirVpks(C + 'Half-Life 2/hl2') }],
  ['portal2', { vpks: dirVpks(C + 'Portal 2/portal2') }],
];
const cand = new Map();
for (const [name, s] of sources) {
  const add = e => { if (!/\.(vvd|vtx|phy)$/.test(e.path)) return; let l = cand.get(e.path); if (!l) cand.set(e.path, l = []); l.push({ src: name, ...e }); };
  for (const e of s.loose ? loose(s.loose) : []) add(e);
  for (const v of s.vpks || []) for (const e of readVpkDir(v)) add(e);
}
const sumOf = { vvd: d => d.readInt32LE(8), vtx: d => d.readInt32LE(16), phy: d => d.readInt32LE(12) };
const findMatch = (paths, kind, sum) => {
  for (const p of paths) for (const e of cand.get(p) || []) { try { if (sumOf[kind](read(e)) === sum) return e; } catch (x) {} }
  return null;
};
const put = (rel, e) => { const f = OUT + rel; fs.mkdirSync(path.dirname(f), { recursive: true }); fs.writeFileSync(f, read(e)); };

let total = 0, fixed = 0; const broken = [], done = [];
for (const k of Object.keys(b)) {
  if (!/^models\/.*\.mdl$/.test(k)) continue;
  total++;
  let sum; try { sum = read(b[k]).readInt32LE(8); } catch (x) { broken.push(k + ' unreadable mdl'); continue; }
  const base = k.replace(/\.mdl$/, '');
  const check = (rel, kind, alts) => {
    const e = b[rel];
    let ok = false, have = !!e;
    if (e) { try { ok = sumOf[kind](read(e)) === sum; } catch (x) {} }
    if (ok) return;
    const m = findMatch(alts, kind, sum);
    if (m) { put(rel, m); fixed++; done.push(rel + ' <- ' + m.path + ' (' + m.src + ')' + (have ? ' was ' + e.src : ' was absent')); }
    else if (kind !== 'phy' || have) broken.push(rel + (have ? ' mismatch, from ' + e.src + ', mdl from ' + b[k].src : ' absent, mdl from ' + b[k].src));
  };
  check(base + '.vvd', 'vvd', [base + '.vvd']);
  check(base + '.dx90.vtx', 'vtx', [base + '.dx90.vtx', base + '.vtx', base + '.dx80.vtx', base + '.sw.vtx']);
  check(base + '.phy', 'phy', [base + '.phy']);
}
console.log('models', total, 'files fixed', fixed, 'unfixable', broken.length);
console.log(done.join('\n'));
console.log('--- could not fix'); console.log(broken.join('\n'));
