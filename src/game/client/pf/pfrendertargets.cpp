//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Implementation for CBaseClientRenderTargets class.
//			Provides Init functions for common render textures used by the engine.
//			Mod makers can inherit from this class, and call the Create functions for
//			only the render textures the want for their mod.
//=============================================================================//


#include "cbase.h"
#include "tier0/icommandline.h"
#include "pfrendertargets.h"
#include "materialsystem/imaterialsystemhardwareconfig.h"
#include "rendertexture.h"



ITexture* CPFRenderTargets::CreatePFRefractionTexture( IMaterialSystem* pMaterialSystem )
{
	return pMaterialSystem->CreateNamedRenderTargetTextureEx2(
		"_rt_Refraction_PF",
		1024, 1024, RT_SIZE_PICMIP,
		// This is different than reflection because it has to have alpha for fog factor.
		IMAGE_FORMAT_RGBA8888, 
		MATERIAL_RT_DEPTH_SHARED, 
		TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT,
		CREATERENDERTARGETFLAGS_HDR );
}

//-----------------------------------------------------------------------------
// Purpose: Called by the engine in material system init and shutdown.
//			Clients should override this in their inherited version, but the base
//			is to init all standard render targets for use.
// Input  : pMaterialSystem - the engine's material system (our singleton is not yet inited at the time this is called)
//			pHardwareConfig - the user hardware config, useful for conditional render target setup
//-----------------------------------------------------------------------------
void CPFRenderTargets::InitClientRenderTargets( IMaterialSystem* pMaterialSystem, IMaterialSystemHardwareConfig* pHardwareConfig )
{
	BaseClass::InitClientRenderTargets(pMaterialSystem, pHardwareConfig );

	m_RefractionPFTexture.Init( CreatePFRefractionTexture( pMaterialSystem ) );
}

//-----------------------------------------------------------------------------
// Purpose: Shut down each CTextureReference we created in InitClientRenderTargets.
//			Called by the engine in material system shutdown.
// Input  :  - 
//-----------------------------------------------------------------------------
void CPFRenderTargets::ShutdownClientRenderTargets()
{
	m_RefractionPFTexture.Shutdown();

	BaseClass::ShutdownClientRenderTargets();
}

static CPFRenderTargets g_PFRenderTargets;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CPFRenderTargets, IClientRenderTargets, CLIENTRENDERTARGETS_INTERFACE_VERSION, g_PFRenderTargets );
CPFRenderTargets* pfrendertargets = &g_PFRenderTargets;
