//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#include "cbase.h"
#include "C_PortalGhostRenderable.h"
#include "PortalRender.h"
#include "c_portal_player.h"
#include "model_types.h"
#include "c_basecombatweapon.h"

static CUtlVector<C_PortalGhostRenderable *> s_PortalGhosts;

C_PortalGhostRenderable::C_PortalGhostRenderable( C_Prop_Portal *pOwningPortal, C_BaseEntity *pGhostSource, RenderableTranslucencyType_t nType, const VMatrix &matGhostTransform, float *pSharedRenderClipPlane, bool bLocalPlayer )
: m_pGhostedRenderable( pGhostSource ), 
	m_matGhostTransform( matGhostTransform ), 
	m_pSharedRenderClipPlane( pSharedRenderClipPlane ),
	m_bLocalPlayer( bLocalPlayer ),
	m_pOwningPortal( pOwningPortal )
{
	s_PortalGhosts.AddToTail( this );
	// The portal updates this render-only proxy; it is not an independent actor.
	RemoveFromEntityList( ENTITY_LIST_SIMULATE );
	m_bSourceIsBaseAnimating = (dynamic_cast<C_BaseAnimating *>(pGhostSource) != NULL);

	cl_entitylist->AddNonNetworkableEntity( GetIClientUnknown() );

	RenderableModelType_t nModelType;
	int nSrcType = modelinfo->GetModelType( pGhostSource->GetModel() );
	switch( nSrcType )
	{
	default: nModelType = RENDERABLE_MODEL_ENTITY; break;
	case mod_brush: nModelType = RENDERABLE_MODEL_BRUSH; break;
	case mod_studio: nModelType = RENDERABLE_MODEL_STUDIOMDL; break;
	}

	g_pClientLeafSystem->AddRenderable( this, pGhostSource->IsRenderingWithViewModels(), nType, nModelType );
}

C_PortalGhostRenderable::~C_PortalGhostRenderable( void )
{
    s_PortalGhosts.FindAndRemove( this );
	m_pGhostedRenderable = NULL;
	g_pClientLeafSystem->RemoveRenderable( RenderHandle() );
	cl_entitylist->RemoveEntity( GetIClientUnknown()->GetRefEHandle() );

	DestroyModelInstance();
}

void C_PortalGhostRenderable::PerFrameUpdate( void )
{
	if ( !m_pGhostedRenderable ) return;
    C_BaseCombatWeapon *weapon = dynamic_cast<C_BaseCombatWeapon *>( m_pGhostedRenderable );
    if ( weapon ) weapon->VerifyAndSetContextSensitiveWeaponModel();
	if( m_pGhostedRenderable )
	{
		SetModelName( m_pGhostedRenderable->GetModelName() );
		SetModelIndex( m_pGhostedRenderable->GetModelIndex() );
		SetEffects( m_pGhostedRenderable->GetEffects() | EF_NOINTERP );		
		m_flAnimTime = m_pGhostedRenderable->m_flAnimTime;		

		if( m_bSourceIsBaseAnimating )
		{
			C_BaseAnimating *pSource = (C_BaseAnimating *)m_pGhostedRenderable;
			SetCycle( pSource->GetCycle() );
			SetSequence( pSource->GetSequence() );
			m_nBody = pSource->m_nBody;
			m_nSkin = pSource->m_nSkin;
		}
	}


	// Set position and angles relative to the object it's ghosting
	Vector ptNewOrigin = m_matGhostTransform * m_pGhostedRenderable->GetAbsOrigin();		
	QAngle qNewAngles = TransformAnglesToWorldSpace( m_pGhostedRenderable->GetAbsAngles(), m_matGhostTransform.As3x4() );

	SetAbsOrigin( ptNewOrigin );
	SetAbsAngles( qNewAngles );

	AddEffects( EF_NOINTERP );

	RemoveFromEntityList(ENTITY_LIST_INTERPOLATE);

	g_pClientLeafSystem->RenderableChanged( RenderHandle() );
}

Vector const& C_PortalGhostRenderable::GetRenderOrigin( void )
{
	if( m_pGhostedRenderable == NULL )
		return m_ReferencedReturns.vRenderOrigin;

	m_ReferencedReturns.vRenderOrigin = m_matGhostTransform * m_pGhostedRenderable->GetRenderOrigin();
	return m_ReferencedReturns.vRenderOrigin;
}

QAngle const& C_PortalGhostRenderable::GetRenderAngles( void )
{
	if( m_pGhostedRenderable == NULL )
		return m_ReferencedReturns.qRenderAngle;

	m_ReferencedReturns.qRenderAngle = TransformAnglesToWorldSpace( m_pGhostedRenderable->GetRenderAngles(), m_matGhostTransform.As3x4() );
	return m_ReferencedReturns.qRenderAngle;
}

