//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef PORTAL_CLIENTMODE_H
#define PORTAL_CLIENTMODE_H
#ifdef _WIN32
#pragma once
#endif

#include "clientmode_shared.h"
#include <vgui_controls/EditablePanel.h>
#include <vgui/Cursor.h>
#include "c_colorcorrection.h"

class CHudViewport;

namespace vgui
{
	typedef unsigned long HScheme;
}

class ClientModePortalNormal : public ClientModeShared 
{
DECLARE_CLASS( ClientModePortalNormal, ClientModeShared );

private:

// IClientMode overrides.
public:

					ClientModePortalNormal();
	virtual			~ClientModePortalNormal();

	virtual void	Init();
	virtual void	InitViewport();
	virtual bool	ShouldDrawViewModel();

	// This engine leaves color correction and glow to the client mode.
	virtual void	OnColorCorrectionWeightsReset( void );
	virtual float	GetColorCorrectionScale( void ) const { return 1.0f; }
	virtual void	DoPostScreenSpaceEffects( const CViewSetup *pSetup );
	
private:
	CHandle<C_ColorCorrection> m_hCurrentColorCorrection;
	
	//	void	UpdateSpectatorMode( void );

};


extern IClientMode *GetClientModeNormal();
extern ClientModePortalNormal* GetClientModePortalNormal();


#endif // PORTAL_CLIENTMODE_H
