#ifndef FSTOP_TOUCH_SETTINGS_H
#define FSTOP_TOUCH_SETTINGS_H
#include "tier1/convar.h"
// Shared by the touch backend and its settings UI, including desktop profile editing.
extern ConVar touch_enable, touch_draw, touch_filter, touch_forwardzone, touch_sidezone;
extern ConVar touch_pitch, touch_yaw, touch_config_file, touch_grid_count, touch_grid_enable;
extern ConVar touch_precise_amount, touch_button_info;
extern ConVar touch_enable_accel, touch_accel, touch_reverse, touch_sensitivity;
#endif
