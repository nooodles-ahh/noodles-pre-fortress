//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: CTF AmmoPack.
//
//=============================================================================//
#include "cbase.h"
#include "items.h"
#include "tf_gamerules.h"
#include "tf_shareddefs.h"
#include "tf_player.h"
#include "tf_team.h"
#include "engine/IEngineSound.h"
#include "entity_ammopack.h"
#include "pf_cvars.h"

//=============================================================================
//
// CTF AmmoPack defines.
//

#define TF_AMMOPACK_PICKUP_SOUND	"AmmoPack.Touch"
#define TF_GRENADEPACK_PICKUP_SOUND "GrenadePack.Touch"

LINK_ENTITY_TO_CLASS( item_ammopack_full, CAmmoPack );
LINK_ENTITY_TO_CLASS( item_ammopack_small, CAmmoPackSmall );
LINK_ENTITY_TO_CLASS( item_ammopack_medium, CAmmoPackMedium );
LINK_ENTITY_TO_CLASS( item_grenadepack, CGrenadePack );


//=============================================================================
//
// CTF AmmoPack functions.
//

//-----------------------------------------------------------------------------
// Purpose: Spawn function for the ammopack
//-----------------------------------------------------------------------------
void CAmmoPack::Spawn( void )
{
	Precache();
	SetModel( GetPowerupModel() );

	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Precache function for the ammopack
//-----------------------------------------------------------------------------
void CAmmoPack::Precache( void )
{
	PrecacheModel( GetPowerupModel() );
	PrecacheScriptSound( TF_AMMOPACK_PICKUP_SOUND );
}

//-----------------------------------------------------------------------------
// Purpose: MyTouch function for the ammopack
//-----------------------------------------------------------------------------
bool CAmmoPack::MyTouch( CBasePlayer *pPlayer )
{
	bool bSuccess = false;

	if ( ValidTouch( pPlayer ) )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
		if ( !pTFPlayer )
			return false;

		int iMaxPrimary = pTFPlayer->m_Shared.MaxAmmo(TF_AMMO_PRIMARY);
		if ( pPlayer->GiveAmmo( ceil(iMaxPrimary * PackRatios[GetPowerupSize()]), TF_AMMO_PRIMARY, true ) )
		{
			bSuccess = true;
		}

		int iMaxSecondary = pTFPlayer->m_Shared.MaxAmmo(TF_AMMO_SECONDARY);
		if ( pPlayer->GiveAmmo( ceil(iMaxSecondary * PackRatios[GetPowerupSize()]), TF_AMMO_SECONDARY, true ) )
		{
			bSuccess = true;
		}

		int iMaxMetal = pTFPlayer->m_Shared.MaxAmmo(TF_AMMO_METAL);
		if ( pPlayer->GiveAmmo( ceil(iMaxMetal * PackRatios[GetPowerupSize()]), TF_AMMO_METAL, true ) )
		{
			bSuccess = true;
		}
		
		float flCloak = pTFPlayer->m_Shared.GetSpyCloakMeter();
		if( flCloak < 100.0f )
		{
			pTFPlayer->m_Shared.SetSpyCloakMeter( min( flCloak + ( 100.0f * PackRatios[GetPowerupSize()] ), 100.0f ) );
			bSuccess = true;
		}

		// did we give them anything?
		if ( bSuccess )
		{
			CSingleUserRecipientFilter filter( pPlayer );
			EmitSound( filter, entindex(), TF_AMMOPACK_PICKUP_SOUND );
		}
	}

	return bSuccess;
}

//-----------------------------------------------------------------------------
// Purpose: MyTouch function for the ammopack
//-----------------------------------------------------------------------------
bool CGrenadePack::MyTouch(CBasePlayer* pPlayer)
{
	bool bSuccess = false;

	if (ValidTouch(pPlayer))
	{
		CTFPlayer* pTFPlayer = ToTFPlayer(pPlayer);
		if (!pTFPlayer)
			return false;

		int iMaxGrenades1 = pTFPlayer->m_Shared.MaxAmmo(TF_AMMO_GRENADES1);
		if (pPlayer->GiveAmmo(ceil(iMaxGrenades1 * PackRatios[GetPowerupSize()]), TF_AMMO_GRENADES1, true))
		{
			bSuccess = true;
		}

		int iMaxGrenades2 = pTFPlayer->m_Shared.MaxAmmo(TF_AMMO_GRENADES2);
		if (pPlayer->GiveAmmo(ceil(iMaxGrenades2 * PackRatios[GetPowerupSize()]), TF_AMMO_GRENADES2, true))
		{
			bSuccess = true;
		}

		// did we give them anything?
		if (bSuccess)
		{
			CSingleUserRecipientFilter filter(pPlayer);
			EmitSound( filter, entindex(), TF_GRENADEPACK_PICKUP_SOUND );
		}
	}

	return bSuccess;
}

float CGrenadePack::GetRespawnDelay( void )
{ 
	return pf_grenadepack_respawn_time.GetFloat();
}
