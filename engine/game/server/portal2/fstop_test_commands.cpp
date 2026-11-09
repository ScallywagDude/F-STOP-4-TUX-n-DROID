// Read-only collision diagnostics for test builds. The normal game target does
// not compile or register this command.
#ifdef FSTOP_TEST_HARNESS
#include "cbase.h"
#include "prop_portal.h"
#include "prop_portal_shared.h"
#include "portal_player.h"
#include "portal_util_shared.h"
#include "PortalSimulation.h"
#include "physicsshadowclone.h"

#include "tier0/memdbgon.h"

static void FStopPrintPortalTrace( const char *pLabel, const trace_t &tr )
{
	Msg( "FSTOP_PROBE %s fraction=%.6f startsolid=%d allsolid=%d hit=%d/%s contents=%x end=(%.3f %.3f %.3f) normal=(%.3f %.3f %.3f)\n",
		pLabel, tr.fraction, tr.startsolid, tr.allsolid,
		tr.m_pEnt ? tr.m_pEnt->entindex() : -1, tr.m_pEnt ? tr.m_pEnt->GetClassname() : "none", tr.contents,
		tr.endpos.x, tr.endpos.y, tr.endpos.z, tr.plane.normal.x, tr.plane.normal.y, tr.plane.normal.z );
}

