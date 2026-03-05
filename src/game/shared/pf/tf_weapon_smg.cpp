//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======
//
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_smg.h"

//=============================================================================
//
// Weapon SMG tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFSMG, DT_WeaponSMG )

BEGIN_NETWORK_TABLE( CTFSMG, DT_WeaponSMG )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFSMG )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_smg, CTFSMG );
PRECACHE_WEAPON_REGISTER( tf_weapon_smg );

// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFSMG )
END_DATADESC()
#endif

//=============================================================================
//
// Weapon SMG functions.
//

#ifdef PF2
//=============================================================================
//
// Weapon SMG Scout tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED(TFSMG_Scout, DT_WeaponSMG_Scout)

BEGIN_NETWORK_TABLE(CTFSMG_Scout, DT_WeaponSMG_Scout)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CTFSMG_Scout)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(tf_weapon_smg_scout, CTFSMG_Scout);
PRECACHE_WEAPON_REGISTER(tf_weapon_smg_scout);

//=============================================================================
//
// Weapon SMG Medic functions.
//
IMPLEMENT_NETWORKCLASS_ALIASED(TFSMG_Medic, DT_WeaponSMG_Medic)

BEGIN_NETWORK_TABLE(CTFSMG_Medic, DT_WeaponSMG_Medic)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CTFSMG_Medic)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(tf_weapon_smg_medic, CTFSMG_Medic);
PRECACHE_WEAPON_REGISTER(tf_weapon_smg_medic);
#endif