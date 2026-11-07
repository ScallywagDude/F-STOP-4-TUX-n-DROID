// Root of the BaseModUI windows. Reduced from the Portal 2 style panel to what a single player game uses:
// window tracking, save listing, chapters from cfg/chapterN.cfg, and the game's own UI sounds.
#ifndef _BASEMODFACTORYBASEPANEL_H__
#define _BASEMODFACTORYBASEPANEL_H__

#define GAMEUI_BASEMODPANEL_VGUI
#define GAMEUI_BASEMODPANEL_SCHEME "basemodui_scheme"

#include "basemodui_compat.h"
#include "vgui_controls/Panel.h"
#include "vgui_controls/EditablePanel.h"
#include "vgui_controls/Frame.h"
#include "vgui_controls/Button.h"
#include "tier1/utllinkedlist.h"
#include "tier1/utlstring.h"
#include "utlmap.h"
#include "matchmaking/imatchframework.h"

#define BASEMODPANEL_SINGLETON BaseModUI::CBaseModPanel::GetSingleton()

struct SaveGameInfo_t
{
	SaveGameInfo_t()
	{
		m_nFileTime = 0;
		m_nElapsedSeconds = 0;
		m_nChapterNum = 1;
		m_bIsAutoSave = false;
		m_bIsCloudSave = false;
		m_bIsInCloud = false;
	}

	CUtlString		m_InternalIDname;
	CUtlString		m_Filename;
	CUtlString		m_FullFilename;
	CUtlString		m_ScreenshotFilename;
	CUtlString		m_MapName;
	CUtlString		m_Comment;
	time_t			m_nFileTime;
	unsigned int	m_nElapsedSeconds;
	int				m_nChapterNum;
	bool			m_bIsAutoSave;
	bool			m_bIsCloudSave;
	bool			m_bIsInCloud;
};

namespace BaseModUI
{
	enum WINDOW_TYPE
	{
		WT_NONE = 0,
		WT_ACHIEVEMENTS,
		WT_AUDIO,
		WT_AUDIOVIDEO,
		WT_CLOUD,
		WT_CONTROLLER,
		WT_CONTROLLER_STICKS,
		WT_CONTROLLER_BUTTONS,
		WT_DOWNLOADS,
		WT_GAMELOBBY,
		WT_GAMEOPTIONS,
		WT_GAMESETTINGS,
		WT_GENERICCONFIRMATION,
		WT_INGAMEDIFFICULTYSELECT,
		WT_INGAMEMAINMENU,
		WT_INGAMECHAPTERSELECT,
		WT_INGAMEKICKPLAYERLIST,
		WT_VOTEOPTIONS,
		WT_KEYBOARDMOUSE,
		WT_LOADINGPROGRESS,
		WT_MAINMENU,
		WT_ENDINGSPLITSCREEN,
		WT_MULTIPLAYER,
		WT_OPTIONS,
		WT_OPTIONSCLOUD,
		WT_SEARCHINGFORLIVEGAMES,
		WT_SIGNINDIALOG,
		WT_STEAMLINKDIALOG,
		WT_SINGLEPLAYER,
		WT_COOPMODESELECT,
		WT_GENERICWAITSCREEN,
		WT_ATTRACTSCREEN,
		WT_STARTCOOPGAME,
		WT_ALLGAMESEARCHRESULTS,
		WT_PVP_LOBBY,
		WT_FOUNDPUBLICGAMES,
		WT_TRANSITIONSCREEN,
		WT_PASSWORDENTRY,
		WT_VIDEO,
		WT_STEAMCLOUDCONFIRM,
		WT_STEAMGROUPSERVERS,
		WT_CUSTOMCAMPAIGNS,
		WT_ADDONS,
		WT_DOWNLOADCAMPAIGN,
		WT_LEADERBOARD,
		WT_ADDONASSOCIATION,
		WT_GETLEGACYDATA,
		WT_NEWGAME,
		WT_CHALLENGEMODE,
		WT_SAVEGAME,
		WT_LOADGAME,
		WT_MOVIEPLAYER,
		WT_COMMENTARY,
		WT_XBOXLIVE,
		WT_AUTOSAVENOTICE,
		WT_FADEOUTSTARTGAME,
		WT_ADVANCEDVIDEO,
		WT_KEYBINDINGS,
		WT_SOUNDTEST,
		WT_PORTALLEADERBOARD,
		WT_PORTALCOOPLEADERBOARD,
		WT_PORTALLEADERBOARDHUD,
		WT_COOPEXITCHOICE,
		WT_EXTRAS,
		WT_FADEOUTTOECONUI,
		WT_WINDOW_COUNT
	};

	enum WINDOW_PRIORITY
	{
		WPRI_NONE,
		WPRI_BKGNDSCREEN,
		WPRI_NORMAL,
		WPRI_WAITSCREEN,
		WPRI_MESSAGE,
		WPRI_LOADINGPLAQUE,
		WPRI_TOPMOST,
		WPRI_COUNT
	};

