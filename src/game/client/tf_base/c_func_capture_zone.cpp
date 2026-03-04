//========= Copyright © 1996-2007, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "c_func_capture_zone.h"
#include "tf_shareddefs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern CUtlVector<int> g_CaptureZones;

IMPLEMENT_CLIENTCLASS_DT( C_CaptureZone, DT_CaptureZone, CCaptureZone )
	RecvPropBool( RECVINFO( m_bDisabled ) ),
END_RECV_TABLE()

LINK_ENTITY_TO_CLASS( func_capturezone, C_CaptureZone );

void C_CaptureZone::Spawn( void )
{
	// add this element if it isn't already in the list
	if ( g_CaptureZones.Find( entindex() ) == -1 )
	{
		g_CaptureZones.AddToTail( entindex() );
	}
}

bool C_CaptureZone::IsDisabled( void )
{
	return m_bDisabled;
}