bool C_PortalGhostRenderable::SetupBones( matrix3x4a_t *out, int capacity, int boneMask, float time )
{
    C_BaseAnimating *source = dynamic_cast<C_BaseAnimating *>( m_pGhostedRenderable );
    CStudioHdr *sourceHdr = source ? source->GetModelPtr() : NULL;
    CStudioHdr *ghostHdr = GetModelPtr();
    if ( !sourceHdr || !ghostHdr || source->GetModel() != GetModel() ||
         sourceHdr->numbones() != ghostHdr->numbones() )
        return false;
    const int count = sourceHdr->numbones();
    if ( count <= 0 || count > MAXSTUDIOBONES || ( out && capacity < count ) )
        return false;

    // Always build into a complete private array. The caller's capacity belongs
    // to the ghost's model, not to a source weapon that may just have changed
    // models. Never leave the tail of the draw array uninitialized.
    matrix3x4a_t sourceBones[MAXSTUDIOBONES];
    if ( !source->SetupBones( sourceBones, MAXSTUDIOBONES, BONE_USED_BY_ANYTHING, time ) )
        return false;
    m_BoneAccessor.SetWritableBones( BONE_USED_BY_ANYTHING );
    matrix3x4a_t *ghostBones = m_BoneAccessor.GetBoneArrayForWrite();
    for ( int i = 0; i < count; ++i )
    {
        ConcatTransforms( m_matGhostTransform.As3x4(), sourceBones[i], ghostBones[i] );
        if ( out ) out[i] = ghostBones[i];
    }
    m_BoneAccessor.SetReadableBones( BONE_USED_BY_ANYTHING );
    return true;
}

void C_PortalGhostRenderable::GetRenderBounds( Vector& mins, Vector& maxs )
{
	if( m_pGhostedRenderable == NULL )
	{
		mins = maxs = vec3_origin;
		return;
	}

	m_pGhostedRenderable->GetRenderBounds( mins, maxs );
}

void C_PortalGhostRenderable::GetRenderBoundsWorldspace( Vector& mins, Vector& maxs )
{
	if( m_pGhostedRenderable == NULL )
	{
		mins = maxs = vec3_origin;
		return;
	}

	m_pGhostedRenderable->GetRenderBoundsWorldspace( mins, maxs );
	TransformAABB( m_matGhostTransform.As3x4(), mins, maxs, mins, maxs );
}

void C_PortalGhostRenderable::GetShadowRenderBounds( Vector &mins, Vector &maxs, ShadowType_t shadowType )
{
	m_pGhostedRenderable->GetShadowRenderBounds( mins, maxs, shadowType );
	TransformAABB( m_matGhostTransform.As3x4(), mins, maxs, mins, maxs );
}

/*bool C_PortalGhostRenderable::GetShadowCastDistance( float *pDist, ShadowType_t shadowType ) const
{
	if( m_pGhostedRenderable == NULL )
		return false;

	return m_pGhostedRenderable->GetShadowCastDistance( pDist, shadowType );
}

bool C_PortalGhostRenderable::GetShadowCastDirection( Vector *pDirection, ShadowType_t shadowType ) const
{
	if( m_pGhostedRenderable == NULL )
		return false;

	if( m_pGhostedRenderable->GetShadowCastDirection( pDirection, shadowType ) )
	{
		if( pDirection )
			*pDirection = m_matGhostTransform.ApplyRotation( *pDirection );

		return true;
	}
	return false;
}*/

const matrix3x4_t & C_PortalGhostRenderable::RenderableToWorldTransform()
{
	if( m_pGhostedRenderable == NULL )
		return m_ReferencedReturns.matRenderableToWorldTransform;

	ConcatTransforms( m_matGhostTransform.As3x4(), m_pGhostedRenderable->RenderableToWorldTransform(), m_ReferencedReturns.matRenderableToWorldTransform );
	return m_ReferencedReturns.matRenderableToWorldTransform;
}

bool C_PortalGhostRenderable::GetAttachment( int number, Vector &origin, QAngle &angles )
{
	if( m_pGhostedRenderable == NULL )
		return false;

	if( m_pGhostedRenderable->GetAttachment( number, origin, angles ) )
	{
		origin = m_matGhostTransform * origin;
		angles = TransformAnglesToWorldSpace( angles, m_matGhostTransform.As3x4() );
		return true;
	}
	return false;
}

bool C_PortalGhostRenderable::GetAttachment( int number, matrix3x4_t &matrix )
{
	if( m_pGhostedRenderable == NULL )
		return false;

	if( m_pGhostedRenderable->GetAttachment( number, matrix ) )
	{
		ConcatTransforms( m_matGhostTransform.As3x4(), matrix, matrix );
		return true;
	}
	return false;
}

bool C_PortalGhostRenderable::GetAttachment( int number, Vector &origin )
{
	if( m_pGhostedRenderable == NULL )
		return false;

	if( m_pGhostedRenderable->GetAttachment( number, origin ) )
	{
		origin = m_matGhostTransform * origin;
		return true;
	}
	return false;
}

bool C_PortalGhostRenderable::GetAttachmentVelocity( int number, Vector &originVel, Quaternion &angleVel )
{
	if( m_pGhostedRenderable == NULL )
		return false;

	Vector ghostVel;
	if( m_pGhostedRenderable->GetAttachmentVelocity( number, ghostVel, angleVel ) )
	{
		Vector3DMultiply( m_matGhostTransform, ghostVel, originVel );
		Vector3DMultiply( m_matGhostTransform, *(Vector*)( &angleVel ), *(Vector*)( &angleVel ) );
		return true;
	}
	return false;
}


