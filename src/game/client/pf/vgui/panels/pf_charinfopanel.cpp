//=============================================================================
//
// Purpose: Replicate the 2008 character info and loadout panels
// This is kind of a clunky mess, but it works. This has been commented pretty
// aggressively so if you're lost, I can't help you. Read it from top to bottom
// Cyanide 11/03/2022;
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"

#include "tf_shareddefs.h"
#include <vgui/IScheme.h>
#include "pf_charinfopanel.h"
#include <vgui/ILocalize.h>
#include "ienginevgui.h"
#include "c_tf_player.h"
#include "tf_inventory.h"
#include <vgui/MouseCode.h>
#include "vgui/IInput.h"
#include "fmtstr.h"
#include "pf_mainmenu_override.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

CPFCharInfoPanel *g_pPFCharacterInfoPanel = NULL;

CPFCharInfoPanel *GetCharacterInfoPanel()
{
	return g_pPFCharacterInfoPanel;
}

void CreatePFInventoryPanel()
{
	if( NULL == g_pPFCharacterInfoPanel )
	{
		g_pPFCharacterInfoPanel = new CPFCharInfoPanel( nullptr );
	}
}

void DestroyPFInventoryPanel()
{
	if( NULL != g_pPFCharacterInfoPanel )
	{
		delete g_pPFCharacterInfoPanel;
		g_pPFCharacterInfoPanel = NULL;
	}
}

//=============================================================================
// Character info panel
//=============================================================================

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CPFCharInfoPanel::CPFCharInfoPanel( vgui::Panel *parent ) : PropertyDialog( parent, "character_info" )
{
	vgui::VPANEL gameuiPanel = enginevgui->GetPanel( PANEL_GAMEUIDLL );
	SetParent( gameuiPanel );

	SetMoveable( false );
	SetSizeable( false );
	MakePopup( true );

	// need to set the scheme because fuck you
	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme" );
	SetScheme( scheme );
	SetProportional( true );

	// We don't actually want to show these buttons
	SetCancelButtonVisible( false );
	SetOKButtonVisible( false );
	m_bCloseExits = false;
#ifdef PF2BETA
	m_pCharInfoPanel = new CPFCharInfoLoadoutSubPanel( this );
#endif
	m_pStatsPanel = new CTFStatsSummaryPanel( this );

	// Set up the stats panel to be displayed as a property page
	m_pStatsPanel->SetupForEmbedded(); // Cyanide; having an non-embedded version of the stats panel is technically redundant now
	GetStatPanel()->UpdateStatSummaryPanel();

	// PFTODO - make these localised
#ifdef PF2BETA
	AddPage( m_pCharInfoPanel, "LOADOUT" );
#endif
	AddPage( m_pStatsPanel, "STATS" );

	// Have to reload the schema here a first time here
	InvalidateLayout( false, true );

	ListenForGameEvent( "gameui_hidden" );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CPFCharInfoPanel::~CPFCharInfoPanel()
{
}

//-----------------------------------------------------------------------------
// Purpose: Applies scheme settings
//-----------------------------------------------------------------------------
void CPFCharInfoPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	SetProportional( true );
	LoadControlSettings( "Resource/UI/CharInfoPanel.res" );
#ifdef PF2BETA
	GetPropertySheet()->SetActivePage( m_pCharInfoPanel ); // set our initial page as the loadout selection page
#else
	GetPropertySheet()->SetActivePage( m_pStatsPanel );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Command handler
//-----------------------------------------------------------------------------
void CPFCharInfoPanel::OnCommand( const char *command )
{
	if( !Q_stricmp( command, "Close" ) )
	{
		CloseModal();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CPFCharInfoPanel::FireGameEvent( IGameEvent *event )
{
	if( !Q_stricmp( event->GetName(), "gameui_hidden" ) )
	{
		// only hide me only if I'm visible
		if( IsVisible() )
			CloseModal();
	}
}

void CPFCharInfoPanel::CloseModal()
{
	if( m_bCloseExits )
	{
		engine->ClientCmd_Unrestricted( "gameui_hide" );
	}
	SetVisible( false );
}

//-----------------------------------------------------------------------------
// Purpose: Shows this dialog as a modal dialog
//-----------------------------------------------------------------------------
void CPFCharInfoPanel::ShowModal( int iClass )
{
	InvalidateLayout( true, true );
	SetVisible( true );
	MoveToFront();

	// set this so we return to the game when we hit close
	bool bShowLoadout = iClass != -1;
	m_bCloseExits = bShowLoadout;

	// Update the stat summary panel with our stats
	GetStatPanel()->UpdateStatSummaryPanel();
#ifdef PF2BETA
	GetPropertySheet()->SetActivePage( m_pCharInfoPanel );


	// are we supposed to be directly accessing a class loadout?
	if( bShowLoadout )
	{
		m_pCharInfoPanel->OpenFullLoadout( iClass );
	}
#else
	GetPropertySheet()->SetActivePage( m_pStatsPanel );
#endif
}

// used for accessing the menu through the main menu
CON_COMMAND( open_charinfo, "Shows the character info menu" )
{
	GetCharacterInfoPanel()->ShowModal(-1);
}


#ifdef PF2BETA
// used for accessing a specific loadout while in-game
CON_COMMAND( open_charinfo_direct, "Shows the character info loadout menu" )
{
	// open gameui
	engine->ClientCmd_Unrestricted( "gameui_activate" );
	// Was an argument supplied with the command? This is done on the class selection screen edit loadout button
	if( args.ArgC() == 2 )
	{
		GetCharacterInfoPanel()->ShowModal( Q_atoi( args[1] ) );
	}
	else
	{
		GetCharacterInfoPanel()->ShowModal( 0 );
	}
}
#endif
