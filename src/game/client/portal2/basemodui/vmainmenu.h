// The main menu window: console, continue, new game, load game, options, quit.
#ifndef __VMAINMENU_H__
#define __VMAINMENU_H__

#include "basemodui.h"
#include "utlstring.h"

namespace BaseModUI
{
// Stacks a menu's visible buttons up from its bottom row and fits the "Backing" panel around them.
void CompactMenuRows( vgui::Panel *pMenu );

class MainMenu : public CBaseModFrame
{
	DECLARE_CLASS_SIMPLE( MainMenu, CBaseModFrame );

public:
	MainMenu( vgui::Panel *parent, const char *panelName );
	~MainMenu();

protected:
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void LoadLayout();
	virtual void OnCommand( const char *command );
	virtual void OnKeyCodePressed( vgui::KeyCode code );
	virtual void PaintBackground();

private:
	static void AcceptQuitGameCallback();

	CUtlString m_ContinueMap;	// newest save, or empty when there is none
	CUtlString m_ContinueFile;
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
