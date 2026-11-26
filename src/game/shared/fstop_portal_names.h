//=============================================================================
// F-STOP on the CS:GO partner engine: the CS:GO base's Portal hooks (#ifdef PORTAL) were written
// for Portal 2's portal class. F-STOP's portals are the Portal 1 style CProp_Portal, so those names
// map onto it here. Included at the top of the client and server cbase.h.
//=============================================================================
#ifndef FSTOP_PORTAL_NAMES_H
#define FSTOP_PORTAL_NAMES_H
#ifdef FSTOP_PORTAL

#ifdef CLIENT_DLL
#define C_Portal_Base2D C_Prop_Portal
#define CPortal_Base2D C_Prop_Portal
#else
#define CPortal_Base2D CProp_Portal
#endif
#define CPortal_Base2D_Shared CProp_Portal_Shared

// Portal 2 told entities when a portal they were near moved, fizzled or linked.
enum PortalEvent_t
{
	PORTALEVENT_LINKED,
	PORTALEVENT_FIZZLE,
	PORTALEVENT_MOVED,
	PORTALEVENT_ENTITY_TELEPORTED_TO,
	PORTALEVENT_ENTITY_TELEPORTED_FROM,
	PORTALEVENT_PLAYER_TELEPORTED_TO,
	PORTALEVENT_PLAYER_TELEPORTED_FROM,

	PORTALEVENT_COUNT,
};

#endif // FSTOP_PORTAL
#endif // FSTOP_PORTAL_NAMES_H
