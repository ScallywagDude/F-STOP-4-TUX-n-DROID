//========= Copyright (c) 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "usermessages.h"
#include "shake.h"
#include "voice_gamemgr.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void RegisterUserMessages()
{
	//copy/paste from hl2
	FStopUserMessages()->Register( "Geiger", 1 );
	FStopUserMessages()->Register( "Train", 1 );
	FStopUserMessages()->Register( "HudText", -1 );
	FStopUserMessages()->Register( "SayText", -1 );
	FStopUserMessages()->Register( "SayText2", -1 );
	FStopUserMessages()->Register( "TextMsg", -1 );
	FStopUserMessages()->Register( "HudMsg", -1 );
	FStopUserMessages()->Register( "ResetHUD", 1);		// called every respawn
	FStopUserMessages()->Register( "GameTitle", 0 );
	FStopUserMessages()->Register( "ItemPickup", -1 );
	FStopUserMessages()->Register( "ShowMenu", -1 );
	FStopUserMessages()->Register( "Shake", 13 );
	FStopUserMessages()->Register( "Tilt", 22 );
	FStopUserMessages()->Register( "Fade", 10 );
	FStopUserMessages()->Register( "VGUIMenu", -1 );	// Show VGUI menu
	FStopUserMessages()->Register( "Rumble", 3 );	// Send a rumble to a controller
	FStopUserMessages()->Register( "Battery", 2 );
	FStopUserMessages()->Register( "Damage", 18 );		// BUG: floats are sent for coords, no variable bitfields in hud & fixed size Msg
	FStopUserMessages()->Register( "VoiceMask", VOICE_MAX_PLAYERS_DW*4 * 2 + 1 );
	FStopUserMessages()->Register( "RequestState", 0 );
	FStopUserMessages()->Register( "CloseCaption", -1 ); // Show a caption (by string id number)(duration in 10th of a second)
	FStopUserMessages()->Register( "HintText", -1 );	// Displays hint text display
	FStopUserMessages()->Register( "KeyHintText", -1 );	// Displays hint text display
	FStopUserMessages()->Register( "SquadMemberDied", 0 );
	FStopUserMessages()->Register( "AmmoDenied", 2 );
	FStopUserMessages()->Register( "CreditsMsg", 1 );
	FStopUserMessages()->Register( "LogoTimeMsg", 4 );
	FStopUserMessages()->Register( "AchievementEvent", -1 );
	FStopUserMessages()->Register( "UpdateJalopyRadar", -1 );

	//new stuff for portal
	FStopUserMessages()->Register( "CreditsPortalMsg", 1 );
	FStopUserMessages()->Register( "EntityPortalled", sizeof( long ) + sizeof( long ) + sizeof( Vector ) + sizeof( QAngle ) ); //something got teleported through a portal
	FStopUserMessages()->Register( "KillCam", -1 );	

#ifdef FSTOP_PORTAL2
	FStopUserMessages()->Register( "InventoryFlash", sizeof( float ) + 1 );
	FStopUserMessages()->Register( "IndicatorFlash", sizeof( float ) + 1 );
	FStopUserMessages()->Register( "ControlHelperAnimate", 2 );
	FStopUserMessages()->Register( "TakePhoto", sizeof( long ) + sizeof( uint8 ) );
	FStopUserMessages()->Register( "Flash", sizeof( float ) + sizeof( Vector ) );
#endif // FSTOP_PORTAL2
}