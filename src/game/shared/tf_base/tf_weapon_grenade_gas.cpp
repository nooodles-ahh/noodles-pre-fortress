//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF Gas Grenade.
//
//=============================================================================//
#include "cbase.h"
#include "tf_weaponbase.h"
#include "tf_gamerules.h"
#include "npcevent.h"
#include "engine/IEngineSound.h"
#include "tf_weapon_grenade_gas.h"

// Server specific.
#ifdef GAME_DLL
#include "tf_player.h"
#include "items.h"
#include "tf_weaponbase_grenadeproj.h"
#include "soundent.h"
#include "KeyValues.h"
#include "trigger_area_capture.h"
#endif

#define GRENADE_GAS_TIMER	3.0f //Seconds

//=============================================================================
//
// TF Gas Grenade tables.
//

IMPLEMENT_NETWORKCLASS_ALIASED( TFGrenadeGas, DT_TFGrenadeGas )

BEGIN_NETWORK_TABLE( CTFGrenadeGas, DT_TFGrenadeGas )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFGrenadeGas )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_grenade_gas, CTFGrenadeGas );
PRECACHE_WEAPON_REGISTER( tf_weapon_grenade_gas );

//=============================================================================
//
// TF Gas Grenade functions.
//

// Server specific.
#ifdef GAME_DLL

BEGIN_DATADESC( CTFGrenadeGas )
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFWeaponBaseGrenadeProj *CTFGrenadeGas::EmitGrenade( Vector vecSrc, QAngle vecAngles, Vector vecVel, 
							        AngularImpulse angImpulse, CBasePlayer *pPlayer, float flTime, int iflags )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	if ( pTFPlayer )
	{
		pTFPlayer->RemoveDisguise();
	}

	return CTFGrenadeGasProjectile::Create( vecSrc, vecAngles, vecVel, angImpulse, 
		                                pPlayer, GetTFWpnData(), flTime );
}

#endif

IMPLEMENT_NETWORKCLASS_ALIASED(TFGrenadeGasProjectile, DT_TFGrenadeGasProjectile)

BEGIN_NETWORK_TABLE(CTFGrenadeGasProjectile, DT_TFGrenadeGasProjectile)
END_NETWORK_TABLE()


//=============================================================================
//
// TF Gas Grenade Projectile functions (Server specific).
//
#ifdef GAME_DLL

#define GRENADE_MODEL "models/weapons/w_models/w_grenade_gas.mdl"
#define GRENADE_SOUND "BaseSmokeEffect.Sound" 

LINK_ENTITY_TO_CLASS( tf_weapon_grenade_gas_projectile, CTFGrenadeGasProjectile );
PRECACHE_WEAPON_REGISTER( tf_weapon_grenade_gas_projectile );


BEGIN_DATADESC( CTFGrenadeGasProjectile )
	DEFINE_THINKFUNC( Think_Emit ),
	DEFINE_THINKFUNC( Think_Fade ),
