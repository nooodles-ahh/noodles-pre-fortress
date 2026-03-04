//========= Copyright � 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: Basically CTFImagePanel but with override support
//
//=============================================================================//


#include "cbase.h"
#include <vgui/IScheme.h>
#include <vgui_controls/ImagePanel.h>

#include "pf_imagepanel.h"
#include "c_tf_player.h"
#include "pf_mainmenu_override.h"

using namespace vgui;

DECLARE_BUILD_FACTORY( CPFImagePanel );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPFImagePanel::CPFImagePanel( Panel *parent, const char *name ) : vgui::ImagePanel( parent, name )
{
	for ( int i = 0; i < TF_TEAM_COUNT; i++ )
	{
		m_szTeamBG[i][0] = '\0';
	}

	C_BasePlayer *pPlayer = guiroot->IsInGame();
	m_iBGTeam = pPlayer ? pPlayer->GetTeamNumber() : guiroot->GetDefaultBGTeam();

	REGISTER_OVERRIDABLE_TEAM_COLORS( m_colorImageDrawColor, "image_drawcolor_team" );

	for( int iTeam = 0; iTeam < TF_TEAM_COUNT; ++iTeam )
	{
		m_colorImageDrawColor[iTeam] = Color( 255, 255, 255, 255 );
	}

	ListenForGameEvent( "localplayer_changeteam" );
}

extern ConVar pf_mainmenu_forceteam;
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPFImagePanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	if( pf_mainmenu_forceteam.GetInt() != -1 )
	{
		m_iBGTeam = pf_mainmenu_forceteam.GetInt();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPFImagePanel::ApplySettings( KeyValues *inResourceData )
{
	const char *imageName = inResourceData->GetString( "image", "" );

	for ( int i = 0; i < TF_TEAM_COUNT; i++ )
	{
		Q_strncpy( m_szTeamBG[i], inResourceData->GetString( VarArgs("teambg_%d", i), "" ), sizeof( m_szTeamBG[i] ) );
		// The Ghidra decompile told me to, I really don't know
		if( m_szTeamBG[i] && m_szTeamBG[i][0] )
			PrecacheMaterial( VarArgs( "vgui/%s", m_szTeamBG[i] ) );
		else
		{
			if( imageName )
			{
				Q_strncpy( m_szTeamBG[i], imageName, sizeof( m_szTeamBG[i] ) );
			}
		}
	}

	// Apply default draw color
	for( int iTeam = 0; iTeam < TF_TEAM_COUNT; ++iTeam )
		m_colorImageDrawColor[iTeam] = BaseClass::GetDrawColor();

	BaseClass::ApplySettings( inResourceData );

	InvalidateLayout( false, true );

	UpdateBGImage();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPFImagePanel::SetTeamBG(int iTeam)
{ 
	m_iBGTeam = iTeam;
	UpdateBGImage();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPFImagePanel::UpdateBGImage( void )
{
	if ( m_iBGTeam >= 0 && m_iBGTeam < TF_TEAM_COUNT )
	{
		if ( m_szTeamBG[m_iBGTeam] && m_szTeamBG[m_iBGTeam][0] )
		{
			SetImage( m_szTeamBG[m_iBGTeam] );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPFImagePanel::FireGameEvent( IGameEvent * event )
{
	if ( FStrEq( "localplayer_changeteam", event->GetName() ) )
	{
		C_BasePlayer *pPlayer = guiroot->IsInGame();
		m_iBGTeam = pPlayer ? pPlayer->GetTeamNumber() : guiroot->GetDefaultBGTeam();
		UpdateBGImage();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Color CPFImagePanel::GetDrawColor( void )
{
	if( m_iBGTeam != TEAM_INVALID )
	{
		return m_colorImageDrawColor[m_iBGTeam];
	}
	
	return BaseClass::GetDrawColor();
}