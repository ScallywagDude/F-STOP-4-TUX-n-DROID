#include "basemodui_compat.h"
#include "cbase.h"
#include "vingamemainmenu.h"
#include "vgenericconfirmation.h"
#include "vfooterpanel.h"
#include "vgui/ISurface.h"
#include "tier0/memdbgon.h"

using namespace vgui;
using namespace BaseModUI;

InGameMainMenu::InGameMainMenu( Panel *parent, const char *panelName ) :
	BaseClass( parent, panelName, false, true )
{
	SetDeleteSelfOnClose( true );
	SetProportional( true );
	SetTitle( "", false );
	SetDialogTitle( NULL );
	SetMoveable( false );
	SetSizeable( false );
	SetFooterEnabled( false );
}

void InGameMainMenu::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	SetBgColor( Color( 0, 0, 0, 0 ) );
}

void InGameMainMenu::PaintBackground()
{
}

void InGameMainMenu::OnKeyCodePressed( KeyCode code )
{
	KeyCode baseCode = GetBaseButtonCode( code );
	if ( baseCode == KEY_XBUTTON_B || baseCode == KEY_XBUTTON_START || baseCode == STEAMCONTROLLER_B || baseCode == STEAMCONTROLLER_START || baseCode == KEY_ESCAPE )
	{
		engine->ClientCmd_Unrestricted( "gameui_hide\n" );
		return;
	}
	BaseClass::OnKeyCodePressed( code );
}

void InGameMainMenu::OnCommand( const char *command )
{
	if ( !V_stricmp( command, "ReturnToGame" ) )
	{
		engine->ClientCmd_Unrestricted( "gameui_hide\n" );
	}
	else if ( !V_stricmp( command, "SaveGame" ) )
	{
		CBaseModPanel::GetSingleton().OpenWindow( WT_SAVEGAME, this, true );
	}
	else if ( !V_stricmp( command, "LoadGame" ) )
	{
		CBaseModPanel::GetSingleton().OpenWindow( WT_LOADGAME, this, true );
	}
	else if ( !V_stricmp( command, "NewGame" ) )
	{
		CBaseModPanel::GetSingleton().OpenWindow( WT_NEWGAME, this, true );
	}
	else if ( !V_stricmp( command, "Options" ) )
	{
		CBaseModPanel::GetSingleton().OpenWindow( WT_OPTIONS, this, true );
	}
	else if ( !V_stricmp( command, "DeveloperConsole" ) )
	{
		engine->ClientCmd_Unrestricted( "gamemenucommand OpenConsole\n" );
	}
	else if ( !V_stricmp( command, "ExitToMainMenu" ) || !V_stricmp( command, "QuitGame" ) )
	{
		bool bQuit = !V_stricmp( command, "QuitGame" );
		GenericConfirmation *pConfirm = static_cast< GenericConfirmation * >( CBaseModPanel::GetSingleton().OpenWindow( WT_GENERICCONFIRMATION, this, false ) );
		if ( pConfirm )
		{
			GenericConfirmation::Data_t data;
			data.pWindowTitle = bQuit ? "#L4D360UI_MainMenu_Quit_Confirm" : "#L4D360UI_LeaveMultiplayerConf";
			data.pMessageText = bQuit ? "#L4D360UI_MainMenu_Quit_ConfirmMsg" : "#PORTAL2_InGameMainMenu_SaveWarning";
			data.bOkButtonEnabled = true;
			data.pfnOkCallback = bQuit ? &AcceptQuitCallback : &AcceptLeaveCallback;
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

void InGameMainMenu::AcceptLeaveCallback()
{
	engine->ClientCmd_Unrestricted( "disconnect\n" );
}

void InGameMainMenu::AcceptQuitCallback()
{
	engine->ClientCmd_Unrestricted( "quit\n" );
}
