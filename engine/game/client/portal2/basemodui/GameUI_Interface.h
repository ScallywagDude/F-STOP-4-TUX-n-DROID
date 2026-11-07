// The small part of the GameUI interface the windows call, carried out through engine commands.
#ifndef GAMEUI_INTERFACE_SHIM_H
#define GAMEUI_INTERFACE_SHIM_H
#include "cbase.h"
class CGameUIShim
{
public:
	void PreventEngineHideGameUI() { engine->ClientCmd_Unrestricted( "gameui_preventescape\n" ); }
	void AllowEngineHideGameUI() { engine->ClientCmd_Unrestricted( "gameui_allowescape\n" ); }
	void HideGameUI() { engine->ClientCmd_Unrestricted( "gameui_hide\n" ); }
	void ActivateGameUI() { engine->ClientCmd_Unrestricted( "gameui_activate\n" ); }
	void OnGameUIHidden() {}
	void OnGameUIActivated() {}
	bool IsInLevel() { return engine->IsInGame() && !engine->IsLevelMainMenuBackground(); }
	bool IsInBackgroundLevel() { return engine->IsLevelMainMenuBackground(); }
};
inline CGameUIShim &GameUI() { static CGameUIShim s; return s; }
#endif
