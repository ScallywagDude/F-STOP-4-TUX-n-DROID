// Finds every material and texture the game asks for that the bundle lacks, then looks for each one in every folder
// we have: exact path first, then the same file name elsewhere, then the closest name. Writes out/texplan.json.
const fs = require('fs'), path = require('path');
const b = require('../out/bundle.json'); const SEP = String.fromCharCode(92);
const norm = s => s.toLowerCase().split(SEP).join('/').replace(/\/+/g, '/').replace(/^\//, '');
const low = {}; for (const k of Object.keys(b)) low[norm(k)] = b[k];
const handles = new Map();
function read(e) { if (e.file) return fs.readFileSync(e.file); const parts = []; if (e.pre) parts.push(Buffer.from(e.preData, 'base64')); if (e.len) { const af = e.arc === 0x7fff ? e.vpk : e.vpk.replace('_dir.vpk', '_' + String(e.arc).padStart(3, '0') + '.vpk'); let fd = handles.get(af); if (fd === undefined) { fd = fs.openSync(af, 'r'); handles.set(af, fd); } const x = Buffer.alloc(e.len); fs.readSync(fd, x, 0, e.len, e.off); parts.push(x); } return Buffer.concat(parts); }

// ---- what is wanted
const want = new Map(); // path -> reason
const need = (p, why) => { p = norm(p); if (!low[p] && !want.has(p)) want.set(p, why); };
const TEXKEYS = /^\$(basetexture|basetexture2|bumpmap|bumpmap2|normalmap|normalmap2|envmapmask|envmapmask2|detail|detail2|selfillummask|phongexponenttexture|phongwarptexture|lightwarptexture|blendmodulatetexture|refracttinttexture|dudvmap|flowmap|tintmasktexture|ssbump|parallaxmap|iris|corneatexture|ambientoccltexture|envmap|texture2|maskstexture|compress|stretch|bumpcompress|bumpstretch)$/i;
const skipTex = v => /^(_rt_|_|env_cubemap|engine\/|shadertest\/|vgui\/|$)/i.test(v) || /^\d|^\[|^\{|^env_/.test(v);
function vmtRefs(text) { const out = []; for (const line of text.replace(/\r/g, '').split('\n')) { const l = line.replace(/\/\/.*$/, '').trim(); const m = l.match(/^"?([$%][\w]+)"?\s+"?([^"\s{][^"]*?)"?\s*$/); if (m && TEXKEYS.test(m[1]) && !skipTex(m[2].trim())) out.push(m[2].trim().replace(/\.vtf$/i, '')); const inc = l.match(/^"?include"?\s+"([^"]+)"/i); if (inc) out.push('INC:' + inc[1]); } return out; }
for (const k of Object.keys(low)) { const e = low[k]; if (!k.endsWith('.vmt')) continue; let t; try { t = read(e).toString('latin1'); } catch (x) { continue; } for (const r of vmtRefs(t)) { if (r.startsWith('INC:')) need(r.slice(4), 'include in ' + k); else need('materials/' + r + '.vtf', 'used by ' + k); } }
// models: a texture is missing only if no search folder holds it
let mdlCount = 0;
for (const k of Object.keys(low)) {
  if (!/^models\/.*\.mdl$/.test(k)) continue; let h; try { h = read(low[k]); } catch (x) { continue; } if (h.readInt32LE(0) !== 0x54534449) continue; mdlCount++;
  const cstr = p => { let s = ''; while (h[p]) s += String.fromCharCode(h[p++]); return s; };
  const nt = h.readInt32LE(204), to = h.readInt32LE(208), nc = h.readInt32LE(212), co = h.readInt32LE(216);
  const cds = []; for (let i = 0; i < nc; i++) cds.push(norm(cstr(h.readInt32LE(co + i * 4))));
  for (let i = 0; i < nt; i++) { const p = to + i * 64; const t = norm(cstr(p + h.readInt32LE(p))); const tries = cds.map(c => norm('materials/' + c + '/' + t + '.vmt')); if (!tries.some(x => low[x])) need(tries[0] || 'materials/' + t + '.vmt', 'model ' + k + ' (also: ' + tries.slice(1, 3).join(' ') + ')'); }
}
// maps
for (const d of ['maps', 'maps_removed']) for (const f of fs.readdirSync(d).filter(x => x.endsWith('.bsp'))) {
  const a = fs.readFileSync(d + '/' + f); const o = a.readInt32LE(8 + 43 * 16), l = a.readInt32LE(12 + 43 * 16); const s = a.slice(o, o + l).toString('latin1').split('\0');
  // lump 40 pak holds some materials: read its names so they are not counted
  const po = a.readInt32LE(8 + 40 * 16), pl = a.readInt32LE(12 + 40 * 16); const pak = a.slice(po, po + pl).toString('latin1').toLowerCase();
  for (const t of s) { if (!t) continue; const n = norm(t); if (/^maps\//.test(n)) continue; if (pak.includes('materials/' + n + '.vmt')) continue; need('materials/' + n + '.vmt', d + '/' + f); }
  const eo = a.readInt32LE(8), el = a.readInt32LE(12); const es = a.slice(eo, eo + el).toString('latin1');
  for (const m of es.matchAll(/"model" "(models\/[^"]+\.mdl)"/g)) need(m[1], f);
  for (const m of es.matchAll(/"(?:material|texture|overlaymaterial|MoveDoneSound_x)" "([^"]+)"/g)) { if (/\.spr$/i.test(m[1])) continue; need('materials/' + m[1].replace(/\.vmt$/i, '') + '.vmt', f + ':ent'); }
}
console.log('models read', mdlCount, 'wanted but missing', want.size);

// ---- where to look
const P = (process.env.FSTOP_INPUT + '/Portal 2 Betas');
const roots = [];
(function walk(d, depth) { let es; try { es = fs.readdirSync(d, { withFileTypes: true }); } catch (x) { return; } for (const e of es) { if (!e.isDirectory()) continue; const f = d + '/' + e.name; if (/^(materials|models)$/i.test(e.name)) { if (!roots.includes(d)) roots.push(d); continue; } if (/^(bin|platform|screenshots|maps|sound|scripts|resource|cfg|media|save|sources)$/i.test(e.name)) continue; if (depth < 6) walk(f, depth + 1); } })(P, 0);
for (const extra of [(__dirname + '/../corehub/portal2_tempcontent'), 'C:/Program Files (x86)/Steam/steamapps/common/Portal/hl2', 'C:/Program Files (x86)/Steam/steamapps/common/Portal/portal']) if (fs.existsSync(extra)) roots.push(extra);
const vpks = []; (function walk(d, depth) { let es; try { es = fs.readdirSync(d, { withFileTypes: true }); } catch (x) { return; } for (const e of es) { const f = d + '/' + e.name; if (e.isDirectory()) { if (depth < 6 && !/^(materials|models|sound|bin|maps)$/i.test(e.name)) walk(f, depth + 1); } else if (/_dir\.vpk$/i.test(e.name)) vpks.push(f); } })(P, 0);
for (const v of ['Portal 2/portal2/pak01_dir.vpk', 'Team Fortress 2/tf/tf2_textures_dir.vpk', 'Team Fortress 2/tf/tf2_misc_dir.vpk', 'Half-Life 2/hl2/hl2_textures_dir.vpk', 'Half-Life 2/hl2/hl2_misc_dir.vpk', 'Half-Life 2/ep2/ep2_pak_dir.vpk', 'Half-Life 2/episodic/ep1_pak_dir.vpk']) { const f = 'C:/Program Files (x86)/Steam/steamapps/common/' + v; if (fs.existsSync(f)) vpks.push(f); }
console.log('loose roots', roots.length, 'vpks', vpks.length);
const index = new Map(); const byName = new Map();
const add = (p, e) => { if (!/^(materials|models)\//.test(p) || !/\.(vmt|vtf|mdl|vvd|vtx|phy|ani)$/.test(p)) return; if (!index.has(p)) index.set(p, e); const base = p.slice(p.lastIndexOf('/') + 1); let l = byName.get(base); if (!l) byName.set(base, l = []); if (l.length < 12) l.push(p); };
function readVpkDir(file) { const d = fs.readFileSync(file); let p = 0, tree = 0; if (d.readUInt32LE(0) === 0x55aa1234) { tree = d.readUInt32LE(4) === 1 ? 12 : 28; p = tree; } const dataStart = tree + (tree ? d.readUInt32LE(8) : 0); const str = () => { const s = p; while (d[p] !== 0) p++; const r = d.toString('latin1', s, p); p++; return r; }; for (;;) { const ext = str(); if (!ext) break; for (;;) { const dir = str(); if (!dir) break; for (;;) { const name = str(); if (!name) break; p += 4; const pre = d.readUInt16LE(p); p += 2; const arc = d.readUInt16LE(p); p += 2; const off = d.readUInt32LE(p); p += 4; const len = d.readUInt32LE(p); p += 4; p += 2; const preData = pre ? Buffer.from(d.subarray(p, p + pre)).toString('base64') : null; p += pre; add(norm((dir === ' ' ? '' : dir + '/') + name + (ext === ' ' ? '' : '.' + ext)), { vpk: file, arc, off: arc === 0x7fff ? off + dataStart : off, len, pre, preData }); } } } }
for (const r of roots) { for (const top of ['materials', 'models']) { const base = r + '/' + top; if (!fs.existsSync(base)) continue; (function w(d) { let es; try { es = fs.readdirSync(d, { withFileTypes: true }); } catch (x) { return; } for (const e of es) { const f = d + '/' + e.name; if (e.isDirectory()) w(f); else add(norm(path.relative(r, f)), { file: f }); } })(base); } }
for (const v of vpks) { try { readVpkDir(v); } catch (x) { console.log('vpk fail', v.slice(-50)); } }
console.log('indexed', index.size);

// ---- resolve
const tokens = s => s.replace(/\.[a-z0-9]+$/, '').split(/[\/_\-.]+/).filter(x => x && !/^(materials|models|props|the)$/.test(x));
const nameList = []; for (const p of index.keys()) if (p.endsWith('.vmt')) nameList.push(p);
function closest(p) { // closest material by shared words, favouring the same folder
  const tk = tokens(p); const dir = p.slice(0, p.lastIndexOf('/')); const base = tokens(p.slice(p.lastIndexOf('/') + 1)); let best = null, bs = 0;
  for (const q of nameList) { const qt = new Set(tokens(q)); let s = 0; for (const t of tk) if (qt.has(t)) s += 1; for (const t of base) if (qt.has(t)) s += 2; if (q.startsWith(dir + '/')) s += 3; if (s > bs) { bs = s; best = q; } }
  return bs >= 3 ? { path: best, score: bs } : null;
}
const plan = { exact: [], sameName: [], closest: [], none: [] };
for (const [p, why] of want) {
  if (index.has(p)) { plan.exact.push({ p, from: p, why }); continue; }
  const bn = p.slice(p.lastIndexOf('/') + 1); const same = byName.get(bn);
  if (same && same.length) { plan.sameName.push({ p, from: same[0], why, others: same.length }); continue; }
  if (p.endsWith('.vmt')) { const c = closest(p); if (c) { plan.closest.push({ p, from: c.path, score: c.score, why }); continue; } }
  plan.none.push({ p, why });
}
const entryOf = {}; for (const list of [plan.exact, plan.sameName, plan.closest]) for (const x of list) entryOf[x.from] = index.get(x.from);
fs.writeFileSync('out/texplan.json', JSON.stringify({ plan, entryOf }));
const srcOf = x => { const e = index.get(x); const s = e.file || e.vpk; return s.replace(P + '/', '').split('/').slice(0, 3).join('/'); };
console.log('exact', plan.exact.length, 'same name', plan.sameName.length, 'closest', plan.closest.length, 'nothing', plan.none.length);
console.log('--- exact'); for (const x of plan.exact.slice(0, 70)) console.log(' ', x.p.slice(0, 70).padEnd(70), srcOf(x.from).slice(0, 40));
console.log('--- same name'); for (const x of plan.sameName.slice(0, 40)) console.log(' ', x.p.slice(0, 60).padEnd(60), '<-', x.from.slice(0, 70));
console.log('--- closest'); for (const x of plan.closest.slice(0, 60)) console.log(' ', x.p.slice(0, 56).padEnd(56), '<-', x.from.slice(0, 60), x.score);
console.log('--- nothing'); for (const x of plan.none.slice(0, 60)) console.log(' ', x.p.slice(0, 70), ' | ', x.why.slice(0, 50));
