// Validates the production placement boundary under the release fast-math flags.
const fs=require('node:fs'),path=require('node:path'),os=require('node:os'),cp=require('node:child_process'),assert=require('node:assert/strict');
const root=path.resolve(__dirname,'../..');
const source=fs.readFileSync(path.join(root,'engine/game/shared/portal/PortalSimulation.cpp'),'latin1');
function body(signature){const start=source.indexOf(signature),open=source.indexOf('{',start);assert(start>=0);let n=1,end=open+1;while(n){if(source[end]==='{')++n;if(source[end]==='}')--n;++end;}return source.slice(open+1,end-1);}
const finite=body('static bool PortalSimulatorHasFinitePlacement(');
const resized=body('void CPortalSimulator::MovedOrResized('),guard=resized.slice(0,resized.indexOf('#ifndef CLIENT_DLL'));
const harness=String.raw`
#include <cstring>
union FloatCast_u {float f;unsigned int i;};
inline unsigned int const&FloatBits(float const&f){return reinterpret_cast<FloatCast_u const*>(&f)->i;}
struct Vector {float x,y,z;Vector(float a=0,float b=0,float c=0):x(a),y(b),z(c){}};typedef Vector QAngle;
static Vector vec3_origin;static QAngle vec3_angle;
static bool PortalSimulatorHasFinitePlacement(const Vector&center,const QAngle&angles,float halfWidth,float halfHeight){${finite}}
struct Portal {struct Data {struct Placement {float fHalfWidth,fHalfHeight;}Placement;}m_InternalData;bool m_bLocalDataIsReady;int clears,generated;Portal():m_bLocalDataIsReady(true),clears(0),generated(0){m_InternalData.Placement.fHalfWidth=0;m_InternalData.Placement.fHalfHeight=0;}void ClearEverything(){++clears;}void resize(const Vector&ptCenter,const QAngle&qAngles,float fHalfWidth,float fHalfHeight){${guard}++generated;}};
float fromBits(unsigned int raw){volatile unsigned int storage=raw;unsigned int bits=storage;float f;std::memcpy(&f,&bits,4);return f;}
int main(){
 for(unsigned int raw:{0x7fc00000u,0x7f800000u,0xff800000u})for(int i=0;i<8;++i){float v[8]={1,2,3,4,5,6,32,56};v[i]=fromBits(raw);if(PortalSimulatorHasFinitePlacement(Vector(v[0],v[1],v[2]),QAngle(v[3],v[4],v[5]),v[6],v[7]))return 1;Portal p;p.resize(Vector(v[0],v[1],v[2]),QAngle(v[3],v[4],v[5]),v[6],v[7]);if(p.generated||p.clears!=1||p.m_bLocalDataIsReady)return 2;}
 Portal p;p.resize(Vector(fromBits(0x7fc00000u),0,0),QAngle(),32,56);if(p.m_InternalData.Placement.fHalfWidth!=32||p.m_InternalData.Placement.fHalfHeight!=56)return 3;p.resize(Vector(-2490,-6940,400.031f),QAngle(0,90,0),32,56);if(p.generated!=1)return 4;
 for(float size:{0.0f,-1.0f}){Portal q;q.resize(Vector(),QAngle(),size,56);if(q.generated||q.clears!=1||q.m_bLocalDataIsReady)return 5;}
 return 0;
}
`;
const compiler=process.env.CXX||(process.platform==='win32'?'C:/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/Llvm/x64/bin/clang++.exe':'clang++');
const temp=fs.mkdtempSync(path.join(os.tmpdir(),'fstop-placement-'));
try{for(const negative of [false,true]){const input=path.join(temp,'test.cpp'),output=path.join(temp,process.platform==='win32'?'test.exe':'test');fs.writeFileSync(input,'#include <initializer_list>\n'+(negative?harness.replace('volatile unsigned int bits = FloatBits','unsigned int bits = FloatBits'):harness));const c=cp.spawnSync(compiler,['-std=c++11','-O2','-ffast-math','-fno-strict-aliasing',input,'-o',output],{encoding:'utf8'});assert.equal(c.status,0,c.stderr);const r=cp.spawnSync(output,[]);if(!negative)assert.equal(r.status,0,'production fast-math guard failed');else assert.notEqual(r.status,0,'negative control did not expose removed volatile boundary');}console.log('PASS: release fast-math rejects NaN/Inf in all8placementvalues, preserves resize-before-move, rejects invalid sizes; negative control fails.');}finally{assert(path.resolve(temp).startsWith(path.resolve(os.tmpdir())+path.sep));fs.rmSync(temp,{recursive:true,force:true});}
