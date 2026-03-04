//=============================================================================
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef PF_LOADING_PANEL_H
#define PF_LOADING_PANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include "tf_controls.h"

namespace vgui
{
	class Label;
};

class CPFLoadingPanel : public vgui::EditablePanel, public CGameEventListener
{
private:
	DECLARE_CLASS_SIMPLE( CPFLoadingPanel, vgui::EditablePanel );

public:
	CPFLoadingPanel( vgui::Panel *parent = NULL );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void FireGameEvent( IGameEvent *event );
	void ClearMapLabel();
	void SetMapImage( const char *mapName );

	void UpdateTip();

private:
	MESSAGE_FUNC( OnActivate, "activate" );
	MESSAGE_FUNC( OnDeactivate, "deactivate" );

private:
	vgui::EditablePanel *m_pMapPanelBGs[2];
	CExLabel *m_pOnYourWayLabel;
	CExLabel *m_pMapNameLabel;
	vgui::Label *m_pTipText;
	vgui::ImagePanel *m_pTipImage;
};

CPFLoadingPanel *GPFLoadingPanel();

#endif // PF_ADVOPTIONSPANEL_H
