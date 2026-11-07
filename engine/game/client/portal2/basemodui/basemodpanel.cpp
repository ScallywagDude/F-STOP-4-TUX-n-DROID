#include "basemodui_compat.h"
#include "cbase.h"
#include "basemodpanel.h"
#include "basemodframe.h"
#include "uigamedata.h"
#include "vfooterpanel.h"
#include "transitionpanel.h"
#include "vflyoutmenu.h"
#include "vmainmenu.h"
#include "vingamemainmenu.h"
#include "vgenericconfirmation.h"
#include "vgameoptions.h"
#include "vnewgamedialog.h"
#include "vsaveloadgamedialog.h"
#include "vvideo.h"
#include "vadvancedvideo.h"
#include "vaudio.h"
#include "vkeyboardmouse.h"
#include "vkeybindings.h"
#include "ienginevgui.h"
#include "vgui/ISurface.h"
#include "vgui/IVGui.h"
#include "vgui/IScheme.h"
#include "vgui/ILocalize.h"
#include "filesystem.h"
#include "tier1/fmtstr.h"
#include "GameUI/IGameUI.h"
#include "tier1/interface.h"
#include "savegame_version.h"
#include "tier0/memdbgon.h"

using namespace vgui;
using namespace BaseModUI;

static int SaveReadNameAndComment( FileHandle_t f, OUT_Z_CAP(nameSize) char *name, int nameSize, OUT_Z_CAP(commentSize) char *comment, int commentSize )
{
	int i, tag, size, tokenSize, tokenCount;
	char *pSaveData, *pFieldName, **pTokenList;

	name[0] = '\0';
	comment[0] = '\0';

	g_pFullFileSystem->Read( &tag, sizeof(int), f );
	if ( tag != MAKEID('J','S','A','V') )
	{
		return 0;
	}
		
	g_pFullFileSystem->Read( &tag, sizeof(int), f );
	if ( tag != SAVEGAME_VERSION )				// Enforce version for now
	{
		return 0;
	}

	g_pFullFileSystem->Read( &size, sizeof(int), f );
	
	g_pFullFileSystem->Read( &tokenCount, sizeof(int), f );	// These two ints are the token list
	g_pFullFileSystem->Read( &tokenSize, sizeof(int), f );
	size += tokenSize;

	// Sanity Check.
	if ( tokenCount < 0 || tokenCount > 1024*1024*32  )
	{
		return 0;
	}

	if ( tokenSize < 0 || tokenSize > 1024*1024*32  )
	{
		return 0;
	}

	pSaveData = (char *)new char[size];
	g_pFullFileSystem->Read(pSaveData, size, f);

	int nNumberOfFields;

	char *pData;
	short nFieldSize;
	
	pData = pSaveData;

	// Allocate a table for the strings, and parse the table
	if ( tokenSize > 0 )
	{
		pTokenList = new char *[tokenCount];

		// Make sure the token strings pointed to by the pToken hashtable.
		for( i=0; i<tokenCount; i++ )
		{
			pTokenList[i] = *pData ? pData : NULL;	// Point to each string in the pToken table
			while( *pData++ );				// Find next token (after next null)
		}
	}
	else
		pTokenList = NULL;

	// short, short (size, index of field name)
	memcpy( &nFieldSize, pData, sizeof(short) );

	pData += sizeof(short);
	short index;
	memcpy( &index, pData, sizeof(short) );
	pFieldName = pTokenList[index];

	if (stricmp(pFieldName, "GameHeader"))
	{
		delete[] pSaveData;
		return 0;
	};

	// int (fieldcount)
	pData += sizeof(short);
	memcpy( &nNumberOfFields, pData, sizeof(int) );
	pData += nFieldSize;

	// Each field is a short (size), short (index of name), binary string of "size" bytes (data)
	for (i = 0; i < nNumberOfFields; i++)
	{
		// Data order is:
		// Size
		// szName
		// Actual Data

		memcpy( &nFieldSize, pData, sizeof(short) );
		pData += sizeof(short);

		short index;
		memcpy( &index, pData, sizeof(short));
		pFieldName = pTokenList[index];
		pData += sizeof(short);

		if (!stricmp(pFieldName, "comment"))
		{
			int copySize = MAX(commentSize, nFieldSize);
			Q_strncpy(comment, pData, copySize);
		}
		else if (!stricmp(pFieldName, "mapName"))
		{
			int copySize = MAX(nameSize, nFieldSize);
			Q_strncpy(name, pData, copySize);
		};

		// Move to Start of next field.
		pData += nFieldSize;
	};

	// Delete the string table we allocated
	delete[] pTokenList;
	delete[] pSaveData;
	
	if (strlen(name) > 0 && strlen(comment) > 0)
		return 1;
	
	return 0;
}

