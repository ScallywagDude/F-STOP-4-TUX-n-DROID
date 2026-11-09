// Photo-resized NPCs must get a collision box scaled exactly once.
// Compiles the production CAI_BaseNPC::SetHullSizeNormal and
// CCollisionProperty::SetCollisionBounds bodies against small stand-ins, then checks
// the box at 0.25x / 1x / 4x and that a 4x chicken can reach an android with its
// production melee range. A negative control re-adds the old pre-multiplication and
// must fail.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const os = require('node:os');
const cp = require('node:child_process');
const root = path.resolve(__dirname, '../..');
const read = name => fs.readFileSync(path.join(root, 'engine/game', name), 'latin1');
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
const npc = read('server/ai_basenpc.cpp');
const coll = read('shared/collisionproperty.cpp');
const chicken = read('server/portal2/npc_chicken.cpp');
const scales = read('server/basecombatcharacter.cpp');

const hullNormal = method(npc, 'void CAI_BaseNPC::SetHullSizeNormal( bool force )');
const hullSmall = method(npc, 'bool CAI_BaseNPC::SetHullSizeSmall( bool force )');
const bounds = method(coll, 'void CCollisionProperty::SetCollisionBounds( const Vector &mins, const Vector &maxs )');
// Only the scaling half of SetCollisionBounds matters here; cut it before the dirty-flag tail.
const boundsCore = bounds.slice(0, bounds.indexOf('if ( bDirty )')) + '(void)bDirty; }';

// Production chicken melee range and the NPC scale table.
const range = chicken.match(/if \( flDist > \(([0-9.]+)f\*([0-9.]+)\)\*GetModelScale\(\) \)/);
assert(range, 'chicken melee range expression changed');
const meleeRange = Number(range[1]) * Number(range[2]);
const table = method(scales, 'CameraInfo_ScaleData_t *CBaseCombatCharacter::CPhotoPlacementQuery::GetSimpleScales( void )');
const npcScales = table.match(/s_DefaultScales\[\]\s*=\s*\{([^}]*)\}/)[1].split(',').map(s => parseFloat(s));
assert.deepEqual(npcScales, [0.25, 1, 4], 'NPC photo scale table changed; review this test');

