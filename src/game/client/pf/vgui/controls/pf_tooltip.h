#ifndef PF_TOOLTIP_H
#define PF_TOOLTIP_H

#include <vgui_controls/Label.h>
#include <vgui_controls/Tooltip.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CMainMenuToolTip : public vgui::BaseTooltip
{
	DECLARE_CLASS_SIMPLE( CMainMenuToolTip, vgui::BaseTooltip );

public:
	CMainMenuToolTip( vgui::Panel *parent, const char *name );
	virtual void PerformLayout();
	
	virtual void HideTooltip();
	virtual void SetText( const char *text );

	virtual const char *GetText() { return NULL; }

	EditablePanel *pPanel;
};

class CTFTextToolTip : public CMainMenuToolTip
{
	DECLARE_CLASS_SIMPLE( CTFTextToolTip, CMainMenuToolTip );

public:
	CTFTextToolTip( vgui::Panel *parent, const char *name );
	virtual void PerformLayout();
	virtual void PositionWindow( Panel *pTipPanel );
	virtual void SetText( const char *text );
};

#endif // #ifndef PF_TOOLTIP_H

