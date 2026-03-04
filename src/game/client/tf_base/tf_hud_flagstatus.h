//========= Copyright � 1996-2007, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_HUD_FLAGSTATUS_H
#define TF_HUD_FLAGSTATUS_H
#ifdef _WIN32
#pragma once
#endif

#include "entity_capture_flag.h"
#include "tf_controls.h"
#include "tf_imagepanel.h"
#include "GameEventListener.h"

//-----------------------------------------------------------------------------
// Purpose:  Draws the rotated arrow panels
//-----------------------------------------------------------------------------
class CTFArrowPanel : public vgui::Panel
{
public:
	DECLARE_CLASS_SIMPLE( CTFArrowPanel, vgui::Panel );

	CTFArrowPanel( vgui::Panel *parent, const char *name );
	virtual VMatrix GetPanelRotationMatrix( int x, int y, int wide, int tall );
	virtual void Paint();
	virtual bool IsVisible( void );
	void SetEntity( EHANDLE hEntity ){ m_hEntity = hEntity; }
	EHANDLE GetEntity( void ) { return m_hEntity; }
	float GetAngleRotation( void );

protected:

	EHANDLE				m_hEntity;	

	CMaterialReference	m_RedMaterial;
	CMaterialReference	m_BlueMaterial;
	CMaterialReference	m_NeutralMaterial;

	CMaterialReference	m_RedMaterialNoArrow;
	CMaterialReference	m_BlueMaterialNoArrow;
	CMaterialReference	m_NeutralMaterialNoArrow;
};

#ifdef PF2
//-----------------------------------------------------------------------------
// Purpose:  
//-----------------------------------------------------------------------------
class CPFArrowPanel : public CTFArrowPanel
{
public:
	DECLARE_CLASS_SIMPLE( CPFArrowPanel, CTFArrowPanel );
	CPFArrowPanel( vgui::Panel *parent, const char *name );
	virtual void Paint();
	virtual VMatrix GetPanelRotationMatrix( int x, int y, int wide, int tall );

protected:
	CMaterialReference	m_RedMaterialBG;
	CMaterialReference	m_BlueMaterialBG;
	CMaterialReference	m_NeutralMaterialBG;
};
#endif

//-----------------------------------------------------------------------------
// Purpose:  
//-----------------------------------------------------------------------------
class CTFFlagStatus : public vgui::EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE( CTFFlagStatus, vgui::EditablePanel );

	CTFFlagStatus( vgui::Panel *parent, const char *name );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual bool IsVisible( void );
	void UpdateStatus( void );

	void SetEntity( EHANDLE hEntity )
	{ 
		m_hEntity = hEntity;

		if ( m_pArrow )
		{
			m_pArrow->SetEntity( hEntity );
		}
	}

	EHANDLE *GetEntity()
	{
		return &m_hEntity;
	}

private:

	EHANDLE			m_hEntity;

	CTFArrowPanel	*m_pArrow;
	CTFImagePanel	*m_pStatusIcon;
	CTFImagePanel	*m_pBriefcase;
};


//-----------------------------------------------------------------------------
// Purpose:  
//-----------------------------------------------------------------------------
class CTFHudFlagObjectives : public vgui::EditablePanel, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CTFHudFlagObjectives, vgui::EditablePanel );

public:

	CTFHudFlagObjectives( vgui::Panel *parent, const char *name );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual bool IsVisible( void );
	virtual void Reset();
	void OnTick();
	void SetCarriedFlag( EHANDLE hEntity ) { m_hCarriedFlag = hEntity; }
	EHANDLE GetCarriedFlag( void ) { return m_hCarriedFlag; }

public: // IGameEventListener:
	virtual void FireGameEvent( IGameEvent *event );

private:
	
	void UpdateStatus( void );
	void SetPlayingToLabelVisible( bool bVisible );
	void CheckCapturePoints( C_TFPlayer *pLocalPlayer );

private:

	CTFImagePanel			*m_pCarriedImage;

	CExLabel				*m_pPlayingTo;
	CTFImagePanel			*m_pPlayingToBG;

	CTFArrowPanel			*m_pRedFlag;
	CTFArrowPanel			*m_pBlueFlag;
	// also used for a neutral flag
	CTFArrowPanel			*m_pCapturePoint;

	bool					m_bFlagAnimationPlayed;
	bool					m_bCarryingFlag;

	vgui::ImagePanel		*m_pSpecCarriedImage;

	bool					m_bHybridMode;

	EHANDLE				m_hCarriedFlag;
};

#endif	// TF_HUD_FLAGSTATUS_H
