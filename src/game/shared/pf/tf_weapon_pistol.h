//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
//
//=============================================================================
#ifndef TF_WEAPON_PISTOL_H
#define TF_WEAPON_PISTOL_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFPistol C_TFPistol
#define CTFPistol_Scout C_TFPistol_Scout
#define CTFPistol_Spy C_TFPistol_Spy
#endif

// We allow the pistol to fire as fast as the player can click.
// This is the minimum time between shots.
#define	PISTOL_FASTEST_REFIRE_TIME		0.1f

// The faster the player fires, the more inaccurate he becomes
#define	PISTOL_ACCURACY_SHOT_PENALTY_TIME		0.2f	// Applied amount of time each shot adds to the time we must recover from
#define	PISTOL_ACCURACY_MAXIMUM_PENALTY_TIME	1.5f	// Maximum time penalty we'll allow

//=============================================================================
//
// TF Weapon Pistol.
//
class CTFPistol : public CTFWeaponBaseGun
{
public:

	DECLARE_CLASS( CTFPistol, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

// Server specific.
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CTFPistol();
	~CTFPistol() {}

	virtual void	ItemPostFrame( void );
	virtual void	PrimaryAttack( void );

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_PISTOL; }
	CNetworkVar( float,	m_flSoonestPrimaryAttack );

private:
	CTFPistol( const CTFPistol & ) {}
};

class CTFPistol_Spy : public CTFPistol
{
public:
	DECLARE_CLASS( CTFPistol_Spy, CTFPistol );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();


	virtual void	ItemPostFrame(void);
	virtual bool	DefaultReload( int iClipSize1, int iClipSize2, int iActivity );
	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_PISTOL_SPY; }
};

#endif // TF_WEAPON_PISTOL_H