//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_brandingiron.h"

//=============================================================================
//
// Weapon BrandingIron tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFBrandingIron, DT_TFWeaponBrandingIron)

BEGIN_NETWORK_TABLE( CTFBrandingIron, DT_TFWeaponBrandingIron )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CTFBrandingIron)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_brandingiron, CTFBrandingIron);
PRECACHE_WEAPON_REGISTER( tf_weapon_brandingiron );