CBaseModPanel *CBaseModPanel::m_CFactoryBasePanel = NULL;

static CDllDemandLoader g_GameUIModule( "GameUI" );

// Creates the root panel and tells the stock game UI to give up its own menu list to it.
void BaseModUI_Create()
{
	if ( CBaseModPanel::GetSingletonPtr() )
		return;
	CBaseModPanel *pPanel = new CBaseModPanel();
	CreateInterfaceFn factory = g_GameUIModule.GetFactory();
	IGameUI *pGameUI = factory ? (IGameUI *)factory( GAMEUI_INTERFACE_VERSION, NULL ) : NULL;
	if ( pGameUI )
		pGameUI->SetMainMenuOverride( pPanel->GetVPanel() );
	else
		Warning( "BaseModUI: no game UI interface\n" );
}

CBaseModPanel::CBaseModPanel() : BaseClass( NULL, "BaseModPanel" )
{
	m_CFactoryBasePanel = this;
	m_bClosingAllWindows = false;
	m_bWasVisible = false;
	m_bHasInGameMenu = false;
	m_hDefaultFont = INVALID_FONT;
	for ( int k = 0; k < WPRI_COUNT; ++k )
		m_ActiveWindow[k] = WT_NONE;

	SetParent( enginevgui->GetPanel( PANEL_GAMEUIDLL ) );
	m_UIScheme = scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_GAMEUIDLL ), "resource/basemodui_scheme.res", GAMEUI_BASEMODPANEL_SCHEME );
	SetScheme( m_UIScheme );
	SetProportional( true );

	int wide, tall;
	surface()->GetScreenSize( wide, tall );
	SetBounds( 0, 0, wide, tall );
	SetPaintBackgroundEnabled( false );
	SetKeyBoardInputEnabled( true );
	SetMouseInputEnabled( true );
	SetVisible( true );

	g_pVGuiLocalize->AddFile( "resource/basemodui_%language%.txt", "GAME", true );

	m_FooterPanel = new CBaseModFooterPanel( this, "FooterPanel" );
	LoadChapters();
	ivgui()->AddTickSignal( GetVPanel() );
}

CBaseModPanel::~CBaseModPanel()
{
	if ( m_CFactoryBasePanel == this )
		m_CFactoryBasePanel = NULL;
}

CBaseModPanel &CBaseModPanel::GetSingleton()
{
	Assert( m_CFactoryBasePanel );
	return *m_CFactoryBasePanel;
}

CBaseModPanel *CBaseModPanel::GetSingletonPtr()
{
	return m_CFactoryBasePanel;
}

void CBaseModPanel::ReloadScheme()
{
}

void CBaseModPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	m_hDefaultFont = pScheme->GetFont( "DefaultSystemUI", true );
	SetBgColor( Color( 0, 0, 0, 0 ) );
	int wide, tall;
	surface()->GetScreenSize( wide, tall );
	SetBounds( 0, 0, wide, tall );
}

// The engine shows and hides the game UI. A change in that state picks the right front window.
void CBaseModPanel::OnThink()
{
	BaseClass::OnThink();

	CBaseModFrame::RunFrameOnListeners();

	bool bVisible = enginevgui->IsGameUIVisible();
	if ( bVisible && !m_bWasVisible )
		OnGameUIActivated();
	else if ( !bVisible && m_bWasVisible )
		OnGameUIHidden();
	m_bWasVisible = bVisible;

	// A level that starts or ends with the UI open still needs the matching front window
	if ( bVisible )
	{
		bool bInLevel = engine->IsInGame() && !engine->IsLevelMainMenuBackground();
		if ( bInLevel != m_bHasInGameMenu && GetActiveWindowPriority() <= WPRI_NORMAL )
			OpenFrontScreen();
	}
}

