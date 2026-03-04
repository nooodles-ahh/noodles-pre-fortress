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

#include "tf_hud_menu_demo_detpack.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//======================================

DECLARE_HUDELEMENT_DEPTH( CHudMenuDemoDetpack, 40 );	// in front of engy building status

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudMenuDemoDetpack::CHudMenuDemoDetpack( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudMenuDetpackBuild" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	for (int i = 0; i<SLOT_COUNT; i++)
	{
		char buf[32];

		Q_snprintf( buf, sizeof(buf), "active_item_%d", i+1 );
		m_pAvailableObjects[i] = new EditablePanel( this, buf );

		Q_snprintf( buf, sizeof(buf), "already_built_item_%d", i+1 );
		m_pAlreadyBuiltObjects[i] = new EditablePanel( this, buf );

		Q_snprintf( buf, sizeof(buf), "cant_afford_item_%d", i+1 );
		m_pCantAffordObjects[i] = new EditablePanel( this, buf );
	}

	vgui::ivgui()->AddTickSignal( GetVPanel() );

	m_pActiveSelection = NULL;

	m_iSelectedItem = -1;

	m_pBuildLabelBright = NULL;
	m_pBuildLabelDim = NULL;

	m_pDestroyLabelBright = NULL;
	m_pDestroyLabelDim = NULL;

	m_bInConsoleMode = false;

	RegisterForRenderGroup( "mid" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMenuDemoDetpack::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	LoadControlSettings( "resource/UI/detpack_menu/HudMenuDetpackBuild.res" );

	// Load the already built images, not destroyable
	m_pAlreadyBuiltObjects[0]->LoadControlSettings( "resource/UI/detpack_menu/slot_1_already_built.res" );
	m_pAlreadyBuiltObjects[1]->LoadControlSettings( "resource/UI/detpack_menu/slot_2_already_built.res" );
	m_pAlreadyBuiltObjects[2]->LoadControlSettings( "resource/UI/detpack_menu/slot_3_already_built.res" );

	m_pAvailableObjects[0]->LoadControlSettings( "resource/UI/detpack_menu/slot_1_active.res" );
	m_pAvailableObjects[1]->LoadControlSettings( "resource/UI/detpack_menu/slot_2_active.res" );
	m_pAvailableObjects[2]->LoadControlSettings( "resource/UI/detpack_menu/slot_3_active.res" );

	m_pCantAffordObjects[0]->LoadControlSettings( "resource/UI/detpack_menu/slot_1_cant_afford.res" );
	m_pCantAffordObjects[1]->LoadControlSettings( "resource/UI/detpack_menu/slot_2_cant_afford.res" );
	m_pCantAffordObjects[2]->LoadControlSettings( "resource/UI/detpack_menu/slot_3_cant_afford.res" );

	m_pActiveSelection = NULL;

	m_pBuildLabelBright = NULL;
	m_pBuildLabelDim = NULL;

	m_pDestroyLabelBright = NULL;
	m_pDestroyLabelDim = NULL;

	// Set the cost label
	for (int i = 0; i<SLOT_COUNT; i++)
	{
		int iCost = GetObjectInfo( GetBuildingIDFromSlot( i+1 ) )->m_Cost;

		m_pAvailableObjects[i]->SetDialogVariable( "metal", iCost );
		m_pAlreadyBuiltObjects[i]->SetDialogVariable( "metal", iCost );
		m_pCantAffordObjects[i]->SetDialogVariable( "metal", iCost );
	}

	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudMenuDemoDetpack::ShouldDraw( void )
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

	if ( !CHudElement::ShouldDraw() )
		return false;

	return ( pWpn->GetWeaponID() == TF_WEAPON_PDA_DEMO );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CHudMenuDemoDetpack::GetBuildingIDFromSlot( int iSlot )
{
	int iBuilding = OBJ_LAST;
	switch( iSlot )
	{
	case 1:
		iBuilding = OBJ_DETPACK;
		break;
	case 2:
		iBuilding = OBJ_DETPACK;
		break;
	case 3:
		iBuilding = OBJ_DETPACK;
		break;

	default:
		Assert( !"What slot are we asking for and why?" );
		break;
	}

	return iBuilding;
}

//-----------------------------------------------------------------------------
// Purpose: Keyboard input hook. Return 0 if handled
//-----------------------------------------------------------------------------
int	CHudMenuDemoDetpack::HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
{
	if ( !ShouldDraw() )
	{
		return 1;
	}

	if ( !down )
	{
		return 1;
	}

	bool bController = ( IsConsole() || ( keynum >= JOYSTICK_FIRST ) );

	if ( bController )
	{
		int iNewSelection = m_iSelectedItem;

		switch( keynum )
		{
		case KEY_XBUTTON_UP:
			// jump to last
			iNewSelection = 4;
			break;

		case KEY_XBUTTON_DOWN:
			// jump to first
			iNewSelection = 1;
			break;

		case KEY_XBUTTON_RIGHT:
			// move selection to the right
			iNewSelection++;
			if ( iNewSelection > 4 )
				iNewSelection = 1;
			break;

		case KEY_XBUTTON_LEFT:
			// move selection to the left
			iNewSelection--;
			if ( iNewSelection < 1 )
				iNewSelection = 4;
			break;

		case KEY_XBUTTON_A:
		case KEY_XBUTTON_RTRIGGER:
			// build selected item
			SendBuildMessage( m_iSelectedItem );
			return 0;

		case KEY_XBUTTON_Y:
		case KEY_XBUTTON_LTRIGGER:
			{
				// destroy selected item
				bool bSuccess = SendDestroyMessage( m_iSelectedItem );

				if ( bSuccess )
				{
					engine->ExecuteClientCmd( "lastinv" );
				}
			}
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
		int iSlot = 0;

		switch( keynum )
		{
		case KEY_1:
			iSlot = 1;
			break;
		case KEY_2:
			iSlot = 2;
			break;
		case KEY_3:
			iSlot = 3;
			break;
		case KEY_4:
		case KEY_5:
		case KEY_6:
		case KEY_7:
		case KEY_8:
		case KEY_9:
			// Eat these keys
			return 0;

		case KEY_0:
		case KEY_XBUTTON_B:
			// cancel, close the menu
			engine->ExecuteClientCmd( "lastinv" );
			return 0;

		default:
			return 1;	// key not handled
		}

		if ( iSlot > 0 )
		{
			// destroy selected item
			/*bool bSuccess = SendDestroyMessage(iSlot); 
			if (!bSuccess)
			{
				// Build selected item
				SendBuildMessage(iSlot);
			}*/
			SendBuildMessage( iSlot );
			return 0;
		}
		
	}

	return 1;	// key not handled
}

void CHudMenuDemoDetpack::SendBuildMessage( int iSlot )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pLocalPlayer )
		return;
	int iMode = 0;
	int iBuilding = GetBuildingIDFromSlot( iSlot );

	C_BaseObject *pObj = pLocalPlayer->GetObjectOfType( iBuilding, iMode );
	if ( pObj == NULL && !pLocalPlayer->m_Shared.GetDetpackUsed() )
	{
		char szCmd[128];
		Q_snprintf( szCmd, sizeof( szCmd ), "build %d %d", iBuilding, iSlot - 1 );
		engine->ClientCmd( szCmd );
	}
	else
	{
		pLocalPlayer->EmitSound( "Player.DenyWeaponSelection" );
	}
}

bool CHudMenuDemoDetpack::SendDestroyMessage( int iSlot )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pLocalPlayer )
		return false;

	bool bSuccess = false;

	int iBuilding = GetBuildingIDFromSlot( iSlot );
	int iMode = 0;
	C_BaseObject *pObj = pLocalPlayer->GetObjectOfType( iBuilding, iMode );

	if ( pObj != NULL )
	{
		char szCmd[128];
		Q_snprintf( szCmd, sizeof(szCmd), "destroy %d", iBuilding );
		engine->ClientCmd( szCmd );
		bSuccess = true; 
	}
	else
	{
		pLocalPlayer->EmitSound( "Player.DenyWeaponSelection" );
	}

	return bSuccess;
}

