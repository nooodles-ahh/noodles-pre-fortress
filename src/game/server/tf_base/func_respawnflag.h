//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Place where we can't build things.
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_FUNC_RESPAWNFLAG_H
#define TF_FUNC_RESPAWNFLAG_H

#ifdef _WIN32
#pragma once
#endif

class CCaptureFlag;
class Vector;

//-----------------------------------------------------------------------------
// Is a given point contained within any construction yard?
//-----------------------------------------------------------------------------
bool PointInRespawnFlagZone( const Vector & vecFlagOrigin);

#endif // TF_FUNC_NO_BUILD_H