CON_COMMAND_F( fstop_portal_probe, "Print player, portable portal support, physics pointers and center-crossing hull traces without moving anything.", FCVAR_CHEAT )
{
	CPortal_Player *pPlayer = ToPortalPlayer( UTIL_GetLocalPlayer() );
	if ( !pPlayer )
		return;

	Vector origin = pPlayer->GetAbsOrigin();
	QAngle angles = pPlayer->EyeAngles();
	Msg( "FSTOP_PROBE player=(%.3f %.3f %.3f) angles=(%.3f %.3f %.3f) environment=%d frame=%d frametime=%.6f\n",
		origin.x, origin.y, origin.z, angles.x, angles.y, angles.z,
		pPlayer->m_hPortalEnvironment.Get() ? pPlayer->m_hPortalEnvironment->entindex() : -1, gpGlobals->framecount, gpGlobals->frametime );

	CTraceFilterSimple basePlayerFilter( pPlayer, COLLISION_GROUP_PLAYER_MOVEMENT );
	CTraceFilterTranslateClones playerFilter( &basePlayerFilter );
	Vector playerForward;
	AngleVectors( angles, &playerForward );
	Ray_t playerRay;
	playerRay.Init( origin, origin + playerForward * 64.0f, pPlayer->WorldAlignMins(), pPlayer->WorldAlignMaxs() );
	trace_t tr;
	enginetrace->TraceRay( playerRay, MASK_PLAYERSOLID, &playerFilter, &tr );
	FStopPrintPortalTrace( "player_forward_real", tr );
	UTIL_Portal_TraceRay_With( pPlayer->m_hPortalEnvironment, playerRay, MASK_PLAYERSOLID, &playerFilter, &tr );
	FStopPrintPortalTrace( "player_forward_environment", tr );

	Vector horizontal( playerForward.x, playerForward.y, 0.0f );
	VectorNormalize( horizontal );
	const char *pWalkLabels[] = { "walk_level", "walk_raised_step", "ground_down" };
	for ( int walk = 0; walk < 3; ++walk )
	{
		Vector start = origin;
		if ( walk == 1 )
			start.z += pPlayer->m_Local.m_flStepSize;
		Vector end = walk == 2 ? start - Vector( 0, 0, 4.0f ) : start + horizontal * 64.0f;
		Ray_t walkRay;
		walkRay.Init( start, end, pPlayer->WorldAlignMins(), pPlayer->WorldAlignMaxs() );
		enginetrace->TraceRay( walkRay, MASK_PLAYERSOLID, &playerFilter, &tr );
		FStopPrintPortalTrace( pWalkLabels[walk], tr );
		if ( pPlayer->m_hPortalEnvironment.Get() && pPlayer->m_hPortalEnvironment->m_PortalSimulator.IsReadyToSimulate() )
		{
			UTIL_Portal_TraceRay( pPlayer->m_hPortalEnvironment, walkRay, MASK_PLAYERSOLID, &playerFilter, &tr );
			FStopPrintPortalTrace( "walk_simulator", tr );
		}
	}

	// Match the four ground sub-hulls used after a full player trace misses.
	// Comparing filters exposes self-contact with the player's shadow clone.
	if ( pPlayer->m_hPortalEnvironment.Get() && pPlayer->m_hPortalEnvironment->m_PortalSimulator.IsReadyToSimulate() )
	{
		Vector down = origin - Vector( 0, 0, 2.0f );
		const Vector fullMins = pPlayer->WorldAlignMins();
		const Vector fullMaxs = pPlayer->WorldAlignMaxs();
		for ( int quadrant = 0; quadrant < 4; ++quadrant )
		{
			Vector mins = fullMins, maxs = fullMaxs;
			if ( quadrant & 1 ) mins.x = MAX( 0.0f, fullMins.x );
			else maxs.x = MIN( 0.0f, fullMaxs.x );
			if ( quadrant & 2 ) mins.y = MAX( 0.0f, fullMins.y );
			else maxs.y = MIN( 0.0f, fullMaxs.y );
			Ray_t groundRay;
			groundRay.Init( origin, down, mins, maxs );
			Msg( "FSTOP_PROBE ground_quadrant=%d\n", quadrant );
			UTIL_Portal_TraceRay( pPlayer->m_hPortalEnvironment, groundRay, MASK_PLAYERSOLID, &basePlayerFilter, &tr );
			FStopPrintPortalTrace( "ground_simple_filter", tr );
			UTIL_Portal_TraceRay( pPlayer->m_hPortalEnvironment, groundRay, MASK_PLAYERSOLID, &playerFilter, &tr );
			FStopPrintPortalTrace( "ground_clone_filter", tr );
		}
	}
	CBaseEntity *pGround = pPlayer->GetGroundEntity();
	Msg( "FSTOP_PROBE ground=%d/%s flags=%x velocity=(%.3f %.3f %.3f)\n",
		pGround ? pGround->entindex() : -1, pGround ? pGround->GetClassname() : "none", pPlayer->GetFlags(),
		pPlayer->GetAbsVelocity().x, pPlayer->GetAbsVelocity().y, pPlayer->GetAbsVelocity().z );

	for ( int i = 0; i < CProp_Portal_Shared::AllPortals.Count(); ++i )
	{
		CProp_Portal *pPortal = CProp_Portal_Shared::AllPortals[i];
		if ( !pPortal || !pPortal->m_bActivated || pPortal->IsMarkedForDeletion() )
			continue;
		if ( args.ArgC() > 1 && atoi( args[1] ) != pPortal->entindex() )
			continue;

		Vector center = pPortal->GetAbsOrigin(), forward;
		QAngle portalAngles = pPortal->GetAbsAngles();
		pPortal->GetVectors( &forward, NULL, NULL );
		CBaseEntity *pOwner = pPortal->GetOwnerEntity();
		const CPortalSimulator &sim = pPortal->m_PortalSimulator;
		Msg( "FSTOP_PROBE portal=%d group=%d linked=%d owner=%d/%s pos=(%.3f %.3f %.3f) angles=(%.3f %.3f %.3f) size=(%.3f %.3f) ready=%d owns_player=%d hole=%d\n",
			pPortal->entindex(), pPortal->GetLinkageGroup(), pPortal->m_hLinkedPortal.Get() ? pPortal->m_hLinkedPortal->entindex() : -1,
			pOwner ? pOwner->entindex() : -1, pOwner ? pOwner->GetClassname() : "none",
			center.x, center.y, center.z, portalAngles.x, portalAngles.y, portalAngles.z,
			pPortal->GetHalfWidth(), pPortal->GetHalfHeight(), sim.IsReadyToSimulate(), sim.OwnsEntity( pPlayer ), sim.EntityIsInPortalHole( pPlayer ) );

		CTraceFilterSkipTwoEntities supportFilter( pPortal, pOwner, COLLISION_GROUP_NONE );
		UTIL_TraceLine( center + forward * 2.0f, center - forward * 32.0f, MASK_SHOT_PORTAL, &supportFilter, &tr );
		FStopPrintPortalTrace( "support_back", tr );

		const PS_InternalData_t &data = sim.m_DataAccess;
		Msg( "FSTOP_PROBE physics world=%p displacement=%p wall=%p tube=%p remote=%p getter=%p\n",
			(void *)data.Simulation.Static.World.Brushes.pPhysicsObject, (void *)data.Simulation.Static.World.Displacements.pPhysicsObject,
			(void *)data.Simulation.Static.Wall.Local.Brushes.pPhysicsObject, (void *)data.Simulation.Static.Wall.Local.Tube.pPhysicsObject,
			(void *)data.Simulation.Static.Wall.RemoteTransformedToLocal.Brushes.pPhysicsObject,
			data.Simulation.pCollisionEntity ? (void *)data.Simulation.pCollisionEntity->VPhysicsGetObject() : NULL );

		// The same upright player hull follows the door normal from 64 units in
		// front to 64 behind. This also covers horizontal floor/ceiling portals.
		Vector centerOffset = pPlayer->WorldSpaceCenter() - origin;
		Vector start = center + forward * 64.0f - centerOffset;
		Vector end = center - forward * 64.0f - centerOffset;
		Ray_t crossing;
		crossing.Init( start, end, pPlayer->WorldAlignMins(), pPlayer->WorldAlignMaxs() );
		enginetrace->TraceRay( crossing, MASK_PLAYERSOLID, &playerFilter, &tr );
		FStopPrintPortalTrace( "center_cross_real", tr );
		if ( sim.IsReadyToSimulate() )
		{
			UTIL_Portal_TraceRay( pPortal, crossing, MASK_PLAYERSOLID, &playerFilter, &tr );
			FStopPrintPortalTrace( "center_cross_simulator", tr );
			UTIL_Portal_TraceEntity( pPlayer, start, end, MASK_PLAYERSOLID, &playerFilter, &tr );
			FStopPrintPortalTrace( "center_cross_entity", tr );
		}
	}
}
#endif
