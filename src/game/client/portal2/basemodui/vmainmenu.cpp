#include "basemodui_compat.h"
#include "cbase.h"
#include "vmainmenu.h"
#include "vgenericconfirmation.h"
#include "vfooterpanel.h"
#include "vhybridbutton.h"
#include "vgui/ISurface.h"
#include "tier0/memdbgon.h"

using namespace vgui;
using namespace BaseModUI;
namespace BaseModUI { void OpenTouchOptions( vgui::Panel *parent ); }

static int PanelY( Panel *p ) { int x, y; p->GetPos( x, y ); return y; }
static int PanelX( Panel *p ) { int x, y; p->GetPos( x, y ); return x; }

// The .res layouts place every row a menu can have. A row that is hidden (Continue with no save) leaves no
// hole: the rows above it move down and the backing panel shrinks to fit.
void BaseModUI::CompactMenuRows( vgui::Panel *pMenu )
{
	CUtlVector< Panel * > rows;
	for ( int i = 0; i < pMenu->GetChildCount(); ++i )
	{
		Panel *pChild = pMenu->GetChild( i );
		if ( !dynamic_cast< BaseModHybridButton * >( pChild ) )
			continue;
		int nInsert = 0;
		for ( ; nInsert < rows.Count() && PanelY( rows[nInsert] ) <= PanelY( pChild ); ++nInsert )
			;
		rows.InsertBefore( nInsert, pChild );
	}
	if ( rows.Count() < 2 )
		return;

	// Spacing and padding come from the layout as authored, before anything is hidden.
	const int nPitch = PanelY( rows[1] ) - PanelY( rows[0] );
	int nY = PanelY( rows.Tail() );
	Panel *pBacking = pMenu->FindChildByName( "Backing" );
	const int nPadTop = pBacking ? PanelY( rows[0] ) - PanelY( pBacking ) : 0;
	const int nPadBottom = pBacking ? PanelY( pBacking ) + pBacking->GetTall() - ( nY + rows.Tail()->GetTall() ) : 0;
	const int nBottom = nY + rows.Tail()->GetTall() + nPadBottom;

	int nTop = nY;
	for ( int i = rows.Count() - 1; i >= 0; --i )
	{
		if ( !rows[i]->IsVisible() )
			continue;
		rows[i]->SetPos( PanelX( rows[i] ), nY );
		nTop = nY;
		nY -= nPitch;
	}
	if ( pBacking )
	{
		pBacking->SetPos( PanelX( pBacking ), nTop - nPadTop );
		pBacking->SetTall( nBottom - ( nTop - nPadTop ) );
	}
}

MainMenu::MainMenu( Panel *parent, const char *panelName ) :
	BaseClass( parent, panelName, true, true, false, false )
{
	SetProportional( true );
	SetDeleteSelfOnClose( true );
	SetPaintBackgroundEnabled( true );
	SetTitle( "", false );
	SetMoveable( false );
	SetSizeable( false );
	SetFooterEnabled( false );
}

MainMenu::~MainMenu()
{
}

void MainMenu::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	SetBgColor( Color( 0, 0, 0, 0 ) );
}

void MainMenu::LoadLayout()
{
	BaseClass::LoadLayout();

	// Continue resumes the newest save of any kind: manual, quick or auto.
	CUtlVector< SaveGameInfo_t > saves;
	CBaseModPanel &panel = CBaseModPanel::GetSingleton();
	int nNewest = panel.GetSaveGameInfos( saves ) ? panel.GetMostRecentSaveGame( saves ) : -1;
	m_ContinueFile = nNewest >= 0 ? saves[nNewest].m_Filename.Get() : "";
	m_ContinueMap = nNewest >= 0 ? saves[nNewest].m_MapName.Get() : "";
	if ( Panel *pContinue = FindChildByName( "BtnContinue" ) )
		pContinue->SetVisible( !m_ContinueFile.IsEmpty() );

	CompactMenuRows( this );
	ConfigureMenuNavigation();
	FindAndSetActiveControl();
}

void MainMenu::PaintBackground()
{
	// The engine's menu background shows through
}

