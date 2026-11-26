#ifndef PLAYER_SHADOW_COLLISION_H
#define PLAYER_SHADOW_COLLISION_H
#ifdef _WIN32
#pragma once
#endif

// A clear endpoint does not prove a physics correction can reach it. Sweep
// from a valid game position so a stale shadow cannot pull a player through
// a wall into the empty space outside the level.
template <typename TraceHull>
Vector ClipPlayerShadowPosition( const Vector &current, const Vector &previous,
                                const Vector &proposed, TraceHull traceHull )
{
    trace_t trace;
    traceHull( current, proposed, trace );
    if ( !trace.startsolid && !trace.allsolid )
        return trace.endpos;

    // A moving physics prop can overlap the current hull. Only use the prior
    // position as a recovery anchor if that hull is still clear.
    traceHull( previous, previous, trace );
    if ( trace.startsolid || trace.allsolid )
        return current;
    traceHull( previous, proposed, trace );
    return ( trace.startsolid || trace.allsolid ) ? previous : trace.endpos;
}

inline Vector ClipPlayerShadowToWorld( CBasePlayer *player, const Vector &previous,
                                     const Vector &proposed )
{
    return ClipPlayerShadowPosition( player->GetAbsOrigin(), previous, proposed,
        [player]( const Vector &start, const Vector &end, trace_t &trace )
        {
            Ray_t ray;
            ray.Init( start, end, player->WorldAlignMins(), player->WorldAlignMaxs() );
            CTraceFilterWorldAndPropsOnly filter;
            enginetrace->TraceRay( ray, MASK_PLAYERSOLID, &filter, &trace );
        } );
}
#endif
