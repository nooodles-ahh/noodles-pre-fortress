//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_BRANDINGIRON_H
#define TF_WEAPON_BRANDINGIRON_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_melee.h"

#ifdef CLIENT_DLL
#define CTFBrandingIron C_TFBrandingIron
#endif

//=============================================================================
//
// BrandingIron class.
//
class CTFBrandingIron : public CTFWeaponBaseMelee
{
public:

	DECLARE_CLASS(CTFBrandingIron, CTFWeaponBaseMelee );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFBrandingIron() {}
	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_BRANDINGIRON; }

private:

	CTFBrandingIron( const CTFBrandingIron& ) {}
};

#endif // TF_WEAPON_BRANDINGIRON_H
