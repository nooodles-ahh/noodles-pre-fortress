//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "c_tf_player.h"
#include "iclientmode.h"
#include "ienginevgui.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include "c_baseobject.h"
#include "IconPanel.h"
#include "tf_controls.h"

#include "tf_hud_menu_spy_disguise.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//======================================

DECLARE_HUDELEMENT( CHudMenuSpyDisguise );

ConVar tf_simple_disguise_menu( "tf_simple_disguise_menu", "0", FCVAR_ARCHIVE, "Use a more concise disguise selection menu." );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudMenuSpyDisguise::CHudMenuSpyDisguise( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudMenuSpyDisguise" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	for ( int i=0; i<9; i++ )
	{
		char buf[32];
		Q_snprintf( buf, sizeof(buf), "class_item_red_%d", i+1 );
		m_pClassItems_Red[i] = new EditablePanel( this, buf );
		m_pKeyBg_Red[i] = new CIconPanel( m_pClassItems_Red[i], "NumberBg" );
		m_pKeyLabel_Red[i] = new CExLabel( m_pClassItems_Red[i], "NumberLabel", "" );
		m_pKeySimpleLabel_Red[i] = new CExLabel( m_pClassItems_Red[i], "NewNumberLabel", "" );

		Q_snprintf( buf, sizeof(buf), "class_item_blue_%d", i+1 );
		m_pClassItems_Blue[i] = new EditablePanel( this, buf );
		m_pKeyBg_Blue[i] = new CIconPanel( m_pClassItems_Blue[i], "NumberBg" );
		m_pKeyLabel_Blue[i] = new CExLabel( m_pClassItems_Blue[i], "NumberLabel", "" );
		m_pKeySimpleLabel_Blue[i] = new CExLabel( m_pClassItems_Blue[i], "NewNumberLabel", "" );
	}

	for( int i = 0; i < 3; i++ )
	{
		char buf[32];
		Q_snprintf( buf, sizeof( buf ), "NumberBg%d", i + 1 );
		m_pSimpleIcons[i] = new CIconPanel( this, buf );

		Q_snprintf( buf, sizeof( buf ), "NumberLabel%d", i + 1 );
		m_pSimpleLabels[i] = new CExLabel( this, buf, "");
	}

	m_iShowingTeam = TF_TEAM_RED;

	ListenForGameEvent( "spy_pda_reset" );

	m_iSelectedItem = -1;

	m_pActiveSelection = NULL;

	InvalidateLayout( false, true );

	m_bInConsoleMode = false;

	m_iSelectedGroup = -1;

	RegisterForRenderGroup( "mid" );
}

