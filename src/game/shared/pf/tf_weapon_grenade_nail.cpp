//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF Nail Grenade.
//
//=============================================================================//
#include "cbase.h"
#include "tf_weaponbase.h"
#include "tf_gamerules.h"
#include "npcevent.h"
#include "engine/IEngineSound.h"
#include "tf_weapon_grenade_nail.h"

// Server specific.
#ifdef GAME_DLL
#include "tf_player.h"
#include "items.h"
#include "tf_weaponbase_grenadeproj.h"
#include "soundent.h"
#include "KeyValues.h"
#include "tf_projectile_nail.h"
#include "physics_saverestore.h"
#include "phys_controller.h"
#endif

#define GRENADE_NAIL_TIMER	3.0f //Seconds

//=============================================================================
//
// TF Nail Grenade tables.
//

IMPLEMENT_NETWORKCLASS_ALIASED( TFGrenadeNail, DT_TFGrenadeNail )

BEGIN_NETWORK_TABLE( CTFGrenadeNail, DT_TFGrenadeNail )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFGrenadeNail )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_grenade_nail, CTFGrenadeNail );
PRECACHE_WEAPON_REGISTER( tf_weapon_grenade_nail );



//=============================================================================
//
// TF Nail Grenade functions.
//

// Server specific.
#ifdef GAME_DLL
ConVar ng_bursts("ng_bursts", "20", FCVAR_NOTIFY);
ConVar ng_nails("ng_nails", "5", FCVAR_NOTIFY);
ConVar ng_angleinc("ng_angleinc", "10", FCVAR_NOTIFY);

BEGIN_DATADESC( CTFGrenadeNail )
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFWeaponBaseGrenadeProj *CTFGrenadeNail::EmitGrenade( Vector vecSrc, QAngle vecAngles, Vector vecVel, 
							        AngularImpulse angImpulse, CBasePlayer *pPlayer, float flTime, int iflags )
{
	return CTFGrenadeNailProjectile::Create( vecSrc, vecAngles, vecVel, angImpulse, 
		                              pPlayer, GetTFWpnData(), flTime );
}

#endif
IMPLEMENT_NETWORKCLASS_ALIASED(TFGrenadeNailProjectile, DT_TFGrenadeNailProjectile)

BEGIN_NETWORK_TABLE(CTFGrenadeNailProjectile, DT_TFGrenadeNailProjectile)
END_NETWORK_TABLE()

//=============================================================================
//
// TF Normal Grenade functions.
//

//=============================================================================
//
// TF Nail Grenade Projectile functions (Server specific).
//
#ifdef GAME_DLL

BEGIN_DATADESC( CTFGrenadeNailProjectile )
	DEFINE_THINKFUNC( EmitNails ),
	DEFINE_EMBEDDED( m_GrenadeController ),
	DEFINE_PHYSPTR( m_pMotionController ),
END_DATADESC()

#define GRENADE_MODEL "models/weapons/w_models/w_grenade_nail.mdl"
#define GRENADE_SOUND "Weapon_Grenade_Nail.Launch"

LINK_ENTITY_TO_CLASS( tf_weapon_grenade_nail_projectile, CTFGrenadeNailProjectile );
PRECACHE_WEAPON_REGISTER( tf_weapon_grenade_nail_projectile );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFGrenadeNailProjectile* CTFGrenadeNailProjectile::Create( const Vector &position, const QAngle &angles, 
																const Vector &velocity, const AngularImpulse &angVelocity, 
																CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo, float timer, int iFlags )
{
	// Nail grenades are always thrown like discs
	QAngle vecCustomAngles = angles;
	if( pf_grenades_tfc.GetBool() )
	{
		vecCustomAngles.x = 0;
		vecCustomAngles.z = 0;
	}
	else
	{
		vecCustomAngles.x = clamp( vecCustomAngles.x, -10,10 );
		vecCustomAngles.z = clamp( vecCustomAngles.x, -10,10 );
	}
	Vector vecCustomAngVelocity = vec3_origin;
	CTFGrenadeNailProjectile *pGrenade = static_cast<CTFGrenadeNailProjectile*>( 
		CTFWeaponBaseGrenadeProj::Create( "tf_weapon_grenade_nail_projectile", 
											position, 
											vecCustomAngles, 
											velocity, 
											vecCustomAngVelocity, 
											pOwner, 
											weaponInfo, 
											timer, 
											iFlags ) 
										);
	if( pf_grenades_tfc.GetBool() && pGrenade )
	{
		pGrenade->ApplyLocalAngularVelocityImpulse( Vector(0, 0, 1000) );
	}
	return pGrenade;
}

