#include "basemodui_compat.h"
//========= Copyright (c) 1996-2008, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=======================================================================================//

#include "vadvancedvideo.h"
#include "vfooterpanel.h"
#include "vdropdownmenu.h"
#include "vslidercontrol.h"
#include "vhybridbutton.h"
#include "EngineInterface.h"
#include "IGameUIFuncs.h"
#include "gameui_util.h"
#include "vgui/ISurface.h"
#include "modes.h"
#include "tier1/fmtstr.h"
#include "videocfg/videocfg.h"
#include "vgenericconfirmation.h"

#include "materialsystem/materialsystem_config.h"

#ifdef _X360
#include "xbox/xbox_launch.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;
using namespace BaseModUI;

// The three detail rows are presets over real engine settings. High matches the shipped defaults.
static void SetEngineVar( const char *pName, const char *pValue )
{
	ConVarRef var( pName, true );
	if ( var.IsValid() )
		var.SetValue( pValue );
}

static void ApplyShaderDetail( int nLevel )
{
	SetEngineVar( "mat_reducefillrate", nLevel >= 2 ? "0" : "1" );
	SetEngineVar( "mat_specular", nLevel >= 1 ? "1" : "0" );
	SetEngineVar( "mat_phong", nLevel >= 1 ? "1" : "0" );
	SetEngineVar( "r_shadowrendertotexture", nLevel >= 2 ? "1" : "0" );
	SetEngineVar( "r_shadowmaxrendered", nLevel >= 3 ? "64" : "32" );
}

static void ApplyEffectDetail( int nLevel )
{
	SetEngineVar( "r_drawdetailprops", nLevel >= 1 ? "1" : "0" );
	SetEngineVar( "cl_detaildist", nLevel >= 2 ? "1200" : "600" );
	SetEngineVar( "r_decals", nLevel >= 2 ? "2048" : ( nLevel == 1 ? "128" : "32" ) );
}

static void ApplyModelTextureDetail( int nLevel )
{
	const char *pValue = nLevel >= 2 ? "0" : ( nLevel == 1 ? "1" : "2" );
	SetEngineVar( "mat_picmip", nLevel >= 3 ? "-1" : pValue );
	SetEngineVar( "r_rootlod", pValue );
}

static void GpuLevelChanged( IConVar *pVar, const char *, float )
{
	ApplyShaderDetail( ConVarRef( pVar ).GetInt() );
}

static void CpuLevelChanged( IConVar *pVar, const char *, float )
{
	ApplyEffectDetail( ConVarRef( pVar ).GetInt() );
}

static void GpuMemLevelChanged( IConVar *pVar, const char *, float )
{
	ApplyModelTextureDetail( ConVarRef( pVar ).GetInt() );
}

static ConVar gpu_level( "gpu_level", "2", FCVAR_ARCHIVE, "Shader detail: 0 low, 1 medium, 2 high, 3 very high", GpuLevelChanged );
static ConVar cpu_level( "cpu_level", "2", FCVAR_ARCHIVE, "Effect detail: 0 low, 1 medium, 2 high", CpuLevelChanged );
static ConVar gpu_mem_level( "gpu_mem_level", "2", FCVAR_ARCHIVE, "Model and texture detail: 0 low, 1 medium, 2 high, 3 very high", GpuMemLevelChanged );

// Runs at the end of autoexec.cfg, which sets the high values on every launch.
CON_COMMAND_F( fstop_apply_detail, "Applies the saved detail levels", FCVAR_HIDDEN )
{
	if ( gpu_level.GetInt() != 2 )
		ApplyShaderDetail( gpu_level.GetInt() );
	if ( cpu_level.GetInt() < 2 )
		ApplyEffectDetail( cpu_level.GetInt() );
	if ( gpu_mem_level.GetInt() != 2 )
		ApplyModelTextureDetail( gpu_mem_level.GetInt() );
}

#define VIDEO_ANTIALIAS_COMMAND_PREFIX "_antialias"

