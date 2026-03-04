//=============================================================================﻿
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================﻿
#include "cbase.h"

#include "pf_menubuttonbase.h"
#include "pf_mainmenu_override.h"
#include "c_tf_player.h"
#include "pf_cvars.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;



//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CPFButtonBase::CPFButtonBase( Panel *parent, const char *panelName, const char *text ) : Button( parent, panelName, text )
{
	m_pButtonImage = new ImagePanel( this, "SubImage" );
	m_iMouseState = MOUSE_DEFAULT;
	m_bSelectLocked = false;

	// Shuts up "parent not sized" warnings.
	SetTall( 50 );
	SetWide( 100 );

	REGISTER_COLOR_AS_OVERRIDABLE( m_colorImageDefault, "image_drawcolor" );
	REGISTER_COLOR_AS_OVERRIDABLE( m_colorImageArmed, "image_armedcolor" );
	REGISTER_COLOR_AS_OVERRIDABLE( m_colorImageDepressed, "image_depressedcolor" );

	REGISTER_OVERRIDABLE_TEAM_COLORS( m_colorImageDefaultTeams, "image_drawcolor_team" );
	REGISTER_OVERRIDABLE_TEAM_COLORS( m_colorImageArmedTeams, "image_armedcolor_team" );
	REGISTER_OVERRIDABLE_TEAM_COLORS( m_colorImageDepressedTeams, "image_depressedcolor_team" );

	REGISTER_OVERRIDABLE_TEAM_COLORS( m_colorBGDefaultTeams, "defaultBgColor_override_team" );
	REGISTER_OVERRIDABLE_TEAM_COLORS( m_colorBGArmedTeams, "armedBgColor_override_team" );
	REGISTER_OVERRIDABLE_TEAM_COLORS( m_colorBGDepressedTeams, "depressedBgColor_override_team" );

	// todo override foreground (text) for teams

	ListenForGameEvent( "localplayer_changeteam" );

	C_BasePlayer *pPlayer = guiroot->IsInGame();
	m_iTeamColor = pPlayer ? pPlayer->GetTeamNumber() : guiroot->GetDefaultBGTeam();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPFButtonBase::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	if( pf_mainmenu_forceteam.GetInt() != -1 )
	{
		m_iTeamColor = pf_mainmenu_forceteam.GetInt();
	}

	SetArmedSound( "ui/buttonrollover.wav" );
	SetDepressedSound( "ui/buttonclick.wav" );
	SetReleasedSound( "ui/buttonclickrelease.wav" );

	// Image shouldn't interfere with the button.
	m_pButtonImage->SetMouseInputEnabled( false );
	m_pButtonImage->SetShouldScaleImage( true );
}

