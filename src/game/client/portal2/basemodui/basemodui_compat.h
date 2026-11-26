// BaseModUI on the CS:GO partner engine. The engine and game base already provide the split screen,
// XBX_* user slot and navigation helpers the old F-STOP engine lacked, so only the copy protection
// markers (no CEG in this build), the Steam switch and the GameUI ConVar wrapper name are left here.
#ifndef BASEMODUI_COMPAT_H
#define BASEMODUI_COMPAT_H

#ifndef NO_STEAM
#define NO_STEAM 1
#endif

#ifndef CEG_NOINLINE
#define CEG_NOINLINE
#define CEG_PROTECT_MEMBER_FUNCTION( x )
#define CEG_PROTECT_VIRTUAL_FUNCTION( x )
#define CEG_PROTECT_STATIC_MEMBER_FUNCTION( x, y )
#define CEG_ENCRYPT_FUNCTION( x )
#define CEG_DECRYPT_FUNCTION( x )
#endif

// CS:GO's GameUI has its own ConVarRef wrapper (cstrike15/gameui/gameui_util.h); a plain ConVarRef does the same here.
#ifndef CGameUIConVarRef
#define CGameUIConVarRef ConVarRef
#endif

#endif
