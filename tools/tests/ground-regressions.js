// Exercise the production portal ground hook through the base virtual interface
// and the actual base movement's four-quadrant fallback.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const os = require('node:os');
const cp = require('node:child_process');
const root = path.resolve(__dirname, '../..');
const read = name => fs.readFileSync(path.join(root, 'engine/game/shared', name), 'latin1');
const base = read('gamemovement.cpp'), portal = read('portal/portal_gamemovement.cpp');
function method(source, signature) {
  const start = source.indexOf(signature);
  assert(start >= 0, `Missing ${signature}`);
  let end = source.indexOf('{', start), depth = 1;
  while (depth && ++end < source.length) {
    if (source[end] === '{') ++depth;
    if (source[end] === '}') --depth;
  }
  assert.equal(depth, 0);
  return source.slice(start, end + 1);
}
assert(method(base, 'void CGameMovement::CategorizePosition( void )').includes('TryTouchGround( bumpOrigin, point,'));
assert(!portal.includes('CategorizePosition( bool'));
const hook = method(portal, 'void CPortalGameMovement::TryTouchGround(');
const quadrants = method(base, 'void CGameMovement::TryTouchGroundInQuadrants(');
const source = String.raw`
#define VPROF(x) ((void)0)
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))
struct Vector {
  float x,y,z;
  Vector(float a=0,float b=0,float c=0):x(a),y(b),z(c){}
  void Init(float a,float b,float c){x=a;y=b;z=c;}
  float operator[](int i)const{return i==0?x:i==1?y:z;}
};
struct Ray_t {
  Vector start,end,mins,maxs;
  void Init(const Vector&a,const Vector&b,const Vector&c,const Vector&d){start=a;end=b;mins=c;maxs=d;}
};
struct trace_t {
  void* m_pEnt; struct {Vector normal;} plane;
  float fraction; Vector endpos;
  trace_t():m_pEnt(0),fraction(1){}
};
struct CPortal_Player {int m_hPortalEnvironment;};
struct CTraceFilterSimple {CTraceFilterSimple(CPortal_Player*,int){}};
struct CTraceFilterTranslateClones {CTraceFilterTranslateClones(CTraceFilterSimple*){}};
static int calls=0, plainWorld=0, solidQuadrant=-1;
static bool blocked=false, correctExtents=true;
template<class Filter>
void UTIL_Portal_TraceRay_With(int environment,const Ray_t&r,unsigned int,Filter*,trace_t*t){
  ++calls;
  correctExtents &= r.start.z==10 && r.end.z==8 && r.mins.z==0 && r.maxs.z==72;
  *t=trace_t();t->endpos=r.end;
  if(!environment || blocked || (solidQuadrant>=0 && calls==solidQuadrant+1)){
    t->m_pEnt=reinterpret_cast<void*>(1);t->plane.normal=Vector(0,0,1);t->fraction=.25f;
  }
}
class CGameMovement {
public:
  virtual ~CGameMovement(){}
  virtual void TryTouchGround(const Vector&,const Vector&,const Vector&,const Vector&,unsigned int,int,trace_t&t){
    ++plainWorld;t=trace_t();t.m_pEnt=reinterpret_cast<void*>(1);t.plane.normal=Vector(0,0,1);t.fraction=.25f;
  }
  Vector GetPlayerMins(){return Vector(-16,-16,0);}
  Vector GetPlayerMaxs(){return Vector(16,16,72);}
  void TryTouchGroundInQuadrants(const Vector&,const Vector&,unsigned int,int,trace_t&);
};
class CPortalGameMovement:public CGameMovement {
public:
  CPortal_Player p;
  CPortal_Player* GetPortalPlayer(){return &p;}
  void TryTouchGround(const Vector&,const Vector&,const Vector&,const Vector&,unsigned int,int,trace_t&) override;
};
${quadrants}
${hook}
#define CHECK(x) do{if(!(x))return __LINE__;}while(0)
int main(){
  CPortalGameMovement movement; CGameMovement* base=&movement;
  movement.p.m_hPortalEnvironment=1;
  trace_t tr; const Vector start(0,0,10),end(0,0,8);
  base->TryTouchGround(start,end,movement.GetPlayerMins(),movement.GetPlayerMaxs(),1,2,tr);
  CHECK(!tr.m_pEnt && tr.fraction==1 && calls==1 && plainWorld==0);
  calls=0; tr=trace_t();tr.fraction=.7f;tr.endpos=Vector(1,2,3);
  base->TryTouchGroundInQuadrants(start,end,1,2,tr);
  CHECK(calls==4 && !tr.m_pEnt && tr.fraction==.7f && tr.endpos.x==1 && tr.endpos.z==3);
  CHECK(correctExtents && plainWorld==0);
  // A valid ledge in any one quadrant still provides support; preserve the
  // original fraction/end position instead of snapping into a sloped edge.
  for(int q=0;q<4;++q){calls=0;solidQuadrant=q;tr=trace_t();tr.fraction=.8f;tr.endpos=Vector(4,5,6);
    base->TryTouchGroundInQuadrants(start,end,1,2,tr);
    CHECK(tr.m_pEnt && calls==q+1 && tr.fraction==.8f && tr.endpos.z==6);
  }
  solidQuadrant=-1;blocked=true;calls=0;
  base->TryTouchGround(start,end,movement.GetPlayerMins(),movement.GetPlayerMaxs(),1,2,tr);
  CHECK(tr.m_pEnt && calls==1);
  blocked=false;movement.p.m_hPortalEnvironment=0;calls=0;
  base->TryTouchGround(start,end,movement.GetPlayerMins(),movement.GetPlayerMaxs(),1,2,tr);
  CHECK(tr.m_pEnt && calls==1);
  return 0;
}
`;
const windowsClang = 'C:/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/Llvm/x64/bin/clang++.exe';
const compiler = process.env.CXX || (process.platform === 'win32' ? windowsClang : 'clang++');
const temp = fs.mkdtempSync(path.join(os.tmpdir(), 'fstop-ground-test-'));
try {
  for (const client of [false, true]) {
    const cpp = path.join(temp, client ? 'client.cpp' : 'server.cpp');
    const binary = path.join(temp, (client ? 'client' : 'server') + (process.platform === 'win32' ? '.exe' : ''));
    fs.writeFileSync(cpp, source);
    const compile = cp.spawnSync(compiler, ['-std=c++11', '-Wall', '-Wextra', '-Werror', ...(client ? ['-DCLIENT_DLL'] : []), cpp, '-o', binary], {encoding:'utf8'});
    assert.equal(compile.status, 0, compile.error?.message || compile.stdout + compile.stderr);
    const run = cp.spawnSync(binary, [], {encoding:'utf8'});
    assert.equal(run.status, 0, `Ground harness failed at ${run.status}: ${run.stderr}`);
  }
  // Reproduce the old dispatch to the raw world-ground implementation. It
  // compiles but must fail the portal-opening assertion above.
  const rawGround = hook.slice(0, hook.indexOf('{')) + `{
    CGameMovement::TryTouchGround(start,end,mins,maxs,fMask,collisionGroup,pm);
  }`;
  const negativeCpp = path.join(temp, 'raw-ground.cpp');
  const negativeBinary = path.join(temp, 'raw-ground' + (process.platform === 'win32' ? '.exe' : ''));
  fs.writeFileSync(negativeCpp, source.replace(hook, rawGround));
  const compileNegative = cp.spawnSync(compiler, ['-std=c++11', negativeCpp, '-o', negativeBinary], {encoding:'utf8'});
  assert.equal(compileNegative.status, 0, compileNegative.stderr);
  const negative = cp.spawnSync(negativeBinary, [], {encoding:'utf8'});
  assert(negative.status > 0, 'Raw-world ground negative control unexpectedly passed');
  console.log('PASS: client/server portal ground virtual dispatch, four fallback quadrants, real ledges, blocked opening and ordinary floor');
} finally {
  fs.rmSync(temp, {recursive:true, force:true});
}