CTFGrenadeNailProjectile::~CTFGrenadeNailProjectile()
{
	if ( m_pMotionController != NULL )
	{
		physenv->DestroyMotionController( m_pMotionController );
		m_pMotionController = NULL;
	}
	StopSound(GRENADE_SOUND);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeNailProjectile::Spawn()
{
	SetModel( GRENADE_MODEL );

	m_pMotionController = NULL;

	m_bActivated = false;

	UseClientSideAnimation();
		
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeNailProjectile::Precache()
{
	PrecacheModel( GRENADE_MODEL );
	PrecacheScriptSound( GRENADE_SOUND );
	PrecacheScriptSound( "Weapon_Grenade_Nail.Single" );
	PrecacheScriptSound( "Weapon_Grenade_Nail.Bounce" );
	PrecacheScriptSound( "Weapon_Grenade_Nail.Disarm" );
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeNailProjectile::BounceSound( void )
{
	EmitSound( "Weapon_Grenade_Nail.Bounce" );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeNailProjectile::Detonate()
{
	if ( ShouldNotDetonate() )
	{
		RemoveGrenade();
		return;
	}

	if( m_iHealth <= 0 )
	{
		BaseClass::Detonate();
		return;
	}

	StartEmittingNails();
}

void CTFGrenadeNailProjectile::PlayDisarmSound()
{
	if(!m_bActivated)
	{
		BaseClass::PlayDisarmSound();
	}
}

void CTFGrenadeNailProjectile::Disarm( const CTakeDamageInfo &info )
{
	if(m_bActivated)
	{
		// re-enable damage
		m_takedamage = DAMAGE_YES;
	
		SetCollisionGroup( TF_COLLISIONGROUP_GRENADES );

		// play the stopping sound
		CPASAttenuationFilter filter( GetAbsOrigin() );
		EmitSound( filter, entindex(), "Weapon_Grenade_Nail.Disarm" );
		
		// change back tot he default animation
		int animDesired = SelectWeightedSequence( ACT_IDLE );
		ResetSequence( animDesired );
		SetPlaybackRate( 1.0 );
	
		// Destroy the physics controller
		IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
		if(pPhysicsObject)
		{
			if ( m_pMotionController )
			{
				m_pMotionController->DetachObject( pPhysicsObject );
				physenv->DestroyMotionController( m_pMotionController );
				m_pMotionController = NULL;
			}
	
			// let the grenade fall out of the sky
			pPhysicsObject->EnableGravity( true );
			pPhysicsObject->Wake();
		}
	}

	BaseClass::Disarm(info);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CTFGrenadeNailProjectile::OnTakeDamage( const CTakeDamageInfo &info )
{
	CTakeDamageInfo info2 = info;
	if ( m_pMotionController != NULL )
	{
		info2.ScaleDamageForce(0.0f);
	}

	return BaseClass::OnTakeDamage( info2 );
}

void CTFGrenadeNailProjectile::StartEmittingNails( void )
{
	// Reset health just incase people were shooting at it
	SetHealth( 900 );

	// 0.4 seconds later, emit nails
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	
	if ( pPhysicsObject )
	{
		m_pMotionController = physenv->CreateMotionController( &m_GrenadeController );
		m_pMotionController->AttachObject( pPhysicsObject, true );
		pPhysicsObject->EnableGravity( false );
		pPhysicsObject->Wake();
		
		if(pf_grenades_tfc.GetBool())
		{
			SetMoveType( MOVETYPE_VPHYSICS, MOVECOLLIDE_FLY_BOUNCE );
			// sync actual position and physics object position?
			pPhysicsObject->SetPosition(GetAbsOrigin(), GetAbsAngles(), false);
		}
		
	}

	m_bActivated = true;

	//RemoveSolidFlags( FSOLID_NOT_STANDABLE );
	//SetCollisionGroup( COLLISION_GROUP_INTERACTIVE );

	//m_takedamage = DAMAGE_EVENTS_ONLY;

	QAngle ang(0,0,0);
	Vector pos = GetAbsOrigin();
	pos.z += 32;
	m_GrenadeController.SetDesiredPosAndOrientation( pos, ang );

	m_flNailAngle = 0;
	m_iNumNailBurstsLeft = ng_bursts.GetInt();

	int animDesired = SelectWeightedSequence( ACT_RANGE_ATTACK1 );
	ResetSequence( animDesired );
	SetPlaybackRate( 1.0 );

	Vector soundPosition = GetAbsOrigin() + Vector( 0, 0, 5 );
	CPASAttenuationFilter filter( soundPosition );
	EmitSound( filter, entindex(), GRENADE_SOUND );

#ifdef GAME_DLL
	SetThink( &CTFGrenadeNailProjectile::EmitNails );
	SetNextThink( gpGlobals->curtime + 0.4 );
#endif
}

void CTFGrenadeNailProjectile::EmitNails( void )
{
	m_iNumNailBurstsLeft--;

	float flDamage = GetTFWeaponInfo(GetWeaponID())->GetWeaponDamage(TF_WEAPON_SECONDARY_MODE);

	if ( m_iNumNailBurstsLeft < 0 )
	{
		BaseClass::Detonate();
		return;
	}

	Vector forward, up;
	float flAngleToAdd = 360.0f/ng_nails.GetFloat(); //36

	// else release some nails
	for ( int i=0; i < ng_nails.GetInt() ;i++ )
	{
		m_flNailAngle = UTIL_AngleMod( m_flNailAngle + flAngleToAdd );

		QAngle angNail( random->RandomFloat( -3, 3 ), m_flNailAngle, 0 );

		// Emit a nail

		CTFProjectile_Nail *pNail = CTFProjectile_Nail::Create( GetAbsOrigin(), angNail, this, GetThrower() );	
		if ( pNail )
		{
			pNail->SetWeaponID( GetWeaponID() );
			pNail->SetDamage( flDamage );
		}
	}
	m_flNailAngle = UTIL_AngleMod( m_flNailAngle + ng_angleinc.GetFloat() );

	// why must source force my hand this way
	CPASAttenuationFilter filter( GetAbsOrigin() );
	EmitSound( filter, -1, "Weapon_Grenade_Nail.Single", &GetAbsOrigin() );

	SetNextThink( gpGlobals->curtime + (4.0f/ng_bursts.GetFloat()) );
}

int	CTFGrenadeNailProjectile::GetDamageType()
{
	int iDmgType = g_aWeaponDamageTypes[GetWeaponID()];
	iDmgType |= DMG_HALF_FALLOFF;

	return iDmgType;
}

IMotionEvent::simresult_e CNailGrenadeController::Simulate( IPhysicsMotionController *pController, IPhysicsObject *pObject, float deltaTime, Vector &linear, AngularImpulse &angular )
{
	if( !pController || !pObject )
		return SIM_NOTHING;

	// Try to get to m_vecDesiredPosition
	// Try to orient ourselves to m_angDesiredOrientation

	Vector currentPos;
	QAngle currentAng;

	pObject->GetPosition( &currentPos, &currentAng );

	Vector vecVel;
	AngularImpulse angVel;
	pObject->GetVelocity( &vecVel, &angVel );

	linear.Init();
	angular.Init();

	if ( m_bReachedPos )
	{
		// Lock at this height
		if ( vecVel.Length() > 1.0 )
		{
			AngularImpulse nil( 0,0,0 );
			pObject->SetVelocityInstantaneous( &vec3_origin, &nil );

			// For now teleport to the proper orientation
			currentAng.x = 0;
			currentAng.y = 0;
			currentAng.z = 0;
			pObject->SetPosition( currentPos, currentAng, true );
		}
	}
	else
	{
		// not at the right height yet, keep moving up
		linear.z =  50 * ( m_vecDesiredPosition.z - currentPos.z );

		if ( currentPos.z > m_vecDesiredPosition.z )
		{
			// lock into position
			m_bReachedPos = true;
		}

		// Start rotating in the right direction
		// we'll lock angles once we reach proper height to stop the oscillating
		matrix3x4_t matrix;
		// get the object's local to world transform
		pObject->GetPositionMatrix( &matrix );

		Vector m_worldGoalAxis(0,0,1);

		// Get the alignment axis in object space
		Vector currentLocalTargetAxis;
		VectorIRotate( m_worldGoalAxis, matrix, currentLocalTargetAxis );

		float invDeltaTime = (1/deltaTime);
		float m_angularLimit = 10;

		angular = ComputeRotSpeedToAlignAxes( m_worldGoalAxis, currentLocalTargetAxis, angVel, 1.0, invDeltaTime * invDeltaTime, m_angularLimit * invDeltaTime );
	}

	return SIM_GLOBAL_ACCELERATION;
}

void CNailGrenadeController::SetDesiredPosAndOrientation( Vector pos, QAngle orientation )
{
	m_vecDesiredPosition = pos;
	m_angDesiredOrientation = orientation;

	m_bReachedPos = false;
	m_bReachedOrientation = false;
}

BEGIN_SIMPLE_DATADESC( CNailGrenadeController )
END_DATADESC()

#endif
