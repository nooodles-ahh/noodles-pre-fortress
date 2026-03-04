//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose:
//
//=============================================================================

#include "cbase.h"
#include "tf_weaponbase_melee.h"
#include "effect_dispatch_data.h"
#include "tf_gamerules.h"
#ifdef PF2
#include "tf_weapon_knife.h"
#include "pf_cvars.h"
#endif
// Server specific.
#if !defined( CLIENT_DLL )
#include "tf_player.h"
#include "tf_gamestats.h"
#include "ilagcompensationmanager.h"
// Client specific.
#else
#include "c_tf_player.h"
#endif

//=============================================================================
//
// TFWeaponBase Melee tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFWeaponBaseMelee, DT_TFWeaponBaseMelee )

BEGIN_NETWORK_TABLE( CTFWeaponBaseMelee, DT_TFWeaponBaseMelee )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFWeaponBaseMelee )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weaponbase_melee, CTFWeaponBaseMelee );

// Server specific.
#if !defined( CLIENT_DLL )
BEGIN_DATADESC( CTFWeaponBaseMelee )
DEFINE_FUNCTION( Smack )
END_DATADESC()
#endif

#ifndef CLIENT_DLL
ConVar tf_meleeattackforcescale( "tf_meleeattackforcescale", "80.0", FCVAR_CHEAT | FCVAR_GAMEDLL | FCVAR_DEVELOPMENTONLY );
#endif

