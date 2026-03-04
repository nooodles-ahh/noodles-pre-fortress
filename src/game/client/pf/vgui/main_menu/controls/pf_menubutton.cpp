//=============================================================================﻿
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================﻿
#include "cbase.h"

#include "pf_menubutton.h"
#include "pf_mainmenu_override.h"
#include "c_tf_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

DECLARE_BUILD_FACTORY_DEFAULT_TEXT( CPFButton, MenuButton );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CPFButton::CPFButton( Panel *parent, const char *panelName, const char *text ) : CPFButtonBase( parent, panelName, text )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPFButton::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	m_colorImageDefault = pScheme->GetColor( "MenuButton.ImageColor", COLOR_WHITE );
	m_colorImageArmed = pScheme->GetColor( "MenuButton.ArmedImageColor", COLOR_WHITE );
	m_colorImageDepressed = pScheme->GetColor( "MenuButton.DepressedImageColor", COLOR_WHITE );
	
	SetDefaultColor( pScheme->GetColor( "MenuButton.TextColor", BaseClass::BaseClass::GetButtonDefaultFgColor() ),
		pScheme->GetColor( "MenuButton.BgColor", BaseClass::BaseClass::GetButtonDefaultBgColor() ) );
	SetArmedColor( pScheme->GetColor( "MenuButton.ArmedTextColor", BaseClass::BaseClass::GetButtonArmedFgColor() ),
		pScheme->GetColor( "MenuButton.ArmedBgColor", BaseClass::BaseClass::GetButtonArmedBgColor() ) );
	SetDepressedColor( pScheme->GetColor( "MenuButton.DepressedTextColor", BaseClass::BaseClass::GetButtonDepressedFgColor() ),
		pScheme->GetColor( "MenuButton.DepressedBgColor", BaseClass::BaseClass::GetButtonDepressedBgColor() ) );
	// used depressed colours by default
	SetSelectedColor( BaseClass::BaseClass::GetButtonDepressedFgColor(), BaseClass::BaseClass::GetButtonDepressedBgColor() );

	// Do this now so they apply to the team arrays
	ApplyOverridableColors();

	for( int iTeam = 0; iTeam < TF_TEAM_COUNT; ++iTeam )
	{
		m_colorImageDefaultTeams[iTeam] = pScheme->GetColor( VarArgs( "MenuButton.ImageColor.Team%d", iTeam ),
			m_colorImageDefault );
		m_colorImageArmedTeams[iTeam] = pScheme->GetColor( VarArgs( "MenuButton.ArmedImageColor.Team%d", iTeam ),
			m_colorImageArmed );
		m_colorImageDepressedTeams[iTeam] = pScheme->GetColor( VarArgs( "MenuButton.DepressedImageColor.Team%d", iTeam ),
			m_colorImageDepressed );

		m_colorBGDefaultTeams[iTeam] = pScheme->GetColor( VarArgs( "MenuButton.BgColor.Team%d", iTeam ),
			BaseClass::BaseClass::GetButtonDefaultBgColor() );
		m_colorBGArmedTeams[iTeam] = pScheme->GetColor( VarArgs( "MenuButton.ArmedBgColor.Team%d", iTeam ),
			BaseClass::BaseClass::GetButtonArmedBgColor() );
		m_colorBGDepressedTeams[iTeam] = pScheme->GetColor( VarArgs( "MenuButton.DepressedBgColor.Team%d", iTeam ),
			BaseClass::BaseClass::GetButtonDepressedBgColor() );
	}
}