//========= Copyright © 1996-2004, Valve LLC, All rights reserved. ============
//
//=============================================================================

#include "cbase.h"
#include "KeyValues.h"
#include "tf_playerclass_shared.h"
#include "pf_cvars.h"
#include "materialsystem/imaterialsystemhardwareconfig.h"
#include "tier2/tier2.h"
#include "tf_gamerules.h"
#ifdef USE_INVENTORY
#include "tf_inventory.h"
#endif

#ifdef CLIENT_DLL
bool UseHWMorphModels();
#endif


#define TF_CLASS_UNDEFINED_FILE			""
#define TF_CLASS_SCOUT_FILE				"scripts/playerclasses/scout"
#define TF_CLASS_SNIPER_FILE			"scripts/playerclasses/sniper"
#define TF_CLASS_SOLDIER_FILE			"scripts/playerclasses/soldier"
#define TF_CLASS_DEMOMAN_FILE			"scripts/playerclasses/demoman"
#define TF_CLASS_MEDIC_FILE				"scripts/playerclasses/medic"
#define TF_CLASS_HEAVYWEAPONS_FILE		"scripts/playerclasses/heavyweapons"
#define TF_CLASS_PYRO_FILE				"scripts/playerclasses/pyro"
#define TF_CLASS_SPY_FILE				"scripts/playerclasses/spy"
#define TF_CLASS_ENGINEER_FILE			"scripts/playerclasses/engineer"
#define TF_CLASS_CIVILIAN_FILE			"scripts/playerclasses/civilian"

const char *s_aPlayerClassFiles[] =
{
	TF_CLASS_UNDEFINED_FILE,
	TF_CLASS_SCOUT_FILE,
	TF_CLASS_SNIPER_FILE,
	TF_CLASS_SOLDIER_FILE,
	TF_CLASS_DEMOMAN_FILE,
	TF_CLASS_MEDIC_FILE,
	TF_CLASS_HEAVYWEAPONS_FILE,
	TF_CLASS_PYRO_FILE,
	TF_CLASS_SPY_FILE,
	TF_CLASS_ENGINEER_FILE,
	TF_CLASS_CIVILIAN_FILE
};

static const char *s_PlayerModelPostfix[] =
{	
	"",
	"t1",
	"t2",
	"ir",
	"", // END Marker
};

#ifdef CLIENT_DLL
/*ConVar pf_playermodel_scout( "pf_playermodel_scout", "0", FCVAR_ARCHIVE, "Changes class model.", true, 0, true, 3 );
ConVar pf_playermodel_sniper( "pf_playermodel_sniper", "0", FCVAR_ARCHIVE, "Changes class model.", true, 0, true, 3 );
ConVar pf_playermodel_soldier( "pf_playermodel_soldier", "0", FCVAR_ARCHIVE, "Changes class model.", true, 0, true, 3 );
ConVar pf_playermodel_demoman( "pf_playermodel_demoman", "0", FCVAR_ARCHIVE, "Changes class model.", true, 0, true, 3 );
ConVar pf_playermodel_medic( "pf_playermodel_medic", "0", FCVAR_ARCHIVE, "Changes class model.", true, 0, true, 3 );
ConVar pf_playermodel_heavyweapons( "pf_playermodel_heavyweapons", "0", FCVAR_ARCHIVE, "Changes class model.", true, 0, true, 3 );
ConVar pf_playermodel_pyro( "pf_playermodel_pyro", "0", FCVAR_ARCHIVE, "Changes class model.", true, 0, true, 3 );
ConVar pf_playermodel_spy( "pf_playermodel_spy", "0", FCVAR_ARCHIVE, "Changes class model.", true, 0, true, 3 );
ConVar pf_playermodel_engineer( "pf_playermodel_engineer", "0", FCVAR_ARCHIVE, "Changes class model.", true, 0, true, 3 );
*/
//ConVar pf_playermodel_era("pf_playermodel_era", "0", FCVAR_ARCHIVE, "Changes class models used", true, 0, true, 3);

#endif

