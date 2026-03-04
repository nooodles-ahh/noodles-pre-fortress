//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======
//
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_tranq.h"
#include "tf_fx_shared.h"
#include "basecombatweapon_shared.h"
#include "tf_viewmodel.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#endif

//=============================================================================
//
// Weapon Tranq tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFTranq, DT_WeaponTranq )

BEGIN_NETWORK_TABLE( CTFTranq, DT_WeaponTranq )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFTranq )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_tranq, CTFTranq );
PRECACHE_WEAPON_REGISTER( tf_weapon_tranq );

// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFTranq )
END_DATADESC()
#endif

#define TRANQ_PASSIVE_RELOAD_TIME 5.0f/3.0f // == 1.666...
#define TRANQ_PASSIVE_RELOAD_MULTIPLIER 1.5

//=============================================================================
//
// Weapon Tranq functions.
//

void CTFTranq::Spawn()
{
	BaseClass::Spawn();

	m_flReloadDuration = -1.0f;
}

bool CTFTranq::Holster(CBaseCombatWeapon *pSwitchingTo)
{
	bool ret = BaseClass::Holster( pSwitchingTo );
	if ( ret )
	{
		m_flHolsteredTime = gpGlobals->curtime;
	}

	return ret;
}

bool CTFTranq::DefaultDeploy( char *szViewModel, char *szWeaponModel, int iActivity, char *szAnimExt )
{
	bool bRet = BaseClass::DefaultDeploy( szViewModel, szWeaponModel, iActivity, szAnimExt );

	if( bRet )
	{
		ReloadCheck();
	}

	return bRet;
}

void CTFTranq::ReloadCheck(void)
{
	if( GetModelPtr() )
	{
		CTFPlayer *pOwner = GetTFPlayerOwner();
		if( pOwner )
		{
			// If we don't currently know the reload duration we need to get it
			if( m_flReloadDuration == -1.0f )
			{
				int iSequence = SelectWeightedSequence( ACT_VM_RELOAD );
				if( iSequence != ACTIVITY_NOT_AVAILABLE )
				{
					m_flReloadDuration = SequenceDuration( GetModelPtr(), iSequence ) * TRANQ_PASSIVE_RELOAD_MULTIPLIER;
				}
				else
				{
					// there was no animation WTF
					m_flReloadDuration = TRANQ_PASSIVE_RELOAD_TIME;
				}

				// set the "passive" reload duration to some multiple of the normal reload duration
				m_flReloadDuration *= TRANQ_PASSIVE_RELOAD_MULTIPLIER;
			}

			// have we have enough time to reload?
			if( m_flHolsteredTime + m_flReloadDuration < gpGlobals->curtime )
			{
				// actually reload
				if( Clip1() <= 0 )
				{
					if( !pf_infinite_ammo.GetBool() )
						pOwner->RemoveAmmo( 1, GetTFWpnData().iAmmoType );
					m_iClip1++;
				}
			}
		}
	}
}