//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======
//
// TF Rocket Projectile
//
//=============================================================================
#ifndef TF_PROJECTILE_ROCKET_H
#define TF_PROJECTILE_ROCKET_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_rocket.h"
#include "iscorer.h"

//=============================================================================
//
// Generic rocket.
//
class CTFProjectile_Rocket : public CTFBaseRocket, public IScorer
{
public:

	DECLARE_CLASS( CTFProjectile_Rocket, CTFBaseRocket );
	DECLARE_NETWORKCLASS();


	virtual float	GetRadius() { return TF_ROCKET_RADIUS; }

	// Creation.
	static CTFProjectile_Rocket *Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner = NULL, CBaseEntity *pScorer = NULL, float flVelocity = 1100.0f);	
	virtual void Spawn();
	virtual void Precache();

	// IScorer interface
	virtual CBasePlayer *GetScorer( void );
	virtual CBasePlayer *GetAssistant( void ) { return NULL; }

	void	SetScorer( CBaseEntity *pScorer );

	void	SetCritical( bool bCritical ) { m_bCritical = bCritical; }
	virtual int		GetDamageType();

private:
	CBaseHandle m_Scorer;
	CNetworkVar( bool,	m_bCritical );
};

#define TF_FLAMETHROWER_ROCKET_BURN_RADIUS			198

// Generic flame rocket.
class CTFFlameRocket : public CTFProjectile_Rocket
{
	DECLARE_CLASS(CTFFlameRocket, CTFProjectile_Rocket);
public:
	DECLARE_NETWORKCLASS();

	virtual int		GetWeaponID(void) const		{ return TF_WEAPON_FLAMETHROWER_ROCKET; }
	virtual float	GetRadius() { return TF_FLAMETHROWER_ROCKET_BURN_RADIUS; }
	void		WaterCheck();
	virtual void		Spawn();

#ifdef GAME_DLL
	// Creation.
	static CTFFlameRocket *Create(const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner = NULL, CBaseEntity *pScorer = NULL, float flVelocity = 650.0f);
	//virtual void	Spawn();
	//virtual void	Precache();
#else
	virtual const char *GetTrailParticleName(void) { return "incentrail"; }
#endif
};

#endif	//TF_PROJECTILE_ROCKET_H