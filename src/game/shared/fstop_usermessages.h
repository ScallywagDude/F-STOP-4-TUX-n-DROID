//=============================================================================
// F-STOP user messages on the CS:GO partner engine.
//
// The engine and the CS:GO base send protobuf user messages. F-STOP's HL2 and Portal code still
// writes its messages bytewise (UserMessageBegin / WRITE_* / MessageEnd) and reads them with
// bf_read in handlers hooked by name. Those messages travel inside one protobuf message,
// CCSUsrMsg_FStopRaw (name + bytes), so the F-STOP code keeps its original layout.
//=============================================================================
#ifndef FSTOP_USERMESSAGES_H
#define FSTOP_USERMESSAGES_H
#ifdef FSTOP_PORTAL

#include "tier1/utldict.h"
#include "tier1/bitbuf.h"

typedef void ( *pfnUserMsgHook )( bf_read &msg );

class CFStopUserMessages
{
public:
	// Message sizes were only used by the old engine to validate; the name is the identity here.
	void Register( const char *pszName, int nSize ) {}

#ifdef CLIENT_DLL
	void HookMessage( const char *pszName, pfnUserMsgHook pfnHook );
	bool Dispatch( const char *pszName, const void *pData, int nBytes );

private:
	CUtlDict< pfnUserMsgHook, int > m_Hooks;
#endif
};

CFStopUserMessages *FStopUserMessages();

#ifdef CLIENT_DLL
// Old-style hook macros, for handlers taking bf_read (the CS:GO ones in hud_macros.h take protobufs).
#define DECLARE_HUD_MESSAGE_BF( y, x ) static void __MsgFunc_##y##_##x( bf_read &msg ) \
	{ \
		y *pElement = ( ( y * )GET_HUDELEMENT( y ) ); \
		if ( pElement ) \
			pElement->MsgFunc_##x( msg ); \
	}
#define HOOK_HUD_MESSAGE_BF( y, x ) FStopUserMessages()->HookMessage( #x, __MsgFunc_##y##_##x )
#define DECLARE_MESSAGE_BF( y, x ) static void __MsgFunc_##y##_##x( bf_read &msg ) { y.MsgFunc_##x( msg ); }
#define HOOK_MESSAGE_BF( x ) FStopUserMessages()->HookMessage( #x, __MsgFunc_##x )
#else
class IRecipientFilter;
// Starts an old-style message; WRITE_* fill it and MessageEnd() sends it (server/gameinterface.cpp).
void UserMessageBegin( IRecipientFilter &filter, const char *pszMessageName );
#endif

#endif // FSTOP_PORTAL
#endif // FSTOP_USERMESSAGES_H
