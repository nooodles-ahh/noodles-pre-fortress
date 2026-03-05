//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF Emp Grenade.
//
//=============================================================================//
#include "cbase.h"
#include "tf_weaponbase.h"
#include "tf_gamerules.h"
#include "npcevent.h"
#include "engine/IEngineSound.h"
#include "tf_weapon_grenade_emp.h"

// Server specific.
#ifdef GAME_DLL
#include "tf_player.h"
#include "items.h"
#include "tf_weaponbase_grenadeproj.h"
#include "soundent.h"
#include "KeyValues.h"
#include "particle_parse.h"
#include "beam_shared.h"
#include "tf_ammo_pack.h"
#include "tf_obj.h"
#endif

#define GRENADE_EMP_MINS				Vector(-7.6f, -7.6f, -2.4f) //Vector(-1, -1, -7.6)
#define GRENADE_EMP_MAXS				Vector(7.6f, 7.6f, 2.4f) //Vector(1, 1, 7.6)

#define GRENADE_EMP_TIMER	3.0f //Seconds
#define	GRENADE_EMP_LEADIN	2.0f 

//=============================================================================
//
// TF Emp Grenade tables.
//

IMPLEMENT_NETWORKCLASS_ALIASED( TFGrenadeEmp, DT_TFGrenadeEmp )

BEGIN_NETWORK_TABLE( CTFGrenadeEmp, DT_TFGrenadeEmp )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFGrenadeEmp )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_grenade_emp, CTFGrenadeEmp );
PRECACHE_WEAPON_REGISTER( tf_weapon_grenade_emp );

//=============================================================================
//
// TF Emp Grenade functions.
//

// Server specific.
#ifdef GAME_DLL

BEGIN_DATADESC( CTFGrenadeEmp )
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFWeaponBaseGrenadeProj *CTFGrenadeEmp::EmitGrenade( Vector vecSrc, QAngle vecAngles, Vector vecVel, 
							        AngularImpulse angImpulse, CBasePlayer *pPlayer, float flTime, int iflags )
{
	return CTFGrenadeEmpProjectile::Create( vecSrc, vecAngles, vecVel, angImpulse, 
		                                pPlayer, GetTFWpnData(), flTime );
}

#endif

//=============================================================================
//
// TF Emp Grenade Projectile functions (Server specific).
//
#ifdef GAME_DLL

#define GRENADE_MODEL "models/weapons/w_models/w_grenade_emp.mdl"

LINK_ENTITY_TO_CLASS( tf_weapon_grenade_emp_projectile, CTFGrenadeEmpProjectile );
PRECACHE_REGISTER( tf_weapon_grenade_emp_projectile );