void CBaseModPanel::OnGameUIActivated()
{
	SetVisible( true );
	MoveToFront();
	OpenFrontScreen();
}

void CBaseModPanel::OnGameUIHidden()
{
	FlyoutMenu::CloseActiveMenu();
	CloseAllWindows( CLOSE_POLICY_EVEN_MSGS );
	SetVisible( false );
}

void CBaseModPanel::OpenFrontScreen( bool )
{
	bool bInLevel = engine->IsInGame() && !engine->IsLevelMainMenuBackground();
	WINDOW_TYPE front = bInLevel ? WT_INGAMEMAINMENU : WT_MAINMENU;
	m_bHasInGameMenu = bInLevel;

	CBaseModFrame *pFront = GetWindow( front );
	if ( pFront && pFront->IsVisible() )
		return;

	CloseAllWindows();
	OpenWindow( front, NULL, true );
}

CBaseModFrame *CBaseModPanel::OpenWindow( const WINDOW_TYPE &wt, CBaseModFrame *caller, bool hidePrevious, KeyValues *pParameters )
{
	// Starting a game has no window: the engine's own loading screen takes over.
	if ( wt == WT_FADEOUTSTARTGAME )
	{
		if ( pParameters )
		{
			const char *pLoad = pParameters->GetString( "loadfilename", "" );
			const char *pMap = pParameters->GetString( "map", "" );
			if ( pLoad[0] )
				engine->ClientCmd_Unrestricted( CFmtStr( "load %s\n", pLoad ) );
			else if ( pMap[0] )
				engine->ClientCmd_Unrestricted( CFmtStr( "map %s\n", pMap ) );
		}
		return NULL;
	}

	CBaseModFrame *newNav = m_Frames[wt].Get();
	bool setActiveWindow = true;

	WINDOW_PRIORITY nWindowPriority = WPRI_NORMAL;
	if ( wt == WT_GENERICCONFIRMATION )
		nWindowPriority = WPRI_MESSAGE;

	if ( !newNav )
	{
		switch ( wt )
		{
		case WT_MAINMENU:			m_Frames[wt] = new MainMenu( this, "MainMenu" ); break;
		case WT_INGAMEMAINMENU:		m_Frames[wt] = new InGameMainMenu( this, "InGameMainMenu" ); break;
		case WT_GENERICCONFIRMATION:	m_Frames[wt] = new GenericConfirmation( this, "GenericConfirmation" ); break;
		case WT_OPTIONS:			m_Frames[wt] = new OptionsMenu( this, "OptionsMenu" ); break;
		case WT_GAMEOPTIONS:		m_Frames[wt] = new GameOptions( this, "GameOptions" ); break;
		case WT_NEWGAME:			m_Frames[wt] = new CNewGameDialog( this, "NewGameDialog", false ); break;
		case WT_SAVEGAME:			m_Frames[wt] = new SaveLoadGameDialog( this, "SaveLoadGameDialog", true ); break;
		case WT_LOADGAME:			m_Frames[wt] = new SaveLoadGameDialog( this, "SaveLoadGameDialog", false ); break;
		case WT_VIDEO:				m_Frames[wt] = new Video( this, "Video" ); break;
		case WT_ADVANCEDVIDEO:		m_Frames[wt] = new CAdvancedVideo( this, "AdvancedVideo" ); break;
		case WT_AUDIO:				m_Frames[wt] = new Audio( this, "Audio" ); break;
		case WT_KEYBOARDMOUSE:		m_Frames[wt] = new KeyboardMouse( this, "KeyboardMouse" ); break;
		case WT_KEYBINDINGS:		m_Frames[wt] = new CKeyBindings( this, "KeyBindings" ); break;
		default:
			Warning( "BaseModUI: window %d is not part of this game\n", (int)wt );
			return NULL;
		}
		newNav = m_Frames[wt].Get();
		if ( !newNav )
			return NULL;
		newNav->SetWindowPriority( nWindowPriority );
		newNav->SetWindowType( wt );
	}
	else
	{
		nWindowPriority = newNav->GetWindowPriority();
	}

	newNav->SetDataSettings( pParameters );

	if ( setActiveWindow )
	{
		m_ActiveWindow[nWindowPriority] = wt;
		newNav->AddActionSignalTarget( this );
		newNav->SetCanBeActiveWindowType( true );
	}

	newNav->SetNavBack( caller );

	if ( hidePrevious && caller )
		caller->SetVisible( false );
	else if ( caller )
		caller->FindAndSetActiveControl();

	CBaseModFooterPanel *pFooter = GetFooterPanel();
	if ( pFooter )
		pFooter->SetFooterType( nWindowPriority == WPRI_MESSAGE ? FOOTER_GENERICCONFIRMATION : FOOTER_MENUS );

	newNav->InvalidateLayout( false, false );
	newNav->OnOpen();

	return newNav;
}

