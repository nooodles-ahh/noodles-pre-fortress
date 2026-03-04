//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. ========//
//
// Purpose: TF Napalm Grenade.
//
//=============================================================================//
#ifndef TF_WEAPON_GRENADE_NAPALM_H
#define TF_WEAPON_GRENADE_NAPALM_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_grenade.h"
#include "tf_weaponbase_grenadeproj.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFGrenadeNapalm C_TFGrenadeNapalm
#define CTFGrenadeNapalmProjectile C_TFGrenadeNapalmProjectile
#endif

//=============================================================================
//
// TF Napalm Grenade
//
class CTFGrenadeNapalm : public CTFWeaponBaseGrenade
{
public:

	DECLARE_CLASS(CTFGrenadeNapalm, CTFWeaponBaseGrenade);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFGrenadeNapalm() {}

	// Unique identifier.
	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_GRENADE_NAPALM; }

// Server specific.
#ifdef GAME_DLL

	DECLARE_DATADESC();

	virtual CTFWeaponBaseGrenadeProj *EmitGrenade( Vector vecSrc, QAngle vecAngles, Vector vecVel, AngularImpulse angImpulse, CBasePlayer *pPlayer, float flTime, int iflags = 0 );

#endif

	CTFGrenadeNapalm( const CTFGrenadeNapalm & ) {}
};

#ifdef CLIENT_DLL
	#define CTFGrenadeNapalmEffect C_TFGrenadeNapalmEffect
#endif

class CTFGrenadeNapalmEffect : public CBaseEntity
{
public:
	DECLARE_CLASS( CTFGrenadeNapalmEffect, CBaseEntity );
	DECLARE_NETWORKCLASS();

	void SetBomb(bool bBomb) { m_bBomb = bBomb; }
	void SetWater(bool bInWater) { m_bInWater = bInWater; }

	CNetworkVar(bool, m_bBomb);
	CNetworkVar(bool, m_bInWater);

#ifndef CLIENT_DLL
	virtual int UpdateTransmitState( void );
#else
	CTFGrenadeNapalmEffect()
	{
		m_pNapalmEffect = NULL;
	}
	virtual void OnDataChanged( DataUpdateType_t updateType );
	CNewParticleEffect *m_pNapalmEffect;
#endif

};

//=============================================================================
//
// TF Naplam Grenade Projectile (Server specific.)
//

class CTFGrenadeNapalmProjectile : public CTFWeaponBaseGrenadeProj
{
public:

	DECLARE_CLASS( CTFGrenadeNapalmProjectile, CTFWeaponBaseGrenadeProj );
	DECLARE_NETWORKCLASS();

#ifdef GAME_DLL
	DECLARE_DATADESC();

	CTFGrenadeNapalmProjectile();
	~CTFGrenadeNapalmProjectile();

	// Unique identifier.
	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_GRENADE_NAPALM; }

	// Creation.
	static CTFGrenadeNapalmProjectile *Create( const Vector &position, const QAngle &angles, const Vector &velocity, 
		                                       const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo, float timer, int iFlags = 0 );

	// Overrides.
	virtual void	Spawn();
	virtual void	Precache();
	virtual void	BounceSound( void );
	virtual void	Detonate();
	virtual void	DetonateThink( void );
	virtual bool	ShouldNotDetonate(void);
	virtual void	VPhysicsCollision(int index, gamevcollisionevent_t* pEvent);
	virtual void	ExplodeInHand(CTFPlayer* pPlayer);
	virtual void	Explode( trace_t *pTrace, int bitsDamageType );

	void Think_Emit( void );

public:
	bool m_bIsBurning;
	bool m_bBomb;
	float m_flMaxLifetime;

private:
	float m_flBurnTime;
	CHandle<CTFGrenadeNapalmEffect> m_hNapalmEffect;

#endif
};

#endif // TF_WEAPON_GRENADE_NAPALM_H