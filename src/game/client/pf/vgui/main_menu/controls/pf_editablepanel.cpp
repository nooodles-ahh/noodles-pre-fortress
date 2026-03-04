//========= Copyright � 1996-2006, Valve Corporation, All rights reserved. ============//
//
// 
//
//=============================================================================//


#include "cbase.h"
#include "pf_editablepanel.h"
#include "c_tf_player.h"
#include "pf_mainmenu_override.h"

using namespace vgui;

DECLARE_BUILD_FACTORY( CPFEditablePanel );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPFEditablePanel::CPFEditablePanel( Panel *parent, const char *name ) : vgui::EditablePanel( parent, name )
{
	C_TFPlayer *pPlayer = ToTFPlayer( guiroot->IsInGame() );
	m_iBGTeam = pPlayer ? pPlayer->GetTeamNumber() : guiroot->GetDefaultBGTeam();

	REGISTER_OVERRIDABLE_TEAM_COLORS( m_colorImageBGColor, "bgcolor_override_team" );

	for( int iTeam = 0; iTeam < TF_TEAM_COUNT; ++iTeam )
	{
		m_colorImageBGColor[iTeam] = Color( 255, 255, 255, 255 );
	}

	ListenForGameEvent( "localplayer_changeteam" );
}

extern ConVar pf_mainmenu_forceteam;
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPFEditablePanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	SetBgColor( GetSchemeColor( "MenuPanel.BgColor", pScheme ) );

	if( pf_mainmenu_forceteam.GetInt() != -1 )
	{
		m_iBGTeam = pf_mainmenu_forceteam.GetInt();
	}

	// Apply default draw color
	for( int iTeam = 0; iTeam < TF_TEAM_COUNT; ++iTeam )
		m_colorImageBGColor[iTeam] = BaseClass::GetBgColor();

	C_BasePlayer *pPlayer = guiroot->IsInGame();
	m_iBGTeam = pPlayer ? pPlayer->GetTeamNumber() : guiroot->GetDefaultBGTeam();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPFEditablePanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPFEditablePanel::PerformLayout()
{
	BaseClass::PerformLayout();
	C_BasePlayer *pPlayer = guiroot->IsInGame();
	m_iBGTeam = pPlayer ? pPlayer->GetTeamNumber() : guiroot->GetDefaultBGTeam();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPFEditablePanel::FireGameEvent( IGameEvent * event )
{
	if ( FStrEq( "localplayer_changeteam", event->GetName() ) )
	{
		C_BasePlayer *pPlayer = guiroot->IsInGame();
		m_iBGTeam = pPlayer ? pPlayer->GetTeamNumber() : guiroot->GetDefaultBGTeam();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Color CPFEditablePanel::GetBgColor( void )
{
	if( m_iBGTeam != TEAM_INVALID )
	{
		Color tempColor = m_colorImageBGColor[m_iBGTeam];
		tempColor[3] = GetParent()->GetAlpha();
		return tempColor;
	}
	
	return BaseClass::GetBgColor();
}