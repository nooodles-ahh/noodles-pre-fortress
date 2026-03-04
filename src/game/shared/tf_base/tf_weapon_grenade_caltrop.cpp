//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF Caltrop Grenade.
//
//=============================================================================//
#include "cbase.h"
#include "tf_weaponbase.h"
#include "tf_gamerules.h"
#include "npcevent.h"
#include "engine/IEngineSound.h"
#include "tf_weapon_grenade_caltrop.h"
#include "tf_shareddefs.h"

// Server specific.
#ifdef GAME_DLL
#include "tf_player.h"
#include "tf_shareddefs.h"
#include "items.h"
#include "tf_weaponbase_grenadeproj.h"
#include "soundent.h"
#include "KeyValues.h"
#endif

#define GRENADE_CALTROP_TIMER			3.0f //Seconds
#define GRENADE_CALTROP_RELEASE_COUNT	6
#define GRENADE_CALTROP_DAMAGE			10

//=============================================================================
//
// TF Caltrop Grenade tables.
//

IMPLEMENT_NETWORKCLASS_ALIASED( TFGrenadeCaltrop, DT_TFGrenadeCaltrop )

BEGIN_NETWORK_TABLE( CTFGrenadeCaltrop, DT_TFGrenadeCaltrop )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFGrenadeCaltrop )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_grenade_caltrop, CTFGrenadeCaltrop );
PRECACHE_WEAPON_REGISTER( tf_weapon_grenade_caltrop );

//=============================================================================
//
// TF Caltrop Grenade functions.
//

// Server specific.
#ifdef GAME_DLL

BEGIN_DATADESC( CTFGrenadeCaltrop )
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFWeaponBaseGrenadeProj *CTFGrenadeCaltrop::EmitGrenade( Vector vecSrc, QAngle vecAngles, Vector vecVel, 
							        AngularImpulse angImpulse, CBasePlayer *pPlayer, float flTime, int iflags )
{
	// Release several at a time (different directions, angles, speeds, etc.)
	for ( int i = 0 ; i < GRENADE_CALTROP_RELEASE_COUNT ; i++ )
	{
		int iIndex = i;
		while (iIndex > 6)
		{
			iIndex -= 6;
		}
		
		Vector velocity( 0, 0, 200 );
		QAngle angThrow = pPlayer->LocalEyeAngles();
		QAngle vecAngleStuff( 0, angThrow.y, 0 );

		Vector vForward, vRight, vUp;
		QAngle vecAngleThrow( angThrow.z, angThrow.y, 0 );
		vecAngleThrow.y += (g_vecCaltropFixedPattern[iIndex].y);
		AngleVectors( vecAngleThrow, &vForward, &vRight, &vUp );
		velocity += vForward * 50;

		vecAngleStuff.y += (g_vecCaltropFixedPattern[iIndex].y);


		CTFGrenadeCaltropProjectile::Create(vecSrc, vecAngleStuff, velocity, angImpulse,
			                                 pPlayer, GetTFWpnData(), random->RandomFloat( 10.0f, 15.0f ) );
	}


	return NULL;
}

#endif

//=============================================================================
//
// TF Caltrop Grenade Projectile functions
//
LINK_ENTITY_TO_CLASS( tf_weapon_grenade_caltrop_projectile, CTFGrenadeCaltropProjectile );
PRECACHE_WEAPON_REGISTER( tf_weapon_grenade_caltrop_projectile );

IMPLEMENT_NETWORKCLASS_ALIASED( TFGrenadeCaltropProjectile, DT_TFGrenadeCaltropProjectile )

BEGIN_NETWORK_TABLE( CTFGrenadeCaltropProjectile, DT_TFGrenadeCaltropProjectile )
END_NETWORK_TABLE()

#define GRENADE_MODEL "models/weapons/w_models/w_grenade_beartrap.mdl"

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeCaltropProjectile::Spawn()
{
	SetModel( GRENADE_MODEL );

	BaseClass::Spawn();

	// We want to get touch functions called so we can damage enemy players
	AddSolidFlags( FSOLID_TRIGGER );

#ifdef CLIENT_DLL
	SetNextClientThink( CLIENT_THINK_ALWAYS );
#endif

}

