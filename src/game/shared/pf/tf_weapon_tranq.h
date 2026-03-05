//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======
//
//
//=============================================================================
#ifndef TF_WEAPON_TRANQ_H
#define TF_WEAPON_TRANQ_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFTranq C_TFTranq
#endif

#define TRANQ_MOVEMENT_CHANGE 0.5f
#define TRANQ_SENSITIVITY_CHANGE 0.6f

//=============================================================================
//
// TF Weapon Tranq.
//
class CTFTranq : public CTFWeaponBaseGun
{
public:

	DECLARE_CLASS( CTFTranq, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

// Server specific.
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CTFTranq() {}
	~CTFTranq() {}

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_TRANQ; }
	void			ReloadCheck(void);
	virtual bool	Holster(CBaseCombatWeapon *pSwitchingTo);
	virtual bool	DefaultDeploy( char *szViewModel, char *szWeaponModel, int iActivity, char *szAnimExt );
	virtual void	Spawn();

private:
	float			m_flHolsteredTime;
	float			m_flReloadDuration;

	CTFTranq( const CTFTranq & ) {}
};

#endif // TF_WEAPON_TRANQ_H