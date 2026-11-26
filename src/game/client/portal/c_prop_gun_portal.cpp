#include "cbase.h"
#include "c_prop_portal.h"
#include "gun_portal_shared.h"
#include "Portal_DynamicMeshRenderingUtils.h"
#include "materialsystem/imaterialvar.h"
#include "view.h"
#include "view_scene.h"
#include "tier0/memdbgon.h"
class C_Prop_GunPortal : public C_Prop_Portal
{
public:
    DECLARE_CLASS( C_Prop_GunPortal, C_Prop_Portal );
    DECLARE_CLIENTCLASS();
    virtual bool IsGunPortal() const { return true; }
    virtual bool IsPointInPortalAperture( const Vector &point ) const { return GunPortalContainsPoint( this, point ); }
    virtual void DrawSimplePortalMesh( const IMaterial *materialOverride = NULL, float forwardOffset = 0.25f );
    virtual void DrawComplexPortalMesh( const IMaterial *materialOverride = NULL, float forwardOffset = 0.25f );
    virtual void DrawRenderFixMesh( const IMaterial *materialOverride = NULL, float frontClip = 0.3f );
    virtual void DrawPreStencilMask();
    virtual void DrawDepthDoublerMesh( float forwardOffset = 0.25f );
private:
    const IMaterial *GunMaterial( const IMaterial *materialOverride, bool renderFix = false );
};
IMPLEMENT_CLIENTCLASS_DT( C_Prop_GunPortal, DT_Prop_GunPortal, CProp_GunPortal )
END_RECV_TABLE()
LINK_ENTITY_TO_CLASS( prop_portal_gun, C_Prop_GunPortal );
const IMaterial *C_Prop_GunPortal::GunMaterial( const IMaterial *materialOverride, bool renderFix )
{
    char name[MAX_PATH];
    if ( !materialOverride )
        V_snprintf( name, sizeof(name), "models/portals/gun/portal_%d_%s", m_bIsPortal2 ? 2 : 1, renderFix ? "renderfix_dynamicmesh" : "dynamicmesh" );
    else
    {
        const char *original = materialOverride->GetName();
        if ( V_strnicmp( original, "models/portals/", 15 ) || V_stristr( original, "/gun/" ) ) return materialOverride;
        V_snprintf( name, sizeof(name), "models/portals/gun/%s", original + 15 );
    }
    return materials->FindMaterial( name, TEXTURE_GROUP_CLIENT_EFFECTS, false );
}
void C_Prop_GunPortal::DrawSimplePortalMesh( const IMaterial *materialOverride, float forwardOffset )
{
    IMaterial *material = const_cast<IMaterial *>( GunMaterial( materialOverride ) );
    CMatRenderContextPtr context( materials );
    context->Bind( material, GetClientRenderable() );
    UpdateFrontBufferTexturesForMaterial( material );
    context->MatrixMode( MATERIAL_MODEL ); context->PushMatrix(); context->LoadIdentity();
    const int segments = 64;
    Vector center = m_ptOrigin + m_vForward * forwardOffset;
    float tangent[4] = { -m_vRight.x, -m_vRight.y, -m_vRight.z, 1.0f };
    IMesh *mesh = context->GetDynamicMesh( false ); CMeshBuilder builder;
    builder.Begin( mesh, MATERIAL_TRIANGLES, segments );
    for ( int i = 0; i < segments; ++i )
    {
        for ( int corner = 0; corner < 3; ++corner )
        {
            float x = 0, y = 0;
            if ( corner ) { float angle = (i + corner - 1) * (2.0f * M_PI / segments); x = cosf(angle); y = sinf(angle); }
            Vector point = center + m_vRight * (x * GetHalfWidth()) + m_vUp * (y * GetHalfHeight());
            builder.Position3fv( point.Base() );
            builder.TexCoord2f( 0, .5f - x*.5f, .5f - y*.5f );
            builder.TexCoord2f( 1, .5f - x*.5f, .5f - y*.5f );
            builder.Normal3fv( m_vForward.Base() ); builder.UserData( tangent ); builder.AdvanceVertex();
        }
    }
    builder.End(); mesh->Draw();
    context->MatrixMode( MATERIAL_MODEL ); context->PopMatrix();
}
void C_Prop_GunPortal::DrawComplexPortalMesh( const IMaterial *materialOverride, float forwardOffset )
{
    const int segments = 64; PortalMeshPoint_t points[segments];
    for ( int i = 0; i < segments; ++i )
    {
        float angle = i * (2.0f * M_PI / segments), x = cosf(angle), y = sinf(angle);
        points[i].vWorldSpacePosition = m_ptOrigin + m_vForward * forwardOffset + m_vRight * (x * GetHalfWidth()) + m_vUp * (y * GetHalfHeight());
        points[i].texCoord.Init( .5f - x*.5f, .5f - y*.5f );
    }
    Clip_And_Render_Convex_Polygon( points, segments, GunMaterial(materialOverride), GetClientRenderable() );
}
void C_Prop_GunPortal::DrawRenderFixMesh( const IMaterial *materialOverride, float frontClip )
{
    BaseClass::DrawRenderFixMesh( GunMaterial(materialOverride, true), frontClip );
}
void C_Prop_GunPortal::DrawPreStencilMask()
{
    if ( m_fOpenAmount > 0.0f && m_fOpenAmount < 1.0f )
        DrawSimplePortalMesh( m_Materials.m_Portal_Refract[m_bIsPortal2 ? 1 : 0] );
}
void C_Prop_GunPortal::DrawDepthDoublerMesh( float forwardOffset )
{
    IMaterial *material = const_cast<IMaterial *>( GunMaterial(m_Materials.m_PortalDepthDoubler) );
    IMaterialVar *matrix = material->FindVar( "$alternateviewmatrix", NULL, false );
    if ( matrix ) matrix->SetMatrixValue( m_InternallyMaintainedData.m_DepthDoublerTextureView );
    DrawSimplePortalMesh( material, forwardOffset );
}
