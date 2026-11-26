//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Special handling for Portal usable ladders
//
//=============================================================================//
#include "cbase.h"
#include "gun_portal_shared.h"
#include "hl_gamemovement.h"
#include "in_buttons.h"
#include "utlrbtree.h"
#include "movevars_shared.h"
#include "portal_shareddefs.h"
#include "portal_collideable_enumerator.h"
#include "prop_portal_shared.h"
#include "rumble_shared.h"

#if defined( CLIENT_DLL )
	#include "c_portal_player.h"
	#include "c_rumble.h"
#else
	#include "portal_player.h"
	#include "env_player_surface_trigger.h"
	#include "portal_gamestats.h"
	#include "physicsshadowclone.h"
	#include "recipientfilter.h"
	#include "SoundEmitterSystem/isoundemittersystembase.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar sv_player_trace_through_portals("sv_player_trace_through_portals", "1", FCVAR_REPLICATED | FCVAR_CHEAT, "Causes player movement traces to trace through portals." );
ConVar sv_player_funnel_into_portals("sv_player_funnel_into_portals", "1", FCVAR_REPLICATED | FCVAR_ARCHIVE | FCVAR_ARCHIVE_XBOX, "Causes the player to auto correct toward the center of floor portals." ); 

class CReservePlayerSpot;



extern bool g_bAllowForcePortalTrace;
extern bool g_bForcePortalTrace;

static inline CBaseEntity *TranslateGroundEntity( CBaseEntity *pGroundEntity )
{
#ifndef CLIENT_DLL
	CPhysicsShadowClone *pClone = dynamic_cast<CPhysicsShadowClone *>(pGroundEntity);

	if( pClone && pClone->IsUntransformedClone() )
	{
		CBaseEntity *pSource = pClone->GetClonedEntity();

		if( pSource )
			return pSource;
	}
#endif //#ifndef CLIENT_DLL

	return pGroundEntity;
}


//-----------------------------------------------------------------------------
// Purpose: Portal specific movement code
//-----------------------------------------------------------------------------
class CPortalGameMovement : public CHL2GameMovement
{
	typedef CHL2GameMovement BaseClass;
public:

	CPortalGameMovement();

	bool	m_bInPortalEnv;
	bool m_bUsingGunFunnel;
// Overrides
	virtual void ProcessMovement( CBasePlayer *pPlayer, CMoveData *pMove );
	virtual bool CheckJumpButton( void );

	bool CanFunnelIntoGunPortal( CProp_Portal *pPortal, const Vector &wishdir );
	void FunnelIntoPortal( CProp_Portal *pPortal, Vector &wishdir );

	virtual void AirAccelerate( Vector& wishdir, float wishspeed, float accel );
	virtual void AirMove( void );

	virtual void PlayerRoughLandingEffects( float fvol );

	void TracePortalPlayerHull( const Ray_t &ray, unsigned int fMask, ITraceFilter *filter, trace_t &trace );
	virtual void TryTouchGround( const Vector& start, const Vector& end, const Vector& mins, const Vector& maxs, unsigned int fMask, int collisionGroup, trace_t& pm ) OVERRIDE;

	// Traces the player bbox as it is swept from start to end
	virtual void TracePlayerBBox( const Vector& start, const Vector& end, unsigned int fMask, int collisionGroup, trace_t& pm ) OVERRIDE;

	// Tests the player position
	virtual EHANDLE	TestPlayerPosition( const Vector& pos, int collisionGroup, trace_t& pm );

	virtual void Duck( void );				// Check for a forced duck

	virtual int CheckStuck( EHANDLE &hStuckEnt );

	virtual void SetGroundEntity( trace_t *pm );

private:


