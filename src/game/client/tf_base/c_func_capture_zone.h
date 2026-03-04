//========= Copyright © 1996-2007, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tf_shareddefs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern CUtlVector<int> g_CaptureZones;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_CaptureZone : public C_BaseEntity
{
	DECLARE_CLASS( C_CaptureZone, C_BaseEntity );

public:
	DECLARE_CLIENTCLASS();

	void Spawn( void );
	bool IsDisabled( void );

private:
	 bool m_bDisabled;		// Enabled/Disabled?
};