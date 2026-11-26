// Portal-gun aperture and Portal 1 movement policy. Portable doors never use these.
#include "cbase.h"
#include "gamemovement.h"
#include "gun_portal_shared.h"
#include "tier0/memdbgon.h"
static const float PORTAL_FUNNEL_AMOUNT = 6.0f;
bool GunPortalContainsPoint( const CProp_Portal *portal, const Vector &point )
{
    Vector right, up; portal->GetVectors( NULL, &right, &up );
    Vector offset = point - portal->GetAbsOrigin();
    float x = offset.Dot( right ) / MAX( portal->GetHalfWidth(), 1.0f );
    float y = offset.Dot( up ) / MAX( portal->GetHalfHeight(), 1.0f );
    return x*x + y*y <= 1.0001f;
}
bool GunPortalCanFunnelPlayer( CBasePlayer *player, CMoveData *mv, CProp_Portal *pPortal, const Vector &wishdir )
{
	// Make sure there's a portal
	if ( !pPortal || !pPortal->IsGunPortal() )
		return false;

	// Get portal vectors
	Vector vPortalForward, vPortalRight, vPortalUp;
	pPortal->GetVectors( &vPortalForward, &vPortalRight, &vPortalUp );

	// Make sure it's a floor portal
	if ( vPortalForward.z < 0.8f )
		return false;

	vPortalRight.z = 0.0f;
	vPortalUp.z = 0.0f;
	VectorNormalize( vPortalRight );
	VectorNormalize( vPortalUp );

	// Make sure the player is looking downward
	CBasePlayer *pPlayer = player;

	Vector vPlayerForward;
	pPlayer->EyeVectors( &vPlayerForward );

	if ( vPlayerForward.z > -0.1f )
		return false;

	Vector vPlayerOrigin = pPlayer->GetAbsOrigin();
	Vector vPlayerToPortal = pPortal->GetAbsOrigin() - vPlayerOrigin;

	// Make sure the player is trying to air control, they're falling downward and they are vertically close to the portal
	if ( fabsf( wishdir[ 0 ] ) > 64.0f || fabsf( wishdir[ 1 ] ) > 64.0f || mv->m_vecVelocity[ 2 ] > -165.0f || vPlayerToPortal.z < -512.0f )
		return false;

	// Make sure we're in the 2D portal rectangle
	if ( ( vPlayerToPortal.Dot( vPortalRight ) * vPortalRight ).Length() > pPortal->GetHalfWidth() * 1.5f )
		return false;
	if ( ( vPlayerToPortal.Dot( vPortalUp ) * vPortalUp ).Length() > pPortal->GetHalfHeight() * 1.5f )
		return false;

	return true;
}
void GunPortalFunnelPlayer( CBasePlayer *player, CMoveData *mv, CProp_Portal *pPortal, Vector &wishdir )
{
	if ( !GunPortalCanFunnelPlayer( player, mv, pPortal, wishdir ) ) return;
	Vector vPlayerToPortal = pPortal->GetAbsOrigin() - player->GetAbsOrigin();

	if ( vPlayerToPortal.z > -8.0f )
	{
		// We're too close the the portal to continue correcting, but zero the velocity so our fling velocity is nice
		mv->m_vecVelocity[ 0 ] = 0.0f;
		mv->m_vecVelocity[ 1 ] = 0.0f;
	}
	else
	{
		// Funnel toward the portal
		float fFunnelX = vPlayerToPortal.x * PORTAL_FUNNEL_AMOUNT - mv->m_vecVelocity[ 0 ];
		float fFunnelY = vPlayerToPortal.y * PORTAL_FUNNEL_AMOUNT - mv->m_vecVelocity[ 1 ];

		wishdir[ 0 ] += fFunnelX;
		wishdir[ 1 ] += fFunnelY;
	}
}
