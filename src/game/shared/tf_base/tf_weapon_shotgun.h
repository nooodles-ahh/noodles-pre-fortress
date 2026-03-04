//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_SHOTGUN_H
#define TF_WEAPON_SHOTGUN_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"

#if defined( CLIENT_DLL )
#define CTFShotgun C_TFShotgun
#define CTFShotgun_Soldier C_TFShotgun_Soldier
#define CTFShotgun_HWG C_TFShotgun_HWG
#define CTFShotgun_Pyro C_TFShotgun_Pyro
#ifdef PF2_CLIENT
#define CTFShotgun_Scout C_TFShotgun_Scout
#else
#define CTFScatterGun C_TFScatterGun
#endif
#endif

// Reload Modes
enum eReloadModes
{
	TF_WEAPON_SHOTGUN_RELOAD_START = 0,
	TF_WEAPON_SHOTGUN_RELOAD_SHELL,
	TF_WEAPON_SHOTGUN_RELOAD_CONTINUE,
	TF_WEAPON_SHOTGUN_RELOAD_FINISH
};

//=============================================================================
//
// Shotgun class.
//
class CTFShotgun : public CTFWeaponBaseGun
{
public:

	DECLARE_CLASS( CTFShotgun, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFShotgun();

	virtual int		GetWeaponID( void ) const { return TF_WEAPON_SHOTGUN_PRIMARY; }
	virtual void	PrimaryAttack();

protected:

	void			Fire( CTFPlayer *pPlayer );
	void			UpdatePunchAngles( CTFPlayer *pPlayer );

private:

	CTFShotgun( const CTFShotgun & ) {}
};

#ifdef PF2
class CTFShotgun_Scout : public CTFShotgun
{
public:
	DECLARE_CLASS( CTFShotgun_Scout, CTFShotgun );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	virtual int		GetWeaponID( void ) const { return TF_WEAPON_SHOTGUN_SCOUT; }
};
#else
// Scout version. Different models, possibly different behaviour later on
class CTFScatterGun : public CTFShotgun
{
public:
	DECLARE_CLASS( CTFScatterGun, CTFShotgun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_SCATTERGUN; }
};
#endif

class CTFShotgun_Soldier : public CTFShotgun
{
public:
	DECLARE_CLASS( CTFShotgun_Soldier, CTFShotgun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual int		GetWeaponID( void ) const { return TF_WEAPON_SHOTGUN_SOLDIER; }
};

// Secondary version. Different weapon slot, different ammo
class CTFShotgun_HWG : public CTFShotgun
{
public:
	DECLARE_CLASS( CTFShotgun_HWG, CTFShotgun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual int		GetWeaponID( void ) const { return TF_WEAPON_SHOTGUN_HWG; }
};

class CTFShotgun_Pyro : public CTFShotgun
{
public:
	DECLARE_CLASS( CTFShotgun_Pyro, CTFShotgun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual int		GetWeaponID( void ) const { return TF_WEAPON_SHOTGUN_PYRO; }
};

#endif // TF_WEAPON_SHOTGUN_H
