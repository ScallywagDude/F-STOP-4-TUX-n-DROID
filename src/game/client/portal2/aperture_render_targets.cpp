//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Portal mod render targets are specified by and accessable through this singleton
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "aperture_render_targets.h"
#include "materialsystem/imaterialsystem.h"
#include "rendertexture.h"
#include "bitmap/tgawriter.h"
#include "filesystem.h"
#include "tier1/utlbuffer.h"

extern CApertureRenderTargets* aperturerendertargets;

void CApertureRenderTargets::InitLargePhotoTextures(IMaterialSystem* pMaterialSystem)
{
	for (int i = 0; i != ARRAYSIZE(m_LargePhotoTextures); ++i)
	{
		char szName[256];
		Q_snprintf(szName, sizeof(szName), "_rt_LargePhoto%d", i + 1);

		m_LargePhotoTextures[i].Init(pMaterialSystem->CreateNamedRenderTargetTextureEx2(
			szName,
			256, 256, RT_SIZE_DEFAULT,
			// F-STOP: photo target: GLES can only render to four-byte colour, and it needs a depth buffer of its own size
			IMAGE_FORMAT_BGRA8888,
			MATERIAL_RT_DEPTH_SEPARATE,
			0,
			CREATERENDERTARGETFLAGS_HDR));
	}
}

/*void CApertureRenderTargets::InitSmallPhotoTextures( IMaterialSystem* pMaterialSystem )
{
	for( int i = 0; i != ARRAYSIZE( m_SmallPhotoTextures ); ++i )
	{
		char szName[256];
		sprintf( szName, "_rt_SmallPhoto%d", i + 1 );

		m_SmallPhotoTextures[i].Init( pMaterialSystem->CreateNamedRenderTargetTextureEx2(
												szName,
												32, 32, RT_SIZE_DEFAULT,
												IMAGE_FORMAT_RGB888, //pMaterialSystem->GetBackBufferFormat(),
												MATERIAL_RT_DEPTH_SHARED,
												0,
												CREATERENDERTARGETFLAGS_HDR ) );
	}
}*/

ITexture *CApertureRenderTargets::GetLargePhotoRenderTarget(int iIndex)
{
	if ((iIndex < 0) || (iIndex >= ARRAYSIZE(m_LargePhotoTextures)))
		return NULL;

	return m_LargePhotoTextures[iIndex];
}

/*ITexture *CApertureRenderTargets::GetSmallPhotoRenderTarget( int iIndex )
{
	if( (iIndex < 0) || (iIndex >= ARRAYSIZE( m_SmallPhotoTextures )) )
		return NULL;

	return m_SmallPhotoTextures[iIndex];
}*/


//-----------------------------------------------------------------------------
// Purpose: InitClientRenderTargets, interface called by the engine at material system init in the engine
// Input  : pMaterialSystem - the interface to the material system from the engine (our singleton hasn't been set up yet)
//			pHardwareConfig - the user's hardware config, useful for conditional render targets setup
//-----------------------------------------------------------------------------
void CApertureRenderTargets::InitClientRenderTargets(IMaterialSystem* pMaterialSystem, IMaterialSystemHardwareConfig* pHardwareConfig)
{
	InitLargePhotoTextures(pMaterialSystem);
	//InitSmallPhotoTextures( pMaterialSystem );

	BaseClass::InitClientRenderTargets(pMaterialSystem, pHardwareConfig);
}

//-----------------------------------------------------------------------------
// Purpose: Shutdown client render targets. This gets called during shutdown in the engine
// Input  :  - 
//-----------------------------------------------------------------------------
void CApertureRenderTargets::ShutdownClientRenderTargets()
{
	for (int i = 0; i != ARRAYSIZE(m_LargePhotoTextures); ++i)
	{
		m_LargePhotoTextures[i].Shutdown();
	}

	/*for( int i = 0; i != ARRAYSIZE( m_SmallPhotoTextures ); ++i )
	{
		m_SmallPhotoTextures[i].Shutdown();
	}*/

	BaseClass::ShutdownClientRenderTargets();
}


