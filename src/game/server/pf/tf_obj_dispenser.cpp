//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Engineer's Dispenser
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"

#include "tf_obj_dispenser.h"
#include "engine/IEngineSound.h"
#include "tf_player.h"
#include "tf_team.h"
#include "tf_gamerules.h"
#include "vguiscreen.h"
#include "world.h"
#include "explode.h"
#include "triggers.h"
#include "particle_parse.h"
#include "pf_cvars.h"
#include "hl2orange.spa.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Ground placed version
#define DISPENSER_MODEL_PLACEMENT			"models/buildables/dispenser_blueprint.mdl"

// Dispenser Model Levels
#define DISPENSER_MODEL_LEVEL_1				"models/buildables/dispenser_light.mdl"
#define DISPENSER_MODEL_LEVEL_1_UPGRADE		"models/buildables/dispenser.mdl"
#define DISPENSER_MODEL_LEVEL_2				"models/buildables/dispenser_lvl2_light.mdl"
#define DISPENSER_MODEL_LEVEL_2_UPGRADE		"models/buildables/dispenser_lvl2.mdl"
#define DISPENSER_MODEL_LEVEL_3				"models/buildables/dispenser_lvl3_light.mdl"
#define DISPENSER_MODEL_LEVEL_3_UPGRADE		"models/buildables/dispenser_lvl3.mdl"

#define DISPENSER_MINS			Vector( -20, -20, 0)
#define DISPENSER_MAXS			Vector( 20, 20, 55)	// tweak me

#define DISPENSER_TRIGGER_MINS			Vector(-70, -70, 0)
#define DISPENSER_TRIGGER_MAXS			Vector( 70,  70, 50)	// tweak me

#define REFILL_CONTEXT			"RefillContext"
#define DISPENSE_CONTEXT		"DispenseContext"

#define DISPENSER_MAX_HEALTH 150

#define DISPENSER_DROP_METAL 40

