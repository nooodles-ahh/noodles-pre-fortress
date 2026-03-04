
//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======
//
// TF Rocket
//
//=============================================================================
#include "cbase.h"
#include "Sprite.h"
#include "tf_projectile_rocket.h"
#include "particle_parse.h"

//=============================================================================
//
// TF Rocket functions (Server specific).
//
#define ROCKET_MODEL "models/weapons/w_models/w_rocket.mdl"
#define FLAME_ROCKET_MODEL "models/weapons/w_models/w_rocket_flame.mdl"

LINK_ENTITY_TO_CLASS( tf_projectile_rocket, CTFProjectile_Rocket );
PRECACHE_REGISTER( tf_projectile_rocket );

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_Rocket, DT_TFProjectile_Rocket )

BEGIN_NETWORK_TABLE( CTFProjectile_Rocket, DT_TFProjectile_Rocket )
	SendPropBool( SENDINFO( m_bCritical ) ),
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS(tf_projectile_flame_rocket, CTFFlameRocket);
PRECACHE_REGISTER(tf_projectile_flame_rocket);

IMPLEMENT_NETWORKCLASS_ALIASED(TFFlameRocket, DT_TFFlameRocket)

BEGIN_NETWORK_TABLE(CTFFlameRocket, DT_TFFlameRocket)
	SendPropBool(SENDINFO(m_bCritical)),
END_NETWORK_TABLE()

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFProjectile_Rocket *CTFProjectile_Rocket::Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, CBaseEntity *pScorer, float flVelocity)
{
	CTFProjectile_Rocket *pRocket = static_cast<CTFProjectile_Rocket*>( CTFBaseRocket::Create( "tf_projectile_rocket", vecOrigin, vecAngles, pOwner, flVelocity ) );

	if ( pRocket )
	{
		pRocket->SetScorer( pScorer );
	}

	return pRocket;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_Rocket::Spawn()
{
	SetModel( ROCKET_MODEL );
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_Rocket::Precache()
{
	PrecacheModel( ROCKET_MODEL );
	PrecacheModel( FLAME_ROCKET_MODEL );
	PrecacheParticleSystem( "critical_rocket_blue" );
	PrecacheParticleSystem( "critical_rocket_red" );
	PrecacheParticleSystem( "rockettrail" );
	PrecacheParticleSystem( "incendiaryrockettrail" );
	PrecacheParticleSystem( "incendiary_disappear" );
	PrecacheTeamParticles( "incendiaryrocket_crit_%s", g_aTeamParticleNames );
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_Rocket::SetScorer( CBaseEntity *pScorer )
{
	m_Scorer = pScorer;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CBasePlayer *CTFProjectile_Rocket::GetScorer( void )
{
	return dynamic_cast<CBasePlayer *>( m_Scorer.Get() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFProjectile_Rocket::GetDamageType() 
{ 
	int iDmgType = BaseClass::GetDamageType();
	if ( m_bCritical )
	{
		iDmgType |= DMG_CRITICAL;
	}

	return iDmgType;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFFlameRocket *CTFFlameRocket::Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, CBaseEntity *pScorer, float flVelocity )
{
	CTFFlameRocket *pRocket = static_cast<CTFFlameRocket*>(CTFBaseRocket::Create( "tf_projectile_flame_rocket", vecOrigin, vecAngles, pOwner, flVelocity ));

	if (pRocket)
	{
		pRocket->SetScorer( pScorer );
	}

	return pRocket;
}

void CTFFlameRocket::Spawn()
{
	WaterCheck();
	BaseClass::Spawn();
	SetModel( FLAME_ROCKET_MODEL );
}

void CTFFlameRocket::WaterCheck()
{
	if (GetWaterLevel() > 0)
	{
		SetThink( &BaseClass::SUB_Remove );
		SetNextThink( gpGlobals->curtime );
		SetTouch( NULL );
		AddEffects( EF_NODRAW );
		DispatchParticleEffect( "incendiary_disappear", GetAbsOrigin(), QAngle( 0, 0, 0 ) );
	}

	SetContextThink(&CTFFlameRocket::WaterCheck, gpGlobals->curtime, "WaterCheck");
}