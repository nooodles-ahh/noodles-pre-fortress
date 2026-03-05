//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======
//
// TF Nail Projectile
//
//=============================================================================
#ifndef TF_PROJECTILE_NAIL_H
#define TF_PROJECTILE_NAIL_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "tf_projectile_base.h"

//-----------------------------------------------------------------------------
// Purpose: Identical to a nail except for model used
//-----------------------------------------------------------------------------
class CTFProjectile_Syringe : public CTFBaseProjectile
{
	DECLARE_CLASS( CTFProjectile_Syringe, CTFBaseProjectile );

public:

	// Creation.
	static CTFProjectile_Syringe *Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner = NULL, CBaseEntity *pScorer = NULL, 
											bool bCritical = false, float flVelocity = 1000.0, float flGravity = 0.3f );	

	virtual const char *GetProjectileModelName( void );
};

//-----------------------------------------------------------------------------
// Purpose: Nail
//-----------------------------------------------------------------------------
class CTFProjectile_Nail : public CTFBaseProjectile
{
	DECLARE_CLASS( CTFProjectile_Nail, CTFBaseProjectile );

public:

	// Creation.
	static CTFProjectile_Nail* Create(const Vector& vecOrigin, const QAngle& vecAngles, CBaseEntity* pOwner = NULL, CBaseEntity* pScorer = NULL, 
										bool bCritical = false, float flVelocity = 1000.0f, float flGravity = 0.15f);

	virtual const char *GetProjectileModelName(void);
};

//-----------------------------------------------------------------------------
// Purpose: Tranq Dart
//-----------------------------------------------------------------------------
class CTFProjectile_Dart : public CTFBaseProjectile
{
	DECLARE_CLASS( CTFProjectile_Dart, CTFBaseProjectile );

public:

	// Creation.
	static CTFProjectile_Dart* Create(const Vector& vecOrigin, const QAngle& vecAngles,  CBaseEntity* pOwner = NULL, CBaseEntity* pScorer = NULL, 
										bool bCritical = false, float flVelocity = 3000.0f, float flGravity = 0.001f);

	virtual const char* GetProjectileModelName(void);
};
#endif	//TF_PROJECTILE_NAIL_H