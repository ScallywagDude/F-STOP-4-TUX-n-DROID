const fs=require('fs');
const b=fs.readFileSync(process.argv[2]);let p=0,tree=0;if(b.readUInt32LE(0)===0x55aa1234){tree=b.readUInt32LE(4)===1?12:28;p=tree}
const str=()=>{const s=p;while(b[p]!==0)p++;const r=b.toString('latin1',s,p);p++;return r};
for(;;){const e=str();if(!e)break;for(;;){const d=str();if(!d)break;for(;;){const n=str();if(!n)break;p+=4;const pre=b.readUInt16LE(p);p+=2;p+=2;p+=4;const len=b.readUInt32LE(p);p+=4;p+=2;p+=pre;console.log((d===' '?'':d+'/')+n+'.'+e, len+pre)}}}
