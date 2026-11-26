//========= F-STOP partner port ===============================================//
//
// Purpose: Steam Audio (phonon) is an external binary that this build does not
//          ship. These are the entry points the sound code calls. Creation
//          reports failure, so HRTF and direct-sound effects stay off and the
//          mixer uses its normal spatialization; the apply calls are never
//          reached because no effect object is ever handed out.
//
//=============================================================================//

#include "phonon/phonon.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IPLerror iplCreateScene( IPLContext context, IPLhandle computeDevice, IPLSimulationSettings simulationSettings, IPLint32 numMaterials, IPLhandle *scene )
{
	if ( scene )
		*scene = NULL;
	return IPL_STATUS_FAILURE;
}

IPLvoid iplSetSceneMaterial( IPLhandle scene, IPLint32 materialIndex, IPLMaterial material )
{
}

IPLvoid iplFinalizeScene( IPLhandle scene, IPLFinalizeSceneProgressCallback progressCallback )
{
}

IPLerror iplCreateEnvironment( IPLContext context, IPLhandle computeDevice, IPLSimulationSettings simulationSettings, IPLhandle scene, IPLhandle probeManager, IPLhandle *environment )
{
	if ( environment )
		*environment = NULL;
	return IPL_STATUS_FAILURE;
}

IPLerror iplCreateBinauralRenderer( IPLContext context, IPLRenderingSettings renderingSettings, IPLHrtfParams params, IPLhandle *renderer )
{
	if ( renderer )
		*renderer = NULL;
	return IPL_STATUS_FAILURE;
}

IPLerror iplCreateBinauralEffect( IPLhandle renderer, IPLAudioFormat inputFormat, IPLAudioFormat outputFormat, IPLhandle *effect )
{
	if ( effect )
		*effect = NULL;
	return IPL_STATUS_FAILURE;
}

IPLvoid iplDestroyBinauralEffect( IPLhandle *effect )
{
	if ( effect )
		*effect = NULL;
}

IPLvoid iplApplyBinauralEffectWithParameters( IPLhandle effect, IPLAudioBuffer inputAudio, IPLVector3 direction, IPLHrtfInterpolation interpolation, IPLAudioBuffer outputAudio, IPLfloat32 *leftDelay, IPLfloat32 *rightDelay )
{
	if ( leftDelay )
		*leftDelay = 0.0f;
	if ( rightDelay )
		*rightDelay = 0.0f;
}

IPLerror iplCreateEnvironmentalRenderer( IPLContext context, IPLhandle environment, IPLRenderingSettings renderingSettings, IPLAudioFormat outputFormat, IPLSimulationThreadCreateCallback threadCreateCallback, IPLSimulationThreadDestroyCallback threadDestroyCallback, IPLhandle *renderer )
{
	if ( renderer )
		*renderer = NULL;
	return IPL_STATUS_FAILURE;
}

IPLerror iplCreateDirectSoundEffect( IPLhandle renderer, IPLAudioFormat inputFormat, IPLAudioFormat outputFormat, IPLhandle *effect )
{
	if ( effect )
		*effect = NULL;
	return IPL_STATUS_FAILURE;
}

IPLvoid iplDestroyDirectSoundEffect( IPLhandle *effect )
{
	if ( effect )
		*effect = NULL;
}

IPLvoid iplApplyDirectSoundEffect( IPLhandle effect, IPLAudioBuffer inputAudio, IPLDirectSoundPath directSoundPath, IPLDirectSoundEffectOptions options, IPLAudioBuffer outputAudio )
{
}