int C_PortalGhostRenderable::DrawModel( int flags, const RenderableInstance_t &instance )
{
	if ( !m_pGhostedRenderable )
		return 0;

    // Model selection can change after Simulate but before a recursive draw.
    // Synchronize before the renderer allocates its bone array from this model.
    PerFrameUpdate();

#ifdef ANDROID
	// Match C_Portal_Player::DrawModel: the phone build hides the local body.
	// Ghosts render through C_BaseAnimating and otherwise bypass that rule.
	if ( m_bLocalPlayer && m_pGhostedRenderable->IsPlayer() )
		return 0;
#endif

	if( m_bSourceIsBaseAnimating )
	{
		if( m_bLocalPlayer )
		{
			C_Portal_Player *pPlayer = C_Portal_Player::GetLocalPlayer();

			if ( !pPlayer->IsAlive() )
			{
				// Dead player uses a ragdoll to draw, so don't ghost the dead entity
				return 0;
			}
			else if( g_pPortalRender->GetViewRecursionLevel() == 0 )
			{
				if( pPlayer->m_bEyePositionIsTransformedByPortal )
					return 0;
			}
			else if( g_pPortalRender->GetViewRecursionLevel() == 1 )
			{
				if( !pPlayer->m_bEyePositionIsTransformedByPortal )
					return 0;
			}
		}

		return C_BaseAnimating::DrawModel( flags, instance );
	}
	else
	{
		render->DrawBrushModel( m_pGhostedRenderable, 
								(model_t *)m_pGhostedRenderable->GetModel(), 
								GetRenderOrigin(), 
								GetRenderAngles(), 
								flags & STUDIO_TRANSPARENCY ? true : false );
		
		return 1;
	}

	return 0;
}

ModelInstanceHandle_t C_PortalGhostRenderable::GetModelInstance()
{
    // Native instances cache model/lighting state. A transformed copy must not
    // share that mutable state with its source or another portal's copy.
    return BaseClass::GetModelInstance();
}

CON_COMMAND_F( fstop_ghost_state, "Report the models and skeletons of portal render copies.", FCVAR_CHEAT )
{
    for ( int i = 0; i < s_PortalGhosts.Count(); ++i )
    {
        C_PortalGhostRenderable *ghost = s_PortalGhosts[i];
        C_BaseEntity *source = ghost->m_pGhostedRenderable;
        if ( !source ) continue;
        C_BaseAnimating *anim = dynamic_cast<C_BaseAnimating *>( source );
        CStudioHdr *sourceHdr = anim ? anim->GetModelPtr() : NULL;
        CStudioHdr *ghostHdr = ghost->GetModelPtr();
        Msg( "FSTOP_GHOST source=%s ghost=%s sourceBones=%d ghostBones=%d sameModel=%d separateInstance=%d\n",
             modelinfo->GetModelName( source->GetModel() ), modelinfo->GetModelName( ghost->GetModel() ),
             sourceHdr ? sourceHdr->numbones() : 0, ghostHdr ? ghostHdr->numbones() : 0,
             source->GetModel() == ghost->GetModel(), source->GetModelInstance() != ghost->GetModelInstance() );
    }
}


RenderableTranslucencyType_t C_PortalGhostRenderable::ComputeTranslucencyType( void ) 
{ 
	if ( m_pGhostedRenderable == NULL )
		return RENDERABLE_IS_OPAQUE;

	return m_pGhostedRenderable->ComputeTranslucencyType();
}

// (UsesPowerOfTwoFrameBufferTexture is not part of IClientRenderable on this engine)

/*const model_t* C_PortalGhostRenderable::GetModel( ) const
{
	if( m_pGhostedRenderable == NULL )
		return NULL;

	return m_pGhostedRenderable->GetModel();
}

int C_PortalGhostRenderable::GetBody()
{
	if( m_pGhostedRenderable == NULL )
		return 0;

	return m_pGhostedRenderable->GetBody();
}*/

void C_PortalGhostRenderable::GetColorModulation( float* color )
{
	if( m_pGhostedRenderable == NULL )
		return;

	return m_pGhostedRenderable->GetColorModulation( color );
}

/*ShadowType_t C_PortalGhostRenderable::ShadowCastType()
{
	if( m_pGhostedRenderable == NULL )
		return SHADOWS_NONE;

	return m_pGhostedRenderable->ShadowCastType();
}*/

int C_PortalGhostRenderable::LookupAttachment( const char *pAttachmentName )
{
	if( m_pGhostedRenderable == NULL )
		return -1;


	return m_pGhostedRenderable->LookupAttachment( pAttachmentName );
}

/*
int C_PortalGhostRenderable::GetSkin()
{
	if( m_pGhostedRenderable == NULL )
		return -1;


	return m_pGhostedRenderable->GetSkin();
}
*/









