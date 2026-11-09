// Runs small native harnesses against production source fragments. This does
// not replace Linux/Android game and GPU testing. Run: node tools/tests/render-regressions.js
// CXX may name a C++ compiler; otherwise clang++ is used.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const os = require('node:os');
const cp = require('node:child_process');
const vm = require('node:vm');
const root = path.resolve(__dirname, '../..');
const view = fs.readFileSync(path.join(root, 'engine/game/client/viewrender.cpp'), 'utf8').replace(/\r\n/g, '\n');
const portal = fs.readFileSync(path.join(root, 'engine/game/client/portal/PortalRender.cpp'), 'utf8').replace(/\r\n/g, '\n');

function body(source, signature) {
  const start = source.indexOf(signature);
  assert(start >= 0, `missing ${signature}`);
  const open = source.indexOf('{', start);
  let depth = 1, end = open + 1;
  while (depth && end < source.length) {
    if (source[end] === '{') ++depth;
    if (source[end] === '}') --depth;
    ++end;
  }
  assert.equal(depth, 0);
  return source.slice(open + 1, end - 1);
}

const scopes = ['CScopedFStopAnalogWaterView', 'CScopedFStopWaterTarget'].map(name => {
  const start = view.indexOf(`class ${name}\n`);
  assert(start >= 0, `missing ${name}`);
  return view.slice(start, view.indexOf('\n};', start) + 3);
}).join('\n');
const refractionSetup = body(view, 'void CAboveWaterView::CRefractionView::Setup()');
const overlayDraw = body(view, 'void CViewRender::DrawUnderwaterOverlay( void )');
const overlayCondition = overlayDraw.match(/if\s*\( ([^\n]+) \)/)[1];
const querySignatures = [
  'void CPortalRenderable::BeginPortalPixelVisibilityQuery( void )',
  'void CPortalRenderable::EndPortalPixelVisibilityQuery( void )',
  'void CPortalRender::UpdatePortalPixelVisibility( void )',
];
const queryGuards = querySignatures.map(signature => {
  const text = body(portal, signature);
  return text.slice(0, text.indexOf('#endif') + '#endif'.length);
});
const queryTests = queryGuards.map((guard, i) => `
#define TEMP_DISABLE_PORTAL_VIS_QUERY
void disabled${i}() { ${guard}\n ++queryCalls; }
#undef TEMP_DISABLE_PORTAL_VIS_QUERY
void enabled${i}() { ${guard}\n ++queryCalls; }
`).join('\n');

// Assert the scopes are installed at the real entry points and both water targets.
for (const signature of ['void CViewRender::DrawWorldAndEntities(', 'void CViewRender::ViewDrawScene_PortalStencil(']) {
  assert.match(body(view, signature), /CScopedFStopAnalogWaterView analogWaterView;/);
}
for (const signature of ['void CAboveWaterView::CReflectionView::Draw()', 'void CAboveWaterView::CRefractionView::Draw()']) {
  assert.match(body(view, signature), /CScopedFStopWaterTarget waterTarget;/);
}
assert.match(body(view, 'void CUnderWaterView::Setup('), /pOverlayName\[0\] && Q_stricmp/);
assert.match(body(view, 'void CUnderWaterView::Setup('), /!pOverlayMaterial->IsErrorMaterial\(\)/);

const door = fs.readFileSync(path.join(root, 'engine/game/server/portal2/prop_portal_tunnel.cpp'), 'latin1').replace(/\r\n/g, '\n');
const particleHeader = fs.readFileSync(path.join(root, 'engine/game/server/particle_system.h'), 'utf8');
const flatPortal = fs.readFileSync(path.join(root, 'engine/game/client/portal/portalrenderable_flatbasic.cpp'), 'utf8');
for (const field of ['m_hParticleField', 'm_hParticleScale', 'm_hPhysicsBlocker']) {
  assert(door.includes(`DEFINE_FIELD( ${field}, FIELD_EHANDLE )`), `save/restore must track ${field}`);
}
// Depth restoration must use the same front offset as the mask, rather than
// moving the portal's depth back onto a coplanar translucent supporting surface.
for (const draw of ['DrawSimplePortalMesh', 'DrawRenderFixMesh']) {
  const args = text => text.match(new RegExp(`${draw}\\( ([^;]+) \\);`))[1].split(',').slice(1).join(',');
  assert.equal(args(body(flatPortal, 'void CPortalRenderable_FlatBasic::DrawStencilMask(')), args(body(flatPortal, 'void CPortalRenderable_FlatBasic::DrawPostStencilFixes(')));
}

