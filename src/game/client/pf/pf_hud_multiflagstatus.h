//========= Copyright � 1996-2007, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef PF_HUD_MULTIFLAGSTATUS_H
#define PF_HUD_MULTIFLAGSTATUS_H
#ifdef _WIN32
#pragma once
#endif

#include "entity_capture_flag.h"
#include "tf_controls.h"
#include "tf_imagepanel.h"
#include "GameEventListener.h"

//-----------------------------------------------------------------------------
// Purpose:  
//-----------------------------------------------------------------------------
class CPFHudMultiFlagObjectives : public vgui::EditablePanel, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CPFHudMultiFlagObjectives, vgui::EditablePanel );

public:

	CPFHudMultiFlagObjectives( vgui::Panel *parent, const char *name );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void PerformLayout();
	virtual bool IsVisible( void );
	virtual void Reset();
	void OnTick();

public: // IGameEventListener:
	virtual void FireGameEvent( IGameEvent *event );

private:

	void UpdateStatus( void );

private:

	CTFImagePanel *m_pCarriedImage;

	CUtlVector< CTFFlagStatus *> m_pFlagStatuses;

	bool					m_bFlagAnimationPlayed;
	bool					m_bCarryingFlag;

	vgui::ImagePanel *m_pSpecCarriedImage;

	CPanelAnimationVarAliasType( int, m_nStatusYPos, "status_ypos", "0", "proportional_ypos" );
	CPanelAnimationVarAliasType( int, m_nStatusWide, "status_wide", "90", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_nStatusTall, "status_tall", "90", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_nStatusXDelta, "status_xdelta", "5", "proportional_xpos" );

};

#endif	// PF_HUD_MULTIFLAGSTATUS_H