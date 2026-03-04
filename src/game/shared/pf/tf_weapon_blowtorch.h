#ifndef TF_WEAPON_BLOWTORCH_H
#define TF_WEAPON_BLOWTORCH_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weapon_minigun.h"

#ifdef CLIENT_DLL
#define CTFBlowtorch C_TFBlowtorch
#endif

class CTFBlowtorch : public CTFMinigun
{
public:
	DECLARE_CLASS(CTFBlowtorch, CTFMinigun);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFBlowtorch();
	virtual int GetCustomDamageType() const { return TF_DMG_WRENCH_FIX; }
	virtual int GetWeaponID(void) const { return TF_WEAPON_BLOWTORCH; }
#ifdef GAME_DLL
	bool	Deploy(void);	//possibly remove when the weapon is complete
#endif
private:
	CTFBlowtorch(const CTFBlowtorch &) {}
};

#endif