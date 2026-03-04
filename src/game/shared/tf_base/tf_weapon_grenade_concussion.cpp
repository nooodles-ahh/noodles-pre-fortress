//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF Concussion Grenade.
//
//=============================================================================//
#include "cbase.h"
#include "tf_weaponbase.h"
#include "tf_gamerules.h"
#include "npcevent.h"
#include "engine/IEngineSound.h"
#include "tf_weapon_grenade_concussion.h"

// Server specific.
#ifdef GAME_DLL
#include "tf_player.h"
#include "items.h"
#include "tf_weaponbase_grenadeproj.h"
#include "soundent.h"
#include "KeyValues.h"
#include "particle_parse.h"
#include "te_effect_dispatch.h"
#else
#include "c_te_effect_dispatch.h"
#include "ragdollexplosionenumerator.h"
#endif

#define GRENADE_CONCUSSION_TIMER	3.0f			// seconds

//=============================================================================
//
// TF Concussion Grenade tables.
//

IMPLEMENT_NETWORKCLASS_ALIASED( TFGrenadeConcussion, DT_TFGrenadeConcussion )

BEGIN_NETWORK_TABLE( CTFGrenadeConcussion, DT_TFGrenadeConcussion )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFGrenadeConcussion )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_grenade_concussion, CTFGrenadeConcussion );
PRECACHE_WEAPON_REGISTER( tf_weapon_grenade_concussion );

IMPLEMENT_NETWORKCLASS_ALIASED(TFGrenadeConcussionProjectile, DT_TFGrenadeConcussionProjectile)

BEGIN_NETWORK_TABLE(CTFGrenadeConcussionProjectile, DT_TFGrenadeConcussionProjectile)
END_NETWORK_TABLE()

//=============================================================================
//
// TF Normal Grenade functions.
//

CTFGrenadeConcussionProjectile::CTFGrenadeConcussionProjectile()
{
}
CTFGrenadeConcussionProjectile::~CTFGrenadeConcussionProjectile()
{
#ifdef CLIENT_DLL
	ParticleProp()->StopEmission();
#endif
}

//=============================================================================
//
// TF Concussion Grenade functions.
//

// Server specific.
#ifdef GAME_DLL

BEGIN_DATADESC( CTFGrenadeConcussion )
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFWeaponBaseGrenadeProj *CTFGrenadeConcussion::EmitGrenade( Vector vecSrc, QAngle vecAngles, Vector vecVel, 
							        AngularImpulse angImpulse, CBasePlayer *pPlayer, float flTime, int iflags )
{
	return CTFGrenadeConcussionProjectile::Create( vecSrc, vecAngles, vecVel, angImpulse, 
		                                pPlayer, GetTFWpnData(), flTime );
}

#endif

//=============================================================================
//
// TF Concussion Grenade Projectile functions (Server specific).
//
#ifdef GAME_DLL

#define GRENADE_MODEL "models/weapons/w_models/w_grenade_conc.mdl"
#define TF_CONC_RADIUS 150

