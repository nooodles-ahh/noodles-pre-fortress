//========= Copyright � 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef PF_MENUPANEL_H
#define PF_MENUPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_shareddefs.h"
#include <vgui/IScheme.h>
#include <vgui_controls/EditablePanel.h>
#include "GameEventListener.h"

#define MAX_BG_LENGTH		128

class CPFEditablePanel : public vgui::EditablePanel, public CGameEventListener
{
public:
	DECLARE_CLASS_SIMPLE( CPFEditablePanel, vgui::EditablePanel );

	CPFEditablePanel( vgui::Panel *parent, const char *name );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void PerformLayout();

	virtual Color GetBgColor( void );

public: // IGameEventListener Interface
	virtual void FireGameEvent( IGameEvent * event );

public:
	Color	m_colorImageBGColor[TF_TEAM_COUNT];
	int		m_iBGTeam;
};

#endif // TF_IMAGEPANEL_H