BEGIN_DATADESC( CTFGrenadeEmpProjectile )
DEFINE_THINKFUNC( DetonateThink ),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFGrenadeEmpProjectile* CTFGrenadeEmpProjectile::Create( const Vector &position, const QAngle &angles, 
																const Vector &velocity, const AngularImpulse &angVelocity, 
																CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo, float timer, int iFlags )
{
	// Nail grenades are always thrown like discs
	QAngle vecCustomAngles = angles;
	Vector vecCustomAngVelocity = angVelocity;
	if( pf_grenades_tfc.GetBool() )
	{
		vecCustomAngles.x = 90;
		vecCustomAngles.z = 0;
		vecCustomAngVelocity = vec3_origin;
	}
	
	CTFGrenadeEmpProjectile *pGrenade = static_cast<CTFGrenadeEmpProjectile*>( 
		CTFWeaponBaseGrenadeProj::Create( "tf_weapon_grenade_emp_projectile", 
											position, 
											vecCustomAngles, 
											velocity, 
											vecCustomAngVelocity, 
											pOwner, 
											weaponInfo, 
											timer, 
											iFlags ) 
										);
	if( pGrenade )
	{
		if( pf_grenades_tfc.GetBool() )
		{
			pGrenade->ApplyLocalAngularVelocityImpulse( Vector(0, 0, 1000) );
			UTIL_SetSize( pGrenade, GRENADE_EMP_MINS, GRENADE_EMP_MAXS );
		}
		else
			pGrenade->ApplyLocalAngularVelocityImpulse( angVelocity );	

#if 0
		ConMsg("Mins: %f, %f, %f\t Maxs: %f, %f, %f\n", pGrenade->WorldAlignMins().x, pGrenade->WorldAlignMins().y, pGrenade->WorldAlignMins().z, 
				pGrenade->WorldAlignMaxs().x, pGrenade->WorldAlignMaxs().y, pGrenade->WorldAlignMaxs().z );
#endif
	}

	return pGrenade;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeEmpProjectile::Spawn()
{
	Precache();
	SetModel( GRENADE_MODEL );

	BaseClass::Spawn();

	m_bPlayedLeadIn = false;

	SetThink( &CTFGrenadeEmpProjectile::DetonateThink );
}

#define LIGHTNING_RED "sprites/lightning_red.vmt"
#define LIGHTNING_BLUE "sprites/lightning_blue.vmt"

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeEmpProjectile::Precache()
{
	PrecacheModel( GRENADE_MODEL );
	PrecacheScriptSound( "Weapon_Grenade_Emp.LeadIn" );
	PrecacheScriptSound("Weapon_Grenade_Emp.Explode");
	PrecacheScriptSound("Weapon_Grenade_Emp.Bounce");
	PrecacheModel( LIGHTNING_RED );
	PrecacheModel( LIGHTNING_BLUE );
	PrecacheParticleSystem( "emp_blue" );
	PrecacheParticleSystem( "emp_red" );
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeEmpProjectile::BounceSound( void )
{
	EmitSound( "Weapon_Grenade_Emp.Bounce" );
}

ConVar tf_emp_explode_ammo( "tf_emp_explode_ammo", "1", FCVAR_DEVELOPMENTONLY );
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeEmpProjectile::Detonate()
{
	if ( ShouldNotDetonate() )
	{
		RemoveGrenade();
		return;
	}

	// Explosion effect on client
	//SendDispatchEffect();

	CTFWeaponInfo pWeaponInfo = *GetTFWeaponInfo( GetWeaponID() );

	float flRadius = pWeaponInfo.m_flDamageRadius;
	float flDamage = pWeaponInfo.GetWeaponDamage(TF_WEAPON_PRIMARY_MODE);
	

	if ( tf_grenade_show_radius.GetBool() )
	{
		DrawRadius( flRadius, tf_grenade_show_radius_time.GetFloat() );
	}

	// Apply some amount of EMP damage to every entity in the radius. They will calculate 
	// their own damage based on how much ammo they have or some other wacky calculation.

	CTakeDamageInfo info( this, GetThrower(), vec3_origin, GetAbsOrigin(), flDamage, DMG_DISSOLVE | DMG_PREVENT_PHYSICS_FORCE, TF_DMG_CUSTOM_EMP_EXPLOSION );
	CBaseEntity *pEntity = NULL;
	for (CEntitySphereQuery sphere( GetAbsOrigin(), flRadius ); (pEntity = sphere.GetCurrentEntity()) != NULL; sphere.NextEntity())
	{
		CTFPlayer* pPlayer = ToTFPlayer( pEntity );
		CTFAmmoPack *pAmmo = dynamic_cast< CTFAmmoPack * >(pEntity);
		CBaseObject *pObj = dynamic_cast<CBaseObject*>(pEntity);
		
		if (!RadiusHit(GetAbsOrigin(), this, pEntity))
			continue;

		if( pPlayer )
		{
			if (!pPlayer->IsAlive() || 
				(pPlayer->GetTeamNumber() == GetThrower()->GetTeamNumber() && pPlayer != GetThrower() && TFGameRules()->FriendlyFireMode() < FFMODE_ON) ||
				 pPlayer->m_Shared.InCond(TF_COND_INVULNERABLE))
				continue;
			EMPBeam( pEntity );
			pPlayer->TakeDamage( info );
		}
		if( pAmmo )
		{
			EMPBeam( pEntity );
			if (tf_emp_explode_ammo.GetBool())
			{
				if(!pAmmo->IsDisintegrating())
				{
					pAmmo->Disintegrate(true, GetThrower());
					pAmmo->GetBaseAnimating()->Dissolve( "", gpGlobals->curtime, false, ENTITY_DISSOLVE_NORMAL );
				}
			}
			else
			{
				if(!pAmmo->IsDisintegrating())
				{
					pAmmo->GetBaseAnimating()->Dissolve( "", gpGlobals->curtime, false, ENTITY_DISSOLVE_NORMAL );
				}
			}
			
		}
		if ( pObj )
		{
			if ((pObj->GetTeamNumber() != GetThrower()->GetTeamNumber()) && !pObj->IsPlacing())
			{
				EMPBeam( pEntity );
				pObj->Disable(7.5);
			}
		}
	}

	DispatchParticleEffect( GetThrower()->GetTeamNumber() == TF_TEAM_RED ? "emp_red" : "emp_blue", GetAbsOrigin(), vec3_angle );

	Vector soundPosition = GetAbsOrigin() + Vector(0, 0, 5);
	CPASAttenuationFilter filter(soundPosition);
	EmitSound(filter, entindex(), "Weapon_Grenade_Emp.Explode");

	UTIL_Remove( this );

#if 0
	// Tell the bots an HE grenade has exploded
	CTFPlayer *pPlayer = ToTFPlayer( GetThrower() );
	if ( pPlayer )
	{
		KeyValues *pEvent = new KeyValues( "tf_weapon_grenade_detonate" );
		pEvent->SetInt( "userid", pPlayer->GetUserID() );
		gameeventmanager->FireEventServerOnly( pEvent );
	}
#endif
}

void CTFGrenadeEmpProjectile::EMPBeam( CBaseEntity *pEntity )
{
	trace_t c;
	UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() + Vector( 0, 0, 16 ), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_DEBRIS, &c );
	trace_t trace;
	UTIL_TraceLine( GetAbsOrigin() + Vector( 0, 0, 16 * c.fraction ), pEntity->GetAbsOrigin(), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_DEBRIS, &trace );
	if (trace.fraction >= 1)
	{
		CBeam* pBeam = CBeam::BeamCreate( GetThrower()->GetTeamNumber() == TF_TEAM_BLUE ? LIGHTNING_BLUE : LIGHTNING_RED, 5.0 );
		if (!pBeam)
			return;

		pBeam->PointsInit( GetAbsOrigin(), pEntity->WorldSpaceCenter() );
		pBeam->SetColor( 255, 255, 255 );
		pBeam->SetBrightness( 128 );
		pBeam->SetNoise( 8.0f );
		pBeam->SetEndWidth( 5.0f );
		pBeam->SetWidth( 5.0f );
		pBeam->LiveForTime( 0.5f );	// Fail-safe
		pBeam->SetFrameRate( 25.0f );
		pBeam->SetFrame( random->RandomInt( 0, 2 ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeEmpProjectile::DetonateThink( void )
{
	if ( !m_bPlayedLeadIn && gpGlobals->curtime > GetDetonateTime() - GRENADE_EMP_LEADIN )
	{
		Vector soundPosition = GetAbsOrigin() + Vector( 0, 0, 5 );
		CPASAttenuationFilter filter( soundPosition );

		EmitSound( filter, entindex(), "Weapon_Grenade_Emp.LeadIn" );
		m_bPlayedLeadIn = true;
	}

	BaseClass::DetonateThink();
}

#endif
