// F-STOP's client root panels on the CS:GO partner engine. The CS:GO client base wants a root panel per
// split screen slot plus a fullscreen root for shared HUD elements (the HL2 version only had one).
#include "cbase.h"
#include "vgui_int.h"
#include "ienginevgui.h"
#include "view_shared.h"
#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

class CFStopRootPanel : public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CFStopRootPanel, vgui::Panel );
public:
	CFStopRootPanel( vgui::VPANEL parent, int slot, const char *panelName )
		: BaseClass( NULL, panelName ), m_nSplitSlot( slot )
	{
		SetParent( parent );
		SetPaintEnabled( false );
		SetPaintBorderEnabled( false );
		SetPaintBackgroundEnabled( false );

		int w, h;
		surface()->GetScreenSize( w, h );
		SetBounds( 0, 0, w, h );
		SetZPos( 0 );
	}

	virtual void PaintTraverse( bool Repaint, bool allowForce = true )
	{
		ACTIVE_SPLITSCREEN_PLAYER_GUARD_VGUI( m_nSplitSlot );
		BaseClass::PaintTraverse( Repaint, allowForce );
	}

	virtual void OnThink()
	{
		ACTIVE_SPLITSCREEN_PLAYER_GUARD_VGUI( m_nSplitSlot );
		BaseClass::OnThink();
	}

private:
	int m_nSplitSlot;
};

static CFStopRootPanel *g_pRootPanel[ MAX_SPLITSCREEN_PLAYERS ];
static CFStopRootPanel *g_pFullscreenRootPanel;

void VGui_GetPanelList( CUtlVector< Panel * > &list )
{
	for ( int i = 0 ; i < MAX_SPLITSCREEN_PLAYERS; ++i )
	{
		list.AddToTail( g_pRootPanel[ i ] );
	}
}

void VGUI_CreateClientDLLRootPanel( void )
{
	for ( int i = 0 ; i < MAX_SPLITSCREEN_PLAYERS; ++i )
	{
		g_pRootPanel[ i ] = new CFStopRootPanel( enginevgui->GetPanel( PANEL_CLIENTDLL ), i, "F-STOP Root Panel" );
	}

	g_pFullscreenRootPanel = new CFStopRootPanel( enginevgui->GetPanel( PANEL_CLIENTDLL ), 0, "Fullscreen Root Panel" );
	g_pFullscreenRootPanel->SetZPos( 1 );
}

void VGUI_DestroyClientDLLRootPanel( void )
{
	for ( int i = 0 ; i < MAX_SPLITSCREEN_PLAYERS; ++i )
	{
		delete g_pRootPanel[ i ];
		g_pRootPanel[ i ] = NULL;
	}

	delete g_pFullscreenRootPanel;
	g_pFullscreenRootPanel = NULL;
}

vgui::VPANEL VGui_GetClientDLLRootPanel( void )
{
	ASSERT_LOCAL_PLAYER_RESOLVABLE();
	return g_pRootPanel[ GET_ACTIVE_SPLITSCREEN_SLOT() ]->GetVPanel();
}

vgui::Panel *VGui_GetFullscreenRootPanel( void )
{
	return g_pFullscreenRootPanel;
}

vgui::VPANEL VGui_GetFullscreenRootVPANEL( void )
{
	return g_pFullscreenRootPanel->GetVPanel();
}

void VGui_GetAllSubViews( int nSlot, const CViewSetup &orig_view, CUtlLinkedList< CViewSetup > &subviews, CUtlVector< vrect_t > &letterbox )
{
	subviews.AddToTail( orig_view );
}
