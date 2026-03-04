//========= Copyright � 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef PF_MENUIMAGEPANEL_H
#define PF_MENUIMAGEPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_shareddefs.h"
#include <vgui/IScheme.h>
#include <vgui_controls/ImagePanel.h>
#include "GameEventListener.h"

#define MAX_BG_LENGTH		128

class CPFImagePanel : public vgui::ImagePanel, public CGameEventListener
{
public:
	DECLARE_CLASS_SIMPLE( CPFImagePanel, vgui::ImagePanel );

	CPFImagePanel( vgui::Panel *parent, const char *name );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void ApplySettings( KeyValues *inResourceData );
	void UpdateBGImage( void );
	void SetTeamBG(int iTeam);

	virtual Color GetDrawColor( void );

public: // IGameEventListener Interface
	virtual void FireGameEvent( IGameEvent * event );

public:
	char	m_szTeamBG[TF_TEAM_COUNT][MAX_BG_LENGTH];
	Color	m_colorImageDrawColor[TF_TEAM_COUNT];
	int		m_iBGTeam;
};


#endif // TF_IMAGEPANEL_H