CAdvancedVideo::CAdvancedVideo(Panel *parent, const char *panelName):
BaseClass(parent, panelName)
{
	SetDeleteSelfOnClose( true );
	SetProportional( true );

	SetDialogTitle( "#GameUI_VideoAdvanced_Title" );

	m_drpModelDetail = NULL;
#ifndef POSIX	
	m_drpPagedPoolMem = NULL;
#endif
	m_drpQueuedMode = NULL;
	m_drpAntialias = NULL;
	m_drpFiltering = NULL;
	m_drpVSync = NULL;
	m_drpShaderDetail = NULL;
	m_drpCPUDetail = NULL;

	m_bDirtyValues = false;
	m_bEnableApply = false;

	m_lblDescriptionTitle = NULL;
	m_lblDescription = NULL;

	m_nNumAAModes = 0;

	m_iModelTextureDetail = 0;
	m_iPagedPoolMem = 0;
	m_nAASamples = 0;
	m_nAAQuality = 0;
	m_iFiltering = 1;
	m_bVSync = false;
	m_bTripleBuffered = false;
	m_iGPUDetail = 0;
	m_iCPUDetail = 0;
	m_iQueuedMode = -1;

	SetFooterEnabled( true );
	UpdateFooter();
}

CAdvancedVideo::~CAdvancedVideo()
{
}

void CAdvancedVideo::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	m_drpModelDetail = dynamic_cast< BaseModHybridButton* >( FindChildByName( "DrpModelDetail" ) );
#ifndef POSIX
	m_drpPagedPoolMem = dynamic_cast< BaseModHybridButton* >( FindChildByName( "DrpPagedPoolMem" ) );
#endif
	m_drpAntialias = dynamic_cast< BaseModHybridButton* >( FindChildByName( "DrpAntialias" ) );
	m_drpFiltering = dynamic_cast< BaseModHybridButton* >( FindChildByName( "DrpFiltering" ) );
	m_drpVSync = dynamic_cast< BaseModHybridButton* >( FindChildByName( "DrpVSync" ) );
	m_drpQueuedMode = dynamic_cast< BaseModHybridButton* >( FindChildByName( "DrpQueuedMode" ) );
	m_drpShaderDetail = dynamic_cast< BaseModHybridButton* >( FindChildByName( "DrpShaderDetail" ) );
	m_drpCPUDetail = dynamic_cast< BaseModHybridButton* >( FindChildByName( "DrpCPUDetail" ) );

	m_lblDescriptionTitle = dynamic_cast< Label* >( FindChildByName( "LblDescriptionTitle" ) );
	m_lblDescription = dynamic_cast< Label* >( FindChildByName( "LblDescription" ) );

	SetupState( false );

	if ( m_drpAntialias )
	{
		if ( m_ActiveControl )
			m_ActiveControl->NavigateFrom();
		m_drpAntialias->NavigateTo();
		m_ActiveControl = m_drpAntialias;
	}

	UpdateFooter();
}

void CAdvancedVideo::OnHybridButtonNavigatedTo( VPANEL defaultButton )
{
	Panel *panel = ipanel()->GetPanel( defaultButton, GetModuleName() );
	const char* buttonName = panel->GetName();

	if (!V_strcmp(buttonName, "DrpAntialias"))
	{
		m_lblDescriptionTitle->SetText("#L4D360UI_VideoOptions_Antialiasing");
		m_lblDescription->SetText("#PORTAL2_VideoOptions_Antialiasing_Info");
	}
	else if (!V_strcmp(buttonName, "DrpFiltering"))
	{
		m_lblDescriptionTitle->SetText( "#GameUI_Filtering_Mode" );
		m_lblDescription->SetText( "#PORTAL2_VideoOptions_Filtering_Info" );
	}
	else if (!V_strcmp(buttonName, "DrpVSync"))
	{
		m_lblDescriptionTitle->SetText( "#GameUI_Wait_For_VSync" );
		m_lblDescription->SetText( "#PORTAL2_VideoOptions_WaitForVSync_Info" );
	}
	else if (!V_strcmp(buttonName, "DrpQueuedMode"))
	{
		m_lblDescriptionTitle->SetText( "#L4D360UI_VideoOptions_Queued_Mode" );
		m_lblDescription->SetText( "#PORTAL2_VideoOptions_QueuedMode_Info" );
	}
	else if (!V_strcmp(buttonName, "DrpShaderDetail"))
	{
		m_lblDescriptionTitle->SetText( "#GameUI_Shader_Detail" );
		m_lblDescription->SetText( "#PORTAL2_VideoOptions_ShaderDetail_Info" );
	}
	else if (!V_strcmp(buttonName, "DrpCPUDetail"))
	{
		m_lblDescriptionTitle->SetText( "#L4D360UI_VideoOptions_CPU_Detail" );
		m_lblDescription->SetText( "#PORTAL2_VideoOptions_CPUDetail_Info" );
	}
	else if (!V_strcmp(buttonName, "DrpModelDetail"))
	{
		m_lblDescriptionTitle->SetText( "#L4D360UI_VideoOptions_Model_Texture_Detail" );
		m_lblDescription->SetText( "#PORTAL2_VideoOptions_ModelDetail_Info" );
	}
	else if (!V_strcmp(buttonName, "DrpPagedPoolMem"))
	{
		m_lblDescriptionTitle->SetText( "#L4D360UI_VideoOptions_Paged_Pool_Mem" );
		m_lblDescription->SetText( "#L4D360UI_VideoOptions_Paged_Pool_Mem_Info" );
	}
}

