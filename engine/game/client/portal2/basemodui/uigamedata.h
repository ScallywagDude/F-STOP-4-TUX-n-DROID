// What the windows ask of the shared UI data object, reduced to one local player with no online services.
#ifndef UIGAMEDATA_SHIM_H
#define UIGAMEDATA_SHIM_H
#include "basemodui_compat.h"
#include "matchmaking/imatchframework.h"
#include "tier1/fmtstr.h"

namespace BaseModUI
{
class CChangeStorageDevice
{
public:
	explicit CChangeStorageDevice( int ) : m_bAnyController( false ), m_bForce( false ) {}
	virtual ~CChangeStorageDevice() {}
	virtual void DeviceChangeCompleted( bool ) {}
	bool m_bAnyController;
	bool m_bForce;
};

class CUIGameData
{
public:
	static CUIGameData *Get() { static CUIGameData s; return &s; }
	bool OpenWaitScreen( const char *, float = 0.0f, KeyValues * = NULL ) { return true; }
	bool CloseWaitScreen( vgui::Panel *, const char * ) { return false; }
	bool SignedInToLive() { return false; }
	void GameStats_ReportAction( const char *, const char *, int ) {}
	void ExecuteOverlayCommand( const char *, const char * = NULL ) {}
	bool CheckAndDisplayErrorIfOffline( void *, const char * ) { return false; }
	bool CheckAndDisplayErrorIfNotSignedInToLive( void * ) { return false; }
	bool IsNetworkCableConnected() { return true; }
	int GetNumOnlineFriends() { return 0; }
	bool CanPlayer( void *, int, void * = NULL ) { return true; }
	void InitiateSplitscreenPartnerDetection() {}
	void SelectStorageDevice( CChangeStorageDevice *p ) { delete p; }
	float GetLookSensitivity() { ConVarRef v( "sensitivity" ); return v.GetFloat(); }
	void SetLookSensitivity( float f ) { ConVarRef v( "sensitivity" ); v.SetValue( f ); }
};
}
inline bool UI_IsDebug() { return false; }
#endif
