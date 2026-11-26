// F-STOP's game UI on the CS:GO partner engine. The engine takes IGameUI and IGameConsole from client.dll
// (there is no separate GameUI.dll), so they live here, with the BaseModUI panel as the menu front end.
// The engine calls Initialize before the client's own Init, so the panel is made once the client is up.
#include "basemodui_compat.h"
#include "cbase.h"
#include "GameUI_Interface.h"
#include "GameUI/IGameConsole.h"
#include "basemodpanel.h"
#include "ienginevgui.h"
#include "engine/IEngineSound.h"
#include "vgui/ISurface.h"
#include "vgui/IVGui.h"
#include "vgui/IScheme.h"
#include "vgui/ILocalize.h"
#include "vgui_controls/consoledialog.h"
#include "vgui_controls/Controls.h"
#include "matsys_controls/matsyscontrols.h"
#include "tier0/memdbgon.h"

using namespace BaseModUI;

extern void BaseModUI_Create();

// The Xbox 360 single-player loading bar, without an opaque dialog behind it.
class CFStopLoadingProgress : public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CFStopLoadingProgress, vgui::Panel );
public:
	CFStopLoadingProgress() : BaseClass( NULL, "FStopLoadingProgress" ), m_flProgress( 0 )
	{
		SetParent( enginevgui->GetPanel( PANEL_GAMEUIBACKGROUND ) );
		SetMouseInputEnabled( false );
		SetKeyBoardInputEnabled( false );
		SetPaintBackgroundEnabled( false );
		SetPaintBorderEnabled( false );
		SetVisible( false );
	}
	bool SetProgress( float progress )
	{
		progress = clamp( progress, 0.f, 1.f );
		bool changed = m_flProgress != progress || !IsVisible();
		m_flProgress = progress;
		int wide, tall;
		vgui::surface()->GetScreenSize( wide, tall );
		const int barWide = vgui::scheme()->GetProportionalScaledValue( 300 );
		const int barTall = vgui::scheme()->GetProportionalScaledValue( 12 );
		SetBounds( ( wide - barWide ) / 2, ( tall - barTall ) * .86f, barWide, barTall );
		SetVisible( true );
		Repaint();
		return changed;
	}
	virtual void Paint()
	{
		int wide, tall;
		GetSize( wide, tall );
		DrawBox( 0, 0, wide, tall, Color( 0, 0, 0, 255 ), 1.f );
		DrawBox( 2, 2, wide - 4, tall - 4, Color( 100, 100, 100, 255 ), 1.f );
		int fill = m_flProgress * ( wide - 4 );
		if ( fill >= 12 )
			DrawBox( 2, 2, fill, tall - 4, Color( 200, 100, 0, 255 ), 1.f );
	}
private:
	float m_flProgress;
};

static vgui::DHANDLE<CFStopLoadingProgress> g_hLoadingProgress;

static CFStopLoadingProgress *LoadingProgress()
{
	if ( !g_hLoadingProgress.Get() )
		g_hLoadingProgress = new CFStopLoadingProgress();
	return g_hLoadingProgress.Get();
}

static CGameUI g_GameUI;
CGameUI &GameUI() { return g_GameUI; }
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CGameUI, IGameUI, GAMEUI_INTERFACE_VERSION, g_GameUI );

CGameUI::CGameUI() : m_bShowProgressText( false ), m_bLoading( false ), m_bPanelCreated( false ), m_nMusicGuid( 0 ), m_flNextMusicAttempt( 0 )
{
}

void CGameUI::Initialize( CreateInterfaceFn factory )
{
	// the console is made right after this, before the client's Init sets up vgui for itself
	vgui::VGui_InitInterfacesList( "GameUI", &factory, 1 );
	vgui::VGui_InitMatSysInterfacesList( "GameUI", &factory, 1 );
	// BaseModUI only supplies the Portal-specific strings. The shared video,
	// audio and keyboard pages also use the desktop GameUI token table.
	g_pVGuiLocalize->AddFile( "resource/gameui_%language%.txt", "GAME", true );
	g_pVGuiLocalize->AddFile( "resource/valve_%language%.txt", "GAME", true );
	g_pVGuiLocalize->AddFile( "resource/fstop_ui_%language%.txt", "GAME", true );
}

void CGameUI::PostInit()
{
}

void CGameUI::Connect( CreateInterfaceFn gameFactory )
{
}

void CGameUI::Start()
{
	g_pVGuiLocalize->AddFile( "Resource/platform_%language%.txt" );
	g_pVGuiLocalize->AddFile( "Resource/vgui_%language%.txt" );
}

static void ShutdownFStopConsole();

void CGameUI::Shutdown()
{
	m_bLoading = true;
	SetBackgroundMusicDesired( false );
	ShutdownFStopConsole();
	DestroyGameSpecificUI();
}

void CGameUI::EnsurePanel()
{
	if ( m_bPanelCreated || !enginevgui || !engine )
		return;
	m_bPanelCreated = true;
	BaseModUI_Create();
}

void CGameUI::CreateGameSpecificUI()
{
	EnsurePanel();
}

