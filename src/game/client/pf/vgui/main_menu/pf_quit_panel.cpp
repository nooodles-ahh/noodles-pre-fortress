//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"

#include <vgui_controls/Label.h>
#include <vgui_controls/PanelListPanel.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include <vgui/IInput.h>
#include "ienginevgui.h"
#include "pf_quit_panel.h"
#include "filesystem.h"
#include "fmtstr.h"
#include "pf_mainmenu_override.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

CPFQuitQueryPanel *g_pPFQuitQueryPanel = NULL;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPFQuitQueryPanel *GPFQuitQueryPanel()
{
	// don't open this is the menu isn't being used
	if( !guiroot )
		return nullptr;

	if( NULL == g_pPFQuitQueryPanel )
	{
		g_pPFQuitQueryPanel = new CPFQuitQueryPanel( guiroot );
	}
	return g_pPFQuitQueryPanel;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CPFQuitQueryPanel::CPFQuitQueryPanel( vgui::Panel *parent ) : BaseClass( parent, "PFQuitQueryPanel", false, true )
{
	SetParent( parent );
	SetProportional( true );
	SetVisible( true );
	SetDeleteSelfOnClose( true );

	SetMoveable( false );
	SetSizeable( false );
	MakePopup( false, true );

	int x, y;
	int width, height;
	//surface()->GetScreenSize(width, height);
	GetParent()->GetBounds( x, y, width, height );
	SetSize( width, height );
	SetPos( 0, 0 );

	InvalidateLayout( true, true );

	guiroot->ShowMainMenu( false );
}

CPFQuitQueryPanel::~CPFQuitQueryPanel()
{
	g_pPFQuitQueryPanel = nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: Applies scheme settings
//-----------------------------------------------------------------------------
void CPFQuitQueryPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	SetProportional( true );
	LoadControlSettings( "Resource/UI/main_menu/quitquery.res" );
}

//-----------------------------------------------------------------------------
// Purpose: Command handler
//-----------------------------------------------------------------------------
void CPFQuitQueryPanel::OnCommand( const char *command )
{
	if( !Q_stricmp( command, "Close" ) )
	{
		Close();
	}
	else
	{
		engine->ExecuteClientCmd( command );
	}
}

void CPFQuitQueryPanel::OnKeyCodePressed( vgui::KeyCode code )
{
	// ESC Closes
	if( code == KEY_ESCAPE )
	{
		Close();
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}

void CPFQuitQueryPanel::PaintBackground()
{
	SetPaintBackgroundType( 0 );
	BaseClass::PaintBackground();
}

void CPFQuitQueryPanel::OnClose()
{
	BaseClass::OnClose();
	// return to the main menu is we don't quit
	guiroot->ShowMainMenu( true );
}

CON_COMMAND( OpenQuitQuery, "Shows the quit dialog" )
{
	GPFQuitQueryPanel();
}