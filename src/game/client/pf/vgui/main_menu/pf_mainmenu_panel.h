//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef PF_MAINMENU_PANEL_H
#define PF_MAINMENU_PANEL_H
#ifdef _WIN32
#pragma once
#endif

#include "controls/pf_menupanelbase.h"
#include "pf_quit_panel.h"
#include <vgui_controls/ImagePanel.h>
#include "pf_news_panel.h"

namespace vgui
{
	class Label;
};

class CPFMainMenuPanel;

class CPFMainMenuRoot : public CPFMenuPanelBase, public CGameEventListener
{
private:
	DECLARE_CLASS_SIMPLE( CPFMainMenuRoot, CPFMenuPanelBase );

public:
	CPFMainMenuRoot( vgui::Panel *parent );
	virtual ~CPFMainMenuRoot();

public: // IGameEventListener Interface
	virtual void FireGameEvent( IGameEvent *event );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void PerformLayout();
	virtual void OnThink();
	void UpdateServerCount();
	void UpdateNews();
	void MinimizeNews();

private:
	int m_iInit;
	vgui::ImagePanel *m_pFakeBGImage;

	vgui::ImagePanel *m_pTopBanner;
	vgui::ImagePanel *m_pBotBanner;
	vgui::ImagePanel *m_pLogoImage;
	CExLabel *m_pVerLabel;

	CPFMainMenuPanel *m_pMainMenuPanel;
	CPFNewsPanel *m_pNewsPanel;

	bool m_bDoOpenAnim;

	// starting positions for things
	int m_iTopBannerPos[2];
	int m_iBotBannerPos[2];

	CPanelAnimationVar( Color, m_UpdateColor, "UpdateColor", "255 0 0 255" );
};

class CPFMainMenuPanel : public vgui::EditablePanel
{
private:
	DECLARE_CLASS_SIMPLE( CPFMainMenuPanel, vgui::EditablePanel );

public:
	CPFMainMenuPanel( vgui::Panel *parent );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

	virtual void OnCommand( const char *command );

	// We recieved a message from a button that they've been hovered over
	MESSAGE_FUNC( OnMenuButtonArmed, "MenuButtonArmed" );
};

#endif // PF_ADVOPTIONSPANEL_H
