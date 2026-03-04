//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Episodic screen-space effects
//

#include "cbase.h"
#include "ScreenSpaceEffects.h"
#include "rendertexture.h"
#include "materialsystem/imaterialsystemhardwareconfig.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/imaterialvar.h"
#include "cdll_client_int.h"
#include "materialsystem/itexture.h"
#include "KeyValues.h"
#include "clienteffectprecachesystem.h"
#include "c_tf_player.h"
#include "tf_shareddefs.h"
#include "iviewrender.h"
#include "viewrender.h"

#include "pf_screenspaceeffects.h"

ConVar pf_drugged_intensity("pf_drugged_intensity", "150", FCVAR_CLIENTDLL | FCVAR_CHEAT, "Control drugged effect intensity", true, 0.0f, true, 255.0f);
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
#define STUN_TEXTURE "_rt_Refraction_PF"

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CPFDruggedEffect::Init( void ) 
{
	m_bUpdateView = true;

	KeyValues *pVMTKeyValues = new KeyValues( "UnlitGeneric" );
	pVMTKeyValues->SetString( "$basetexture", STUN_TEXTURE );
	m_EffectMaterial.Init( "__pfdruggedeffect", TEXTURE_GROUP_CLIENT_EFFECTS, pVMTKeyValues );
	m_StunTexture.Init( STUN_TEXTURE, TEXTURE_GROUP_CLIENT_EFFECTS );
}

void CPFDruggedEffect::Shutdown( void ) 
{
	m_EffectMaterial.Shutdown();
	m_StunTexture.Shutdown();
}

//------------------------------------------------------------------------------
// Purpose: 
//------------------------------------------------------------------------------
void CPFDruggedEffect::SetParameters( KeyValues *params )
{
	// do nothing
}

//------------------------------------------------------------------------------
// Purpose: 
//------------------------------------------------------------------------------
void CPFDruggedEffect::Enable( bool bEnable ) 
{ 
	m_bEnabled = bEnable;
}

//-----------------------------------------------------------------------------
// Purpose: Get the alpha value depending on various factors and time
//-----------------------------------------------------------------------------
inline unsigned char CPFDruggedEffect::GetFadeAlpha( void )
{
	// Find our percentage between fully "on" and "off" in the pulse range
	C_TFPlayer *pTFPlayer = C_TFPlayer::GetLocalTFPlayer();

	if(!pTFPlayer || !pTFPlayer->IsAlive() || pTFPlayer->m_Shared.m_flHallucinationTime < gpGlobals->curtime || !pTFPlayer->m_Shared.InCond(TF_COND_HALLUCINATING))
		return (unsigned char) 0;

	float flEffectPerc = (pTFPlayer->m_Shared.m_flHallucinationTime - gpGlobals->curtime) / TF_HALLUCINATION_TIME;
	// HDR requires us to be more subtle, or we get uber-brightening
	if( g_pMaterialSystemHardwareConfig->GetHDRType() != HDR_TYPE_NONE )
	{
		unsigned char alpha = (unsigned char)clamp( 230.0f * flEffectPerc, 0.0f, pf_drugged_intensity.GetFloat() );
		return alpha * 0.7f;
	}

	// Non-HDR
	return (unsigned char) clamp( 240.0f * flEffectPerc, 0.0f, pf_drugged_intensity.GetFloat() );
}

//-----------------------------------------------------------------------------
// Purpose: Render the effect
//-----------------------------------------------------------------------------
void CPFDruggedEffect::Render( int x, int y, int w, int h )
{
	if ( IsEnabled() == false )
		return;

	CMatRenderContextPtr pRenderContext( materials );

	// Set ourselves to the proper rendermode
	pRenderContext->MatrixMode( MATERIAL_VIEW );
	pRenderContext->PushMatrix();
	pRenderContext->LoadIdentity();
	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->PushMatrix();
	pRenderContext->LoadIdentity();

	if ( m_bUpdateView )
	{
		// Save off this pass
		Rect_t srcRect;
		srcRect.x = x;
		srcRect.y = y;
		srcRect.width = w;
		srcRect.height = h;
		pRenderContext->CopyRenderTargetToTextureEx( m_StunTexture, 0, &srcRect, NULL );
		m_bUpdateView = false;
	}

	byte overlaycolor[4] = { 255, 238, 255, 0 };

	// Get our fade value depending on our fade duration
	overlaycolor[3] = GetFadeAlpha();

	// Disable overself if we're done fading out
	if ( overlaycolor[3] == 0 )
	{
		// Takes effect next frame (we don't want to hose our matrix stacks here)
		g_pScreenSpaceEffects->DisableScreenSpaceEffect( "pf_drugged_effect" );
		m_bUpdateView = true;
	}

	// Calculate some wavey noise to jitter the view by
	float vX = 3.0f * cosf( gpGlobals->curtime ) * cosf( gpGlobals->curtime * 3.0 );
	float vY = 2.0f * cosf( gpGlobals->curtime ) * cosf( gpGlobals->curtime * 2.0 );

	float flBaseScale = 0.1f + 0.008f * sinf( gpGlobals->curtime * 3.0f );

	// Scale percentage
	float flScalePerc = flBaseScale + ( 0.01f * cosf( gpGlobals->curtime * 2.0f ) * cosf( gpGlobals->curtime * 0.5f ) );

    // Scaled offsets for the UVs (as texels)
	float flUOffset = ( m_StunTexture->GetActualWidth() - 1 ) * flScalePerc * 0.5f;
	float flVOffset = ( m_StunTexture->GetActualHeight() - 1 ) * flScalePerc * 0.5f;

	// New UVs with scaling offsets
	float flU1 = flUOffset;
	float flU2 = ( m_StunTexture->GetActualWidth() - 1 ) - flUOffset;
	float flV1 = flVOffset;
	float flV2 = ( m_StunTexture->GetActualHeight() - 1 ) - flVOffset;

	// Draw the "zoomed" overlay
	pRenderContext->DrawScreenSpaceRectangle( m_EffectMaterial, vX, vY, w, h,
		flU1, flV1, 
		flU2, flV2, 
		m_StunTexture->GetActualWidth(), m_StunTexture->GetActualHeight() );
	

	render->ViewDrawFade( overlaycolor, m_EffectMaterial );

	// Save off this pass
	Rect_t srcRect;
	srcRect.x = x;
	srcRect.y = y;
	srcRect.width = w;
	srcRect.height = h;
	pRenderContext->CopyRenderTargetToTextureEx( m_StunTexture, 0, &srcRect, NULL );

	// Restore our state
	
	pRenderContext->MatrixMode( MATERIAL_VIEW );
	pRenderContext->PopMatrix();
	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->PopMatrix();
}
