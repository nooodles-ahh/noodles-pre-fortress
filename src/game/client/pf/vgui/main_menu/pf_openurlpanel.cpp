//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include <vgui/IInput.h>
#include <vgui/ISystem.h>
#include "ienginevgui.h"
#include "pf_openurlpanel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

CPFOpenURLPanel *g_pPFWebPanel = NULL;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPFOpenURLPanel *GPFWebPanel()
{
	if( NULL == g_pPFWebPanel )
	{
		g_pPFWebPanel = new CPFOpenURLPanel( nullptr );
	}
	return g_pPFWebPanel;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CPFOpenURLPanel::CPFOpenURLPanel( vgui::Panel *parent ) : BaseClass( parent, "PFOpenURLPanel", false, true )
{
	vgui::VPANEL pParent = enginevgui->GetPanel( PANEL_GAMEUIDLL );
	SetParent( pParent );

	SetMoveable( false );
	SetSizeable( false );
	MakePopup( true );
	SetDeleteSelfOnClose(true);

	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme" );
	SetScheme( scheme );
	SetProportional( true );

	InvalidateLayout( true, true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPFOpenURLPanel::~CPFOpenURLPanel()
{
	g_pPFWebPanel = nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: Applies scheme settings
//-----------------------------------------------------------------------------
void CPFOpenURLPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	SetProportional( true );
	LoadControlSettings( "Resource/UI/pfopenurl.res" );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CPFOpenURLPanel::OnKeyCodeTyped(vgui::KeyCode code)
{
	if ( code == KEY_ESCAPE )
	{
		Close();
	}
	else
	{
		BaseClass::OnKeyCodeTyped( code );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPFOpenURLPanel::OnKeyCodePressed(vgui::KeyCode code)
{
	if ( GetBaseButtonCode( code ) == KEY_XBUTTON_B )
	{
		Close();
	}
	else if ( code == KEY_ENTER )
	{
		// do nothing, the default is to close the panel!
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Command handler
//-----------------------------------------------------------------------------
void CPFOpenURLPanel::OnCommand( const char *command )
{
	if( !Q_stricmp( command, "Close" ) )
	{
		Close();
	}
	else if( !Q_stricmp( command, "OpenBrowser" ) )
	{
		system()->ShellExecute( "open", m_sURL );
		Close();
	}
	else if( !Q_stricmp( command, "OpenOverlay" ) )
	{
		steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( m_sURL );
		Close();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Shows this dialog as a modal dialog
//-----------------------------------------------------------------------------
void CPFOpenURLPanel::ShowModal(const char *url)
{
	SetVisible( true );
	MoveToFront();
	DoModal();

	m_sURL = url;

	int x, y, ww, wt, wide, tall;
    vgui::surface()->GetWorkspaceBounds( x, y, ww, wt );
    GetSize(wide, tall);

    // Center it, keeping requested size
    SetPos(x + ((ww - wide) / 2), y + ((wt - tall) / 2));
}