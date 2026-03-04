//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: Weapon Knife.
//
//=============================================================================
#include "cbase.h"
#include "tf_gamerules.h"
#include "tf_weapon_knife.h"
#include "decals.h"
#include "pf_cvars.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#include "tf_gamestats.h"
#include "ilagcompensationmanager.h"
#endif

//=============================================================================
//
// Weapon Knife tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFKnife, DT_TFWeaponKnife );

BEGIN_NETWORK_TABLE( CTFKnife, DT_TFWeaponKnife )
#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO( m_bReadyToBackstab ) ),
#else
	SendPropBool( SENDINFO( m_bReadyToBackstab ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFKnife )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_bReadyToBackstab, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_knife, CTFKnife );
PRECACHE_WEAPON_REGISTER( tf_weapon_knife );

//=============================================================================
//
// Weapon Knife functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFKnife::CTFKnife()
{
	m_bDelayedStab = pf_delayed_knife.GetBool();
}

//-----------------------------------------------------------------------------
// Purpose: Change idle anim to raised if we're ready to backstab.
//-----------------------------------------------------------------------------
bool CTFKnife::Deploy( void )
{
	if (BaseClass::Deploy())
	{
		m_bReadyToBackstab = false;
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFKnife::ItemPostFrame( void )
{
	if (!m_bDelayedStab)
		BackstabVMThink();
	BaseClass::ItemPostFrame();
}

//-----------------------------------------------------------------------------
// Purpose: Set stealth attack bool
//-----------------------------------------------------------------------------
void CTFKnife::PrimaryAttack( void )
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );

	if ( !CanAttack() )
		return;

#ifndef CLIENT_DLL
	lagcompensation->StartLagCompensation( pPlayer, pPlayer->GetCurrentCommand() );
#endif
	// Set the weapon usage mode - primary, secondary.
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

	trace_t trace;

	if( pPlayer->m_Shared.InCond(TF_COND_CRITBOOSTED) )
	{
		m_iWeaponMode = TF_WEAPON_SECONDARY_MODE;
	}

	if ( DoSwingTrace( trace ) == true )
	{
		// we will hit something with the attack
		if( trace.m_pEnt && trace.m_pEnt->IsPlayer() )
		{
			CTFPlayer *pTarget = ToTFPlayer( trace.m_pEnt );

			if ( pTarget && (pTarget->GetTeamNumber() != pPlayer->GetTeamNumber() || TFGameRules()->FriendlyFireMode()) )
			{
				// Deal extra damage to players when stabbing them from behind
				if (m_bDelayedStab ? IsBehindTarget( trace.m_pEnt ) : IsBehindAndFacingTarget( trace.m_pEnt ))
				{
					// this will be a backstab, do the strong anim
					m_iWeaponMode = TF_WEAPON_SECONDARY_MODE;

					// store the victim to compare when we do the damage
					m_hBackstabVictim = trace.m_pEnt;
				}
			}
		}
	}

#ifndef CLIENT_DLL
	pPlayer->RemoveInvisibility();
	pPlayer->RemoveDisguise();
	lagcompensation->FinishLagCompensation( pPlayer );
#endif

	m_bReadyToBackstab = false;

	// Swing the weapon.
	Swing( pPlayer );
	
	// And hit instantly.
	if (!m_bDelayedStab)
	{
		Smack();
		m_flSmackTime = 0.0f;
	}

#if !defined( CLIENT_DLL ) 
	pPlayer->SpeakWeaponFire();
	CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, IsCurrentAttackACritical() );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Do backstab damage
