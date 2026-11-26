//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: F-STOP weapon data: the classic script-file FileWeaponInfo_t (scripts/weapon_*.txt) that the
//          HL2 and Portal weapons were written for. CS:GO replaced it with item schema data
//          (weapon_cache.h); weapon_parse.h includes this file instead when CSTRIKE15 is not defined.
//
//=============================================================================//
#ifndef WEAPON_PARSE_CLASSIC_H
#define WEAPON_PARSE_CLASSIC_H
#ifdef _WIN32
#pragma once
#endif

typedef unsigned short WEAPON_FILE_INFO_HANDLE;
class CEconItemView;

class CHudTexture;
class KeyValues;

//-----------------------------------------------------------------------------
// Purpose: Contains the data read from the weapon's script file. 
// It's cached so we only read each weapon's script file once.
// Each game provides a CreateWeaponInfo function so it can have game-specific
// data (like CS move speeds) in the weapon script.
//-----------------------------------------------------------------------------
class FileWeaponInfo_t
{
public:

	FileWeaponInfo_t();
	
	// Each game can override this to get whatever values it wants from the script.
	virtual ~FileWeaponInfo_t() {}
	virtual void Parse( KeyValues *pKeyValuesData, const char *szWeaponName );

	// The CS:GO weapon base asks for these through the item (CEconItemView) a weapon was made from.
	// F-STOP weapons come from their script files only, so the item is ignored.
	const char *GetWorldModel( const CEconItemView *, int = 0 ) const { return szWorldModel; }
	const char *GetViewModel( const CEconItemView *, int = 0 ) const { return szViewModel; }
	const char *GetWorldDroppedModel( const CEconItemView *, int = 0 ) const { return szWorldModel; }
	const char *GetShootSound( const CEconItemView *, int iSound ) const { return aShootSounds[iSound]; }
	const char *GetPrimaryAmmo( const CEconItemView * ) const { return szAmmo1; }
	const char *GetSecondaryAmmo( const CEconItemView * ) const { return szAmmo2; }
	const char *GetPrintName( const CEconItemView * ) const { return szPrintName; }
	const char *GetClassName( const CEconItemView * ) const { return szClassName; }
	bool AllowsFlipping( const CEconItemView * ) const { return m_bAllowFlipping; }
	bool ModelIsRightHanded( const CEconItemView * ) const { return m_bBuiltRightHanded; }
	bool HasFlag( const CEconItemView *, int nFlag ) const { return ( iFlags & nFlag ) != 0; }
	bool IsMeleeWeapon( const CEconItemView * ) const { return m_bMeleeWeapon; }
	int GetWeight( const CEconItemView * ) const { return iWeight; }
	bool AllowsAutoSwitchTo( const CEconItemView * ) const { return bAutoSwitchTo; }
	bool AllowsAutoSwitchFrom( const CEconItemView * ) const { return bAutoSwitchFrom; }
	int GetPrimaryAmmoType( const CEconItemView * ) const { return iAmmoType; }
	int GetSecondaryAmmoType( const CEconItemView * ) const { return iAmmo2Type; }
	int GetGearSlot( const CEconItemView * ) const { return iSlot; }
	int GetGearSlotPosition( const CEconItemView * ) const { return iPosition; }
	int GetLoadoutPosition( const CEconItemView *, int = -1 ) const { return -1; }
	int GetRumbleEffect( const CEconItemView * ) const { return iRumbleEffect; }
	virtual int GetPrimaryClipSize( const CEconItemView *, int = 0 ) const { return iMaxClip1; }
	virtual int GetSecondaryClipSize( const CEconItemView *, int = 0 ) const { return iMaxClip2; }
	virtual int GetDefaultPrimaryClipSize( const CEconItemView *, int = 0 ) const { return iDefaultClip1; }
	virtual int GetDefaultSecondaryClipSize( const CEconItemView *, int = 0 ) const { return iDefaultClip2; }
	virtual int GetPrimaryReserveAmmoMax( const CEconItemView *, int = 0 ) const { return 0; }
	virtual int GetSecondaryReserveAmmoMax( const CEconItemView *, int = 0 ) const { return 0; }

	
public:	
	bool					bParsedScript;
	bool					bLoadedHudElements;

// SHARED
	char					szClassName[MAX_WEAPON_STRING];
	char					szPrintName[MAX_WEAPON_STRING];			// Name for showing in HUD, etc.

