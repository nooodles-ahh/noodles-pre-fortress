//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_cubemap.h"
#include "decals.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#endif

//=============================================================================
//
// Weapon cubemap tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFCubemap, DT_TFWeaponCubemap )

BEGIN_NETWORK_TABLE( CTFCubemap, DT_TFWeaponCubemap )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFCubemap )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_cubemap, CTFCubemap );
PRECACHE_WEAPON_REGISTER( tf_weapon_cubemap );

//=============================================================================
//
// Weapon Cubemap functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFCubemap::CTFCubemap()
{
}