// Mock only engine services around the actual door lifecycle and helper methods.
const doorHarness = `
namespace DoorTest {
#define NULL 0
struct Vector { float x,y,z; Vector(float a=0,float b=0,float c=0):x(a),y(b),z(c) {} Vector operator*(float f)const{return Vector(x*f,y*f,z*f);} Vector operator+(Vector b)const{return Vector(x+b.x,y+b.y,z+b.z);} };
typedef Vector QAngle;
enum { FL_EDICT_ALWAYS=8, SOLID_OBB=4, COLLISION_GROUP_PORTAL_TUNNEL_BLOCKER=64 };
#define DECLARE_CLASS(a,b)
struct Entity {
  Vector origin, angles; Entity *parent,*owner; bool removed; int spawned,activated,solid,group; float scale;
  Entity():parent(0),owner(0),removed(false),spawned(0),activated(0),solid(0),group(0),scale(1){}
  virtual int UpdateTransmitState(){return 0;}
  int SetTransmitState(int state){return state;}
  void SetAbsOrigin(Vector v){origin=v;} void SetAbsAngles(Vector v){angles=v;}
  void SetParent(Entity *p){parent=p;} void SetSolid(int s){solid=s;} void SetCollisionGroup(int g){group=g;}
  void Activate(){++activated;} void KeyValue(const char*,const char*){}
};
typedef Entity CBaseEntity; typedef Entity CPointEntity;
template<class T> struct Handle { T *p; Handle():p(0){} operator T*()const{return p;} T*operator->()const{return p;} T*Get()const{return p;} Handle&operator=(T*v){p=v;return *this;} };
typedef Handle<Entity> EHANDLE;
${door.slice(door.indexOf('class CPortalTunnelParticleScale'), door.indexOf('LINK_ENTITY_TO_CLASS( portal_tunnel_particle_scale'))}
struct ControlPoints { Entity *points[63]; int writes; ControlPoints():writes(0){for(int i=0;i<63;++i)points[i]=0;} void Set(int index,Entity *e){points[index]=e;++writes;} };
struct CParticleSystem : Entity { enum{kMAXCONTROLPOINTS=63}; ControlPoints m_hControlPointEnts;
  void SetControlPointEntity(int nControlPoint, CBaseEntity *pEntity) {
#define Assert(x) ((void)0)
    ${body(particleHeader, 'void SetControlPointEntity(')}
#undef Assert
  }
};
struct CEntityBlocker : Entity { Vector mins,maxs; int physics; CEntityBlocker():physics(0){} void VPhysicsInitStatic(){++physics;} void VPhysicsSetObject(void*){} static CEntityBlocker*Create(Vector,Vector,Vector,Entity*,bool); };
void*PhysModelCreateOBB(CEntityBlocker*b,Vector,Vector,Vector,Vector,bool isStatic){if(isStatic&&b->group==COLLISION_GROUP_PORTAL_TUNNEL_BLOCKER)++b->physics;return b;}
static CParticleSystem particles[4]; static CPortalTunnelParticleScale scalePoints[4]; static CEntityBlocker blockers[4]; static Entity portals[4],player;
static int particleIndex=0, scaleIndex=0, blockerIndex=0, portalIndex=0;static Entity*localPlayer=0;
bool equals(const char*a,const char*b){while(*a&&*a==*b){++a;++b;}return *a==*b;}
Entity*CreateEntityByName(const char*name){return equals(name,"info_particle_system")?static_cast<Entity*>(&particles[particleIndex++]):static_cast<Entity*>(&scalePoints[scaleIndex++]);}
void UTIL_Remove(Entity *e){e->removed=true;} void DispatchSpawn(Entity *e){++e->spawned;} Entity*UTIL_GetLocalPlayer(){return localPlayer;}
CEntityBlocker*CEntityBlocker::Create(Vector origin,Vector mins,Vector maxs,Entity*owner,bool physics){CEntityBlocker*b=&blockers[blockerIndex++];b->origin=origin;b->owner=owner;b->mins=mins;b->maxs=maxs;if(physics)b->VPhysicsInitStatic();return b;}
struct Collision { Vector OBBMins(){return Vector(-4,-40,-60);} Vector OBBMaxs(){return Vector(4,40,60);} };
struct BaseDoor : Entity { bool captured; int releases; BaseDoor():captured(false),releases(0){} void OnCaptured(){captured=true;} void OnReleased(){captured=false;++releases;} };
struct CPropPortalTunnel : BaseDoor {
  typedef BaseDoor BaseClass; Handle<CPropPortalTunnel>m_hPartner; Handle<Entity>m_hPortal;
  Handle<CParticleSystem>m_hParticleField; EHANDLE m_hParticleScale; Handle<CEntityBlocker>m_hPhysicsBlocker; Collision collision;
  Vector GetAbsOrigin(){return origin;} Vector GetAbsAngles(){return angles;} float GetModelScale(){return scale;}
  void GetVectors(Vector*v,void*,void*){*v=Vector(1,0,0);} Collision*CollisionProp(){return &collision;}
  void SetPartner(CPropPortalTunnel*p){m_hPartner=p;} void CreateTunnel(){m_hPortal=&portals[portalIndex++];}
  void CreateParticleField(){${body(door, 'void CPropPortalTunnel::CreateParticleField(')}}
  void CreatePhysicsBlocker(){${body(door, 'void CPropPortalTunnel::CreatePhysicsBlocker(')}}
  void Destroy(){${body(door, 'void CPropPortalTunnel::Destroy(')}}
  void OnCaptured(){${body(door, 'void CPropPortalTunnel::OnCaptured(')}}
  void OnReleased(){${body(door, 'void CPropPortalTunnel::OnReleased(')}}
};
int run() {
  CPropPortalTunnel door; door.scale=2; door.origin=Vector(500,600,700);door.angles=Vector(90,45,20);
  door.OnReleased();
  if(door.releases!=1 || !door.m_hParticleField || !door.m_hPhysicsBlocker || !door.m_hParticleScale)return 1;
  CParticleSystem *field=door.m_hParticleField;Entity *scalePoint=door.m_hParticleScale;CEntityBlocker*blocker=door.m_hPhysicsBlocker;
  if(field->m_hControlPointEnts.points[0]!=scalePoint || scalePoint->origin.x!=2 || scalePoint->UpdateTransmitState()!=FL_EDICT_ALWAYS)return 2;
  if(field->parent!=&door || field->spawned!=1 || field->activated!=1)return 3;
  if(blocker->solid!=SOLID_OBB || blocker->angles.x!=90 || blocker->angles.y!=45 || blocker->physics!=1)return 4;
  if(blocker->mins.y!=-36 || blocker->maxs.z!=54)return 5;
  // Initial map entities spawn before a local player exists.
  if(blocker->owner || blocker->group!=COLLISION_GROUP_PORTAL_TUNNEL_BLOCKER)return 9;
  field->SetControlPointEntity(0,scalePoint);field->SetControlPointEntity(64,scalePoint);
  if(field->m_hControlPointEnts.writes!=1)return 6;
  door.OnCaptured();
  if(!field->removed || !scalePoint->removed || !blocker->removed || door.m_hParticleField || door.m_hParticleScale || door.m_hPhysicsBlocker)return 7;
  localPlayer=&player;door.scale=0.6f;door.OnReleased();
  if(door.m_hParticleField==field || !door.m_hParticleField || door.m_hParticleScale->origin.x!=0.6f || door.releases!=2)return 8;
  if(door.m_hPhysicsBlocker->owner!=&player || door.m_hPhysicsBlocker->group!=COLLISION_GROUP_PORTAL_TUNNEL_BLOCKER)return 10;
  return 0;
}
}
`;