ConVar tf_disguise_menu_controller_mode( "tf_disguise_menu_controller_mode", "0", FCVAR_ARCHIVE, "Use console controller disguise menus. 1 = ON, 0 = OFF." );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMenuSpyDisguise::ApplySchemeSettings( IScheme *pScheme )
{
	bool b360Style = ( IsConsole() || tf_disguise_menu_controller_mode.GetBool() );

	if ( b360Style )
	{
		// load control settings...
		LoadControlSettings( "resource/UI/disguise_menu_360/HudMenuSpyDisguise.res" );

		m_pClassItems_Red[0]->LoadControlSettings( "resource/UI/disguise_menu_360/scout_red.res" );
		m_pClassItems_Red[1]->LoadControlSettings( "resource/UI/disguise_menu_360/soldier_red.res" );
		m_pClassItems_Red[2]->LoadControlSettings( "resource/UI/disguise_menu_360/pyro_red.res" );
		m_pClassItems_Red[3]->LoadControlSettings( "resource/UI/disguise_menu_360/demoman_red.res" );
		m_pClassItems_Red[4]->LoadControlSettings( "resource/UI/disguise_menu_360/heavy_red.res" );
		m_pClassItems_Red[5]->LoadControlSettings( "resource/UI/disguise_menu_360/engineer_red.res" );
		m_pClassItems_Red[6]->LoadControlSettings( "resource/UI/disguise_menu_360/medic_red.res" );
		m_pClassItems_Red[7]->LoadControlSettings( "resource/UI/disguise_menu_360/sniper_red.res" );
		m_pClassItems_Red[8]->LoadControlSettings( "resource/UI/disguise_menu_360/spy_red.res" );

		m_pClassItems_Blue[0]->LoadControlSettings( "resource/UI/disguise_menu_360/scout_blue.res" );
		m_pClassItems_Blue[1]->LoadControlSettings( "resource/UI/disguise_menu_360/soldier_blue.res" );
		m_pClassItems_Blue[2]->LoadControlSettings( "resource/UI/disguise_menu_360/pyro_blue.res" );
		m_pClassItems_Blue[3]->LoadControlSettings( "resource/UI/disguise_menu_360/demoman_blue.res" );
		m_pClassItems_Blue[4]->LoadControlSettings( "resource/UI/disguise_menu_360/heavy_blue.res" );
		m_pClassItems_Blue[5]->LoadControlSettings( "resource/UI/disguise_menu_360/engineer_blue.res" );
		m_pClassItems_Blue[6]->LoadControlSettings( "resource/UI/disguise_menu_360/medic_blue.res" );
		m_pClassItems_Blue[7]->LoadControlSettings( "resource/UI/disguise_menu_360/sniper_blue.res" );
		m_pClassItems_Blue[8]->LoadControlSettings( "resource/UI/disguise_menu_360/spy_blue.res" );

		m_pActiveSelection = dynamic_cast< EditablePanel * >( FindChildByName( "active_selection_bg" ) );

		// Reposition the activeselection to the default position
		m_iSelectedItem = -1;	// force reposition
		SetSelectedItem( 5 );
	}
	else
	{
		// load control settings...
		LoadControlSettings( "resource/UI/disguise_menu/HudMenuSpyDisguise.res" );

		m_pClassItems_Red[0]->LoadControlSettings( "resource/UI/disguise_menu/scout_red.res" );
		m_pClassItems_Red[1]->LoadControlSettings( "resource/UI/disguise_menu/soldier_red.res" );
		m_pClassItems_Red[2]->LoadControlSettings( "resource/UI/disguise_menu/pyro_red.res" );
		m_pClassItems_Red[3]->LoadControlSettings( "resource/UI/disguise_menu/demoman_red.res" );
		m_pClassItems_Red[4]->LoadControlSettings( "resource/UI/disguise_menu/heavy_red.res" );
		m_pClassItems_Red[5]->LoadControlSettings( "resource/UI/disguise_menu/engineer_red.res" );
		m_pClassItems_Red[6]->LoadControlSettings( "resource/UI/disguise_menu/medic_red.res" );
		m_pClassItems_Red[7]->LoadControlSettings( "resource/UI/disguise_menu/sniper_red.res" );
		m_pClassItems_Red[8]->LoadControlSettings( "resource/UI/disguise_menu/spy_red.res" );

		m_pClassItems_Blue[0]->LoadControlSettings( "resource/UI/disguise_menu/scout_blue.res" );
		m_pClassItems_Blue[1]->LoadControlSettings( "resource/UI/disguise_menu/soldier_blue.res" );
		m_pClassItems_Blue[2]->LoadControlSettings( "resource/UI/disguise_menu/pyro_blue.res" );
		m_pClassItems_Blue[3]->LoadControlSettings( "resource/UI/disguise_menu/demoman_blue.res" );
		m_pClassItems_Blue[4]->LoadControlSettings( "resource/UI/disguise_menu/heavy_blue.res" );
		m_pClassItems_Blue[5]->LoadControlSettings( "resource/UI/disguise_menu/engineer_blue.res" );
		m_pClassItems_Blue[6]->LoadControlSettings( "resource/UI/disguise_menu/medic_blue.res" );
		m_pClassItems_Blue[7]->LoadControlSettings( "resource/UI/disguise_menu/sniper_blue.res" );
		m_pClassItems_Blue[8]->LoadControlSettings( "resource/UI/disguise_menu/spy_blue.res" );

		m_pActiveSelection = NULL;
	}


	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudMenuSpyDisguise::ShouldDraw( void )
{
	CTFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return false;

	CTFWeaponBase *pWpn = pPlayer->GetActiveTFWeapon();

	if ( !pWpn )
		return false;

	// Don't show the menu for first person spectator
	if ( pPlayer != pWpn->GetOwner() )
		return false;

	if ( pPlayer->m_Shared.InCond( TF_COND_TAUNTING ) )
		return false;

	return ( pWpn->GetWeaponID() == TF_WEAPON_PDA_SPY );
}

//-----------------------------------------------------------------------------
// Purpose: Keyboard input hook. Return 0 if handled
//-----------------------------------------------------------------------------
int	CHudMenuSpyDisguise::HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
{
	if ( !ShouldDraw() )
	{
		return 1;
	}

	if ( !down )
	{
		return 1;
	}

	// menu classes are not in the same order as the defines
	static int iRemapKeyToClass[9] = 
	{
		TF_CLASS_SCOUT,
		TF_CLASS_SOLDIER,
		TF_CLASS_PYRO,
		TF_CLASS_DEMOMAN,
		TF_CLASS_HEAVYWEAPONS,
		TF_CLASS_ENGINEER,
		TF_CLASS_MEDIC,
		TF_CLASS_SNIPER,
		TF_CLASS_SPY
	};

	bool bController = ( IsConsole() || ( keynum >= JOYSTICK_FIRST ) );

	if ( bController )
	{
		int iNewSelection = m_iSelectedItem;

		switch( keynum )
		{
		case KEY_XBUTTON_UP:
			// jump to last
			iNewSelection = 9;
			break;

		case KEY_XBUTTON_DOWN:
			// jump to first
			iNewSelection = 1;
			break;

		case KEY_XBUTTON_RIGHT:
			// move selection to the right
			iNewSelection++;
			if ( iNewSelection > 9 )
				iNewSelection = 1;
			break;

		case KEY_XBUTTON_LEFT:
			// move selection to the right
			iNewSelection--;
			if ( iNewSelection < 1 )
				iNewSelection = 9;
			break;

		case KEY_XBUTTON_RTRIGGER:
		case KEY_XBUTTON_A:
			{
				// select disguise
				int iClass = iRemapKeyToClass[m_iSelectedItem-1];
				int iTeam = ( m_iShowingTeam == TF_TEAM_BLUE ) ? 1 : 0;

				SelectDisguise( iClass, iTeam );
			}
			return 0;

		case KEY_XBUTTON_Y:
			ToggleDisguiseTeam();
			return 0;

		case KEY_XBUTTON_B:
			// cancel, close the menu
			engine->ExecuteClientCmd( "lastinv" );
			return 0;

		default:
			return 1;	// key not handled
		}

		SetSelectedItem( iNewSelection );

		return 0;
	}
	else
	{
		switch( keynum )
		{
		case KEY_1:
		case KEY_2:
		case KEY_3:
		{
			// done here probably so we don't accidentally switch off the disguise kit
			if( tf_simple_disguise_menu.GetBool() )
			{
				if( m_iSelectedGroup == -1 )
				{
					m_iSelectedGroup = keynum - KEY_1;
					ToggleSelectionIcons( true );
				}
				else
				{
					int iClass = iRemapKeyToClass[(keynum - KEY_1) + (m_iSelectedGroup * 3)];
					int iTeam = (m_iShowingTeam == TF_TEAM_BLUE) ? 1 : 0;

					SelectDisguise( iClass, iTeam );
				}
				return 0;
			}
		}
		case KEY_4:
		case KEY_5:
		case KEY_6:
		case KEY_7:
		case KEY_8:
		case KEY_9:
			{
				if( tf_simple_disguise_menu.GetBool() )
					break;

				int iClass = iRemapKeyToClass[ keynum - KEY_1 ];
				int iTeam = ( m_iShowingTeam == TF_TEAM_BLUE ) ? 1 : 0;

				SelectDisguise( iClass, iTeam );
			}
			return 0;

		// make this rebindable
		case KEY_MINUS:
			ToggleDisguiseTeam();
			return 0;

		case KEY_R:
			ToggleDisguiseTeam();
			return 0;

		case KEY_0:
			// cancel, close the menu
			engine->ExecuteClientCmd( "lastinv" );
			return 0;

		default:
			return 1;	// key not handled
		}
	}
	

	return 1;	// key not handled
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMenuSpyDisguise::SelectDisguise( int iClass, int iTeam )
{
	CTFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer )
	{
		char szCmd[64];
		Q_snprintf( szCmd, sizeof(szCmd), "disguise %d %d; lastinv", iClass, iTeam );
		engine->ExecuteClientCmd( szCmd );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMenuSpyDisguise::ToggleDisguiseTeam( void )
{
	// flip the teams
	m_iShowingTeam = ( m_iShowingTeam == TF_TEAM_BLUE ) ? TF_TEAM_RED : TF_TEAM_BLUE;

	// show / hide the class items
	bool bShowBlue = ( m_iShowingTeam == TF_TEAM_BLUE );

	for ( int i=0; i<9; i++ )
	{
		m_pClassItems_Red[i]->SetVisible( !bShowBlue );
		m_pClassItems_Blue[i]->SetVisible( bShowBlue );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMenuSpyDisguise::SetSelectedItem( int iSlot )
{
	if ( m_iSelectedItem != iSlot )
	{
		m_iSelectedItem = iSlot;

		// move the selection item to the new position
		if ( m_pActiveSelection )
		{
			// move the selection background
			int x, y;
			m_pClassItems_Blue[m_iSelectedItem-1]->GetPos( x, y );
			m_pActiveSelection->SetPos( x, y );	
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMenuSpyDisguise::FireGameEvent( IGameEvent *event )
{
	const char * type = event->GetName();

	if ( Q_strcmp(type, "spy_pda_reset") == 0 )
	{
		CTFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pPlayer )
		{
			bool bShowBlue = ( pPlayer->GetTeamNumber() == TF_TEAM_RED );

			for ( int i=0; i<9; i++ )
			{
				m_pClassItems_Red[i]->SetVisible( !bShowBlue );
				m_pClassItems_Blue[i]->SetVisible( bShowBlue );
			}

			m_iShowingTeam = ( bShowBlue ) ? TF_TEAM_BLUE : TF_TEAM_RED;

			m_iSelectedGroup = -1;
			ToggleSelectionIcons( false );
		}
	}
	else
	{
		CHudElement::FireGameEvent( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMenuSpyDisguise::SetVisible( bool state )
{
	if ( state == true )
	{
		// close the weapon selection menu
		engine->ClientCmd( "cancelselect" );

		bool bConsoleMode = ( IsConsole() || tf_disguise_menu_controller_mode.GetBool() );
			
		if ( bConsoleMode != m_bInConsoleMode )
		{
			InvalidateLayout( true, true );
			m_bInConsoleMode = bConsoleMode;
		}

		m_iSelectedGroup = -1;
		ToggleSelectionIcons( false );

		// set the %lastinv% dialog var to our binding
		const char *key = engine->Key_LookupBinding( "lastinv" );
		if ( !key )
		{
			key = "< not bound >";
		}

		SetDialogVariable( "lastinv", key );

		HideLowerPriorityHudElementsInGroup( "mid" );
	}
	else
	{
		UnhideLowerPriorityHudElementsInGroup( "mid" );
	}

	BaseClass::SetVisible( state );
}

void CHudMenuSpyDisguise::ToggleSelectionIcons( bool bHasSelectedGroup )
{
	bool bUsingSimple = tf_simple_disguise_menu.GetBool();

	for( int i = 0; i < 3; ++i )
	{
		m_pSimpleIcons[i]->SetVisible( !bHasSelectedGroup && bUsingSimple );
		m_pSimpleLabels[i]->SetVisible( !bHasSelectedGroup && bUsingSimple );
	}
	
	if( bUsingSimple )
	{
		for( int i = 0; i < 9; ++i )
		{
			bool bVar2 = false;
			if( bHasSelectedGroup != false )
			{
				// pretty wicked
				bVar2 = (m_iSelectedGroup == -1) || (uint)((m_iSelectedGroup * -3) + i) < 3;
			}

			m_pKeyBg_Red[i]->SetVisible( bVar2 );
			m_pKeyLabel_Red[i]->SetVisible( false );
			m_pKeySimpleLabel_Red[i]->SetVisible( bVar2 );

			m_pKeyBg_Blue[i]->SetVisible( bVar2 );
			m_pKeyLabel_Blue[i]->SetVisible( false );
			m_pKeySimpleLabel_Blue[i]->SetVisible( bVar2 );
		}
	}
	else
	{
		for( int i = 0; i < 9; ++i )
		{
			m_pKeyBg_Red[i]->SetVisible( true );
			m_pKeyLabel_Red[i]->SetVisible( true );
			m_pKeySimpleLabel_Red[i]->SetVisible( false );

			m_pKeyBg_Blue[i]->SetVisible( true );
			m_pKeyLabel_Blue[i]->SetVisible( true );
			m_pKeySimpleLabel_Blue[i]->SetVisible( false );
		}
	}
}