void CPFButtonBase::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	const char *pszToolTip = inResourceData->GetString( "tooltiptext", nullptr );
	if( pszToolTip && guiroot->m_pToolTip )
	{
		SetTooltip( guiroot->m_pToolTip, pszToolTip );
	}

	KeyValues *pImageKey = inResourceData->FindKey( "SubImage" );
	if( pImageKey )
	{
		// Workaround for this not being an EditablePanel.
		if( m_pButtonImage )
			m_pButtonImage->ApplySettings( pImageKey );
	}

	SetArmed( false );

	InvalidateLayout( false, true ); // Force ApplySchemeSettings to run.
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPFButtonBase::PerformLayout()
{
	BaseClass::PerformLayout();

	if( m_pButtonImage )
	{
		// Set image color based on our state.
		if( IsDepressed() )
		{
			if( m_iTeamColor == TEAM_INVALID )
				m_pButtonImage->SetDrawColor( m_colorImageDepressed );
			else
				m_pButtonImage->SetDrawColor( m_colorImageDepressedTeams[m_iTeamColor] );
		}
		else if( IsArmed() || IsSelected() )
		{
			if( m_iTeamColor == TEAM_INVALID )
				m_pButtonImage->SetDrawColor( m_colorImageArmed );
			else
				m_pButtonImage->SetDrawColor( m_colorImageArmedTeams[m_iTeamColor] );
		}
		else
		{
			if( m_iTeamColor == TEAM_INVALID )
				m_pButtonImage->SetDrawColor( m_colorImageDefault );
			else
				m_pButtonImage->SetDrawColor( m_colorImageDefaultTeams[m_iTeamColor] );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPFButtonBase::OnCursorEntered()
{
	BaseClass::OnCursorEntered();

	if( vgui::input()->IsMouseDown( MOUSE_LEFT ) )
		m_bSelectLocked = true;

	if( m_iMouseState != MOUSE_ENTERED )
	{
		SetMouseEnteredState( MOUSE_ENTERED );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPFButtonBase::OnCursorExited()
{
	BaseClass::OnCursorExited();

	m_bSelectLocked = false;

	SetSelected( false );

	SetArmed( false );

	if( m_iMouseState != MOUSE_EXITED )
	{
		SetMouseEnteredState( MOUSE_EXITED );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPFButtonBase::OnMousePressed( MouseCode code )
{
	BaseClass::OnMousePressed( code );

	if( code == MOUSE_LEFT && m_iMouseState != MOUSE_PRESSED )
	{
		if( !m_bSelectLocked && !IsSelected() )
		{
			SetSelected( true );
		}

		SetMouseEnteredState( MOUSE_PRESSED );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPFButtonBase::OnMouseReleased( MouseCode code )
{
	if( m_bSelectLocked )
	{
		m_bSelectLocked = false;
		return;
	}

	BaseClass::OnMouseReleased( code );

	if( code == MOUSE_LEFT && m_iMouseState == MOUSE_PRESSED )
	{
		SetMouseEnteredState( MOUSE_DEFAULT );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPFButtonBase::SetArmed( bool bState )
{
	BaseClass::SetArmed( bState );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPFButtonBase::SetSelected( bool bState )
{
	BaseClass::SetSelected( bState );

	// Sometimes SetArmed( false ) won't get called...
	if( !bState )
	{
		SetArmed( false );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPFButtonBase::SetMouseEnteredState( MouseState flag )
{
	m_iMouseState = flag;

	// Can't use SetArmed for that since we want to show tooltips for selected buttons, too.
	switch( m_iMouseState )
	{
	case MOUSE_ENTERED:
		PostActionSignal( new KeyValues( "MenuButtonArmed" ) );
		break;
	default:
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPFButtonBase::FireGameEvent( IGameEvent *event )
{
	if( FStrEq( "localplayer_changeteam", event->GetName() ) )
	{
		C_BasePlayer *pPlayer = guiroot->IsInGame();
		m_iTeamColor = pPlayer ? pPlayer->GetTeamNumber() : guiroot->GetDefaultBGTeam();
		InvalidateLayout(false, true);
	}
}

// We need these so they can be overridable in .res files
Color CPFButtonBase::GetButtonDefaultBgColor()
{
	if( m_iTeamColor != TEAM_INVALID )
		return m_colorBGDefaultTeams[m_iTeamColor];
	return BaseClass::GetButtonDefaultBgColor();
}

Color CPFButtonBase::GetButtonArmedBgColor()
{
	if( m_iTeamColor != TEAM_INVALID )
		return m_colorBGArmedTeams[m_iTeamColor];
	return BaseClass::GetButtonArmedBgColor();
}

Color CPFButtonBase::GetButtonSelectedBgColor()
{
	if( m_iTeamColor != TEAM_INVALID )
		return m_colorBGDepressedTeams[m_iTeamColor];
	return BaseClass::GetButtonDepressedBgColor();
}

Color CPFButtonBase::GetButtonDepressedBgColor()
{
	if( m_iTeamColor != TEAM_INVALID )
		return m_colorBGDepressedTeams[m_iTeamColor];
	return BaseClass::GetButtonDepressedBgColor();
}