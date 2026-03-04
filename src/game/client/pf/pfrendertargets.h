//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Has init functions for all the standard render targets used by most games.
//			Mods who wish to make their own render targets can inherit from this class
//			and in the 'InitClientRenderTargets' interface called by the engine, set up
//			their own render targets as well as calling the init functions for various
//			common render targets provided by this class.
//
//			Note: Unless the client defines a singleton interface by inheriting from
//			this class and exposing the singleton instance, these init and shutdown
//			functions WILL NOT be called by the engine.
//
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#ifndef PFRENDERTARTETS_H_
#define PFRENDERTARTETS_H_
#ifdef _WIN32
#pragma once
#endif


#include "baseclientrendertargets.h" // Base class, with interfaces called by engine and inherited members to init common render targets

// Externs
class IMaterialSystem;
class IMaterialSystemHardwareConfig;

class CPFRenderTargets : public CBaseClientRenderTargets
{
	// no networked vars
	DECLARE_CLASS_GAMEROOT( CPFRenderTargets, CBaseClientRenderTargets );
public:
	// Interface called by engine during material system startup.
	virtual void InitClientRenderTargets( IMaterialSystem* pMaterialSystem, IMaterialSystemHardwareConfig* pHardwareConfig );
	// Shutdown all custom render targets here.
	virtual void ShutdownClientRenderTargets ( void );

protected:
	
	CTextureReference		m_RefractionPFTexture;

	ITexture* CreatePFRefractionTexture( IMaterialSystem* pMaterialSystem );
};

extern CPFRenderTargets* pfrendertargets;

#endif // PFRENDERTARTETS_H_