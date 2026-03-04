#include "cbase.h"
#include "pf_news_panel.h"
#include "pf_mainmenu_override.h"
#include <vgui/ILocalize.h>
#include <vgui/ISystem.h>
#include <vgui_controls/AnimationController.h>
#include "pf_webstuff.h"
#include "pf_openurlpanel.h"

using namespace vgui;
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar pf_news_last_date( "pf_news_last_date", "0", FCVAR_ARCHIVE );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CPFNewsPanel::CPFNewsPanel( vgui::Panel *parent ) : BaseClass( parent, "NewsPanel" )
{
	m_bOpened = false;

	m_pOpenNewsButton = new CPFButton( this, "OpenNewsButton", "" );

	m_pTitleLabel = new CExLabel( this, "TitleLabel", "" );
	m_pAuthorLabel = new CExLabel( this, "AuthorLabel", "" );
	m_pRichText = new CExRichText( this, "NewsExerpt" );

	m_pCloseButton = new CPFButton( this, "CloseButton", "Close" );
	m_pOpenInBrowserButton = new CPFButton( this, "OpenBrowserButton", "Read more" );

	m_pBlogTitle = "";
	m_szBlogURL[0] = '\0';
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CPFNewsPanel::~CPFNewsPanel()
{
}

void CPFNewsPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	SetProportional( true );
	LoadControlSettings( "resource/UI/main_menu/newspanel.res" );

	m_pTitleLabel->SetText( m_pBlogTitle );

	m_DefaultColor = GetBgColor();

	// Set up or sizes and positions for the open and close animations
	m_ValMaxSize.a = GetWide();
	m_ValMaxSize.b = GetTall();
	m_ValMaxPos.a = GetXPos();
	m_ValMaxPos.b = GetYPos();

	m_ValMinSize.a = m_nMinWidth;
	m_ValMinSize.b = m_nMinHeight;
	m_ValMinPos.a = GetXPos() + GetWide() - m_nMinWidth;
	m_ValMinPos.b = GetYPos();

	UpdateNews();

	ResetPanel();
}

void CPFNewsPanel::OnThink( void )
{
	BaseClass::OnThink();
	if( m_pOpenNewsButton->GetAlpha() == 0 && m_pOpenNewsButton->IsVisible() )
		m_pOpenNewsButton->SetVisible( false );
	else if( m_pOpenNewsButton->GetAlpha() > 0 && !m_pOpenNewsButton->IsVisible() )
		m_pOpenNewsButton->SetVisible( true );
	
	if( !m_bOpened && m_pOpenNewsButton->GetAlpha() == 255 )
	{
		m_pOpenNewsButton->SetButtonImageVisible( true );
	}
}

