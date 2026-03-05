//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_CUBEMAP_H
#define TF_WEAPON_CUBEMAP_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_melee.h"

#ifdef CLIENT_DLL
#define CTFCubemap C_TFCubemap
#endif

//=============================================================================
//
// Bat class.
//
class CTFCubemap : public CTFWeaponBaseMelee
{
public:

	DECLARE_CLASS( CTFCubemap, CTFWeaponBaseMelee );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFCubemap();

	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_CUBEMAP; }

	// Special weapon for mappers, can't hit with it
	virtual void		PrimaryAttack( void ) {}
	virtual void		SecondaryAttack( void ) {}

private:
	CTFCubemap ( const CTFCubemap & ) {}
};

#endif // TF_WEAPON_BAT_H