const ghost = fs.readFileSync(path.join(root, 'engine/game/client/portal/C_PortalGhostRenderable.cpp'), 'utf8');
const gles = fs.readFileSync(path.join(root, 'engine/togles/linuxwin/dxabstract.cpp'), 'utf8');
assert.match(gles, /pCaps->MaxUserClipPlanes\s*=\s*0;/);
assert.doesNotMatch(gles, /pCaps->MaxUserClipPlanes\s*=\s*2;/);
const desktopTranslator = fs.readFileSync(path.join(root, 'engine/togl/linuxwin/dx9asmtogl2.cpp'), 'utf8');
assert.match(desktopTranslator, /StrcatToALUCode\( "gl_ClipVertex = vTempPos;/);
const ghostHarness = `
namespace GhostTest {
struct matrix3x4_t { int value; };
struct VMatrix { matrix3x4_t value; VMatrix operator*(matrix3x4_t m){VMatrix out;out.value.value=m.value+100;return out;} matrix3x4_t As3x4(){return value;} };
struct CStudioHdr { int count; int numbones(){return count;} };
struct model_t {};
struct C_BaseEntity { bool ready,player; C_BaseEntity():ready(true),player(false){} virtual ~C_BaseEntity(){} virtual bool SetupBones(matrix3x4_t*,int,int,float){return false;} bool IsPlayer(){return player;} model_t*GetModel(){return 0;} };
struct C_BaseAnimating : C_BaseEntity { CStudioHdr studio; C_BaseAnimating(){studio.count=2;} virtual CStudioHdr*GetModelPtr(){return &studio;} bool SetupBones(matrix3x4_t*out,int capacity,int,float){if(!ready)return false;if(out)for(int i=0;i<capacity&&i<GetModelPtr()->count;++i)out[i].value=10+i;return true;} int DrawModel(int){return 10;} };
struct CBaseCombatWeapon : C_BaseAnimating { int index; CStudioHdr worldStudio,viewStudio; CBaseCombatWeapon():index(3){worldStudio.count=2;viewStudio.count=1;} int GetModelIndex(){return index;} int GetWorldModelIndex(){return 7;} void SetModelIndex(int n){index=n;} CStudioHdr*GetModelPtr(){return index==7?&worldStudio:&viewStudio;} };
struct C_Portal_Player : C_BaseAnimating { bool m_bEyePositionIsTransformedByPortal; C_Portal_Player():m_bEyePositionIsTransformedByPortal(false){player=true;} bool IsAlive(){return true;} static C_Portal_Player*GetLocalPlayer(); };
static C_Portal_Player localPlayer; C_Portal_Player*C_Portal_Player::GetLocalPlayer(){return &localPlayer;}
struct PortalRender { int GetViewRecursionLevel(){return 0;} }; static PortalRender portalRender;static PortalRender*g_pPortalRender=&portalRender;
typedef int DrawBrushModelMode_t;enum {DBM_DRAW_ALL,DBM_DRAW_TRANSLUCENT_ONLY,DBM_DRAW_OPAQUE_ONLY,STUDIO_TWOPASS=8,STUDIO_TRANSPARENCY=16};
struct Render { void DrawBrushModelEx(C_BaseEntity*,model_t*,int,int,int){} };static Render renderImpl;static Render*render=&renderImpl;
#define ANDROID
#define MIN(a,b) ((a)<(b)?(a):(b))
struct C_PortalGhostRenderable : C_BaseAnimating {
  C_BaseEntity*m_pGhostedRenderable;VMatrix m_matGhostTransform;bool m_bSourceIsBaseAnimating,m_bLocalPlayer;
  C_PortalGhostRenderable():m_pGhostedRenderable(0),m_bSourceIsBaseAnimating(true),m_bLocalPlayer(false){}
  int GetRenderOrigin(){return 0;}int GetRenderAngles(){return 0;}
  bool SetupBones(matrix3x4_t*pBoneToWorldOut,int nMaxBones,int boneMask,float currentTime){${body(ghost, 'bool C_PortalGhostRenderable::SetupBones(')}}
  int DrawModel(int flags){${body(ghost, 'int C_PortalGhostRenderable::DrawModel(')}}
};
#undef MIN
#undef ANDROID
int run(){
  C_PortalGhostRenderable ghost;CBaseCombatWeapon weapon;ghost.m_pGhostedRenderable=&weapon;
  matrix3x4_t bones[4]={{-1},{-1},{-1},{-1}};
  if(!ghost.SetupBones(bones,4,0,0)||weapon.GetModelIndex()!=3)return 1;
  if(bones[0].value!=110||bones[1].value!=111||bones[2].value!=-1||bones[3].value!=-1)return 2;
  weapon.ready=false;if(ghost.SetupBones(bones,4,0,0)||weapon.GetModelIndex()!=3)return 3;
  ghost.m_pGhostedRenderable=0;if(ghost.SetupBones(bones,4,0,0)||ghost.DrawModel(0)!=0)return 4;
  ghost.m_bLocalPlayer=true;ghost.m_pGhostedRenderable=&localPlayer;if(ghost.DrawModel(0)!=0)return 5;
  ghost.m_pGhostedRenderable=&weapon;if(ghost.DrawModel(0)!=10)return 6;
  ghost.m_bLocalPlayer=false;ghost.m_pGhostedRenderable=&localPlayer;if(ghost.DrawModel(0)!=10)return 7;
  return 0;
}
}
`;
const desktopGhostHarness = ghostHarness.replace('namespace GhostTest {', 'namespace DesktopGhostTest {')
  .replace('#define ANDROID\n', '')
  .replace('if(ghost.DrawModel(0)!=0)return 5;', 'if(ghost.DrawModel(0)!=10)return 5;');

const portalUtil = fs.readFileSync(path.join(root, 'engine/game/shared/portal/portal_util_shared.cpp'), 'latin1');
const boundsHarness = `
namespace BoundsTest {
struct Vector { float x,y,z;Vector(float a=0,float b=0,float c=0):x(a),y(b),z(c){} float&operator[](int i){return (&x)[i];}const float&operator[](int i)const{return (&x)[i];} };
struct Matrix { float m[3][4]; };
Vector point(const Matrix&m,Vector v){Vector out;for(int i=0;i<3;++i)out[i]=m.m[i][0]*v.x+m.m[i][1]*v.y+m.m[i][2]*v.z+m.m[i][3];return out;}
struct VMatrix { Matrix matrix;const Matrix&As3x4()const{return matrix;}Vector operator*(Vector v)const{return point(matrix,v);} };
float absolute(float x){return x<0?-x:x;}
void TransformAABB(const Matrix&m,Vector mins,Vector maxs,Vector&outMin,Vector&outMax){Vector center((mins.x+maxs.x)/2,(mins.y+maxs.y)/2,(mins.z+maxs.z)/2),extent((maxs.x-mins.x)/2,(maxs.y-mins.y)/2,(maxs.z-mins.z)/2);center=point(m,center);for(int i=0;i<3;++i){float e=absolute(m.m[i][0])*extent.x+absolute(m.m[i][1])*extent.y+absolute(m.m[i][2])*extent.z;outMin[i]=center[i]-e;outMax[i]=center[i]+e;}}
struct Wrapped { int calls;Wrapped():calls(0){}void WorldSpaceTriggerBounds(Vector*mins,Vector*maxs){++calls;if(mins)*mins=Vector(-1,-2,-3);if(maxs)*maxs=Vector(1,2,3);} };
struct CTransformedCollideable { Wrapped*m_pWrappedCollideable;VMatrix m_matTransform;
  void WorldSpaceTriggerBounds(Vector*pVecWorldMins,Vector*pVecWorldMaxs)const{${body(portalUtil, 'void CTransformedCollideable::WorldSpaceTriggerBounds(')}}
};
int run(){
  const float c=0.707106781f;
  Matrix rotations[3]={{{{0,-1,0,10},{1,0,0,20},{0,0,1,30}}},{{{-1,0,0,10},{0,-1,0,20},{0,0,1,30}}},{{{c,0,c,10},{0,1,0,20},{-c,0,c,30}}}};
  Wrapped source;CTransformedCollideable wrapper;wrapper.m_pWrappedCollideable=&source;
  for(int r=0;r<3;++r){wrapper.m_matTransform.matrix=rotations[r];Vector mins,maxs;wrapper.WorldSpaceTriggerBounds(&mins,&maxs);
    for(int axis=0;axis<3;++axis)if(mins[axis]>maxs[axis])return 1;
    for(int i=0;i<8;++i){Vector corner(i&1?1:-1,i&2?2:-2,i&4?3:-3);corner=point(rotations[r],corner);for(int a=0;a<3;++a)if(corner[a]<mins[a]-0.0001f||corner[a]>maxs[a]+0.0001f)return 2;}
    Vector onlyMin,onlyMax;wrapper.WorldSpaceTriggerBounds(&onlyMin,0);wrapper.WorldSpaceTriggerBounds(0,&onlyMax);for(int a=0;a<3;++a)if(onlyMin[a]!=mins[a]||onlyMax[a]!=maxs[a])return 3;
  }
  int calls=source.calls;wrapper.WorldSpaceTriggerBounds(0,0);if(source.calls!=calls)return 4;
  return 0;
}
}
`;

const simulation = fs.readFileSync(path.join(root, 'engine/game/shared/portal/PortalSimulation.cpp'), 'latin1');
const localPhysics = body(simulation, 'void CPortalSimulator::CreateLocalPhysics(');
const physicsParams = localPhysics.slice(localPhysics.indexOf('objectparams_t params'), localPhysics.indexOf('//World'));
const shapePaths = ['World.Brushes', 'World.Displacements', 'Wall.Local.Brushes', 'Wall.Local.Tube'];
const createShapeBranches = shapePaths.map(shape => {
  const condition = `if( m_InternalData.Simulation.Static.${shape}.pCollideable != NULL )`;
  return `${condition} { ${body(localPhysics, condition)} }`;
}).join('\n');
const physicsHarness = `
namespace PhysicsTest {
struct IPhysicsObject { int rechecks;IPhysicsObject():rechecks(0){}void RecheckCollisionFilter(){++rechecks;} };
struct CollisionEntity { IPhysicsObject*object;int assignments;CollisionEntity():object(0),assignments(0){}IPhysicsObject*VPhysicsGetObject(){return object;}void VPhysicsSetObject(IPhysicsObject*p){object=p;++assignments;} };
struct objectparams_t { void*pGameData; };static const objectparams_t g_PhysDefaultObjectParams={0};
static int world;void*GetWorldEntity(){return &world;}const int vec3_origin=0,vec3_angle=0;
struct Environment { IPhysicsObject object;int creates;void*data;Environment():creates(0),data(0){}IPhysicsObject*CreatePolyObjectStatic(void*,int,int,int,objectparams_t*p){++creates;data=p->pGameData;return &object;} };
struct Shape { void*pCollideable;IPhysicsObject*pPhysicsObject;Shape():pCollideable(0),pPhysicsObject(0){} };
struct World { Shape Brushes,Displacements; };struct Local { Shape Brushes,Tube; };struct Wall { Local Local;struct Remote { Shape Brushes; }RemoteTransformedToLocal; };
struct Static { World World;Wall Wall;struct SurfaceProperties { struct Surface { int surfaceProps;Surface():surfaceProps(0){} }surface; }SurfaceProperties; };
struct Simulation { Static Static;CollisionEntity*pCollisionEntity;Environment*pPhysicsEnvironment;Simulation():pCollisionEntity(0),pPhysicsEnvironment(0){} };
struct Data { Simulation Simulation; };
struct CPortalSimulator { Data m_InternalData;Data&m_DataAccess;CPortalSimulator():m_DataAccess(m_InternalData){}void create(){${physicsParams}\n${createShapeBranches}} };
struct CPSCollisionEntity { CPortalSimulator*m_pOwningSimulator;IPhysicsObject*VPhysicsGetObject(){${body(simulation, 'IPhysicsObject *CPSCollisionEntity::VPhysicsGetObject(')}} };
int run(){
  for(int i=0;i<4;++i){CPortalSimulator sim;Environment environment;CollisionEntity entity;sim.m_InternalData.Simulation.pCollisionEntity=&entity;sim.m_InternalData.Simulation.pPhysicsEnvironment=&environment;
    Shape*shapes[]={&sim.m_InternalData.Simulation.Static.World.Brushes,&sim.m_InternalData.Simulation.Static.World.Displacements,&sim.m_InternalData.Simulation.Static.Wall.Local.Brushes,&sim.m_InternalData.Simulation.Static.Wall.Local.Tube};
    shapes[i]->pCollideable=&world;sim.create();if(environment.creates!=1||shapes[i]->pPhysicsObject!=&environment.object||entity.object!=&environment.object||entity.assignments!=1||environment.data!=&entity||environment.object.rechecks!=1)return 1;
  }
  CPortalSimulator fallback;Environment environment;fallback.m_InternalData.Simulation.pPhysicsEnvironment=&environment;fallback.m_InternalData.Simulation.Static.World.Brushes.pCollideable=&world;fallback.create();if(environment.data!=GetWorldEntity())return 2;
  IPhysicsObject object;for(int i=0;i<4;++i){CPortalSimulator sim;CPSCollisionEntity entity;entity.m_pOwningSimulator=&sim;Shape*shapes[]={&sim.m_InternalData.Simulation.Static.World.Brushes,&sim.m_InternalData.Simulation.Static.Wall.Local.Brushes,&sim.m_InternalData.Simulation.Static.Wall.Local.Tube,&sim.m_InternalData.Simulation.Static.Wall.RemoteTransformedToLocal.Brushes};shapes[i]->pPhysicsObject=&object;if(entity.VPhysicsGetObject()!=&object)return 3;}
  CPSCollisionEntity empty;empty.m_pOwningSimulator=0;if(empty.VPhysicsGetObject()!=0)return 4;
  return 0;
}
}
`;

const shadowSource = fs.readFileSync(path.join(root, 'engine/game/server/portal/physicsshadowclone.cpp'), 'latin1');
const physicsObjectSource = fs.readFileSync(path.join(root, 'engine/vphysics/physics_object.cpp'), 'latin1');
const cloneHarness = `
namespace CloneTest {
struct Vector { float x,y,z;Vector(float a=0,float b=0,float c=0):x(a),y(b),z(c){}bool operator!=(const Vector&v)const{return x!=v.x||y!=v.y||z!=v.z;} };
typedef Vector QAngle;struct Matrix {};
struct VMatrix { Vector operator*(Vector v)const{return Vector(v.x+100,v.y,v.z);}Vector ApplyRotation(Vector v)const{return v;}Matrix As3x4()const{return Matrix();} };
QAngle TransformAnglesToWorldSpace(QAngle v,Matrix){return v;}
static int collisionsDuringTransform=0;
struct IPhysicsShadowController { bool*enabled;void GetMaxSpeed(float*a,float*b){*a=*b=10;}bool AllowsTranslation(){return true;}bool AllowsRotation(){return true;}void SetTeleportDistance(float){}float GetTeleportDistance(){return 1;}void SetPhysicallyControlled(bool){}bool IsPhysicallyControlled(){return true;}float GetTargetPosition(Vector*p,QAngle*a){*p=Vector(5,0,0);*a=Vector();return 0;}void Update(Vector,QAngle,float){if(*enabled)++collisionsDuringTransform;} };
struct IPhysicsObject { bool enabled,hasShadow;Vector position,angles,velocity,angular,inertia;IPhysicsShadowController shadow;int events[16],count;IPhysicsObject():enabled(false),hasShadow(false),count(0){shadow.enabled=&enabled;}
  bool IsCollisionEnabled(){return enabled;}void EnableCollisions(bool value){enabled=value;events[count++]=value?3:0;}bool IsAsleep(){return false;}void Wake(){}
  unsigned GetGameFlags(){return 0;}void*GetGameData(){return 0;}int GetGameIndex(){return 0;}void SetGameIndex(int){}int GetCallbackFlags(){return 0;}void SetCallbackFlags(int){}void SetGameFlags(unsigned){}int GetMaterialIndex(){return 0;}void SetMaterialIndex(int){}int GetContents(){return 0;}void SetContents(int){}
  bool IsGravityEnabled(){return true;}bool IsDragEnabled(){return true;}bool IsMotionEnabled(){return true;}void EnableGravity(bool){}void EnableDrag(bool){}void EnableMotion(bool){}void GetDamping(float*a,float*b){if(a)*a=0;if(b)*b=0;}void SetDamping(float*,float*){}float GetMass(){return 10;}void SetMass(float){}
  void GetPosition(Vector*p,QAngle*a){*p=position;*a=angles;}void GetVelocity(Vector*v,Vector*a){*v=velocity;*a=angular;}Vector GetInertia(){return inertia;}void SetInertia(Vector v){inertia=v;}
  void SetPosition(Vector p,QAngle a,bool){if(enabled)++collisionsDuringTransform;position=p;angles=a;events[count++]=1;}void SetVelocityInstantaneous(Vector*v,Vector*a){if(enabled)++collisionsDuringTransform;velocity=*v;angular=*a;events[count++]=2;}
  IPhysicsShadowController*GetShadowController(){return hasShadow?&shadow:0;}void RemoveShadowController(){hasShadow=false;}void SetShadow(float,float,bool,bool){hasShadow=true;}
};
struct CGrabController{};struct CBaseEntity{};struct CBasePlayer { void*GetActiveWeapon(){return 0;} };
CBasePlayer*GetPlayerHoldingEntity(CBaseEntity*){return 0;}CGrabController*GetGrabControllerForPlayer(CBasePlayer*){return 0;}CGrabController*GetGrabControllerForPhysCannon(void*){return 0;}void GetSavedParamsForCarriedPhysObject(CGrabController*,IPhysicsObject*,float*a,float*b){*a=10;*b=0;}
enum { FVPHYSICS_PLAYER_HELD=1,FVPHYSICS_NO_SELF_COLLISIONS=2,FVPHYSICS_IS_SHADOWCLONE=4 };
#define AssertMsg(a,b) ((void)0)
void FullSyncPhysicsObject(IPhysicsObject*pSource,IPhysicsObject*pDest,const VMatrix*pTransform,bool bTeleport){${body(shadowSource, 'static void FullSyncPhysicsObject(')}}
#undef AssertMsg
struct CPhysicsEnvironment{};struct vphysics_save_cphysicsobject_t { bool collisionEnabled,hasShadowController; };
static int allocations=0;struct CPhysicsObject : IPhysicsObject { CPhysicsObject(){++allocations;}void InitFromTemplate(CPhysicsEnvironment*,void*,const vphysics_save_cphysicsobject_t&t){enabled=t.collisionEnabled;hasShadow=t.hasShadowController;} };
IPhysicsObject*CreateObjectFromBuffer(CPhysicsEnvironment*pEnvironment,void*pGameData,unsigned char*pBuffer,unsigned bufferSize,bool enableCollisions){${body(physicsObjectSource, 'IPhysicsObject *CreateObjectFromBuffer( ')}}
int run(){
  IPhysicsObject source,dest;source.enabled=true;source.hasShadow=true;source.position=Vector(5,0,0);source.velocity=Vector(2,0,0);dest.enabled=true;VMatrix transform;FullSyncPhysicsObject(&source,&dest,&transform,true);
  if(collisionsDuringTransform!=0||!dest.enabled||dest.position.x!=105||dest.velocity.x!=2||dest.events[0]!=0||dest.events[dest.count-1]!=3)return 1;
  source.enabled=false;dest.count=0;FullSyncPhysicsObject(&source,&dest,0,false);if(dest.enabled||dest.events[dest.count-1]!=0)return 2;
  // The initial clone stays inert until its transform, even without a teleport.
  source.enabled=true;dest.enabled=false;dest.count=0;collisionsDuringTransform=0;FullSyncPhysicsObject(&source,&dest,&transform,false);if(collisionsDuringTransform||!dest.enabled||dest.events[dest.count-1]!=3)return 6;
  // A steady-state update must not disable live contacts each physics tick.
  dest.count=0;FullSyncPhysicsObject(&source,&dest,&transform,false);for(int i=0;i<dest.count;++i)if(dest.events[i]==0)return 7;
  for(int src=0;src<2;++src)for(int allow=0;allow<2;++allow){vphysics_save_cphysicsobject_t data={src!=0,true};IPhysicsObject*object=CreateObjectFromBuffer(0,0,reinterpret_cast<unsigned char*>(&data),sizeof(data),allow!=0);if(!object||object->enabled!=(src&&allow)||object->hasShadow||data.collisionEnabled!=(src!=0)||!data.hasShadowController)return 3;delete static_cast<CPhysicsObject*>(object);}
  int count=allocations;if(CreateObjectFromBuffer(0,0,0,0,true)!=0||allocations!=count)return 4;
  return 0;
}
}
`;

assert.equal((flatPortal.match(/PortalExitClipDistance\(/g) || []).length, 3, 'both stencil and RT portal views must use the exit-plane helper');
const exitPlaneBody = body(flatPortal, 'static float PortalExitClipDistance(');
const exitPlaneHarness = `
namespace ExitPlaneTest {
struct Vector { float x,y,z;Vector(float a=0,float b=0,float c=0):x(a),y(b),z(c){}float Dot(const Vector&v)const{return x*v.x+y*v.y+z*v.z;}Vector operator+(Vector v)const{return Vector(x+v.x,y+v.y,z+v.z);}Vector operator*(float f)const{return Vector(x*f,y*f,z*f);} };
#define PORTAL2
float portable(const Vector&forward,const Vector&origin){${exitPlaneBody}}
#undef PORTAL2
float classic(const Vector&forward,const Vector&origin){${exitPlaneBody}}
int run(){Vector origin(-2193.331f,-5088.769f,186.031f);Vector normals[]={Vector(1,0,0),Vector(0,1,0),Vector(0,0,1),Vector(-1,0,0),Vector(0,-1,0),Vector(0,0,-1),Vector(0.707106781f,0,0.707106781f)};for(int i=0;i<7;++i){Vector normal=normals[i],glass=origin+normal*(-0.2f),front=origin+normal;float plane=portable(normal,origin);if(normal.Dot(glass)>=plane||normal.Dot(front)<plane)return 1;if(normal.Dot(glass)<classic(normal,origin))return 2;}return 0;}
}
`;

// Compile the real shared collision-group rules and verify the full group matrix.
// The tunnel field must clear both player paths even with no owner at map start,
// while every other collision keeps its existing behavior.
const constants = fs.readFileSync(path.join(root, 'engine/public/const.h'), 'utf8');
const baseRules = fs.readFileSync(path.join(root, 'engine/game/shared/gamerules.cpp'), 'utf8');
const portalRules = fs.readFileSync(path.join(root, 'engine/game/shared/portal/portal_gamerules.cpp'), 'utf8');
const compat = fs.readFileSync(path.join(root, 'engine/game/shared/fstop_compat.h'), 'utf8');
const collisionHarness = `
namespace CollisionTest {
template<class T>void V_swap(T&a,T&b){T temp=a;a=b;b=temp;}
${constants.match(/enum Collision_Group_t[\s\S]+?\n};/)[0]}
${compat.match(/^#define COLLISION_GROUP_.*$/gm).map(text => text.replace(/^#define (\w+) (.*)$/, 'enum { $1 = $2 };')).join('\n')}
class CGameRules { public:bool ShouldCollide(int collisionGroup0,int collisionGroup1){${body(baseRules, 'bool CGameRules::ShouldCollide(').replace('::V_swap','V_swap')}} };
#define PORTAL2
class CPortalGameRules:public CGameRules { typedef CGameRules BaseClass; public:bool ShouldCollide(int collisionGroup0,int collisionGroup1){${body(portalRules, 'bool CPortalGameRules::ShouldCollide(')}} };
#undef PORTAL2
int run(){
 CGameRules original;CPortalGameRules rules;
 for(int a=0;a<=COLLISION_GROUP_PORTAL_TUNNEL_BLOCKER;++a)for(int b=0;b<=COLLISION_GROUP_PORTAL_TUNNEL_BLOCKER;++b){
  bool expected=(a==COLLISION_GROUP_CAMERA_SOLID||b==COLLISION_GROUP_CAMERA_SOLID||a==COLLISION_GROUP_PLACEMENT_SOLID||b==COLLISION_GROUP_PLACEMENT_SOLID)?a==b:original.ShouldCollide(a,b);
  bool player=(a==COLLISION_GROUP_PLAYER||a==COLLISION_GROUP_PLAYER_MOVEMENT||b==COLLISION_GROUP_PLAYER||b==COLLISION_GROUP_PLAYER_MOVEMENT);
  if(player&&(a==COLLISION_GROUP_PORTAL_TUNNEL_BLOCKER||b==COLLISION_GROUP_PORTAL_TUNNEL_BLOCKER))expected=false;
  if(rules.ShouldCollide(a,b)!=expected)return 1;
 }
 if(!rules.ShouldCollide(COLLISION_GROUP_PORTAL_TUNNEL_BLOCKER,COLLISION_GROUP_NONE))return 2;
 if(!rules.ShouldCollide(COLLISION_GROUP_INTERACTIVE_DEBRIS,COLLISION_GROUP_PORTAL_TUNNEL_BLOCKER))return 3;
 if(!rules.ShouldCollide(COLLISION_GROUP_PORTAL_TUNNEL_BLOCKER,COLLISION_GROUP_NPC))return 4;
 return 0;
}
}
`;

const cameraSource = fs.readFileSync(path.join(root, 'engine/game/client/portal/c_weapon_stubs_portal.cpp'), 'utf8').replace(/\r\n/g, '\n');
assert.match(cameraSource, /STUB_WEAPON_CLASS\( weapon_camera, WeaponCamera, C_FStopCameraWeapon \)/);
assert.match(cameraSource, /STUB_WEAPON_CLASS\( weapon_placement, WeaponPlacement, C_BaseCombatWeapon \)/);
const cameraClass = cameraSource.slice(cameraSource.indexOf('class C_FStopCameraWeapon'), cameraSource.indexOf('\n};', cameraSource.indexOf('class C_FStopCameraWeapon')) + 3);
const cameraHarness = `
namespace CameraTest {
struct CStudioHdr { int count;int GetNumSeq(){return count;} };
typedef int DataUpdateType_t;
class C_BaseCombatWeapon { public:int model,selected,world,sequence,updates,resets;float cycle;bool loaded;CStudioHdr studio;
 C_BaseCombatWeapon():model(3),selected(7),world(7),sequence(1),updates(0),resets(0),cycle(.4f),loaded(true){studio.count=1;}
 virtual void OnDataChanged(int){++updates;model=selected;}
 int GetModelIndex(){return model;}int GetWorldModelIndex(){return world;}int GetSequence(){return sequence;}
 CStudioHdr*GetModelPtr(){return loaded?&studio:0;}void ResetSequence(int value){sequence=value;++resets;}
};
#define DECLARE_CLASS(name,parent) typedef parent BaseClass
${cameraClass}
#undef DECLARE_CLASS
int run(){
 C_FStopCameraWeapon camera;camera.OnDataChanged(0);if(camera.updates!=1||camera.sequence!=0||camera.resets!=1)return 1;
 camera.OnDataChanged(1);if(camera.updates!=2||camera.sequence!=0||camera.resets!=1||camera.cycle!=.4f)return 2;
 camera.studio.count=3;camera.sequence=2;camera.OnDataChanged(1);if(camera.sequence!=2||camera.resets!=1)return 3;
 camera.selected=3;camera.sequence=10;camera.OnDataChanged(1);if(camera.sequence!=10||camera.resets!=1)return 4;
 camera.selected=7;camera.loaded=false;camera.OnDataChanged(1);if(camera.sequence!=10||camera.resets!=1)return 5;
 camera.loaded=true;camera.studio.count=0;camera.OnDataChanged(1);if(camera.sequence!=10||camera.resets!=1)return 6;
 camera.studio.count=1;camera.sequence=-1;camera.OnDataChanged(1);if(camera.sequence!=0||camera.resets!=2)return 7;
 return 0;
}
}
`;

// Dynamic water uses moving brush bounds and material fog, not BSP water leaves.
const analogUnderSetup = body(view, 'void CUnderWaterView::Setup(');
const analogUnderFlags = analogUnderSetup.slice(analogUnderSetup.indexOf('m_DrawFlags = DF_FUDGE_UP'), analogUnderSetup.indexOf('\tm_fogInfo = fogInfo;'));
assert.match(analogUnderSetup, /FStopWaterFogMaterial\( fogInfo.m_pFogVolumeMaterial, s_bFStopAnalogWaterActive \)/);
assert.match(body(view, 'void CUnderWaterView::CRefractionView::Draw()'), /CScopedFStopWaterTarget waterTarget;/);
const analogWaterHarness = `
#include <cstring>
#include <cmath>
#include <cfloat>
namespace AnalogWaterTest {
#define MAX(a,b) ((a)>(b)?(a):(b))
struct Vector {float x,y,z;Vector(float a=0,float b=0,float c=0):x(a),y(b),z(c){} Vector&operator+=(Vector v){x+=v.x;y+=v.y;z+=v.z;return *this;} };
struct IMaterialVar {bool defined;const char*text;float value,vec[3];IMaterialVar():defined(true),text(""),value(1){vec[0]=.2f;vec[1]=.3f;vec[2]=.4f;}bool IsDefined(){return defined;}const char*GetStringValue(){return text;}int GetIntValue(){return (int)value;}float GetFloatValue(){return value;}void GetVecValue(float*v,int){for(int i=0;i<3;++i)v[i]=vec[i];} };
struct IMaterial {bool error;IMaterialVar bottom,enable,color,start,end;IMaterial():error(false){start.value=-100;end.value=700;}bool IsErrorMaterial(){return error;}IMaterialVar*FindVar(const char*name,void*,bool){return !std::strcmp(name,"$bottommaterial")?&bottom:!std::strcmp(name,"$fogenable")?&enable:!std::strcmp(name,"$fogcolor")?&color:!std::strcmp(name,"$fogstart")?&start:!std::strcmp(name,"$fogend")?&end:0;} };
static IMaterial surface,bottom,errorMaterial;
struct Materials {IMaterial*FindMaterial(const char*name,int){return !std::strcmp(name,"bottom")?&bottom:&errorMaterial;} };static Materials materialSystem;static Materials*materials=&materialSystem;
enum {TEXTURE_GROUP_OTHER,MATERIAL_FOG_NONE,MATERIAL_FOG_LINEAR,MATERIAL_FOG_LINEAR_BELOW_FOG_Z};
int Q_stricmp(const char*a,const char*b){return std::strcmp(a,b);}
IMaterial*FStopWaterFogMaterial(IMaterial*pSurface,bool bUnderwater){${body(view, 'static IMaterial *FStopWaterFogMaterial(')}}
bool FStopAnalogWaterContainsEye(const Vector&eye,const Vector&mins,const Vector&maxs){${body(view, 'static bool FStopAnalogWaterContainsEye(')}}
struct model_t {Vector mins,maxs;IMaterial*material;};
struct C_BaseEntity {model_t model;Vector origin;bool IsDormant(){return false;}model_t*GetModel(){return &model;}Vector GetAbsOrigin(){return origin;} };
struct ModelInfo {void GetModelBounds(model_t*p,Vector&mins,Vector&maxs){mins=p->mins;maxs=p->maxs;}};static ModelInfo modelInfo;static ModelInfo*modelinfo=&modelInfo;
struct Cache {C_BaseEntity*entity;int Count(){return 1;}C_BaseEntity*operator[](int){return entity;}};static Cache s_FStopAnalogWater;
struct Time {float curtime;};static Time time={1};static Time*gpGlobals=&time;static float s_flFStopNextWaterScan=100;
void FStopScanAnalogWater(){} IMaterial*FStopWaterSurfaceMaterial(model_t*m){return m->material;}
static bool s_bFStopAnalogWaterActive=false;
struct VisibleFogVolumeInfo_t {int m_nVisibleFogVolume,m_nVisibleFogVolumeLeaf;IMaterial*m_pFogVolumeMaterial;float m_flWaterHeight,m_flDistanceToWater;bool m_bEyeInFogVolume;VisibleFogVolumeInfo_t():m_nVisibleFogVolume(-1),m_nVisibleFogVolumeLeaf(-1),m_pFogVolumeMaterial(0),m_flWaterHeight(0),m_flDistanceToWater(0),m_bEyeInFogVolume(false){} };
void FStopFindAnalogWater(const Vector&vecOrigin,VisibleFogVolumeInfo_t&fogInfo){${body(view, 'static void FStopFindAnalogWater(')}}
static bool fogEnabled=true;struct ConVarRef {ConVarRef(const char*){}bool GetBool(){return fogEnabled;}};
struct Context {int mode;float z,start,end,density,color[3];void SetFogZ(float v){z=v;}void FogMode(int v){mode=v;}void FogColor3fv(float*v){for(int i=0;i<3;++i)color[i]=v[i];}void FogStart(float v){start=v;}void FogEnd(float v){end=v;}void FogMaxDensity(float v){density=v;}};static Context context;
struct CMatRenderContextPtr {CMatRenderContextPtr(Materials*){}Context*operator->(){return &context;}};
struct Render {int calls;void SetFogVolumeState(int,bool){++calls;}};static Render renderer;static Render*render=&renderer;
struct Rendering {void SetFogVolumeState(const VisibleFogVolumeInfo_t&fogInfo,bool bUseHeightFog){${body(view, 'void CRendering3dView::SetFogVolumeState(')}}};
enum {DF_FUDGE_UP=1,DF_RENDER_UNDERWATER=2,DF_DRAW_ENTITITES=4,DF_CLIP_Z=8,DF_RENDER_WATER=16,DF_RENDER_ABOVEWATER=32,VIEW_CLEAR_DEPTH=64};
struct WaterInfo {bool m_bDrawWaterSurface,m_bRefract,m_bOpaqueWater;};
struct Setup {int m_DrawFlags,m_ClearFlags;bool m_bSoftwareUserClipPlane;void run(WaterInfo waterInfo){${analogUnderFlags}}};
int run(){
 errorMaterial.error=true;surface.bottom.text="bottom";bottom.color.vec[0]=.5f;bottom.start.value=-350;bottom.end.value=1050;
 if(FStopWaterFogMaterial(&surface,false)!=&surface||FStopWaterFogMaterial(&surface,true)!=&bottom)return 1;
 surface.bottom.text="missing";if(FStopWaterFogMaterial(&surface,true)!=&surface)return 2;surface.bottom.text="bottom";
 C_BaseEntity sea;sea.origin=Vector(1000,2000,3000);sea.model.mins=Vector(-100,-100,-200);sea.model.maxs=Vector(100,100,0);sea.model.material=&surface;s_FStopAnalogWater.entity=&sea;
 VisibleFogVolumeInfo_t above;FStopFindAnalogWater(Vector(1000,2000,3050),above);if(!s_bFStopAnalogWaterActive||above.m_bEyeInFogVolume||above.m_flWaterHeight!=3000||above.m_flDistanceToWater!=50)return 3;
 VisibleFogVolumeInfo_t under;FStopFindAnalogWater(Vector(1000,2000,2950),under);if(!s_bFStopAnalogWaterActive||!under.m_bEyeInFogVolume||under.m_flDistanceToWater!=50)return 4;
 VisibleFogVolumeInfo_t outside;FStopFindAnalogWater(Vector(1200,2000,2950),outside);if(s_bFStopAnalogWaterActive)return 5;
 FStopFindAnalogWater(Vector(1000,2000,2700),outside);if(s_bFStopAnalogWaterActive)return 6;
 VisibleFogVolumeInfo_t bsp;bsp.m_nVisibleFogVolume=2;FStopFindAnalogWater(Vector(1000,2000,2950),bsp);if(s_bFStopAnalogWaterActive||bsp.m_nVisibleFogVolume!=2)return 7;
 s_bFStopAnalogWaterActive=true;Rendering view;view.SetFogVolumeState(under,false);if(context.mode!=MATERIAL_FOG_LINEAR||context.start!=-350||context.end!=1050||context.color[0]!=.5f||context.z!=3000)return 8;
 view.SetFogVolumeState(above,true);if(context.mode!=MATERIAL_FOG_LINEAR_BELOW_FOG_Z||context.end!=700||context.color[0]!=.2f)return 9;
 fogEnabled=false;view.SetFogVolumeState(under,false);if(context.mode!=MATERIAL_FOG_NONE)return 10;fogEnabled=true;
 view.SetFogVolumeState(bsp,false);if(renderer.calls!=1)return 11;
 Setup setup;setup.m_bSoftwareUserClipPlane=true;setup.run(WaterInfo{true,true,false});if((setup.m_DrawFlags&(DF_RENDER_ABOVEWATER|DF_RENDER_UNDERWATER|DF_CLIP_Z))!=(DF_RENDER_ABOVEWATER|DF_RENDER_UNDERWATER|DF_CLIP_Z))return 12;
 s_bFStopAnalogWaterActive=false;setup.run(WaterInfo{true,true,false});if(setup.m_DrawFlags&(DF_RENDER_ABOVEWATER|DF_CLIP_Z))return 13;
 return 0;
}
#undef MAX
}
`;

const harness = `
const char *PORTAL_DOOR_FIZZLE_EFFECT="fizzler_field_scalable";
${analogWaterHarness}
${collisionHarness}
${cameraHarness}
${doorHarness}
${ghostHarness}
${desktopGhostHarness}
${boundsHarness}
${physicsHarness}
${cloneHarness}
${exitPlaneHarness}
static bool s_bFStopAnalogWaterActive = false;
static bool s_bFStopInWaterView = false;
${scopes}
enum { DF_RENDER_REFRACTION=1, DF_CLIP_Z=2, DF_RENDER_UNDERWATER=4,
       DF_FUDGE_UP=8, DF_DRAW_ENTITITES=16, DF_RENDER_ABOVEWATER=32,
       VIEW_CLEAR_COLOR=1, VIEW_CLEAR_DEPTH=2 };
struct Outer {};
struct Base { void Setup(Outer&) {} };
struct Refraction : Base {
  typedef Base BaseClass;
  Outer outer; int m_DrawFlags, m_ClearFlags;
  Outer* GetOuter() { return &outer; }
  void Setup() { ${refractionSetup} }
};
struct Material { bool error; bool IsErrorMaterial() { return error; } };
bool drawsOverlay(Material *pOverlayMat) { return ${overlayCondition}; }
static int queryCalls = 0;
${queryTests}
#define CHECK(condition) do { if (!(condition)) return __LINE__; } while (0)
void earlyExit() { CScopedFStopWaterTarget target; }
int main() {
  // Nested dry portal views must restore their wet parent's state.
  { CScopedFStopAnalogWaterView outer;
    s_bFStopAnalogWaterActive = true;
    { CScopedFStopWaterTarget reflection;
      CHECK(s_bFStopInWaterView);
      { CScopedFStopAnalogWaterView dryPortal;
        CHECK(!s_bFStopAnalogWaterActive);
        CHECK(s_bFStopInWaterView);
      }
      CHECK(s_bFStopAnalogWaterActive && s_bFStopInWaterView);
      { CScopedFStopWaterTarget nestedTarget; }
      CHECK(s_bFStopInWaterView);
    }
    CHECK(!s_bFStopInWaterView);
    earlyExit(); CHECK(!s_bFStopInWaterView);
  }
  CHECK(!s_bFStopAnalogWaterActive && !s_bFStopInWaterView);
  Refraction refraction;
  refraction.Setup();
  CHECK((refraction.m_DrawFlags & DF_RENDER_ABOVEWATER) == 0);
  s_bFStopAnalogWaterActive = true;
  refraction.Setup();
  CHECK((refraction.m_DrawFlags & DF_RENDER_ABOVEWATER) != 0);
  CHECK((refraction.m_DrawFlags & (DF_RENDER_UNDERWATER | DF_CLIP_Z)) == (DF_RENDER_UNDERWATER | DF_CLIP_Z));
  Material valid = {false}, missing = {true};
  CHECK(!drawsOverlay(0)); CHECK(!drawsOverlay(&missing)); CHECK(drawsOverlay(&valid));
  disabled0(); disabled1(); disabled2(); CHECK(queryCalls == 0);
  enabled0(); enabled1(); enabled2(); CHECK(queryCalls == 3);
  CHECK(AnalogWaterTest::run() == 0);
  CHECK(CameraTest::run() == 0);
  CHECK(CollisionTest::run() == 0);
  CHECK(DoorTest::run() == 0);
  CHECK(GhostTest::run() == 0);
  CHECK(DesktopGhostTest::run() == 0);
  CHECK(BoundsTest::run() == 0);
  CHECK(PhysicsTest::run() == 0);
  CHECK(CloneTest::run() == 0);
  CHECK(ExitPlaneTest::run() == 0);
  return 0;
}
`;

// Exercise the production asset dependency scanner with a custom overlay outside
// the stock effects folders, including its texture dependency.
const assets = fs.readFileSync(path.join(root, 'tools/assets.js'), 'utf8');
const fixture = new Map([
  ['materials/water/test.vmt', 'Water { "$underwateroverlay" "custom/water_warp" }'],
  ['materials/custom/water_warp.vmt', 'Refract { "$normalmap" "custom/water_normal" }'],
  ['materials/custom/water_normal.vtf', 'texture'],
]);
// Use the real restored overlay VMTs so packaging keeps their texture closure.
for (const name of ['water_warp', 'water_warp_2fort']) {
  const material = fs.readFileSync(path.join(root, 'tools/loose_materials/materials/effects', name + '.vmt'), 'latin1');
  assert.match(material, /^\s*"Refract"/);
  fixture.set('materials/effects/' + name + '.vmt', material);
  fixture.set('materials/water/' + name + '.vmt', 'Water { "$underwateroverlay" "effects/' + name + '" }');
}
fixture.set('materials/dev/water_dudv.vtf', 'texture');
fixture.set('materials/water/tfwater001_normal.vtf', 'texture');
const found = new Set(), queue = ['materials/water/test.vmt', 'materials/water/water_warp.vmt', 'materials/water/water_warp_2fort.vmt'];
const context = {
  pak: {}, index: fixture, norm: text => text.toLowerCase().replace(/\\/g, '/'),
  read: value => Buffer.from(value),
  need: name => { found.add(name); },
  needMat: name => { const file = `materials/${name}.vmt`; found.add(file); queue.push(file); },
};
vm.createContext(context);
vm.runInContext(assets.slice(assets.indexOf('function scanVmt(p)'), assets.indexOf('// ---------- whole-folder includes')), context);
while (queue.length) context.scanVmt(queue.shift());
assert(found.has('materials/custom/water_warp.vmt'));
assert(found.has('materials/custom/water_normal.vtf'));
for (const name of ['water_warp', 'water_warp_2fort']) assert(found.has('materials/effects/' + name + '.vmt'));
assert(found.has('materials/dev/water_dudv.vtf'));
assert(found.has('materials/water/tfwater001_normal.vtf'));

const windowsClang = 'C:/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/Llvm/x64/bin/clang++.exe';
const compiler = process.env.CXX || (process.platform === 'win32' && fs.existsSync(windowsClang) ? windowsClang : 'clang++');
const temp = fs.mkdtempSync(path.join(os.tmpdir(), 'fstop-render-test-'));
try {
  const source = path.join(temp, 'render.cpp'), binary = path.join(temp, process.platform === 'win32' ? 'render.exe' : 'render');
  fs.writeFileSync(source, harness);
  const compile = cp.spawnSync(compiler, ['-std=c++11', '-Wall', '-Wextra', '-Werror', source, '-o', binary], { encoding: 'utf8' });
  assert.equal(compile.status, 0, compile.error?.message || compile.stdout + compile.stderr);
  const run = cp.spawnSync(binary, [], { encoding: 'utf8' });
  assert.equal(run.status, 0, `native harness failed (${run.status}): ${run.error?.message || run.stdout + run.stderr}`);
  console.log('PASS: water/underwater bounds/fog/clipping, overlays, portal queries/depth/exit planes, door lifecycle/scale/blocker/player collision matrix, camera sequence validity, GLES caps, ghost restoration/bounds/visibility, rotated trigger bounds, physics branches, inert clone factory/sync order');
} finally {
  fs.rmSync(temp, { recursive: true, force: true });
}