void CAdvancedVideo::GetCurrentSettings( void )
{
	CGameUIConVarRef gpu_mem_level( "gpu_mem_level" );
	m_iModelTextureDetail = clamp( gpu_mem_level.GetInt(), 0, 3 );

	CGameUIConVarRef mat_antialias( "mat_antialias" );
	CGameUIConVarRef mat_aaquality( "mat_aaquality" );
	m_nAASamples = mat_antialias.GetInt();
	m_nAAQuality = mat_aaquality.GetInt();

	CGameUIConVarRef mat_forceaniso( "mat_forceaniso" );
	m_iFiltering = mat_forceaniso.GetInt();
	if ( m_iFiltering <= 1 )
	{
		CGameUIConVarRef mat_trilinear( "mat_trilinear" );
		m_iFiltering = mat_trilinear.GetBool() ? 1 : 0;
	}

	CGameUIConVarRef mat_vsync( "mat_vsync" );
	m_bVSync = mat_vsync.GetBool();

	CGameUIConVarRef mat_queue_mode( "mat_queue_mode" );
	m_iQueuedMode = mat_queue_mode.GetInt();

	CGameUIConVarRef gpu_level( "gpu_level" );
	m_iGPUDetail = clamp( gpu_level.GetInt(), 0, 3 );

	CGameUIConVarRef cpu_level( "cpu_level" );
	m_iCPUDetail = clamp( cpu_level.GetInt(), 0, 2 );
}

bool CAdvancedVideo::GetRecommendedSettings( void )
{
	m_iModelTextureDetail = 2;
	m_nAASamples = 1;
	m_nAAQuality = 0;
	m_iFiltering = 16;
	m_bVSync = false;
	m_bTripleBuffered = false;
	m_iGPUDetail = 2;
	m_iCPUDetail = 2;
	m_iQueuedMode = 0;

	m_bDirtyValues = true;

	return true;
}

