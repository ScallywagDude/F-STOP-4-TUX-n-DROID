//=============================================================================
// F-STOP user messages on the CS:GO partner engine; see fstop_usermessages.h.
//=============================================================================
#include "cbase.h"
#include "fstop_usermessages.h"
#include "usermessages.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CFStopUserMessages *FStopUserMessages()
{
	static CFStopUserMessages s_Messages;
	return &s_Messages;
}

#ifdef CLIENT_DLL

class CFStopRawMessageReceiver
{
public:
	void Bind()
	{
		if ( !m_Binder.IsBound() )
			m_Binder.Bind< CS_UM_FStopRaw, CCSUsrMsg_FStopRaw >( UtlMakeDelegate( this, &CFStopRawMessageReceiver::OnMessage ) );
	}

	bool OnMessage( const CCSUsrMsg_FStopRaw &msg )
	{
		return FStopUserMessages()->Dispatch( msg.name().c_str(), msg.data().data(), (int)msg.data().size() );
	}

private:
	CUserMessageBinder m_Binder;
};

static CFStopRawMessageReceiver s_RawReceiver;

void CFStopUserMessages::HookMessage( const char *pszName, pfnUserMsgHook pfnHook )
{
	s_RawReceiver.Bind();
	int i = m_Hooks.Find( pszName );
	if ( i == m_Hooks.InvalidIndex() )
		m_Hooks.Insert( pszName, pfnHook );
	else
		m_Hooks[i] = pfnHook;
}

bool CFStopUserMessages::Dispatch( const char *pszName, const void *pData, int nBytes )
{
	int i = m_Hooks.Find( pszName );
	if ( i == m_Hooks.InvalidIndex() )
	{
		DevMsg( "F-STOP user message %s has no handler\n", pszName );
		return true;
	}
	bf_read buf( pszName, pData, nBytes );
	m_Hooks[i]( buf );
	return true;
}

#endif // CLIENT_DLL
