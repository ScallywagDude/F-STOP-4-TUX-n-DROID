// Stands in for the matchmaking framework: F-STOP is single player, so there is never a session.
#ifndef IMATCHFRAMEWORK_SHIM_H
#define IMATCHFRAMEWORK_SHIM_H
#include "tier1/KeyValues.h"

class IMatchEventsSink
{
public:
	virtual void OnEvent( KeyValues *pEvent ) {}
};
class IMatchEventsSubscription
{
public:
	void Subscribe( IMatchEventsSink * ) {}
	void Unsubscribe( IMatchEventsSink * ) {}
	void BroadcastEvent( KeyValues *pEvent ) { if ( pEvent ) pEvent->deleteThis(); }
};
class IMatchSession
{
public:
	KeyValues *GetSessionSettings() { return NULL; }
	void UpdateSessionSettings( KeyValues * ) {}
	void Command( KeyValues * ) {}
};
class IMatchAsyncOperation;
class IPlayerLocal;
class IMatchFramework
{
public:
	IMatchSession *GetMatchSession() { return NULL; }
	void CreateSession( KeyValues * ) {}
	void MatchSession( KeyValues * ) {}
	void CloseSession() {}
	IMatchEventsSubscription *GetEventsSubscription() { static IMatchEventsSubscription s; return &s; }
};
inline IMatchFramework *MatchFrameworkShim() { static IMatchFramework s; return &s; }
#define g_pMatchFramework MatchFrameworkShim()
#endif