void CAdvancedVideo::ProcessAAList()
{
	if ( !m_drpAntialias )
		return;

	// We start with no entries
	m_nNumAAModes = 0;

	char szCurrentButton[ 32 ];
	V_strncpy( szCurrentButton, VIDEO_ANTIALIAS_COMMAND_PREFIX, sizeof( szCurrentButton ) );

	int iCommandNumberPosition = Q_strlen( szCurrentButton );
	szCurrentButton[ iCommandNumberPosition + 1 ] = '\0';

	// Always have the possibility of no AA
	Assert( m_nNumAAModes < MAX_DYNAMIC_AA_MODES );
	szCurrentButton[ iCommandNumberPosition ] = m_nNumAAModes + '0';
	m_drpAntialias->ModifySelectionString( szCurrentButton, "#GameUI_None" );

	m_nAAModes[m_nNumAAModes].m_nNumSamples = 1;
	m_nAAModes[m_nNumAAModes].m_nQualityLevel = 0;
	m_nNumAAModes++;

	// Add other supported AA settings
	if ( materials->SupportsMSAAMode( 2 ) )
	{
		Assert( m_nNumAAModes < MAX_DYNAMIC_AA_MODES );
		szCurrentButton[ iCommandNumberPosition ] = m_nNumAAModes + '0';
		m_drpAntialias->ModifySelectionString( szCurrentButton, "#GameUI_2X" );

		m_nAAModes[m_nNumAAModes].m_nNumSamples = 2;
		m_nAAModes[m_nNumAAModes].m_nQualityLevel = 0;
		m_nNumAAModes++;
	}

	if ( materials->SupportsMSAAMode( 4 ) )
	{
		Assert( m_nNumAAModes < MAX_DYNAMIC_AA_MODES );
		szCurrentButton[ iCommandNumberPosition ] = m_nNumAAModes + '0';
		m_drpAntialias->ModifySelectionString( szCurrentButton, "#GameUI_4X" );

		m_nAAModes[m_nNumAAModes].m_nNumSamples = 4;
		m_nAAModes[m_nNumAAModes].m_nQualityLevel = 0;
		m_nNumAAModes++;
	}

	if ( materials->SupportsMSAAMode( 6 ) )
	{
		Assert( m_nNumAAModes < MAX_DYNAMIC_AA_MODES );
		szCurrentButton[ iCommandNumberPosition ] = m_nNumAAModes + '0';
		m_drpAntialias->ModifySelectionString( szCurrentButton, "#GameUI_6X" );

		m_nAAModes[m_nNumAAModes].m_nNumSamples = 6;
		m_nAAModes[m_nNumAAModes].m_nQualityLevel = 0;
		m_nNumAAModes++;
	}

	// nVidia CSAA "8x"
	if ( materials->SupportsCSAAMode( 4, 2 ) )							
	{
		Assert( m_nNumAAModes < MAX_DYNAMIC_AA_MODES );
		szCurrentButton[ iCommandNumberPosition ] = m_nNumAAModes + '0';
		m_drpAntialias->ModifySelectionString( szCurrentButton, "#GameUI_8X_CSAA" );

		m_nAAModes[m_nNumAAModes].m_nNumSamples = 4;
		m_nAAModes[m_nNumAAModes].m_nQualityLevel = 2;
		m_nNumAAModes++;
	}

	// nVidia CSAA "16x"
	if ( materials->SupportsCSAAMode( 4, 4 ) )							
	{
		Assert( m_nNumAAModes < MAX_DYNAMIC_AA_MODES );
		szCurrentButton[ iCommandNumberPosition ] = m_nNumAAModes + '0';
		m_drpAntialias->ModifySelectionString( szCurrentButton, "#GameUI_16X_CSAA" );

		m_nAAModes[m_nNumAAModes].m_nNumSamples = 4;
		m_nAAModes[m_nNumAAModes].m_nQualityLevel = 4;
		m_nNumAAModes++;
	}

	if ( materials->SupportsMSAAMode( 8 ) )
	{
		Assert( m_nNumAAModes < MAX_DYNAMIC_AA_MODES );
		szCurrentButton[ iCommandNumberPosition ] = m_nNumAAModes + '0';
		m_drpAntialias->ModifySelectionString( szCurrentButton, "#GameUI_8X" );

		m_nAAModes[m_nNumAAModes].m_nNumSamples = 8;
		m_nAAModes[m_nNumAAModes].m_nQualityLevel = 0;
		m_nNumAAModes++;
	}

	// nVidia CSAA "16xQ"
	if ( materials->SupportsCSAAMode( 8, 2 ) )							
	{
		Assert( m_nNumAAModes < MAX_DYNAMIC_AA_MODES );
		szCurrentButton[ iCommandNumberPosition ] = m_nNumAAModes + '0';
		m_drpAntialias->ModifySelectionString( szCurrentButton, "#GameUI_16XQ_CSAA" );

		m_nAAModes[m_nNumAAModes].m_nNumSamples = 8;
		m_nAAModes[m_nNumAAModes].m_nQualityLevel = 2;
		m_nNumAAModes++;
	}

	// Enable the valid possible choices
	for ( int i = 0; i < m_nNumAAModes; ++i )
	{
		szCurrentButton[ iCommandNumberPosition ] = i + '0';
		char szString[256];
		if ( m_drpAntialias->GetListSelectionString( szCurrentButton, szString, sizeof( szString ) ) )
		{
			m_drpAntialias->EnableListItem( szString, true );
		}
	}
	
	// Disable the remaining possible choices
	for ( int i = m_nNumAAModes; i < MAX_DYNAMIC_AA_MODES; ++i )
	{
		szCurrentButton[ iCommandNumberPosition ] = i + '0';
		char szString[256];
		if ( m_drpAntialias->GetListSelectionString( szCurrentButton, szString, sizeof( szString ) ) )
		{
			m_drpAntialias->EnableListItem( szString, false );
		}
	}

	// Select the currently set type
	m_iAntiAlias = FindMSAAMode( m_nAASamples, m_nAAQuality );
}

