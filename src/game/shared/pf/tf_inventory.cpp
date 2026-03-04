//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Simple Inventory
// by MrModez
// $NoKeywords: $
//=============================================================================//


#include "cbase.h"
#include "tf_shareddefs.h"
#include "tf_inventory.h"
#include "tf_playerclass_shared.h"

CTFInventory *pTFInventory = NULL;

const char *CTFInventory::g_aPlayerSlotNames[INVENTORY_SLOTS] =
{
	"primary",
	"secondary",
	"melee",
	"pda1",
	"pda2"
};

void InitTFInventory()
{
	pTFInventory = new CTFInventory();
	// Initialize the classes here.
	InitPlayerClasses();
}

// Initializing TF inventory whenever its first called is REALLY unsafe
// So I moved that part into the Init function above
// It gets called in the ::Init functions of the client and server - Kay
CTFInventory *GetTFInventory()
{
	return pTFInventory;
}

CTFInventory::CTFInventory()
{
	m_bParsedScript = false;
	ParseItems();
#ifdef CLIENT_DLL
	ValidateLocalInventory();
#endif
}

void CTFInventory::ParseItems()
{
	KeyValues* pKeyValuesData = new KeyValues("item_classes.txt");
	if (!pKeyValuesData->LoadFromFile(filesystem, "scripts/items/item_classes.txt", "MOD"))
		Error("Could not load item_classes.txt");

	if(m_bParsedScript)
	{
		m_WeaponsDictionary.PurgeAndDeleteElements();
		for(int i = 0; i < TF_CLASS_COUNT_ALL; ++i)
		{
			m_hLoadoutWeapons[i].RemoveAll();
		}
	}
	for ( KeyValues* kv = pKeyValuesData->GetFirstSubKey(); kv; kv = kv->GetNextKey() )
	{
		// have we already added this weapon?
		if(m_WeaponsDictionary.Find(kv->GetName()) != m_WeaponsDictionary.InvalidIndex())
			continue;
	
		PFLoadoutWeaponInfo *pLoadoutWeapon = new PFLoadoutWeaponInfo();
		pLoadoutWeapon->Parse(kv);
		m_WeaponsDictionary.Insert(kv->GetName(), pLoadoutWeapon);
	}

	for(int i = 0; i < TF_CLASS_COUNT_ALL; ++i)
	{
		for( int j = 0; j < INVENTORY_SLOTS; ++j )
		{
			CCopyableUtlVector<PFLoadoutWeaponInfo *> hSlot;
			m_hLoadoutWeapons[i].AddToTail( hSlot );
		}
	}
	
	for(unsigned int i = 0; i < m_WeaponsDictionary.Count(); ++i)
	{
		for( unsigned int j = 0; j < INVENTORY_SLOTS; ++j )
		{
			if(!Q_stricmp(g_aPlayerSlotNames[j], m_WeaponsDictionary[i]->m_szItemSlot))
			{
				for(int k = 0; k < TF_CLASS_COUNT_ALL; ++k)
				{
					if(m_WeaponsDictionary[i]->m_bUsedByClasses[k] == true)
					{
						m_hLoadoutWeapons[k][j].AddToTail( m_WeaponsDictionary[i] );
					}
				}
			}
		}
	}
	m_bParsedScript = true;
}

#ifdef CLIENT_DLL
void CTFInventory::ValidateLocalInventory( void )
{
	bool bReSave = false;

	KeyValues *pInventoryKeys = GetInventory( filesystem );
	for( int iClass = 0; iClass < TF_CLASS_COUNT_ALL; ++iClass )
	{
		KeyValues *pClass = pInventoryKeys->FindKey( g_aPlayerClassNames_NonLocalized[iClass], false );
		if( pClass )
		{
			for( int iSlot = 0; iSlot < INVENTORY_SLOTS; ++iSlot )
			{
				// is there an kv for this slot?
				if( pClass->FindKey( g_aPlayerSlotNames[iSlot]) )
				{
					int iPreset = pClass->GetInt( g_aPlayerSlotNames[iSlot], 0 );
					if( !CheckValidWeapon( iClass, iSlot, iPreset ) )
					{
						// reset the slot to 0
						bReSave = true;
						pClass->SetInt( g_aPlayerSlotNames[iSlot], 0 );
					}
				}
			}
		}
	}

	// only resave if we have to
	if( bReSave )
	{
		SetInventory( filesystem, pInventoryKeys );
	}
}
#endif

