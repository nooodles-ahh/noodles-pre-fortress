//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_pistol_spy.h"
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
IMPLEMENT_NETWORKCLASS_ALIASED(TFPistolSpy, DT_WeaponPistol_spy)

BEGIN_NETWORK_TABLE_NOBASE(CTFPistolSpy, DT_PistolSpyLocalData)
#if !defined( CLIENT_DLL )
SendPropTime(SENDINFO(m_flSoonestPrimaryAttack)),
#else
RecvPropTime(RECVINFO(m_flSoonestPrimaryAttack)),
#endif
END_NETWORK_TABLE()

BEGIN_NETWORK_TABLE(CTFPistolSpy, DT_WeaponPistol_spy)
#if !defined( CLIENT_DLL )
SendPropDataTable("PistolSpyLocalData", 0, &REFERENCE_SEND_TABLE(DT_PistolSpyLocalData), SendProxy_SendLocalWeaponDataTable),
#else
RecvPropDataTable("PistolSpyLocalData", 0, 0, &REFERENCE_RECV_TABLE(DT_PistolSpyLocalData)),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CTFPistolSpy)
#ifdef CLIENT_DLL
DEFINE_PRED_FIELD(m_flSoonestPrimaryAttack, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(tf_weapon_pistol_spy, CTFPistolSpy);
PRECACHE_WEAPON_REGISTER(tf_weapon_pistol_spy);

// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC(CTFPistolSpy)
END_DATADESC()
#endif


//=============================================================================
//
// Weapon Pistol functions.
//

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFPistolSpy::CTFPistolSpy(void)
{
	m_flSoonestPrimaryAttack = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: Allows firing as fast as button is pressed
//-----------------------------------------------------------------------------
void CTFPistolSpy::ItemPostFrame(void)
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	if (pOwner == NULL)
		return;

	BaseClass::ItemPostFrame();

	if (m_bInReload)
		return;

	//Allow a refire as fast as the player can click
	if (((pOwner->m_nButtons & IN_ATTACK) == false) && (m_flSoonestPrimaryAttack < gpGlobals->curtime))
	{
		m_flNextPrimaryAttack = gpGlobals->curtime - 0.1f;
	}
}

bool CTFPistolSpy::DefaultReload(int iClipSize1, int iClipSize2, int iActivity)
{
	if (m_bInReload)
		return false;
	// The the owning local player.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if (!pPlayer)
		return false;

	if (pPlayer->IsPlayerClass(TF_CLASS_SPY))
	{
		if (pPlayer->m_Shared.InCond(TF_COND_STEALTHED))
		{
			return false;
		}
	}

	return BaseClass::DefaultReload(iClipSize1, iClipSize2, iActivity);

}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPistolSpy::PrimaryAttack(void)
{
	m_flSoonestPrimaryAttack = gpGlobals->curtime + PISTOL_FASTEST_REFIRE_TIME;

	CBasePlayer *pOwner = ToBasePlayer(GetOwner());

	if (pOwner)
	{
		// Each time the player fires the pistol, reset the view punch. This prevents
		// the aim from 'drifting off' when the player fires very quickly. This may
		// not be the ideal way to achieve this, but it's cheap and it works, which is
		// great for a feature we're evaluating. (sjb)
		pOwner->ViewPunchReset();
	}
	if (!CanAttack())
		return;

	BaseClass::PrimaryAttack();
}
