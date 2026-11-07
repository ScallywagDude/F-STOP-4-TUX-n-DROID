// A brush that travels far keeps reflecting the room it was compiled in. Moves a named brush, and the brushes parented to it, onto the cubemap of where it ends up,
// in both the LDR and HDR face lumps, adding materials where the map has no version for that cubemap. New lumps go at the end of the file.
// usage: node tools/moverfix.js <map> <entity name> <from cube x_y_z> <to cube x_y_z> [--write]
const fs = require('fs'), path = require('path');
const ROOT = (__dirname + '/../');
const [mapName, entName, FROMK, TOK] = process.argv.slice(2);
const WRITE = process.argv.includes('--write');
const f = ROOT + 'maps/' + mapName + '.bsp';
const a = fs.readFileSync(f);
const L = n => [a.readInt32LE(8 + n * 16), a.readInt32LE(12 + n * 16)];
const [mo] = L(14), [fo, fl] = L(7), [ho, hl] = L(58), [tio, til] = L(6), [tdo, tdl] = L(2), [sdo, sdl] = L(43), [sto, stl] = L(44), [po, pl] = L(40), [eo, el] = L(0);
if (fl !== hl) throw new Error('face lumps differ');
const FACE = 56, TI = 72, TD = 32;
const FROM = '_' + FROMK, TO = '_' + TOK;
const CUBE_FROM = `maps/${mapName}/c${FROMK}`, CUBE_TO = `maps/${mapName}/c${TOK}`;
const nameOf = td => { const so = a.readInt32LE(sto + a.readInt32LE(tdo + td * TD + 12) * 4); let e = sdo + so; while (a[e]) e++; return a.toString('latin1', sdo + so, e); };

// a stored file from the map's own zip
function pakFile(wanted) {
  const p = a.subarray(po, po + pl); let q = 0;
  for (;;) {
    q = p.indexOf('PK\x03\x04', q, 'latin1'); if (q < 0) return null;
    const m = p.readUInt16LE(q + 8), sz = p.readUInt32LE(q + 18), nl = p.readUInt16LE(q + 26), ex = p.readUInt16LE(q + 28), n = p.toString('latin1', q + 30, q + 30 + nl);
    if (n.toLowerCase() === wanted.toLowerCase()) { if (m !== 0) throw new Error('compressed ' + n); return p.toString('latin1', q + 30 + nl + ex, q + 30 + nl + ex + sz); }
    q += 30 + nl + ex + sz;
  }
}
for (const c of [`materials/${CUBE_TO}.vtf`, `materials/${CUBE_TO}.hdr.vtf`]) if (!pakFile(c)) throw new Error('target cubemap missing: ' + c);

// the brush and what is parented to it
const t = a.toString('latin1', eo, eo + el);
const es = t.split('}').map(s => s.slice(s.indexOf('{') + 1));
const models = new Set();
for (const e of es) {
  const nm = /"targetname" "([^"]+)"/.exec(e), pn = /"parentname" "([^"]+)"/.exec(e), mod = /"model" "\*(\d+)"/.exec(e);
  if (mod && ((nm && nm[1] === entName) || (pn && pn[1] === entName))) models.add(Number(mod[1]));
}
if (!models.size) throw new Error('no brush named ' + entName);

const tdByName = new Map(); for (let td = 0; td < tdl / TD; td++) tdByName.set(nameOf(td).toLowerCase(), td);
let strings = Buffer.from(a.subarray(sdo, sdo + sdl));
const table = [Buffer.from(a.subarray(sto, sto + stl))], texdatas = [Buffer.from(a.subarray(tdo, tdo + tdl))], texinfos = [Buffer.from(a.subarray(tio, tio + til))];
let nStr = stl / 4, nTd = tdl / TD, nTi = til / TI;
const newTi = new Map(), changes = [], materials = [], summary = {};
for (const m of models) {
  const b = mo + m * 48, first = a.readInt32LE(b + 40), num = a.readInt32LE(b + 44);
  for (let i = 0; i < num; i++) {
    for (const lumpOff of [fo, ho]) {
      const p = lumpOff + (first + i) * FACE, ti = a.readInt16LE(p + 10);
      if (ti < 0) continue;
      const td = a.readInt32LE(tio + ti * TI + 68), n = nameOf(td);
      if (!n.toLowerCase().endsWith(FROM)) continue;
      const target = n.slice(0, -FROM.length) + TO;
      let t2 = tdByName.get(target.toLowerCase());
      if (t2 === undefined) {
        const vmt = pakFile('materials/' + n + '.vmt');
        if (!vmt) throw new Error('no patch material: ' + n);
        if (!vmt.toLowerCase().includes(CUBE_FROM.toLowerCase())) throw new Error('unexpected patch material: ' + vmt);
        materials.push(['materials/' + target + '.vmt', vmt.replace(new RegExp(CUBE_FROM, 'i'), CUBE_TO)]);
        const off = strings.length;
        strings = Buffer.concat([strings, Buffer.from(target + '\0', 'latin1')]);
        const e = Buffer.alloc(4); e.writeInt32LE(off); table.push(e);
        const d = Buffer.from(a.subarray(tdo + td * TD, tdo + (td + 1) * TD)); d.writeInt32LE(nStr, 12); texdatas.push(d);
        t2 = nTd++; nStr++;
        tdByName.set(target.toLowerCase(), t2);
      }
      const key = ti + ':' + t2;
      let nt = newTi.get(key);
      if (nt === undefined) { const c = Buffer.from(a.subarray(tio + ti * TI, tio + (ti + 1) * TI)); c.writeInt32LE(t2, 68); texinfos.push(c); nt = nTi++; newTi.set(key, nt); }
      changes.push([p, nt]);
      summary[n] = (summary[n] || 0) + 1;
    }
  }
}
console.log(mapName, entName, 'models', [...models].join(','), 'face entries moved', changes.length, 'new materials', materials.length, 'new texinfos', newTi.size, 'total', nTi);
for (const k in summary) console.log(' ', summary[k], k);
if (nTi > 32767) throw new Error('too many texinfos');
if (!WRITE || !changes.length) process.exit(0);
const backup = ROOT + `out/${mapName}_before_moverfix.bsp`;
if (!fs.existsSync(backup)) fs.copyFileSync(f, backup);
for (const [p, nt] of changes) a.writeInt16LE(nt, p + 10);
let r = a;
for (const [n, data] of [[43, strings], [44, Buffer.concat(table)], [2, Buffer.concat(texdatas)], [6, Buffer.concat(texinfos)]]) {
  const at = r.length;
  r = Buffer.concat([r, data]);
  r.writeInt32LE(at, 8 + n * 16); r.writeInt32LE(data.length, 12 + n * 16);
}
fs.writeFileSync(f, r);
fs.copyFileSync(f, ROOT + 'fsdata/fstop/maps/' + mapName + '.bsp');
for (const [k, v] of materials) for (const base of ['tools/loose_materials/', 'fsdata/fstop/']) { const o = ROOT + base + k; fs.mkdirSync(path.dirname(o), { recursive: true }); fs.writeFileSync(o, v, 'latin1'); }
console.log('written', r.length);
