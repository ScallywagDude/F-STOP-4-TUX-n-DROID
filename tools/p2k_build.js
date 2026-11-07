// Builds the Project 2008 fixes folder from the port's data. Nothing of P2008's is changed: the folder sits first in its search path.
// usage: node tools/p2k_build.js [--maps]       --maps also rewrites the map copies, which is slow
const fs = require('fs'), path = require('path');
const P = (process.env.P2008_DIR + '/game/');
const ROOT = (__dirname + '/../');
const OUT = ROOT + 'p2k/fstop_fixes/';
const SEP = String.fromCharCode(92);
const norm = s => s.toLowerCase().split(SEP).join('/').replace(/\/+/g, '/').replace(/^\//, '');
const WITH_MAPS = process.argv.includes('--maps');

// ---- P2008's files, first search path wins
const have = new Map();
const add = (k, v) => { k = norm(k); if (!have.has(k)) have.set(k, v); };
function vpk(file) {
  const a = fs.readFileSync(file); let p = 0, tree = 0;
  if (a.readUInt32LE(0) === 0x55aa1234) { tree = a.readUInt32LE(4) === 1 ? 12 : 28; p = tree; }
  const dataStart = tree + (tree ? a.readUInt32LE(8) : 0);
  const str = () => { const s = p; while (a[p] !== 0) p++; const r = a.toString('latin1', s, p); p++; return r; };
  for (;;) { const ext = str(); if (!ext) break; for (;;) { const dir = str(); if (!dir) break; for (;;) { const name = str(); if (!name) break;
    p += 4; const pre = a.readUInt16LE(p); p += 2; const arc = a.readUInt16LE(p); p += 2; const off = a.readUInt32LE(p); p += 4; const len = a.readUInt32LE(p); p += 4; p += 2;
    add((dir === ' ' ? '' : dir + '/') + name + (ext === ' ' ? '' : '.' + ext), { vpk: file, arc, off: arc === 0x7fff ? off + dataStart : off, len, preData: pre ? Buffer.from(a.subarray(p, p + pre)).toString('base64') : null, pre });
    p += pre;
  } } }
}
function walk(root, sub, fn) { if (!fs.existsSync(root + sub)) return; for (const e of fs.readdirSync(root + sub, { withFileTypes: true })) { const r = sub + e.name; if (e.isDirectory()) walk(root, r + '/', fn); else fn(r, root + r); } }
for (const sp of ['p2008', 'overrides', 'aperture', 'mounts/portal', 'mounts/tf', 'mounts/ep2', 'mounts/hl2', 'core', 'platform']) {
  const root = P + sp + '/';
  walk(root, '', (r, f) => { if (!/_\d+\.vpk$|_dir\.vpk$/i.test(r)) add(r, { file: f }); });
  for (const f of fs.readdirSync(root).filter(x => /_dir\.vpk$/i.test(x))) vpk(root + f);
}
function read(e) {
  if (e.file) return fs.readFileSync(e.file);
  const parts = []; if (e.pre) parts.push(Buffer.from(e.preData, 'base64'));
  if (e.len) { const af = e.arc === 0x7fff ? e.vpk : e.vpk.replace(/_dir\.vpk$/i, '_' + String(e.arc).padStart(3, '0') + '.vpk'); const fd = fs.openSync(af, 'r'); const b = Buffer.alloc(e.len); fs.readSync(fd, b, 0, e.len, e.off); fs.closeSync(fd); parts.push(b); }
  return Buffer.concat(parts);
}

// ---- the port's files: loose first, then the packed bundle
const port = new Map();
for (const r of ['tools/loose_models/', 'tools/loose_materials/', 'fsdata/fstop/']) walk(ROOT + r, '', (k, f) => { if (/^(models|materials|sound)\//i.test(k) && !port.has(norm(k))) port.set(norm(k), { file: f }); });
const b = require('../out/bundle.json'); for (const k of Object.keys(b)) if (!port.has(norm(k))) port.set(norm(k), b[k]);

const report = { models: [], kept: [], mats: 0, tex: 0, gone: [] };
const written = new Set();
function put(k, data) { if (written.has(k)) return; const f = OUT + k; fs.mkdirSync(path.dirname(f), { recursive: true }); fs.writeFileSync(f, data); written.add(k); }

// ---- models: P2008's copy stays unless it is incomplete or its files come from different compiles
const parts = ['.mdl', '.vvd', '.dx90.vtx', '.dx80.vtx', '.sw.vtx', '.phy', '.ani', '.xbox.vtx'];
const sum = (ext, d) => { if (!d || d.length < 32) return null; if (ext === '.mdl') return d.readInt32LE(8); if (ext === '.vvd') return d.readInt32LE(8); if (ext === '.phy') return d.readInt32LE(12); if (/vtx$/.test(ext)) return d.readInt32LE(16); return null; };
function setStatus(base, get) {
  const m = get(base + '.mdl'); if (!m) return 'nomdl';
  const ms = sum('.mdl', m);
  const v = get(base + '.vvd'), x = get(base + '.dx90.vtx') || get(base + '.vtx');
  if (m.length < 300) return 'bad';
  // a model with no mesh has no vvd or vtx at all
  const numBodyParts = m.readInt32LE(232);
  if (numBodyParts > 0 && (!v || !x)) return 'incomplete';
  if (v && sum('.vvd', v) !== ms) return 'mismatch';
  if (x && sum('.dx90.vtx', x) !== ms) return 'mismatch';
  const ph = get(base + '.phy'); if (ph && sum('.phy', ph) !== ms) return 'mismatch';
  return 'ok';
}
const getP = k => { const e = have.get(k); return e ? read(e) : null; };
const getF = k => { const e = port.get(k); if (!e) return null; try { return read(e); } catch (x) { return null; } };
const bases = new Set(); for (const k of port.keys()) if (/^models\/.*\.mdl$/.test(k)) bases.add(k.slice(0, -4));
// only the ones the port had to fix, or ones P2008 cannot show
const portFixed = new Set(); walk(ROOT + 'tools/loose_models/', '', (k) => { const n = norm(k); if (n.endsWith('.mdl') || /\.(vvd|phy|vtx|ani)$/.test(n)) portFixed.add(n.replace(/\.(dx90\.vtx|dx80\.vtx|sw\.vtx|xbox\.vtx|mdl|vvd|phy|ani)$/, '')); });
for (const base of bases) {
  if (!portFixed.has(base) && have.has(base + '.mdl')) continue;
  const st = setStatus(base, getP);
  if (st === 'ok' && !(base === 'models/zombie/new_zombie' && !have.has(base + '.phy'))) { report.kept.push(base); continue; }
  if (setStatus(base, getF) !== 'ok') { report.gone.push(base + ':' + st + '/port-bad'); continue; }
  for (const x of parts) { const d = getF(base + x); if (d) put(base + x, d); }
  report.models.push(base + ':' + st);
}

// ---- materials and textures that models or maps ask for and P2008 lacks
const TEX = /^\s*"?\$?(basetexture|basetexture2|bumpmap|bumpmap2|normalmap|normalmap2|envmapmask|envmapmask2|detail|detail2|selfillummask|phongexponenttexture|lightwarptexture|ambientoccltexture|blendmodulatetexture|texture2|dudvmap)"?\s+"?([^"\s]+)"?/i;
function vmtClosure(k) {
  if (have.has(k) && !written.has(k)) return true;
  if (written.has(k)) return true;
  const d = getF(k); if (!d) return false;
  put(k, d); report.mats++;
  for (const line of d.toString('latin1').split(/\r?\n/)) {
    const clean = line.replace(/\/\/.*$/, '');
    const inc = /^\s*"?include"?\s+"?([^"\s]+\.vmt)"?/i.exec(clean); if (inc) vmtClosure(norm(inc[1]));
    const m = TEX.exec(clean); if (!m) continue; const v = norm(m[2]).replace(/\.vtf$/, '');
    if (/^(_rt_|env_cubemap|engine\/|\[|\{|\d)/.test(v)) continue;
    const t = 'materials/' + v + '.vtf'; if (have.has(t) || written.has(t)) continue;
    const td = getF(t); if (td) { put(t, td); report.tex++; }
  }
  return true;
}
// everything the port added loose
walk(ROOT + 'tools/loose_materials/', '', k => { const n = norm(k); if (n.endsWith('.vmt') && !have.has(n)) vmtClosure(n); else if (n.endsWith('.vtf') && !have.has(n)) { const d = getF(n); if (d) { put(n, d); report.tex++; } } });
walk(ROOT + 'fsdata/fstop/', 'materials/', k => { const n = norm(k); if (n.endsWith('.vmt') && !have.has(n)) vmtClosure(n); else if (n.endsWith('.vtf') && !have.has(n)) { const d = getF(n); if (d) { put(n, d); report.tex++; } } });
// the audit's list
const audit = require('../out/p2k_audit.json');
for (const [k] of audit.missMat) vmtClosure(k);
for (const [v] of audit.missVtf) { const t = 'materials/' + v + '.vtf'; const d = getF(t); if (d) { put(t, d); report.tex++; } }
for (const [t] of audit.missTex) for (const k of port.keys()) if (k.endsWith('/' + t + '.vmt')) vmtClosure(k);
// maps' missing models
for (const [k] of audit.missMdl) { const base = k.slice(0, -4); if (written.has(k)) continue; for (const x of parts) { const d = getF(base + x); if (d) put(base + x, d); } report.models.push(base + ':missing'); }
// textures of every model we just added
function mdlTextures(k) { const h = fs.readFileSync(OUT + k); if (h.readInt32LE(0) !== 0x54534449) return; const cstr = p => { let s = ''; while (p < h.length && h[p]) s += String.fromCharCode(h[p++]); return s; };
  const nt = h.readInt32LE(204), to = h.readInt32LE(208), nc = h.readInt32LE(212), co = h.readInt32LE(216); const cds = []; for (let i = 0; i < nc; i++) cds.push(norm(cstr(h.readInt32LE(co + i * 4))));
  for (let i = 0; i < nt; i++) { const p = to + i * 64; const t = norm(cstr(p + h.readInt32LE(p))); let ok = cds.some(c => have.has(norm('materials/' + c + '/' + t + '.vmt')) || written.has(norm('materials/' + c + '/' + t + '.vmt'))); if (!ok) for (const c of cds) if (vmtClosure(norm('materials/' + c + '/' + t + '.vmt'))) { ok = true; break; } if (!ok) report.gone.push(k + ' tex ' + t); } }
for (const k of [...written]) if (k.endsWith('.mdl')) mdlTextures(k);

// ---- the phone build's water, replacing P2008's
const w = ROOT + 'tools/loose_materials/materials/water/water_2fort.vmt'; if (fs.existsSync(w)) { written.delete('materials/water/water_2fort.vmt'); put('materials/water/water_2fort.vmt', fs.readFileSync(w)); }

// ---- lab_intro's shoreline: definitions, and the sounds they use where P2008 lacks them
for (const n of ['soundscapes_fstop.txt', 'soundscapes_manifest.txt']) { const k = 'scripts/' + n; written.delete(k); put(k, fs.readFileSync(ROOT + 'fsdata/fstop/' + k)); }
{
  const defs = fs.readFileSync(ROOT + 'fsdata/fstop/scripts/soundscapes_fstop.txt', 'latin1');
  let added = 0, lacking = [];
  for (const m of defs.matchAll(/"wave"\s+"([^"]+)"/gi)) {
    const k = 'sound/' + norm(m[1].replace(/^[#*@<>^)(}$!?]+/, ''));
    if (have.has(k) || written.has(k)) continue;
    const d = getF(k);
    if (d) { put(k, d); added++; } else lacking.push(k);
  }
  console.log('shoreline sounds added', added, 'not found', lacking.join(' '));
}

// ---- maps
if (WITH_MAPS) {
  fs.mkdirSync(OUT + 'maps', { recursive: true });
  const blocks = t => t.split('}').map(s => s.replace(/^\s+/, '') + '}').slice(0, -1);
  for (const f of fs.readdirSync(ROOT + 'maps').filter(x => x.endsWith('.bsp'))) {
    const orig = fs.readFileSync(ROOT + 'maps_orig/' + f), theirs = fs.readFileSync(P + 'p2008/maps/' + f);
    if (orig.equals(theirs)) { fs.copyFileSync(ROOT + 'maps/' + f, OUT + 'maps/' + f); continue; }
    // P2008 changed this map's entities: carry those edits over to the fixed copy
    const L = x => x.toString('latin1', x.readInt32LE(8), x.readInt32LE(8) + x.readInt32LE(12));
    const A = blocks(L(orig)), B = new Set(blocks(L(theirs))), hid = s => (/"hammerid" "(\d+)"/.exec(s) || [])[1];
    const byId = new Map(); for (const s of B) byId.set(hid(s), s);
    const mine = fs.readFileSync(ROOT + 'maps/' + f); let text = L(mine); let edits = 0;
    for (const s of A) { if (B.has(s)) continue; const r = byId.get(hid(s)); const at = text.indexOf(s); if (at < 0) { console.log(' could not place', f, hid(s)); continue; } text = text.slice(0, at) + (r || '') + text.slice(at + s.length); edits++; }
    const lump = Buffer.from(text + '\0', 'latin1'); const o = mine.length; const out = Buffer.concat([mine, lump]); out.writeInt32LE(o, 8); out.writeInt32LE(lump.length - 1, 12);
    fs.writeFileSync(OUT + 'maps/' + f, out); console.log(' merged', f, 'entity edits', edits);
  }
}
fs.writeFileSync(ROOT + 'out/p2k_build.json', JSON.stringify(report, null, 1));
console.log('model sets added', report.models.length, 'P2008 kept', report.kept.length, 'materials', report.mats, 'textures', report.tex, 'files', written.size, 'unfilled', report.gone.length);
const g = {}; for (const m of report.models) { const s = m.split(':')[1]; g[s] = (g[s] || 0) + 1; } console.log(JSON.stringify(g));
console.log(report.models.filter(x => /zombie|ghost|magnusson|gnome|camera/.test(x)).join(' ').slice(0, 900)); console.log('unfilled:', report.gone.slice(0, 20).join(' | ').slice(0, 900));
