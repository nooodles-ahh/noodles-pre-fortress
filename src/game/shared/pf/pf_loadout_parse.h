#ifndef PF_LOADOUT_PARSE_H
#define PF_LOADOUT_PARSE_H
#ifdef _WIN32
#pragma once
#endif

#define PF_MAX_WEAPON_STRING	80
#define PF_MAX_WEAPON_PREFIX	16

#define PF_WEAPON_PRINTNAME_MISSING "missingno"

typedef struct
{
	const char *name;
	const char *replace;
	acttable_t	acttable;
} override_acttable_t;


// TODO - this could probably just be a structure
class PFLoadoutWeaponInfo
{
public:

	PFLoadoutWeaponInfo();
	~PFLoadoutWeaponInfo();

	virtual void Parse(KeyValues* pKeyValuesData, bool disabled = false);

public:
	char	m_szItemName[PF_MAX_WEAPON_STRING];
	char	m_szItemTypeName[PF_MAX_WEAPON_STRING];
	char	m_szItemSlot[PF_MAX_WEAPON_STRING];
	char	m_szItemQuality[PF_MAX_WEAPON_STRING];
	char	m_szInventoryModel[PF_MAX_WEAPON_STRING];
	char	m_szPlayerModel[PF_MAX_WEAPON_STRING];
	bool	m_bUsedByClasses[TF_CLASS_COUNT_ALL];

	bool	m_bAttachToHands;
	bool	m_bDisabled;
	int		m_iItemType;
	int  	m_iAnimSlot;
	int		m_iItemSlot;
	int		m_iWeaponClassID;
	Vector	m_vecViewmodelMinOffset;
	CUtlVector< override_acttable_t > m_ActivityOverrides;
};

#endif