void CGameUI::DestroyGameSpecificUI()
{
    // Called by CHLClient::Shutdown while client VGUI is still connected.
    // Deferred panel deletion runs after DisconnectTier3Libraries, when a
    // client Frame destructor can no longer use input()/ivgui().
    ShutdownFStopConsole();
    if ( vgui::ivgui() && vgui::ipanel() )
    {
        delete g_hLoadingProgress.Get();
        if ( CBaseModPanel *pPanel = CBaseModPanel::GetSingletonPtr() )
            delete pPanel;
    }
    g_hLoadingProgress = NULL;
    m_bPanelCreated = false;
}

void CGameUI::RunFrame()
{
	EnsurePanel();
	SetBackgroundMusicDesired( !m_bLoading && !IsInLevel() );
	if ( CBaseModPanel *pPanel = CBaseModPanel::GetSingletonPtr() )
		pPanel->RunFrame( !m_bLoading && enginevgui->IsGameUIVisible() );
}

void CGameUI::OnGameUIActivated()
{
	EnsurePanel();
	if ( !m_bLoading )
	{
		// The menu over a level is a pause menu. The engine's menu check only stops the server simulation;
		// pausing the server also freezes client time (particles, animated materials, sound). The menu map keeps running.
		if ( IsInLevel() )
			engine->ClientCmd_Unrestricted( "setpause nomsg\n" );
		if ( CBaseModPanel *pPanel = CBaseModPanel::GetSingletonPtr() )
			pPanel->OnGameUIActivated();
	}
}

void CGameUI::OnGameUIHidden()
{
	engine->ClientCmd_Unrestricted( "unpause nomsg\n" );
	if ( CBaseModPanel *pPanel = CBaseModPanel::GetSingletonPtr() )
		pPanel->OnGameUIHidden();
}

void CGameUI::OnLevelLoadingStarted( const char *levelName, bool bShowProgressDialog )
{
	m_bLoading = true;
	SetBackgroundMusicDesired( false );
	OnGameUIHidden();
	LoadingProgress()->SetProgress( 0.f );
}

void CGameUI::SetBackgroundMusicDesired( bool bPlayMusic, bool bAllowFinishCurrentTrack )
{
	if ( !enginesound )
		return;
	// The engine also emits player_connect_full for menu maps.
	if ( !m_bLoading && IsInBackgroundLevel() )
		bPlayMusic = true;
	if ( !bPlayMusic )
	{
		if ( m_nMusicGuid )
			enginesound->StopSoundByGuid( m_nMusicGuid );
		m_nMusicGuid = 0;
		m_flNextMusicAttempt = 0;
		return;
	}
	ConVarRef menuVolume( "snd_menumusic_volume", true );
	float volume = menuVolume.IsValid() ? menuVolume.GetFloat() : .3f;
	if ( m_nMusicGuid && enginesound->IsSoundStillPlaying( m_nMusicGuid ) )
	{
		enginesound->SetVolumeByGuid( m_nMusicGuid, volume );
	}
	else if ( Plat_FloatTime() >= m_flNextMusicAttempt )
	{
		m_nMusicGuid = enginesound->EmitAmbientSound( "*music/tiki.mp3", volume );
		m_flNextMusicAttempt = Plat_FloatTime() + 5;
	}
}

bool CGameUI::UpdateProgressBar( float progress, const char *statusText, bool showDialog )
{
	return m_bLoading && showDialog && LoadingProgress()->SetProgress( progress );
}

void CGameUI::OnLevelLoadingFinished( bool bError, const char *failureReason, const char *extendedReason )
{
	m_bLoading = false;
	if ( g_hLoadingProgress.Get() )
		g_hLoadingProgress->SetVisible( false );
	if ( bError )
	{
		ActivateGameUI();
		OnGameUIActivated();
		if ( failureReason && failureReason[0] )
			ShowMessageDialog( failureReason, "#GameUI_Disconnected" );
	}
	else if ( IsInLevel() )
	{
		// Loading activates the engine's menu layer. Release it once gameplay is
		// ready, including its background and input capture.
		OnGameUIHidden();
		HideGameUI();
	}
	else if ( IsInBackgroundLevel() )
	{
		ActivateGameUI();
		OnGameUIActivated();
	}
}

void CGameUI::ShowMessageDialog( const char *messageID, const char *titleID )
{
	if ( CBaseModPanel *pPanel = CBaseModPanel::GetSingletonPtr() )
		pPanel->OpenMessageDialog( titleID, messageID );
	else
		Warning( "%s: %s\n", titleID ? titleID : "", messageID ? messageID : "" );
}

void CGameUI::CreateCommandMsgBox( const char *pszTitle, const char *pszMessage, bool showOk, bool showCancel, const char *okCommand, const char *cancelCommand, const char *closedCommand, const char *pszLegend, const char *szPanelID )
{
	ShowMessageDialog( pszMessage, pszTitle );
}

void CGameUI::CreateCommandMsgBoxInSlot( ECommandMsgBoxSlot slot, const char *pszTitle, const char *pszMessage, bool showOk, bool showCancel, const char *okCommand, const char *cancelCommand, const char *closedCommand, const char *pszLegend )
{
	ShowMessageDialog( pszMessage, pszTitle );
}

