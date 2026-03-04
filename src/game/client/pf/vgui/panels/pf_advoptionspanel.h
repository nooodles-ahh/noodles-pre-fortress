//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef PF_ADVOPTIONSPANEL_H
#define PF_ADVOPTIONSPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/PropertyDialog.h"
#include "vgui_controls/PropertySheet.h"
#include "vgui_controls/PropertyPage.h"
#include "controls/pf_tooltip.h"

namespace vgui
{
	class Label;
};

/*class CPFTextToolTip : public vgui::BaseTooltip
{
};*/

class CPFOptionsSubPanel : public vgui::PropertyPage
{
	DECLARE_CLASS_SIMPLE( CPFOptionsSubPanel, vgui::PropertyPage );
public:
	CPFOptionsSubPanel( vgui::Panel *parent, const char *name, char *defFile, char *file, CTFTextToolTip *pToolTip );
	virtual ~CPFOptionsSubPanel();

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void PerformLayout();

	void CreateControls();
	void DestroyControls();

	void GatherCurrentValues();
	void SaveValues();

private:
	vgui::PanelListPanel *m_pListPanel;
	mpcontrol_t *m_pList;
	CInfoDescription *m_pDescription;

	char *m_cFile;
	char *m_cDefFile;

	CTFTextToolTip *m_pParentToolTip;

	CPanelAnimationVarAliasType( int, m_iControlWidth, "control_w", "500", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iControlHeight, "control_h", "25", "proportional_int" );
};

class CPFAdvOptionsPanel : public vgui::PropertyDialog
{
private:
	DECLARE_CLASS_SIMPLE( CPFAdvOptionsPanel, vgui::PropertyDialog );

public:
	CPFAdvOptionsPanel( vgui::Panel *parent );
	void DestroyControls();

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

	void ShowModal();
	virtual void OnCommand( const char *command );


private:
	virtual void OnKeyCodePressed( vgui::KeyCode code );
	virtual void OnKeyCodeTyped( vgui::KeyCode code );
	void SaveSettings();
	virtual void OnClose();

private:

	CPFOptionsSubPanel *m_pGameOptionsPage;
	CPFOptionsSubPanel *m_pPFOptionsPage;

	// tooltip garbage
	CTFTextToolTip *m_pToolTip;
	vgui::EditablePanel *m_pToolTipPanel;
};

#endif // PF_ADVOPTIONSPANEL_H
