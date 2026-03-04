//=============================================================================
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef PF_MENUPANELBASE_H
#define PF_MENUPANELBASE_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>
#include <vgui/KeyCode.h>

namespace vgui
{
	class Label;
};

class CPFMenuPanelBase : public vgui::Frame
{
private:
	DECLARE_CLASS_SIMPLE( CPFMenuPanelBase, vgui::Frame);

public:
	CPFMenuPanelBase( vgui::Panel *parent, const char *name );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnCommand( const char *command );
	virtual void PaintBackground();
};

#endif // PF_MENUPANELBASE_H
