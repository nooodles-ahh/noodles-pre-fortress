//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF Base Grenade.
//
//=============================================================================//
#include "cbase.h"
#include "tf_weaponbase.h"
#include "tf_weaponbase_grenade.h"
#include "tf_gamerules.h"
#include "npcevent.h"
#include "engine/IEngineSound.h"
#include "in_buttons.h"	
#include "tf_weaponbase_grenadeproj.h"
#include "eventlist.h"
#include "pf_cvars.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#include "items.h"
#include "soundent.h"
#endif

#define GRENADE_TIMER	1.5f			// seconds
#define GRENADE_THROW_SOUND		"Weapon_Grenade_Normal.Single"

//=============================================================================
//
// TF Grenade tables.
//

#if defined (CLIENT_DLL)
ConVar pf_grenade_press_throw( "pf_grenade_press_throw", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE | FCVAR_USERINFO, "Causes grenades to require a second button press to throw." );
#else // GAME_DLL
ConVar pf_grenades_throw_mode("pf_grenades_throw_mode", "1", FCVAR_GAMEDLL | FCVAR_NOTIFY );
ConVar pf_grenades_throw_center("pf_grenades_throw_center", "0", FCVAR_GAMEDLL | FCVAR_NOTIFY );
#endif


IMPLEMENT_NETWORKCLASS_ALIASED( TFWeaponBaseGrenade, DT_TFWeaponBaseGrenade )

BEGIN_NETWORK_TABLE( CTFWeaponBaseGrenade, DT_TFWeaponBaseGrenade )
// Client specific.
#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO( m_bPrimed ) ),
	RecvPropFloat( RECVINFO( m_flThrowTime ) ),
	RecvPropBool( RECVINFO( m_bThrow ) ),
// Server specific.
#else
	SendPropBool( SENDINFO( m_bPrimed ) ),
	SendPropTime( SENDINFO( m_flThrowTime ) ),
	SendPropBool( SENDINFO( m_bThrow ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFWeaponBaseGrenade )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_bPrimed, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flThrowTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bThrow, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weaponbase_grenade, CTFWeaponBaseGrenade );

//=============================================================================
//
// TF Grenade functions.
//