void MainMenu::OnKeyCodePressed( KeyCode code )
{
	// Nothing to go back to from the front screen
	KeyCode baseCode = GetBaseButtonCode( code );
	if ( baseCode == KEY_ESCAPE || baseCode == KEY_XBUTTON_B || baseCode == STEAMCONTROLLER_B )
		return;
	BaseClass::OnKeyCodePressed( code );
}

void MainMenu::OnCommand( const char *command )
{
	if ( !V_stricmp( command, "Continue" ) )
	{
		if ( !m_ContinueFile.IsEmpty() )
		{
			KeyValues *pSettings = new KeyValues( "FadeOutStartGame" );
			KeyValues::AutoDelete autodelete_pSettings( pSettings );
			pSettings->SetString( "map", m_ContinueMap.Get() );
			pSettings->SetString( "loadfilename", m_ContinueFile.Get() );
			pSettings->SetString( "reason", "load" );
			CBaseModPanel::GetSingleton().OpenWindow( WT_FADEOUTSTARTGAME, this, true, pSettings );
		}
	}
	else if ( !V_stricmp( command, "NewGame" ) )
	{
		CBaseModPanel::GetSingleton().OpenWindow( WT_NEWGAME, this, true );
	}
	else if ( !V_stricmp( command, "LoadGame" ) )
	{
		CBaseModPanel::GetSingleton().OpenWindow( WT_LOADGAME, this, true );
	}
	else if ( !V_stricmp( command, "Options" ) )
	{
		CBaseModPanel::GetSingleton().OpenWindow( WT_OPTIONS, this, true );
	}
	else if ( !V_stricmp( command, "DeveloperConsole" ) )
	{
		engine->ClientCmd_Unrestricted( "con_enable 1\nshowconsole\n" );
	}
	else if ( !V_stricmp( command, "QuitGame" ) )
	{
		GenericConfirmation *pConfirm = static_cast< GenericConfirmation * >( CBaseModPanel::GetSingleton().OpenWindow( WT_GENERICCONFIRMATION, this, false ) );
		if ( pConfirm )
		{
			GenericConfirmation::Data_t data;
			data.pWindowTitle = "#L4D360UI_MainMenu_Quit_Confirm";
			data.pMessageText = "#L4D360UI_MainMenu_Quit_ConfirmMsg";
			data.bOkButtonEnabled = true;
			data.pfnOkCallback = &AcceptQuitGameCallback;
			data.pOkButtonText = "#PORTAL2_ButtonAction_Quit";
			data.bCancelButtonEnabled = true;
			pConfirm->SetUsageData( data );
			NavigateFrom();
		}
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

void MainMenu::AcceptQuitGameCallback()
{
	engine->ClientCmd_Unrestricted( "quit\n" );
}

OptionsMenu::OptionsMenu( Panel *parent, const char *panelName ) :
	BaseClass( parent, panelName, false, true )
{
	SetDeleteSelfOnClose( true );
	SetProportional( true );
	SetTitle( "", false );
	SetMoveable( false );
	SetSizeable( false );
	SetFooterEnabled( false );
}

void OptionsMenu::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	SetBgColor( Color( 0, 0, 0, 0 ) );
}

void OptionsMenu::PaintBackground()
{
	// Whatever is behind the menu shows through
}

void OptionsMenu::OnCommand( const char *command )
{
	if ( !V_stricmp( command, "Video" ) )
		CBaseModPanel::GetSingleton().OpenWindow( WT_VIDEO, this, true );
	else if ( !V_stricmp( command, "AdvancedVideo" ) )
		CBaseModPanel::GetSingleton().OpenWindow( WT_ADVANCEDVIDEO, this, true );
	else if ( !V_stricmp( command, "Audio" ) )
		CBaseModPanel::GetSingleton().OpenWindow( WT_AUDIO, this, true );
	else if ( !V_stricmp( command, "KeyboardMouse" ) )
		CBaseModPanel::GetSingleton().OpenWindow( WT_KEYBOARDMOUSE, this, true );
	else if ( !V_stricmp( command, "AllSettings" ) )
		OpenTouchOptions( this );
	else if ( !V_stricmp( command, "Back" ) )
		NavigateBack();
	else
		BaseClass::OnCommand( command );
}