bool CGameUI::IsInLevel()
{
	if ( !engine )
		return false;
	const char *pLevel = engine->GetLevelName();
	return pLevel && pLevel[0] && !engine->IsLevelMainMenuBackground();
}

bool CGameUI::IsInBackgroundLevel()
{
	return engine && engine->IsLevelMainMenuBackground();
}

void CGameUI::RestoreTopLevelMenu()
{
	if ( CBaseModPanel *pPanel = CBaseModPanel::GetSingletonPtr() )
		pPanel->OpenFrontScreen();
}

void CGameUI::PreventEngineHideGameUI() { engine->ClientCmd_Unrestricted( "gameui_preventescape\n" ); }
void CGameUI::AllowEngineHideGameUI() { engine->ClientCmd_Unrestricted( "gameui_allowescape\n" ); }
void CGameUI::HideGameUI() { engine->ClientCmd_Unrestricted( "gameui_hide\n" ); }
void CGameUI::ActivateGameUI() { engine->ClientCmd_Unrestricted( "gameui_activate\n" ); }

//-----------------------------------------------------------------------------
// The developer console
//-----------------------------------------------------------------------------
class CFStopConsoleDialog : public vgui::CConsoleDialog
{
	DECLARE_CLASS_SIMPLE( CFStopConsoleDialog, vgui::CConsoleDialog );
public:
	CFStopConsoleDialog() : BaseClass( NULL, "GameConsole", false )
    {
        // This DLL does not drive Frame's global fade controller. A fading
        // invisible console otherwise remains visible and pauses the server.
        SetFadeEffectDisableOverride( true );
        AddActionSignalTarget( this );
    }

	virtual void OnCommand( const char *command )
	{
		// closing the console over the menu map lets it run again
		if ( !Q_stricmp( command, "Close" ) && GameUI().IsInBackgroundLevel() )
			engine->ClientCmd_Unrestricted( "unpause" );
		BaseClass::OnCommand( command );
	}

	MESSAGE_FUNC_CHARPTR( OnCommandSubmitted, "CommandSubmitted", command )
	{
		engine->ClientCmd_Unrestricted( command, true );
	}
};

class CGameConsole : public IGameConsole
{
public:
	CGameConsole() : m_bInitialized( false ) {}

	virtual void Activate()
	{
		if ( !m_bInitialized || !m_pConsole.Get() )
			return;
		vgui::surface()->RestrictPaintToSinglePanel( NULL );
		// Opening the console in a level brings up the pause menu under it. Called directly, not through
		// "gameui_activate": toggleconsole runs from the command buffer, so a command would be queued and the
		// menu would open after the console and cover it.
		if ( GameUI().IsInLevel() && !enginevgui->IsGameUIVisible() )
			enginevgui->ActivateGameUI();
		m_pConsole->Activate();
	}

	virtual void Initialize()
	{
		m_pConsole = vgui::SETUP_PANEL( new CFStopConsoleDialog() );
		int swide, stall;
		vgui::surface()->GetScreenSize( swide, stall );
		int offset = vgui::scheme()->GetProportionalScaledValue( 16 );
		m_pConsole->SetBounds( swide / 2 - ( offset * 4 ), offset, ( swide / 2 ) + ( offset * 3 ), stall - ( offset * 8 ) );
		m_pConsole->InvalidateLayout( false, true );
		m_bInitialized = true;
	}

	virtual void Hide()
	{
		if ( m_bInitialized && m_pConsole.Get() )
        {
            m_pConsole->Hide();
            m_pConsole->SetVisible( false );
        }
	}

	virtual void Clear()
	{
		if ( m_bInitialized && m_pConsole.Get() )
			m_pConsole->Clear();
	}

	virtual bool IsConsoleVisible()
	{
		return m_bInitialized && m_pConsole.Get() && m_pConsole->IsVisible();
	}

	virtual void SetParent( uintp parent )
	{
		if ( m_bInitialized && m_pConsole.Get() )
			m_pConsole->SetParent( static_cast< vgui::VPANEL >( parent ) );
	}

	virtual void Shutdown()
	{
        if ( !m_bInitialized )
            return;
        m_bInitialized = false;
        // The engine can call IGameConsole::Shutdown after client VGUI has
        // disconnected. Resolving a DHANDLE then dereferences a null IVGui.
        if ( vgui::ivgui() && vgui::ipanel() && m_pConsole.Get() )
        {
            m_pConsole->SetVisible( false );
            delete m_pConsole.Get();
        }
        m_pConsole = NULL;
	}

private:
	// VGUI may delete the parent's children before the console's Shutdown call.
	vgui::DHANDLE<CFStopConsoleDialog> m_pConsole;
	bool m_bInitialized;
};

static CGameConsole g_GameConsole;
static void ShutdownFStopConsole() { g_GameConsole.Shutdown(); }
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CGameConsole, IGameConsole, GAMECONSOLE_INTERFACE_VERSION, g_GameConsole );
