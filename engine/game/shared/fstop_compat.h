//=============================================================================
// Small differences between the engine branch F-Stop was written against and this one.
//=============================================================================
#ifndef FSTOP_COMPAT_H
#define FSTOP_COMPAT_H
#ifdef PORTAL2

// F-Stop's branch allowed min()/max() on mixed numeric types.
inline float min( int a, float b ) { return ( (float)a < b ) ? (float)a : b; }
inline float min( float a, int b ) { return ( a < (float)b ) ? a : (float)b; }
inline float min( float a, double b ) { return ( a < (float)b ) ? a : (float)b; }
inline float min( double a, float b ) { return ( (float)a < b ) ? (float)a : b; }
inline float min( int a, double b ) { return ( (float)a < (float)b ) ? (float)a : (float)b; }
inline float min( double a, int b ) { return ( (float)a < (float)b ) ? (float)a : (float)b; }
inline float max( int a, float b ) { return ( (float)a > b ) ? (float)a : b; }
inline float max( float a, int b ) { return ( a > (float)b ) ? a : (float)b; }
inline float max( float a, double b ) { return ( a > (float)b ) ? a : (float)b; }
inline float max( double a, float b ) { return ( (float)a > b ) ? (float)a : b; }
inline float max( int a, double b ) { return ( (float)a > (float)b ) ? (float)a : (float)b; }
inline float max( double a, int b ) { return ( (float)a > (float)b ) ? (float)a : (float)b; }

#define MWHEEL_UP 1
#define MWHEEL_DOWN -1

// Effects don't need precaching on this branch.
#define PrecacheEffect( name ) ((void)0)

// Collision groups used by the camera. They only ever collide with their own group.
#define COLLISION_GROUP_PLACEMENT_SOLID ( LAST_SHARED_COLLISION_GROUP + 40 )
#define COLLISION_GROUP_CAMERA_SOLID    ( LAST_SHARED_COLLISION_GROUP + 41 )
// Tunnel fields block physical objects while allowing every player through.
#define COLLISION_GROUP_PORTAL_TUNNEL_BLOCKER ( LAST_SHARED_COLLISION_GROUP + 42 )

#ifdef CLIENT_DLL
// Materials load on first use on this branch, so the registration blocks compile to nothing.
#define PRECACHE_REGISTER_BEGIN( group, name ) static inline void name##_unused( void ) {
#define PRECACHE( type, name ) ;
#define PRECACHE_REGISTER_END() }
#define GetClientMode() g_pClientMode
#endif

#endif // PORTAL2
#endif // FSTOP_COMPAT_H
