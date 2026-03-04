//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF basic grenade projectile functionality.
//
//=============================================================================//
#ifndef TF_WEAPONBASE_GRENADEPROJ_H
#define TF_WEAPONBASE_GRENADEPROJ_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_shareddefs.h"
#include "tf_weaponbase.h"
#include "basegrenade_shared.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFWeaponBaseGrenadeProj C_TFWeaponBaseGrenadeProj
#endif


extern ConVar sv_gravity;
#ifdef GAME_DLL
extern ConVar tf_grenade_show_radius_time;
extern ConVar tf_grenade_show_radius;
extern ConVar pf_grenades_tfc;
//-----------------------------------------------------------------------------
// Purpose: This will hit only things that are in newCollisionGroup, but NOT in collisionGroupAlreadyChecked
//			Always ignores other grenade projectiles.
//-----------------------------------------------------------------------------
class CTraceFilterCollisionGrenades : public CTraceFilterEntitiesOnly
{
public:
	// It does have a base, but we'll never network anything below here..
	DECLARE_CLASS_NOBASE( CTraceFilterCollisionGrenades );

	CTraceFilterCollisionGrenades( const IHandleEntity *passentity, const IHandleEntity *passentity2 )
		: m_pPassEnt(passentity), m_pPassEnt2(passentity2)
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
	{
		if ( !PassServerEntityFilter( pHandleEntity, m_pPassEnt ) )
			return false;
		CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );
		if ( pEntity )
		{
			if ( pEntity == m_pPassEnt2 )
				return false;
			if ( pEntity->GetCollisionGroup() == TF_COLLISIONGROUP_GRENADES )
				return false;
			if ( pEntity->GetCollisionGroup() == TFCOLLISION_GROUP_ROCKETS )
				return false;
			if ( pEntity->GetCollisionGroup() == COLLISION_GROUP_DEBRIS )
				return false;
			if ( pEntity->GetCollisionGroup() == COLLISION_GROUP_NONE )
				return false;
			if( pEntity->GetCollisionGroup() == TFCOLLISION_GROUP_RESPAWNROOMS )
				return false;

			return true;
		}

		return true;
	}

protected:
	const IHandleEntity *m_pPassEnt;
	const IHandleEntity *m_pPassEnt2;
};
#endif

//=============================================================================
//
// TF base grenade projectile class.
//
class CTFWeaponBaseGrenadeProj : public CBaseGrenade
{
public:

	DECLARE_CLASS( CTFWeaponBaseGrenadeProj, CBaseGrenade );
	DECLARE_NETWORKCLASS();

							CTFWeaponBaseGrenadeProj();
	virtual					~CTFWeaponBaseGrenadeProj();
	virtual void			Spawn();
	virtual void			Precache();

	void					InitGrenade( const Vector &velocity, const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo );

	// Unique identifier.
	virtual int GetWeaponID( void ) const { return TF_WEAPON_NONE; }

	virtual const char* GetTrailParticleName(void);
	virtual const char* GetPulseParticleName(void);
	virtual const char* GetFinalPulseParticleName(void);

	// This gets sent to the client and placed in the client's interpolation history
	// so the projectile starts out moving right off the bat.
	CNetworkVector( m_vInitialVelocity );

	virtual float		GetShakeAmplitude( void ) { return 10.0; }
	virtual float		GetShakeRadius( void ) { return 300.0; }

	void				SetCritical( bool bCritical ) { m_bCritical = bCritical; }
	virtual int			GetDamageType();
	virtual bool		IsPillGrenade() { return false; }

private:

	CTFWeaponBaseGrenadeProj( const CTFWeaponBaseGrenadeProj & );

	// Client specific.
#ifdef CLIENT_DLL

public:

	virtual void			OnDataChanged( DataUpdateType_t type );


	float					m_flSpawnTime;
	bool					m_bCritical;
	bool					m_bGrenade;

	// Server specific.
#else

public:

	DECLARE_DATADESC();

	unsigned int	PhysicsSolidMaskForEntity( void ) const;

	static CTFWeaponBaseGrenadeProj *Create( const char *szName, const Vector &position, const QAngle &angles, 
				const Vector &velocity, const AngularImpulse &angVelocity, 
				CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo, int iFlags );
	static CTFWeaponBaseGrenadeProj* Create(const char* szName, const Vector& position, const QAngle& angles,
				const Vector& velocity, const AngularImpulse& angVelocity,
				CBaseCombatCharacter* pOwner, const CTFWeaponInfo& weaponInfo, float timer, int iFlags);

	int						OnTakeDamage( const CTakeDamageInfo &info );

	virtual void			DetonateThink( void );
	void					Detonate( void );
	virtual void			ExplodeInHand(CTFPlayer* pPlayer);

	void					BeepThink(void);

	void					SetupInitialTransmittedGrenadeVelocity( const Vector &velocity )	{ m_vInitialVelocity = velocity; }

	bool					ShouldNotDetonate( void );
	void					RemoveGrenade( bool bBlinkOut = true );

	void					SetTimer( float time ){ m_flDetonateTime = time; }
	float					GetDetonateTime( void ){ return m_flDetonateTime; }

	void					SetDetonateTimerLength( float timer );

	void					VPhysicsUpdate( IPhysicsObject *pPhysics );

	void					Explode( trace_t *pTrace, int bitsDamageType );

	bool					UseImpactNormal()							{ return m_bUseImpactNormal; }
	const Vector			&GetImpactNormal( void ) const				{ return m_vecImpactNormal; }

	virtual bool			RadiusHit( const Vector &vecSrc, CBaseEntity *pInflictor, CBaseEntity *pInflicted );

	virtual void			Disarm( const CTakeDamageInfo &info );
	virtual void			DisarmThink( void );
	virtual void			PlayDisarmSound();

protected:

	// Custom collision to allow for constant elasticity on hit surfaces.
	virtual void			ResolveFlyCollisionCustom( trace_t &trace, Vector &vecVelocity );

	void					DrawRadius( float flRadius, float flLifetime = 5.0f );

	bool					m_bUseImpactNormal;
	Vector					m_vecImpactNormal;

	int						m_nDisarmShots;

	bool					m_bRemoving;

private:

	float					m_flDetonateTime;

	bool					m_bInSolid;

	CNetworkVar( bool,		m_bCritical );
	CNetworkVar( bool, m_bGrenade );

	float					m_flCollideWithTeammatesTime;
	bool					m_bCollideWithTeammates;

#endif
};

#endif // TF_WEAPONBASE_GRENADEPROJ_H
