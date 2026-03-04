//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_OBJ_SAPPER_H
#define C_OBJ_SAPPER_H
#ifdef _WIN32
#pragma once
#endif

#include "c_baseobject.h"
#include "ObjectControlPanel.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_ObjectDetpack : public C_BaseObject
{
	DECLARE_CLASS( C_ObjectDetpack, C_BaseObject );
public:
	DECLARE_CLIENTCLASS();

	virtual void OnDataChanged( DataUpdateType_t type );
	virtual void	FinishedBuilding( void );
};


#endif // C_OBJ_SAPPER_H