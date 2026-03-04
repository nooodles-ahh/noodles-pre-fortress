//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======
//
//
//=============================================================================
#ifndef TF_WEAPON_FLAMETHROWER_H
#define TF_WEAPON_FLAMETHROWER_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"
#include "tf_weaponbase_rocket.h"

// Client specific.
#ifdef CLIENT_DLL
	#include "particlemgr.h"
	#include "particle_util.h"
	#include "particles_simple.h"
	#include "c_tf_projectile_rocket.h"

	#define CTFFlameThrower C_TFFlameThrower
	#define CTFFlameRocket C_TFFlameRocket
#else
	#include "tf_projectile_rocket.h"
	#include "baseentity.h"
#endif

enum FlameThrowerState_t
{
	// Firing states.
	FT_STATE_IDLE = 0,
	FT_STATE_STARTFIRING,
	FT_STATE_FIRING
};

//=========================================================
// Flamethrower Weapon
//=========================================================
class CTFFlameThrower : public CTFWeaponBaseGun
{
	DECLARE_CLASS( CTFFlameThrower, CTFWeaponBaseGun );
public:
	DECLARE_DATADESC();
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFFlameThrower();
	~CTFFlameThrower();

	virtual void Precache();

	virtual void	Spawn( void );

	virtual int		GetWeaponID( void ) const { return TF_WEAPON_FLAMETHROWER; }

	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual void	ItemPostFrame( void );
	virtual void	PrimaryAttack();
	virtual void	SecondaryAttack();
	virtual bool	CanReload( void )	{ return false; }
	virtual bool	Reload(void);
	virtual bool	Lower( void );
	virtual void	WeaponReset( void );
	virtual void	PlayWeaponShootSound(void) { WeaponSound(WPN_DOUBLE); }

	virtual void	DestroySounds( void );

	Vector GetVisualMuzzlePos();
	Vector GetFlameOriginPos();
#if defined( CLIENT_DLL )
	virtual bool	Deploy( void );

	virtual void	OnDataChanged(DataUpdateType_t updateType);
	virtual void	UpdateOnRemove( void );
	virtual void	SetDormant( bool bDormant );

	//	Start/stop flame sound and particle effects
	void			StartFlame();
	void			StopFlame( bool bAbrupt = false );

	void			RestartParticleEffect();	

	// constant pilot light sound
	void 			StartPilotLight();
	void 			StopPilotLight();

	// Player hit sound
	void StopHitSound();
#else // GAME_DLL
	void SetHitTarget();
	void HitTargetThink();
#endif

private:
	Vector GetMuzzlePosHelper( bool bVisualPos );
	CNetworkVar( int, m_iWeaponState );
	CNetworkVar( bool, m_bCritFire );
	CNetworkVar( bool, m_bTargetHit );

	float m_flStartFiringTime;
	float m_flNextPrimaryAttackAnim;

	int			m_iParticleWaterLevel;
	float		m_flAmmoUseRemainder;

#if defined( CLIENT_DLL )
	CSoundPatch	*m_pFiringStartSound;
	CSoundPatch	*m_pFiringLoop;
	bool		m_bFiringLoopCritical;
	CNewParticleEffect *m_pFlameEffect;
	EHANDLE		m_hFlameEffectHost;
	CSoundPatch *m_pPilotLightSound;
	float		flNextFailAttack;

	CSoundPatch *m_pFlameHitSound;
#else
	float m_flNextFlameHitThink;
#endif

	CTFFlameThrower( const CTFFlameThrower & );
};

#ifdef GAME_DLL

class CTFFlameEntity : public CBaseEntity
{
	DECLARE_CLASS( CTFFlameEntity, CBaseEntity );
public:

	virtual void Spawn( void );

public:
	static CTFFlameEntity *Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, int iDmgType, float m_flDmgAmount );

	void FlameThink( void );
	void CheckCollision( CBaseEntity *pOther, bool *pbHitWorld );
private:
	void OnCollide( CBaseEntity *pOther );
	void SetHitTarget();

	Vector					m_vecInitialPos;		// position the flame was fired from
	Vector					m_vecPrevPos;			// position from previous frame
	Vector					m_vecBaseVelocity;		// base velocity vector of the flame (ignoring rise effect)
	Vector					m_vecAttackerVelocity;	// velocity of attacking player at time flame was fired
	float					m_flTimeRemove;			// time at which the flame should be removed
	int						m_iDmgType;				// damage type
	float					m_flDmgAmount;			// amount of base damage
	CUtlVector<EHANDLE>		m_hEntitiesBurnt;		// list of entities this flame has burnt
	EHANDLE					m_hAttacker;			// attacking player
	int						m_iAttackerTeam;		// team of attacking player
};

#endif // GAME_DLL

#endif // TF_WEAPON_FLAMETHROWER_H
