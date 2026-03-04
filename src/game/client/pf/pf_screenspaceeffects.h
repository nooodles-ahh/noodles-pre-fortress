//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef PF_SCREENSPACEEFFECTS_H
#define PF_SCREENSPACEEFFECTS_H
#ifdef _WIN32
#pragma once
#endif

#include "ScreenSpaceEffects.h"

class CPFDruggedEffect : public IScreenSpaceEffect
{
public:
	CPFDruggedEffect( void ) : 
	  m_bUpdateView( true ),
	  m_bEnabled( false ){}

	virtual void Init( void );
	virtual void Shutdown( void );
	virtual void SetParameters( KeyValues *params );
	virtual void Enable( bool bEnable );
	virtual bool IsEnabled( ) { return m_bEnabled; }

	virtual void Render( int x, int y, int w, int h );

private:

	inline unsigned char	GetFadeAlpha( void );

	CTextureReference m_StunTexture;
	CMaterialReference m_EffectMaterial;
	bool		m_bUpdateView;
	bool		m_bEnabled;
};

ADD_SCREENSPACE_EFFECT( CPFDruggedEffect, pf_drugged_effect );

#endif // PF_SCREENSPACEEFFECTS_H
