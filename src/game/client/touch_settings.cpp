#include "touch_settings.h"

#ifdef ANDROID
#define TOUCH_DEFAULT "1"
#else
#define TOUCH_DEFAULT "0"
#endif

ConVar touch_enable( "touch_enable", TOUCH_DEFAULT, FCVAR_ARCHIVE );
ConVar touch_draw( "touch_draw", "1", FCVAR_ARCHIVE );
ConVar touch_filter( "touch_filter", "0", FCVAR_ARCHIVE );
ConVar touch_forwardzone( "touch_forwardzone", "0.06", FCVAR_ARCHIVE, "forward touch zone" );
ConVar touch_sidezone( "touch_sidezone", "0.06", FCVAR_ARCHIVE, "side touch zone" );
ConVar touch_pitch( "touch_pitch", "90", FCVAR_ARCHIVE, "touch pitch sensitivity" );
ConVar touch_yaw( "touch_yaw", "120", FCVAR_ARCHIVE, "touch yaw sensitivity" );
ConVar touch_config_file( "touch_config_file", "touch.cfg", FCVAR_ARCHIVE, "current touch profile file" );
ConVar touch_grid_count( "touch_grid_count", "50", FCVAR_ARCHIVE, "touch grid count" );
ConVar touch_grid_enable( "touch_grid_enable", "1", FCVAR_ARCHIVE, "enable touch grid" );
ConVar touch_precise_amount( "touch_precise_amount", "0.5", FCVAR_ARCHIVE, "sensitivity multiplier for precise-look" );
ConVar touch_button_info( "touch_button_info", "0", FCVAR_ARCHIVE );
ConVar touch_enable_accel( "touch_enable_accel", "0", FCVAR_ARCHIVE );
ConVar touch_accel( "touch_accel", "1.0", FCVAR_ARCHIVE );
ConVar touch_reverse( "touch_reverse", "0", FCVAR_ARCHIVE );
ConVar touch_sensitivity( "touch_sensitivity", "3.0", FCVAR_ARCHIVE, "touch look sensitivity" );
