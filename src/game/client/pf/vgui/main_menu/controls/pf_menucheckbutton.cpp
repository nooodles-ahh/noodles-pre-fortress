#include "cbase.h"
#include "pf_menucheckbutton.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

#define DEFAULT_CHECKIMAGE				"main_menu/glyph_off_toggled"

DECLARE_BUILD_FACTORY_DEFAULT_TEXT( CPFCheckButton, CPFCheckButton );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CPFCheckButton::CPFCheckButton( Panel *parent, const char *panelName, const char *text ) : CPFButtonBase( parent, panelName, text )
{
	m_pCheckImage = new ImagePanel( this, "SubCheckImage" );
	
	m_bChecked = false;
	m_bInverted = false;
	m_pCvarRef = nullptr;
}

void CPFCheckButton::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	m_bInverted = inResourceData->GetBool( "inverted" );

	const char *pCvarName = inResourceData->GetString( "cvar_name", nullptr );

	KeyValues *pCheckImageKey = inResourceData->FindKey( "SubCheckImage" );

	if( pCheckImageKey )
	{
		if( m_pCheckImage )
			m_pCheckImage->ApplySettings( pCheckImageKey );
	}

	if( pCvarName )
	{
		if( m_pCvarRef )
			delete[] m_pCvarRef;

		m_pCvarRef = new ConVarRef( pCvarName );
		if( m_pCvarRef )
		{
			if( m_bInverted )
				SetChecked( m_pCvarRef->GetBool() );
			else
				SetChecked( !m_pCvarRef->GetBool() );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPFCheckButton::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	m_colorImageDefault = pScheme->GetColor( "MenuCheckButton.ImageColor", COLOR_WHITE );
	m_colorImageArmed = pScheme->GetColor( "MenuCheckButton.ArmedImageColor", COLOR_WHITE );
	m_colorImageDepressed = pScheme->GetColor( "MenuCheckButton.DepressedImageColor", COLOR_WHITE );

	SetDefaultColor( pScheme->GetColor( "MenuCheckButton.TextColor", COLOR_WHITE ), 
		pScheme->GetColor( "MenuCheckButton.BgColor", COLOR_WHITE ) );
	SetArmedColor( pScheme->GetColor( "MenuCheckButton.ArmedTextColor", COLOR_WHITE ), 
		pScheme->GetColor( "MenuCheckButton.ArmedBgColor", COLOR_WHITE ) );
	SetDepressedColor( pScheme->GetColor( "MenuCheckButton.DepressedTextColor", COLOR_WHITE ), 
		pScheme->GetColor( "MenuCheckButton.DepressedBgColor", COLOR_WHITE ) );
	// used depressed colours by default
	SetSelectedColor( BaseClass::BaseClass::GetButtonDepressedFgColor(), BaseClass::BaseClass::GetButtonDepressedBgColor() );

	// Do this now so they apply to the team arrays
	ApplyOverridableColors();

	for( int iTeam = 0; iTeam < TF_TEAM_COUNT; ++iTeam )
	{
		m_colorImageDefaultTeams[iTeam] = pScheme->GetColor( VarArgs( "MenuCheckButton.ImageColor.Team%d", iTeam ),
			m_colorImageDefault );
		m_colorImageArmedTeams[iTeam] = pScheme->GetColor( VarArgs( "MenuCheckButton.ArmedImageColor.Team%d", iTeam ),
			m_colorImageArmed );
		m_colorImageDepressedTeams[iTeam] = pScheme->GetColor( VarArgs( "MenuCheckButton.DepressedImageColor.Team%d", iTeam ),
			m_colorImageDepressed );

		m_colorBGDefaultTeams[iTeam] = pScheme->GetColor( VarArgs( "MenuCheckButton.BgColor.Team%d", iTeam ),
			BaseClass::BaseClass::GetButtonDefaultBgColor() );
		m_colorBGArmedTeams[iTeam] = pScheme->GetColor( VarArgs( "MenuCheckButton.ArmedBgColor.Team%d", iTeam ),
			BaseClass::BaseClass::GetButtonArmedBgColor() );
		m_colorBGDepressedTeams[iTeam] = pScheme->GetColor( VarArgs( "MenuCheckButton.DepressedBgColor.Team%d", iTeam ),
			BaseClass::BaseClass::GetButtonDepressedBgColor() );
	}
	
	// Show it above checkbox BG.
	m_pButtonImage->SetZPos( 1 );

	if( m_pCheckImage->GetImage() == NULL )
	{
		// Set default check image if we're not overriding it.
		m_pCheckImage->SetImage( DEFAULT_CHECKIMAGE );
	}

	m_pCheckImage->SetMouseInputEnabled( false );
	m_pCheckImage->SetShouldScaleImage( true );

	// Show check image above everything.
	m_pCheckImage->SetZPos( 2 );
}

void CPFCheckButton::PerformLayout()
{
	BaseClass::PerformLayout();

	// If it's inverted than we want to show check mark and when the box is off.
	m_pCheckImage->SetVisible( m_bInverted ? !IsChecked() : IsChecked() );

	// Set image color based on our state.
	if( IsDepressed() )
	{
		if( m_iTeamColor == TEAM_INVALID )
			m_pCheckImage->SetDrawColor( m_colorImageDepressed );
		else
			m_pCheckImage->SetDrawColor( m_colorImageDepressedTeams[m_iTeamColor] );
	}
	else if( IsArmed() || IsSelected() )
	{
		if( m_iTeamColor == TEAM_INVALID )
			m_pCheckImage->SetDrawColor( m_colorImageArmed );
		else
			m_pCheckImage->SetDrawColor( m_colorImageArmedTeams[m_iTeamColor] );
	}
	else
	{
		if( m_iTeamColor == TEAM_INVALID )
			m_pCheckImage->SetDrawColor( m_colorImageDefault );
		else
			m_pCheckImage->SetDrawColor( m_colorImageDefaultTeams[m_iTeamColor] );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPFCheckButton::DoClick( void )
{
	// Base class handling of "selected" flag is kind of finicky so we're using our own value.
	SetChecked( !IsChecked() );

	BaseClass::DoClick();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPFCheckButton::SetChecked( bool bState )
{
	m_bChecked = bState;

	if( m_pCvarRef )
	{
		if( m_bInverted )
		{
			m_pCvarRef->SetValue( IsChecked() );
		}
		else
		{
			m_pCvarRef->SetValue( !IsChecked() );
		}
	}

	InvalidateLayout();
}