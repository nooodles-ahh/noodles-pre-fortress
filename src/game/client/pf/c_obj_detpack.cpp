//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "c_obj_detpack.h"
#include "c_tf_player.h"
#include <igameevents.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Start thinking
//-----------------------------------------------------------------------------
void C_ObjectDetpack::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if ( type == DATA_UPDATE_CREATED )
	{
		SetNextClientThink( CLIENT_THINK_ALWAYS );
	}
	if ( IsBuilding() )
	{
		if ( GetBuilder() )
		{
			if( !GetBuilder()->m_Shared.InCond( TF_COND_BUILDING_DETPACK ) )
				GetBuilder()->m_Shared.AddCond( TF_COND_BUILDING_DETPACK );
		}
	}
}

void C_ObjectDetpack::FinishedBuilding( void )
{
	if ( GetOwner() )
	{
		GetOwner()->m_Shared.RemoveCond( TF_COND_BUILDING_DETPACK );
	}
}

IMPLEMENT_CLIENTCLASS_DT( C_ObjectDetpack, DT_TFObjectDetpack, CObjectDetpack )
END_RECV_TABLE()