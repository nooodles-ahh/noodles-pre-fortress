#ifndef PF_LOADOUT_H
#define PF_LOADOUT_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_shareddefs.h"
#include "pf_loadout_parse.h"

void PFLoadout_Parse();
PFLoadoutWeaponInfo *PFLoadout_GetWeaponByID(int weaponID);

#endif