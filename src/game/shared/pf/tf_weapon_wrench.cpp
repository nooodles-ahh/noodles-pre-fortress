//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_wrench.h"
#include "decals.h"
#include "baseobject_shared.h"
#include "tf_gamerules.h"

// Client specific.
#ifdef CLIENT_DLL
	#include "c_tf_player.h"
	#include "c_obj_sapper.h"
// Server specific.
#else
	#include "tf_player.h"
	#include "variant_t.h"
	#include "tf_obj_sapper.h"
	#include "pf_cvars.h"
	#include "tf_gamestats.h"
#endif

#ifdef CLIENT_DLL
#define CObjectSapper C_ObjectSapper
#endif

#define ARMOR_HEAL_SND "Weapon_Wrench.HealArmor"

//=============================================================================
//
// Weapon Wrench tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFWrench, DT_TFWeaponWrench )

BEGIN_NETWORK_TABLE( CTFWrench, DT_TFWeaponWrench )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFWrench )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_wrench, CTFWrench );
PRECACHE_WEAPON_REGISTER( tf_weapon_wrench );

//=============================================================================
//
// Weapon Wrench functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFWrench::CTFWrench()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWrench::Precache()
{
	PrecacheScriptSound( ARMOR_HEAL_SND );

	BaseClass::Precache();
}

#ifdef GAME_DLL
void CTFWrench::OnFriendlyBuildingHit( CBaseObject *pObject, CTFPlayer *pPlayer, Vector vecHitPos )
{
	// Did this object hit do any work? repair or upgrade?
	bool bUsefulHit = pObject->InputWrenchHit( pPlayer, this, vecHitPos );

	CDisablePredictionFiltering disabler;

	if ( bUsefulHit )
	{
		// play success sound
		WeaponSound( SPECIAL1 );
	}
	else
	{
		// play failure sound
		WeaponSound( SPECIAL2 );
	}
}

ConVar pf_armor_repair_cost_percentage( "pf_armor_repair_cost_percentage", "0.2", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY );
void CTFWrench::OnEntityHit( CBaseEntity* pEntity )
{
	if( TFGameRules()->FriendlyFireMode() == FFMODE_FORCED )
		return;

	CTFPlayer* pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	CTFPlayer* pTarget = ToTFPlayer( pEntity );
	if ( !pTarget )
		return;

	if ( pTarget->GetTeamNumber() == pPlayer->GetTeamNumber() )
	{
		int iArmorMax = pTarget->GetPlayerClass()->GetMaxArmor();
		int iArmor = pTarget->ArmorValue();

		int iAmountToHeal = min( 50, iArmorMax - iArmor );

		// Can we restore armor on this player?
		if( iAmountToHeal > 0 )
		{
			// how much is this gonna cost us?
			int iRepairCost = ceil( (float)(iAmountToHeal)*pf_armor_repair_cost_percentage.GetFloat() );
			float flNewArmor = 0;

			// Do we need to calculate how much metal to take away?
			if( pf_armor_repair_cost_percentage.GetFloat() > 0.0f )
			{
				int iRepairMulti = (int)(1 / pf_armor_repair_cost_percentage.GetFloat());
				// is the cost higher than our metal count?
				if( iRepairCost > pPlayer->GetAmmoCount( TF_AMMO_METAL ) )
				{
					iRepairCost = pPlayer->GetAmmoCount( TF_AMMO_METAL );
				}

				if( !pf_infinite_ammo.GetBool() )
					pPlayer->RemoveAmmo( iRepairCost, TF_AMMO_METAL );


				flNewArmor = min( iArmorMax, iArmor + (iRepairCost * iRepairMulti) );
			}
			else
			{
				flNewArmor = min( iArmorMax, iArmor + iAmountToHeal );
			}

			// Are we actually restoring any armor?
			if( flNewArmor > (float)iArmor )
			{
				int iArmorAdded = flNewArmor - iArmor;
				pTarget->SetArmorValue( flNewArmor );

				CPASAttenuationFilter filter( pTarget->WorldSpaceCenter() );
				EmitSound( filter, entindex(), ARMOR_HEAL_SND );

				// Update player stats
				CTF_GameStats.Event_PlayerArmorRepairedOther( pPlayer, iArmorAdded );

				// Send event saying that we did a thing
				IGameEvent *event = gameeventmanager->CreateEvent( "player_repaired" );
				if( event )
				{
					event->SetInt( "priority", 1 );
					event->SetInt( "patient", pTarget->GetUserID() );
					event->SetInt( "repairer", pPlayer->GetUserID() );
					event->SetInt( "amount", iArmorAdded );

					gameeventmanager->FireEvent( event );
				}

				event = gameeventmanager->CreateEvent( "player_repaironhit" );
				if( event )
				{
					event->SetInt( "priority", 1 );
					event->SetInt( "entindex", pTarget->entindex() );
					event->SetInt( "amount", iArmorAdded );

					gameeventmanager->FireEvent( event );
				}
			}
		}
	}
}
#endif

void CTFWrench::Smack( void )
{
	// see if we can hit an object with a higher range

	// Get the current player.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	if ( !CanAttack() )
		return;

	// Setup a volume for the melee weapon to be swung - approx size, so all melee behave the same.
	static Vector vecSwingMins( -18, -18, -18 );
	static Vector vecSwingMaxs( 18, 18, 18 );

	// Setup the swing range.
	Vector vecForward; 
	AngleVectors( pPlayer->EyeAngles(), &vecForward );
	Vector vecSwingStart = pPlayer->Weapon_ShootPosition();
	Vector vecSwingEnd = vecSwingStart + vecForward * 70;

	// only trace against objects

	// See if we hit anything.
	trace_t trace;	

	CTraceFilterIgnorePlayers traceFilter( NULL, COLLISION_GROUP_NONE );
	UTIL_TraceLine( vecSwingStart, vecSwingEnd, MASK_SOLID, &traceFilter, &trace );
	if ( trace.fraction >= 1.0 )
	{
		UTIL_TraceHull( vecSwingStart, vecSwingEnd, vecSwingMins, vecSwingMaxs, MASK_SOLID, &traceFilter, &trace );
	}

	// We hit, setup the smack.
	if ( trace.fraction < 1.0f &&
		 trace.m_pEnt &&
		 trace.m_pEnt->IsBaseObject() &&
		 trace.m_pEnt->GetTeamNumber() == pPlayer->GetTeamNumber() )
	{
#ifdef GAME_DLL
		OnFriendlyBuildingHit( dynamic_cast< CBaseObject * >( trace.m_pEnt ), pPlayer, trace.endpos );
#endif
	}
	else
	{
		// if we cannot, Smack as usual for player hits
		BaseClass::Smack();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFWrench::GetMeleeDamage( CBaseEntity *pTarget, int &iCustomDamage )
{
	float flBaseDamage = BaseClass::GetMeleeDamage( pTarget, iCustomDamage );
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if (pTarget->IsPlayer() || pTarget->IsBaseObject())
	{
		if( TFGameRules()->FriendlyFireMode() != FFMODE_FORCED )
		{
			if( pPlayer && pTarget->GetTeamNumber() == pPlayer->GetTeamNumber() )
				flBaseDamage = 0.0f;
		}
	}
	return flBaseDamage;
}
