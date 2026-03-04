//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======
//
//
//=============================================================================
#ifndef TF_WEAPON_SMG_H
#define TF_WEAPON_SMG_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFSMG C_TFSMG
#ifdef PF2_CLIENT
#define CTFSMG_Scout C_TFSMG_Scout
#define CTFSMG_Medic C_TFSMG_Medic
#endif
#endif

//=============================================================================
//
// TF Weapon Sub-machine gun.
//
class CTFSMG : public CTFWeaponBaseGun
{
public:

	DECLARE_CLASS( CTFSMG, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

// Server specific.
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CTFSMG() {}
	~CTFSMG() {}

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_SMG; }

private:

	CTFSMG( const CTFSMG & ) {}
};

#ifdef PF2
// Scout version
class CTFSMG_Scout : public CTFSMG
{
public:
	DECLARE_CLASS(CTFSMG_Scout, CTFSMG);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	virtual int		GetWeaponID(void) const { return TF_WEAPON_SMG_SCOUT; }
};

// Scout version
class CTFSMG_Medic : public CTFSMG
{
public:
	DECLARE_CLASS(CTFSMG_Medic, CTFSMG);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	virtual int		GetWeaponID(void) const { return TF_WEAPON_SMG_MEDIC; }
};
#endif
#endif // TF_WEAPON_SMG_H