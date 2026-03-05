//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. ========//
//
// Purpose: TF Caltrop Grenade.
//
//=============================================================================//
#ifndef TF_WEAPON_GRENADE_CALTROP_H
#define TF_WEAPON_GRENADE_CALTROP_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_grenade.h"
#include "tf_weaponbase_grenadeproj.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFGrenadeCaltrop C_TFGrenadeCaltrop
#define CTFGrenadeCaltropProjectile C_TFGrenadeCaltropProjectile
#endif

#define CALTROP_MOVEMENT_CHANGE 0.5f

//=============================================================================
//
// TF Caltrop Grenade
//
class CTFGrenadeCaltrop : public CTFWeaponBaseGrenade
{
public:

	DECLARE_CLASS( CTFGrenadeCaltrop, CTFWeaponBaseGrenade );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
//	DECLARE_ACTTABLE();

	CTFGrenadeCaltrop() {}

	// Unique identifier.
	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_GRENADE_CALTROP; }

// Server specific.
#ifdef GAME_DLL

	DECLARE_DATADESC();

	virtual CTFWeaponBaseGrenadeProj *EmitGrenade( Vector vecSrc, QAngle vecAngles, Vector vecVel, AngularImpulse angImpulse, CBasePlayer *pPlayer, float flTime, int iflags = 0 );

#endif

	CTFGrenadeCaltrop( const CTFGrenadeCaltrop & ) {}
};

//=============================================================================
//
// TF Caltrop Grenade Projectile (Server specific.)
//
class CTFGrenadeCaltropProjectile : public CTFWeaponBaseGrenadeProj
{
public:
	DECLARE_CLASS( CTFGrenadeCaltropProjectile, CTFWeaponBaseGrenadeProj );
	DECLARE_NETWORKCLASS();

	// Overrides.
	virtual void	Spawn( void );

	// Unique identifier.
	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_GRENADE_CALTROP; }

#ifdef GAME_DLL
	CTFGrenadeCaltropProjectile();

	// Creation.
	static CTFGrenadeCaltropProjectile *Create( const Vector &position, const QAngle &angles, const Vector &velocity, 
		                                       const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo, float timer, int iFlags = 0 );
	virtual void	Precache( void );
	virtual void	BounceSound( void );
	virtual void	Detonate( void );
	virtual void	DetonateThink( void );

	virtual void	Touch( CBaseEntity *pOther );
	void			VPhysicsUpdate( IPhysicsObject *pPhysics );
	virtual void	VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );
	virtual void	Disarm( const CTakeDamageInfo &info );
	//void			SetYRot(float y) { m_flYRot = y; }
#else // CLIENT_DLL
	virtual void	ClientThink( void );
#endif

private:

	float m_flDetonateTime;

#ifdef GAME_DLL
	bool	m_bBounced;
	float	m_flTimeToOpen;
	float	m_flYRot; // need to save this for when we open clatrops
	bool	m_bActive;
#endif
};

#endif // TF_WEAPON_GRENADE_CALTROP_H