void CPFNewsPanel::UpdateNews()
{
	if( GetWebStuff()->HasNews() )
	{
		KeyValues *kvNews = GetWebStuff()->GetNews();
		if( kvNews )
		{
			// if we have an update we need to change the blog button to represent this
			if( GetWebStuff()->m_bNeedsUpdating )
			{
				m_pBlogTitle = kvNews->GetString( "updatetitle", "" );
				m_pTitleLabel->SetText( m_pBlogTitle );
				m_pRichText->SetText( kvNews->GetString( "updateexcerpt", "" ) );

				// construct our blog url
				Q_snprintf( m_szBlogURL, sizeof( m_szBlogURL ), "%s%s", WEBSITE_URL, kvNews->GetString( "updateurl", "" ) );

				m_pOpenNewsButton->SetText( "Update" ); // LOCALIZE ME

				// If we've got an update make the background colour something else
				SetBgColor( m_UpdateColor );

				SetVisible( true );
			}
			// Show a normal blog post if we haven't seen it already
			else if( pf_news_last_date.GetInt() < kvNews->GetInt( "blogdate", 0 ) )
			{
				m_pBlogTitle = kvNews->GetString( "blogtitle", "" );
				m_pTitleLabel->SetText( m_pBlogTitle );
				m_pRichText->SetText( kvNews->GetString( "blogexcerpt", "" ) );

				int iBlogDate = kvNews->GetInt( "blogdate", 0 );
				// Cyanide; I'm lazy and stupid so I'm just gonna split up the date this way
				// Formatted: yyyy/mm/dd
				Q_snprintf( m_szBlogAuthorDate, sizeof( m_szBlogAuthorDate ), "%s - %d/%d/%d", kvNews->GetString( "blogauthor", "PF2 Team" ), iBlogDate/10000, (iBlogDate/100)%100, iBlogDate%100 );
				m_pAuthorLabel->SetText( m_szBlogAuthorDate );

				// construct our blog url
				Q_snprintf( m_szBlogURL, sizeof( m_szBlogURL ), "%s%s", WEBSITE_URL, kvNews->GetString( "blogurl", "" ) );

				m_pOpenNewsButton->SetText( "Blog" ); // LOCALIZE ME

				SetVisible( true );

				// Haven't decided if I want this behaviour yet
				// If you do only news the player hasn't already seen will be shown
				//pf_news_last_date.SetValue( iBlogDate );
			}
			else
			{
				// no news or update, hide.
				SetVisible( false );
			}
		}
		else
		{
			SetVisible( false );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Command handler
//-----------------------------------------------------------------------------
void CPFNewsPanel::OnCommand( const char *command )
{
	if( !Q_stricmp( command, "OpenNews" ) )
	{
		OpenPanel();
	}
	else if( !Q_stricmp( command, "OpenBrowser" ) )
	{
		if( m_szBlogURL && m_szBlogURL[0] != '\0' )
		{
			// if the steam overlay is active ask if the player wants to open in there
			// of in their web browser
			if( steamapicontext->SteamUtils()->IsOverlayEnabled() )
				GPFWebPanel()->ShowModal( m_szBlogURL );
			else
				system()->ShellExecute( "open", m_szBlogURL );

			MinimizePanel();
		}
	}
	else if( !Q_stricmp( command, "Close" ) )
	{
		MinimizePanel();
	}
}

void CPFNewsPanel::MinimizePanel()
{
	if( m_bOpened )
	{
		vgui::GetAnimationController()->CancelAnimationsForPanel( this );
		// set the position and size
		vgui::GetAnimationController()->RunAnimationCommand( this, "wide", m_ValMinSize.a, 0.0f, 0.5f, AnimationController::INTERPOLATOR_SIMPLESPLINE, NULL );
		vgui::GetAnimationController()->RunAnimationCommand( this, "tall", m_ValMinSize.b, 0.0f, 0.5f, AnimationController::INTERPOLATOR_SIMPLESPLINE, NULL );
		vgui::GetAnimationController()->RunAnimationCommand( this, "Position", m_ValMinPos, 0.0f, 0.5f, AnimationController::INTERPOLATOR_SIMPLESPLINE, NULL );
		// set the visiblity of our controls
		vgui::GetAnimationController()->RunAnimationCommand( m_pOpenNewsButton, "Alpha", 255, 0.4f, 0.1f, AnimationController::INTERPOLATOR_SIMPLESPLINE, NULL );
		vgui::GetAnimationController()->RunAnimationCommand( m_pTitleLabel, "Alpha", 0, 0.f, 0.4f, AnimationController::INTERPOLATOR_SIMPLESPLINE, NULL );
		vgui::GetAnimationController()->RunAnimationCommand( m_pAuthorLabel, "Alpha", 0, 0.f, 0.4f, AnimationController::INTERPOLATOR_SIMPLESPLINE, NULL );
		vgui::GetAnimationController()->RunAnimationCommand( m_pRichText, "Alpha", 0, 0.f, 0.4f, AnimationController::INTERPOLATOR_SIMPLESPLINE, NULL );
		m_bOpened = false;
	}
}

void CPFNewsPanel::OpenPanel()
{
	if( !m_bOpened )
	{
		vgui::GetAnimationController()->CancelAnimationsForPanel( this );

		m_pOpenNewsButton->SetButtonImageVisible( false );
		// set the position and size
		vgui::GetAnimationController()->RunAnimationCommand( this, "wide", m_ValMaxSize.a, 0.0f, 0.5f, AnimationController::INTERPOLATOR_SIMPLESPLINE, NULL );
		vgui::GetAnimationController()->RunAnimationCommand( this, "tall", m_ValMaxSize.b, 0.0f, 0.5f, AnimationController::INTERPOLATOR_SIMPLESPLINE, NULL );
		vgui::GetAnimationController()->RunAnimationCommand( this, "Position", m_ValMaxPos, 0.0f, 0.5f, AnimationController::INTERPOLATOR_SIMPLESPLINE, NULL );
		// set the visiblity of our controls
		vgui::GetAnimationController()->RunAnimationCommand( m_pOpenNewsButton, "Alpha", 0, 0.0f, 0.1f, AnimationController::INTERPOLATOR_SIMPLESPLINE, NULL );
		vgui::GetAnimationController()->RunAnimationCommand( m_pTitleLabel, "Alpha", 255, 0.1f, 0.4f, AnimationController::INTERPOLATOR_SIMPLESPLINE, NULL );
		vgui::GetAnimationController()->RunAnimationCommand( m_pAuthorLabel, "Alpha", 255, 0.1f, 0.4f, AnimationController::INTERPOLATOR_SIMPLESPLINE, NULL );
		vgui::GetAnimationController()->RunAnimationCommand( m_pRichText, "Alpha", 255, 0.1f, 0.4f, AnimationController::INTERPOLATOR_SIMPLESPLINE, NULL );
		SetBgColor( m_DefaultColor );
		m_bOpened = true;
	}
}

void CPFNewsPanel::ResetPanel()
{
	vgui::GetAnimationController()->CancelAnimationsForPanel( this );
	m_pOpenNewsButton->SetAlpha( 255 );
	m_pOpenNewsButton->SetVisible( true );
	m_pOpenNewsButton->SetButtonImageVisible( true );
	m_pTitleLabel->SetAlpha( 0 );
	m_pAuthorLabel->SetAlpha( 0 );
	m_pRichText->SetAlpha( 0 );
	SetBounds( m_ValMinPos.a, m_ValMinPos.b, m_ValMinSize.a, m_ValMinSize.b );
	m_bOpened = false;
}