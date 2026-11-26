#include "cbase.h"
#include "c_prop_portal.h"
#include "tier0/memdbgon.h"
class C_Prop_DoorPortal : public C_Prop_Portal
{
public:
    DECLARE_CLASS( C_Prop_DoorPortal, C_Prop_Portal );
    DECLARE_CLIENTCLASS();
};
IMPLEMENT_CLIENTCLASS_DT( C_Prop_DoorPortal, DT_Prop_DoorPortal, CProp_DoorPortal )
END_RECV_TABLE()
LINK_ENTITY_TO_CLASS( prop_portal_door, C_Prop_DoorPortal );
