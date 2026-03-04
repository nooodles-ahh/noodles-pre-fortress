//=============================================================================
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"

#include <vgui_controls/Label.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include <vgui/IInput.h>
#include "ienginevgui.h"
#include "pf_mainmenu_panel.h"
#include "pf_mainmenu_override.h"
#include "pf_webstuff.h"
#include <vgui_controls/AnimationController.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

void AddSubKeyNamed( KeyValues *pKeys, const char *pszName );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CPFMainMenuRoot::CPFMainMenuRoot( vgui::Panel *parent ) : BaseClass( parent, "PFMainMenu" )
{
	m_pFakeBGImage = nullptr;
	m_pTopBanner = nullptr;
	m_pBotBanner = nullptr;
	m_pLogoImage = nullptr;
	m_pVerLabel = nullptr;

	m_pMainMenuPanel = new CPFMainMenuPanel(this);
	m_pNewsPanel = new CPFNewsPanel(this);

	m_iInit = 4;

	m_bDoOpenAnim = false;

	// Cyanide; IMPORTANT! need to get pointers to all our controls before Think can run
	InvalidateLayout( true, true );

	ListenForGameEvent( "gameui_hidden" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPFMainMenuRoot::~CPFMainMenuRoot()
{
	delete[] m_pMainMenuPanel;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPFMainMenuRoot::FireGameEvent( IGameEvent *event )
{
	if( FStrEq( "gameui_hidden", event->GetName() ) )
	{
		m_bDoOpenAnim = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPFMainMenuRoot::PerformLayout()
{
	BaseClass::PerformLayout();

	m_iTopBannerPos[0] = m_pTopBanner->GetXPos();
	m_iTopBannerPos[1] = m_pTopBanner->GetYPos();

	m_iBotBannerPos[0] = m_pBotBanner->GetXPos();
	m_iBotBannerPos[1] = m_pBotBanner->GetYPos();

	if( m_iInit > 0 && !(engine->IsInGame() && !engine->IsLevelMainMenuBackground()))
	{
		// set the initial position of the main panel
		m_pMainMenuPanel->SetPos( -150, 0 );
		// set the initial positions of our top and bottom images
		m_pTopBanner->SetPos( m_iTopBannerPos[0], m_iTopBannerPos[1] - m_pTopBanner->GetTall() );
		m_pBotBanner->SetPos( m_iBotBannerPos[0], m_iBotBannerPos[1] + m_pBotBanner->GetTall() );
		m_pLogoImage->SetAlpha( 0 );

		// Self explanitory, it would be insane to change the transparency of every element
		// of the panel so an image gets set the same image as the background and the
		// alpha is applied to that instead
		char szBGName[128];
		engine->GetMainMenuBackgroundName( szBGName, sizeof( szBGName ) );
		char szImage[128];
		Q_snprintf( szImage, sizeof( szImage ), "../console/%s", szBGName );
		int width, height;
		surface()->GetScreenSize( width, height );
		float fRatio = (float)width / (float)height;
		bool bWidescreen = (fRatio < 1.5 ? false : true);
		if( bWidescreen )
			Q_strcat( szImage, "_widescreen", sizeof( szImage ) );
		m_pFakeBGImage->SetImage( szImage );
		m_pFakeBGImage->SetVisible( true );
		m_pFakeBGImage->SetAlpha( 255 );
	}
	else
	{
		m_iInit = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Applies scheme settings
//-----------------------------------------------------------------------------
void CPFMainMenuRoot::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	KeyValues *pConditions = nullptr;

	if( guiroot->IsInGame() )
	{
		pConditions = new KeyValues( "conditions" );
		AddSubKeyNamed( pConditions, "if_ingame" );
	}

	LoadControlSettings( "Resource/UI/main_menu/mainmenubanners.res", NULL, NULL, pConditions );

	if( pConditions )
	{
		pConditions->deleteThis();
	}

	m_pMainMenuPanel->SetDialogVariable( "servercount", g_pVGuiLocalize->Find( "#PF_Menu_CountingServers" ) );

	if( GetWebStuff()->GetCurrentServerCount() != -1 )
		UpdateServerCount();

	m_pFakeBGImage = dynamic_cast<vgui::ImagePanel *>(FindChildByName( "FakeBGImage" ));

	m_pTopBanner = dynamic_cast<vgui::ImagePanel *>(FindChildByName( "BannerImage" ));
	m_pLogoImage = dynamic_cast<vgui::ImagePanel *>(FindChildByName( "Logo" ));
	m_pBotBanner = dynamic_cast<vgui::ImagePanel *>(FindChildByName( "FooterPanel" ));
	m_pVerLabel = dynamic_cast<CExLabel *>(FindChildByName( "VersionLabel" ));

	if( m_pVerLabel )
	{
		m_pVerLabel->SetText( VarArgs("v%s", GetWebStuff()->m_szVerString) );
		if( GetWebStuff()->m_bNeedsUpdating && m_pVerLabel )
		{
			m_pVerLabel->SetFgColor( m_UpdateColor );
		}
	}

	BaseClass::ApplySchemeSettings( pScheme );
}

void CPFMainMenuRoot::OnThink()
{
	if( m_iInit > 0 )
	{
		// if we're loading into a background map we need to wait extra time
		if( engine->IsDrawingLoadingImage() && engine->IsLevelMainMenuBackground() )
		{
			m_iInit += 2;
		}
		else if( engine->IsInGame() && !engine->IsLevelMainMenuBackground() )
		{
			m_iInit = 0;
		}

		m_bDoOpenAnim = false;

		// Cyanide; nonsense incrementor from TF2C. It exists to prevent the animation command running too early as it'll stutter otherwise
		--m_iInit;
		if( m_iInit == 0 )
		{
			vgui::GetAnimationController()->RunAnimationCommand( m_pLogoImage, "Alpha", 255, 0.0f, 0.8f, vgui::AnimationController::INTERPOLATOR_SIMPLESPLINE );
			
			AnimationController::PublicValue_t p_AnimFakeBG( 0, 0 );
			vgui::GetAnimationController()->RunAnimationCommand( m_pMainMenuPanel, "Position", p_AnimFakeBG, 0.0f, 0.5f, AnimationController::INTERPOLATOR_DEACCEL, NULL );
			if( engine->IsLevelMainMenuBackground() )
			{
				m_pFakeBGImage->SetVisible( false );
				m_pMainMenuPanel->SetAlpha( 50 );
				vgui::GetAnimationController()->RunAnimationCommand( m_pMainMenuPanel, "Alpha", 255, 0.0f, 0.6f, vgui::AnimationController::INTERPOLATOR_SIMPLESPLINE );
			}
			else
				vgui::GetAnimationController()->RunAnimationCommand( m_pFakeBGImage, "Alpha", 0, 0.0f, 0.6f, vgui::AnimationController::INTERPOLATOR_SIMPLESPLINE );
			
			AnimationController::PublicValue_t p_AnimTop( m_pTopBanner->GetXPos(), m_pTopBanner->GetYPos() + m_pTopBanner->GetTall());
			AnimationController::PublicValue_t p_AnimBot( m_pBotBanner->GetXPos(), m_pBotBanner->GetYPos() - m_pBotBanner->GetTall() );
			vgui::GetAnimationController()->RunAnimationCommand( m_pTopBanner, "Position", p_AnimTop, 0.0f, 0.5f, AnimationController::INTERPOLATOR_DEACCEL, NULL );
			vgui::GetAnimationController()->RunAnimationCommand( m_pBotBanner, "Position", p_AnimBot, 0.0f, 0.5f, AnimationController::INTERPOLATOR_DEACCEL, NULL );
		}
	}
	if( m_pFakeBGImage && m_pFakeBGImage->IsVisible() && m_pFakeBGImage->GetAlpha() == 0 )
	{
		m_pFakeBGImage->SetVisible( false );
	}

	if( m_bDoOpenAnim )
	{
		vgui::GetAnimationController()->CancelAnimationsForPanel( this );
		vgui::GetAnimationController()->CancelAnimationsForPanel( m_pMainMenuPanel );

		// set the initial position of the main panel
		m_pMainMenuPanel->SetPos( -50, 0 );
		// Cyanide; There is a bug where if the alpha is below a certain point corners of Panels don't draw
		m_pMainMenuPanel->SetAlpha( 50 );
		// set the initial positions of our top and bottom images
		m_pTopBanner->SetPos( m_iTopBannerPos[0], m_iTopBannerPos[1] - m_pTopBanner->GetTall() );
		m_pBotBanner->SetPos( m_iBotBannerPos[0], m_iBotBannerPos[1] + m_pBotBanner->GetTall() );
		m_pLogoImage->SetAlpha( 0 );


		vgui::GetAnimationController()->RunAnimationCommand( m_pLogoImage, "Alpha", 255, 0.0f, 0.4f, vgui::AnimationController::INTERPOLATOR_SIMPLESPLINE );

		AnimationController::PublicValue_t p_AnimFakeBG( 0, 0 );
		vgui::GetAnimationController()->RunAnimationCommand( m_pMainMenuPanel, "Position", p_AnimFakeBG, 0.0f, 0.25f, AnimationController::INTERPOLATOR_DEACCEL, NULL );
		vgui::GetAnimationController()->RunAnimationCommand( m_pMainMenuPanel, "Alpha", 255, 0.0f, 0.25f, vgui::AnimationController::INTERPOLATOR_SIMPLESPLINE );

		AnimationController::PublicValue_t p_AnimTop( m_pTopBanner->GetXPos(), m_pTopBanner->GetYPos() + m_pTopBanner->GetTall() );
		AnimationController::PublicValue_t p_AnimBot( m_pBotBanner->GetXPos(), m_pBotBanner->GetYPos() - m_pBotBanner->GetTall() );
		vgui::GetAnimationController()->RunAnimationCommand( m_pTopBanner, "Position", p_AnimTop, 0.0f, 0.25f, AnimationController::INTERPOLATOR_DEACCEL, NULL );
		vgui::GetAnimationController()->RunAnimationCommand( m_pBotBanner, "Position", p_AnimBot, 0.0f, 0.25f, AnimationController::INTERPOLATOR_DEACCEL, NULL );

		m_bDoOpenAnim = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPFMainMenuRoot::UpdateServerCount()
{
	int iCount = GetWebStuff()->GetCurrentServerCount();

	if( iCount != -1 )
	{
		wchar_t wszLocalized[64];
		wchar_t wszCount[4];
		_snwprintf( wszCount, sizeof( wszCount ) / sizeof( wchar_t ), L"%d", iCount );
		g_pVGuiLocalize->ConstructString( wszLocalized, sizeof( wszLocalized ),
			g_pVGuiLocalize->Find( iCount == 1 ? "#PF_Menu_ServerCountOne" : "#PF_Menu_ServerCountMany" ), 1, wszCount );

		m_pMainMenuPanel->SetDialogVariable( "servercount", wszLocalized );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPFMainMenuRoot::UpdateNews()
{
	// lol lmao
	if( GetWebStuff()->m_bNeedsUpdating && m_pVerLabel )
	{
		m_pVerLabel->SetFgColor( m_UpdateColor );
	}
	m_pNewsPanel->UpdateNews();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPFMainMenuRoot::MinimizeNews()
{
	if( m_pNewsPanel->m_bOpened )
		m_pNewsPanel->MinimizePanel();
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CPFMainMenuPanel::CPFMainMenuPanel( vgui::Panel *parent ) : BaseClass( parent, "PFMainMenu" )
{
	SetParent( parent );
	SetProportional( true );
	SetVisible( true );

	int x, y;
	int width, height;
	GetParent()->GetBounds( x, y, width, height );
	SetSize( width, height );
}

//-----------------------------------------------------------------------------
// Purpose: Applies scheme settings
//-----------------------------------------------------------------------------
void CPFMainMenuPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	KeyValues *pConditions = nullptr;

	if( guiroot->IsInGame() )
	{
		pConditions = new KeyValues( "conditions" );
		AddSubKeyNamed( pConditions, "if_ingame" );
	}

	LoadControlSettings( "Resource/UI/main_menu/mainmenupanel.res", NULL, NULL, pConditions );

	if( pConditions )
	{
		pConditions->deleteThis();
	}

	BaseClass::ApplySchemeSettings( pScheme );

	SetProportional( true );
}

//-----------------------------------------------------------------------------
// Purpose: Command handler
//-----------------------------------------------------------------------------
void CPFMainMenuPanel::OnCommand( const char *command )
{
	engine->ExecuteClientCmd( command );
}

//-----------------------------------------------------------------------------
void CPFMainMenuPanel::OnMenuButtonArmed()
{
	// Need to tell the parent that we want the news minimized
	dynamic_cast<CPFMainMenuRoot *>(GetParent())->MinimizeNews();
}