CBaseModFrame *CBaseModPanel::GetWindow( const WINDOW_TYPE &wt )
{
	return m_Frames[wt].Get();
}

WINDOW_TYPE CBaseModPanel::GetActiveWindowType()
{
	for ( int k = WPRI_COUNT; k-- > 0; )
	{
		if ( m_ActiveWindow[k] != WT_NONE )
		{
			CBaseModFrame *pFrame = m_Frames[m_ActiveWindow[k]].Get();
			if ( !pFrame || !pFrame->IsVisible() )
				continue;
			return m_ActiveWindow[k];
		}
	}
	return WT_NONE;
}

WINDOW_PRIORITY CBaseModPanel::GetActiveWindowPriority()
{
	for ( int k = WPRI_COUNT; k-- > 0; )
	{
		if ( m_ActiveWindow[k] != WT_NONE )
		{
			CBaseModFrame *pFrame = m_Frames[m_ActiveWindow[k]].Get();
			if ( !pFrame || !pFrame->IsVisible() )
				continue;
			return (WINDOW_PRIORITY)k;
		}
	}
	return WPRI_NONE;
}

void CBaseModPanel::SetActiveWindow( CBaseModFrame *frame )
{
	if ( !frame )
		return;
	m_ActiveWindow[frame->GetWindowPriority()] = frame->GetWindowType();
	if ( GetActiveWindowPriority() <= frame->GetWindowPriority() )
		frame->OnOpen();
}

void CBaseModPanel::OnFrameClosed( WINDOW_PRIORITY pri, WINDOW_TYPE wt )
{
	m_Frames[wt] = NULL;

	if ( m_bClosingAllWindows || pri <= WPRI_NORMAL )
		return;

	for ( int k = 0; k < WPRI_COUNT; ++k )
	{
		if ( m_ActiveWindow[k] == wt )
			m_ActiveWindow[k] = WT_NONE;
	}

	// A message going away brings back whatever sits beneath it
	for ( int k = WPRI_COUNT; k-- > 0; )
	{
		if ( m_ActiveWindow[k] == WT_NONE )
			continue;
		CBaseModFrame *pFrame = m_Frames[m_ActiveWindow[k]].Get();
		if ( !pFrame )
			continue;
		if ( GetFooterPanel() )
			GetFooterPanel()->SetFooterType( FOOTER_MENUS );
		pFrame->InvalidateLayout( false, false );
		pFrame->OnOpen();
		pFrame->SetVisible( true );
		pFrame->Activate();
		return;
	}
}

void CBaseModPanel::CloseAllWindows( int ePolicyFlags )
{
	CAutoPushPop< bool > auto_m_bClosingAllWindows( m_bClosingAllWindows, true );
	FlyoutMenu::CloseActiveMenu();

	for ( int i = 0; i < WT_WINDOW_COUNT; ++i )
	{
		CBaseModFrame *pFrame = m_Frames[i].Get();
		if ( !pFrame )
			continue;
		if ( pFrame->GetWindowPriority() == WPRI_MESSAGE && !( ePolicyFlags & CLOSE_POLICY_EVEN_MSGS ) )
			continue;
		pFrame->Close();
		m_Frames[i] = NULL;
	}
	m_ActiveWindow[WPRI_NORMAL] = WT_NONE;
	if ( ePolicyFlags & CLOSE_POLICY_EVEN_MSGS )
		m_ActiveWindow[WPRI_MESSAGE] = WT_NONE;
}

