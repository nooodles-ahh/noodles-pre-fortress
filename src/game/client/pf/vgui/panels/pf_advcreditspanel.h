//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef PF_ADVCREDITSPANEL_H
#define PF_ADVCREDITSPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui/KeyCode.h>

namespace vgui
{
	class Label;
};

class CPFAdvCreditsPanel : public vgui::Frame
{
private:
	DECLARE_CLASS_SIMPLE( CPFAdvCreditsPanel, vgui::Frame );

public:
	CPFAdvCreditsPanel( vgui::Panel *parent );
	virtual ~CPFAdvCreditsPanel();

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void PerformLayout();

	void ShowModal();
	virtual void OnCommand( const char *command );
	virtual void OnClose();

private:
	virtual void OnKeyCodePressed( vgui::KeyCode code );
	virtual void OnKeyCodeTyped( vgui::KeyCode code );
	void CreateControls();
	void DestroyControls();

private:
	vgui::PanelListPanel *m_pListPanel;
	KeyValues *m_pCreditsKV;

	CPanelAnimationVarAliasType( int, m_iTitleHeight, "title_h", "15", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iControlWidth, "control_w", "500", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iControlHeight, "control_h", "15", "proportional_int" );
};

#endif // PF_ADVOPTIONSPANEL_H
