//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_SYRINGE_H
#define TF_WEAPON_SYRINGE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_melee.h"

#ifdef CLIENT_DLL
#define CTFSyringe C_TFSyringe
#endif

//=============================================================================
//
// Bonesaw class.
//
class CTFSyringe : public CTFWeaponBaseMelee
{
public:

	DECLARE_CLASS(CTFSyringe, CTFWeaponBaseMelee );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFSyringe() {}
	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_SYRINGE; }
#ifdef GAME_DLL
	void				OnEntityHit(CBaseEntity *pEntity);
#endif
	virtual bool DoSwingTrace( trace_t &tr );
	virtual float		GetMeleeDamage( CBaseEntity *pTarget, int &iCustomDamage );
	virtual float GetSwingRange() { return 60.0f; }

private:

	CTFSyringe( const CTFSyringe & ) {}
};

#endif // TF_WEAPON_SYRINGE_H
