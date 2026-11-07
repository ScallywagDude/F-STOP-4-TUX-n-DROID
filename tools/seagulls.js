// Gives the shoreline's seagulls their own layer. In the stock set they share a quiet, infrequent pool with the other birds, so they are rarely heard.
const fs = require('fs');
const f = (__dirname + '/../fsdata/fstop/scripts/soundscapes_fstop.txt');
let t = fs.readFileSync(f, 'latin1');
if (t.includes('fstop.seagulls')) { console.log('already there'); process.exit(0); }
const anchor = /("name"\s+"coast\.util_windgusts"\s+"volume"\s+"0\.5"\s*\r?\n\s*\}\r?\n)/;
if (!anchor.test(t)) throw new Error('general_shoreline layout not as expected');
t = t.replace(anchor, '$1\r\n\t"playsoundscape"\r\n\t{\r\n\t\t"name"\t"fstop.seagulls"\r\n\t\t"volume"\t"1.0"\r\n\t}\r\n');
t += `\r\n"fstop.seagulls"\r\n{\r\n\t"playrandom"\r\n\t{\r\n\t\t"time"\t\t"7,16"\r\n\t\t"volume"\t"0.4,0.65"\r\n\t\t"pitch"\t\t"92,108"\r\n\t\t"soundlevel"\t"SNDLVL_140dB"\r\n\t\t"position"\t"random"\r\n\t\t"rndwave"\r\n\t\t{\r\n`;
for (let i = 1; i <= 5; i++) t += `\t\t\t"wave"\t"ambient/levels/coast/seagulls_ambient${i}.wav"\r\n`;
t += '\t\t}\r\n\t}\r\n}\r\n';
fs.writeFileSync(f, t, 'latin1');
console.log(t.replace(/\r/g, '').split('\n').slice(0, 34).join('\n'));
