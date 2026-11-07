#include "basemodui_compat.h"
#include "cbase.h"
#include "vmainmenu.h"
#include "vgenericconfirmation.h"
#include "vfooterpanel.h"
#include "vgui/ISurface.h"
#include "tier0/memdbgon.h"

using namespace vgui;
using namespace BaseModUI;

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

void MainMenu::PaintBackground()
{
	// The engine's menu background shows through
}

void MainMenu::OnKeyCodePressed( KeyCode code )
{
	// Nothing to go back to from the front screen
	if ( code == KEY_ESCAPE || code == KEY_XBUTTON_B )
		return;
	BaseClass::OnKeyCodePressed( code );
}

void MainMenu::OnCommand( const char *command )
{
	if ( !V_stricmp( command, "NewGame" ) )
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
		engine->ClientCmd_Unrestricted( "gamemenucommand OpenConsole\n" );
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
		engine->ClientCmd_Unrestricted( "gamemenucommand OpenOptionsDialog\n" );
	else if ( !V_stricmp( command, "Back" ) )
		NavigateBack();
	else
		BaseClass::OnCommand( command );
}
