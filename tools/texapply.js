// Copies what texaudit.js found into tools/loose_materials, with hand-picked stand-ins where a name match isn't good enough.
const fs = require('fs'), path = require('path'); const SEP = String.fromCharCode(92);
const { plan, entryOf } = require('../out/texplan.json'); const b = require('../out/bundle.json');
const norm = s => s.toLowerCase().split(SEP).join('/').replace(/\/+/g, '/').replace(/^\//, '');
const low = {}; for (const k of Object.keys(b)) low[norm(k)] = b[k];
const P = (process.env.FSTOP_INPUT + '/Portal 2 Betas');
const OUT = (__dirname + '/../tools/loose_materials/');
const index = new Map();
const add = (p, e) => { if (!index.has(p)) index.set(p, e); };
const roots = [];
(function walk(d, depth) { let es; try { es = fs.readdirSync(d, { withFileTypes: true }); } catch (x) { return; } for (const e of es) { if (!e.isDirectory()) continue; const f = d + '/' + e.name; if (/^(materials|models)$/i.test(e.name)) { if (!roots.includes(d)) roots.push(d); continue; } if (/^(bin|platform|screenshots|maps|sound|scripts|resource|cfg|media|save|sources)$/i.test(e.name)) continue; if (depth < 6) walk(f, depth + 1); } })(P, 0);
for (const extra of [(__dirname + '/../corehub/portal2_tempcontent'), 'C:/Program Files (x86)/Steam/steamapps/common/Portal/hl2', 'C:/Program Files (x86)/Steam/steamapps/common/Portal/portal']) if (fs.existsSync(extra)) roots.push(extra);
for (const r of roots) for (const top of ['materials', 'models']) { const base = r + '/' + top; if (!fs.existsSync(base)) continue; (function w(d) { let es; try { es = fs.readdirSync(d, { withFileTypes: true }); } catch (x) { return; } for (const e of es) { const f = d + '/' + e.name; if (e.isDirectory()) w(f); else add(norm(path.relative(r, f)), { file: f }); } })(base); }
function readVpkDir(file) { const d = fs.readFileSync(file); let p = 0, tree = 0; if (d.readUInt32LE(0) === 0x55aa1234) { tree = d.readUInt32LE(4) === 1 ? 12 : 28; p = tree; } const dataStart = tree + (tree ? d.readUInt32LE(8) : 0); const str = () => { const s = p; while (d[p] !== 0) p++; const r = d.toString('latin1', s, p); p++; return r; }; for (;;) { const ext = str(); if (!ext) break; for (;;) { const dir = str(); if (!dir) break; for (;;) { const name = str(); if (!name) break; p += 4; const pre = d.readUInt16LE(p); p += 2; const arc = d.readUInt16LE(p); p += 2; const off = d.readUInt32LE(p); p += 4; const len = d.readUInt32LE(p); p += 4; p += 2; const preData = pre ? Buffer.from(d.subarray(p, p + pre)).toString('base64') : null; p += pre; const pth = norm((dir === ' ' ? '' : dir + '/') + name + (ext === ' ' ? '' : '.' + ext)); if (pth.startsWith('materials/') || pth.startsWith('models/')) add(pth, { vpk: file, arc, off: arc === 0x7fff ? off + dataStart : off, len, pre, preData }); } } } }
for (const v of ['C:/Program Files (x86)/Steam/steamapps/common/Portal/portal/portal_pak_dir.vpk', 'C:/Program Files (x86)/Steam/steamapps/common/Portal/hl2/hl2_textures_dir.vpk', 'C:/Program Files (x86)/Steam/steamapps/common/Portal/hl2/hl2_misc_dir.vpk', 'C:/Program Files (x86)/Steam/steamapps/common/Team Fortress 2/tf/tf2_textures_dir.vpk', 'C:/Program Files (x86)/Steam/steamapps/common/Team Fortress 2/tf/tf2_misc_dir.vpk', 'C:/Program Files (x86)/Steam/steamapps/common/Portal 2/portal2/pak01_dir.vpk', 'C:/Program Files (x86)/Steam/steamapps/common/Half-Life 2/hl2/hl2_textures_dir.vpk', 'C:/Program Files (x86)/Steam/steamapps/common/Half-Life 2/hl2/hl2_misc_dir.vpk', 'C:/Program Files (x86)/Steam/steamapps/common/Half-Life 2/ep2/ep2_pak_dir.vpk']) if (fs.existsSync(v)) readVpkDir(v);
const handles = new Map();
function readEntry(e) { if (e.file) return fs.readFileSync(e.file); const parts = []; if (e.pre) parts.push(Buffer.from(e.preData, 'base64')); if (e.len) { const af = e.arc === 0x7fff ? e.vpk : e.vpk.replace('_dir.vpk', '_' + String(e.arc).padStart(3, '0') + '.vpk'); let fd = handles.get(af); if (fd === undefined) { fd = fs.openSync(af, 'r'); handles.set(af, fd); } const x = Buffer.alloc(e.len); fs.readSync(fd, x, 0, e.len, e.off); parts.push(x); } return Buffer.concat(parts); }
const find = p => low[p] || index.get(p) || entryOf[p];
const put = (dest, buf) => { const f = OUT + dest; fs.mkdirSync(path.dirname(f), { recursive: true }); fs.writeFileSync(f, buf); };
const TEX = /^\$(basetexture|basetexture2|bumpmap|bumpmap2|normalmap|envmapmask|detail|selfillummask|phongexponenttexture|phongwarptexture|lightwarptexture|blendmodulatetexture|dudvmap|flowmap|envmap)$/i;
const log = { copied: 0, unresolved: new Set(), used: [] };
const seen = new Set();
function copyDeps(text) { for (const line of text.replace(/\r/g, '').split('\n')) { const l = line.replace(/\/\/.*$/, '').trim(); const m = l.match(/^"?([$%][\w]+)"?\s+"?([^"\s{][^"]*?)"?\s*$/); if (!m || !TEX.test(m[1])) continue; const v = norm(m[2].trim()).replace(/\.vtf$/, ''); if (/^(_rt_|_|env_cubemap|engine\/|$)/.test(v)) continue; const p = 'materials/' + v + '.vtf'; if (low[p] || seen.has(p)) continue; const e = index.get(p); if (e) { seen.add(p); put(p, readEntry(e)); log.copied++; } else log.unresolved.add(p); } }
function give(dest, from) { dest = norm(dest); from = norm(from); if (seen.has(dest) || low[dest]) return true; const e = find(from); if (!e) { log.unresolved.add(from); return false; } const buf = readEntry(e); seen.add(dest); put(dest, buf); log.copied++; log.used.push(dest + ' <- ' + from); if (dest.endsWith('.vmt')) copyDeps(buf.toString('latin1')); return true; }

// ---- hand-picked stand-ins, these beat a name match
const w = 'materials/wallpaper/wallpaper001a.vmt';
const palm = 'materials/models/props_island/palm_trees02.vmt';
const hand = {
  'materials/models/props_island/lambert1.vmt': palm,
  'materials/floors/carpet_001.vmt': 'materials/motel/carpetfloor003a.vmt', 'materials/floors/carpet_002.vmt': 'materials/motel/carpetfloor008a.vmt',
  'materials/floors/carpet_003.vmt': 'materials/props/carpetfloor001a.vmt', 'materials/floors/carpet_004.vmt': 'materials/props/carpetfloor003b.vmt',
  'materials/plastic/plasticceiling_portalable_01a.vmt': 'materials/props/plasticceiling001a.vmt',
  'materials/models/popup/popup_chickenfarm02/lambert1.vmt': 'materials/models/popup/popup_chickenfarm02/wood_bridge002.vmt',
  'materials/models/popup/popup_chickenfarm02/blinn1.vmt': 'materials/models/popup/popup_chickenfarm02/wood_bridge002.vmt',
  'materials/models/popup/popup_chickenfarm02/blinn2.vmt': 'materials/models/popup/popup_chickenfarm02/wood_bridge002.vmt',
  'materials/models/popup/popup_chickenfarm02/blinn3.vmt': 'materials/models/popup/popup_chickenfarm02/wood_bridge002.vmt',
  'materials/models/popup/popup_chickenfarm02/blinn4.vmt': 'materials/models/popup/popup_chickenfarm02/wood_bridge002.vmt',
  'materials/models/ghostanim/noicon.vmt': 'materials/debug/debugempty.vmt', 'materials/models/props_lab/noicon.vmt': 'materials/debug/debugempty.vmt',
  'materials/models/portals/portal1_renderfix.vmt': 'materials/debug/debugempty.vmt', 'materials/models/portals/portal2_renderfix.vmt': 'materials/debug/debugempty.vmt',
};
for (const n of ['pattern_01', 'pattern_02', 'pattern_03', 'pattern_04', 'wallpaper_01', 'wallpaper_02', 'wallpaper_03', 'wallpaper_04', 'wallpaper_06', 'wallpaper_07']) hand['materials/wallpaper/' + n + '.vmt'] = w;
const skip = /^materials\/[012]\.vmt$|\.vmt\.vmt$/;   // junk names from entity keys, not real materials
let done = 0;
for (const list of [plan.exact, plan.sameName, plan.closest]) for (const x of list) {
  if (skip.test(x.p)) continue;
  let from = hand[x.p] || x.from;
  const ph = x.p.match(/^materials\/materials\/photo\/(.+)\.vtf$/); if (ph) from = 'materials/photos/' + ph[1] + '.vtf';
  const me = x.p.match(/^materials\/materials\/(.+)$/); if (me && !ph) from = 'materials/' + me[1];
  if (x.p.endsWith('.mdl')) { const base = from.replace(/\.mdl$/, ''), dbase = x.p.replace(/\.mdl$/, ''); if (give(x.p, from)) { for (const ext of ['.vvd', '.dx90.vtx', '.phy', '.ani']) if (find(base + ext)) give(dbase + ext, base + ext); if (!find(base + '.dx90.vtx') && find(base + '.vtx')) give(dbase + '.dx90.vtx', base + '.vtx'); } continue; }
  if (give(x.p, from)) done++;
}
// textures nothing else had: nearest neighbour by name among textures we can reach
const vtfs = [];  for (const p of [...Object.keys(low), ...index.keys()]) if (p.endsWith('.vtf')) vtfs.push(p);
const tokens = s => s.replace(/\.[a-z0-9]+$/, '').split(/[\/_\-.]+/).filter(x => x && !/^(materials|models|props)$/.test(x));
const optional = /(_height|_envmask|_exponent|_ref|_dudv|_normal|_tint|_physbox)\.vtf$/;
const leftovers = [];
for (const x of plan.none) {
  if (!x.p.endsWith('.vtf')) { leftovers.push(x.p); continue; }
  if (optional.test(x.p)) { leftovers.push(x.p + ' (optional layer)'); continue; }
  const tk = tokens(x.p), dir = x.p.slice(0, x.p.lastIndexOf('/')), bn = x.p.slice(x.p.lastIndexOf('/') + 1).replace('.vtf', ''); let best = null, bs = 0;
  for (const q of vtfs) { const qt = new Set(tokens(q)); let s = 0; for (const t of tk) if (qt.has(t)) s++; if (q.startsWith(dir + '/')) s += 2; const qb = q.slice(q.lastIndexOf('/') + 1).replace('.vtf', ''); let c = 0; while (c < bn.length && c < qb.length && bn[c] === qb[c]) c++; s += c / 4; if (s > bs) { bs = s; best = q; } }
  if (best && bs >= 3) give(x.p, best); else leftovers.push(x.p);
}
// real models the code asks for by name
for (const [d, f] of [['models/npcs/hover_turret', 'models/npcs/hover_turret_old'], ['models/props_fstop/dollhouse04', 'models/props_fstop/dollhouse01']]) { if (!find(d + '.mdl') && find(f + '.mdl')) for (const ext of ['.mdl', '.vvd', '.dx90.vtx', '.phy']) if (find(f + ext)) give(d + ext, f + ext); }
console.log('files written', log.copied, 'materials placed', done);
console.log('still unresolved', [...log.unresolved].slice(0, 40).join('\n  '));
console.log('leftovers', leftovers.join('\n  '));
fs.writeFileSync('out/texapply.txt', log.used.join('\n'));

// ---- models that exist in another build, plus stand-ins for the three manual scraps
{
  const giveModel = (d, f) => { if (find(d + '.mdl')) { if (low[d + '.mdl']) return; } if (!find(f + '.mdl')) { console.log('no model', f); return; } for (const ext of ['.mdl', '.vvd', '.dx90.vtx', '.phy', '.ani']) if (find(f + ext)) give(d + ext, f + ext); if (!find(f + '.dx90.vtx') && find(f + '.vtx')) give(d + '.dx90.vtx', f + '.vtx'); const h = readEntry(find(f + '.mdl')); console.log('model', d, 'v' + h.readInt32LE(4), 'from', f); };
  giveModel('models/npcs/hover_turret', 'models/npcs/hover_turret');
  giveModel('models/props_fstop/dollhouse04', 'models/props_fstop/dollhouse04');
  for (const n of ['02', '03', '04']) giveModel('models/props_fstop/instruction_manual_scrap' + n, 'models/props_fstop/instruction_manual_scrap01');
  // the textures those models name
  for (const m of ['models/npcs/hover_turret', 'models/props_fstop/dollhouse04', 'models/props_fstop/instruction_manual_scrap01']) {
    if (!find(m + '.mdl')) continue; const h = readEntry(find(m + '.mdl')); const cs = p => { let s = ''; while (h[p]) s += String.fromCharCode(h[p++]); return s; };
    const nt = h.readInt32LE(204), to = h.readInt32LE(208), nc = h.readInt32LE(212), co = h.readInt32LE(216); const cds = []; for (let i = 0; i < nc; i++) cds.push(norm(cs(h.readInt32LE(co + i * 4))));
    for (let i = 0; i < nt; i++) { const p = to + i * 64; const t = norm(cs(p + h.readInt32LE(p))); let ok = false; for (const c of cds) { const q = norm('materials/' + c + '/' + t + '.vmt'); if (find(q)) { give(q, q); ok = true; break; } } if (!ok) console.log('  no material', m, t); }
  }
  console.log('total files now', seen.size);
}

// ---- last hand picks
{
  const force = (dest, from) => { const e = find(from); if (!e) { console.log('no source', from); return; } put(dest, readEntry(e)); console.log('stand-in', dest, '<-', from); };
  force('materials/models/props_bts/bts_shelf.vtf', 'materials/models/props/lab_shelf/lab_shelf.vtf');
  force('materials/overlays/combineburn001a.vtf', 'materials/decals/burn03a.vtf');
  force('materials/models/ghostanim/noicon.vmt', 'materials/debug/debugempty.vmt');
}
