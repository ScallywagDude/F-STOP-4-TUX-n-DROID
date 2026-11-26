//========= F-STOP port =======================================================//
//
// The maps ship seven small Squirrel scripts (scripts/vscripts/button_door/*.nut)
// that build the button and door puzzles at map start. This engine branch has no
// script VM, so this file does the same work natively:
//
//   maker.nut        spawns a button template at every "...buttonNN" info_target and
//                    moves that template's sign to the matching "...signNN" info_target
//   sign.nut         finds the portal door near the sign and tells it about presses
//   door.nut         opens once every button on it is held down, closes otherwise
//   *_target.nut     mark info_targets as button or sign positions
//
//=============================================================================//

#include "cbase.h"
#include "eventqueue.h"
#include "fstop_vscript.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static const float FSTOP_SIGN_FIND_RADIUS = 1000.0f;	// maker.nut
static const float FSTOP_DOOR_FIND_RADIUS = 200.0f;		// sign.nut FIND_DOOR_RANGE
static const float FSTOP_SIGN_TARGET_RADIUS = 8.0f;		// sign.nut

//-----------------------------------------------------------------------------
// Which script a "vscripts" keyvalue attaches
//-----------------------------------------------------------------------------
void FStopScript_ParseKeyValue( CBaseEntity *pEntity, const char *pszScripts )
{
	if ( V_stristr( pszScripts, "button_door/maker" ) )
		pEntity->m_nFStopScript = FSTOP_SCRIPT_MAKER;
	else if ( V_stristr( pszScripts, "button_door/sign_target" ) )
		pEntity->m_nFStopScript = FSTOP_SCRIPT_SIGN_TARGET;
	else if ( V_stristr( pszScripts, "button_door/sign" ) )
		pEntity->m_nFStopScript = FSTOP_SCRIPT_SIGN;
	else if ( V_stristr( pszScripts, "button_door/door" ) )
		pEntity->m_nFStopScript = FSTOP_SCRIPT_DOOR;
	else if ( V_stristr( pszScripts, "button_door/button_target_small" ) )
		pEntity->m_nFStopScript = FSTOP_SCRIPT_BUTTON_SMALL;
	else if ( V_stristr( pszScripts, "button_door/button_target_large" ) )
		pEntity->m_nFStopScript = FSTOP_SCRIPT_BUTTON_LARGE;
	else if ( V_stristr( pszScripts, "button_door/button_target" ) )
		pEntity->m_nFStopScript = FSTOP_SCRIPT_BUTTON_NORMAL;
}

//-----------------------------------------------------------------------------
// sign.nut OnPostSpawn: runs once the sign is at its final position
//-----------------------------------------------------------------------------
static void BindSignToDoor( CBaseEntity *pSign )
{
	CBaseEntity *pDoor = NULL;
	CBaseEntity *pCur = NULL;
	while ( ( pCur = gEntList.FindEntityByClassnameWithin( pCur, "func_door", pSign->GetAbsOrigin(), FSTOP_DOOR_FIND_RADIUS ) ) != NULL )
	{
		if ( pCur->m_nFStopScript == FSTOP_SCRIPT_DOOR )
			pDoor = pCur;
	}

	if ( pDoor == NULL )
	{
		DevMsg( "F-STOP script: sign %s has no door nearby\n", STRING( pSign->GetEntityName() ) );
		return;
	}

	pSign->m_hFStopDoor = pDoor;
	pDoor->m_nFStopDoorTotal++;
	DevMsg( "F-STOP script: sign %s -> door %s (%d buttons)\n", STRING( pSign->GetEntityName() ), STRING( pDoor->GetEntityName() ), pDoor->m_nFStopDoorTotal );
}

//-----------------------------------------------------------------------------
// sign.nut OnUser1Output / OnUser2Output and door.nut CloseButton / OpenButton
//-----------------------------------------------------------------------------
void FStopScript_OnUser( CBaseEntity *pSign, int nUser )
{
	if ( pSign->m_nFStopScript != FSTOP_SCRIPT_SIGN )
		return;

	variant_t emptyVariant;

	CBaseEntity *pDoor = pSign->m_hFStopDoor.Get();
	if ( pDoor )
	{
		if ( nUser == 1 )
		{
			pDoor->m_nFStopDoorPressed++;
			if ( pDoor->m_nFStopDoorPressed == pDoor->m_nFStopDoorTotal )
				g_EventQueue.AddEvent( STRING( pDoor->GetEntityName() ), "Open", emptyVariant, 0.0f, pSign, pSign );
		}
		else
		{
			pDoor->m_nFStopDoorPressed--;
			g_EventQueue.AddEvent( STRING( pDoor->GetEntityName() ), "Close", emptyVariant, 0.0f, pSign, pSign );
		}
		DevMsg( "F-STOP script: door %s %d/%d\n", STRING( pDoor->GetEntityName() ), pDoor->m_nFStopDoorPressed, pDoor->m_nFStopDoorTotal );
	}

	// the info_target the sign was moved to hears about it too
	CBaseEntity *pCur = NULL;
	while ( ( pCur = gEntList.FindEntityByClassnameWithin( pCur, "info_target", pSign->GetAbsOrigin(), FSTOP_SIGN_TARGET_RADIUS ) ) != NULL )
	{
		if ( pCur->m_nFStopScript == FSTOP_SCRIPT_SIGN_TARGET )
			g_EventQueue.AddEvent( STRING( pCur->GetEntityName() ), nUser == 1 ? "FireUser1" : "FireUser2", emptyVariant, 0.0f, pSign, pSign );
	}
}