	CPortal_Player	*GetPortalPlayer();
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPortalGameMovement::CPortalGameMovement()
{
	m_bUsingGunFunnel = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline CPortal_Player	*CPortalGameMovement::GetPortalPlayer()
{
	return static_cast< CPortal_Player * >( player );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pMove - 
//-----------------------------------------------------------------------------
void CPortalGameMovement::ProcessMovement( CBasePlayer *pPlayer, CMoveData *pMove )
{
    // Keep the partner/EP3 movement lifecycle, including trace and stuck-test
    // state. Portal-specific collision continues through the virtual hooks.
    m_bInPortalEnv = ( static_cast<CPortal_Player *>( pPlayer )->m_hPortalEnvironment != NULL );
    g_bAllowForcePortalTrace = m_bInPortalEnv;
    g_bForcePortalTrace = m_bInPortalEnv;
    CPortal_Player *portalPlayer = static_cast<CPortal_Player *>( pPlayer );
#ifndef CLIENT_DLL
    if ( !portalPlayer->m_hAirVentSource ||
         gpGlobals->curtime - portalPlayer->m_flAirVentLastTouch > 0.05f )
        portalPlayer->m_vecAirVentDirection.GetForModify().Init();
#endif
    const Vector windDirection = portalPlayer->m_vecAirVentDirection;
    const bool inWind = windDirection.LengthSqr() > 0.5f &&
        pPlayer->GetMoveType() == MOVETYPE_WALK && pPlayer->IsAlive();
    Vector originalBaseVelocity = pPlayer->GetBaseVelocity();
    const bool originalBaseFlag = (pPlayer->GetFlags() & FL_BASEVELOCITY) != 0;
    if ( inWind )
    {
        const float fallingBoost = MAX( -pMove->m_vecVelocity.z, 0.0f ) * 2.0f;
        Vector push = windDirection * ( 600.0f + fallingBoost * windDirection.z );
        if ( push.z > 0.0f && (pPlayer->GetFlags() & FL_ONGROUND) )
        {
            pPlayer->SetGroundEntity( NULL );
            pMove->SetAbsOrigin( pMove->GetAbsOrigin() + Vector(0,0,4) );
        }
        else
            push += windDirection * 320.0f;
        pPlayer->SetBaseVelocity( originalBaseVelocity + push );
        pPlayer->AddFlag( FL_BASEVELOCITY );
    }
    BaseClass::ProcessMovement( pPlayer, pMove );
    if ( inWind )
    {
        // Gravity consumes vertical base velocity; do not leave the wind in
        // prediction history, where it would accumulate on later commands.
        originalBaseVelocity.z = pPlayer->GetBaseVelocity().z;
        pPlayer->SetBaseVelocity( originalBaseVelocity );
        if ( !originalBaseFlag )
            pPlayer->RemoveFlag( FL_BASEVELOCITY );
    }
    g_bAllowForcePortalTrace = false;
    g_bForcePortalTrace = false;
#ifndef CLIENT_DLL
    pPlayer->UnforceButtons( IN_DUCK );
    pPlayer->UnforceButtons( IN_JUMP );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Base jump behavior, plus an anim event
// Input  :  - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPortalGameMovement::CheckJumpButton()
{
	if ( BaseClass::CheckJumpButton() && GetPortalPlayer() )
	{
		GetPortalPlayer()->DoAnimationEvent( PLAYERANIMEVENT_JUMP, 0 );
		return true;
	}

	return false;
}

bool CPortalGameMovement::CanFunnelIntoGunPortal( CProp_Portal *pPortal, const Vector &wishdir )
{
    return GunPortalCanFunnelPlayer( player, mv, pPortal, wishdir );
}

void CPortalGameMovement::FunnelIntoPortal( CProp_Portal *pPortal, Vector &wishdir )
{
    GunPortalFunnelPlayer( player, mv, pPortal, wishdir );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : wishdir - 
//			accel - 
//-----------------------------------------------------------------------------
void CPortalGameMovement::AirAccelerate( Vector& wishdir, float wishspeed, float accel )
{
	// Ordinary movement follows the EP3 base; only portal traversal needs funneling.
	if ( !m_bUsingGunFunnel )
	{
		BaseClass::AirAccelerate( wishdir, wishspeed, accel );
		return;
	}

	int i;
	float addspeed, accelspeed, currentspeed;
	float wishspd;

	wishspd = wishspeed;

	if (player->pl.deadflag)
		return;

	if (player->m_flWaterJumpTime)
		return;

	// Cap speed
	if (wishspd > 60.0f)
		wishspd = 60.0f;

	// Determine veer amount
	currentspeed = mv->m_vecVelocity.Dot(wishdir);

	// See how much to add
	addspeed = wishspd - currentspeed;

	// If not adding any, done.
	if (addspeed <= 0)
		return;

	// Determine acceleration speed after acceleration
	accelspeed = accel * wishspeed * gpGlobals->frametime * player->m_surfaceFriction;

	// Cap it
	if (accelspeed > addspeed)
		accelspeed = addspeed;

	// Adjust pmove vel.
	for (i=0 ; i<3 ; i++)
	{
		mv->m_vecVelocity[i] += accelspeed * wishdir[i];
		mv->m_outWishVel[i] += accelspeed * wishdir[i];
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPortalGameMovement::AirMove( void )
{
	m_bUsingGunFunnel = false;

	int			i;
	Vector		wishvel;
	float		fmove, smove;
	Vector		wishdir;
	float		wishspeed;
	Vector forward, right, up;

	AngleVectors (mv->m_vecViewAngles, &forward, &right, &up);  // Determine movement angles

	// Copy movement amounts
	fmove = mv->m_flForwardMove;
	smove = mv->m_flSideMove;

	// Zero out z components of movement vectors
	forward[2] = 0;
	right[2]   = 0;
	VectorNormalize(forward);  // Normalize remainder of vectors
	VectorNormalize(right);    // 

	for (i=0 ; i<2 ; i++)       // Determine x and y parts of velocity
		wishvel[i] = forward[i]*fmove + right[i]*smove;
	wishvel[2] = 0;             // Zero out z part of velocity

	VectorCopy (wishvel, wishdir);   // Determine maginitude of speed of move
	if ( sv_player_funnel_into_portals.GetBool() )
	{
		for ( int n = 0; n < CProp_Portal_Shared::AllPortals.Count(); ++n )
		{
			CProp_Portal *portal = CProp_Portal_Shared::AllPortals[n];
			if ( portal->IsActivedAndLinked() && CanFunnelIntoGunPortal( portal, wishdir ) )
			{ m_bUsingGunFunnel = true; break; }
		}
	}
	if ( !m_bUsingGunFunnel )
	{
		BaseClass::AirMove();
		return;
	}


	//
	// Don't let the player screw their fling because of adjusting into a floor portal
	//
	if ( mv->m_vecVelocity[ 0 ] * mv->m_vecVelocity[ 0 ] + mv->m_vecVelocity[ 1 ] * mv->m_vecVelocity[ 1 ] > MIN_FLING_SPEED * MIN_FLING_SPEED )
	{
		if ( mv->m_vecVelocity[ 0 ] > MIN_FLING_SPEED * 0.5f && wishdir[ 0 ] < 0.0f )
			wishdir[ 0 ] = 0.0f;
		else if ( mv->m_vecVelocity[ 0 ] < -MIN_FLING_SPEED * 0.5f && wishdir[ 0 ] > 0.0f )
			wishdir[ 0 ] = 0.0f;

		if ( mv->m_vecVelocity[ 1 ] > MIN_FLING_SPEED * 0.5f && wishdir[ 1 ] < 0.0f )
			wishdir[ 1 ] = 0.0f;
		else if ( mv->m_vecVelocity[ 1 ] < -MIN_FLING_SPEED * 0.5f && wishdir[ 1 ] > 0.0f )
			wishdir[ 1 ] = 0.0f;
	}

	//
	// Try to autocorrect the player to fall into the middle of the portal
	//
	else if ( sv_player_funnel_into_portals.GetBool() )
	{
		int iPortalCount = CProp_Portal_Shared::AllPortals.Count();
		if( iPortalCount != 0 )
		{
			CProp_Portal **pPortals = CProp_Portal_Shared::AllPortals.Base();
			for( int i = 0; i != iPortalCount; ++i )
			{
				CProp_Portal *pTempPortal = pPortals[i];
				if( pTempPortal->IsActivedAndLinked() )
				{
					FunnelIntoPortal( pTempPortal, wishdir );
				}
			}
		}
	}

	wishspeed = VectorNormalize(wishdir);

	//
	// clamp to server defined max speed
	//
	if ( wishspeed != 0 && (wishspeed > mv->m_flMaxSpeed))
	{
		VectorScale (wishvel, mv->m_flMaxSpeed/wishspeed, wishvel);
		wishspeed = mv->m_flMaxSpeed;
	}

	AirAccelerate( wishdir, wishspeed, 15.0f );

	// Add in any base velocity to the current velocity.
	VectorAdd(mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity );

	TryPlayerMove();

	// Now pull the base velocity back out.   Base velocity is set if you are on a moving object, like a conveyor (or maybe another monster?)
	VectorSubtract( mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity );
}

void CPortalGameMovement::PlayerRoughLandingEffects( float fvol )
{
	BaseClass::PlayerRoughLandingEffects( fvol );

#ifndef CLIENT_DLL
	if ( fvol >= 1.0 )
	{
		// Play the future shoes sound
		CRecipientFilter filter;
		filter.AddRecipientsByPAS( player->GetAbsOrigin() );

		CSoundParameters params;
		if ( CBaseEntity::GetParametersForSound( "PortalPlayer.FallRecover", params, NULL ) )
		{
			EmitSound_t ep( params );
			ep.m_nPitch = 125.0f - player->m_Local.m_flFallVelocity * 0.03f;					// lower pitch the harder they land
			ep.m_flVolume = min( player->m_Local.m_flFallVelocity * 0.00075f - 0.38, 1.0f );	// louder the harder they land

			CBaseEntity::EmitSound( filter, player->entindex(), ep );
		}
	}
#endif
}

// Detect the aperture on the incoming sweep, before the first touch callback
// assigns simulator ownership. Otherwise that first sweep lands on the uncut
// world brush and discards the player's incoming speed.
void CPortalGameMovement::TracePortalPlayerHull( const Ray_t &ray, unsigned int mask, ITraceFilter *filter, trace_t &trace )
{
    CProp_Portal *portal = GetPortalPlayer()->m_hPortalEnvironment;
    if ( !portal )
    {
        enginetrace->TraceRay( ray, mask, filter, &trace );
        CProp_Portal *candidate = NULL;
        if ( !UTIL_DidTraceTouchPortals( ray, trace, &candidate, NULL ) || !candidate ||
             !candidate->m_PortalSimulator.IsReadyToSimulate() )
            return;
        portal = candidate;
    }
    UTIL_Portal_TraceRay_With( portal, ray, mask, filter, &trace );
}

// The base movement implementation calls this hook for both the full ground
// hull and its four fallback quadrants. The old CategorizePosition(bool)
// overload did not override this engine's CategorizePosition(void), so walking
// onto a floor portal still found the original, uncut world floor.
void CPortalGameMovement::TryTouchGround( const Vector& start, const Vector& end, const Vector& mins, const Vector& maxs, unsigned int fMask, int collisionGroup, trace_t& pm )
{
	Ray_t ray;
	ray.Init( start, end, mins, maxs );
	CPortal_Player *pPortalPlayer = GetPortalPlayer();

#ifdef CLIENT_DLL
	CTraceFilterSimple traceFilter( pPortalPlayer, collisionGroup );
#else
	CTraceFilterSimple baseFilter( pPortalPlayer, collisionGroup );
	CTraceFilterTranslateClones traceFilter( &baseFilter );
#endif

	TracePortalPlayerHull( ray, fMask, &traceFilter, pm );
}

void CPortalGameMovement::Duck( void )
{
	return BaseClass::Duck();
}

int CPortalGameMovement::CheckStuck( EHANDLE &hStuckEnt )
{
	if( BaseClass::CheckStuck( hStuckEnt ) )
	{
		CPortal_Player *pPortalPlayer = GetPortalPlayer();

#ifndef CLIENT_DLL
		if( pPortalPlayer->IsAlive() )
			g_PortalGameStats.Event_PlayerStuck( pPortalPlayer );
#endif

		//try to fix it, then recheck
		Vector vIndecisive;
		if( pPortalPlayer->m_hPortalEnvironment )
		{
			pPortalPlayer->m_hPortalEnvironment->GetVectors( &vIndecisive, NULL, NULL );
		}
		else
		{
			vIndecisive.Init( 0.0f, 0.0f, 1.0f );
		}
		Vector ptOldOrigin = pPortalPlayer->GetAbsOrigin();

		if( pPortalPlayer->m_hPortalEnvironment )
		{
			if( !FindClosestPassableSpace( pPortalPlayer, vIndecisive ) )
			{
#ifndef CLIENT_DLL
				DevMsg( "Hurting the player for FindClosestPassableSpaceFailure!" );

				CTakeDamageInfo info( pPortalPlayer, pPortalPlayer, vec3_origin, vec3_origin, 1e10, DMG_CRUSH );
				pPortalPlayer->OnTakeDamage( info );
#endif
			}

			//make sure we didn't get put behind the portal >_<
			Vector ptCurrentOrigin = pPortalPlayer->GetAbsOrigin();
			if( vIndecisive.Dot( ptCurrentOrigin - ptOldOrigin ) < 0.0f )
			{
				pPortalPlayer->SetAbsOrigin( ptOldOrigin + (vIndecisive * 5.0f) ); //this is an anti-bug hack, since this would have probably popped them out of the world, we're just going to move them forward a few units
			}
		}

		mv->SetAbsOrigin( pPortalPlayer->GetAbsOrigin() );
		return BaseClass::CheckStuck( hStuckEnt );
	}
	else
	{
		return 0;
	}
}

void CPortalGameMovement::SetGroundEntity( trace_t *pm )
{
#if !defined(CLIENT_DLL) && !defined(FSTOP_PORTAL2)
	if ( !player->GetGroundEntity() && pm && pm->m_pEnt )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "portal_player_touchedground" );
		if ( event )
		{
			event->SetInt( "userid", player->GetUserID() );
			gameeventmanager->FireEvent( event );
		}
	}
#endif

	BaseClass::SetGroundEntity( pm );
}

void CPortalGameMovement::TracePlayerBBox( const Vector& start, const Vector& end, unsigned int fMask, int collisionGroup, trace_t& pm )
{
	VPROF( "CGameMovement::TracePlayerBBox" );
	
	CPortal_Player *pPortalPlayer = (CPortal_Player *)((CBaseEntity *)mv->m_nPlayerHandle.Get());

	Ray_t ray;
	ray.Init( start, end, GetPlayerMins(), GetPlayerMaxs() );

#ifdef CLIENT_DLL
	CTraceFilterSimple traceFilter( mv->m_nPlayerHandle.Get(), collisionGroup );
#else
	CTraceFilterSimple baseFilter( mv->m_nPlayerHandle.Get(), collisionGroup );
	CTraceFilterTranslateClones traceFilter( &baseFilter );

	//verify agreement on whether the player is in a portal simulator or not
	Assert( (pPortalPlayer->m_hPortalEnvironment.Get() != NULL) || (CPortalSimulator::GetSimulatorThatOwnsEntity( pPortalPlayer ) == NULL) );
#endif

	TracePortalPlayerHull( ray, fMask, &traceFilter, pm );

	// The entity sweep needs simulator ownership. Before the first touch it
	// falls back to the uncut world, undoing the incoming aperture sweep.
	// If we're moving through a portal and failed to hit anything with the above ray trace
	// Use UTIL_Portal_TraceEntity to test this movement through a portal and override the trace with the result
	if ( pPortalPlayer->m_hPortalEnvironment && pm.fraction == 1.0f && UTIL_DidTraceTouchPortals( ray, pm ) && sv_player_trace_through_portals.GetBool() )
	{
		trace_t tempTrace;
		UTIL_Portal_TraceEntity( pPortalPlayer, start, end, fMask, &traceFilter, &tempTrace );

		if ( tempTrace.DidHit() && tempTrace.fraction < pm.fraction && !tempTrace.startsolid && !tempTrace.allsolid )
		{
			pm = tempTrace;
		}
	}
}

EHANDLE CPortalGameMovement::TestPlayerPosition( const Vector& pos, int collisionGroup, trace_t& pm )
{
	TracePlayerBBox( pos, pos, MASK_PLAYERSOLID, collisionGroup, pm ); //hook into the existing portal special trace functionality

	//Ray_t ray;
	//ray.Init( pos, pos, GetPlayerMins(), GetPlayerMaxs() );
	//UTIL_TraceRay( ray, MASK_PLAYERSOLID, mv->m_nPlayerHandle.Get(), collisionGroup, &pm );
	if( pm.startsolid && pm.m_pEnt && (pm.contents & MASK_PLAYERSOLID) )
	{
#ifdef _DEBUG
		AssertMsgOnce( false, "The player got stuck on something. Break to investigate." ); //happens enough to just leave in a perma-debugger
		//this next trace is PURELY for tracking down how the player got stuck. Nothing new is discovered over the same trace about 10 lines up
        TracePlayerBBox( pos, pos, MASK_PLAYERSOLID, collisionGroup, pm );		
#endif
		return pm.m_pEnt;
	}
#ifndef CLIENT_DLL
	else if ( pm.startsolid && pm.m_pEnt && CPSCollisionEntity::IsPortalSimulatorCollisionEntity( pm.m_pEnt ) )
	{
		// Stuck in a portal environment object, so unstick them!
		CPortal_Player *pPortalPlayer = (CPortal_Player *)((CBaseEntity *)mv->m_nPlayerHandle.Get());
		pPortalPlayer->SetStuckOnPortalCollisionObject();

		return INVALID_EHANDLE;
	}
#endif
	else
	{	
		return INVALID_EHANDLE;
	}
}


// Expose our interface.
static CPortalGameMovement g_GameMovement;
IGameMovement *g_pGameMovement = ( IGameMovement * )&g_GameMovement;

EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CGameMovement, IGameMovement,INTERFACENAME_GAMEMOVEMENT, g_GameMovement );

