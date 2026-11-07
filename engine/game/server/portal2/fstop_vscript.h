//========= F-STOP port: native versions of the maps' button_door scripts =====//
#ifndef FSTOP_VSCRIPT_H
#define FSTOP_VSCRIPT_H
#pragma once

enum
{
	FSTOP_SCRIPT_NONE = 0,
	FSTOP_SCRIPT_MAKER,
	FSTOP_SCRIPT_SIGN,
	FSTOP_SCRIPT_SIGN_TARGET,
	FSTOP_SCRIPT_DOOR,
	FSTOP_SCRIPT_BUTTON_SMALL,
	FSTOP_SCRIPT_BUTTON_NORMAL,
	FSTOP_SCRIPT_BUTTON_LARGE,
};

class CBaseEntity;

void FStopScript_ParseKeyValue( CBaseEntity *pEntity, const char *pszScripts );
void FStopScript_OnUser( CBaseEntity *pSign, int nUser );

#endif // FSTOP_VSCRIPT_H
