#include "cbase.h"
#include "pf_loadout.h"
#include "pf_loadout_parse.h"
#include "tf_playerclass_shared.h"
#include "filesystem.h"

static PFLoadoutWeapons *m_PFLoadoutWeaponInfoDatabase;

//PFLoadoutWeaponInfo *PFLoadout_GetWeaponClassSlot(int iClass, int iSlot)
//{
//	PFLoadout_Parse();
//	return m_PFLoadoutWeaponInfoDatabase->FindWeapon(weaponID);
//}

PFLoadoutWeaponInfo *PFLoadout_GetWeaponByID(int weaponID)
{
	PFLoadout_Parse();
	return m_PFLoadoutWeaponInfoDatabase->FindWeapon(weaponID);

}

void PFLoadout_Parse()
{
	if(m_PFLoadoutWeaponInfoDatabase && m_PFLoadoutWeaponInfoDatabase->isParsed())
		return;

	m_PFLoadoutWeaponInfoDatabase = new PFLoadoutWeapons();
}
