//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF Napalm Grenade.
//
//=============================================================================//
#include "cbase.h"
#include "tf_weaponbase.h"
#include "tf_gamerules.h"
#include "npcevent.h"
#include "engine/IEngineSound.h"
#include "tf_weapon_grenade_napalm.h"

// Server specific.
#ifdef GAME_DLL
#include "tf_player.h"
#include "items.h"
#include "tf_weaponbase_grenadeproj.h"
#include "soundent.h"
#include "KeyValues.h"
#include "trigger_area_capture.h"
#include "explode.h"
#include "tf/tf_fx.h"
#endif

#define NAPALM_BURN_TIMER 5.0f
#define NAPALM_BURN_TIME_BOMB 1.0f //will only burn twice

//=============================================================================
//
// TF Napalm Grenade tables.
//

IMPLEMENT_NETWORKCLASS_ALIASED(TFGrenadeNapalm, DT_TFGrenadeNapalm)

BEGIN_NETWORK_TABLE(CTFGrenadeNapalm, DT_TFGrenadeNapalm)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CTFGrenadeNapalm)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(tf_weapon_grenade_napalm, CTFGrenadeNapalm);
PRECACHE_WEAPON_REGISTER(tf_weapon_grenade_napalm);

//=============================================================================
//
// TF Napalm Grenade functions.
//

// Server specific.
#ifdef GAME_DLL
ConVar pf_napalm_bomb( "pf_napalm_bomb", "0", FCVAR_NOTIFY, "Alternative napalm grenade functionality. Behaves more like a standard frag grenade, but sets people on fire " );

BEGIN_DATADESC(CTFGrenadeNapalm)
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFWeaponBaseGrenadeProj* CTFGrenadeNapalm::EmitGrenade(Vector vecSrc, QAngle vecAngles, Vector vecVel, 
							        AngularImpulse angImpulse, CBasePlayer* pPlayer, float flTime, int iflags)
{
	return CTFGrenadeNapalmProjectile::Create(vecSrc, vecAngles, vecVel, angImpulse, 
		                                pPlayer, GetTFWpnData(), flTime);
}

#endif
IMPLEMENT_NETWORKCLASS_ALIASED(TFGrenadeNapalmProjectile, DT_TFGrenadeNapalmProjectile)

BEGIN_NETWORK_TABLE(CTFGrenadeNapalmProjectile, DT_TFGrenadeNapalmProjectile)
END_NETWORK_TABLE()

#ifdef GAME_DLL
#define GRENADE_MODEL "models/weapons/w_models/w_grenade_napalm.mdl"
#define GRENADE_MODEL_BOMB "models/weapons/w_models/w_grenade_napalm_bomb.mdl"
#define GRENADE_SOUND "Weapon_Grenade_Napalm.Fire"

//=============================================================================
//
// TF Normal Grenade functions.
//
CTFGrenadeNapalmProjectile::CTFGrenadeNapalmProjectile()
{
}
CTFGrenadeNapalmProjectile::~CTFGrenadeNapalmProjectile()
{
	if ( m_hNapalmEffect.Get() )
	{
		UTIL_Remove( m_hNapalmEffect );
	}
	StopSound( GRENADE_SOUND );
}

//=============================================================================
//
// TF Napalm Grenade Projectile functions (Server specific).
//
LINK_ENTITY_TO_CLASS(tf_weapon_grenade_napalm_projectile, CTFGrenadeNapalmProjectile);
PRECACHE_WEAPON_REGISTER(tf_weapon_grenade_napalm_projectile);


