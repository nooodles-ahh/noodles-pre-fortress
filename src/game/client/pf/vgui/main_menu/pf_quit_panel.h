//=============================================================================
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef PF_QUIT_PANEL_H
#define PF_QUIT_PANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>
#include <vgui/KeyCode.h>

namespace vgui
{
	class Label;
};

class CPFQuitQueryPanel : public vgui::Frame
{
private:
	DECLARE_CLASS_SIMPLE( CPFQuitQueryPanel, vgui::Frame );

public:
	CPFQuitQueryPanel( vgui::Panel *parent );
	~CPFQuitQueryPanel();
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnKeyCodePressed( vgui::KeyCode code );
	virtual void OnCommand( const char *command );
	virtual void PaintBackground();
	virtual void OnClose();
};

#endif // PF_ADVOPTIONSPANEL_H