void CAdvancedVideo::SetAntiAliasingState()
{
	char szCurrentButton[ 32 ];
	V_strncpy( szCurrentButton, VIDEO_ANTIALIAS_COMMAND_PREFIX, sizeof( szCurrentButton ) );

	int iCommandNumberPosition = Q_strlen( szCurrentButton );
	szCurrentButton[ iCommandNumberPosition + 1 ] = '\0';

	szCurrentButton[ iCommandNumberPosition ] = m_iAntiAlias + '0';
	char szString[256];
	if ( m_drpAntialias->GetListSelectionString( szCurrentButton, szString, sizeof( szString ) ) )
	{
		m_drpAntialias->SetCurrentSelection( szString );
	}
}

void CAdvancedVideo::SetFilteringState()
{
	if ( m_drpFiltering )
	{
		switch ( m_iFiltering )
		{
		case 0:
			m_drpFiltering->SetCurrentSelection( "#GameUI_Bilinear" );
			break;
		case 1:
			m_drpFiltering->SetCurrentSelection( "#GameUI_Trilinear" );
			break;
		case 2:
			m_drpFiltering->SetCurrentSelection( "#GameUI_Anisotropic2X" );
			break;
		case 4:
			m_drpFiltering->SetCurrentSelection( "#GameUI_Anisotropic4X" );
			break;
		case 8:
			m_drpFiltering->SetCurrentSelection( "#GameUI_Anisotropic8X" );
			break;
		case 16:
			m_drpFiltering->SetCurrentSelection( "#GameUI_Anisotropic16X" );
			break;
		default:
			m_drpFiltering->SetCurrentSelection( "#GameUI_Trilinear" );
			m_iFiltering = 1;
			break;
		}
	}
}

void CAdvancedVideo::SetVSyncState()
{
	if ( m_drpVSync )
	{
		if ( m_bVSync )
		{
			if ( m_bTripleBuffered )
			{
				m_drpVSync->SetCurrentSelection( "#L4D360UI_VideoOptions_VSync_TripleBuffered" );
			}
			else
			{
				m_drpVSync->SetCurrentSelection( "#L4D360UI_VideoOptions_VSync_DoubleBuffered" );
			}
		}
		else
		{
			m_drpVSync->SetCurrentSelection( "#L4D360UI_Disabled" );
		}
	}
}

void CAdvancedVideo::SetQueuedModeState()
{
	if ( m_drpQueuedMode )
	{
		// Only allow the options on multi-processor machines.
		// every supported device has more than one core, and the count is unreliable on phones
		if ( true )
		{
			if ( m_iQueuedMode != 0 )
			{
				m_drpQueuedMode->SetCurrentSelection( "#L4D360UI_Enabled" );
			}
			else
			{
				m_drpQueuedMode->SetCurrentSelection( "#L4D360UI_Disabled" );
			}
		}
		else
		{
			m_drpQueuedMode->SetEnabled( false );
		}
	}
}

void CAdvancedVideo::SetShaderDetailState()
{
	if ( m_drpShaderDetail )
	{
		switch ( m_iGPUDetail )
		{
		case 0:
			m_drpShaderDetail->SetCurrentSelection( "#GameUI_Low" );
			break;
		case 1:
			m_drpShaderDetail->SetCurrentSelection( "#GameUI_Medium" );
			break;
		case 2:
			m_drpShaderDetail->SetCurrentSelection( "#GameUI_High" );
			break;
		case 3:
			m_drpShaderDetail->SetCurrentSelection( "#FSTOP_VeryHigh" );
			break;
		}
	}
}

void CAdvancedVideo::SetCPUDetailState()
{
	if ( m_drpCPUDetail )
	{
		switch ( m_iCPUDetail )
		{
		case 0:
			m_drpCPUDetail->SetCurrentSelection( "#GameUI_Low" );
			break;
		case 1:
			m_drpCPUDetail->SetCurrentSelection( "#GameUI_Medium" );
			break;
		case 2:
			m_drpCPUDetail->SetCurrentSelection( "#GameUI_High" );
			break;
		}
	}
}