CTFWeaponBaseGrenade::CTFWeaponBaseGrenade()
{
}

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
void CTFWeaponBaseGrenade::Spawn( void )
{
	BaseClass::Spawn();

	SetViewModelIndex( TF_VM_OFFHAND );

#ifdef GAME_DLL
	RegisterThinkContext("BeepThink");
	SetContextThink(&CTFWeaponBaseGrenade::BeepThink, gpGlobals->curtime, "BeepThink");
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBaseGrenade::Precache()
{
	PrecacheScriptSound("Weapon_Grenade.Beep");
	PrecacheScriptSound("Weapon_Grenade.FinalBeep");
	PrecacheScriptSound( GRENADE_THROW_SOUND );
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBaseGrenade::IsPrimed( void )
{
	return m_bPrimed;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBaseGrenade::WeaponReset()
{
	m_bPrimed = false;
	m_bThrow = false;
	BaseClass::WeaponReset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBaseGrenade::Prime() 
{
	CTFWeaponInfo weaponInfo = GetTFWpnData();
	m_flThrowTime = gpGlobals->curtime + weaponInfo.m_flPrimerTime;
	m_flNextPrimaryAttack = gpGlobals->curtime + 0.8f;

#ifndef CLIENT_DLL
	// Don't want the smokebomb to beep
	if ( !m_bPrimed && GetWeaponID() != TF_WEAPON_GRENADE_SMOKE_BOMB )
	{		
		SetNextThink(gpGlobals->curtime + 0.2, "BeepThink"); // Start thinking now
	}

	// Get the player owning the weapon.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if( !pPlayer )
		return;

	if( GetWeaponID() != TF_WEAPON_GRENADE_CALTROP )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "grenade_primed" );
		if( event )
		{
			event->SetInt( "index", this->entindex() );
			event->SetFloat( "time", weaponInfo.m_flPrimerTime );
			event->SetInt( "player", pPlayer->entindex() );

			gameeventmanager->FireEvent( event );
		}
	}

	pPlayer->RemoveInvisibility();
#endif

	m_bPrimed = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBaseGrenade::Throw() 
{
	if ( !m_bPrimed )
		return;

	m_bPrimed = false;
	m_bThrow = false;

	// Get the owning player.
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if ( !pPlayer )
		return;

#ifdef GAME_DLL
	// Calculate the time remaining.
	float flTime = m_flThrowTime - gpGlobals->curtime;
	bool bExplodingInHand = ( flTime <= 0.0f );

	// Players who are dying may not have their death state set, so check that too
	bool bExplodingOnDeath = ( !pPlayer->IsAlive() || pPlayer->StateGet() == TF_STATE_DYING );

	Vector vecSrc, vecThrow;
	QAngle angThrow = pPlayer->LocalEyeAngles() + pPlayer->ConcAngles();

	// Don't throw if we've changed team
	if ( bExplodingOnDeath && GetTeamNumber() != pPlayer->GetTeamNumber() )
		return;

	Vector vForward, vRight, vUp;
	if ( bExplodingInHand || bExplodingOnDeath || pPlayer->m_Shared.IsLoser() )
	{
		vecThrow = vec3_origin;
		// Don't use WorldSpaceCenter
		AngleVectors( angThrow, &vForward, &vRight, &vUp );
		Vector tall = Vector(0, 0, pPlayer->CollisionProp()->CollisionSpaceMaxs().z);
		if( pPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_SCOUT )
		{
			vecSrc = pPlayer->GetAbsOrigin() + (tall * 0.6);
			vecSrc += vForward * -6.0f;
		}
		else
		{
			vecSrc = pPlayer->GetAbsOrigin() + (tall * 0.5);
			vecSrc += vForward * -4.0f;
		}
			
	}
	else
	{
		vecSrc = pPlayer->Weapon_ShootPosition();

		if(!pf_grenades_throw_mode.GetBool())
		{
			// Determine the throw angle and velocity.
			if ( angThrow.x < 90.0f )
			{
				angThrow.x = -10.0f + angThrow.x * ( ( 90.0f + 10.0f ) / 90.0f );
			}
			else
			{
				angThrow.x = 360.0f - angThrow.x;
				angThrow.x = -10.0f + angThrow.x * -( ( 90.0f - 10.0f ) / 90.0f );
			}

			// Adjust for the lowering of the spawn point
			angThrow.x -= 10;
		}
		else
		{
			// Adjust for the lowering of the spawn point
			angThrow.x -= 5;
		}
		

		float flVelocity = pf_grenades_throw_mode.GetBool() ? ( 90.0f - angThrow.x ) * 12.0f : ( 90.0f - angThrow.x ) * 8.0f;
		if ( flVelocity > 950.0f )
		{
			flVelocity = 950.0f;
		}

		AngleVectors(angThrow, &vForward, &vRight, &vUp);

		float flRightOffset = -8.0f;
		if( pPlayer->IsViewModelFlipped() )
		{
			flRightOffset *= -1.0f;
		}

		vecSrc += vForward * 16.0f + vRight * flRightOffset + vUp * -6.0f;

		trace_t tr;
		CTraceFilterIgnoreTeammates traceFilter( pPlayer, COLLISION_GROUP_NONE, pPlayer->GetTeamNumber() );
		UTIL_TraceHull( pPlayer->Weapon_ShootPosition(), vecSrc, Vector( -8, -8, -8 ), Vector( 8, 8, 8 ), MASK_SOLID, &traceFilter, &tr );
		vecSrc = tr.endpos;

		vecThrow = vForward * flVelocity;
	}

#if 0
	// Debug!!!
	char str[256];
	Q_snprintf( str, sizeof( str ),"GrenadeTime = %f\n", flTime );
	NDebugOverlay::ScreenText( 0.5f, 0.38f, str, 255, 255, 255, 255, 2.0f );
#endif

	QAngle vecAngles = QAngle(0, UTIL_AngleMod(angThrow.y + 90), 0);
	CTFWeaponBaseGrenadeProj* pGrenade = EmitGrenade(vecSrc, vecAngles, vecThrow, AngularImpulse(600, 0, 0), pPlayer, bExplodingInHand ? 0.0 : flTime);
	// Create the projectile and send in the time remaining.
	if( pGrenade )
	{
		if ( !bExplodingInHand )
		{
			pGrenade->SetContextThink(&CTFWeaponBaseGrenadeProj::BeepThink, GetNextThink("BeepThink"), "BeepThink");
			IGameEvent* event = gameeventmanager->CreateEvent( "grenade_thrown" );
			if ( event )
			{
				event->SetInt( "oldindex", this->entindex() );
				event->SetInt( "newindex", pGrenade->entindex() );
				event->SetInt( "player", pPlayer->entindex() );

				gameeventmanager->FireEvent( event );
			}
		}
		else
		{
			// We're holding onto an exploding grenade
			pGrenade->ExplodeInHand(GetTFPlayerOwner());
		}
	}

	if (!pf_infinite_ammo.GetBool() && !pf_grenades_infinite.GetBool())
	{
		if(TFGameRules()->GrenadesRegenerate())
		{
			TFPlayerClassData_t *pData = GetPlayerClassData(pPlayer->GetPlayerClass()->GetClassIndex());
			for (int i = 0; i < TF_PLAYER_GRENADE_COUNT; i++)
			{
				if(pData->m_aGrenades[i] == GetWeaponID())
				{
					pPlayer->m_Shared.StartGrenadeReneration(i);
					break;
				}
			}
		}
		else
		{
			pPlayer->RemoveAmmo( 1, GetPrimaryAmmoType() );
		}
	}
		
	
	// The grenade is about to be destroyed, so it won't be able to holster.
	// Handle the viewmodel hiding for it.
	if ( bExplodingInHand || bExplodingOnDeath )
	{
		SetWeaponVisible( false );
		CBaseViewModel *vm = pPlayer->GetViewModel( TF_VM_OFFHAND );
		if( vm )
			vm->AddEffects( EF_NODRAW );
	}
#endif

	// Reset the throw time
	m_flThrowTime = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBaseGrenade::ShouldDetonate( void )
{
	return (m_flThrowTime != 0.0f) && (m_flThrowTime < gpGlobals->curtime);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBaseGrenade::ItemPostFrame()
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if( !pPlayer )
		return;

	if( m_bPrimed )
	{
		// Is our timer up? If so, blow up immediately
		if( ShouldDetonate() || pPlayer->m_Shared.IsLoser() )
		{
			Throw();
			return;
		}

		if( m_flNextPrimaryAttack > gpGlobals->curtime )
			return;

		bool bThrow = false;
		if( pPlayer->GetGrenadePressThrow() )
		{
			if( (pPlayer->m_afButtonPressed & IN_GRENADE1 || pPlayer->m_afButtonPressed & IN_GRENADE2) )
				bThrow = true;
		}
		else
		{
			if( !(pPlayer->m_nButtons & IN_GRENADE1 || pPlayer->m_nButtons & IN_GRENADE2) )
				bThrow = true;
		}

		if( !m_bThrow && bThrow )
		{
			m_bThrow = true;
		}
	}

	if( m_bThrow )
	{
		// Start throwing
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_GRENADE );
		SendWeaponAnim( ACT_VM_PRIMARYATTACK );
		PoseParameterOverride( true );

		Throw();
		return;
	}
	
	if( !m_bPrimed && !m_bThrow )
	{
		// Once we've finished being holstered, we'll be hidden. When that happens,
		// tell our player that we're all done with the grenade throw.
		if( IsEffectActive( EF_NODRAW ) )
		{
			pPlayer->FinishThrowGrenade();
			return;
		}

		// We've been thrown. Go away.
		if( HasWeaponIdleTimeElapsed() )
		{
			Holster();
			// Cyanide; just incase
			SetWeaponVisible( false );
		}
		return;
	}

	// Go straight to idle anim when deploy is done
	if( m_flTimeWeaponIdle <= gpGlobals->curtime )
	{
		SendWeaponAnim( ACT_VM_IDLE );
	}
}

bool CTFWeaponBaseGrenade::ShouldLowerMainWeapon( void )
{
	return GetTFWpnData().m_bLowerWeapon;
}

//=============================================================================
//
// Client specific functions.
//
#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBaseGrenade::ShouldDraw( void )
{
	if( !BaseClass::ShouldDraw() )
	{
		// Grenades need to be visible whenever they're being primed & thrown
		if( !m_bPrimed )
			return false;

		// Don't draw primed grenades for local player in first person players
		if( GetOwner() == C_BasePlayer::GetLocalPlayer() && !C_BasePlayer::ShouldDrawLocalPlayer() )
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBaseGrenade::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	UpdateVisibility();
}

//=============================================================================
//
// Server specific functions.
//
#else

BEGIN_DATADESC( CTFWeaponBaseGrenade )
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBaseGrenade::HandleAnimEvent( animevent_t *pEvent )
{
	if ( (pEvent->type & AE_TYPE_NEWEVENTSYSTEM) )
	{
		if ( pEvent->event == AE_WPN_PRIMARYATTACK )
		{
			Throw();
			return;
		}
	}

	BaseClass::HandleAnimEvent( pEvent );
}

void CTFWeaponBaseGrenade::BeepThink( void )
{
	if( IsPrimed() )
	{
		if( m_flThrowTime - 1.0 > gpGlobals->curtime )
		{
			if( GetTFPlayerOwner() )
			{
				CPASAttenuationFilter filter( GetAbsOrigin() );
				EmitSound( filter, entindex(), "Weapon_Grenade.Beep" );
			}
			SetNextThink( gpGlobals->curtime + 1.0f, "BeepThink" );
		}
		else
		{
			CPASAttenuationFilter filter( GetAbsOrigin() );
			EmitSound( filter, entindex(), "Weapon_Grenade.FinalBeep" );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFWeaponBaseGrenadeProj *CTFWeaponBaseGrenade::EmitGrenade( Vector vecSrc, QAngle vecAngles, Vector vecVel, AngularImpulse angImpulse, CBasePlayer *pPlayer, float flTime, int iFlags )
{
	Assert( 0 && "CBaseCSGrenade::EmitGrenade should not be called. Make sure to implement this in your subclass!\n" );
	return NULL;
}

#endif

