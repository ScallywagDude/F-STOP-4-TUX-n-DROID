//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef HUD_CHAT_H
#define HUD_CHAT_H
#ifdef _WIN32
#pragma once
#endif

#include <hud_basechat.h>

class CHudChat : public CBaseHudChat
{
	DECLARE_CLASS_SIMPLE( CHudChat, CBaseHudChat );

public:
	explicit CHudChat( const char *pElementName );

	virtual void	Init( void );

#ifdef FSTOP_PORTAL
	// The server sends protobuf chat/text messages here; CBaseHudChat handles them.
	CUserMessageBinder m_UMCMsgSayText;
	CUserMessageBinder m_UMCMsgSayText2;
	CUserMessageBinder m_UMCMsgTextMsg;
#else
	void			MsgFunc_SayText(bf_read &msg);
	void			MsgFunc_SayText2( bf_read &msg );
	void			MsgFunc_TextMsg(bf_read &msg);
#endif
};

#endif	//HUD_CHAT_H