ConVar obj_dispenser_heal_rate( "obj_dispenser_heal_rate", "10.0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
extern ConVar tf_cheapobjects;

//-----------------------------------------------------------------------------
// Purpose: SendProxy that converts the Healing list UtlVector to entindices
//-----------------------------------------------------------------------------
void SendProxy_HealingList( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID )
{
	CObjectDispenser *pDispenser = (CObjectDispenser*)pStruct;

	// If this assertion fails, then SendProxyArrayLength_HealingArray must have failed.
	Assert( iElement < pDispenser->m_hHealingTargets.Size() );

	CBaseEntity *pEnt = pDispenser->m_hHealingTargets[iElement].Get();
	EHANDLE hOther = pEnt;

	SendProxy_EHandleToInt( pProp, pStruct, &hOther, pOut, iElement, objectID );
}

int SendProxyArrayLength_HealingArray( const void *pStruct, int objectID )
{
	CObjectDispenser *pDispenser = (CObjectDispenser*)pStruct;
	return pDispenser->m_hHealingTargets.Count();
}

IMPLEMENT_SERVERCLASS_ST( CObjectDispenser, DT_ObjectDispenser )
	SendPropInt( SENDINFO( m_iAmmoMetal ), 10 ),

	SendPropArray2( 
		SendProxyArrayLength_HealingArray,
		SendPropInt("healing_array_element", 0, SIZEOF_IGNORE, NUM_NETWORKED_EHANDLE_BITS, SPROP_UNSIGNED, SendProxy_HealingList), 
		MAX_PLAYERS, 
		0, 
		"healing_array"
		)

END_SEND_TABLE()

BEGIN_DATADESC( CObjectDispenser )
	DEFINE_KEYFIELD( m_szTriggerName, FIELD_STRING, "touch_trigger" ),
	DEFINE_THINKFUNC( RefillThink ),
	DEFINE_THINKFUNC( DispenseThink ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( obj_dispenser, CObjectDispenser );
PRECACHE_REGISTER( obj_dispenser );


#define DISPENSER_MAX_HEALTH	150

// How much of each ammo gets added per refill
#define DISPENSER_REFILL_METAL_AMMO			40

// How much ammo is given our per use
#define DISPENSER_DROP_PRIMARY		40
#define DISPENSER_DROP_SECONDARY	40
#define DISPENSER_DROP_METAL		40

class CDispenserTouchTrigger : public CBaseTrigger
{
	DECLARE_CLASS( CDispenserTouchTrigger, CBaseTrigger );

public:
	CDispenserTouchTrigger() {}

	void Spawn( void )
	{
		BaseClass::Spawn();
		AddSpawnFlags( SF_TRIGGER_ALLOW_CLIENTS );
		InitTrigger();
	}

	virtual void StartTouch( CBaseEntity *pEntity )
	{
		CBaseEntity *pParent = GetOwnerEntity();

		if ( pParent )
		{
			pParent->StartTouch( pEntity );
		}
	}

	virtual void EndTouch( CBaseEntity *pEntity )
	{
		CBaseEntity *pParent = GetOwnerEntity();

		if ( pParent )
		{
			pParent->EndTouch( pEntity );
		}
	}
};

LINK_ENTITY_TO_CLASS( dispenser_touch_trigger, CDispenserTouchTrigger );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CObjectDispenser::CObjectDispenser()
{
	SetMaxHealth( DISPENSER_MAX_HEALTH );
	m_iHealth = DISPENSER_MAX_HEALTH;
	UseClientSideAnimation();

	m_hTouchingEntities.Purge();
	m_bDisplayingHint = false;
	m_flHintDisplayTime = gpGlobals->curtime;

	SetType( OBJ_DISPENSER );
}

CObjectDispenser::~CObjectDispenser()
{
	if ( m_hTouchTrigger.Get() )
	{
		UTIL_Remove( m_hTouchTrigger );
	}

	int iSize = m_hHealingTargets.Count();
	for ( int i = iSize-1; i >= 0; i-- )
	{
		EHANDLE hOther = m_hHealingTargets[i];

		StopHealing( hOther );
	}

	StopSound( "Building_Dispenser.Idle" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectDispenser::Spawn()
{
	SetModel( GetPlacementModel() );
	SetSolid( SOLID_BBOX );

	UTIL_SetSize( this, DISPENSER_MINS, DISPENSER_MAXS );
	m_takedamage = DAMAGE_YES;

	m_iAmmoMetal = 0;
	m_bDetonated = false;

	BaseClass::Spawn();
}

void CObjectDispenser::MakeCarriedObject(CTFPlayer* pPlayer)
{
	StopSound( "Building_Dispenser.Idle" );

	// Remove our healing trigger.
	if ( m_hTouchTrigger.Get() )
	{
		UTIL_Remove( m_hTouchTrigger );
		m_hTouchTrigger = NULL;
	}

	// Stop healing everyone.
	for ( int i = m_hTouchingEntities.Count() - 1; i >= 0; i-- )
	{
		EHANDLE hEnt = m_hTouchingEntities[ i ];

		CBaseEntity* pOther = hEnt.Get();

		if ( pOther )
		{
			EndTouch( pOther );
		}
	}

	// Stop all thinking, we'll resume it once we get re-deployed.
	SetContextThink( NULL, 0, DISPENSE_CONTEXT );
	SetContextThink( NULL, 0, REFILL_CONTEXT );

	BaseClass::MakeCarriedObject( pPlayer );
}

void CObjectDispenser::DropCarriedObject( CTFPlayer* pPlayer )
{
	BaseClass::DropCarriedObject( pPlayer );
}

//-----------------------------------------------------------------------------
// Purpose: Start building the object
//-----------------------------------------------------------------------------
bool CObjectDispenser::StartBuilding( CBaseEntity *pBuilder )
{
	SetModel( DISPENSER_MODEL_LEVEL_1_UPGRADE );

	CreateBuildPoints();

	return BaseClass::StartBuilding( pBuilder );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectDispenser::InitializeMapPlacedObject( void )
{
	// Must set model here so we can add control panels.
	SetModel( DISPENSER_MODEL_LEVEL_1 );
	BaseClass::InitializeMapPlacedObject();
}

void CObjectDispenser::SetModel( const char *pModel )
{
	BaseClass::SetModel( pModel );
	UTIL_SetSize( this, DISPENSER_MINS, DISPENSER_MAXS );
	
	SetSolid( SOLID_BBOX );

	CreateBuildPoints();

	ReattachChildren();

	ResetSequenceInfo();
}

//-----------------------------------------------------------------------------
// Purpose: Finished building
//-----------------------------------------------------------------------------
void CObjectDispenser::OnGoActive( void )
{
	/*
	CTFPlayer *pBuilder = GetBuilder();

	Assert( pBuilder );

	if ( !pBuilder )
		return;
	*/
	SetModel( DISPENSER_MODEL_LEVEL_1 );
	CreateBuildPoints();

	// Put some ammo in the Dispenser
	if ( !m_bCarryDeploy )
		m_iAmmoMetal = 25;

	// Begin thinking
	SetContextThink( &CObjectDispenser::RefillThink, gpGlobals->curtime + 3, REFILL_CONTEXT );
	SetContextThink( &CObjectDispenser::DispenseThink, gpGlobals->curtime + 0.1, DISPENSE_CONTEXT );

	m_flNextAmmoDispense = gpGlobals->curtime + 0.5;

	CDispenserTouchTrigger *pTriggerEnt;

	if ( m_szTriggerName != NULL_STRING )
	{
		pTriggerEnt = dynamic_cast< CDispenserTouchTrigger* >( gEntList.FindEntityByName( NULL, m_szTriggerName ) );
		if ( pTriggerEnt )
		{	
			pTriggerEnt->SetOwnerEntity( this );
			m_hTouchTrigger = pTriggerEnt;
		}
	}
	else
	{
		pTriggerEnt = dynamic_cast< CDispenserTouchTrigger* >( CBaseEntity::Create( "dispenser_touch_trigger", GetAbsOrigin(), vec3_angle, this ) );
		if ( pTriggerEnt )
		{
			pTriggerEnt->SetSolid( SOLID_BBOX );
			UTIL_SetSize( pTriggerEnt, Vector( -70,-70,-70 ), Vector( 70,70,70 ) );
			m_hTouchTrigger = pTriggerEnt;
		}
	}

	BaseClass::OnGoActive();

	EmitSound( "Building_Dispenser.Idle" );
}

//-----------------------------------------------------------------------------
// Spawn the vgui control screens on the object
//-----------------------------------------------------------------------------
void CObjectDispenser::GetControlPanelInfo( int nPanelIndex, const char *&pPanelName )
{
	// Panels 0 and 1 are both control panels for now
	if ( nPanelIndex == 0 || nPanelIndex == 1 )
	{
		switch( GetTeamNumber() )
		{
			case TF_TEAM_RED:
				pPanelName = "screen_obj_dispenser_red";
				break;
			default:
			case TF_TEAM_BLUE:
				pPanelName = "screen_obj_dispenser_blue";
				break;
		}
	}
	else
	{
		BaseClass::GetControlPanelInfo( nPanelIndex, pPanelName );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectDispenser::Precache()
{
	BaseClass::Precache();

	int iModelIndex;

	iModelIndex = PrecacheModel(GetPlacementModel());
	PrecacheGibsForModel(iModelIndex);

	iModelIndex = PrecacheModel( DISPENSER_MODEL_LEVEL_1 );
	PrecacheGibsForModel(iModelIndex);

	iModelIndex = PrecacheModel( DISPENSER_MODEL_LEVEL_1_UPGRADE );
	PrecacheGibsForModel(iModelIndex);

	iModelIndex = PrecacheModel( DISPENSER_MODEL_LEVEL_2 );
	PrecacheGibsForModel(iModelIndex);

	iModelIndex = PrecacheModel( DISPENSER_MODEL_LEVEL_2_UPGRADE );
	PrecacheGibsForModel(iModelIndex);

	iModelIndex = PrecacheModel( DISPENSER_MODEL_LEVEL_3 );
	PrecacheGibsForModel(iModelIndex);

	iModelIndex = PrecacheModel( DISPENSER_MODEL_LEVEL_3_UPGRADE );
	PrecacheGibsForModel(iModelIndex);

	PrecacheVGuiScreen( "screen_obj_dispenser_blue" );
	PrecacheVGuiScreen( "screen_obj_dispenser_red" );

	PrecacheScriptSound( "Building_Dispenser.Idle" );
	PrecacheScriptSound( "Building_Dispenser.GenerateMetal" );
	PrecacheScriptSound( "Building_Dispenser.Heal" );
	PrecacheScriptSound( "Hud.Hint" );

	PrecacheParticleSystem( "dispenser_heal_red" );
	PrecacheParticleSystem( "dispenser_heal_blue" );
}
#define DISPENSER_UPGRADE_DURATION	1.5f

//-----------------------------------------------------------------------------
// Hit by a friendly engineer's wrench
//-----------------------------------------------------------------------------
bool CObjectDispenser::OnWrenchHit( CTFPlayer* pPlayer, CTFWrench* pWrench, Vector vecHitPos )
{
	bool bDidWork = false;

	bDidWork = BaseClass::OnWrenchHit( pPlayer, pWrench, vecHitPos );

	return bDidWork;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
bool CObjectDispenser::IsUpgrading( void ) const
{
	return m_bIsUpgrading;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
char* CObjectDispenser::GetPlacementModel( void )
{
	return DISPENSER_MODEL_PLACEMENT;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int CObjectDispenser::GetMaxUpgradeLevel( void )
{
	return ( pf_upgradable_buildings.GetBool() ? 3 : 1 );
}

//-----------------------------------------------------------------------------
// Raises the dispenser one level
//-----------------------------------------------------------------------------
void CObjectDispenser::StartUpgrading(void)
{
	ResetHealingTargets();

	BaseClass::StartUpgrading();

	switch ( GetUpgradeLevel() )
	{
	case 1:
		SetModel( DISPENSER_MODEL_LEVEL_1_UPGRADE );
		break;
	case 2:
		SetModel( DISPENSER_MODEL_LEVEL_2_UPGRADE );
		break;
	case 3:
		SetModel( DISPENSER_MODEL_LEVEL_3_UPGRADE );
		break;
	default:
		Assert(0);
		break;
	}
	
	m_bIsUpgrading = true;

	// Start upgrade anim instantly
	DetermineAnimation();
}

void CObjectDispenser::FinishUpgrading(void)
{
	switch ( GetUpgradeLevel() )
	{
	case 1:
		SetModel( DISPENSER_MODEL_LEVEL_1 );
		break;
	case 2:
		SetModel( DISPENSER_MODEL_LEVEL_2 );
		break;
	case 3:
		SetModel( DISPENSER_MODEL_LEVEL_3 );
		break;
	default:
		Assert( 0 );
		break;
	}

	m_bIsUpgrading = false;
	
	SetActivity( ACT_RESET );

	BaseClass::FinishUpgrading();
}

//-----------------------------------------------------------------------------
// If detonated, do some damage
//-----------------------------------------------------------------------------
void CObjectDispenser::DetonateObject( void )
{
	float flDamage = min(100 + m_iAmmoMetal, 220);

	// Play explosion sound and effect.
	Vector vecOrigin = GetAbsOrigin();
	CTakeDamageInfo info( this, GetBuilder(), vec3_origin, GetAbsOrigin(), flDamage, DMG_BLAST | DMG_HALF_FALLOFF);
	info.SetDamageCustom( TF_DMG_CUSTOM_DISPENSER_EXPLOSION );
	RadiusDamage( info, vecOrigin, flDamage, CLASS_NONE, NULL );

	m_bDetonated = true;
	BaseClass::DetonateObject();
}

//-----------------------------------------------------------------------------
// Purpose: Destroy building and give builder metal invested + amount stored
//-----------------------------------------------------------------------------
void CObjectDispenser::DismantleObject( CTFPlayer *pPlayer )
{
	pPlayer->SetAllowAmmoOverdraw( true );

	int iMetal = m_iAmmoMetal;
	iMetal += m_iUpgradeMetal;
	iMetal += GetObjectInfo( ObjectType() )->m_Cost;
	iMetal += ( m_iUpgradeLevel - 1 ) * m_iUpgradeMetalRequired;
	iMetal *= pf_building_dismantle_factor.GetFloat();

	int iMetalGiven = pPlayer->GiveAmmo( iMetal, TF_AMMO_METAL, false );

	pPlayer->SetAllowAmmoOverdraw( false );
	BaseClass::DismantleObject( pPlayer );
}

//-----------------------------------------------------------------------------
// Handle commands sent from vgui panels on the client 
//-----------------------------------------------------------------------------
bool CObjectDispenser::ClientCommand( CTFPlayer *pPlayer, const CCommand &args )
{
	const char *pCmd = args[0];
	if ( FStrEq( pCmd, "use" ) )
	{
		// I can't do anything if I'm not active
		if ( !ShouldBeActive() ) 
			return true;

		// player used the dispenser
		if ( DispenseAmmo( pPlayer ) )
		{
			CSingleUserRecipientFilter filter( pPlayer );
			pPlayer->EmitSound( filter, pPlayer->entindex(), "BaseCombatCharacter.AmmoPickup" );
		}

		return true;
	}
	else if ( FStrEq( pCmd, "repair" ) )
	{
		Command_Repair( pPlayer );
		return true;
	}

	return BaseClass::ClientCommand( pPlayer, args );
}

bool CObjectDispenser::DispenseAmmo( CTFPlayer *pPlayer )
{
	int iTotalPickedUp = 0;

	// primary
	int iPrimary = pPlayer->GiveAmmo( floor(pPlayer->m_Shared.MaxAmmo( TF_AMMO_PRIMARY ) * g_flDispenserAmmoRates[ GetUpgradeLevel() - 1 ] ), TF_AMMO_PRIMARY );
	iTotalPickedUp += iPrimary;

	// secondary
	int iSecondary = pPlayer->GiveAmmo( floor( pPlayer->m_Shared.MaxAmmo( TF_AMMO_SECONDARY ) * g_flDispenserAmmoRates[ GetUpgradeLevel() - 1 ] ), TF_AMMO_SECONDARY );
	iTotalPickedUp += iSecondary;

	// Cart dispenser has infinite metal.
	int iMetalToGive = DISPENSER_DROP_METAL + 10 * ( GetUpgradeLevel() - 1 );

	if( ( GetObjectFlags() & OF_IS_CART_OBJECT ) == 0 )
		iMetalToGive = min( m_iAmmoMetal.Get(), iMetalToGive);

	// metal
	int iMetal = pPlayer->GiveAmmo( iMetalToGive, TF_AMMO_METAL, false );
	iTotalPickedUp += iMetal;

	if( ( GetObjectFlags() & OF_IS_CART_OBJECT ) == 0 )
		m_iAmmoMetal -= iMetal;

	if( iTotalPickedUp > 0 )
	{
		return true;
	}

	// return false if we didn't pick up anything
	return false;
}

int CObjectDispenser::GetBaseHealth(void)
{
	return DISPENSER_MAX_HEALTH;
}

float CObjectDispenser::GetDispenserRadius(void)
{
	return 64.0f;
}

float CObjectDispenser::GetHealRate( void )
{
	return g_flDispenserHealRates[ GetUpgradeLevel() - 1 ];
}

#ifdef PF2_DLL
float CObjectDispenser::GetArmorRate( CBaseEntity* pOther )
{
	CTFPlayer* pPlayer = ToTFPlayer( pOther );

	if ( !pPlayer )
		return 0;

	int iArmorType = pPlayer->GetPlayerClass()->GetArmorType();
	return g_flDispenserArmorRates[GetUpgradeLevel()][iArmorType];
}
#endif

void CObjectDispenser::RefillThink( void )
{
	if (GetObjectFlags() & OF_IS_CART_OBJECT)
		return;

	if ( IsDisabled() || IsUpgrading() || IsRedeploying() )
	{
		// Hit a refill time while disabled, so do the next refill ASAP.
		SetContextThink( &CObjectDispenser::RefillThink, gpGlobals->curtime + 0.1, REFILL_CONTEXT );
		return;
	}

	// Auto-refill half the amount as tfc, but twice as often
	if ( m_iAmmoMetal < DISPENSER_MAX_METAL_AMMO )
	{
		m_iAmmoMetal = MIN(m_iAmmoMetal + DISPENSER_MAX_METAL_AMMO * (0.1f + 0.025f * (GetUpgradeLevel() - 1)), DISPENSER_MAX_METAL_AMMO);
		EmitSound( "Building_Dispenser.GenerateMetal" );
	}

	SetContextThink( &CObjectDispenser::RefillThink, gpGlobals->curtime + 6, REFILL_CONTEXT );
}

//-----------------------------------------------------------------------------
// Generate ammo over time
//-----------------------------------------------------------------------------
void CObjectDispenser::DispenseThink(void)
{
	if ( IsDisabled() || IsUpgrading() || IsRedeploying() )
	{
		// Don't heal or dispense ammo
		SetContextThink(&CObjectDispenser::DispenseThink, gpGlobals->curtime + 0.1, DISPENSE_CONTEXT);

		// stop healing everyone
		for ( int i = m_hHealingTargets.Count() - 1; i >= 0; i-- )
		{
			EHANDLE hEnt = m_hHealingTargets[ i ];

			CBaseEntity* pOther = hEnt.Get();

			if ( pOther )
			{
				StopHealing( pOther );
			}
		}
		
		return;
	}

	if ( m_flHintDisplayTime < gpGlobals->curtime && m_bDisplayingHint )
	{
		m_bDisplayingHint = false;
	}

	if ( m_flNextAmmoDispense <= gpGlobals->curtime )
	{
		int iNumNearbyPlayers = 0;
		int iEnemiesUsing = 0;

		// find players in sphere, that are visible
		static float flRadius = GetDispenserRadius();
		Vector vecOrigin = GetAbsOrigin() + Vector(0, 0, 32);

		CBaseEntity* pListOfNearbyEntities[32];
		int iNumberOfNearbyEntities = UTIL_EntitiesInSphere( pListOfNearbyEntities, 32, vecOrigin, flRadius, FL_CLIENT );
		for (int i = 0; i < iNumberOfNearbyEntities; i++)
		{
			CTFPlayer* pPlayer = ToTFPlayer( pListOfNearbyEntities[i] );

			if ( !pPlayer || !pPlayer->IsAlive() || !CouldHealTarget( pPlayer ) )
				continue;

			DispenseAmmo( pPlayer );

			iNumNearbyPlayers++;

			if ( GetBuilder() && pPlayer->GetTeamNumber() != GetBuilder()->GetTeamNumber() )
			{
				if (pPlayer->GetPlayerClass()->GetClassIndex() != TF_CLASS_SPY && !pPlayer->m_Shared.InCond(TF_COND_DISGUISED))
				{
					//SetEnemyUsing(true);
					iEnemiesUsing++;
				}
			}
		}

		if ( iEnemiesUsing == 0 )
		{
			SetEnemyUsing( false );
		}
		else
		{
			SetEnemyUsing( true );
		}

		// for each player in touching list
		int iSize = m_hTouchingEntities.Count();
		for ( int i = iSize-1; i >= 0; i-- )
		{
			EHANDLE hOther = m_hTouchingEntities[i];

			CBaseEntity *pEnt = hOther.Get();
			bool bHealingTarget = IsHealingTarget( pEnt );
			bool bValidHealTarget = CouldHealTarget( pEnt );

			if ( bHealingTarget  && !bValidHealTarget )
			{
				// if we can't see them, remove them from healing list
				// does nothing if we are not healing them already
				StopHealing( pEnt );
			}
			else if ( !bHealingTarget && bValidHealTarget )
			{
				// if we can see them, add to healing list	
				// does nothing if we are healing them already
				StartHealing( pEnt );	
			}	

		}
		// Try to dispense more often when no players are around so we 
		// give it as soon as possible when a new player shows up
		m_flNextAmmoDispense = gpGlobals->curtime + ( ( iNumNearbyPlayers > 0 ) ? 1.0 : 0.1);
	}

	ResetHealingTargets();

	SetContextThink( &CObjectDispenser::DispenseThink, gpGlobals->curtime + 0.1, DISPENSE_CONTEXT );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectDispenser::ResetHealingTargets( void )
{
	// for each player in touching list
	int iSize = m_hTouchingEntities.Count();
	for ( int i = iSize-1; i >= 0; i-- )
	{
		EHANDLE hOther = m_hTouchingEntities[i];

		CBaseEntity *pEnt = hOther.Get();
		bool bHealingTarget = IsHealingTarget( pEnt );
		bool bValidHealTarget = CouldHealTarget( pEnt );

		if ( bHealingTarget && !bValidHealTarget )
		{
			// if we can't see them, remove them from healing list
			// does nothing if we are not healing them already
			StopHealing( pEnt );
		}
		else if ( !bHealingTarget && bValidHealTarget )
		{
			// if we can see them, add to healing list	
			// does nothing if we are healing them already
			StartHealing( pEnt );
		}	
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectDispenser::StartTouch( CBaseEntity *pOther )
{
	// add to touching entities
	EHANDLE hOther = pOther;
	m_hTouchingEntities.AddToTail( hOther );

	if ( !IsBuilding() && !IsDisabled() && !IsRedeploying() && CouldHealTarget( pOther ) && !IsHealingTarget( pOther ) )
	{
		// try to start healing them
		StartHealing( pOther );		
	}

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectDispenser::EndTouch( CBaseEntity *pOther )
{
	// remove from touching entities
	EHANDLE hOther = pOther;
	m_hTouchingEntities.FindAndRemove( hOther );

	// remove from healing list
	StopHealing( pOther );
}

//-----------------------------------------------------------------------------
// Purpose: Try to start healing this target
//-----------------------------------------------------------------------------
void CObjectDispenser::StartHealing( CBaseEntity *pOther )
{
	AddHealingTarget( pOther );

	CTFPlayer *pPlayer = ToTFPlayer( pOther );

	if ( pPlayer )
	{
		pPlayer->m_Shared.Heal( GetOwner(), GetHealRate(), true );
#ifdef PF2_DLL
		pPlayer->m_Shared.RegenerateArmor( GetOwner(), GetArmorRate( pOther ) );
#endif
	}
}

//-----------------------------------------------------------------------------
// Purpose: Stop healing this target
//-----------------------------------------------------------------------------
void CObjectDispenser::StopHealing( CBaseEntity *pOther )
{
	bool bFound = false;

	EHANDLE hOther = pOther;
	bFound = m_hHealingTargets.FindAndRemove( hOther );

	if ( bFound )
	{
		CTFPlayer *pPlayer = ToTFPlayer( pOther );

		if ( pPlayer )
		{
			pPlayer->m_Shared.StopHealing( GetOwner() );
#ifdef PF2_DLL
			pPlayer->m_Shared.StopRegeneratingArmor( GetOwner() );
#endif
		}

		NetworkStateChanged();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Is this a valid heal target? and not already healing them?
//-----------------------------------------------------------------------------
bool CObjectDispenser::CouldHealTarget( CBaseEntity *pTarget )
{
	if ( !pTarget )
	{
		return false;
	}

	if ( !HasSpawnFlags( SF_IGNORE_LOS ) && !pTarget->FVisible(this, MASK_BLOCKLOS) )
		return false;

	if ( pTarget->IsPlayer() && pTarget->IsAlive() )
	{
		/*CTFPlayer *pTFPlayer = ToTFPlayer( pTarget );

		// don't heal enemies unless they are disguised as our team
		int iTeam = GetTeamNumber();
		int iPlayerTeam = pTFPlayer->GetTeamNumber();

		if ( iPlayerTeam != iTeam && pTFPlayer->m_Shared.InCond( TF_COND_DISGUISED ) )
		{
			iPlayerTeam = pTFPlayer->m_Shared.GetDisguiseTeam();
		}

		if ( iPlayerTeam != iTeam )
		{
			return false;
		}*/

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CObjectDispenser::AddHealingTarget( CBaseEntity *pOther )
{
	// add to tail
	EHANDLE hOther = pOther;
	m_hHealingTargets.AddToTail( hOther );
	NetworkStateChanged();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CObjectDispenser::RemoveHealingTarget( CBaseEntity *pOther )
{
	// remove
	EHANDLE hOther = pOther;
	m_hHealingTargets.FindAndRemove( hOther );
}

//-----------------------------------------------------------------------------
// Purpose: Are we healing this target already
//-----------------------------------------------------------------------------
bool CObjectDispenser::IsHealingTarget( CBaseEntity *pTarget )
{
	EHANDLE hOther = pTarget;
	return m_hHealingTargets.HasElement( hOther );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CObjectDispenser::DrawDebugTextOverlays( void ) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];
		Q_snprintf( tempstr, sizeof( tempstr ),"Metal: %d", m_iAmmoMetal );
		EntityText(text_offset,tempstr,0);
		text_offset++;
	}
	return text_offset;
}


// Payload cart stuff


IMPLEMENT_SERVERCLASS_ST( CObjectCartDispenser, DT_ObjectCartDispenser )
END_SEND_TABLE()

BEGIN_DATADESC( CObjectCartDispenser )
DEFINE_KEYFIELD( m_szTriggerName, FIELD_STRING, "touch_trigger" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( mapobj_cart_dispenser, CObjectCartDispenser );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectCartDispenser::Spawn( void )
{
	SetObjectFlags( OF_IS_CART_OBJECT );

	m_takedamage = DAMAGE_NO;

	m_iUpgradeLevel = 1;
	m_iUpgradeMetal = 0;

	AddFlag( FL_OBJECT );

	m_iAmmoMetal = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectCartDispenser::SetModel( const char *pModel )
{
	// Deliberately skip dispenser since it has some stuff we don't want.
	CBaseObject::SetModel( pModel );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectCartDispenser::OnGoActive( void )
{
	// Hacky: base class needs a model to init some things properly so we gotta clear it here.
	BaseClass::OnGoActive();
	SetModel( "" );
}

//-----------------------------------------------------------------------------
// Purpose: Is this a valid heal target? and not already healing them?
//-----------------------------------------------------------------------------
bool CObjectCartDispenser::CouldHealTarget( CBaseEntity *pTarget )
{
	if( !pTarget )
		return false;

	if( !HasSpawnFlags( SF_IGNORE_LOS ) && !pTarget->FVisible( this, MASK_BLOCKLOS ) )
		return false;

	if( pTarget->IsPlayer() && pTarget->IsAlive() )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( pTarget );

		// don't heal enemies unless they are disguised as our team
		int iTeam = GetTeamNumber();
		int iPlayerTeam = pTFPlayer->GetTeamNumber();

		if ( iPlayerTeam != iTeam && pTFPlayer->m_Shared.InCond( TF_COND_DISGUISED ) )
		{
			iPlayerTeam = pTFPlayer->m_Shared.GetDisguiseTeam();
		}

		if ( iPlayerTeam != iTeam )
		{
			return false;
		}

		return true;
	}

	return false;
}