static CApertureRenderTargets g_ApertureRenderTargets;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CApertureRenderTargets, IClientRenderTargets, CLIENTRENDERTARGETS_INTERFACE_VERSION, g_ApertureRenderTargets);
CApertureRenderTargets* aperturerendertargets = &g_ApertureRenderTargets;
//CPortalRenderTargets* portalrendertargets = &g_ApertureRenderTargets;
// the portal renderer reads the aperture targets through the base class pointer (as in 1.1)
CPortalRenderTargets *portalrendertargets = &g_ApertureRenderTargets;
CON_COMMAND_F( fstop_dump_photo, "Write the current photo render target for visual validation.", FCVAR_CHEAT )
{
    ITexture *pTexture = aperturerendertargets->GetLargePhotoRenderTarget( 0 );
    if ( !pTexture ) return;
    int width = pTexture->GetActualWidth(), height = pTexture->GetActualHeight();
    CUtlVector<unsigned char> pixels;
    pixels.SetCount( width * height * 3 );
    CMatRenderContextPtr context( materials );
    context->PushRenderTargetAndViewport( pTexture );
    context->ReadPixels( 0, 0, width, height, pixels.Base(), IMAGE_FORMAT_RGB888 );
    context->PopRenderTargetAndViewport();
    CUtlBuffer buffer;
    TGAWriter::WriteToBuffer( pixels.Base(), buffer, width, height, IMAGE_FORMAT_RGB888, IMAGE_FORMAT_RGB888 );
    filesystem->CreateDirHierarchy( "screenshots", "GAME" );
    filesystem->WriteFile( "screenshots/photo_target.tga", "GAME", buffer );
    Msg( "FSTOP_PHOTO_TARGET %dx%d format=%d\n", width, height, pTexture->GetImageFormat() );
}

CON_COMMAND_F( fstop_dump_water, "Inspect the water refraction fog and color.", FCVAR_CHEAT )
{
    ITexture *texture = materials->FindTexture( "_rt_WaterRefraction", TEXTURE_GROUP_RENDER_TARGET );
    if ( !texture || texture->IsError() ) return;
    int w = texture->GetActualWidth(), h = texture->GetActualHeight();
    CUtlVector<unsigned char> rgba, rgb, alpha;
    rgba.SetCount( w*h*4 ); rgb.SetCount( w*h*3 ); alpha.SetCount( w*h*3 );
    CMatRenderContextPtr context( materials );
    context->PushRenderTargetAndViewport( texture );
    context->ReadPixels( 0, 0, w, h, rgba.Base(), IMAGE_FORMAT_RGBA8888 );
    context->PopRenderTargetAndViewport();
    int lo = 255, hi = 0; double sum = 0;
    for ( int i = 0; i < w*h; ++i )
    {
        int a = rgba[i*4+3]; lo = MIN(lo,a); hi = MAX(hi,a); sum += a;
        for ( int c = 0; c < 3; ++c ) { rgb[i*3+c] = rgba[i*4+c]; alpha[i*3+c] = a; }
    }
    CUtlBuffer colorBuffer, alphaBuffer;
    TGAWriter::WriteToBuffer( rgb.Base(), colorBuffer, w, h, IMAGE_FORMAT_RGB888, IMAGE_FORMAT_RGB888 );
    TGAWriter::WriteToBuffer( alpha.Base(), alphaBuffer, w, h, IMAGE_FORMAT_RGB888, IMAGE_FORMAT_RGB888 );
    filesystem->CreateDirHierarchy( "screenshots", "GAME" );
    filesystem->WriteFile( "screenshots/water_refraction.tga", "GAME", colorBuffer );
    filesystem->WriteFile( "screenshots/water_fog_alpha.tga", "GAME", alphaBuffer );
    Msg( "FSTOP_WATER_TARGET %dx%d format=%d alpha=%d..%d mean=%.3f\n", w,h,texture->GetImageFormat(),lo,hi,sum/(w*h) );
}
