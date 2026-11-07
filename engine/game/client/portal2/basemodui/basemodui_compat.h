// Lets the BaseModUI windows build on this engine branch, which has no matchmaking framework,
// no split screen, and no console paths. Everything here answers as a single local PC player.
#ifndef BASEMODUI_COMPAT_H
#define BASEMODUI_COMPAT_H

#define NO_STEAM 1

#define CEG_NOINLINE
#define CEG_PROTECT_MEMBER_FUNCTION( x )
#define CEG_PROTECT_VIRTUAL_FUNCTION( x )
#define CEG_PROTECT_STATIC_MEMBER_FUNCTION( x, y )
#define CEG_ENCRYPT_FUNCTION( x )
#define CEG_DECRYPT_FUNCTION( x )

#define IsGameConsole() false
#define IsPS3() false

#define CGameUIConVarRef ConVarRef

inline int XBX_GetNumGameUsers() { return 1; }
inline int XBX_GetUserId( int ) { return 0; }
inline int XBX_GetActiveUserId() { return 0; }
inline bool XBX_GetPrimaryUserIsGuest() { return false; }
inline bool XBX_GetUserIsGuest( int ) { return false; }
inline void XBX_SetPrimaryUserId( int ) {}
inline void XBX_SetPrimaryUserIsGuest( int ) {}
inline void XBX_SetNumGameUsers( int ) {}
inline void XBX_ResetUserIdSlots() {}
inline int XBX_GetStorageDeviceId( int ) { return 0; }
inline void XBX_SetStorageDeviceId( int, int ) {}
inline const char *XBX_MakeStorageContainerRoot( int, const char *, char *out, int ) { return out; }
inline int GetGameUIActiveSplitScreenPlayerSlot() { return 0; }
#define ACTIVE_SPLITSCREEN_PLAYER_GUARD( x )
#define ACTIVE_SPLITSCREEN_PLAYER_GUARD_VGUI( x )
#define FOR_EACH_VALID_SPLITSCREEN_PLAYER( x ) for ( int x = 0; x < 1; ++x )
#define XBX_INVALID_USER_ID -1
#define XBX_STORAGE_DECLINED -1


#define PLATFORM_EXT ""
#define PRI_WS_FOR_WS L"%ls"
#define Q_snwprintf V_snwprintf
#define AreBoundsValid( a, b ) ( (a).x <= (b).x && (a).y <= (b).y )
#define KEY_XBUTTON_INACTIVE_START SK_BUTTON_INACTIVE_START

// Keyboard and pad navigation holds no lock on this branch
struct NavLockShim { operator int() const { return 0; } void operator=( int ) {} };
static NavLockShim s_NavLock;

#endif
