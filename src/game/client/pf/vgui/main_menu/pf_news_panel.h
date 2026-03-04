#ifndef PF_NEWS_PANEL_H
#define PF_NEWS_PANEL_H

#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/AnimationController.h>
#include "controls/pf_menubutton.h"
#include "controls/pf_imagepanel.h"
#include "tf_controls.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CPFNewsPanel : public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CPFNewsPanel, EditablePanel );

public:
	CPFNewsPanel( vgui::Panel *parent );
	virtual ~CPFNewsPanel();
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnThink( void );
	virtual void OnCommand( const char *command );
	void UpdateNews();
	void MinimizePanel();
	void OpenPanel();

	void ResetPanel();

	bool m_bOpened;

private:
	AnimationController::PublicValue_t m_ValMinSize;
	AnimationController::PublicValue_t m_ValMaxSize;

	AnimationController::PublicValue_t m_ValMinPos;
	AnimationController::PublicValue_t m_ValMaxPos;

	CPFButton *m_pOpenNewsButton;

	CExLabel *m_pTitleLabel;
	CExLabel *m_pAuthorLabel;
	CExRichText *m_pRichText;

	CPFButton *m_pCloseButton;
	CPFButton *m_pOpenInBrowserButton;

	const char *m_pBlogTitle; // need this because the label's not gonna save this
	char m_szBlogAuthorDate[256];
	char m_szBlogURL[256];
	char m_pUpdateURL[256];
	
	Color m_DefaultColor;

	CPanelAnimationVarAliasType( int, m_nMinWidth, "min_width", "0", "proportional_xpos" );
	CPanelAnimationVarAliasType( int, m_nMinHeight, "min_height", "0", "proportional_ypos" );
	CPanelAnimationVar( Color, m_UpdateColor, "UpdateColor", "255 0 0 128" );
};

#endif // #ifndef PF_NEWS_PANEL_H