	enum UISound_t
	{
		UISOUND_BACK,
		UISOUND_ACCEPT,
		UISOUND_INVALID,
		UISOUND_COUNTDOWN,
		UISOUND_FOCUS,
		UISOUND_CLICK,
		UISOUND_DENY,
		UISOUND_TILE_CLICK1,
		UISOUND_TILE_CLICK2,
	};

	class CBaseModFrame;
	class CBaseModFooterPanel;
	class CBaseModTransitionPanel;

	class CBaseModPanel : public vgui::EditablePanel
	{
		DECLARE_CLASS_SIMPLE( CBaseModPanel, vgui::EditablePanel );

	public:
		CBaseModPanel();
		~CBaseModPanel();

		static CBaseModPanel &GetSingleton();
		static CBaseModPanel *GetSingletonPtr();

		void ReloadScheme();
		CBaseModFrame *OpenWindow( const WINDOW_TYPE &wt, CBaseModFrame *caller, bool hidePrevious = true, KeyValues *pParameters = NULL );
		CBaseModFrame *GetWindow( const WINDOW_TYPE &wt );
		void OnFrameClosed( WINDOW_PRIORITY pri, WINDOW_TYPE wt );
		WINDOW_TYPE GetActiveWindowType();
		WINDOW_PRIORITY GetActiveWindowPriority();
		void SetActiveWindow( CBaseModFrame *frame );
		bool OpenMessageDialog( const char *lpszTitle, const char *lpszMessage );

		enum CloseWindowsPolicy_t
		{
			CLOSE_POLICY_DEFAULT = 0,
			CLOSE_POLICY_EVEN_MSGS = 1,
			CLOSE_POLICY_EVEN_LOADING = 2,
			CLOSE_POLICY_KEEP_BKGND = 0x100,
		};
		void CloseAllWindows( int ePolicyFlags = CLOSE_POLICY_DEFAULT );

		void OnGameUIActivated();
		void OnGameUIHidden();
		void OpenFrontScreen( bool bIgnoreMatchSession = false );
		bool IsLevelLoading() { return false; }

		void SetOkButtonEnabled( bool enabled );
		void SetCancelButtonEnabled( bool enabled );

		const char *GetUISoundName( UISound_t uiSound );
		void PlayUISound( UISound_t uiSound );

		void StartExitingProcess( bool bWarmRestart );

		CBaseModFooterPanel *GetFooterPanel() { return m_FooterPanel; }
		void SetLastActiveUserId( int ) {}
		int GetLastActiveUserId() { return 0; }

		void OpenOptionsDialog( vgui::Panel *parent );
		void OpenKeyBindingsDialog( vgui::Panel *parent );
		void OnTileSetChanged() {}
		void SafeNavigateTo( vgui::Panel *pExpectedFrom, vgui::Panel *pDesiredTo, bool bAllowStealFocus );
		void ResetAttractDemoTimeout( bool = false ) {}
		bool IsControllerActive() const { return false; }
		void SetControllerActive( bool ) {}
		bool IsQuickplay() const { return false; }
		bool IsTransitionEffectEnabled() { return false; }
		CBaseModTransitionPanel *GetTransitionEffectPanel();
		bool IsOpaqueOverlayActive() { return false; }

		int GetChapterProgress() { return GetNumChapters(); }
		int GetNumChapters( bool bSinglePlayer = true );
		const char *ChapterToMapName( int nChapter );
		int MapNameToChapter( const char *pMapName, bool bSinglePlayer = true );
		int MapNameToSubChapter( const char * ) { return 0; }

		bool GetSaveGameInfos( CUtlVector< SaveGameInfo_t > &saveGameInfos, bool bFindAll = true );
		int GetMostRecentSaveGame( CUtlVector< SaveGameInfo_t > &saveGameInfos );

		int GetImageId( const char *pImageName );
		void DrawColoredText( int x, int y, Color color, const char *pAnsiText, vgui::HFont hFont = vgui::INVALID_FONT );

		vgui::HScheme GetScheme() const { return m_UIScheme; }

	protected:
		virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
		virtual void OnThink();
		virtual void OnCommand( const char *command );

	private:
		void LoadChapters();

		static CBaseModPanel *m_CFactoryBasePanel;
		vgui::DHANDLE< CBaseModFrame > m_Frames[WT_WINDOW_COUNT];
		WINDOW_TYPE m_ActiveWindow[WPRI_COUNT];
		bool m_bClosingAllWindows;
		bool m_bWasVisible;
		bool m_bHasInGameMenu;
		vgui::HScheme m_UIScheme;
		vgui::HFont m_hDefaultFont;
		CBaseModFooterPanel *m_FooterPanel;
		CUtlVector< CUtlString > m_ChapterMaps;
	};
}

#endif
