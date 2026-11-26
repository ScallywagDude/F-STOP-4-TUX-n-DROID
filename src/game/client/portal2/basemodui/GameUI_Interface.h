// F-STOP's game UI: the IGameUI and IGameConsole interfaces the engine takes from client.dll,
// with the BaseModUI panel as the front end. The engine shows and hides the UI; the panel follows.
#ifndef FSTOP_GAMEUI_INTERFACE_H
#define FSTOP_GAMEUI_INTERFACE_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "GameUI/IGameUI.h"

class CGameUI : public IGameUI
{
public:
	CGameUI();

	// IGameUI
	virtual void Initialize( CreateInterfaceFn appFactory );
	virtual void PostInit();
	virtual void Connect( CreateInterfaceFn gameFactory );
	virtual void Start();
	virtual void Shutdown();
	virtual void RunFrame();
	virtual void OnGameUIActivated();
	virtual void OnGameUIHidden();
	virtual void OLD_OnConnectToServer( const char *game, int IP, int port ) {}
	virtual void OnDisconnectFromServer_OLD( uint8 eSteamLoginFailure, const char *username ) {}
	virtual void OnLevelLoadingStarted( const char *levelName, bool bShowProgressDialog );
	virtual void OnLevelLoadingFinished( bool bError, const char *failureReason, const char *extendedReason );
	virtual void StartLoadingScreenForCommand( const char *command ) {}
	virtual void StartLoadingScreenForKeyValues( KeyValues *keyValues ) {}
	virtual bool UpdateProgressBar( float progress, const char *statusText, bool showDialog = true );
	virtual bool SetShowProgressText( bool show ) { bool bOld = m_bShowProgressText; m_bShowProgressText = show; return bOld; }
	virtual bool UpdateSecondaryProgressBar( float progress, const wchar_t *desc ) { return false; }
	virtual void SetProgressLevelName( const char *levelName ) {}
	virtual void ShowMessageDialog( const uint nType, vgui::Panel *pOwner ) {}
	virtual void ShowMessageDialog( const char *messageID, const char *titleID );
	virtual void CreateCommandMsgBox( const char *pszTitle, const char *pszMessage, bool showOk = true, bool showCancel = false, const char *okCommand = NULL, const char *cancelCommand = NULL, const char *closedCommand = NULL, const char *pszLegend = NULL, const char *szPanelID = nullptr );
	virtual void CreateCommandMsgBoxInSlot( ECommandMsgBoxSlot slot, const char *pszTitle, const char *pszMessage, bool showOk = true, bool showCancel = false, const char *okCommand = NULL, const char *cancelCommand = NULL, const char *closedCommand = NULL, const char *pszLegend = NULL );
	virtual void SetLoadingBackgroundDialog( vgui::VPANEL panel ) {}
	virtual void OnConnectToServer2( const char *game, int IP, int connectionPort, int queryPort ) {}
	virtual void SetProgressOnStart() {}
	virtual void OnDisconnectFromServer( uint8 eSteamLoginFailure ) {}
	virtual void NeedConnectionProblemWaitScreen() {}
	virtual void ShowPasswordUI( char const *pchCurrentPW ) {}
	virtual bool LoadingProgressWantsIsolatedRender( bool bContextValid ) { return false; }
	virtual bool IsPlayingFullScreenVideo() { return false; }
	virtual bool IsTransitionEffectEnabled() { return false; }
	virtual bool IsInLevel();
	virtual void RestoreTopLevelMenu();
	virtual void SetBackgroundMusicDesired( bool bPlayMusic, bool bAllowFinishCurrentTrack = false );
	virtual void ShowCustomProgress() {}
	virtual bool IsInGameUiFrontEndInteractiveState() { return !IsInLevel(); }

	// called by the client
	void CreateGameSpecificUI();
	void DestroyGameSpecificUI();
	bool IsPanoramaEnabled() const { return false; }
	bool IsLoadingScreenVisible() const { return m_bLoading; }
	bool IsInBackgroundLevel();
	bool IsInMultiplayer() { return IsInLevel() && engine->GetMaxClients() > 1; }

	// called by the BaseModUI windows
	void PreventEngineHideGameUI();
	void AllowEngineHideGameUI();
	void HideGameUI();
	void ActivateGameUI();

private:
	void EnsurePanel();

	bool m_bShowProgressText;
	bool m_bLoading;
	bool m_bPanelCreated;
	int m_nMusicGuid;
	double m_flNextMusicAttempt;
};

CGameUI &GameUI();

#endif // FSTOP_GAMEUI_INTERFACE_H
