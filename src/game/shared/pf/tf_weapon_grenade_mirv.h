//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. ========//
//
// Purpose: TF Mirv Grenade.
//
//=============================================================================//
#ifndef TF_WEAPON_GRENADE_MIRV_H
#define TF_WEAPON_GRENADE_MIRV_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_grenade.h"
#include "tf_weaponbase_grenadeproj.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFGrenadeMirv C_TFGrenadeMirv
#define CTFGrenadeMirvProjectile C_TFGrenadeMirvProjectile
#endif

//=============================================================================
//
// TF Mirv Grenade
//
class CTFGrenadeMirv : public CTFWeaponBaseGrenade
{
public:

	DECLARE_CLASS( CTFGrenadeMirv, CTFWeaponBaseGrenade );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFGrenadeMirv() {}

	// Unique identifier.
	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_GRENADE_MIRV; }

// Server specific.
#ifdef GAME_DLL

	DECLARE_DATADESC();

	virtual CTFWeaponBaseGrenadeProj *EmitGrenade( Vector vecSrc, QAngle vecAngles, Vector vecVel, AngularImpulse angImpulse, CBasePlayer *pPlayer, float flTime, int iflags = 0 );

#endif

	CTFGrenadeMirv( const CTFGrenadeMirv & ) {}
};
//=============================================================================
//
// TF Mirv Grenade Projectile and Bombs (Server specific.)
//


class CTFGrenadeMirvProjectile : public CTFWeaponBaseGrenadeProj
{
public:

	DECLARE_CLASS( CTFGrenadeMirvProjectile, CTFWeaponBaseGrenadeProj );
	DECLARE_NETWORKCLASS();

	CTFGrenadeMirvProjectile();
	~CTFGrenadeMirvProjectile();
#ifndef CLIENT_DLL
	// Unique identifier.
	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_GRENADE_MIRV; }

	// Creation.
	static CTFGrenadeMirvProjectile *Create( const Vector &position, const QAngle &angles, const Vector &velocity, 
		                                     const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo, float timer, int iFlags = 0 );

	// Overrides.
	virtual void	Spawn();
	virtual void	Precache();
	virtual void	BounceSound( void );
	virtual void	Detonate();
	virtual void	Explode( trace_t *pTrace, int bitsDamageType );
	void			DetonateThink( void );

	DECLARE_DATADESC();
#endif
private:

	bool			m_bPlayedLeadIn;
};
#ifdef GAME_DLL
class CTFGrenadeMirvBomb : public CTFWeaponBaseGrenadeProj
{
public:

	DECLARE_CLASS( CTFGrenadeMirvBomb, CTFWeaponBaseGrenadeProj );

	CTFGrenadeMirvBomb();
	~CTFGrenadeMirvBomb();

	// Creation.
	static CTFGrenadeMirvBomb *Create( const Vector &position, const QAngle &angles, const Vector &velocity, 
		                               const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, float timer );

	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_GRENADE_MIRVBOMB; }

	virtual void	Spawn();
	virtual void	Precache();
	virtual void	Detonate();
	virtual void	BounceSound( void );
};

#endif

#endif // TF_WEAPON_GRENADE_MIRV_H