PFLoadoutWeaponInfo *CTFInventory::GetWeapon(int iClass, int iSlot, int iNum)
{
	if( iClass < TF_CLASS_UNDEFINED || iClass > TF_CLASS_COUNT )
		return nullptr;

	int iCount = INVENTORY_WEAPONS;
	if( iSlot >= iCount || iSlot < 0 )
		return nullptr;

	if( iSlot >= m_hLoadoutWeapons[iClass].Count() )
		return nullptr;

	if( iNum >= m_hLoadoutWeapons[iClass][iSlot].Count() )
		return nullptr;

	return m_hLoadoutWeapons[iClass][iSlot][iNum];
};

PFLoadoutWeaponInfo *CTFInventory::GetWeapon(unsigned short iIndex)
{
	if( iIndex == m_WeaponsDictionary.InvalidIndex() )
		return nullptr;

	return m_WeaponsDictionary[iIndex];
};

PFLoadoutWeaponInfo *CTFInventory::GetRandomSlotWeapon(int iSlot)
{
	int iClass = 0;
	do
	{
		iClass = RandomInt(TF_CLASS_SCOUT, TF_LAST_PLAYABLE_CLASS);
	} while (m_hLoadoutWeapons[iClass].Count() == 0 || m_hLoadoutWeapons[iClass][iSlot].Count() == 0);

	return m_hLoadoutWeapons[iClass][iSlot][0];
};

unsigned short CTFInventory::FindWeapon(const char *weaponName)
{
	int index = m_WeaponsDictionary.Find(weaponName);
	// funny
	if(index == m_WeaponsDictionary.InvalidIndex())
	{
		return m_WeaponsDictionary.Insert(weaponName, new PFLoadoutWeaponInfo());
	}
	return index;
}


int CTFInventory::CheckValidSlot(int iClass, int iSlot)
{
	if (iClass < TF_CLASS_UNDEFINED || iClass >= TF_CLASS_COUNT_ALL)
		return 0;
	if (iSlot >= INVENTORY_WEAPONS || iSlot < 0)
		return 0;

	return m_hLoadoutWeapons[iClass][iSlot].Count();
}

bool CTFInventory::CheckValidWeapon(int iClass, int iSlot, int iWeapon)
{
	if (iClass < TF_CLASS_UNDEFINED || iClass > TF_CLASS_COUNT)
		return false;
	int iCount = INVENTORY_WEAPONS;
	if (iSlot >= iCount || iSlot < 0)
		return false;
	if( !GetWeapon( iClass, iSlot, iWeapon ) )
		return false;
	return true;
}

const char* CTFInventory::GetSlotName(int iSlot)
{
	return g_aPlayerSlotNames[iSlot];
};

#if defined( CLIENT_DLL )
CHudTexture *CTFInventory::FindHudTextureInDict(CUtlDict< CHudTexture *, int >& list, const char *psz)
{
	int idx = list.Find(psz);
	if (idx == list.InvalidIndex())
		return NULL;

	return list[idx];
};

KeyValues* CTFInventory::GetInventory(IBaseFileSystem *pFileSystem)
{
	KeyValues *pInv = new KeyValues("Inventory");
	pInv->LoadFromFile(pFileSystem, "scripts/tf_inventory.txt");
	return pInv;
};

void CTFInventory::SetInventory(IBaseFileSystem *pFileSystem, KeyValues* pInventory)
{
	pInventory->SaveToFile(pFileSystem, "scripts/tf_inventory.txt");
};

char* CTFInventory::GetWeaponBucket(int iWeapon, int iTeam)
{
	if (iWeapon == TF_WEAPON_BUILDER) //shit but works
		return "sprites/bucket_sapper";

	CTFWeaponInfo* pWeaponInfo = GetTFWeaponInfo(iWeapon);
	if (!pWeaponInfo)
		return "";
	CHudTexture *pHudTexture = (iTeam == TF_TEAM_RED ? pWeaponInfo->iconInactive : pWeaponInfo->iconActive);
	if (!pHudTexture)
		return "";
	return pHudTexture->szTextureFile;
};

int CTFInventory::GetLocalPreset(KeyValues* pInventory, int iClass, int iSlot)
{
	KeyValues *pSub = pInventory->FindKey(g_aPlayerClassNames_NonLocalized[iClass]);
	if (!pSub)
		return 0;
	const int iPreset = pSub->GetInt(g_aPlayerSlotNames[iSlot], 0);
	return iPreset;
};

int CTFInventory::GetWeaponPreset(IBaseFileSystem *pFileSystem, int iClass, int iSlot)
{
	return GetLocalPreset(GetInventory(pFileSystem), iClass, iSlot);
};
#endif

#ifdef CLIENT_DLL
void CC_pf_reload_inventory( const CCommand& args )
{
	GetTFInventory()->ParseItems();
}
static ConCommand pf_reload_inventory("pf_reload_inventory", CC_pf_reload_inventory, "Reload the inventory\n", FCVAR_CHEAT);
#endif