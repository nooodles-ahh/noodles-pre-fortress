
#ifndef PF_MENUCHECKBUTTON_H
#define PF_MENUCHECKBUTTON_H
#ifdef _WIN32
#pragma once
#endif

#include "pf_menubutton.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CPFCheckButton : public CPFButtonBase
{
public:
	DECLARE_CLASS_SIMPLE( CPFCheckButton, CPFButtonBase );

	CPFCheckButton( vgui::Panel *parent, const char *panelName, const char *text );

	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void PerformLayout();
	virtual void DoClick( void );

	void SetChecked( bool bState );
	bool IsChecked( void ) { return m_bChecked; }

private:

	vgui::ImagePanel *m_pCheckImage;

	bool					m_bChecked;
	bool					m_bInverted;
	ConVarRef				*m_pCvarRef;
};


#endif // PF_MENUCHECKBUTTON_H
