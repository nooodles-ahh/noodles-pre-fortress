//====== Copyright © 1996-2006, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_flag.h"
#include "decals.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#include "tf_team.h"
#endif

//=============================================================================
//
// Weapon Flag tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFFlag, DT_TFWeaponFlag )

BEGIN_NETWORK_TABLE( CTFFlag, DT_TFWeaponFlag )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFFlag )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_flag, CTFFlag );
PRECACHE_WEAPON_REGISTER( tf_weapon_flag );

//=============================================================================
//
// Weapon Flag functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFFlag::CTFFlag()
{

}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFFlag::Deploy( void )
{
	if ( BaseClass::Deploy() )
	{
#ifdef GAME_DLL
        TFTeamMgr()->PlayerCenterPrint( ToTFPlayer( GetOwner() ), "#TF_Flag_AltFireToDrop" );
#endif
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlag::SecondaryAttack( void )
{
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );

	if ( !pPlayer )
		return;

	if ( !CanAttack() )
		return;

#ifdef GAME_DLL
	pPlayer->DropFlag();
	pPlayer->TFWeaponRemove(TF_WEAPON_FLAG);
	CTFWeaponBase *pMelee = pPlayer->Weapon_GetWeaponByType(TF_WPN_TYPE_MELEE);
	if (!pMelee)
	{
		const char* szMelee;
		switch (pPlayer->GetPlayerClass()->GetClassIndex()) //will change this later since it sucks.
		{
		case TF_CLASS_SCOUT:
			szMelee = "tf_weapon_bat";
			break;
		case TF_CLASS_SOLDIER:
			szMelee = "tf_weapon_shovel";
			break;
		case TF_CLASS_PYRO:
			szMelee = "tf_weapon_brandingiron";
			break;
		case TF_CLASS_DEMOMAN:
			szMelee = "tf_weapon_bottle";
			break;
		case TF_CLASS_HEAVYWEAPONS:
			szMelee = "tf_weapon_pipe";
			break;
		case TF_CLASS_ENGINEER:
			szMelee = "tf_weapon_wrench";
			break;
		case TF_CLASS_MEDIC:
			szMelee = "tf_weapon_syringe";
			break;
		case TF_CLASS_SNIPER:
			szMelee = "tf_weapon_club";
			break;
		case TF_CLASS_SPY:
			szMelee = "tf_weapon_knife";
			break;
		default:
			szMelee = "";
			break;
		}
		CTFWeaponBase *pCreateMelee = static_cast<CTFWeaponBase*>(pPlayer->GiveNamedItem(szMelee));
		if (pCreateMelee)
		{
			pCreateMelee->DefaultTouch(pPlayer);
		}
	}
#endif
	pPlayer->SwitchToNextBestWeapon(this);

}