void CBaseModPanel::StartExitingProcess( bool )
{
	engine->ClientCmd_Unrestricted( "quit\n" );
}

void CBaseModPanel::OnCommand( const char *command )
{
	BaseClass::OnCommand( command );
}

bool CBaseModPanel::OpenMessageDialog( const char *lpszTitle, const char *lpszMessage )
{
	GenericConfirmation *pConfirm = static_cast< GenericConfirmation * >( OpenWindow( WT_GENERICCONFIRMATION, NULL, false ) );
	if ( !pConfirm )
		return false;
	GenericConfirmation::Data_t data;
	data.pWindowTitle = lpszTitle;
	data.pMessageText = lpszMessage;
	data.bOkButtonEnabled = true;
	pConfirm->SetUsageData( data );
	return true;
}

void CBaseModPanel::SetOkButtonEnabled( bool enabled )
{
	if ( m_FooterPanel )
		m_FooterPanel->SetButtons( enabled ? FB_ABUTTON : FB_NONE );
}

void CBaseModPanel::SetCancelButtonEnabled( bool )
{
}

// The menu keeps the sounds the game already ships
const char *CBaseModPanel::GetUISoundName( UISound_t uiSound )
{
	switch ( uiSound )
	{
	case UISOUND_FOCUS:
		return "UI/buttonrollover.wav";
	case UISOUND_BACK:
	case UISOUND_ACCEPT:
	case UISOUND_CLICK:
	case UISOUND_TILE_CLICK1:
	case UISOUND_TILE_CLICK2:
		return "UI/buttonclick.wav";
	case UISOUND_INVALID:
	case UISOUND_DENY:
	case UISOUND_COUNTDOWN:
		return "UI/buttonclickrelease.wav";
	}
	return NULL;
}

void CBaseModPanel::PlayUISound( UISound_t uiSound )
{
	const char *pSound = GetUISoundName( uiSound );
	if ( pSound )
		surface()->PlaySound( pSound );
}

void CBaseModPanel::OpenOptionsDialog( Panel *parent )
{
	OpenWindow( WT_GAMEOPTIONS, static_cast< CBaseModFrame * >( parent ) );
}

void CBaseModPanel::OpenKeyBindingsDialog( Panel *parent )
{
	OpenWindow( WT_KEYBINDINGS, static_cast< CBaseModFrame * >( parent ) );
}

void CBaseModPanel::SafeNavigateTo( Panel *pExpectedFrom, Panel *pDesiredTo, bool bAllowStealFocus )
{
	if ( pDesiredTo )
		pDesiredTo->RequestFocus();
}

CBaseModTransitionPanel *CBaseModPanel::GetTransitionEffectPanel()
{
	static CBaseModTransitionPanel s_Transition;
	return &s_Transition;
}

void CBaseModPanel::LoadChapters()
{
	m_ChapterMaps.Purge();
	for ( int i = 1; ; ++i )
	{
		CUtlBuffer buf( 0, 0, CUtlBuffer::TEXT_BUFFER );
		if ( !g_pFullFileSystem->ReadFile( CFmtStr( "cfg/chapter%d.cfg", i ), "GAME", buf ) )
			break;
		char szLine[256];
		V_strncpy( szLine, (const char *)buf.Base(), MIN( (int)sizeof( szLine ), buf.TellPut() + 1 ) );
		const char *pMap = V_stristr( szLine, "map " );
		char szMap[MAX_MAP_NAME] = { 0 };
		if ( pMap )
		{
			pMap += 4;
			int n = 0;
			while ( *pMap && *pMap != ' ' && *pMap != '\r' && *pMap != '\n' && *pMap != '"' && n < MAX_MAP_NAME - 1 )
				szMap[n++] = *pMap++;
			szMap[n] = 0;
		}
		m_ChapterMaps.AddToTail( CUtlString( szMap ) );
	}
}

int CBaseModPanel::GetNumChapters( bool )
{
	return m_ChapterMaps.Count();
}

const char *CBaseModPanel::ChapterToMapName( int nChapter )
{
	if ( nChapter < 1 || nChapter > m_ChapterMaps.Count() )
		return "";
	return m_ChapterMaps[nChapter - 1].Get();
}