void CHudMenuDemoDetpack::OnTick( void )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pLocalPlayer )
		return;
	int iMode = 0;
	for (int i = 0; i<SLOT_COUNT; i++)
	{
		int iRemappedObjectID = GetBuildingIDFromSlot( i + 1 );

		// update this slot
		C_BaseObject *pObj = NULL;

		if ( pLocalPlayer )
		{
			pObj = pLocalPlayer->GetObjectOfType( iRemappedObjectID, iMode );
		}			

		m_pAvailableObjects[i]->SetVisible( false );
		m_pAlreadyBuiltObjects[i]->SetVisible( false );
		m_pCantAffordObjects[i]->SetVisible( false );

		// If the building is already built
		if ( pObj != NULL && !pObj->IsPlacing() )
		{
			m_pAlreadyBuiltObjects[i]->SetVisible( true );
		}
		// See if we can afford it
		else if ( pLocalPlayer->m_Shared.GetDetpackUsed() )
		{
			m_pCantAffordObjects[i]->SetVisible( true );
		}
		else
		{
			// we can buy it
			m_pAvailableObjects[i]->SetVisible( true );
		}
	}
}

void CHudMenuDemoDetpack::SetVisible( bool state )
{
	if ( state == true )
	{
		// close the weapon selection menu
		engine->ClientCmd( "cancelselect" );

		// set the %lastinv% dialog var to our binding
		const char *key = engine->Key_LookupBinding( "lastinv" );
		if ( !key )
		{
			key = "< not bound >";
		}

		SetDialogVariable( "lastinv", key );

		// Set selection to the first available building that we can build

		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

		if ( !pLocalPlayer )
			return;

		int iDefaultSlot = 1;

		// Find the first slot that represents a building that we haven't built
		int iSlot;
		for (iSlot = 1; iSlot <= SLOT_COUNT; iSlot++)
		{
			int iBuilding = GetBuildingIDFromSlot( iSlot );
			C_BaseObject *pObj = pLocalPlayer->GetObjectOfType( iBuilding, 0 );

			if ( pObj == NULL )
			{
				iDefaultSlot = iSlot;
				break;
			}
		}

		m_iSelectedItem = -1;	//force redo
		SetSelectedItem( iDefaultSlot );

		HideLowerPriorityHudElementsInGroup( "mid" );
	}
	else
	{
		UnhideLowerPriorityHudElementsInGroup( "mid" );
	}

	BaseClass::SetVisible( state );
}

