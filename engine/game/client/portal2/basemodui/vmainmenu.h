// The main menu window: new game, load game, options, console, quit.
#ifndef __VMAINMENU_H__
#define __VMAINMENU_H__

#include "basemodui.h"

namespace BaseModUI
{
class MainMenu : public CBaseModFrame
{
	DECLARE_CLASS_SIMPLE( MainMenu, CBaseModFrame );

public:
	MainMenu( vgui::Panel *parent, const char *panelName );
	~MainMenu();

protected:
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnCommand( const char *command );
	virtual void OnKeyCodePressed( vgui::KeyCode code );
	virtual void PaintBackground();

private:
	static void AcceptQuitGameCallback();
};

class OptionsMenu : public CBaseModFrame
{
	DECLARE_CLASS_SIMPLE( OptionsMenu, CBaseModFrame );

public:
	OptionsMenu( vgui::Panel *parent, const char *panelName );

protected:
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void PaintBackground();
	virtual void OnCommand( const char *command );
};
}

#endif
