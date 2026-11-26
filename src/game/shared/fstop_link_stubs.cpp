//=============================================================================
// F-STOP on the CS:GO partner engine: definitions the CS:GO base code links against but that live
// in CS:GO-only files (cstrike15/, killer replay, GC, workshop, votes, Steam stats). F-STOP leaves
// those systems out, so these keep their CS:GO defaults or do nothing.
//=============================================================================
#include "cbase.h"
#include "steam/steamclientpublic.h"

#ifdef CLIENT_DLL
#include "hltvreplaysystem.h"
#include "baseviewmodel_shared.h"
#include "sourcevr/isourcevirtualreality.h"
#include "netmessages.h"
#else
#include "gameinterface.h"
#include "eiface.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// ConVars defined by the CS:GO game rules / player files (same names, defaults and flags)
//-----------------------------------------------------------------------------
ConVar game_type( "game_type", "0", FCVAR_REPLICATED | FCVAR_RELEASE | FCVAR_GAMEDLL | FCVAR_CLIENTDLL, "The current game type. See GameModes.txt." );
ConVar sv_coaching_enabled( "sv_coaching_enabled", "0", FCVAR_REPLICATED | FCVAR_RELEASE, "Allows spectating and communicating with a team ( 'coach t' or 'coach ct' )" );
ConVar sv_max_allowed_net_graph( "sv_max_allowed_net_graph", "1", FCVAR_NOTIFY | FCVAR_RELEASE | FCVAR_REPLICATED, "Determines max allowed net_graph value for clients." );
ConVar sv_disable_motd( "sv_disable_motd", "1", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, "Prevent the motd from showing." );
ConVar sv_force_reflections( "sv_force_reflections", "0", FCVAR_RELEASE | FCVAR_REPLICATED );
ConVar sv_penetration_type( "sv_penetration_type", "1", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "What type of penertration to use. 0 = old CS, 1 = new penetration" );
ConVar sv_server_graphic1( "sv_server_graphic1", "", FCVAR_REPLICATED | FCVAR_RELEASE, "A 360x60 (<16kb) image file that will be displayed to spectators." );
ConVar sv_server_graphic2( "sv_server_graphic2", "", FCVAR_REPLICATED | FCVAR_RELEASE, "A 220x45 (<16kb) image file that will be displayed to spectators." );
ConVar sv_falldamage_to_below_player_multiplier( "sv_falldamage_to_below_player_multiplier", "1", FCVAR_REPLICATED | FCVAR_RELEASE, "Scale damage when distributed across two players" );
ConVar sv_falldamage_to_below_player_ratio( "sv_falldamage_to_below_player_ratio", "0", FCVAR_REPLICATED | FCVAR_RELEASE, "Landing on a another player's head gives them this ratio of the damage." );
ConVar mp_verbose_changelevel_spew( "mp_verbose_changelevel_spew", "1", FCVAR_RELEASE | FCVAR_REPLICATED );
ConVar sv_deadtalk( "sv_deadtalk", "0", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_RELEASE, "Dead players can speak (voice, text) to the living" );
ConVar mp_solid_teammates( "mp_solid_teammates", "1", FCVAR_REPLICATED | FCVAR_RELEASE, "How solid are teammates: 0 = transparent; 1 = fully solid; 2 = can stand on top of heads" );

//-----------------------------------------------------------------------------
// CSteamID's string constructor lives in common/steamid.cpp, which needs the Steam build setup.
// Parses "STEAM_X:Y:Z" and "[U:1:N]"; anything else is an invalid ID.
//-----------------------------------------------------------------------------
CSteamID::CSteamID( const char *pchSteamID, EUniverse eDefaultUniverse )
{
	m_steamid.m_unAll64Bits = 0;
	if ( !pchSteamID )
		return;

	unsigned int unX = 0, unY = 0, unZ = 0;
	if ( sscanf( pchSteamID, "STEAM_%u:%u:%u", &unX, &unY, &unZ ) == 3 )
	{
		Set( unZ * 2 + unY, eDefaultUniverse != k_EUniverseInvalid ? eDefaultUniverse : k_EUniversePublic, k_EAccountTypeIndividual );
	}
	else if ( sscanf( pchSteamID, "[U:%u:%u]", &unX, &unY ) == 2 )
	{
		Set( unY, (EUniverse)unX, k_EAccountTypeIndividual );
	}
}

#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Killer replay (CS:GO HLTV replay) is off: it never starts, so every query says "not replaying".
//-----------------------------------------------------------------------------
CHltvReplaySystem g_HltvReplaySystem;

CHltvReplaySystem::CLocalPlayerProps::CLocalPlayerProps() : m_bLastSeenAlive( false ), m_nLastTickUpdated( 0 ) {}
CHltvReplaySystem::CHltvReplaySystem() {}
CHltvReplaySystem::~CHltvReplaySystem() {}
void CHltvReplaySystem::OnHltvReplay( const CSVCMsg_HltvReplay &msg ) {}
void CHltvReplaySystem::OnHltvReplayTick() {}
bool CHltvReplaySystem::IsInPermanentReplay() const { return false; }
int CHltvReplaySystem::GetHltvReplayDelay() const { return 0; }
void CHltvReplaySystem::StopHltvReplay() {}
void CHltvReplaySystem::OnLevelInit() {}
void CHltvReplaySystem::OnLevelShutdown() {}
void CHltvReplaySystem::OnDemoPlayback( bool bPlaying ) {}
void CHltvReplaySystem::SetDemoPlaybackHighlightXuid( uint64 xuid, bool bLowlights ) {}
void CHltvReplaySystem::SetDemoPlaybackFadeBrackets( int nCurrentPlaybackTick, int nStartAt, int nStopAt ) {}
void CHltvReplaySystem::FireGameEvent( IGameEvent *event ) {}
void CHltvReplaySystem::Update() {}

