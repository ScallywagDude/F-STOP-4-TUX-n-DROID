// Packs a folder into one self-contained VPK v1 (all data inside the _dir file), the form P2008's own overrides use.
// usage: node tools/p2k_vpk.js <folder> <out_dir.vpk> [skip-regex]
const fs=require('fs'),path=require('path'),zlib=require('zlib');
const [src,out,skip]=process.argv.slice(2);const SKIP=skip?new RegExp(skip,'i'):null;
const crcT=[];for(let n=0;n<256;n++){let c=n;for(let k=0;k<8;k++)c=c&1?0xedb88320^(c>>>1):c>>>1;crcT[n]=c>>>0}
const crc=b=>{let c=~0>>>0;for(let i=0;i<b.length;i++)c=crcT[(c^b[i])&255]^(c>>>8);return (~c)>>>0};
const files=[];(function w(d){for(const e of fs.readdirSync(src+d,{withFileTypes:true})){const r=d+e.name;if(e.isDirectory())w(r+'/');else if(!SKIP||!SKIP.test(r))files.push(r)}})('');
const tree=new Map();for(const r of files){const l=r.toLowerCase();const ext=path.posix.extname(l).slice(1)||' ';const dir=path.posix.dirname(l);const name=path.posix.basename(l,ext===' '?'':'.'+ext);if(!tree.has(ext))tree.set(ext,new Map());const t=tree.get(ext);if(!t.has(dir==='.'?' ':dir))t.set(dir==='.'?' ':dir,[]);t.get(dir==='.'?' ':dir).push({name,r})}
const parts=[];let off=0;const chunks=[];const z=Buffer.from([0]);const S=s=>Buffer.from(s+'\0','latin1');
for(const [ext,dirs] of tree){parts.push(S(ext));for(const [dir,list] of dirs){parts.push(S(dir));for(const f of list){const d=fs.readFileSync(src+f.r);const e=Buffer.alloc(18);e.writeUInt32LE(crc(d),0);e.writeUInt16LE(0,4);e.writeUInt16LE(0x7fff,6);e.writeUInt32LE(off,8);e.writeUInt32LE(d.length,12);e.writeUInt16LE(0xffff,16);parts.push(S(f.name),e);chunks.push(d);off+=d.length}parts.push(z)}parts.push(z)}parts.push(z);
const T=Buffer.concat(parts);const h=Buffer.alloc(12);h.writeUInt32LE(0x55aa1234,0);h.writeUInt32LE(1,4);h.writeUInt32LE(T.length,8);
const fd=fs.openSync(out,'w');fs.writeSync(fd,h);fs.writeSync(fd,T);for(const c of chunks)fs.writeSync(fd,c);fs.closeSync(fd);console.log('packed',files.length,'files',(off/1048576|0)+'MB ->',out);
