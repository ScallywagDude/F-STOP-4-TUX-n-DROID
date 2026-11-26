// The pause menu window: resume, save, load, options, leave.
#ifndef __VINGAMEMAINMENU_H__
#define __VINGAMEMAINMENU_H__

#include "basemodui.h"

namespace BaseModUI
{
class InGameMainMenu : public CBaseModFrame
{
	DECLARE_CLASS_SIMPLE( InGameMainMenu, CBaseModFrame );

public:
	InGameMainMenu( vgui::Panel *parent, const char *panelName );

protected:
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnCommand( const char *command );
	virtual void OnKeyCodePressed( vgui::KeyCode code );
	virtual void PaintBackground();

private:
	static void AcceptLeaveCallback();
	static void AcceptQuitCallback();
};
}

#endif