void CAdvancedVideo::SetModelDetailState()
{
	if ( m_drpModelDetail )
	{
		switch ( m_iModelTextureDetail )
		{
		case 0:
			m_drpModelDetail->SetCurrentSelection( "#GameUI_Low" );
			break;
		case 1:
			m_drpModelDetail->SetCurrentSelection( "#GameUI_Medium" );
			break;
		case 2:
			m_drpModelDetail->SetCurrentSelection( "#GameUI_High" );
			break;
		case 3:
			m_drpModelDetail->SetCurrentSelection( "#FSTOP_VeryHigh" );
			break;
		}
	}
}

void CAdvancedVideo::SetPagedPoolState()
{
#ifndef POSIX
	if ( m_drpPagedPoolMem )
	{
		switch ( m_iPagedPoolMem )
		{
		case 0:
			m_drpPagedPoolMem->SetCurrentSelection( "#GameUI_Low" );
			break;
		case 1:
			m_drpPagedPoolMem->SetCurrentSelection( "#GameUI_Medium" );
			break;
		case 2:
			m_drpPagedPoolMem->SetCurrentSelection( "#GameUI_High" );
			break;
		}
	}
#endif
}

static void AcceptDefaultsOkCallback()
{
	CAdvancedVideo *pSelf = 
		static_cast< CAdvancedVideo* >( CBaseModPanel::GetSingleton().GetWindow( WT_ADVANCEDVIDEO ) );
	if ( pSelf )
	{
		pSelf->SetDefaults();
	}
}

static void DiscardChangesOkCallback()
{
	CAdvancedVideo *pSelf = 
		static_cast< CAdvancedVideo* >( CBaseModPanel::GetSingleton().GetWindow( WT_ADVANCEDVIDEO ) );
	if ( pSelf )
	{
		pSelf->DiscardChangesAndClose();
	}
}

void CAdvancedVideo::DiscardChangesAndClose()
{
	BaseClass::OnKeyCodePressed( ButtonCodeToJoystickButtonCode( KEY_XBUTTON_B, CBaseModPanel::GetSingleton().GetLastActiveUserId() ) );
}

void CAdvancedVideo::SetDefaults()
{
	SetupState( true );
}

void CAdvancedVideo::SetupState( bool bRecommendedSettings )
{
	if ( !bRecommendedSettings )
	{
		GetCurrentSettings();
	}
	else if ( !GetRecommendedSettings() )
	{
		return;
	}

	ProcessAAList();

	SetAntiAliasingState();
	SetFilteringState();
	SetVSyncState();
	SetQueuedModeState();
	SetShaderDetailState();
	SetCPUDetailState();
	SetModelDetailState();
	SetPagedPoolState();

	UpdateFooter();
}

void CAdvancedVideo::Activate()
{
	BaseClass::Activate();

	UpdateFooter();
}

void CAdvancedVideo::OnKeyCodePressed(KeyCode code)
{
	int joystick = GetJoystickForCode( code );
	int userId = CBaseModPanel::GetSingleton().GetLastActiveUserId();
	if ( joystick != userId || joystick < 0 )
	{	
		return;
	}

	switch ( GetBaseButtonCode( code ) )
	{
	case KEY_XBUTTON_A:
		// apply changes and close
		ApplyChanges();
		BaseClass::OnKeyCodePressed( ButtonCodeToJoystickButtonCode( KEY_XBUTTON_B, CBaseModPanel::GetSingleton().GetLastActiveUserId() ) );
		break;

	case KEY_XBUTTON_B:
		// leaving the window keeps the changes, see NavigateBack
		BaseClass::OnKeyCodePressed( code );
		break;

	case KEY_XBUTTON_X:
		// use defaults
		{
			CBaseModPanel::GetSingleton().PlayUISound( UISOUND_ACCEPT );

			GenericConfirmation *pConfirmation = 
				static_cast< GenericConfirmation* >( CBaseModPanel::GetSingleton().OpenWindow( WT_GENERICCONFIRMATION, this, true ) );

			GenericConfirmation::Data_t data;
			data.pWindowTitle = "#PORTAL2_AdvancedVideoConf";
			data.pMessageText = "#PORTAL2_VideoSettingsUseDefaultsQ";
			data.bOkButtonEnabled = true;
			data.pfnOkCallback = &AcceptDefaultsOkCallback;
			data.pOkButtonText = "#PORTAL2_ButtonAction_Reset";
			data.bCancelButtonEnabled = true;
			pConfirmation->SetUsageData( data );
		}
		break;

	default:
		BaseClass::OnKeyCodePressed(code);
		break;
	}
}

