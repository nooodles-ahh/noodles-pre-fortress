//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_pipe.h"
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
// Weapon Lead Pipe tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFPipe, DT_TFWeaponPipe )

BEGIN_NETWORK_TABLE(CTFPipe, DT_TFWeaponPipe)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CTFPipe)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(tf_weapon_pipe, CTFPipe );
PRECACHE_WEAPON_REGISTER(tf_weapon_pipe);

//=============================================================================
//
// Weapon Lead Pipe functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFPipe::CTFPipe()
{
}

