#include "cbase.h"
#include "tf_shareddefs.h"
#include "pf_loadout_parse.h"
#include "pf_loadout.h"
#include "weapon_parse.h"
#include "filesystem.h"

PFLoadoutWeaponInfo::PFLoadoutWeaponInfo()
{
	m_szItemName[0] = '\0';
	m_szItemTypeName[0] = '\0';
	m_szItemSlot[0] = '\0';
	m_szItemQuality[0] = '\0';
	m_szPlayerModel[0] = '\0';
	m_szInventoryModel[0] = '\0';

	m_iItemType = 0;
	m_iItemSlot = 0;
	m_iAnimSlot = -1;
	m_iWeaponClassID = TF_WEAPON_NONE;
	m_bDisabled = true;
	m_vecViewmodelMinOffset = Vector(0);
}

PFLoadoutWeaponInfo::~PFLoadoutWeaponInfo()
{
}

#define PFGetString(var, str) const char * c##var## = pKeyValuesData->GetString(##str##); if (c##var##) Q_strncpy(##var##, c##var##, sizeof(##var##))
#define PFGetInt(var, str) iType = pKeyValuesData->GetInt(##str##); if (c##var##) Q_strncpy(##var##, c##var##, sizeof(##var##))

void PFLoadoutWeaponInfo::Parse(KeyValues* pKeyValuesData, bool disabled)
{
	const char* cszItemClass = pKeyValuesData->GetString("item_class"); 
	if (cszItemClass && cszItemClass[0]) 
	{
		char	szItemClass[PF_MAX_WEAPON_STRING];
		Q_strncpy(szItemClass, cszItemClass, sizeof(szItemClass));
		strupr(szItemClass);
		m_iWeaponClassID = GetWeaponId(szItemClass);
	}

	const char* cszItemTypeName = pKeyValuesData->GetString("item_type_name"); 
	if (cszItemTypeName && cszItemTypeName[0]) 
		Q_strncpy(m_szItemTypeName, cszItemTypeName, sizeof(m_szItemTypeName));

	const char* cszItemName = pKeyValuesData->GetString("item_name"); 
	if (cszItemName && cszItemName[0]) 
		Q_strncpy(m_szItemName, cszItemName, sizeof(m_szItemName));

	const char* cszItemSlot = pKeyValuesData->GetString("item_slot", "");
	if (cszItemSlot && cszItemSlot[0]) 
		Q_strncpy(m_szItemSlot, cszItemSlot, sizeof(m_szItemSlot));

	const char *cszAnimSlot = pKeyValuesData->GetString( "anim_slot" );

	if ( !Q_strcmp( cszAnimSlot, "primary" ) )
	{
		m_iAnimSlot = TF_WPN_TYPE_PRIMARY;
	}
	else if ( !Q_strcmp( cszAnimSlot, "secondary" ) )
	{
		m_iAnimSlot = TF_WPN_TYPE_SECONDARY;
	}
	else if ( !Q_strcmp( cszAnimSlot, "melee" ) )
	{
		m_iAnimSlot = TF_WPN_TYPE_MELEE;
	}
	else if ( !Q_strcmp( cszAnimSlot, "grenade" ) )
	{
		m_iAnimSlot = TF_WPN_TYPE_GRENADE;
	}
	else if ( !Q_strcmp( cszAnimSlot, "building" ) )
	{
		m_iAnimSlot = TF_WPN_TYPE_BUILDING;
	}
	else if ( !Q_strcmp(cszAnimSlot, "pda"))
	{
		m_iAnimSlot = TF_WPN_TYPE_PDA;
	}
	else if (!Q_strcmp(cszAnimSlot, "item1"))
	{
		m_iAnimSlot = TF_WPN_TYPE_ITEM1;
	}
	else if (!Q_strcmp(cszAnimSlot, "item2"))
	{
		m_iAnimSlot = TF_WPN_TYPE_ITEM2;
	}
	else if (!Q_strcmp(cszAnimSlot, "item3"))
	{
		m_iAnimSlot = TF_WPN_TYPE_ITEM3;
	}
	else if (!Q_strcmp(cszAnimSlot, "item4"))
	{
		m_iAnimSlot = TF_WPN_TYPE_ITEM4;
	}

	const char* cszItemQuality = pKeyValuesData->GetString("item_quality");
	if (cszItemQuality && cszItemQuality[0]) 
		Q_strncpy(m_szItemQuality, cszItemSlot, sizeof(m_szItemQuality));

	const char* cszInventoryModel = pKeyValuesData->GetString("model_inventory"); 
	if (cszInventoryModel && cszInventoryModel[0]) 
		Q_strncpy(m_szInventoryModel, cszInventoryModel, sizeof(m_szInventoryModel));

	const char* cszPlayerModel = pKeyValuesData->GetString("model_player"); 
	if (cszPlayerModel && cszPlayerModel[0]) 
		Q_strncpy(m_szPlayerModel, cszPlayerModel, sizeof(m_szPlayerModel));

	// God why isn't there a GetVector function
	const char* cszViewmodelOffset = pKeyValuesData->GetString("min_viewmodel_offset");
	if(cszViewmodelOffset && cszViewmodelOffset[0])
		UTIL_StringToVector(m_vecViewmodelMinOffset.Base(), cszViewmodelOffset);

	m_bAttachToHands = pKeyValuesData->GetBool( "attach_to_hands", false );
	
	KeyValues* pKVUsedByClasses = pKeyValuesData->FindKey("used_by_classes");
	if(pKVUsedByClasses)
	{
		for(int i = 0; i < TF_CLASS_COUNT_ALL; ++i)
		{
			m_bUsedByClasses[i] = pKVUsedByClasses->GetBool(g_aPlayerClassNamesLower[i], false);
		}
	}

	// Create the activity override list
	// Based on UTIL_LoadActivityRemapFile
	KeyValues *pKVActivityOverrides = pKeyValuesData->FindKey( "animation_replacement" );
	if( pKVActivityOverrides )
	{
		KeyValues *pKey = pKVActivityOverrides->GetFirstSubKey();
		while( pKey )
		{
			const char *pKeyName = pKey->GetName();
			const char *pKeyValue = pKey->GetString();

			override_acttable_t act;
			act.name = pKeyName;
			act.replace = pKeyValue;
			// We haven't actually registered any activities yet
			//act.baseAct = ActivityList_IndexForName( pKeyName );
			//act.weaponAct = ActivityList_IndexForName( pKeyValue );
			// -1 means missing
			act.acttable.baseAct = -2;
			act.acttable.weaponAct = -2;
			act.acttable.required = false;

			m_ActivityOverrides.AddToTail( act );

			pKey = pKVActivityOverrides->GetNextKey();
		}
	}
	
	//bDisabled = disabled || pKeyValuesData->GetBool("disabled", disabled);
	m_bDisabled = false;
}