BEGIN_DATADESC( CTFGrenadeNapalmProjectile )
	DEFINE_THINKFUNC( Think_Emit ),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFGrenadeNapalmProjectile* CTFGrenadeNapalmProjectile::Create( const Vector& position, const QAngle& angles, 
																const Vector& velocity, const AngularImpulse& angVelocity, 
																CBaseCombatCharacter* pOwner, const CTFWeaponInfo& weaponInfo, float timer, int iFlags)
{
	CTFGrenadeNapalmProjectile* pGrenade = static_cast<CTFGrenadeNapalmProjectile*>( CTFWeaponBaseGrenadeProj::Create("tf_weapon_grenade_napalm_projectile", position, angles, velocity, angVelocity, pOwner, weaponInfo, timer, iFlags));
	if (pGrenade)
	{
		pGrenade->ApplyLocalAngularVelocityImpulse(angVelocity);
		pGrenade->SetDetonateTimerLength(timer);
		pGrenade->ChangeTeam(pOwner->GetTeamNumber());
		pGrenade->m_flMaxLifetime = gpGlobals->curtime + timer + 10.0f; // incase we're stuck
	}

	return pGrenade;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeNapalmProjectile::Spawn()
{
	m_bBomb = pf_napalm_bomb.GetBool();
	SetModel( m_bBomb ? GRENADE_MODEL_BOMB : GRENADE_MODEL );

	BaseClass::Spawn();

	m_hNapalmEffect = NULL;
	m_bIsBurning = false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeNapalmProjectile::Precache()
{
	PrecacheModel(GRENADE_MODEL);
	PrecacheModel(GRENADE_MODEL_BOMB);
	PrecacheParticleSystem("napalm_water");
	PrecacheParticleSystem("napalm_explosion_flames");
	PrecacheParticleSystem("napalm_fire_red");
	PrecacheParticleSystem("napalm_fire_blue");
	PrecacheScriptSound(GRENADE_SOUND);
	PrecacheScriptSound("Weapon_Grenade_Napalm.Bounce");
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeNapalmProjectile::BounceSound( void )
{
	EmitSound( "Weapon_Grenade_Napalm.Bounce" );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeNapalmProjectile::DetonateThink( void )
{
	// if we're past the detonate time but still moving, delay the detonate
	if (!m_bBomb && gpGlobals->curtime >= GetDetonateTime() && gpGlobals->curtime < m_flMaxLifetime)
	{
		if( GetMoveType() == MOVETYPE_VPHYSICS && VPhysicsGetObject() )
		{
			Vector vel;
			VPhysicsGetObject()->GetVelocity(&vel, NULL);
			// we've failed to find a ground entity so wait for the grenade to stop moving or to find ground
			// this also helps in cases where we've somehow not registered that we've touched the ground
			if (!GetGroundEntity() && vel.Length() > 25.0)
			{
				SetTimer(gpGlobals->curtime + 0.05f);
			}
		}
		else
		{
			if (!GetGroundEntity())
			{
				SetTimer(gpGlobals->curtime + 0.05f);
			}
		}
	}
	BaseClass::DetonateThink();
	SetNextThink( gpGlobals->curtime + 0.1f );
}

bool CTFGrenadeNapalmProjectile::ShouldNotDetonate(void)
{
	CBaseEntity* pTempEnt = NULL;

	while ((pTempEnt = gEntList.FindEntityByClassname(pTempEnt, "trigger_capture_area")) != NULL)
	{
		CTriggerAreaCapture* pZone = dynamic_cast<CTriggerAreaCapture*>(pTempEnt);

		if (pZone->IsTouching(this))
		{
			return true;
		}
	}

	if(m_flMaxLifetime <= gpGlobals->curtime)
		return true;

	return BaseClass::ShouldNotDetonate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeNapalmProjectile::Explode( trace_t *pTrace, int bitsDamageType )
{
	AddSolidFlags( FSOLID_NOT_SOLID );

	m_takedamage = DAMAGE_NO;

	// Pull out of the wall a bit
	if ( pTrace->fraction != 1.0 )
	{
		SetAbsOrigin( pTrace->endpos + ( pTrace->plane.normal * 1.0f ) );
	}

	CSoundEnt::InsertSound ( SOUND_COMBAT, GetAbsOrigin(), BASEGRENADE_EXPLOSION_VOLUME, 3.0 );

	// Explosion effect on client
	Vector vecOrigin = GetAbsOrigin();
	CPVSFilter filter( vecOrigin );
	TE_TFExplosion( filter, 0.0f, vecOrigin, pTrace->plane.normal, GetWeaponID(), pTrace->GetEntityIndex());
	//ExplosionCreate(vecOrigin, vec3_angle, GetOwnerEntity(), 0, 0, false, 0.0);

	// Use the thrower's position as the reported position
	Vector vecReported = GetThrower() ? GetThrower()->GetAbsOrigin() : vec3_origin;

	CTakeDamageInfo info( this, GetThrower(), GetBlastForce(), GetAbsOrigin(), m_flDamage, bitsDamageType, 0, &vecReported );

	float flRadius = GetDamageRadius();

	RadiusDamage( info, vecOrigin, flRadius, CLASS_NONE, NULL );

	// Don't decal players with scorch.
	if ( pTrace->m_pEnt && !pTrace->m_pEnt->IsPlayer() )
	{
		UTIL_DecalTrace( pTrace, "Scorch" );
	}
	if(VPhysicsGetObject())
		VPhysicsGetObject()->EnableMotion(false);
	SetTouch( NULL );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeNapalmProjectile::Detonate()
{
	if ( ShouldNotDetonate() )
	{
		RemoveGrenade();
		return;
	}

	BaseClass::Detonate();

	m_bIsBurning = true;

	bool bInWater = (UTIL_PointContents(GetAbsOrigin()) & CONTENTS_WATER) != 0;
	if(bInWater || m_bBomb)
		m_flBurnTime = gpGlobals->curtime + NAPALM_BURN_TIME_BOMB;
	else
		m_flBurnTime = gpGlobals->curtime + NAPALM_BURN_TIMER;

	EmitSound(GRENADE_SOUND);
	
	m_hNapalmEffect = ( CTFGrenadeNapalmEffect * )CreateEntityByName("tf_grenade_napalm_effect");
	CTFGrenadeNapalmEffect *pNapalmEffect = m_hNapalmEffect.Get();
	if ( pNapalmEffect )
	{	
		pNapalmEffect->SetWater(bInWater);
		pNapalmEffect->SetBomb(m_bBomb);
		DispatchSpawn(pNapalmEffect);
		pNapalmEffect->SetAbsOrigin(GetAbsOrigin());
		pNapalmEffect->SetParent(this);
		pNapalmEffect->ChangeTeam(GetTeamNumber());
	}

	AddEffects( EF_NODRAW );
	SetThink( &CTFGrenadeNapalmProjectile::Think_Emit );
	SetNextThink( gpGlobals->curtime + 0.1f );
}

//-----------------------------------------------------------------------------
// Purpose: Emit napalm fire
//-----------------------------------------------------------------------------
void CTFGrenadeNapalmProjectile::Think_Emit( void )
{
	CTFPlayer* pOwner = ToTFPlayer(GetThrower());

	if (gpGlobals->curtime >= m_flBurnTime || !pOwner)
	{
		Remove();
		return;
	}

	CTFWeaponInfo pWeaponInfo = *GetTFWeaponInfo( GetWeaponID() );

	float flDamage = pWeaponInfo.m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_nDamage;
	float flRadius = pWeaponInfo.m_flDamageRadius;

	CBaseEntity *pEntity = NULL;
	for (CEntitySphereQuery sphere( GetAbsOrigin(), flRadius ); (pEntity = sphere.GetCurrentEntity()) != NULL; sphere.NextEntity())
	{
		if (pEntity->m_takedamage == DAMAGE_NO)
			continue;

		// check for valid player
		if (!pEntity->IsPlayer())
			continue;

		CTFPlayer* pPlayer = ToTFPlayer( pEntity );

		if (pPlayer /*&& (pPlayer->GetTeamNumber() != pOwner->GetTeamNumber() || pPlayer == pOwner)*/)
		{
			trace_t c;
			UTIL_TraceLine(GetAbsOrigin(), GetAbsOrigin() + Vector(0, 0, 16), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_DEBRIS, &c);
			trace_t trace;
			UTIL_TraceLine(GetAbsOrigin() + Vector(0, 0, 16 * c.fraction), pPlayer->GetAbsOrigin(), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_DEBRIS, &trace);
			if (trace.fraction >= 1)
			{
				CTFPlayer* attacker = ToTFPlayer(GetThrower());
				if (!attacker)
				{
					attacker = pPlayer;
				}
				if (!pPlayer->m_Shared.InCond( TF_COND_INVULNERABLE ))
				{
					CTakeDamageInfo info( this, attacker, vec3_origin, pPlayer->GetAbsOrigin(), flDamage, DMG_IGNITE | DMG_BURN | DMG_PREVENT_PHYSICS_FORCE );
					info.SetDamageCustom( TF_DMG_CUSTOM_NAPALM_BURNING );
					pPlayer->TakeDamage( info );
				}
			}
		}
	}
	SetNextThink(gpGlobals->curtime + 0.5);
}

void CTFGrenadeNapalmProjectile::VPhysicsCollision(int index, gamevcollisionevent_t* pEvent)
{
	BaseClass::VPhysicsCollision( index, pEvent );

	if ( !m_bIsBurning && GetMoveType() == MOVETYPE_VPHYSICS )
	{
		int otherIndex = !index;
		CBaseEntity* pHitEntity = pEvent->pEntities[otherIndex];

		if (pEvent->deltaCollisionTime < 0.2f && (pHitEntity == this))
			return;

		Vector vel;
		VPhysicsGetObject()->GetVelocity(&vel, NULL);
		float flSpeedSqr = DotProduct(vel, vel);
		if (flSpeedSqr < 400.0f)
		{
			if (pHitEntity->IsStandable())
			{
				SetGroundEntity(pHitEntity);
			}
		}
		return;
	}
	VPhysicsGetObject()->EnableMotion(false);
}
void CTFGrenadeNapalmProjectile::ExplodeInHand(CTFPlayer* pPlayer)
{
	SetTimer(gpGlobals->curtime+0.2);
}
#endif


IMPLEMENT_NETWORKCLASS_ALIASED( TFGrenadeNapalmEffect, DT_TFGrenadeNapalmEffect )

BEGIN_NETWORK_TABLE(CTFGrenadeNapalmEffect, DT_TFGrenadeNapalmEffect )
#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO( m_bBomb ) ),
	RecvPropBool( RECVINFO( m_bInWater ) ),
#else
	SendPropBool( SENDINFO( m_bBomb ) ),
	SendPropBool( SENDINFO( m_bInWater ) ),
#endif
END_NETWORK_TABLE()

#ifndef CLIENT_DLL
	LINK_ENTITY_TO_CLASS( tf_grenade_napalm_effect, CTFGrenadeNapalmEffect );
#endif

#ifndef CLIENT_DLL

int CTFGrenadeNapalmEffect::UpdateTransmitState( void )
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

#else

	void CTFGrenadeNapalmEffect::OnDataChanged( DataUpdateType_t updateType )
	{
		if ( updateType == DATA_UPDATE_CREATED && m_pNapalmEffect == NULL )
		{
			if(m_bInWater)
			{
				m_pNapalmEffect = ParticleProp()->Create("napalm_water", PATTACH_ABSORIGIN );
			}
			else if (m_bBomb)
			{
				m_pNapalmEffect = ParticleProp()->Create("napalm_explosion_flames", PATTACH_ABSORIGIN );
			}
			else
			{
				m_pNapalmEffect = ParticleProp()->Create(GetTeamNumber() == TF_TEAM_BLUE ? "napalm_fire_blue" : "napalm_fire_red", PATTACH_ABSORIGIN );
			}
			
		}
	}

#endif // CLIENT_DLL