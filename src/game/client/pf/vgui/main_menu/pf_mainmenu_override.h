#ifndef TFMAINMENU_H
#define TFMAINMENU_H

#include "vgui_controls/Panel.h"
#include "GameUI/IGameUI.h"
#include "pf_mainmenu_panel.h"
#include "controls/pf_editablepanel.h"
#include "vgui/controls/pf_tooltip.h"

// This class is what is actually used instead of the main menu.
class OverrideUI_RootPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( OverrideUI_RootPanel, vgui::EditablePanel );
public:
	OverrideUI_RootPanel(vgui::VPANEL parent);
	virtual ~OverrideUI_RootPanel();

	IGameUI*		GetGameUI();
	virtual void OnCommand( const char *command );
	void ShowMainMenu( bool bShow );
	virtual void OnTick();

	C_BasePlayer			*IsInGame();

	void UpdateServerCount();
	void UpdateNews();
	int GetDefaultBGTeam() { return m_iDefaultBGTeam; }

	CMainMenuToolTip *m_pToolTip;
	CPFEditablePanel *m_pToolTipPanel;

protected:
	virtual void	ApplySchemeSettings(vgui::IScheme *pScheme);

private:
	bool			LoadGameUI();

	IGameUI*		gameui;

	CPFMainMenuRoot *m_pMainMenu;

	bool			m_bInGame;
	bool			m_bForceHideMenu;

	int				m_iDefaultBGTeam;

};

extern OverrideUI_RootPanel *guiroot;

#endif // TFMAINMENU_H