ConVar tf_weapon_criticals_melee( "tf_weapon_criticals_melee", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Controls random crits for melee weapons.\n0 - Melee weapons do not randomly crit. \n1 - Melee weapons can randomly crit only if tf_weapon_criticals is also enabled. \n2 - Melee weapons can always randomly crit regardless of the tf_weapon_criticals setting.", true, 0, true, 2 );
extern ConVar tf_weapon_criticals;

//=============================================================================
//
// TFWeaponBase Melee functions.
//

// -----------------------------------------------------------------------------
// Purpose: Constructor.
// -----------------------------------------------------------------------------
CTFWeaponBaseMelee::CTFWeaponBaseMelee()
{
	WeaponReset();
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFWeaponBaseMelee::WeaponReset( void )
{
	BaseClass::WeaponReset();

	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;
	m_flSmackTime = -1.0f;
	m_bConnected = false;
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFWeaponBaseMelee::Precache()
{
	BaseClass::Precache();
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFWeaponBaseMelee::Spawn()
{
	Precache();

	// Get the weapon information.
	WEAPON_FILE_INFO_HANDLE	hWpnInfo = LookupWeaponInfoSlot( GetClassname() );
	Assert( hWpnInfo != GetInvalidWeaponInfoHandle() );
	CTFWeaponInfo *pWeaponInfo = dynamic_cast< CTFWeaponInfo* >( GetFileWeaponInfoFromHandle( hWpnInfo ) );
	Assert( pWeaponInfo && "Failed to get CTFWeaponInfo in melee weapon spawn" );
	m_pWeaponInfo = pWeaponInfo;
	Assert( m_pWeaponInfo );

	// No ammo.
	m_iClip1 = -1;

	BaseClass::Spawn();
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
bool CTFWeaponBaseMelee::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	m_flSmackTime = -1.0f;
	if ( GetPlayerOwner() )
	{
		GetPlayerOwner()->m_flNextAttack = gpGlobals->curtime + 0.5;
	}
	return BaseClass::Holster( pSwitchingTo );
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFWeaponBaseMelee::PrimaryAttack()
{
	// Get the current player.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	if ( !CanAttack() )
		return;

	// Set the weapon usage mode - primary, secondary.
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;
	m_bConnected = false;

	// Swing the weapon.
	Swing( pPlayer );

#if !defined( CLIENT_DLL )
	pPlayer->SpeakWeaponFire();
	CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, IsCurrentAttackACritical() );
#endif
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFWeaponBaseMelee::SecondaryAttack()
{
	// semi-auto behaviour
	if ( m_bInAttack2 )
		return;

	// Get the current player.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	pPlayer->DoClassSpecialSkill();

	m_bInAttack2 = true;

	m_flNextSecondaryAttack = gpGlobals->curtime + 0.5;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  : *pPlayer -
//-----------------------------------------------------------------------------
void CTFWeaponBaseMelee::Swing( CTFPlayer *pPlayer )
{
	CalcIsAttackCritical();

	// Play the melee swing and miss (whoosh) always.
	SendPlayerAnimEvent( pPlayer );

	DoViewModelAnimation();

	// Set next attack times.
	m_flNextPrimaryAttack = gpGlobals->curtime + m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay;

	//SetWeaponIdleTime( m_flNextPrimaryAttack + m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeIdleEmpty );
	if ( IsCurrentAttackACrit() )
	{
		WeaponSound( BURST );
	}
	else
	{
		WeaponSound( MELEE_MISS );
	}

	m_flSmackTime = gpGlobals->curtime + m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flSmackDelay;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBaseMelee::DoViewModelAnimation( void )
{
#ifdef PF2
	Activity act = ( m_iWeaponMode == TF_WEAPON_PRIMARY_MODE ) ? ( IsCurrentAttackACrit() ? ACT_VM_SWINGHARD : ACT_VM_HITCENTER ) : ACT_VM_SWINGHARD;

	// Check if a crit animation exists
	if ( act == ACT_VM_SWINGHARD && SelectWeightedSequence( ACT_VM_SWINGHARD ) == ACTIVITY_NOT_AVAILABLE )
		act = ACT_VM_HITCENTER;

#else
	Activity act = ( m_iWeaponMode == TF_WEAPON_PRIMARY_MODE ) ? ACT_VM_HITCENTER : ACT_VM_SWINGHARD;
#endif
	SendWeaponAnim( act );
	SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration() );
}

//-----------------------------------------------------------------------------
// Purpose: Allow melee weapons to send different anim events
// Input  :  -
//-----------------------------------------------------------------------------
void CTFWeaponBaseMelee::SendPlayerAnimEvent( CTFPlayer *pPlayer )
{
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :  -
//-----------------------------------------------------------------------------
void CTFWeaponBaseMelee::ItemPostFrame()
{
	// Check for smack.
	if ( m_flSmackTime > 0.0f && gpGlobals->curtime > m_flSmackTime )
	{
		Smack();
		m_flSmackTime = -1.0f;
	}

	BaseClass::ItemPostFrame();
}

bool CTFWeaponBaseMelee::DoSwingTrace( trace_t &trace )
{
	// Setup a volume for the melee weapon to be swung - approx size, so all melee behave the same.
	static Vector vecSwingMins( -18, -18, -18 );
	static Vector vecSwingMaxs( 18, 18, 18 );

	// Get the current player.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return false;

	// Setup the swing range.
	Vector vecForward;
#ifdef PF2
	AngleVectors( pPlayer->EyeAngles() + pPlayer->ConcAngles(), &vecForward );
#else
	AngleVectors( pPlayer->EyeAngles(), &vecForward );
#endif
	Vector vecSwingStart = pPlayer->Weapon_ShootPosition();
	Vector vecSwingEnd = vecSwingStart + vecForward * GetSwingRange();

#ifdef PF2
	// Here is a bunch of totally unnecessary code for "muh immersion" 
	// The gist of it is that we use 2 traces, one that filters out teammates and one that doesn't
	// If our filtered trace hits something sooner than our unfiltered trace
	// we use the filtered trace for the hull trace, otherwise we use the normal one.
	// Why have I done this? So you can smack your teammates, while also being able to hit through them when needed
	// - Cyanide

	CTraceFilterIgnoreTeammates filter( pPlayer, COLLISION_GROUP_NONE, pPlayer->GetTeamNumber() );
	trace_t first, second;

	// Our two traces
	UTIL_TraceLine( vecSwingStart, vecSwingEnd, MASK_SOLID, &filter, &first ); // trace that ignores friends
	UTIL_TraceLine( vecSwingStart, vecSwingEnd, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &second ); // trace that doesn't

	// If our filtered trace hit something sooner use the filtered trace for the hull trace
	// any number < 1.0 means we hit something
	bool bUseTeamIgnoreTrace = false;
	if (first.fraction < second.fraction)
		bUseTeamIgnoreTrace = true;

	// alway use the filtered trace as it hits what we want
	trace = first;
#else
	// See if we hit anything.
	UTIL_TraceLine( vecSwingStart, vecSwingEnd, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &trace );
#endif

	if ( trace.fraction >= 1.0 )
	{
#ifdef PF2
		if ( bUseTeamIgnoreTrace )
		{
			UTIL_TraceHull( vecSwingStart, vecSwingEnd, vecSwingMins, vecSwingMaxs, MASK_SOLID, &filter, &trace );
		}
		else
#endif
		{
			UTIL_TraceHull( vecSwingStart, vecSwingEnd, vecSwingMins, vecSwingMaxs, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &trace );
		}
		if ( trace.fraction < 1.0 )
		{
			// Calculate the point of intersection of the line (or hull) and the object we hit
			// This is and approximation of the "best" intersection
			CBaseEntity *pHit = trace.m_pEnt;
			if ( !pHit || pHit->IsBSPModel() )
			{
				// Why duck hull min/max?
				FindHullIntersection( vecSwingStart, trace, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, pPlayer );
			}

#ifndef PF2
			// This is the point on the actual surface (the hull could have hit space)
			vecSwingEnd = trace.endpos;	
#endif
		}
	}

	return ( trace.fraction < 1.0f );
}

// -----------------------------------------------------------------------------
// Purpose:
// Note: Think function to delay the impact decal until the animation is finished
//       playing.
// -----------------------------------------------------------------------------
void CTFWeaponBaseMelee::Smack( void )
{
	trace_t trace;
#ifdef PF2
	CTFPlayer *pPlayer = GetTFPlayerOwner();
#else
	CBasePlayer *pPlayer = GetPlayerOwner();
#endif
	if ( !pPlayer )
		return;

#if !defined (CLIENT_DLL)
	// Move other players back to history positions based on local player's lag
	lagcompensation->StartLagCompensation( pPlayer, pPlayer->GetCurrentCommand() );
#endif

	// We hit, setup the smack.
	if ( DoSwingTrace( trace ) )
	{
#ifdef PF2
		// As for some bizzare reason the knife has a crit specific impact sound
		bool bKnife = GetWeaponID() == TF_WEAPON_KNIFE;
#endif
		// Hit sound - immediate.
		if( trace.m_pEnt->IsPlayer()  )
		{
#ifdef PF2
			if ( bKnife && IsCurrentAttackACrit())
			{
				WeaponSound( SPECIAL1 );
			}
			else
#endif
			{
				WeaponSound( MELEE_HIT );
			}
		}
		else
		{
			WeaponSound( MELEE_HIT_WORLD );
		}

		// Get the current player.
		CTFPlayer *pPlayer = GetTFPlayerOwner();
		if ( !pPlayer )
			return;

		Vector vecForward;
#ifdef PF2
		AngleVectors( pPlayer->EyeAngles() + pPlayer->ConcAngles(), &vecForward );
#else
		AngleVectors( pPlayer->EyeAngles(), &vecForward );
#endif
		Vector vecSwingStart = pPlayer->Weapon_ShootPosition();
		Vector vecSwingEnd = vecSwingStart + vecForward * GetSwingRange();

#ifndef CLIENT_DLL
		// Do Damage.
		int iCustomDamage = TF_DMG_CUSTOM_NONE;
		float flDamage = GetMeleeDamage( trace.m_pEnt, iCustomDamage );
		int iDmgType = DMG_BULLET | DMG_NEVERGIB | DMG_CLUB;
		if ( IsCurrentAttackACrit() )
		{
			// TODO: Not removing the old critical path yet, but the new custom damage is marking criticals as well for melee now.
			iDmgType |= DMG_CRITICAL;
		}
		CTakeDamageInfo info( pPlayer, pPlayer, flDamage, iDmgType, iCustomDamage );
		CalculateMeleeDamageForce( &info, vecForward, vecSwingEnd, 1.0f / flDamage * tf_meleeattackforcescale.GetFloat() );
		trace.m_pEnt->DispatchTraceAttack( info, vecForward, &trace );
		ApplyMultiDamage();

		OnEntityHit( trace.m_pEnt );
#endif
		// Don't impact trace friendly players or objects
#ifdef PF2
		if ( trace.m_pEnt && (trace.m_pEnt->GetTeamNumber() != pPlayer->GetTeamNumber() || TFGameRules()->FriendlyFireMode()) )
#else
		if ( trace.m_pEnt && trace.m_pEnt->GetTeamNumber() != pPlayer->GetTeamNumber() )
#endif
		{
#ifdef CLIENT_DLL
			UTIL_ImpactTrace( &trace, DMG_CLUB );
#endif
			m_bConnected = true;
		}

	}

#if !defined (CLIENT_DLL)
	lagcompensation->FinishLagCompensation( pPlayer );
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
// Output : float
//-----------------------------------------------------------------------------
float CTFWeaponBaseMelee::GetMeleeDamage( CBaseEntity *pTarget, int &iCustomDamage )
{
	return static_cast<float>( m_pWeaponInfo->GetWeaponDamage( m_iWeaponMode ) );
}

void CTFWeaponBaseMelee::OnEntityHit( CBaseEntity *pEntity )
{
	NULL;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFWeaponBaseMelee::CalcIsAttackCriticalHelper( void )
{
	int nCvarValue = tf_weapon_criticals_melee.GetInt();
	if ( nCvarValue == 0 )
		return false;

	if ( nCvarValue == 1 && !tf_weapon_criticals.GetBool() )
		return false;

	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return false;

	float flPlayerCritMult = pPlayer->GetCritMult();

	return ( RandomInt( 0, WEAPON_RANDOM_RANGE-1 ) <= ( TF_DAMAGE_CRIT_CHANCE_MELEE * flPlayerCritMult ) * WEAPON_RANDOM_RANGE );
}