	char					szViewModel[MAX_WEAPON_STRING];			// View model of this weapon
	char					szWorldModel[MAX_WEAPON_STRING];		// Model of this weapon seen carried by the player
	char					szAnimationPrefix[MAX_WEAPON_PREFIX];	// Prefix of the animations that should be used by the player carrying this weapon
	int						iSlot;									// inventory slot.
	int						iPosition;								// position in the inventory slot.
	int						iMaxClip1;								// max primary clip size (-1 if no clip)
	int						iMaxClip2;								// max secondary clip size (-1 if no clip)
	int						iDefaultClip1;							// amount of primary ammo in the gun when it's created
	int						iDefaultClip2;							// amount of secondary ammo in the gun when it's created
	int						iWeight;								// this value used to determine this weapon's importance in autoselection.
	int						iRumbleEffect;							// Which rumble effect to use when fired? (xbox)
	bool					bAutoSwitchTo;							// whether this weapon should be considered for autoswitching to
	bool					bAutoSwitchFrom;						// whether this weapon can be autoswitched away from when picking up another weapon or ammo
	int						iFlags;									// miscellaneous weapon flags
	char					szAmmo1[MAX_WEAPON_AMMO_NAME];			// "primary" ammo type
	char					szAmmo2[MAX_WEAPON_AMMO_NAME];			// "secondary" ammo type

	// Sound blocks
	char					aShootSounds[NUM_SHOOT_SOUND_TYPES][MAX_WEAPON_STRING];	

	int						iAmmoType;
	int						iAmmo2Type;
	bool					m_bMeleeWeapon;		// Melee weapons can always "fire" regardless of ammo.

	// This tells if the weapon was built right-handed (defaults to true).
	// This helps cl_righthand make the decision about whether to flip the model or not.
	bool					m_bBuiltRightHanded;
	bool					m_bAllowFlipping;	// False to disallow flipping the model, regardless of whether
												// it is built left or right handed.

// CLIENT DLL
	// Sprite data, read from the data file
	int						iSpriteCount;
	CHudTexture						*iconActive;
	CHudTexture	 					*iconInactive;
	CHudTexture 					*iconAmmo;
	CHudTexture 					*iconAmmo2;
	CHudTexture 					*iconCrosshair;
	CHudTexture 					*iconAutoaim;
	CHudTexture 					*iconZoomedCrosshair;
	CHudTexture 					*iconZoomedAutoaim;
	CHudTexture						*iconSmall;

// TF2 specific
	bool					bShowUsageHint;							// if true, then when you receive the weapon, show a hint about it

// SERVER DLL

};

// The weapon parse function
bool ReadWeaponDataFromFileForSlot( IFileSystem* filesystem, const char *szWeaponName, 
	WEAPON_FILE_INFO_HANDLE *phandle, const unsigned char *pICEKey = NULL );

// If weapon info has been loaded for the specified class name, this returns it.
WEAPON_FILE_INFO_HANDLE LookupWeaponInfoSlot( const char *name );

FileWeaponInfo_t *GetFileWeaponInfoFromHandle( WEAPON_FILE_INFO_HANDLE handle );
WEAPON_FILE_INFO_HANDLE GetInvalidWeaponInfoHandle( void );
void PrecacheFileWeaponInfoDatabase( IFileSystem *filesystem, const unsigned char *pICEKey );


// 
// Read a possibly-encrypted KeyValues file in. 
// If pICEKey is NULL, then it appends .txt to the filename and loads it as an unencrypted file.
// If pICEKey is non-NULL, then it appends .ctx to the filename and loads it as an encrypted file.
//
// (This should be moved into a more appropriate place).
//
KeyValues* ReadEncryptedKVFile( IFileSystem *filesystem, const char *szFilenameWithoutExtension, const unsigned char *pICEKey, bool bForceReadEncryptedFile = false );


// Each game implements this. It can return a derived class and override Parse() if it wants.
extern FileWeaponInfo_t* CreateWeaponInfo();



#endif // WEAPON_PARSE_CLASSIC_H
