#ifndef GUN_PORTAL_SHARED_H
#define GUN_PORTAL_SHARED_H
#include "prop_portal_shared.h"
class CMoveData;
bool GunPortalContainsPoint( const CProp_Portal *portal, const Vector &point );
bool GunPortalCanFunnelPlayer( CBasePlayer *player, CMoveData *mv, CProp_Portal *pPortal, const Vector &wishdir );
void GunPortalFunnelPlayer( CBasePlayer *player, CMoveData *mv, CProp_Portal *pPortal, Vector &wishdir );
#endif
