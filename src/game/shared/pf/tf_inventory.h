//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Simple Inventory
// by MrModez
// $NoKeywords: $
//=============================================================================//
#ifndef TF_INVENTORY_H
#define TF_INVENTORY_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
//#include "server_class.h"
#include "tf_playeranimstate.h"
#include "tf_shareddefs.h"
#include "tf_weapon_parse.h"
#include "filesystem.h" 
#include "pf_loadout_parse.h"
#include "utldict.h"
#if defined( CLIENT_DLL )
#include "c_tf_player.h"
#endif

#define INVENTORY_SLOTS			5
#define INVENTORY_WEAPONS		5
#define INVENTORY_WEAPONS_COUNT	500
#define INVENTORY_COLNUM		5
#define INVENTORY_ROWNUM		3
#define INVENTORY_VECTOR_NUM	INVENTORY_COLNUM * INVENTORY_ROWNUM

extern void InitTFInventory();

class CTFInventory
{
public:
	CTFInventory();

	PFLoadoutWeaponInfo *GetWeapon(int iClass, int iSlot, int iNum);
	PFLoadoutWeaponInfo *GetWeapon(unsigned short iIndex);
	PFLoadoutWeaponInfo *GetRandomSlotWeapon(int iSlot);
	unsigned short FindWeapon(const char *weaponName);
	int CheckValidSlot(int iClass, int iSlot);
	bool CheckValidWeapon(int iClass, int iSlot, int iWeapon);
	void ParseItems( void );

#if defined( CLIENT_DLL )
	KeyValues* GetInventory(IBaseFileSystem *pFileSystem);
	void SetInventory(IBaseFileSystem *pFileSystem, KeyValues* pInventory);
	int GetLocalPreset(KeyValues* pInventory, int iClass, int iSlot);
	int GetWeaponPreset(IBaseFileSystem *pFileSystem, int iClass, int iSlot);
	char* GetWeaponBucket(int iWeapon, int iTeam);
	CHudTexture *FindHudTextureInDict(CUtlDict< CHudTexture *, int >& list, const char *psz);
#endif
	const char* GetSlotName(int iSlot);

#ifdef CLIENT_DLL
private:
	void ValidateLocalInventory( void );
#endif

private:
	static const char *g_aPlayerSlotNames[INVENTORY_SLOTS];
	CUtlDict<PFLoadoutWeaponInfo *> m_WeaponsDictionary;
	CUtlVector<CCopyableUtlVector<PFLoadoutWeaponInfo *>> m_hLoadoutWeapons[TF_CLASS_COUNT_ALL];
	bool m_bParsedScript;
};

CTFInventory *GetTFInventory();

#endif // TF_INVENTORY_H