int CBaseModPanel::MapNameToChapter( const char *pMapName, bool )
{
	if ( pMapName )
	{
		for ( int i = 0; i < m_ChapterMaps.Count(); ++i )
		{
			if ( !V_stricmp( m_ChapterMaps[i].Get(), pMapName ) )
				return i + 1;
		}
	}
	return 1;
}

bool CBaseModPanel::GetSaveGameInfos( CUtlVector< SaveGameInfo_t > &saveGameInfos, bool bFindAll )
{
	saveGameInfos.Purge();

	FileFindHandle_t handle;
	const char *pName = g_pFullFileSystem->FindFirstEx( "save/*.sav", "MOD", &handle );
	while ( pName )
	{
		char szFull[MAX_PATH];
		V_snprintf( szFull, sizeof( szFull ), "save/%s", pName );

		char szMap[64] = { 0 }, szComment[128] = { 0 };
		FileHandle_t fh = g_pFullFileSystem->Open( szFull, "rb", "MOD" );
		if ( fh == FILESYSTEM_INVALID_HANDLE )
		{
			pName = g_pFullFileSystem->FindNext( handle );
			continue;
		}
		int readok = SaveReadNameAndComment( fh, szMap, sizeof( szMap ), szComment, sizeof( szComment ) );
		g_pFullFileSystem->Close( fh );
		if ( !readok )
		{
			pName = g_pFullFileSystem->FindNext( handle );
			continue;
		}

		// the last six characters of the comment are the elapsed time, mmm:ss
		unsigned int nElapsed = 0;
		int nLen = V_strlen( szComment );
		if ( nLen >= 6 )
		{
			nElapsed = atoi( szComment + nLen - 6 ) * 60 + atoi( szComment + nLen - 2 );
			szComment[nLen - 6] = 0;
			for ( int n = nLen - 7; n >= 0 && szComment[n] == ' '; --n )
				szComment[n] = 0;
		}

		SaveGameInfo_t info;
		info.m_InternalIDname = pName;
		info.m_Filename = pName;
		info.m_FullFilename = szFull;
		char szShot[MAX_PATH];
		V_strncpy( szShot, szFull, sizeof( szShot ) );
		V_SetExtension( szShot, ".tga", sizeof( szShot ) );
		info.m_ScreenshotFilename = szShot;
		info.m_MapName = szMap;
		info.m_Comment = szComment[0] ? szComment : pName;
		info.m_nFileTime = g_pFullFileSystem->GetFileTime( szFull, "MOD" );
		info.m_bIsAutoSave = V_stristr( pName, "autosave" ) != NULL;
		info.m_nChapterNum = MapNameToChapter( szMap );
		info.m_nElapsedSeconds = nElapsed;
		saveGameInfos.AddToTail( info );

		if ( !bFindAll )
			break;
		pName = g_pFullFileSystem->FindNext( handle );
	}
	g_pFullFileSystem->FindClose( handle );
	return saveGameInfos.Count() != 0;
}

int CBaseModPanel::GetMostRecentSaveGame( CUtlVector< SaveGameInfo_t > &saveGameInfos )
{
	int best = -1;
	time_t bestTime = 0;
	for ( int i = 0; i < saveGameInfos.Count(); ++i )
	{
		if ( saveGameInfos[i].m_nFileTime > bestTime )
		{
			bestTime = saveGameInfos[i].m_nFileTime;
			best = i;
		}
	}
	return best;
}

int CBaseModPanel::GetImageId( const char *pImageName )
{
	int nImageId = surface()->DrawGetTextureId( pImageName );
	if ( nImageId == -1 )
	{
		nImageId = surface()->CreateNewTextureID();
		surface()->DrawSetTextureFile( nImageId, pImageName, true, false );
	}
	return nImageId;
}

void CBaseModPanel::DrawColoredText( int x, int y, Color color, const char *pAnsiText, HFont hFont )
{
	wchar_t szText[256];
	g_pVGuiLocalize->ConvertANSIToUnicode( pAnsiText, szText, sizeof( szText ) );
	surface()->DrawSetTextFont( hFont != INVALID_FONT ? hFont : m_hDefaultFont );
	surface()->DrawSetTextColor( color );
	surface()->DrawSetTextPos( x, y );
	surface()->DrawPrintText( szText, V_wcslen( szText ) );
}
