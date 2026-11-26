//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Linux/Android touch implementation for inputsystem
//
// SDL finger events arrive through the launcher manager's locked event queue (sdlmgr.cpp / sdlmgr_gles.cpp turn
// SDL_FINGER* into CocoaEvent_Finger*), so they are handled here on the input system's own thread. On Android SDL
// pushes touch from the Java UI thread; an SDL event watch would run there and race the event queue.
//
//===========================================================================//

#include "inputsystem.h"
#include "tier0/icommandline.h"
// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Initialize touch
//-----------------------------------------------------------------------------
void CInputSystem::InitializeTouch( void )
{
	if ( m_bTouchInitialized )
		ShutdownTouch();

	// abort startup if user requests no touch
	if ( CommandLine()->FindParm( "-notouch" ) )
		return;

	memset( m_touchAccumX, 0, sizeof( m_touchAccumX ) );
	memset( m_touchAccumY, 0, sizeof( m_touchAccumY ) );

	m_bTouchInitialized = true;
}

void CInputSystem::ShutdownTouch()
{
	m_bTouchInitialized = false;
}

bool CInputSystem::GetTouchAccumulators( int fingerId, float &dx, float &dy )
{
	if ( fingerId < 0 || fingerId >= TOUCH_FINGER_MAX_COUNT )
	{
		dx = dy = 0.f;
		return false;
	}

	dx = m_touchAccumX[fingerId];
	dy = m_touchAccumY[fingerId];

	m_touchAccumX[fingerId] = m_touchAccumY[fingerId] = 0.f;

	return true;
}

void CInputSystem::FingerEvent( int eventType, int fingerId, float x, float y, float dx, float dy )
{
	if ( !m_bTouchInitialized || fingerId < 0 || fingerId >= TOUCH_FINGER_MAX_COUNT )
		return;

	if ( eventType == IE_FingerUp )
	{
		m_touchAccumX[fingerId] = 0.f;
		m_touchAccumY[fingerId] = 0.f;
	}
	else
	{
		m_touchAccumX[fingerId] += dx;
		m_touchAccumY[fingerId] += dy;
	}

	int _x, _y;
	memcpy( &_x, &x, sizeof( float ) );
	memcpy( &_y, &y, sizeof( float ) );
	PostEvent( eventType, m_nLastSampleTick, fingerId, _x, _y );
}
