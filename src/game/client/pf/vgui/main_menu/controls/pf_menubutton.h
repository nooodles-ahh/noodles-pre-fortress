#ifndef PF_MENUBUTTON
#define PF_MENUBUTTON
#ifdef _WIN32
#pragma once
#endif

#include "pf_menubuttonbase.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CPFButton : public CPFButtonBase
{
public:
	DECLARE_CLASS_SIMPLE( CPFButton, CPFButtonBase );

	CPFButton( vgui::Panel *parent, const char *panelName, const char *text );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
};


#endif // PF_MENUBUTTON
