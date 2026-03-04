//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: CTF HealthKit.
//
//=============================================================================//
#include "cbase.h"
#include "items.h"
#include "tf_gamerules.h"
#include "tf_shareddefs.h"
#include "tf_player.h"
#include "tf_team.h"
#include "engine/IEngineSound.h"
#include "entity_healthkit.h"

//=============================================================================
//
// CTF HealthKit defines.
//

#define TF_HEALTHKIT_MODEL						"models/items/healthkit.mdl"
#define TF_HEALTHKIT_PICKUP_SOUND				"HealthKit.Touch"
#define PF_ARMOR_PICKUP_SOUND					"Armor.Touch"
#define PF_ARMOR_AND_HEALTHKIT_PICKUPSOUND		"ArmorAndHealth.Touch"

LINK_ENTITY_TO_CLASS( item_healthkit_full, CHealthKit );
LINK_ENTITY_TO_CLASS( item_healthkit_small, CHealthKitSmall );
LINK_ENTITY_TO_CLASS( item_healthkit_medium, CHealthKitMedium );

//=============================================================================
//
// CTF HealthKit functions.
//

//-----------------------------------------------------------------------------
// Purpose: Spawn function for the healthkit
//-----------------------------------------------------------------------------
void CHealthKit::Spawn( void )
{
	Precache();
	SetModel( GetPowerupModel() );

	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Precache function for the healthkit
//-----------------------------------------------------------------------------
void CHealthKit::Precache( void )
{
	PrecacheModel( GetPowerupModel() );
	PrecacheScriptSound( TF_HEALTHKIT_PICKUP_SOUND );
	PrecacheScriptSound( PF_ARMOR_PICKUP_SOUND );
	PrecacheScriptSound( PF_ARMOR_AND_HEALTHKIT_PICKUPSOUND );
}

//-----------------------------------------------------------------------------
// Purpose: MyTouch function for the healthkit
//-----------------------------------------------------------------------------
bool CHealthKit::MyTouch( CBasePlayer *pPlayer )
{
	bool bSuccess = false;

	if ( ValidTouch( pPlayer ) )
	{
		CTFPlayer* pTFPlayer = ToTFPlayer(pPlayer);
		Assert(pTFPlayer);
#ifdef PF2_DLL
		int iMaxArmor = pTFPlayer->GetPlayerClass()->GetMaxArmor();
		int iArmorRepaired = 0;
		if( TFGameRules()->HealthkitRepairsArmor() )
		{
			iArmorRepaired = pTFPlayer->TakeArmor( iMaxArmor * PackRatios[GetPowerupSize()] );
		}
#endif
		int iAmountHealed = pTFPlayer->TakeHealth( ceil( pTFPlayer->GetMaxHealth() * PackRatios[GetPowerupSize()] ), DMG_GENERIC );
		if ( iAmountHealed > 0
#ifdef PF2_DLL
			|| iArmorRepaired > 0
#endif
			)
		{
			CSingleUserRecipientFilter user(pPlayer);
			user.MakeReliable();

			UserMessageBegin(user, "ItemPickup");
			WRITE_STRING(GetClassname());
			MessageEnd();

			if( iAmountHealed > 0)
			{
				IGameEvent *event = gameeventmanager->CreateEvent( "player_healonhit" );
				if( event )
				{
					event->SetInt( "priority", 1 );
					event->SetInt( "entindex", pPlayer->entindex() );
					event->SetInt( "amount", iAmountHealed );

					gameeventmanager->FireEvent( event );
				}
			}

			
			if( TFGameRules()->HealthkitRepairsArmor() )
			{
				if( iArmorRepaired > 0 && iAmountHealed > 0 )
				{
					EmitSound( user, entindex(), PF_ARMOR_AND_HEALTHKIT_PICKUPSOUND );
				}
				else if( iArmorRepaired > 0 )
				{
					EmitSound( user, entindex(), PF_ARMOR_PICKUP_SOUND );
				}
				else
				{
					EmitSound( user, entindex(), TF_HEALTHKIT_PICKUP_SOUND );
				}
				
			}
			else
			{
				EmitSound( user, entindex(), TF_HEALTHKIT_PICKUP_SOUND );
			}

			

			bSuccess = true;

			// Healthkits also contain a fire blanket, leg unbreaking serum, and penicillin
			if (pTFPlayer->m_Shared.InCond(TF_COND_BURNING))
			{
				pTFPlayer->m_Shared.RemoveCond(TF_COND_BURNING);
			}
			if (pTFPlayer->m_Shared.InCond(TF_COND_LEG_DAMAGED))
			{
				pTFPlayer->m_Shared.RemoveCond(TF_COND_LEG_DAMAGED);
			}
			if( pTFPlayer->m_Shared.InCond( TF_COND_LEG_DAMAGED_GUNSHOT ) )
			{
				pTFPlayer->m_Shared.RemoveCond( TF_COND_LEG_DAMAGED_GUNSHOT );
			}
			if (pTFPlayer->m_Shared.InCond(TF_COND_INFECTED))
			{
				pTFPlayer->m_Shared.RemoveCond(TF_COND_INFECTED);
			}
#ifdef PF2_DLL
			if( iArmorRepaired > 0 )
			{
				IGameEvent *event = gameeventmanager->CreateEvent( "player_repaironhit" );
				if( event )
				{
					event->SetInt( "entindex", pPlayer->entindex() );
					event->SetInt( "amount", iArmorRepaired );

					gameeventmanager->FireEvent( event );
				}
			}
#endif
		}
	}

	return bSuccess;
}
