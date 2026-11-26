// There is no wait screen in a single player menu. The window type never opens, so lookups find nothing.
#ifndef VGENERICWAITSCREEN_SHIM_H
#define VGENERICWAITSCREEN_SHIM_H
#include "basemodframe.h"
namespace BaseModUI
{
class GenericWaitScreen : public CBaseModFrame
{
public:
	GenericWaitScreen( vgui::Panel *parent, const char *name ) : CBaseModFrame( parent, name ) {}
};
}
#endif
