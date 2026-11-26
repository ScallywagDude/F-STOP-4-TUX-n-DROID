#include "cbase.h"
#include "prop_portal.h"
#include "gun_portal_shared.h"
#include "tier0/memdbgon.h"
class CProp_GunPortal : public CProp_Portal
{
public:
    DECLARE_CLASS( CProp_GunPortal, CProp_Portal );
    DECLARE_SERVERCLASS();
    DECLARE_DATADESC();
    virtual bool IsGunPortal() const { return true; }
    virtual bool IsPointInPortalAperture( const Vector &point ) const { return GunPortalContainsPoint( this, point ); }
    virtual void Spawn()
    {
        KeyValue( "HalfWidth", "32" ); KeyValue( "HalfHeight", "54" );
        BaseClass::Spawn();
    }
    virtual void Precache()
    {
        PrecacheModel( "models/portals/gun/portal1.mdl" );
        PrecacheModel( "models/portals/gun/portal2.mdl" );
        BaseClass::Precache();
    }
    virtual void ResetModel()
    {
        BaseClass::ResetModel();
        SetModel( m_bIsPortal2 ? "models/portals/gun/portal2.mdl" : "models/portals/gun/portal1.mdl" );
    }
};
LINK_ENTITY_TO_CLASS( prop_portal_gun, CProp_GunPortal );
IMPLEMENT_SERVERCLASS_ST( CProp_GunPortal, DT_Prop_GunPortal )
END_SEND_TABLE()
BEGIN_DATADESC( CProp_GunPortal )
END_DATADESC()
CProp_Portal *CProp_Portal::FindGunPortal( unsigned char group, bool portal2, bool create )
{
    const CUtlVector<CProp_Portal *> *portals = GetPortalLinkageGroup( group );
    CProp_Portal *inactive = NULL;
    for ( int i = 0; i < portals->Count(); ++i )
    {
        CProp_Portal *portal = (*portals)[i];
        if ( !portal->IsGunPortal() || portal->m_bIsPortal2 != portal2 ) continue;
        if ( portal->m_bActivated ) return portal;
        inactive = portal;
    }
    if ( inactive || !create ) return inactive;
    CProp_Portal *portal = static_cast<CProp_Portal *>( CreateEntityByName( "prop_portal_gun" ) );
    if ( !portal ) return NULL;
    portal->m_iLinkageGroupID = group;
    portal->m_bIsPortal2 = portal2;
    DispatchSpawn( portal );
    return portal;
}