LINK_ENTITY_TO_CLASS( tf_weapon_grenade_concussion_projectile, CTFGrenadeConcussionProjectile );
PRECACHE_WEAPON_REGISTER( tf_weapon_grenade_concussion_projectile );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFGrenadeConcussionProjectile* CTFGrenadeConcussionProjectile::Create( const Vector &position, const QAngle &angles, 
																const Vector &velocity, const AngularImpulse &angVelocity, 
																CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo, float timer, int iFlags )
{
	CTFGrenadeConcussionProjectile *pGrenade = static_cast<CTFGrenadeConcussionProjectile*>( CTFWeaponBaseGrenadeProj::Create( "tf_weapon_grenade_concussion_projectile", position, angles, velocity, angVelocity, pOwner, weaponInfo, timer, iFlags ) );
	if ( pGrenade )
	{
		pGrenade->ApplyLocalAngularVelocityImpulse( angVelocity );
		pGrenade->SetDamageRadius(TF_CONC_RADIUS);
	}

	return pGrenade;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeConcussionProjectile::Spawn()
{
	SetModel( GRENADE_MODEL );

	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeConcussionProjectile::Precache()
{
	PrecacheModel( GRENADE_MODEL );
	PrecacheParticleSystem("conc_warp");
	PrecacheScriptSound("Weapon_Grenade_Concussion.Explode");
	PrecacheScriptSound("Weapon_Grenade_Concussion.Bounce");

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeConcussionProjectile::BounceSound( void )
{
	EmitSound( "Weapon_Grenade_Concussion.Bounce" );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeConcussionProjectile::Detonate()
{
	if ( ShouldNotDetonate() )
	{
		RemoveGrenade();
		return;
	}

	// The trace start/end.
	Vector vecStart = GetAbsOrigin() + Vector( 0.0f, 0.0f, 8.0f );
	Vector vecEnd = vecStart + Vector( 0.0f, 0.0f, -32.0f );

	trace_t	trace;
	UTIL_TraceLine( vecStart, vecEnd, MASK_SHOT_HULL, this, COLLISION_GROUP_DEBRIS, &trace );

	// Explode (concuss).
	Explode( &trace, GetDamageType());

	// Screen shake.
	if ( GetShakeAmplitude() )
	{
		UTIL_ScreenShake( GetAbsOrigin(), GetShakeAmplitude(), 150.0, 1.0, GetShakeRadius(), SHAKE_START );
	}
}

extern ConVar tf_grenade_show_radius;

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeConcussionProjectile::Explode( trace_t *pTrace, int bitsDamageType )
{
	// Server specific.
#ifdef GAME_DLL

	// Invisible.
	SetModelName(NULL_STRING);
	AddSolidFlags(FSOLID_NOT_SOLID);
	m_takedamage = DAMAGE_NO;

	// Move the impact point away from the surface a little bit.
	if (pTrace->fraction != 1.0)
	{
		SetLocalOrigin(pTrace->endpos + (pTrace->plane.normal * 0.6));
	}

	// Explosion sound.
	Vector soundPosition = GetAbsOrigin() + Vector(0, 0, 5);
	CPASAttenuationFilter filter(soundPosition);

	EmitSound(filter, entindex(), "Weapon_Grenade_Concussion.Explode");

	// Explosion damage, using the thrower's position as the report position.
	CTFPlayer* pThrower = ToTFPlayer(GetThrower());
	Vector vecReported = pThrower ? pThrower->GetAbsOrigin() : vec3_origin;
	CTakeDamageInfo info( this, pThrower, GetBlastForce(), GetAbsOrigin(), m_flDamage, bitsDamageType, 0, &vecReported );
	CTFWeaponInfo pWeaponInfo = *GetTFWeaponInfo( GetWeaponID() );
	float flRadius = pWeaponInfo.m_flDamageRadius;

	RadiusDamage( info, GetAbsOrigin(), flRadius, CLASS_NONE, NULL );

	CEffectData data;
	data.m_vOrigin = GetAbsOrigin();
	data.m_flRadius = flRadius;
	data.m_flMagnitude = m_flDamage;
	DispatchEffect( "conc_explosion", data );

	if (tf_grenade_show_radius.GetBool())
	{
		DrawRadius( flRadius, tf_grenade_show_radius_time.GetFloat() );
	}

	// Explosion decal.
	UTIL_DecalTrace(pTrace, "Scorch");

	// Reset.
	SetThink(&CBaseGrenade::SUB_Remove);
	SetTouch(NULL);
	AddEffects(EF_NODRAW);
	SetAbsVelocity(vec3_origin);
	SetNextThink(gpGlobals->curtime);
#endif
}

void CTFGrenadeConcussionProjectile::ExplodeInHand(CTFPlayer* pPlayer)
{
	//if (!pPlayer)
	//	return;

	//Vector a = pPlayer->GetAbsVelocity() * m_flDamage;
	//pPlayer->VelocityPunch(a);
	Detonate();
}
#else
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientsideConcExplosionCallback( const CEffectData &data )
{
	DispatchParticleEffect( "conc_warp", data.m_vOrigin, QAngle( 0, 0, 0 ) );
	// Cyanide; cus it looks funny
	CRagdollExplosionEnumerator	ragdollEnum( data.m_vOrigin, data.m_flRadius, data.m_flMagnitude );
	partition->EnumerateElementsInSphere( PARTITION_CLIENT_RESPONSIVE_EDICTS, data.m_vOrigin, data.m_flRadius, false, &ragdollEnum );
}

DECLARE_CLIENT_EFFECT( "conc_explosion", ClientsideConcExplosionCallback);
#endif
