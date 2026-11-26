//=============================================================================
// F-STOP game code on the CS:GO partner engine: the few spellings that changed between the
// Alien Swarm-era branch F-STOP was written against and this one. Included at the end of the
// client and server cbase.h when building F-STOP (FSTOP_PORTAL).
//=============================================================================
#ifndef FSTOP_COMPAT_H
#define FSTOP_COMPAT_H
#ifdef FSTOP_PORTAL

// F-STOP code calls min()/max() unqualified, with mixed types too; this branch only has MIN/MAX.
// (Skipped where windows.h or another header defined min/max as macros.)
#if !defined( min ) && !defined( max )
#include <algorithm>
using std::min;
using std::max;
inline float min( int a, float b ) { return ( (float)a < b ) ? (float)a : b; }
inline float min( float a, int b ) { return ( a < (float)b ) ? a : (float)b; }
inline float min( float a, double b ) { return ( a < (float)b ) ? a : (float)b; }
inline float min( double a, float b ) { return ( (float)a < b ) ? (float)a : b; }
inline float max( int a, float b ) { return ( (float)a > b ) ? (float)a : b; }
inline float max( float a, int b ) { return ( a > (float)b ) ? a : (float)b; }
inline float max( float a, double b ) { return ( a > (float)b ) ? a : (float)b; }
inline float max( double a, float b ) { return ( (float)a > b ) ? (float)a : b; }
#endif

// F-STOP 1.1: tunnel fields block physical objects while letting every player through.
#define COLLISION_GROUP_PORTAL_TUNNEL_BLOCKER ( LAST_SHARED_COLLISION_GROUP + 42 )

// basegrenade/physics shared code uses the CS:GO grenade hull size (cs_shareddefs.h).
#define GRENADE_DEFAULT_SIZE 2.0

// The AI class list ends in LAST_SHARED_ENTITY_CLASS here (shareddefs.h).
#define NUM_AI_CLASSES LAST_SHARED_ENTITY_CLASS

// CS:GO base code compares against the CS teams in many places; F-STOP's player is never on them,
// so those branches just don't run. (Values from cs_shareddefs.h.)
#ifndef TEAM_TERRORIST
#define TEAM_TERRORIST 2
#define TEAM_CT 3
#endif
// Sentinel used by the base weapon selection code (cstrike15_item_constants.h).
#define GEAR_SLOTPOSITION_MAX 9999
// HL2 weapon slot count, standing in for the CS:GO gear slots
#define GEAR_SLOT_COUNT 6

// CS:GO movement constants the shared player code still names (values from cs_shareddefs.h)
#define CS_PLAYER_SPEED_RUN 260.0f
#define CS_PLAYER_DUCK_SPEED_IDEAL 8.0f
#ifndef MAX_DECORATED_PLAYER_NAME_LENGTH
#define MAX_DECORATED_PLAYER_NAME_LENGTH ( ( MAX_NETWORKID_LENGTH * 10 ) + 20 )
#endif

#ifndef IsXbox
#define IsXbox() false
#endif

// Mouse wheel direction passed to CBaseCombatWeapon::OnMouseWheel (camera zoom, placement tool).
#ifndef MWHEEL_UP
#define MWHEEL_UP 1
#define MWHEEL_DOWN -1
#endif

// Mouse wheel notches reach the server as impulses in the user command, like the flashlight,
// so they ride the normal input stream (one per command) instead of one string command per notch.
#define FSTOP_IMPULSE_MWHEEL_UP		250
#define FSTOP_IMPULSE_MWHEEL_DOWN	251

// Renamed console flag.
#ifndef FCVAR_ARCHIVE_XBOX
#define FCVAR_ARCHIVE_XBOX FCVAR_ARCHIVE_GAMECONSOLE
#endif

#ifdef CLIENT_DLL
// The client mode is per split screen slot here.
#define g_pClientMode GetClientMode()

// The CS:GO client gets ILocalize through its CS headers; F-STOP and shared code expect it from cbase.h.
#include <vgui/ILocalize.h>

// Steam headers come in through the CS headers on the CS:GO client too. F-STOP runs without a Steam
// client: steamapicontext stays uninitialised and every Steam call site checks it for NULL.
#include "steam/steam_api.h"
#endif

#include "fstop_sizescale.h"

#include "fstop_usermessages.h"

#endif // FSTOP_PORTAL
#endif // FSTOP_COMPAT_H
