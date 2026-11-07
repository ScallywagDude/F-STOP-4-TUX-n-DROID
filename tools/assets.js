// Works out every file F-STOP needs: walks maps -> materials/models -> textures, across all source folders.
// usage: node assets.js            (writes out/bundle.json with the chosen file for each game path)
const fs = require('fs'), path = require('path');
const C = 'C:/Program Files (x86)/Steam/steamapps/common/';
const P = (process.env.P2008_DIR + '/game/');
const OUT = (__dirname + '/../out/');
const MAPS = (__dirname + '/../maps/');
const norm = s => s.toLowerCase().split(String.fromCharCode(92)).join('/').replace(/\/+/g, '/');

function readVpkDir(file) {
  const b = fs.readFileSync(file); let p = 0, tree = 0;
  if (b.readUInt32LE(0) === 0x55aa1234) { tree = b.readUInt32LE(4) === 1 ? 12 : 28; p = tree; }
  const dataStart = tree + (tree ? b.readUInt32LE(8) : 0);
  const str = () => { const s = p; while (b[p] !== 0) p++; const r = b.toString('latin1', s, p); p++; return r; };
  const out = [];
  for (;;) { const ext = str(); if (!ext) break; for (;;) { const dir = str(); if (!dir) break; for (;;) { const name = str(); if (!name) break;
    p += 4; const pre = b.readUInt16LE(p); p += 2; const arc = b.readUInt16LE(p); p += 2; const off = b.readUInt32LE(p); p += 4; const len = b.readUInt32LE(p); p += 4; p += 2;
    const preData = pre ? Buffer.from(b.subarray(p, p + pre)).toString('base64') : null; p += pre;
    out.push({ path: norm((dir === ' ' ? '' : dir + '/') + name + (ext === ' ' ? '' : '.' + ext)), vpk: file, arc, off: arc === 0x7fff ? off + dataStart : off, len, pre, preData, size: len + pre });
  } } }
  return out;
}
function loose(root) {
  const out = []; if (!fs.existsSync(root)) return out;
  (function w(d) { for (const e of fs.readdirSync(d, { withFileTypes: true })) { const f = d + '/' + e.name; if (e.isDirectory()) w(f); else out.push({ path: norm(path.relative(root, f)), file: f, size: fs.statSync(f).size }); } })(root);
  return out;
}
const dirVpks = d => fs.existsSync(d) ? fs.readdirSync(d).filter(x => x.endsWith('_dir.vpk')).map(x => d + '/' + x) : [];
// Same priority order as the Project 2008 mount list, then the extras.
const sources = [
  ['overrides', { loose: P + 'overrides' }],
  ['aperture', { loose: P + 'aperture' }],
  ['p2008', { loose: P + 'p2008' }],
  ['corehub', { loose: (__dirname + '/../corehub/portal2_tempcontent') }],
  ['portal', { vpks: [C + 'Portal/portal/portal_pak_dir.vpk'], loose: C + 'Portal/portal' }],
  ['tf', { vpks: dirVpks(C + 'Team Fortress 2/tf') }],
  ['ep2', { vpks: dirVpks(C + 'Half-Life 2/ep2'), loose: P + 'mounts/ep2' }],
  ['ep1', { vpks: dirVpks(C + 'Half-Life 2/episodic') }],
  ['hl2', { vpks: dirVpks(C + 'Portal/hl2'), loose: C + 'Portal/hl2' }],
  ['hl2b', { vpks: dirVpks(C + 'Half-Life 2/hl2') }],
  ['portal2', { vpks: dirVpks(C + 'Portal 2/portal2') }],
];
const skipLoose = /^(maps|bin|shaders|save|cfg\/config|media|downloadlists|screenshots)\b|\.(cache|log|dem|tmp)$/;
const index = new Map();
const coreArt = /^materials\/(sprites|effects|particle|engine|dev|debug|cable|shadertest|editor|tools)\//;
const late = [];
const stats = {};
for (const [name, s] of sources) {
  let n = 0;
  // Other games only lend art: their configs and scripts must never replace the base game's.
  const artOnly = { tf: /^(materials|models)\//, ep1: /^(materials|models|sound)\//, ep2: /^(materials|models|sound)\//, hl2b: /^(materials|models|sound)\//, portal2: /^(materials|models)\// }[name];
  const noUiArt = /^materials\/(console|vgui|hud|hlmv|gamepadui)\//;
  const add = e => { if (artOnly && (!artOnly.test(e.path) || noUiArt.test(e.path))) return;
    if (artOnly && coreArt.test(e.path) && !index.has(e.path)) { late.push({ src: name, ...e }); return; }
    if (name === 'overrides' && (/^resource\//.test(e.path) || /^scripts\/game_sounds_(manifest|weapons_aperture)/.test(e.path))) return; if (!index.has(e.path)) index.set(e.path, { src: name, ...e }); n++; };
  for (const e of s.loose ? loose(s.loose) : []) if (!skipLoose.test(e.path) && !/_\d\d\d\.vpk$|_dir\.vpk$|\.vpk$/.test(e.path)) add(e);
  for (const v of s.vpks || []) for (const e of readVpkDir(v)) add(e);
  stats[name] = n;
}
for (const e of late) if (!index.has(e.path)) index.set(e.path, e);
console.log('indexed', stats, 'unique', index.size);

const handles = new Map();
function read(e) {
  if (e.file) return fs.readFileSync(e.file);
  const parts = [];
  if (e.pre) parts.push(Buffer.from(e.preData, 'base64'));
  if (e.len) {
    const af = e.arc === 0x7fff ? e.vpk : e.vpk.replace('_dir.vpk', '_' + String(e.arc).padStart(3, '0') + '.vpk');
    let fd = handles.get(af); if (fd === undefined) { fd = fs.openSync(af, 'r'); handles.set(af, fd); }
    const b = Buffer.alloc(e.len); fs.readSync(fd, b, 0, e.len, e.off); parts.push(b);
  }
  return Buffer.concat(parts);
}
const pak = {}; // files embedded in maps
function pakEntries(b) {
  const out = []; let i = b.length - 22; while (i >= 0 && b.readUInt32LE(i) !== 0x06054b50) i--; if (i < 0) return out;
  const n = b.readUInt16LE(i + 10); let p = b.readUInt32LE(i + 16);
  for (let k = 0; k < n; k++) { if (b.readUInt32LE(p) !== 0x02014b50) break;
    const method = b.readUInt16LE(p + 10), cs = b.readUInt32LE(p + 20), fl = b.readUInt16LE(p + 28), el = b.readUInt16LE(p + 30), cl = b.readUInt16LE(p + 32), lo = b.readUInt32LE(p + 42);
    const name = norm(b.toString('latin1', p + 46, p + 46 + fl));
    const lnl = b.readUInt16LE(lo + 26), lxl = b.readUInt16LE(lo + 28), ds = lo + 30 + lnl + lxl;
    out.push({ name, method, data: method === 0 ? b.subarray(ds, ds + cs) : null });
    p += 46 + fl + el + cl; }
  return out;
}
const want = new Map(), missing = new Map(), queue = [];
const OTHER_LANG = /_(brazilian|bulgarian|czech|danish|dutch|finnish|french|german|greek|hungarian|italian|japanese|korean|koreana|latam|norwegian|polish|portuguese|romanian|russian|schinese|spanish|swedish|tchinese|thai|turkish|ukrainian|vietnamese|pirate)\.(txt|dat)$/;
function need(p, from) {
  p = norm(p);
  if (OTHER_LANG.test(p)) return false;
  if (want.has(p) || missing.has(p)) return true;
  if (pak[p]) { want.set(p, 'pak'); if (p.endsWith('.vmt')) queue.push(p); return true; }
  const e = index.get(p);
  if (!e) { missing.set(p, from); return false; }
  want.set(p, e.src); queue.push(p); return true;
}
function needMat(name, from) { // material name without materials/ or .vmt
  name = norm(name).replace(/^materials\//, '').replace(/\.vmt$/, '');
  return need('materials/' + name + '.vmt', from);
}
function needModel(p, from) {
  p = norm(p);
  if (!need(p, from)) return;
  const base = p.slice(0, -4);
  for (const x of ['.vvd', '.dx90.vtx', '.phy', '.ani', '.vtx']) { const f = base + x; if (index.has(f)) need(f, p); }
  if (!index.has(base + '.dx90.vtx')) for (const x of ['.dx80.vtx', '.sw.vtx']) if (index.has(base + x)) need(base + x, p);
}
function scanModel(p) {
  let b; try { b = read(index.get(p)); } catch (e) { return; }
  if (b.toString('latin1', 0, 4) !== 'IDST') return;
  const cstr = o => { let e = o; while (e < b.length && b[e]) e++; return b.toString('latin1', o, e); };
  try {
    const numT = b.readInt32LE(204), ti = b.readInt32LE(208), numC = b.readInt32LE(212), ci = b.readInt32LE(216);
    const cds = []; for (let i = 0; i < numC; i++) cds.push(norm(cstr(b.readInt32LE(ci + i * 4))));
    for (let i = 0; i < numT; i++) {
      const e = ti + i * 64; const nm = norm(cstr(e + b.readInt32LE(e)));
      let ok = false;
      for (const c of cds) { const cd = c.replace(/^\/+|\/+$/g, ''); const m = 'materials/' + (cd ? cd + '/' : '') + nm + '.vmt'; if (pak[m] || index.has(m)) { need(m, p); ok = true; break; } }
      if (!ok && cds.length) missing.set('materials/' + cds[0].replace(/^\/+|\/+$/g, '') + '/' + nm + '.vmt', p);
    }
    const numI = b.readInt32LE(336), ii = b.readInt32LE(340);
    for (let i = 0; i < numI; i++) { const e = ii + i * 8; const inc = cstr(e + b.readInt32LE(e + 4)); if (inc) needModel(inc, p); }
  } catch (e) { /* unusual header */ }
}
function scanVmt(p) {
  let t;
  if (pak[p]) t = pak[p].toString('latin1'); else { try { t = read(index.get(p)).toString('latin1'); } catch (e) { return; } }
  for (const m of t.matchAll(/"?include"?\s+"([^"]+)"/gi)) need(m[1], p);
  for (const m of t.matchAll(/"?(\$[a-z0-9_]+|%[a-z0-9_]+)"?\s+"?([^"\s{}]+)"?/gi)) {
    const v = norm(m[2]); if (/^[\d.\-\[{]/.test(v) || v === 'env_cubemap') continue;
    const tex = 'materials/' + v.replace(/\.vtf$/, '');
    if (index.has(tex + '.vtf') || pak[tex + '.vtf']) need(tex + '.vtf', p);
    if (index.has(tex + '.hdr.vtf')) need(tex + '.hdr.vtf', p);
    if (/fallback|material$/.test(m[1].toLowerCase()) && (index.has(tex + '.vmt'))) need(tex + '.vmt', p);
  }
}
// ---------- whole-folder includes ----------
const keepAll = e => {
  if (/^sound\/commentary\//.test(e.path)) return false;
  if (e.src === 'aperture' && e.path === 'scenes/scenes.image') return false;
  if (/\.(vtx)$/.test(e.path) && !/\.dx90\.vtx$/.test(e.path) && index.has(e.path.replace(/\.(dx80|sw|xbox|360)\.vtx$/, '.dx90.vtx'))) return false;
  if (/\.360\.|\.xbox\.|^maps\/|^bin\/|^shaders\//.test(e.path)) return false;
  return true;
};
for (const [p, e] of index) {
  if (e.src === 'aperture' || e.src === 'overrides' || e.src === 'corehub') { if (keepAll(e)) need(p, 'all:' + e.src); }
  else if (e.src === 'portal') { if (keepAll(e) && !/^sound\/vo\/.*_(?:fr|de|ru|es)\//.test(p)) need(p, 'all:portal'); }
  else if (e.src === 'hl2' && !/^materials\//.test(p) && !/^sound\/vo\//.test(p)) { if (keepAll(e) && (e.file ? /^(resource|scripts|cfg)\//.test(p) : /hl2_(misc|sound_misc)/.test(e.vpk))) need(p, 'all:hl2'); }
  else if (e.src === 'hl2' && /^materials\/(console|vgui|engine|dev|debug|effects|particle|sprites|hud|cable|decals|shadertest|detail|overlays|editor|tools)\//.test(p)) need(p, 'all:hl2mat');
  else if (/^(resource|scripts|cfg)\//.test(p) && e.src === 'hl2') need(p, 'all:hl2cfg');
}
for (const [p, e] of index) if (/^sound\/vo\/(k_lab|k_lab2|eli_lab|magnus|outland_12a\/launch)/.test(p)) need(p, 'vo');
// ---------- maps ----------
const ents = {}; const classes = {};
for (const f of fs.readdirSync(MAPS).filter(x => x.endsWith('.bsp'))) {
  const b = fs.readFileSync(MAPS + f);
  const lump = i => { const o = 8 + i * 16; return { off: b.readInt32LE(o), len: b.readInt32LE(o + 4) }; };
  const pk = lump(40);
  if (pk.len > 0) for (const e of pakEntries(b.subarray(pk.off, pk.off + pk.len))) if (e.data && !pak[e.name]) pak[e.name] = e.data;
}
for (const f of fs.readdirSync(MAPS).filter(x => x.endsWith('.bsp'))) {
  const b = fs.readFileSync(MAPS + f); const from = f;
  const lump = i => { const o = 8 + i * 16; return { off: b.readInt32LE(o), len: b.readInt32LE(o + 4) }; };
  const sd = lump(43); for (const s of b.toString('latin1', sd.off, sd.off + sd.len).split('\0')) if (s) needMat(s, from);
  const ent = b.toString('latin1', lump(0).off, lump(0).off + lump(0).len);
  for (const m of ent.matchAll(/"([^"]+)"\s+"([^"]*)"/g)) {
    const k = m[1].toLowerCase(), v = norm(m[2]);
    if (k === 'skyname' && v) for (const s of ['ft', 'bk', 'lf', 'rt', 'up', 'dn']) { needMat('skybox/' + v + s, from); if (index.has('materials/skybox/' + v + '_hdr' + s + '.vmt')) needMat('skybox/' + v + '_hdr' + s, from); if (index.has('materials/skybox/' + v + 'hdr' + s + '.vmt')) needMat('skybox/' + v + 'hdr' + s, from); }
    if (k === 'detailmaterial' && v) needMat(v, from);
    if (/\.mdl$/.test(v) && !v.startsWith('*')) needModel(v, from + ':ent');
    else if (/\.(vmt|spr)$/.test(v)) needMat(v.replace(/\.spr$/, ''), from + ':ent');
    else if (/\.(wav|mp3)$/.test(v)) { const s = 'sound/' + v.replace(/^[#*@$^)(}{<>!?&~+%-]+/, ''); if (index.has(s)) need(s, from); }
    else if (/\.vcd$/.test(v)) { const s = v.startsWith('scenes/') ? v : 'scenes/' + v; if (index.has(s)) need(s, from); }
  }
  const gl = lump(35); if (gl.len > 0) { const cnt = b.readInt32LE(gl.off);
    for (let i = 0; i < cnt; i++) { const o = gl.off + 4 + i * 16; if (b.toString('latin1', o, o + 4) === 'prps') { let p = b.readInt32LE(o + 8); const nd = b.readInt32LE(p); p += 4; for (let k = 0; k < nd; k++) { const nm = b.toString('latin1', p, p + 128).split('\0')[0]; if (nm) needModel(nm, from + ':prop'); p += 128; } } } }
}
for (const [p] of Object.entries(pak)) if (p.endsWith('.vmt')) { want.set(p, 'pak'); queue.push(p); }
// ---------- models named in the game code ----------
(function w(d) { for (const e of fs.readdirSync(d, { withFileTypes: true })) { const f = d + '/' + e.name; if (e.isDirectory()) w(f); else if (/\.(cpp|h)$/.test(f)) {
  const t = fs.readFileSync(f, 'latin1');
  for (const m of t.matchAll(/"([\w\/\\.\-]+\.mdl)"/g)) needModel(m[1], 'code');
  for (const m of t.matchAll(/"([\w\/\\.\-]+\.vmt)"/g)) needMat(m[1], 'code');
} } })((__dirname + '/../engine/game/server/portal2'));
// ---------- follow references ----------
for (const [p] of [...want]) if (/\.mdl$/.test(p)) needModel(p, 'all');
while (queue.length) { const p = queue.pop();
  if (p.endsWith('.vmt')) scanVmt(p);
  else if (p.endsWith('.mdl')) { needModel(p, 'dep'); scanModel(p); }
}
// ---------- report ----------
const by = {}; let total = 0; const list = {};
for (const [p, s] of want) { if (s === 'pak') continue; const e = index.get(p); by[e.src] = by[e.src] || { n: 0, mb: 0 }; by[e.src].n++; by[e.src].mb += e.size / 1048576; total += e.size; list[p] = e; }
for (const k of Object.keys(by)) by[k].mb = Math.round(by[k].mb);
console.log('bundle files', Object.keys(list).length, 'MB', Math.round(total / 1048576), by);
const mm = [...missing].filter(([p]) => !/^models\/.*\.(vvd|phy)$/.test(p));
const missModels = mm.filter(([p]) => p.endsWith('.mdl')), missMats = mm.filter(([p]) => p.endsWith('.vmt'));
console.log('missing models', missModels.length, missModels.slice(0, 40).map(x => x[0]).join(' '));
console.log('missing materials', missMats.length, missMats.slice(0, 60).map(x => x[0].replace('materials/', '')).join(' '));
const top = {}; for (const [p, e] of Object.entries(list)) { const d = e.src + ':' + p.split('/').slice(0, 2).join('/'); top[d] = (top[d] || 0) + e.size; }
console.log(Object.entries(top).sort((a, b) => b[1] - a[1]).slice(0, 26).map(([k, v]) => k + '=' + Math.round(v / 1048576)).join(' '));
fs.writeFileSync(OUT + 'bundle.json', JSON.stringify(list));
fs.writeFileSync(OUT + 'missing_assets.txt', mm.map(([p, f]) => p + '  <- ' + f).join('\n'));
