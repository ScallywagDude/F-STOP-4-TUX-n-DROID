// Lists the faces of every func_water_analog in the given maps: material, plane normal, area. usage: node watermodel.js <bsp>...
const fs = require('fs');
for (const f of process.argv.slice(2)) {
  const a = fs.readFileSync(f);
  const L = n => [a.readInt32LE(8 + n * 16), a.readInt32LE(12 + n * 16)];
  const [mo] = L(14), [ho] = L(58), [pno] = L(1), [tio] = L(6), [tdo] = L(2), [sdo] = L(43), [sto] = L(44), [eo, el] = L(0);
  const name = ti => { const so = a.readInt32LE(sto + a.readInt32LE(tdo + a.readInt32LE(tio + ti * 72 + 68) * 32 + 12) * 4); let e = sdo + so; while (a[e]) e++; return a.toString('latin1', sdo + so, e); };
  for (const e of a.toString('latin1', eo, eo + el).split('}')) {
    if (!/"classname" "func_water_analog"/.test(e)) continue;
    const m = +/"model" "\*(\d+)"/.exec(e)[1], tn = (/"targetname" "([^"]+)"/.exec(e) || [])[1] || '-';
    const b = mo + m * 48, first = a.readInt32LE(b + 40), num = a.readInt32LE(b + 44), sum = {};
    for (let i = 0; i < num; i++) {
      const p = ho + (first + i) * 56, pl = a.readUInt16LE(p), side = a[p + 2], ti = a.readInt16LE(p + 10);
      const s = side ? -1 : 1, n = [0, 4, 8].map(k => (a.readFloatLE(pno + pl * 20 + k) * s).toFixed(2)).join(' ');
      const k = (ti < 0 ? 'none' : name(ti)) + ' n=' + n;
      sum[k] = (sum[k] || 0) + 1;
    }
    console.log(f.split('/').pop(), '*' + m, tn, 'faces', num, JSON.stringify(sum));
  }
}
