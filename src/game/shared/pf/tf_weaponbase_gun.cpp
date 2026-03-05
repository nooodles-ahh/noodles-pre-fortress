//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: Weapon Base Gun 
//
//=============================================================================

#include "cbase.h"
#include "tf_weaponbase_gun.h"
#include "tf_fx_shared.h"
#include "effect_dispatch_data.h"
#include "takedamageinfo.h"
#include "tf_projectile_nail.h"

#if !defined( CLIENT_DLL )	// Server specific.

	#include "tf_gamestats.h"
	#include "tf_player.h"
	#include "tf_fx.h"
	#include "te_effect_dispatch.h"

	#include "tf_projectile_rocket.h"
	#include "tf_weapon_grenade_pipebomb.h"
	#include "te.h"

#else	// Client specific.

	#include "c_tf_player.h"
	#include "c_te_effect_dispatch.h"

#endif

//=============================================================================
//
// TFWeaponBase Gun tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFWeaponBaseGun, DT_TFWeaponBaseGun )

BEGIN_NETWORK_TABLE( CTFWeaponBaseGun, DT_TFWeaponBaseGun )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFWeaponBaseGun )
END_PREDICTION_DATA()

// Server specific.
#if !defined( CLIENT_DLL ) 
BEGIN_DATADESC( CTFWeaponBaseGun )
DEFINE_THINKFUNC( ZoomOutIn ),
DEFINE_THINKFUNC( ZoomOut ),
DEFINE_THINKFUNC( ZoomIn ),
END_DATADESC()
#endif

//=============================================================================
//
// TFWeaponBase Gun functions.
//

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CTFWeaponBaseGun::CTFWeaponBaseGun()
{
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::PrimaryAttack( void )
{
	// Check for ammunition.
	if ( m_iClip1 <= 0 && m_iClip1 != -1 )
		return;

	// Are we capable of firing again?
	if ( m_flNextPrimaryAttack > gpGlobals->curtime )
		return;

	// Get the player owning the weapon.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	if ( !CanAttack() )
		return;

	CalcIsAttackCritical();

#ifndef CLIENT_DLL
	pPlayer->RemoveInvisibility();
	pPlayer->RemoveDisguise();

	// Minigun has custom handling
	if ( GetWeaponID() != TF_WEAPON_MINIGUN )
	{
		pPlayer->SpeakWeaponFire();
	}
	CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, IsCurrentAttackACrit() );
#endif

	// Set the weapon mode.
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

#ifdef PF2
	// check if the model has a dryfire animation and if clip is 1, else send normal primary attack
	if ( LookupActivity( "ACT_VM_DRYFIRE" ) != -1 && m_iClip1 <= 1)
	{
		SendWeaponAnim( ACT_VM_DRYFIRE );
	}
	else
#endif
	{
		SendWeaponAnim( ACT_VM_PRIMARYATTACK );
	}

	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	FireProjectile( pPlayer );

	// Set next attack times.
	m_flNextPrimaryAttack = gpGlobals->curtime + m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay;

	// Don't push out secondary attack, because our secondary fire
	// systems are all separate from primary fire (sniper zooming, demoman pipebomb detonating, etc)
	//m_flNextSecondaryAttack = gpGlobals->curtime + m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay;

	// Set the idle animation times based on the sequence duration, so that we play full fire animations
	// that last longer than the refire rate may allow.
	if ( Clip1() > 0 )
	{
		SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration() );
	}
	else
	{
		SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration() );
	}

	// Check the reload mode and behave appropriately.
	if ( m_bReloadsSingly )
	{
		m_iReloadMode.Set( TF_RELOAD_START );
	}
}	

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::SecondaryAttack( void )
{
	// semi-auto behaviour
	if ( m_bInAttack2 )
		return;

	// Get the player owning the weapon.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	pPlayer->DoClassSpecialSkill();

	m_bInAttack2 = true;

	m_flNextSecondaryAttack = gpGlobals->curtime + 0.5;
}