void CHudMenuDemoDetpack::SetSelectedItem( int iSlot )
{
	if ( m_iSelectedItem != iSlot )
	{
		m_iSelectedItem = iSlot;

		// move the selection item to the new position
		if ( m_pActiveSelection )
		{
			// move the selection background
			int x, y;
			m_pAlreadyBuiltObjects[m_iSelectedItem-1]->GetPos( x, y );

			x -= XRES(4);
			y -= XRES(4);

			m_pActiveSelection->SetPos( x, y );

			UpdateHintLabels();			
		}
	}
}

void CHudMenuDemoDetpack::UpdateHintLabels( void )
{
	// hilight the action we can perform ( build or destroy or neither )
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( pLocalPlayer )
	{
		int iBuilding = GetBuildingIDFromSlot( m_iSelectedItem );
		C_BaseObject *pObj = pLocalPlayer->GetObjectOfType( iBuilding, 0 );

		bool bDestroyLabelBright = false;
		bool bBuildLabelBright = false;

		if ( pObj )
		{
			// hilight destroy, we have a building
			bDestroyLabelBright = true;
		}
		else if ( !pLocalPlayer->m_Shared.GetDetpackUsed() )	// I can afford it
		{
			// hilight build, we can build this
			bBuildLabelBright = true;
		}
		else
		{
			// dim both, do nothing
		}

		if ( m_pDestroyLabelBright && m_pDestroyLabelDim && m_pBuildLabelBright && m_pBuildLabelDim )
		{
			m_pDestroyLabelBright->SetVisible( bDestroyLabelBright );
			m_pDestroyLabelDim->SetVisible( !bDestroyLabelBright );

			m_pBuildLabelBright->SetVisible( bBuildLabelBright );
			m_pBuildLabelDim->SetVisible( !bBuildLabelBright );
		}
	}
}