END_DATADESC()
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFGrenadeGasProjectile* CTFGrenadeGasProjectile::Create( const Vector &position, const QAngle &angles, 
																const Vector &velocity, const AngularImpulse &angVelocity, 
																CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo, float timer, int iFlags )
{
	CTFGrenadeGasProjectile *pGrenade = static_cast<CTFGrenadeGasProjectile*>( CTFWeaponBaseGrenadeProj::Create( "tf_weapon_grenade_gas_projectile", position, angles, velocity, angVelocity, pOwner, weaponInfo, timer, iFlags ) );
	if ( pGrenade )
	{
		pGrenade->ApplyLocalAngularVelocityImpulse( angVelocity );
		pGrenade->SetDetonateTimerLength(timer);
		pGrenade->ChangeTeam(pOwner->GetTeamNumber());
		pGrenade->m_flMaxLifetime = gpGlobals->curtime + timer + 10.0f; // incase we're stuck
	}

	return pGrenade;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeGasProjectile::Spawn()
{
	SetModel( GRENADE_MODEL );

	BaseClass::Spawn();

	m_hGasEffect = NULL;
}

CTFGrenadeGasProjectile::~CTFGrenadeGasProjectile()
{
	if ( m_hGasEffect.Get() )
	{
		UTIL_Remove( m_hGasEffect );
	}
	StopSound( GRENADE_SOUND );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeGasProjectile::Precache()
{
	PrecacheModel( GRENADE_MODEL );
	PrecacheParticleSystem( "spy_gasgrenade_blue" );
	PrecacheParticleSystem( "spy_gasgrenade_red" );
	PrecacheScriptSound(GRENADE_SOUND);
	PrecacheScriptSound("Weapon_Grenade_Gas.Bounce");
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeGasProjectile::BounceSound( void )
{
	EmitSound( "Weapon_Grenade_Gas.Bounce" );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeGasProjectile::DetonateThink( void )
{
	// if we're past the detonate time but still moving, delay the detonate
	/*if( gpGlobals->curtime > GetDetonateTime() && VPhysicsGetObject() && gpGlobals->curtime < m_flMaxLifetime )
	{
		Vector vel;
		VPhysicsGetObject()->GetVelocity(&vel, NULL);
		// we've failed to find a ground entity so wait for the grenade to stop moving or to find ground
		// this also helps in cases where we've somehow not registered that we've touched the ground
		if (!GetGroundEntity() && vel.Length() > 25.0)
		{
			SetTimer(gpGlobals->curtime + 0.05f);
		}
	}*/

	BaseClass::DetonateThink();
}

bool CTFGrenadeGasProjectile::ShouldNotDetonate(void)
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
	if (UTIL_PointContents(GetAbsOrigin()) & CONTENTS_WATER)
	{
		return true;
	}

	if(m_flMaxLifetime <= gpGlobals->curtime)
		return true;

	return BaseClass::ShouldNotDetonate();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeGasProjectile::Detonate()
{
	if ( ShouldNotDetonate() )
	{
		RemoveGrenade();
		return;
	}

	// start emitting gas
	m_bIsGassing = true;

	m_hGasEffect = ( CTFGasGrenadeEffect * )CreateEntityByName("tf_gas_grenade_effect");
	CBaseEntity *pGasEffect = m_hGasEffect.Get();
	if ( pGasEffect )
	{	
		DispatchSpawn( pGasEffect );
		pGasEffect->SetAbsOrigin( GetAbsOrigin() );
		pGasEffect->SetParent(this);
		pGasEffect->ChangeTeam(GetTeamNumber());
		pGasEffect->SetAbsAngles( QAngle(0, 0, 0) );
	}

	EmitSound( GRENADE_SOUND );

	// damage / hallucination effect in waves
	m_nPulses = 30;

	SetThink( &CTFGrenadeGasProjectile::Think_Emit );
	SetNextThink( gpGlobals->curtime + 0.1f );
}

//-----------------------------------------------------------------------------
// Purpose: Emit gas pulses
//-----------------------------------------------------------------------------
void CTFGrenadeGasProjectile::Think_Emit( void )
{
	if( GetMoveType() == MOVETYPE_VPHYSICS && VPhysicsGetObject() )
	{
		Vector vel;
		VPhysicsGetObject()->GetVelocity( &vel, NULL );
		// we've failed to find a ground entity so wait for the grenade to stop moving or to find ground
		// this also helps in cases where we've somehow not registered that we've touched the ground
		if( GetGroundEntity() )
		{
			VPhysicsGetObject()->EnableMotion( false );
			m_hGasEffect.Get()->SetAbsAngles( QAngle( 0, 0, 0 ) );
		}
	}

	Vector vecOrigin = GetAbsOrigin();

	CTFWeaponInfo pWeaponInfo = *GetTFWeaponInfo( GetWeaponID() );

	float flDamage = pWeaponInfo.GetWeaponDamage(TF_WEAPON_PRIMARY_MODE);
	float flRadius = pWeaponInfo.m_flDamageRadius;

	// damage on every second pluse
	bool bDoDamage = m_nPulses % 2 == 0;

	if( bDoDamage && tf_grenade_show_radius.GetBool() )
	{
		DrawRadius( flRadius, 0.75f );
	}

	CBaseEntity * pEntity = NULL;

	for (CEntitySphereQuery sphere( GetAbsOrigin(), flRadius ); (pEntity = sphere.GetCurrentEntity()) != NULL; sphere.NextEntity())
	{
		if (pEntity->m_takedamage == DAMAGE_NO)
			continue;

		// check for valid player
		if (!pEntity->IsPlayer())
			continue;

		if (!RadiusHit( GetAbsOrigin(), this, pEntity ))
			continue;

		CTFPlayer *pPlayer = ToTFPlayer(pEntity);
		if (pPlayer && !pPlayer->m_Shared.InCond( TF_COND_INVULNERABLE ))
		{
			if( pPlayer == GetThrower() || TFGameRules()->FriendlyFireMode() > FFMODE_EVENTS || pPlayer->GetTeamNumber() != GetTeamNumber() )
				pPlayer->m_Shared.AddCond( TF_COND_ARMOR_MELTING, 0.375f );
			if( bDoDamage )
			{
				CTakeDamageInfo info( this, GetThrower(), vec3_origin, pPlayer->WorldSpaceCenter(), flDamage, DMG_NERVEGAS | DMG_PREVENT_PHYSICS_FORCE );
				info.SetIgnoreArmor( true );
				pPlayer->TakeDamage( info );
			}
		}
	}

	m_nPulses--;

	if (m_nPulses <= 0)
	{
		// Fade out
		SetThink(&CTFGrenadeGasProjectile::Think_Fade);
	}

	SetNextThink(gpGlobals->curtime + 0.375f );
}

void CTFGrenadeGasProjectile::VPhysicsCollision(int index, gamevcollisionevent_t* pEvent)
{
	BaseClass::VPhysicsCollision( index, pEvent );

	/*if( !m_bIsGassing && GetMoveType() == MOVETYPE_VPHYSICS )
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
	VPhysicsGetObject()->EnableMotion(false);*/
}
void CTFGrenadeGasProjectile::ExplodeInHand(CTFPlayer* pPlayer)
{
	SetTimer(gpGlobals->curtime+0.5);
}
//-----------------------------------------------------------------------------
// Fade the projectile out over time before making it disappear
//-----------------------------------------------------------------------------
void CTFGrenadeGasProjectile::Think_Fade()
{
	color32 c = GetRenderColor();
	c.a -= 1;
	SetRenderColor( c.r, c.b, c.g, c.a );

	if ( !c.a )
	{
		UTIL_Remove( this );
	}

	SetNextThink( gpGlobals->curtime );
}

void CTFGrenadeGasProjectile::Disarm( const CTakeDamageInfo &info )
{
	if(m_bIsGassing)
	{
		// re-enable damage
		m_takedamage = DAMAGE_YES;
		SetCollisionGroup( TF_COLLISIONGROUP_GRENADES );
		m_bIsGassing = false;
		VPhysicsGetObject()->EnableMotion(true);
		if ( m_hGasEffect.Get() )
		{
			UTIL_Remove( m_hGasEffect );
		}	
	}

	BaseClass::Disarm(info);
}
#endif


IMPLEMENT_NETWORKCLASS_ALIASED( TFGasGrenadeEffect, DT_TFGasGrenadeEffect )

BEGIN_NETWORK_TABLE(CTFGasGrenadeEffect, DT_TFGasGrenadeEffect )
END_NETWORK_TABLE()

#ifndef CLIENT_DLL
	LINK_ENTITY_TO_CLASS( tf_gas_grenade_effect, CTFGasGrenadeEffect );
#endif

#ifndef CLIENT_DLL

int CTFGasGrenadeEffect::UpdateTransmitState( void )
{
	return SetTransmitState( FL_EDICT_PVSCHECK );
}

#else

	void CTFGasGrenadeEffect::OnDataChanged( DataUpdateType_t updateType )
	{
		if ( updateType == DATA_UPDATE_CREATED && m_pGasEffect == NULL )
		{
			m_pGasEffect = ParticleProp()->Create(GetTeamNumber() == TF_TEAM_BLUE ? "spy_gasgrenade_blue" : "spy_gasgrenade_red", PATTACH_ABSORIGIN );
		}
	}

#endif // CLIENT_DLL