//-----------------------------------------------------------------------------
// logic_script: the entity maker.nut is attached to
//-----------------------------------------------------------------------------
class CLogicScript : public CLogicalEntity
{
public:
	DECLARE_CLASS( CLogicScript, CLogicalEntity );
	DECLARE_DATADESC();

	CLogicScript() : m_bRan( false ) {}

	virtual void Activate( void );
	void RunMaker( void );

private:
	void SpawnButton( CBaseEntity *pTarget );

	enum { MAX_GROUPS = 3 };	// maker.nut's small, normal and large
	string_t	m_iszGroup[MAX_GROUPS];
	bool		m_bRan;
};

LINK_ENTITY_TO_CLASS( logic_script, CLogicScript );

BEGIN_DATADESC( CLogicScript )
	DEFINE_KEYFIELD( m_iszGroup[0], FIELD_STRING, "Group00" ),
	DEFINE_KEYFIELD( m_iszGroup[1], FIELD_STRING, "Group01" ),
	DEFINE_KEYFIELD( m_iszGroup[2], FIELD_STRING, "Group02" ),
	DEFINE_FIELD( m_bRan, FIELD_BOOLEAN ),
	DEFINE_THINKFUNC( RunMaker ),
END_DATADESC()

void CLogicScript::Activate( void )
{
	BaseClass::Activate();

	// OnPostSpawn: wait until every entity in the map exists
	if ( !m_bRan && m_nFStopScript == FSTOP_SCRIPT_MAKER )
	{
		SetThink( &CLogicScript::RunMaker );
		SetNextThink( gpGlobals->curtime );
	}
}

void CLogicScript::RunMaker( void )
{
	SetThink( NULL );
	if ( m_bRan )
		return;
	m_bRan = true;

	// spawning adds entities to the list we are walking, so collect the targets first
	CUtlVector<EHANDLE> targets;
	CBaseEntity *pCur = NULL;
	while ( ( pCur = gEntList.FindEntityByClassname( pCur, "info_target" ) ) != NULL )
	{
		const int nScript = pCur->m_nFStopScript;
		if ( ( nScript == FSTOP_SCRIPT_BUTTON_SMALL || nScript == FSTOP_SCRIPT_BUTTON_NORMAL || nScript == FSTOP_SCRIPT_BUTTON_LARGE )
			&& strstr( STRING( pCur->GetEntityName() ), "button" ) )
		{
			EHANDLE hTarget;
			hTarget = pCur;
			targets.AddToTail( hTarget );
		}
	}

	for ( int i = 0; i < targets.Count(); i++ )
	{
		if ( targets[i].Get() )
			SpawnButton( targets[i].Get() );
	}

	DevMsg( "F-STOP script: maker placed %d buttons\n", targets.Count() );
}

void CLogicScript::SpawnButton( CBaseEntity *pTarget )
{
	const char *pszName = STRING( pTarget->GetEntityName() );
	const char *pszButton = strstr( pszName, "button" );

	char szSystem[128];
	V_strncpy( szSystem, pszName, MIN( (int)sizeof( szSystem ), (int)( pszButton - pszName ) + 1 ) );
	const char *pszNumber = pszButton + 6;

	int nGroup = 1;
	if ( pTarget->m_nFStopScript == FSTOP_SCRIPT_BUTTON_SMALL )
		nGroup = 0;
	else if ( pTarget->m_nFStopScript == FSTOP_SCRIPT_BUTTON_LARGE )
		nGroup = 2;

	CBaseEntity *pMaker = gEntList.FindEntityByName( NULL, m_iszGroup[nGroup] );
	if ( pMaker == NULL )
	{
		Warning( "F-STOP script: no button template for %s\n", pszName );
		return;
	}

	variant_t value;
	value.SetString( pTarget->GetEntityName() );
	pMaker->AcceptInput( "ForceSpawnAtEntityOrigin", this, this, value, 0 );

	// the sign that just came with this button: the newest unplaced one nearby
	CBaseEntity *pSign = NULL;
	CBaseEntity *pCur = NULL;
	while ( ( pCur = gEntList.FindEntityByClassnameWithin( pCur, "func_brush", pTarget->GetAbsOrigin(), FSTOP_SIGN_FIND_RADIUS ) ) != NULL )
	{
		if ( pCur->m_nFStopScript == FSTOP_SCRIPT_SIGN && !pCur->m_bFStopSignPlaced )
			pSign = pCur;
	}

	if ( pSign == NULL )
	{
		Warning( "F-STOP script: no sign spawned for %s\n", pszName );
		return;
	}

	pSign->m_bFStopSignPlaced = true;

	char szSignName[256];
	V_snprintf( szSignName, sizeof( szSignName ), "%ssign%s", szSystem, pszNumber );

	CBaseEntity *pDest = gEntList.FindEntityByName( NULL, szSignName );
	if ( pDest )
	{
		Vector vecForward;
		pDest->GetVectors( &vecForward, NULL, NULL );
		QAngle angSign;
		VectorAngles( vecForward, angSign );
		Vector vecDest = pDest->GetAbsOrigin();
		pSign->Teleport( &vecDest, &angSign, NULL );
	}
	else
	{
		Warning( "F-STOP script: no sign position %s\n", szSignName );
	}

	BindSignToDoor( pSign );
}
