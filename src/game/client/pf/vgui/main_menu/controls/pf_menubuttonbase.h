#ifndef PF_MENUBUTTONBASE
#define PF_MENUBUTTONBASE
#ifdef _WIN32
#pragma once
#endif

#include "tf_controls.h"
#include <vgui/IInput.h>
#include <vgui/KeyCode.h>

class CPFButtonBase;

enum MouseState
{
	MOUSE_DEFAULT,
	MOUSE_ENTERED,
	MOUSE_EXITED,
	MOUSE_PRESSED,
	MOUSE_RELEASED
};	

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CPFButtonBase : public vgui::Button, public CGameEventListener
{
public:
	DECLARE_CLASS_SIMPLE( CPFButtonBase, vgui::Button );

	CPFButtonBase( vgui::Panel *parent, const char *panelName, const char *text );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void PerformLayout();

	virtual void OnCursorExited();
	virtual void OnCursorEntered();
	virtual void OnMousePressed( vgui::MouseCode code );
	virtual void OnMouseReleased( vgui::MouseCode code );
	virtual MouseState GetState() { return m_iMouseState; };
	virtual void SetArmed( bool bState );
	virtual void SetSelected( bool bState );
	void SetMouseEnteredState( MouseState flag );

	void SetButtonImageVisible( bool bVisible ) { if( m_pButtonImage ) m_pButtonImage->SetVisible( bVisible ); }

	virtual void FireGameEvent( IGameEvent *event );

	virtual Color		  GetButtonDefaultBgColor();
	virtual Color		  GetButtonArmedBgColor();
	virtual Color		  GetButtonSelectedBgColor();
	virtual Color		  GetButtonDepressedBgColor();


protected:

	// muh alternative colours
	Color			m_colorImageDefault;
	Color			m_colorImageArmed;
	Color			m_colorImageDepressed;

	Color			m_colorImageDefaultTeams[TF_TEAM_COUNT];
	Color			m_colorImageArmedTeams[TF_TEAM_COUNT];
	Color			m_colorImageDepressedTeams[TF_TEAM_COUNT];

	Color			m_colorBGDefaultTeams[TF_TEAM_COUNT];
	Color			m_colorBGArmedTeams[TF_TEAM_COUNT];
	Color			m_colorBGDepressedTeams[TF_TEAM_COUNT];

	vgui::ImagePanel *m_pButtonImage;

	MouseState		m_iMouseState;

	// Cheesy way to only select if we're over the button when we click
	bool			m_bSelectLocked;

	int		m_iTeamColor;
};


#endif // PF_MENUBUTTON
