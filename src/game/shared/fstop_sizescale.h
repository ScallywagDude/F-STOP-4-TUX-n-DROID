//=============================================================================
// F-STOP: animation speed of resized things, shared by the server and client baseanimating headers.
//=============================================================================
#ifndef FSTOP_SIZESCALE_H
#define FSTOP_SIZESCALE_H
#ifdef _WIN32
#pragma once
#endif
#include <math.h>

// Keep the original size-based animation timing for props and NPCs alike.
// An extra NPC-only inverse-scale factor made a 4x NPC play at 1/8 speed,
// including attacks. The original rule gives it 1/2 speed instead.
inline float FStop_SizePlaybackScale( float flScale, bool bNPC )
{
	flScale = ( flScale > 0.001f ? flScale : 0.001f );
	return 1.0f / sqrtf( flScale );
}

#endif // FSTOP_SIZESCALE_H