#ifdef GAME_DLL
//=============================================================================
//
// Server specific functions
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFGrenadeCaltropProjectile* CTFGrenadeCaltropProjectile::Create( const Vector &position, const QAngle &angles, 
																  const Vector &velocity, const AngularImpulse &angVelocity, 
																  CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo,
																  float timer, int iFlags )
{
	CTFGrenadeCaltropProjectile *pGrenade = static_cast<CTFGrenadeCaltropProjectile*>( CTFWeaponBaseGrenadeProj::Create( "tf_weapon_grenade_caltrop_projectile", position, angles, velocity, angVelocity, pOwner, weaponInfo, timer, iFlags ) );
	if ( pGrenade )
	{
		pGrenade->ApplyLocalAngularVelocityImpulse( angVelocity );
		pGrenade->SetTouch( &CTFGrenadeCaltropProjectile::Touch );
		//pGrenade->SetYRot(angles.y);
	}

	return pGrenade;
}

CTFGrenadeCaltropProjectile::CTFGrenadeCaltropProjectile()
{
	m_bBounced = false;
	m_flTimeToOpen = 0;
	m_bActive = false;
	UseClientSideAnimation();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeCaltropProjectile::Precache()
{
	PrecacheModel( GRENADE_MODEL );
	PrecacheScriptSound( "Weapon_Grenade_Caltrop.Close" );
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeCaltropProjectile::BounceSound( void )
{
	EmitSound( "Weapon_Grenade_Caltrop.Bounce" );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeCaltropProjectile::Detonate()
{

	RemoveGrenade();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeCaltropProjectile::DetonateThink( void )
{
	BaseClass::DetonateThink();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeCaltropProjectile::Touch( CBaseEntity *pOther )
{
	if( m_bRemoving )
		return;

	if( !pOther->IsPlayer() || !(pOther->GetFlags() & FL_ONGROUND) || !pOther->IsAlive() )
		return;

	// Don't hurt friendlies
	//if (pThrower == pTFPlayer || !InSameTeam( pTFPlayer ))
	if( pOther != GetThrower() && GetTeamNumber() == pOther->GetTeamNumber() )
		return;

	// Caltrops need to be on the ground. Check to see if we're still moving.
	/*Vector vecVelocity;
	VPhysicsGetObject()->GetVelocity( &vecVelocity, NULL );
	if( vecVelocity.LengthSqr() > (1*1) )
		return;*/

	if( !m_bActive )
		return;

	CTFPlayer *pPlayer = ToTFPlayer( pOther );
	if( pPlayer && !pPlayer->m_Shared.InCond( TF_COND_INVULNERABLE ) )
	{
		// Do the leg damage to the player
		CTakeDamageInfo info( this, GetThrower(), GetTFWeaponInfo( GetWeaponID() )->GetWeaponDamage( TF_WEAPON_PRIMARY_MODE ), DMG_CLUB | DMG_PREVENT_PHYSICS_FORCE );
		info.SetDamageCustom( TF_DMG_CUSTOM_CALTROP );
		pPlayer->TakeDamage( info );
		pPlayer->m_Shared.AddCond( TF_COND_LEG_DAMAGED, TF_BROKEN_LEG_DURATION );
		pPlayer->TeamFortress_SetSpeed();
	}
	CRecipientFilter filter;
	filter.AddRecipientsByPAS( GetAbsOrigin() + Vector( 0, 0, 5 ) );
	CBaseEntity::EmitSound( filter, entindex(), "Weapon_Grenade_Caltrop.Close" );

	// have the caltrop disappear
	UTIL_Remove( this );
}

void CTFGrenadeCaltropProjectile::VPhysicsUpdate( IPhysicsObject *pPhysics )
{
	// We have to be slow for a second in order to open
	if( GetAbsVelocity().Length() <= 10.0f )
		m_flTimeToOpen += gpGlobals->frametime;
	else
		m_flTimeToOpen = 0;

	// Partially Taken From the Stickybomb Function + DroptoGround
	if( !m_bRemoving && !IsEffectActive( EF_NODRAW ) && m_bBounced && m_flTimeToOpen >= 1 )
	{
		// As we delete on detonation we'll do this here
		if ( ShouldNotDetonate() )
		{
			m_bRemoving = true;
			SetDetonateTimerLength(2.0f);
			SetThink( &CTFWeaponBaseGrenadeProj::DisarmThink );
			SetNextThink(gpGlobals->curtime);
			BaseClass::VPhysicsUpdate( pPhysics );
			return;
		}
		/*IPhysicsObject *pPhysicsObject = pPhysics;
		// Stop moving
		if ( pPhysicsObject )
			pPhysicsObject->EnableMotion( false );*/

		trace_t tr;

		// Cyanide; if you are displeased with this implementation use CTraceFilterIgnorePlayers and have fun
		UTIL_TraceLine( GetAbsOrigin() + Vector( 0, 0, 4 ), // not too far up, maybe get the difference at some point?
			GetAbsOrigin() + Vector( 0, 0, -500 ), MASK_PLAYERSOLID, this, COLLISION_GROUP_DEBRIS, &tr );

		// are we about to open on a player?
		if( tr.m_pEnt && tr.m_pEnt->IsPlayer() )
		{
			m_bActive = true;
			Touch( tr.m_pEnt );
			return;
		}

		Vector vecNormal = tr.plane.normal;

		// Actually have a FLOOR underneath ( so something players can walk, no surf, on )
		if( vecNormal.z > -0.7f )
		{
			SetGroundEntity( tr.m_pEnt );

			Vector vecPos = tr.endpos;
		
			// Flip it
			vecNormal.Negate();

			// Turn it into an angle
			QAngle angUp;
			VectorAngles( vecNormal, angUp );
			angUp.x += 90;
			//angUp.y = m_flYRot; // set y angle we got from our initial emit

			// PFTODO; Determine if we even need this in conjunction with negate()
			if( tr.fraction < 1.0f )
			{
				// For some reason its flipped when inside someething so just 180 that sucker
				angUp.x += 180.0f;
				vecPos = vecPos + ( m_vecImpactNormal * -2 );
			}
			else
				vecPos = vecPos + ( m_vecImpactNormal * 2 );
			
			SetAbsOrigin(vecPos);
			SetAbsAngles(angUp);

			if( GetSequence() != LookupSequence( "open" ) )
				SetSequence( LookupSequence( "open" ) );

			SetPlaybackRate( 1.0f );

			m_bActive = true;
		}
	}
	else
		BaseClass::VPhysicsUpdate( pPhysics );
}

void CTFGrenadeCaltropProjectile::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	BaseClass::VPhysicsCollision( index, pEvent );

	int otherIndex = !index;
	CBaseEntity *pHitEntity = pEvent->pEntities[otherIndex];
	if ( !pHitEntity )
		return;

	if( !pHitEntity->IsWorld() && V_strcmp(pHitEntity->GetClassname(), "prop_dynamic") != 0 )
		return DevMsg("caltrop not colliding with a object of classname: \"%s\" \n", pHitEntity->GetClassname() );

	// Get the normal
	Vector vecNormal;
	pEvent->pInternalData->GetSurfaceNormal( vecNormal );

	if( vecNormal.z < -0.7f )
	{
		m_bBounced = true;
	}
}

void CTFGrenadeCaltropProjectile::Disarm( const CTakeDamageInfo &info )
{
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	if( pPhysicsObject )
	{
		pPhysicsObject->SetPosition( GetAbsOrigin(), GetAbsAngles(), false );
	}

	if ( GetSequence() != LookupSequence( "close" ) )
		SetSequence( LookupSequence( "close" ) );

	BaseClass::Disarm( info );
}
#else
//=============================================================================
//
// Client specific functions
//

void CTFGrenadeCaltropProjectile::ClientThink()
{
	BaseClass::ClientThink();

	StudioFrameAdvance();
	Simulate();
	// Why is this check here? It didn't allow any other animation to play so it's commented out.
	//if( IsSequenceFinished() && GetSequence() != LookupSequence("idle_open") )
	//	SetSequence( LookupSequence("idle_open") );

	SetNextClientThink( CLIENT_THINK_ALWAYS );
}
#endif