#ifdef PF2
CBaseEntity *CTFWeaponBaseGun::FireProjectile( CTFPlayer *pPlayer, bool consumeAmmo )
#else
CBaseEntity *CTFWeaponBaseGun::FireProjectile( CTFPlayer *pPlayer )
#endif
{
	int iProjectile = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_iProjectile;

	CBaseEntity *pProjectile = NULL;

	switch( iProjectile )
	{
	case TF_PROJECTILE_BULLET:
		FireBullet( pPlayer );
		break;

	case TF_PROJECTILE_ROCKET:
#ifdef PF2
	case TF_PROJECTILE_FLAME_ROCKET:
		pProjectile = FireRocket( pPlayer, iProjectile );
#else
		pProjectile = FireRocket( pPlayer );
#endif
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
		break;

	case TF_PROJECTILE_SYRINGE:
#ifdef PF2
	case TF_PROJECTILE_NAIL:
	case TF_PROJECTILE_DART:
		FireNail( pPlayer, iProjectile );
#else
		pProjectile = FireNail( pPlayer, iProjectile );
#endif
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
		break;

	case TF_PROJECTILE_PIPEBOMB:
		pProjectile = FirePipeBomb( pPlayer, false );
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
		break;

	case TF_PROJECTILE_PIPEBOMB_REMOTE:
		pProjectile = FirePipeBomb( pPlayer, true );
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
		break;

	case TF_PROJECTILE_NONE:
	default:
		// do nothing!
		DevMsg( "Weapon does not have a projectile type set\n" );
		break;
	}
#ifdef PF2
	if (consumeAmmo && !pf_infinite_ammo.GetBool())
#endif
	{
		if ( m_iClip1 != -1 )
		{
			m_iClip1 -= m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_iAmmoPerShot;
		}
		else
		{
			if ( m_iWeaponMode == TF_WEAPON_PRIMARY_MODE )
			{
				pPlayer->RemoveAmmo( m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_iAmmoPerShot, m_iPrimaryAmmoType );
			}
			else
			{
				pPlayer->RemoveAmmo( m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_iAmmoPerShot, m_iSecondaryAmmoType );
			}
		}
	}

	DoFireEffects();

	UpdatePunchAngles( pPlayer );

	return pProjectile;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::UpdatePunchAngles( CTFPlayer *pPlayer )
{
	// Update the player's punch angle.
	QAngle angle = pPlayer->GetPunchAngle();
	float flPunchAngle = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flPunchAngle;

	if ( flPunchAngle > 0 )
	{
		angle.x -= SharedRandomInt( "ShotgunPunchAngle", ( flPunchAngle - 1 ), ( flPunchAngle + 1 ) );
		pPlayer->SetPunchAngle( angle );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Fire a bullet!
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::FireBullet( CTFPlayer *pPlayer )
{
	PlayWeaponShootSound();

	FX_FireBullets(
		pPlayer->entindex(),
		pPlayer->Weapon_ShootPosition(),
#ifdef PF2
		pPlayer->EyeAngles() + pPlayer->GetPunchAngle() + pPlayer->ConcAngles(),
#else
		pPlayer->EyeAngles() + pPlayer->GetPunchAngle(),
#endif
		GetWeaponID(),
		m_iWeaponMode,
		CBaseEntity::GetPredictionRandomSeed() & 255,
		GetWeaponSpread(),
		GetProjectileDamage(),
		IsCurrentAttackACrit());
}

#ifndef PF2
class CTraceFilterIgnoreTeammates : public CTraceFilterSimple
{
public:
	// It does have a base, but we'll never network anything below here..
	DECLARE_CLASS( CTraceFilterIgnoreTeammates, CTraceFilterSimple );

	CTraceFilterIgnoreTeammates( const IHandleEntity *passentity, int collisionGroup, int iIgnoreTeam )
		: CTraceFilterSimple( passentity, collisionGroup ), m_iIgnoreTeam( iIgnoreTeam )
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{
		CBaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );

		if ( pEntity->IsPlayer() && pEntity->GetTeamNumber() == m_iIgnoreTeam )
		{
			return false;
		}

		return true;
	}

	int m_iIgnoreTeam;
};
#endif

//-----------------------------------------------------------------------------
// Purpose: Return the origin & angles for a projectile fired from the player's gun
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::GetProjectileFireSetup( CTFPlayer *pPlayer, Vector vecOffset, Vector *vecSrc, QAngle *angForward, bool bHitTeammates /* = true */ )
{
	Vector vecForward, vecRight, vecUp;
#ifdef PF2
	AngleVectors( pPlayer->EyeAngles() + pPlayer->ConcAngles(), &vecForward, &vecRight, &vecUp );
#else
	AngleVectors( pPlayer->EyeAngles(), &vecForward, &vecRight, &vecUp );
#endif
	Vector vecShootPos = pPlayer->Weapon_ShootPosition();

	// Estimate end point
	Vector endPos = vecShootPos + vecForward * 2000;	

	// Trace forward and find what's in front of us, and aim at that
	trace_t tr;

	if ( bHitTeammates )
	{
		CTraceFilterSimple filter( pPlayer, COLLISION_GROUP_NONE );
		UTIL_TraceLine( vecShootPos, endPos, MASK_SOLID, &filter, &tr );
	}
	else
	{
		CTraceFilterIgnoreTeammates filter( pPlayer, COLLISION_GROUP_NONE, pPlayer->GetTeamNumber() );
		UTIL_TraceLine( vecShootPos, endPos, MASK_SOLID, &filter, &tr );
	}

	// Offset actual start point
	float flRightOffset = vecOffset.y;
	if( pPlayer->IsViewModelFlipped() )
	{
		flRightOffset *= -1.0f;
	}
	Vector vecStart = vecShootPos + (vecForward * vecOffset.x) + (vecRight * flRightOffset) + (vecUp * vecOffset.z);

	// Find angles that will get us to our desired end point
	// Only use the trace end if it wasn't too close, which results
	// in visually bizarre forward angles
	if ( tr.fraction > 0.1 )
	{
		VectorAngles( tr.endpos - vecStart, *angForward );
	}
	else
	{
		VectorAngles( endPos - vecStart, *angForward );
	}

	trace_t trace;
	CTraceFilterSimple traceFilter( this, COLLISION_GROUP_NONE );
	UTIL_TraceLine( vecShootPos, vecStart, MASK_SOLID, &traceFilter, &trace );
	*vecSrc = trace.endpos;
}

//-----------------------------------------------------------------------------
// Purpose: Fire a rocket
//-----------------------------------------------------------------------------
#ifdef PF2
CBaseEntity *CTFWeaponBaseGun::FireRocket( CTFPlayer *pPlayer, int iType )
#else
CBaseEntity *CTFWeaponBaseGun::FireRocket( CTFPlayer *pPlayer )
#endif
{
	PlayWeaponShootSound();

	// Server only - create the rocket.
#ifdef GAME_DLL

	Vector vecSrc;
	QAngle angForward;
	Vector vecOffset( 23.5f, 12.0f, -3.0f );
	if ( pPlayer->GetFlags() & FL_DUCKING )
	{
		vecOffset.z = 8.0f;
	}
	GetProjectileFireSetup( pPlayer, vecOffset, &vecSrc, &angForward, false );

#ifdef PF2_DLL
	float flProjectileSpeed = m_pWeaponInfo->GetWeaponData(m_iWeaponMode).m_flProjectileSpeed;
	float flProjectileGravity = m_pWeaponInfo->GetWeaponData(m_iWeaponMode).m_flProjectileGravity;

	CTFProjectile_Rocket *pProjectile = nullptr;
	switch( iType )
	{
		case TF_PROJECTILE_ROCKET:
			pProjectile = CTFProjectile_Rocket::Create( vecSrc, angForward, pPlayer, pPlayer, flProjectileSpeed );
			break;
		case TF_PROJECTILE_FLAME_ROCKET:
			pProjectile = CTFFlameRocket::Create( vecSrc, angForward, pPlayer, pPlayer, flProjectileSpeed );
			break;
	}
#else
	CTFProjectile_Rocket *pProjectile = CTFProjectile_Rocket::Create( vecSrc, angForward, pPlayer, pPlayer );
#endif
	if ( pProjectile )
	{
		pProjectile->SetCritical( IsCurrentAttackACrit() );
		pProjectile->SetDamage( GetProjectileDamage() );
#ifdef PF2_DLL
		pProjectile->SetGravity( flProjectileGravity );
#endif
	}
	return pProjectile;

#endif

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Fire a projectile nail
//-----------------------------------------------------------------------------
#ifdef PF2
void CTFWeaponBaseGun::FireNail( CTFPlayer *pPlayer, int iSpecificNail )
#else
CBaseEntity *CTFWeaponBaseGun::FireNail( CTFPlayer *pPlayer, int iSpecificNail )
#endif
{
	PlayWeaponShootSound();

#ifdef PF2
	int nBulletsPerShot = m_pWeaponInfo->GetWeaponData(m_iWeaponMode).m_nBulletsPerShot;
	float flProjectileSpeed = m_pWeaponInfo->GetWeaponData(m_iWeaponMode).m_flProjectileSpeed;
	float flProjectileGravity = m_pWeaponInfo->GetWeaponData(m_iWeaponMode).m_flProjectileGravity;
	for (int iBullet = 0; iBullet < nBulletsPerShot; ++iBullet)
#endif
	{
		Vector vecSrc;
		QAngle angForward;
		GetProjectileFireSetup( pPlayer, Vector(16,6,-8), &vecSrc, &angForward );
		
		// Add some spread
#ifdef PF2
		// not for the tranq because that's shitty
		if ( iSpecificNail != TF_PROJECTILE_DART )
#endif
		{ 
			angForward.x += RandomFloat(RAD2DEG(-GetWeaponSpread()), RAD2DEG(GetWeaponSpread()));
			angForward.y += RandomFloat(RAD2DEG(-GetWeaponSpread()), RAD2DEG(GetWeaponSpread()));
		}
		
		CTFBaseProjectile *pProjectile = NULL;
		switch( iSpecificNail )
		{
#ifdef PF2
		case TF_PROJECTILE_SYRINGE:
			pProjectile = CTFProjectile_Syringe::Create( vecSrc, angForward, pPlayer, pPlayer, IsCurrentAttackACrit(), flProjectileSpeed, flProjectileGravity );
			break;

		case TF_PROJECTILE_NAIL:
			pProjectile = CTFProjectile_Nail::Create(vecSrc, angForward, pPlayer, pPlayer, IsCurrentAttackACrit(), flProjectileSpeed, flProjectileGravity);
			break;

		case TF_PROJECTILE_DART:
			pProjectile = CTFProjectile_Dart::Create(vecSrc, angForward, pPlayer, pPlayer, IsCurrentAttackACrit(), flProjectileSpeed, flProjectileGravity);
			break;
#else
		case TF_PROJECTILE_SYRINGE:
			pProjectile = CTFProjectile_Syringe::Create( vecSrc, angForward, pPlayer, pPlayer, IsCurrentAttackACrit() );
			break;
#endif
		
		default:
			Assert(0);
		}

		if ( pProjectile )
		{
			pProjectile->SetWeaponID( GetWeaponID() );
			pProjectile->SetCritical( IsCurrentAttackACrit() );
#ifdef GAME_DLL
			pProjectile->SetDamage( GetProjectileDamage() );
#endif
		}

#ifndef PF2
		return pProjectile;
#endif
	}
}

//-----------------------------------------------------------------------------
// Purpose: Fire a  pipe bomb
//-----------------------------------------------------------------------------
CBaseEntity *CTFWeaponBaseGun::FirePipeBomb( CTFPlayer *pPlayer, bool bRemoteDetonate )
{
	PlayWeaponShootSound();

#ifdef GAME_DLL

	Vector vecForward, vecRight, vecUp;
	AngleVectors( pPlayer->EyeAngles() + pPlayer->ConcAngles(), &vecForward, &vecRight, &vecUp );

	// Create grenades here!!
	Vector vecSrc = pPlayer->Weapon_ShootPosition();

	// Offset actual start point
	float flRightOffset = 8.0f;
	if( pPlayer->IsViewModelFlipped() )
	{
		flRightOffset *= -1.0f;
	}
	vecSrc += vecForward * 16.0f + vecRight * flRightOffset + vecUp * -6.0f;
	
	Vector vecVelocity = ( vecForward * GetProjectileSpeed() ) + ( vecUp * 200.0f ) + ( random->RandomFloat( -10.0f, 10.0f ) * vecRight ) +		
		( random->RandomFloat( -10.0f, 10.0f ) * vecUp );
	
	trace_t tr;
	CTraceFilterIgnoreTeammates traceFilter( pPlayer, COLLISION_GROUP_NONE, pPlayer->GetTeamNumber() );
	UTIL_TraceHull( pPlayer->Weapon_ShootPosition(), vecSrc, Vector( -8, -8, -8 ), Vector( 8, 8, 8 ), MASK_SOLID, &traceFilter, &tr );

	CTFGrenadePipebombProjectile *pProjectile = CTFGrenadePipebombProjectile::Create( tr.endpos, pPlayer->EyeAngles() + pPlayer->ConcAngles(), vecVelocity,
		AngularImpulse( 600, random->RandomInt( -1200, 1200 ), 0 ),
		pPlayer, GetTFWpnData(), bRemoteDetonate );


	if ( pProjectile )
	{
		pProjectile->SetCritical( IsCurrentAttackACrit() );
	}
	return pProjectile;

#endif

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::PlayWeaponShootSound( void )
{
	if ( IsCurrentAttackACrit() )
	{
		WeaponSound( BURST );
	}
	else
	{
		WeaponSound( SINGLE );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Accessor for damage, so sniper etc can modify damage
//-----------------------------------------------------------------------------
float CTFWeaponBaseGun::GetProjectileSpeed( void )
{
	// placeholder for now
	// grenade launcher and pipebomb launcher hook this to set variable pipebomb speed

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFWeaponBaseGun::GetWeaponSpread( void )
{
	return m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flSpread;
}

//-----------------------------------------------------------------------------
// Purpose: Accessor for damage, so sniper etc can modify damage
//-----------------------------------------------------------------------------
float CTFWeaponBaseGun::GetProjectileDamage( void )
{
	return (float)m_pWeaponInfo->GetWeaponDamage(m_iWeaponMode);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFWeaponBaseGun::Holster( CBaseCombatWeapon *pSwitchingTo )
{
// Server specific.
#if !defined( CLIENT_DLL )

	// Make sure to zoom out before we holster the weapon.
	ZoomOut();
	SetContextThink( NULL, 0, ZOOM_CONTEXT );

#endif

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose:
// NOTE: Should this be put into fire gun
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::DoFireEffects()
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	// Muzzle flash on weapon.
	bool bMuzzleFlash = true;
	CTFWeaponBase *pWeapon = pPlayer->GetActiveTFWeapon();
	if ( pWeapon && pWeapon->GetWeaponID() == TF_WEAPON_MINIGUN )
	{
		bMuzzleFlash = false;
	}

	if ( bMuzzleFlash )
	{
		
		pPlayer->DoMuzzleFlash();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::ToggleZoom( void )
{
	// Toggle the zoom.
	CBasePlayer *pPlayer = GetPlayerOwner();
	if ( pPlayer )
	{
		if( pPlayer->GetFOV() >= 75 )
		{
			ZoomIn();
		}
		else
		{
			ZoomOut();
		}
	}

	// Get the zoom animation time.
	m_flNextSecondaryAttack = gpGlobals->curtime + 1.2;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::ZoomIn( void )
{
	// The the owning player.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	// Set the weapon zoom.
	// TODO: The weapon fov should be gotten from the script file.
	pPlayer->SetFOV( pPlayer, TF_WEAPON_ZOOM_FOV, 0.1f );
	pPlayer->m_Shared.AddCond( TF_COND_ZOOMED );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::ZoomOut( void )
{
	// The the owning player.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	if ( pPlayer->m_Shared.InCond( TF_COND_ZOOMED ) )
	{
		// Set the FOV to 0 set the default FOV.
		pPlayer->SetFOV( pPlayer, 0, 0.1f );
		pPlayer->m_Shared.RemoveCond( TF_COND_ZOOMED );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBaseGun::ZoomOutIn( void )
{
	//Zoom out, set think to zoom back in.
	ZoomOut();
	SetContextThink( &CTFWeaponBaseGun::ZoomIn, gpGlobals->curtime + ZOOM_REZOOM_TIME, ZOOM_CONTEXT );
}