TFPlayerClassData_t s_aTFPlayerClassData[TF_CLASS_COUNT_ALL];

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
TFPlayerClassData_t::TFPlayerClassData_t()
{
	m_szClassName[0] = '\0';
	m_szModelName[0] = '\0';
	m_szHWMModelName[0] = '\0';
	m_szArmModelName[0] = '\0';
	m_szLocalizableName[0] = '\0';
	m_flMaxSpeed = 0.0f;
	m_flFlagSpeed = 0.0f;
	m_nMaxHealth = 0;
	m_nMaxArmor = 0;
	m_nArmorType = ARMOR_NONE;

#ifdef GAME_DLL
	m_szDeathSound[0] = '\0';
	m_szCritDeathSound[0] = '\0';
	m_szMeleeDeathSound[0] = '\0';
	m_szExplosionDeathSound[0] = '\0';
#endif

	for ( int iWeapon = 0; iWeapon < TF_PLAYER_WEAPON_COUNT; ++iWeapon )
	{
		m_aWeapons[iWeapon] = TF_WEAPON_NONE;
	}

	for ( int iGrenade = 0; iGrenade < TF_PLAYER_GRENADE_COUNT; ++iGrenade )
	{
		m_aGrenades[iGrenade] = TF_WEAPON_NONE;
	}

	for ( int iAmmo = 0; iAmmo < TF_AMMO_COUNT; ++iAmmo )
	{
		m_aAmmoMax[iAmmo] = TF_AMMO_DUMMY;
	}

	for ( int iBuildable = 0; iBuildable < TF_PLAYER_BUILDABLE_COUNT; ++iBuildable )
	{
		m_aBuildable[iBuildable] = OBJ_LAST;
	}

	//m_pBetaPlayerModelsKV = nullptr;

	m_bParsed = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TFPlayerClassData_t::Parse( const char *szName )
{
	// Have we parsed this file already?
	if ( m_bParsed )
		return;

	// No filesystem at this point????  Hmmmm......

	// Parse class file.

	const unsigned char *pKey = NULL;

	if ( g_pGameRules )
	{
		pKey = g_pGameRules->GetEncryptionKey();
	}

	KeyValues *pKV = ReadEncryptedKVFile( filesystem, szName, pKey );
	if ( pKV )
	{
		ParseData( pKV );
		pKV->deleteThis();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *TFPlayerClassData_t::GetModelName() const
{
#ifdef CLIENT_DLL
	/*if (m_pBetaPlayerModelsKV)
	{
		ConVar * pf_playermodel_era = nullptr;
		//convar_betamodels = g_pCVar->FindVar( VarArgs( "pf_playermodel_%s", m_szClassName ) );
		pf_playermodel_era = g_pCVar->FindVar( "pf_playermodel_era" );
		if ( pf_playermodel_era && pf_playermodel_era->GetInt() > 0)
		{
			for (KeyValues *kv = m_pBetaPlayerModelsKV->GetFirstSubKey(); kv; kv = kv->GetNextKey())
			{
				if (!Q_stricmp( kv->GetName(), s_PlayerModelPostfix[ pf_playermodel_era->GetInt() ] ))
				{
					const char *pszModelName = kv->GetString();
					if (pszModelName)
						return pszModelName;
				}
			}
		}
	}*/

	return m_szModelName;
#else
	return m_szModelName;
#endif
}

// WOW IT EVEN CALLS BACK PRETTY EPIC
#if defined (GAME_DLL)
void ClassFlagSpeed_ChangeCallback( IConVar *var, const char *pOldValue, float flOldValue )
{
	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

		if( pPlayer )
		{
			if( pPlayer->GetTeamNumber() > LAST_SHARED_TEAM )
			{
				pPlayer->TeamFortress_SetSpeed();
			}
		}
	}
}
#else
#define ClassFlagSpeed_ChangeCallback nullptr
#endif
#define LAZYCONVARDELETEME( name ) ConVar name(#name, "100", FCVAR_NOTIFY | FCVAR_REPLICATED | FCVAR_DONTRECORD | FCVAR_SERVER_CAN_EXECUTE, "", ClassFlagSpeed_ChangeCallback );
LAZYCONVARDELETEME( pf_flag_speed_scout );
LAZYCONVARDELETEME( pf_flag_speed_soldier );
LAZYCONVARDELETEME( pf_flag_speed_pyro );
LAZYCONVARDELETEME( pf_flag_speed_demoman );
LAZYCONVARDELETEME( pf_flag_speed_heavyweapons );
LAZYCONVARDELETEME( pf_flag_speed_engineer );
LAZYCONVARDELETEME( pf_flag_speed_medic );
LAZYCONVARDELETEME( pf_flag_speed_sniper );
LAZYCONVARDELETEME( pf_flag_speed_spy );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TFPlayerClassData_t::ParseData( KeyValues *pKeyValuesData )
{
	// Attributes.
	Q_strncpy( m_szClassName, pKeyValuesData->GetString( "name" ), TF_NAME_LENGTH );

	// Load the high res model or the lower res model.
	if( !IsX360() )
	{
		Q_strncpy( m_szHWMModelName, pKeyValuesData->GetString( "model_hwm" ), TF_NAME_LENGTH );
	}
	Q_strncpy( m_szModelName, pKeyValuesData->GetString( "model" ), TF_NAME_LENGTH );

	Q_strncpy( m_szArmModelName, pKeyValuesData->GetString( "model_hands" ), TF_NAME_LENGTH );

	/*for (int i = 1; i < ARRAYSIZE(s_PlayerModelPostfix); ++i )
	{
		char buf[ 128 ];
		V_snprintf( buf, sizeof( buf ), "model_%s", s_PlayerModelPostfix[i] );
		const char *pszModelName = pKeyValuesData->GetString( buf, nullptr );
		if (pszModelName != nullptr)
		{
			if (!m_pBetaPlayerModelsKV)
			{
				m_pBetaPlayerModelsKV = new KeyValues( "class_betamodels" );;
			}
			m_pBetaPlayerModelsKV->SetString( s_PlayerModelPostfix[i], pszModelName );
		}
	}*/

	Q_strncpy( m_szLocalizableName, pKeyValuesData->GetString( "localize_name" ), TF_NAME_LENGTH );

	m_flMaxSpeed = pKeyValuesData->GetFloat( "speed_max" );
	m_flFlagSpeed = pKeyValuesData->GetFloat( "flag_speed_max", m_flMaxSpeed );
#ifdef GAME_DLL
	 ConVar *flagspeed = g_pCVar->FindVar( UTIL_VarArgs( "pf_flag_speed_%s", m_szClassName ) );
	if( flagspeed )
	{
		flagspeed->SetValue( m_flFlagSpeed );
	}
#endif
	m_nMaxHealth = pKeyValuesData->GetInt( "health_max" );
	m_nMaxArmor = pKeyValuesData->GetInt( "armor_max" );
	m_nArmorType = pKeyValuesData->GetInt( "armortype" );

	// Weapons.
	int i;
	char buf[32];
	for( i = 0; i<TF_PLAYER_WEAPON_COUNT; i++ )
	{
		Q_snprintf( buf, sizeof( buf ), "weapon%d", i + 1 );
		m_aWeapons[i] = GetWeaponId( pKeyValuesData->GetString( buf ) );
	}

	// Grenades.
	m_aGrenades[0] = GetWeaponId( pKeyValuesData->GetString( "grenade1" ) );
	m_aGrenades[1] = GetWeaponId( pKeyValuesData->GetString( "grenade2" ) );

	// Ammo Max.
	KeyValues *pAmmoKeyValuesData = pKeyValuesData->FindKey( "AmmoMax" );
	if( pAmmoKeyValuesData )
	{
		for( int iAmmo = 1; iAmmo < TF_AMMO_COUNT; ++iAmmo )
		{
			m_aAmmoMax[iAmmo] = pAmmoKeyValuesData->GetInt( g_aAmmoNames[iAmmo], 0 );
		}
	}

	// Buildables
	for( i = 0; i<TF_PLAYER_BUILDABLE_COUNT; i++ )
	{
		Q_snprintf( buf, sizeof( buf ), "buildable%d", i + 1 );
		m_aBuildable[i] = GetBuildableId( pKeyValuesData->GetString( buf ) );
	}

	// Temp animation flags
	m_bDontDoAirwalk = (pKeyValuesData->GetInt( "DontDoAirwalk", 0 ) > 0);
	m_bDontDoNewJump = (pKeyValuesData->GetInt( "DontDoNewJump", 0 ) > 0);

#ifdef GAME_DLL		// right now we only emit these sounds from server. if that changes we can do this in both dlls

	// Death Sounds
	Q_strncpy( m_szDeathSound, pKeyValuesData->GetString( "sound_death", "Player.Death" ), MAX_PLAYERCLASS_SOUND_LENGTH );
	Q_strncpy( m_szCritDeathSound, pKeyValuesData->GetString( "sound_crit_death", "TFPlayer.CritDeath" ), MAX_PLAYERCLASS_SOUND_LENGTH );
	Q_strncpy( m_szMeleeDeathSound, pKeyValuesData->GetString( "sound_melee_death", "Player.MeleeDeath" ), MAX_PLAYERCLASS_SOUND_LENGTH );
	Q_strncpy( m_szExplosionDeathSound, pKeyValuesData->GetString( "sound_explosion_death", "Player.ExplosionDeath" ), MAX_PLAYERCLASS_SOUND_LENGTH );

#endif

	// The file has been parsed.
	m_bParsed = true;
}

//-----------------------------------------------------------------------------
// Purpose: Initialize the player class data (keep a cache).
//-----------------------------------------------------------------------------
void InitPlayerClasses( void )
{
	// Special case the undefined class.
	TFPlayerClassData_t *pClassData = &s_aTFPlayerClassData[TF_CLASS_UNDEFINED];
	Assert( pClassData );
	Q_strncpy( pClassData->m_szClassName, "undefined", TF_NAME_LENGTH );
	Q_strncpy( pClassData->m_szModelName, "models/player/scout.mdl", TF_NAME_LENGTH );	// Undefined players still need a model
	Q_strncpy( pClassData->m_szArmModelName, "models/weapons/c_models/c_scout_arms.mdl", TF_NAME_LENGTH );
	Q_strncpy( pClassData->m_szLocalizableName, "undefined", TF_NAME_LENGTH );

	// Initialize the classes.
	for ( int iClass = 1; iClass < TF_CLASS_COUNT_ALL; ++iClass )
	{
		TFPlayerClassData_t *pClassData = &s_aTFPlayerClassData[iClass];
		Assert( pClassData );
		pClassData->Parse( s_aPlayerClassFiles[iClass] );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Helper function to get player class data.
//-----------------------------------------------------------------------------
TFPlayerClassData_t *GetPlayerClassData( int iClass )
{
	Assert ( ( iClass >= 0 ) && ( iClass < TF_CLASS_COUNT_ALL ) );
	return &s_aTFPlayerClassData[iClass];
}

//=============================================================================
//
// Shared player class data.
//

//=============================================================================
//
// Tables.
//

// Client specific.
#ifdef CLIENT_DLL

BEGIN_RECV_TABLE_NOBASE( CTFPlayerClassShared, DT_TFPlayerClassShared )
	RecvPropInt( RECVINFO( m_iClass ) ),
END_RECV_TABLE()

// Server specific.
#else

BEGIN_SEND_TABLE_NOBASE( CTFPlayerClassShared, DT_TFPlayerClassShared )
	SendPropInt( SENDINFO( m_iClass ), Q_log2( TF_CLASS_COUNT_ALL )+1, SPROP_UNSIGNED ),
END_SEND_TABLE()

#endif


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFPlayerClassShared::CTFPlayerClassShared()
{
	m_iClass.Set( TF_CLASS_UNDEFINED );
}

//-----------------------------------------------------------------------------
// Purpose: Initialize the player class.
//-----------------------------------------------------------------------------
bool CTFPlayerClassShared::Init( int iClass )
{
	Assert ( ( iClass >= TF_FIRST_NORMAL_CLASS ) && ( iClass <= TF_LAST_NORMAL_CLASS ) );
	m_iClass = iClass;
	return true;
}

// If needed, put this into playerclass scripts
bool CTFPlayerClassShared::CanBuildObject( int iObjectType )
{
	bool bFound = false;

	TFPlayerClassData_t  *pData = GetData();

	int i;
	for ( i=0;i<TF_PLAYER_BUILDABLE_COUNT;i++ )
	{
		if ( iObjectType == pData->m_aBuildable[i] )
		{
			bFound = true;
			break;
		}
	}

	return bFound;
}


#ifdef CLIENT_DLL
#define SHARED_VarArgs VarArgs
#else
#define SHARED_VarArgs UTIL_VarArgs
#endif

float CTFPlayerClassShared::GetMaxFlagSpeed( void )
{
	ConVar *flagspeed = g_pCVar->FindVar( SHARED_VarArgs( "pf_flag_speed_%s", GetPlayerClassData( m_iClass )->m_szClassName ) );
	if( flagspeed )
	{
		return flagspeed->GetFloat();
	}
	return GetPlayerClassData( m_iClass )->m_flFlagSpeed;
}