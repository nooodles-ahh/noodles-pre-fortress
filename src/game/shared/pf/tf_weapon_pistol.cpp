//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_pistol.h"
#include "tf_fx_shared.h"
#include "in_buttons.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#endif

//=============================================================================
//
// Weapon Pistol tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFPistol, DT_WeaponPistol )

BEGIN_NETWORK_TABLE_NOBASE(CTFPistol, DT_PistolLocalData)
#if !defined( CLIENT_DLL )
SendPropTime(SENDINFO(m_flSoonestPrimaryAttack)),
#else
RecvPropTime(RECVINFO(m_flSoonestPrimaryAttack)),
#endif
END_NETWORK_TABLE()

BEGIN_NETWORK_TABLE( CTFPistol, DT_WeaponPistol )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFPistol )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_pistol, CTFPistol );
PRECACHE_WEAPON_REGISTER( tf_weapon_pistol );

// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFPistol )
END_DATADESC()
#endif

//============================

IMPLEMENT_NETWORKCLASS_ALIASED( TFPistol_Spy, DT_WeaponPistol_Spy )

BEGIN_NETWORK_TABLE( CTFPistol_Spy, DT_WeaponPistol_Spy )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFPistol_Spy )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_pistol_spy, CTFPistol_Spy );
PRECACHE_WEAPON_REGISTER( tf_weapon_pistol_spy );



//=============================================================================
//
// Weapon Pistol functions.
//

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFPistol::CTFPistol( void )
{
	m_flSoonestPrimaryAttack = gpGlobals->curtime;
}



//-----------------------------------------------------------------------------
// Purpose: Allows firing as fast as button is pressed
//-----------------------------------------------------------------------------
void CTFPistol::ItemPostFrame( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( pOwner == NULL )
		return;

	BaseClass::ItemPostFrame();

	if ( m_bInReload )
		return;

#if 0
	//Allow a refire as fast as the player can click
	if (((pOwner->m_nButtons & IN_ATTACK) == false) && (m_flSoonestPrimaryAttack < gpGlobals->curtime))
	{
		m_flNextPrimaryAttack = gpGlobals->curtime - 0.1f;
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPistol::PrimaryAttack( void )
{
	m_flSoonestPrimaryAttack = gpGlobals->curtime + PISTOL_FASTEST_REFIRE_TIME;
#if 0
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());

	if (pOwner)
	{
		// Each time the player fires the pistol, reset the view punch. This prevents
		// the aim from 'drifting off' when the player fires very quickly. This may
		// not be the ideal way to achieve this, but it's cheap and it works, which is
		// great for a feature we're evaluating. (sjb)
		//pOwner->ViewPunchReset();
	}
#endif

	if ( !CanAttack() )
		return;

	BaseClass::PrimaryAttack();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPistol_Spy::DefaultReload( int iClipSize1, int iClipSize2, int iActivity )
{
	if (m_bInReload)
		return false;

	return BaseClass::DefaultReload( iClipSize1, iClipSize2, iActivity );
}

//-----------------------------------------------------------------------------
// Purpose: Allows firing as fast as button is pressed
//-----------------------------------------------------------------------------
void CTFPistol_Spy::ItemPostFrame(void)
{
	CBasePlayer* pOwner = ToBasePlayer(GetOwner());
	if (pOwner == NULL)
		return;

	CTFWeaponBaseGun::ItemPostFrame();
}