const source = String.raw`
#include <cstdio>
#include <cstdlib>
#include <cmath>
struct Vector {
  float x,y,z;
  Vector(float a=0,float b=0,float c=0):x(a),y(b),z(c){}
  Vector operator*(float s)const{return Vector(x*s,y*s,z*s);}
  bool operator!=(const Vector&o)const{return x!=o.x||y!=o.y||z!=o.z;}
};
struct CBaseAnimating { float scale; float GetModelScale()const{return scale;} };
struct Outer { CBaseAnimating anim; CBaseAnimating *GetBaseAnimating(){return &anim;} };
struct CCollisionProperty {
  Outer *outer; Vector m_vecMinsPreScaled, m_vecMaxsPreScaled, m_vecMins, m_vecMaxs;
  Outer *GetOuter(){return outer;}
  void SetCollisionBounds( const Vector &mins, const Vector &maxs );
};
${boundsCore}
static const Vector kTinyMins(-12,-12,0), kTinyMaxs(12,12,24);
static const Vector kTinySmallMins(-8,-8,0), kTinySmallMaxs(8,8,24);
struct NAI_Hull {
  static const Vector &SmallMins(int){return kTinySmallMins;}
  static const Vector &SmallMaxs(int){return kTinySmallMaxs;}
};
struct CAI_BaseNPC {
  Outer outer; CCollisionProperty coll; bool m_fIsUsingSmallHull = true;
  CAI_BaseNPC(float s){ outer.anim.scale = s; coll.outer = &outer; }
  float GetModelScale()const{return outer.anim.scale;}
  const Vector &GetHullMins()const{return kTinyMins;}
  const Vector &GetHullMaxs()const{return kTinyMaxs;}
  int GetHullType()const{return 0;}
  void *VPhysicsGetObject(){return nullptr;}
  void SetupVPhysicsHull(){}
  void SetHullSizeNormal( bool force = false );
  bool SetHullSizeSmall( bool force = false );
};
static void UTIL_SetSize( CAI_BaseNPC *e, const Vector &mins, const Vector &maxs ){ e->coll.SetCollisionBounds(mins,maxs); }
${hullNormal}
${hullSmall}
static int fails = 0;
static void expect(bool ok, const char *what){ if(!ok){ std::printf("FAIL: %s\n", what); ++fails; } }
int main(){
  const float scales[] = { ${npcScales.join(', ')} };
  for (float s : scales) {
    CAI_BaseNPC n(s);
    n.SetHullSizeNormal(true);
    char msg[128];
    std::snprintf(msg, sizeof msg, "normal hull width at %.2fx", s);
    expect(std::fabs((n.coll.m_vecMaxs.x - n.coll.m_vecMins.x) - 24.0f*s) < 0.001f, msg);
    std::snprintf(msg, sizeof msg, "normal hull height at %.2fx", s);
    expect(std::fabs(n.coll.m_vecMaxs.z - 24.0f*s) < 0.001f, msg);
    n.SetHullSizeSmall(true);
    std::snprintf(msg, sizeof msg, "small hull width at %.2fx", s);
    expect(std::fabs((n.coll.m_vecMaxs.x - n.coll.m_vecMins.x) - 16.0f*s) < 0.001f, msg);
    n.SetHullSizeNormal();
    std::snprintf(msg, sizeof msg, "normal hull restored after small at %.2fx", s);
    expect(std::fabs((n.coll.m_vecMaxs.x - n.coll.m_vecMins.x) - 24.0f*s) < 0.001f, msg);
  }
  // A 4x chicken touching a normal android (human hull half-width 13) side by side:
  // centers are (chicken half-width + 13) apart and must be inside the melee range.
  CAI_BaseNPC big(4.0f); big.SetHullSizeNormal(true);
  float closest = big.coll.m_vecMaxs.x + 13.0f;
  expect(closest <= ${meleeRange.toFixed(1)}f * 4.0f, "4x chicken can reach an adjacent android with its melee range");
  CAI_BaseNPC normal(1.0f); normal.SetHullSizeNormal(true);
  expect(normal.coll.m_vecMaxs.x + 13.0f <= ${meleeRange.toFixed(1)}f, "1x chicken can reach an adjacent android");
  if (!fails) std::printf("ok\n");
  return fails ? 1 : 0;
}
`;
const windowsClang = 'C:/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/Llvm/x64/bin/clang++.exe';
const compiler = process.env.CXX || (process.platform === 'win32' ? windowsClang : 'clang++');
const temp = fs.mkdtempSync(path.join(os.tmpdir(), 'fstop-npc-scale-test-'));
const exe = process.platform === 'win32' ? '.exe' : '';
function build(name, text) {
  const cpp = path.join(temp, name + '.cpp'), bin = path.join(temp, name + exe);
  fs.writeFileSync(cpp, text);
  const c = cp.spawnSync(compiler, ['-std=c++11', '-Wall', '-Wextra', '-Werror', '-Wno-unused-parameter', cpp, '-o', bin], {encoding: 'utf8'});
  assert.equal(c.status, 0, c.stderr);
  return cp.spawnSync(bin, [], {encoding: 'utf8'});
}
try {
  const run = build('npc-scale', source);
  assert.equal(run.status, 0, run.stdout + run.stderr);
  // Negative control: the old body that multiplied by GetModelScale() before UTIL_SetSize.
  const oldBody = hullNormal.slice(0, hullNormal.indexOf('{')) + `{
	if ( m_fIsUsingSmallHull || force )
	{
		const float flScale = GetModelScale();
		Vector vecMins = ( GetHullMins() * flScale );
		Vector vecMaxs = ( GetHullMaxs() * flScale );
		UTIL_SetSize( this, vecMins, vecMaxs );
		m_fIsUsingSmallHull = false;
		if ( VPhysicsGetObject() )
		{
			SetupVPhysicsHull();
		}
	}
}`;
  const negative = build('npc-scale-old', source.replace(hullNormal, oldBody));
  assert(negative.status > 0, 'Double-scaling negative control unexpectedly passed');
  assert(/4x chicken can reach/.test(negative.stdout), 'negative control failed for an unexpected reason:\n' + negative.stdout);
  console.log(`PASS: NPC hull scaled once at ${npcScales.join('x/')}x, small/normal round trip, 4x and 1x chicken melee reach (range ${meleeRange}*scale); double-scale negative control fails`);
} finally {
  fs.rmSync(temp, {recursive: true, force: true});
}
