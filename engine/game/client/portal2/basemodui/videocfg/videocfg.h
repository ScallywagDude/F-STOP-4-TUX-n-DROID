// The video windows read and write settings as keys named after console variables.
#ifndef VIDEOCFG_SHIM_H
#define VIDEOCFG_SHIM_H
#include "cbase.h"
#include "tier1/KeyValues.h"
#include "vgui/ISurface.h"
#include "vgui_controls/Controls.h"

inline bool ReadCurrentVideoConfig( KeyValues *pConfig, bool = false )
{
	static const char *s_Names[] = { "cpu_level", "gpu_level", "gpu_mem_level", "mem_level", "mat_antialias", "mat_aaquality",
		"mat_forceaniso", "mat_queue_mode", "mat_triplebuffered", "mat_vsync" };
	for ( int i = 0; i < ARRAYSIZE( s_Names ); ++i )
	{
		ConVarRef v( s_Names[i], true );
		if ( v.IsValid() )
			pConfig->SetInt( CFmtStr( "setting.%s", s_Names[i] ), v.GetInt() );
	}
	int wide, tall;
	vgui::surface()->GetScreenSize( wide, tall );
	pConfig->SetInt( "setting.defaultres", wide );
	pConfig->SetInt( "setting.defaultresheight", tall );
	pConfig->SetInt( "setting.fullscreen", 1 );
	pConfig->SetInt( "setting.nowindowborder", 0 );
	return true;
}

inline void UpdateCurrentVideoConfig( int, int, int, bool, bool = false )
{
	engine->ClientCmd_Unrestricted( "mat_savechanges\nhost_writeconfig\n" );
}
#endif