int CL_GetHltvReplayDelay() { return 0; }
bool CL_HltvReplayOnDeath() { return false; }
bool CL_HltvReplayPassThroughRulesActive() { return false; }

//-----------------------------------------------------------------------------
// CS:GO graffiti (player decals) is left out.
//-----------------------------------------------------------------------------
void OnPlayerDecalsUpdate() {}
void OnPlayerDecalsLevelShutdown() {}
void TE_PlayerDecal( IRecipientFilter& filter, float delay, const Vector* pos, const Vector* start, const Vector* right, int player, int entity, int hitbox, int nAdditionalDecalFlags ) {}

// CS:GO fills this with nearby players for its proximity shaders; F-STOP has none.
CUtlVector< Vector4D > g_vNearbyPlayerOriginsInLocalView;

// No VR on this port.
ISourceVirtualReality *g_pSourceVR = NULL;

ConVar cl_righthand( "cl_righthand", "1", FCVAR_ARCHIVE | FCVAR_SS, "Use right-handed view models." );

// 1.1 kept these in in_mouse.cpp and baseviewmodel_shared.cpp (mouse inversion when upside down,
// view model lag toggled by the portal player). TODO: hook them back into the CS:GO input / view model code.
bool g_bUpsideDown = false;
float g_fMaxViewModelLag = 1.5f;

// The portal code reconciles physics props near the player before predicting; F-STOP doesn't predict physics props.
void MoveUnpredictedPhysicsNearPlayerToNetworkedPosition( CBasePlayer *pPlayer ) {}

//-----------------------------------------------------------------------------
// View model attachments (CS:GO arms and scope masks) normally come from the econ entity code.
//-----------------------------------------------------------------------------
bool C_ViewmodelAttachmentModel::InitializeAsClientEntity( const char *pszModelName, bool bRenderWithViewModels )
{
	if ( !BaseClass::InitializeAsClientEntity( pszModelName, bRenderWithViewModels ) )
		return false;

	AddEffects( EF_BONEMERGE );
	AddEffects( EF_BONEMERGE_FASTCULL );

	// Invisible by default, and made visible->drawn->made invisible when the viewmodel is drawn
	AddEffects( EF_NODRAW );
	return true;
}

int C_ViewmodelAttachmentModel::InternalDrawModel( int flags, const RenderableInstance_t &instance )
{
	return BaseClass::InternalDrawModel( flags, instance );
}

void C_ViewmodelAttachmentModel::SetViewmodel( C_BaseViewModel *pVM )
{
	m_hViewmodel = pVM;
}

#else // GAME_DLL

//-----------------------------------------------------------------------------
// CS:GO server interface hooks (GC, tournaments, casters, workshop): F-STOP doesn't use them.
//-----------------------------------------------------------------------------
void ClientFullyConnect( edict_t *pEntity ) {}

bool CServerGameDLL::ValidateAndAddActiveCaster( const CSteamID &steamID, bool bAdd ) { return false; }
void CServerGameDLL::UpdateGCInformation() {}
void CServerGameDLL::OnPureServerFileValidationFailure( edict_t *edictClient, const char *path, const char *fileName, uint32 crc, int32 hashType, int32 len, int packNumber, int packFileID ) {}
void CServerGameDLL::OnEngineClientNetworkEvent( edict_t *edictClient, uint64 ullSteamID, int nEventType, void *pvParam ) {}
void CServerGameDLL::GetMatchmakingGameData( char *buf, size_t bufSize ) { if ( buf && bufSize ) buf[0] = 0; }
void CServerGameDLL::EngineGotvSyncPacket( const CEngineGotvSyncPacket *pPkt ) {}
EncryptedMessageKeyType_t CServerGameDLL::GetMessageEncryptionKey( INetMessage *pMessage ) { return kEncryptedMessageKeyType_None; }
char const *CServerGameDLL::ClientConnectionValidatePreNetChan( bool bGameServer, char const *adr, int nAuthProtocol, uint64 ullSteamID ) { return NULL; } // allow connections
bool CServerGameDLL::ShouldHoldGameServerReservation( float flTimeElapsedWithoutClients ) { return false; }
bool CServerGameDLL::ReportGCQueuedMatchStart( int32 iReservationStage, uint32 *puiConfirmedAccounts, int numConfirmedAccounts ) { return false; }
bool CServerGameDLL::OnEngineClientProxiedRedirect( uint64 ullClient, const char *adrProxiedRedirect, const char *adrRegular ) { return false; }
bool CServerGameDLL::LogForHTTPListeners( const char* szLogLine ) { return false; }
bool CServerGameDLL::ApplyGameSettings( KeyValues *pKV ) { return false; }

void CServerGameClients::NetworkIDValidated( const char *pszUserName, const char *pszNetworkID, CSteamID steamID ) {}
void CServerGameClients::ClientSvcUserMessage( edict_t *pEntity, int nType, int nPassthrough, uint32 cbSize, const void *pvBuffer ) {}
const char *CServerGameClients::ClientNameHandler( uint64 xuid, const char *pchName ) { return pchName; }

#endif // CLIENT_DLL