//-----------------------------------------------------------------------------
float CTFKnife::GetMeleeDamage( CBaseEntity *pTarget, int &iCustomDamage )
{

	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );

	float flBaseDamage = BaseClass::GetMeleeDamage( pTarget, iCustomDamage );

	if (pTarget->IsPlayer())
	{
		// This counts as a backstab if:
		// a ) we are behind the target player
		// or b) we were behind this target player when we started the stab
		if ((m_bDelayedStab ? IsBehindTarget( pTarget ) : IsBehindAndFacingTarget( pTarget )) ||
			(m_iWeaponMode == TF_WEAPON_SECONDARY_MODE && m_hBackstabVictim.Get() == pTarget))
		{
			// this will be a backstab, do the strong anim.
			// Do twice the target's health so that random modification will still kill him.
			flBaseDamage = pTarget->GetHealth() * 2;

			// Declare a backstab.
			iCustomDamage = TF_DMG_CUSTOM_BACKSTAB;
		}
		else
		{
			if( pPlayer->m_Shared.InCond( TF_COND_CRITBOOSTED ) )
			{
				m_bCurrentAttackIsCrit = true;
			}
			else
			{
				m_bCurrentAttackIsCrit = false;
			}
		}
	}

	return flBaseDamage;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFKnife::IsBehindTarget( CBaseEntity *pTarget )
{
	Assert( pTarget );

	// Get the forward view vector of the target, ignore Z
	Vector vecVictimForward;
	AngleVectors( pTarget->EyeAngles(), &vecVictimForward, NULL, NULL );
	vecVictimForward.z = 0.0f;
	vecVictimForward.NormalizeInPlace();

	// Get a vector from my origin to my targets origin
	Vector vecToTarget;
	vecToTarget = pTarget->WorldSpaceCenter() - GetOwner()->WorldSpaceCenter();
	vecToTarget.z = 0.0f;
	vecToTarget.NormalizeInPlace();

	float flDot = DotProduct( vecVictimForward, vecToTarget );

	return ( flDot > -0.1 );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFKnife::IsBehindAndFacingTarget( CBaseEntity *pTarget )
{
	CTFPlayer *pSpy = ToTFPlayer( this->GetPlayerOwner() );
	if (pSpy == nullptr) 
		return false;

	CTFPlayer *pVictim = ToTFPlayer( pTarget );

	Vector2D wsc_spy_to_victim = (pVictim->WorldSpaceCenter() - pSpy->WorldSpaceCenter()).AsVector2D();
	wsc_spy_to_victim.NormalizeInPlace();

	Vector temp1; pSpy->EyeVectors( &temp1 );
	Vector2D eye_spy = temp1.AsVector2D();
	eye_spy.NormalizeInPlace();

	Vector temp2; 
	pVictim->EyeVectors( &temp2 );
	Vector2D eye_victim = temp2.AsVector2D();
	eye_victim.NormalizeInPlace();

	if (DotProduct2D( wsc_spy_to_victim, eye_victim ) <= 0.0f) 
		return false;
	if (DotProduct2D( wsc_spy_to_victim, eye_spy ) <= 0.5f) 
		return false;
	if (DotProduct2D( eye_spy, eye_victim ) <= -0.3f) 
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFKnife::CalcIsAttackCriticalHelper( void )
{
	// Always crit from behind, never from front
	return ( m_iWeaponMode == TF_WEAPON_SECONDARY_MODE );
}

//-----------------------------------------------------------------------------
// Purpose: Allow melee weapons to send different anim events
// Input  :  - 
//-----------------------------------------------------------------------------
void CTFKnife::SendPlayerAnimEvent( CTFPlayer *pPlayer )
{
	if ( m_iWeaponMode == TF_WEAPON_SECONDARY_MODE )
	{
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_CUSTOM_GESTURE, ACT_MP_ATTACK_STAND_SECONDARYFIRE );
	}
	else
	{
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFKnife::DoViewModelAnimation( void )
{
	Activity act;
	if (m_bDelayedStab)
		act = (m_iWeaponMode == TF_WEAPON_PRIMARY_MODE) ? ACT_VM_HITCENTER : ACT_VM_SWINGHARD_SPECIAL;
	else
		act = (m_iWeaponMode == TF_WEAPON_PRIMARY_MODE) ? ACT_VM_HITCENTER : ACT_VM_SWINGHARD;	

	SendWeaponAnim( act );
}

//-----------------------------------------------------------------------------
// Purpose: Change idle anim to raised if we're ready to backstab.
//-----------------------------------------------------------------------------
bool CTFKnife::SendWeaponAnim( int iActivity )
{
	if ( m_bReadyToBackstab && iActivity == ACT_VM_IDLE )
	{
		return BaseClass::SendWeaponAnim( ACT_BACKSTAB_VM_IDLE );
	}

	return BaseClass::SendWeaponAnim( iActivity );
}

//-----------------------------------------------------------------------------
// Purpose: Check for knife raise conditions.
//-----------------------------------------------------------------------------
void CTFKnife::BackstabVMThink( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

	if ( GetActivity() == ACT_VM_IDLE || GetActivity() == ACT_BACKSTAB_VM_IDLE )
	{
		trace_t tr;
		if ( CanAttack() && DoSwingTrace( tr ) &&
			tr.m_pEnt->IsPlayer() && (tr.m_pEnt->GetTeamNumber() != pOwner->GetTeamNumber() || TFGameRules()->FriendlyFireMode()) &&
			IsBehindAndFacingTarget( tr.m_pEnt ) )
		{
			if ( !m_bReadyToBackstab )
			{
				m_bReadyToBackstab = true;
				SendWeaponAnim( ACT_BACKSTAB_VM_UP );
			}
		}
		else
		{
			if ( m_bReadyToBackstab )
			{
				m_bReadyToBackstab = false;
				SendWeaponAnim( ACT_BACKSTAB_VM_DOWN );
			}
		}
	}
}