#include "cbase.h"
#include "pf_mainmenu_override.h"
#include "ioverrideinterface.h"

#include <vgui/IScheme.h>
#include "vgui/ILocalize.h"
#include "vgui/IPanel.h"
#include "vgui/ISurface.h"
#include "vgui/ISystem.h"
#include "vgui/IVGui.h"
#include "ienginevgui.h"
#include <engine/IEngineSound.h>
#include "filesystem.h"
#include <vgui_controls/EditablePanel.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

// See interface.h/.cpp for specifics:  basically this ensures that we actually Sys_UnloadModule the dll and that we don't call Sys_LoadModule 
//  over and over again.
static CDllDemandLoader g_GameUIDLL( "GameUI" );

OverrideUI_RootPanel *guiroot = NULL;

void OverrideGameUI()
{
	if( !OverrideUI->GetPanel() )
	{
		OverrideUI->Create(NULL);
	}
	if( guiroot->GetGameUI() )
	{
		guiroot->GetGameUI()->SetMainMenuOverride( guiroot->GetVPanel() );
		ConColorMsg( Color( 255, 255, 64, 255 ), "Overriding mainmenu!\n" );
		return;
	}
}

CON_COMMAND( pf_mainmenu_reload, "Reload Main Menu" )
{
	guiroot->InvalidateLayout( true, true );
}


OverrideUI_RootPanel::OverrideUI_RootPanel(VPANEL parent) : BaseClass( NULL, "MainMenuOverride" )
{
	SetParent(parent);

	guiroot = this;
	gameui = NULL;
	LoadGameUI();
	SetScheme( "ClientScheme" );

	SetDragEnabled( false );
	SetShowDragHelper( false );
	SetProportional( true );
	SetVisible( true );

	int width, height;
	surface()->GetScreenSize( width, height );
	SetSize( width, height );
	SetPos( 0, 0 );

	m_bInGame = false;

	m_pMainMenu = nullptr;
	m_pToolTip = nullptr;

	m_bForceHideMenu = false;

	m_iDefaultBGTeam = TEAM_INVALID;

	m_pToolTip = new CMainMenuToolTip( this, NULL );
	m_pToolTipPanel = new CPFEditablePanel( this, "TooltipPanel" );
	m_pToolTip->pPanel = m_pToolTipPanel;
	m_pToolTip->SetTooltipDelay( 0 );

	vgui::ivgui()->AddTickSignal( GetVPanel() );
}

IGameUI *OverrideUI_RootPanel::GetGameUI()
{
	if( !gameui )
	{
		if ( !LoadGameUI() )
			return NULL;
	}

	return gameui;
}

bool OverrideUI_RootPanel::LoadGameUI()
{
	if( !gameui )
	{
		CreateInterfaceFn gameUIFactory = g_GameUIDLL.GetFactory();
		if ( gameUIFactory )
		{
			gameui = (IGameUI *) gameUIFactory(GAMEUI_INTERFACE_VERSION, NULL);
			if( !gameui )
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}
	return true;
}

OverrideUI_RootPanel::~OverrideUI_RootPanel()
{
	gameui = NULL;
	g_GameUIDLL.Unload();
}

void OverrideUI_RootPanel::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	// Resize the panel to the screen size
	// Otherwise, it'll just be in a little corner
	int wide, tall;
	vgui::surface()->GetScreenSize(wide, tall);
	SetSize(wide,tall);

	SetProportional( true );
	LoadControlSettings( "Resource/UI/main_menu/mainmenuoverride.res" );

	// Cyanide; This might not be ideal
	if( !m_pMainMenu )
	{
		char szBGName[128];
		engine->GetMainMenuBackgroundName( szBGName, sizeof( szBGName ) );
		int i = 0;
		char num = 48;
		while( szBGName[i] != '\0' )
		{
			num = szBGName[i];
			++i;
		}

		// make bg blue if it's an even bg
		if( (num - 48) % 2 == 0 )
			m_iDefaultBGTeam = TF_TEAM_BLUE;

		m_pMainMenu = new CPFMainMenuRoot( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Actually execute commands
//-----------------------------------------------------------------------------
void OverrideUI_RootPanel::OnCommand( const char *command )
{
	engine->ExecuteClientCmd( command );
}


void OverrideUI_RootPanel::ShowMainMenu( bool bShow )
{
	m_pMainMenu->SetVisible( bShow );
	m_bForceHideMenu = !bShow;
}

void OverrideUI_RootPanel::OnTick()
{
	BaseClass::OnTick();
	if( !engine->IsDrawingLoadingImage() && !IsVisible() )
	{
		SetVisible( true );
	}
	else if( engine->IsDrawingLoadingImage() && engine->IsLevelMainMenuBackground() && IsVisible() )
	{
		SetVisible( false );
	}

	// Cyanide; only invalidate if we're visible
	if( IsVisible() )
	{
		// Set the menu's visibility if we're not forcing it off (i.e. the quit query)
		if( m_pMainMenu && !m_bForceHideMenu )
			m_pMainMenu->SetVisible( !engine->IsDrawingLoadingImage() );

		// was ingame
		if( IsInGame() == nullptr && m_bInGame )
		{
			m_bInGame = false;
			guiroot->InvalidateLayout( true, true );
		}
		// was in menu
		else if( IsInGame() && !m_bInGame )
		{
			m_bInGame = true;
			guiroot->InvalidateLayout( true, true );
		}
	}
}

C_BasePlayer *OverrideUI_RootPanel::IsInGame()
{
	if( engine->IsLevelMainMenuBackground() )
		return nullptr;

	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if( pPlayer && IsVisible() )
	{
		return pPlayer;
	}
	else
	{
		return nullptr;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called from CPFWebStuff to tell the server count text to update
//-----------------------------------------------------------------------------
void OverrideUI_RootPanel::UpdateServerCount()
{
	m_pMainMenu->UpdateServerCount();
}

//-----------------------------------------------------------------------------
// Purpose: Called from CPFWebStuff to tell the server count text to update
//-----------------------------------------------------------------------------
void OverrideUI_RootPanel::UpdateNews()
{
	m_pMainMenu->UpdateNews();
}