void CAdvancedVideo::OnCommand(const char *command)
{
	if ( !V_stricmp( command, "ModelDetailVeryHigh" ) )
	{
		m_iModelTextureDetail = 3;
		m_bDirtyValues = true;
	}
	else if ( !V_stricmp( command, "ModelDetailHigh" ) )
	{
		m_iModelTextureDetail = 2;
		m_bDirtyValues = true;
	}
	else if ( !V_stricmp( command, "ModelDetailMedium" ) )
	{
		m_iModelTextureDetail = 1;
		m_bDirtyValues = true;
	}
	else if ( !V_stricmp( command, "ModelDetailLow" ) )
	{
		m_iModelTextureDetail = 0;
		m_bDirtyValues = true;
	}
#ifndef POSIX
	else if ( !V_stricmp( command, "PagedPoolMemHigh" ) )
	{
		m_iPagedPoolMem = 2;
		m_bDirtyValues = true;
		
	}
	else if ( !V_stricmp( command, "PagedPoolMemMedium" ) )
	{
		m_iPagedPoolMem = 1;
		m_bDirtyValues = true;
	}
	else if ( !V_stricmp( command, "PagedPoolMemLow" ) )
	{
		m_iPagedPoolMem = 0;
		m_bDirtyValues = true;
	}
#endif
	else if ( StringHasPrefix( command, VIDEO_ANTIALIAS_COMMAND_PREFIX ) )
	{
		int iCommandNumberPosition = Q_strlen( VIDEO_ANTIALIAS_COMMAND_PREFIX );
		m_iAntiAlias = clamp( command[ iCommandNumberPosition ] - '0', 0, m_nNumAAModes - 1 );
		m_bDirtyValues = true;
	}
	else if ( !V_stricmp( command, "#GameUI_Bilinear" ) )
	{
		m_iFiltering = 0;
		m_bDirtyValues = true;
	}
	else if ( !V_stricmp( command, "#GameUI_Trilinear" ) )
	{
		m_iFiltering = 1;
		m_bDirtyValues = true;
	}
	else if ( !V_stricmp( command, "#GameUI_Anisotropic2X" ) )
	{
		m_iFiltering = 2;
		m_bDirtyValues = true;
	}
	else if ( !V_stricmp( command, "#GameUI_Anisotropic4X" ) )
	{
		m_iFiltering = 4;
		m_bDirtyValues = true;
	}
	else if ( !V_stricmp( command, "#GameUI_Anisotropic8X" ) )
	{
		m_iFiltering = 8;
		m_bDirtyValues = true;
	}
	else if ( !V_stricmp( command, "#GameUI_Anisotropic16X" ) )
	{
		m_iFiltering = 16;
		m_bDirtyValues = true;
	}
	else if ( !V_stricmp( command, "VSyncTripleBuffered" ) )
	{
		m_bVSync = true;
		m_bTripleBuffered = true;
		m_bDirtyValues = true;
	}
	else if ( !V_stricmp( command, "VSyncEnabled" ) )
	{
		m_bVSync = true;
		m_bTripleBuffered = false;
		m_bDirtyValues = true;
	}
	else if ( !V_stricmp( command, "VSyncDisabled" ) )
	{
		m_bVSync = false;
		m_bTripleBuffered = false;
		m_bDirtyValues = true;
	}
	else if ( !V_stricmp( command, "QueuedModeEnabled" ) )
	{
		m_iQueuedMode = -1;
		m_bDirtyValues = true;
	}
	else if ( !V_stricmp( command, "QueuedModeDisabled" ) )
	{
		m_iQueuedMode = 0;
		m_bDirtyValues = true;
	}
	else if ( !V_stricmp( command, "ShaderDetailVeryHigh" ) )
	{
		m_iGPUDetail = 3;
		m_bDirtyValues = true;
	}
	else if ( !V_stricmp( command, "ShaderDetailHigh" ) )
	{
		m_iGPUDetail = 2;
		m_bDirtyValues = true;
	}
	else if ( !V_stricmp( command, "ShaderDetailMedium" ) )
	{
		m_iGPUDetail = 1;
		m_bDirtyValues = true;
	}
	else if ( !V_stricmp( command, "ShaderDetailLow" ) )
	{
		m_iGPUDetail = 0;
		m_bDirtyValues = true;
	}
	else if ( !V_stricmp( command, "CPUDetailHigh" ) )
	{
		m_iCPUDetail = 2;
		m_bDirtyValues = true;
	}
	else if ( !V_stricmp( command, "CPUDetailMedium" ) )
	{
		m_iCPUDetail = 1;
		m_bDirtyValues = true;
	}
	else if ( !V_stricmp( command, "CPUDetailLow" ) )
	{
		m_iCPUDetail = 0;
		m_bDirtyValues = true;
	}
	else if ( !V_stricmp( "Cancel", command ) || !V_stricmp( "Back", command ) )
	{
		OnKeyCodePressed( ButtonCodeToJoystickButtonCode( KEY_XBUTTON_B, CBaseModPanel::GetSingleton().GetLastActiveUserId() ) );
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

int CAdvancedVideo::FindMSAAMode( int nAASamples, int nAAQuality )
{
	// Run through the AA Modes supported by the device
	for ( int nAAMode = 0; nAAMode < m_nNumAAModes; nAAMode++ )
	{
		// If we found the mode that matches what we're looking for, return the index
		if ( ( m_nAAModes[nAAMode].m_nNumSamples == nAASamples) && ( m_nAAModes[nAAMode].m_nQualityLevel == nAAQuality) )
		{
			return nAAMode;
		}
	}

	return 0;	// Didn't find what we're looking for, so no AA
}

void CAdvancedVideo::ApplyChanges()
{
	if ( !m_bDirtyValues )
	{
		// No need to apply settings
		return;
	}

	CGameUIConVarRef gpu_mem_level( "gpu_mem_level" );
	gpu_mem_level.SetValue( m_iModelTextureDetail );

	CGameUIConVarRef mat_antialias( "mat_antialias" );
	CGameUIConVarRef mat_aaquality( "mat_aaquality" );
	mat_antialias.SetValue( m_nAAModes[ m_iAntiAlias ].m_nNumSamples );
	mat_aaquality.SetValue( m_nAAModes[ m_iAntiAlias ].m_nQualityLevel );

	CGameUIConVarRef mat_forceaniso( "mat_forceaniso" );
	mat_forceaniso.SetValue( MAX( m_iFiltering, 1 ) );

	CGameUIConVarRef mat_trilinear( "mat_trilinear" );
	mat_trilinear.SetValue( m_iFiltering != 0 );

	CGameUIConVarRef mat_vsync( "mat_vsync" );
	mat_vsync.SetValue( m_bVSync );

	CGameUIConVarRef mat_queue_mode( "mat_queue_mode" );
	mat_queue_mode.SetValue( m_iQueuedMode );

	CGameUIConVarRef cpu_level( "cpu_level" );
	cpu_level.SetValue( m_iCPUDetail );

	CGameUIConVarRef gpu_level( "gpu_level" );
	gpu_level.SetValue( m_iGPUDetail );

	// apply changes
	engine->ClientCmd_Unrestricted( "mat_savechanges\n" );
	engine->ClientCmd_Unrestricted( "host_writeconfig\n" );

	m_bDirtyValues = false;
}

Panel *CAdvancedVideo::NavigateBack()
{
	ApplyChanges();
	return BaseClass::NavigateBack();
}

void CAdvancedVideo::OnThink()
{
	BaseClass::OnThink();

	if ( m_bEnableApply != m_bDirtyValues )
	{
		// enable the apply button
		m_bEnableApply = m_bDirtyValues;
		UpdateFooter();
	}
}

void CAdvancedVideo::UpdateFooter()
{
	CBaseModFooterPanel *pFooter = BaseModUI::CBaseModPanel::GetSingleton().GetFooterPanel();
	if ( pFooter )
	{
		int visibleButtons = FB_BBUTTON | FB_XBUTTON;
		if ( m_bEnableApply )
		{
			visibleButtons |= FB_ABUTTON;
		}

		pFooter->SetButtons( visibleButtons );
		pFooter->SetButtonText( FB_ABUTTON, "#GameUI_Apply" );
		pFooter->SetButtonText( FB_BBUTTON, "#L4D360UI_Back" );
		pFooter->SetButtonText( FB_XBUTTON, "#GameUI_UseDefaults" );
	}
}