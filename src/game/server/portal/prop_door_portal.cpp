#include "cbase.h"
#include "prop_portal.h"
#include "tier0/memdbgon.h"
// Door-owned portals retain rectangular rendering, dimensions and traversal.
class CProp_DoorPortal : public CProp_Portal
{
public:
    DECLARE_CLASS( CProp_DoorPortal, CProp_Portal );
    DECLARE_SERVERCLASS();
    DECLARE_DATADESC();
};
LINK_ENTITY_TO_CLASS( prop_portal_door, CProp_DoorPortal );
IMPLEMENT_SERVERCLASS_ST( CProp_DoorPortal, DT_Prop_DoorPortal )
END_SEND_TABLE()
BEGIN_DATADESC( CProp_DoorPortal )
END_DATADESC()
