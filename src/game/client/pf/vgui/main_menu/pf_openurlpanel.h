//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef PF_HTMLPANEL_H
#define PF_HTMLPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/HTML.h>
#include <vgui/KeyCode.h>

namespace vgui
{
	class Label;
};

class CPFOpenURLPanel : public vgui::Frame
{
private:
	DECLARE_CLASS_SIMPLE( CPFOpenURLPanel, vgui::Frame );

public:
	CPFOpenURLPanel( vgui::Panel *parent );
	virtual ~CPFOpenURLPanel();
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

	void ShowModal(const char *url);
	virtual void OnCommand( const char *command );

private:
	virtual void OnKeyCodePressed( vgui::KeyCode code );
	virtual void OnKeyCodeTyped( vgui::KeyCode code );

	const char *m_sURL;
};

extern CPFOpenURLPanel *GPFWebPanel();

#endif // PF_ADVOPTIONSPANEL_H
