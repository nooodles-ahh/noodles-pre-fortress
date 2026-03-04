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
#include "pf_loadoutpanel.h"
#include "tf_statsummary.h"
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

//=============================================================================
// ImageButton
//=============================================================================

DECLARE_BUILD_FACTORY( ImageButton );

ImageButton::ImageButton( vgui::Panel *parent, const char *panelName ) :
	Button( parent, panelName, "" )
{
	m_pBackdrop = new vgui::ImagePanel( this, "ImageButtonBackdrop" );
	ButtonInit();

	ListenForGameEvent( "localplayer_changeteam" );
}

void ImageButton::ButtonInit()
{
	m_szButtonTexture[0] = '\0';
	m_szButtonOverTexture[0] = '\0';
	SetPaintBackgroundEnabled( false );
	m_bUsingOverTexture = false;
	SetEnabled( true );
	SetMouseInputEnabled( true );
	m_pBackdrop->SetMouseInputEnabled( false );
	SetParentNeedsCursorMoveEvents( true );
	m_iBGTeam = TEAM_UNASSIGNED;
}

void ImageButton::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	SetCommand( inResourceData->GetString( "command", "" ) );

	Q_strncpy( m_szButtonOverTexture, inResourceData->GetString( "activeimage" ), sizeof( m_szButtonOverTexture ) );
	Q_strncpy( m_szButtonOverTextureTeam3, inResourceData->GetString( "activeimage_team3" ), sizeof( m_szButtonOverTextureTeam3 ) );
	Q_strncpy( m_szButtonTexture, inResourceData->GetString( "inactiveimage", m_szButtonOverTexture ), sizeof( m_szButtonTexture ) );
	if( m_szButtonTexture[0] != '\0' )
	{
		m_pBackdrop->SetImage( m_szButtonTexture );
	}

	C_TFPlayer *pPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
	m_iBGTeam = pPlayer ? pPlayer->GetTeamNumber() : TEAM_UNASSIGNED;
}

void ImageButton::SetButtonTexture( const char *szFilename )
{
	Q_strcpy( m_szButtonTexture, szFilename );
	if( (!IsCursorOver() || !IsEnabled()) && m_pBackdrop )
	{
		m_pBackdrop->SetImage( szFilename );
		m_pBackdrop->SetShouldScaleImage( true );
		m_bUsingOverTexture = false;
	}
}

void ImageButton::SetButtonOverTexture( const char *szFilename )
{
	Q_strcpy( m_szButtonOverTexture, szFilename );
	if( IsCursorOver() && IsEnabled() && m_pBackdrop )
	{
		m_pBackdrop->SetImage( szFilename );
		m_pBackdrop->SetShouldScaleImage( true );
		m_bUsingOverTexture = true;
	}
}

void ImageButton::SetArmed( bool state )
{
	BaseClass::SetArmed( state );
	if( state && !m_bUsingOverTexture )
	{
		if( m_pBackdrop )
		{
			if( m_szButtonOverTextureTeam3[0] != '\0' && m_iBGTeam == TF_TEAM_BLUE )
				m_pBackdrop->SetImage( m_szButtonOverTextureTeam3 );
			else if ( m_szButtonOverTexture[0] != '\0' )
				m_pBackdrop->SetImage( m_szButtonOverTexture );
		}
			
		m_bUsingOverTexture = true;
	}
	else if( !state && m_bUsingOverTexture )
	{
		if( m_pBackdrop && m_szButtonTexture[0] != '\0' )
			m_pBackdrop->SetImage( m_szButtonTexture );
		m_bUsingOverTexture = false;
	}
}

void ImageButton::OnThink()
{
	BaseClass::OnThink();

	// check our label and backdrop are the right sizes
	int w = GetWide();
	int t = GetTall();

	int bw = m_pBackdrop->GetWide();
	int bt = m_pBackdrop->GetTall();
	if( bw != w || bt != t )
	{
		m_pBackdrop->SetSize( w, t );
	}
}

void ImageButton::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	m_pBackdrop->SetShouldScaleImage( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ImageButton::FireGameEvent( IGameEvent *event )
{
	if( FStrEq( "localplayer_changeteam", event->GetName() ) )
	{
		C_TFPlayer *pPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
		m_iBGTeam = pPlayer ? pPlayer->GetTeamNumber() : TEAM_UNASSIGNED;
	}
}

//=============================================================================
// Item model panel
//=============================================================================

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CPFItemModelPanel::CPFItemModelPanel( vgui::Panel *parent, const char *name ) : BaseClass( parent, name,
	vgui::scheme()->LoadSchemeFromFile( "Resource/ClientScheme.res", "ClientScheme" ) )
{
	m_pModelPanel = new CBaseModelPanel( this, "itemmodelpanel" );
	m_pNameLabel = new CExLabel( this, "namelabel", "" );
	m_pAttribLabel = new CExLabel( this, "attriblabel", "" );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CPFItemModelPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	SetProportional( true );
	LoadControlSettings( "Resource/UI/ItemModelPanel.res" );
}

void CPFItemModelPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );
	InvalidateLayout( false, true ); // Force ApplySchemeSettings to run.
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CPFItemModelPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	int x, y, w, h;
	m_pModelPanel->GetBounds( x, y, w, h );
	int iAdjustedY = y + ((GetTall() - m_nModelTall)/2);
	m_pModelPanel->SetBounds( m_nModelX, iAdjustedY, m_nModelWidth, m_nModelTall );

	m_pNameLabel->SizeToContents();
	m_pAttribLabel->SizeToContents();

	int iNameWide, iNameTall;
	m_pNameLabel->GetContentSize( iNameWide, iNameTall );

	int iAttribWide, iAttribTall;
	m_pAttribLabel->GetContentSize( iAttribWide, iAttribTall );

	// I legitimately don't know
	iAdjustedY = abs(GetTall() - (iAttribTall + iNameTall))/2;

	m_pNameLabel->SetBounds( m_nTextX, iAdjustedY, m_nTextWide, iNameTall );
	m_pNameLabel->SetContentAlignment( vgui::Label::Alignment::a_south );

	m_pAttribLabel->SetBounds( m_nTextX, iAdjustedY + iNameTall, m_nTextWide, iAttribTall );
	m_pAttribLabel->SetContentAlignment( vgui::Label::Alignment::a_north );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CPFItemModelPanel::SetModel( const char *weaponMDL, int nSkin )
{
	m_pModelPanel->SetMDL( weaponMDL );
	m_pModelPanel->SetSkin( nSkin );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CPFItemModelPanel::SetDetails( const char *weaponName, const char *weaponDescription )
{
	// Have to do it this way since we're using dialog variables 
	const wchar_t *pchLocalizedWeaponName = g_pVGuiLocalize->Find( weaponName );
	if( pchLocalizedWeaponName )
		SetDialogVariable( "itemname", pchLocalizedWeaponName );
	else
		SetDialogVariable( "itemname", weaponName );

	const wchar_t *pchLocalizedWeaponDescription = g_pVGuiLocalize->Find( weaponDescription );
	if( pchLocalizedWeaponDescription )
		SetDialogVariable( "attriblist", pchLocalizedWeaponDescription );
	else
		SetDialogVariable( "attriblist", weaponDescription );
}

//=============================================================================
// Item selection panel
//=============================================================================

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CPFItemSelectionPanel::CPFItemSelectionPanel( vgui::Panel *parent ) : BaseClass( parent, "ItemSelectionPanel",
	vgui::scheme()->LoadSchemeFromFile( "Resource/ClientScheme.res", "ClientScheme" ) )
{
	m_iTeam = TEAM_UNASSIGNED;
	m_iClass = TF_CLASS_UNDEFINED;
	m_iSlot = -1;
	m_iItemCount = 0;
	m_iSelectedPreset = 0;

	m_pItemContainer = new EditablePanel(this, "itemcontainer");
	m_pItemContainerScroller = new ScrollableEditablePanel( this, m_pItemContainer, "itemcontainerscroller" );
	m_pSlotLabel = new CExLabel( this, "ItemSlotLabel", "" );

	InvalidateLayout( true, true );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CPFItemSelectionPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	SetProportional( true );
	LoadControlSettings( "Resource/UI/ItemSelectionPanel.res" );
}

void CPFItemSelectionPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	wchar_t *wzLocalizedClassName = g_pVGuiLocalize->Find( g_aPlayerClassNames[m_iClass] );
	SetDialogVariable( "loadoutclass", wzLocalizedClassName );

	KeyValues *pItemsKV = inResourceData->FindKey( "itemskv" );
	KeyValues *pButtonsKV = inResourceData->FindKey( "buttonskv" );
	if( pItemsKV )
	{
		// apply the keyvalues specified in the res to our panels and buttons
		FOR_EACH_VEC( m_pItemModelPanels, i )
		{
			m_pItemModelPanels[i]->ApplySettings( pItemsKV );

			if( pButtonsKV )
			{
				m_pItemEquipButtons[i]->ApplySettings(pButtonsKV);
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CPFItemSelectionPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	int y = m_nItemYDelta;
	// set the position of each of our loadout items in the scrollable panel
	FOR_EACH_VEC( m_pItemModelPanels, i )
	{
		m_pItemModelPanels[i]->SetPos( m_nItemX, y );

		// if the item we're setting the position of is our currently equipped weapon
		// show the currently equipped label and bg behind it
		if( m_iSelectedPreset == i )
		{
			EditablePanel *pBG = FindControl<EditablePanel>( "CurrentlyEquippedBackground", true );
			Label *pLabel = FindControl<Label>( "CurrentlyEquippedLabel", true );
			if( pBG && pLabel )
			{
				pBG->SetVisible( true );
				pBG->SetPos( pBG->GetXPos(), y );
				int iLabelY = (pBG->GetTall() - pLabel->GetTall()) / 2;
				pLabel->SetVisible( true );
				pLabel->SetPos( pLabel->GetXPos(), y + iLabelY );
			}
			m_pItemEquipButtons[i]->SetText( "#Keep" );
		}

		int iButtonY = (m_pItemModelPanels[i]->GetTall() - m_pItemEquipButtons[i]->GetTall()) / 2;
		m_pItemEquipButtons[i]->SetPos( m_nButtonXPos, y + iButtonY );
		y += m_pItemModelPanels[i]->GetTall() + m_nItemYDelta;
	}

	// Set the item selection title
	switch( m_iSelectedPreset )
	{
	case 0:
		m_pSlotLabel->SetText( "#ItemSel_PRIMARY" );
		break;
	case 1:
		m_pSlotLabel->SetText( "#ItemSel_SECONDARY" );
		break;
	case 2:
		m_pSlotLabel->SetText( "#ItemSel_MELEE" );
		break;
	}

	SetVisible( true );
}

//-----------------------------------------------------------------------------
// Purpose: Command handler
//-----------------------------------------------------------------------------
void CPFItemSelectionPanel::OnCommand( const char *command )
{
	if( !Q_stricmp( command, "back" ) )
	{
		SetVisible( false );
	}
	// have we selected a new weapon?
	else if( !Q_strnicmp( command, "preset", 6 ) )
	{
		// Get and update the local inventory
		KeyValues *pInventoryKeys = GetTFInventory()->GetInventory( filesystem );
		KeyValues *pClass = pInventoryKeys->FindKey( g_aPlayerClassNames_NonLocalized[m_iClass], true );
		int iPreset = Q_atoi( command + 6 );
		pClass->SetInt( GetTFInventory()->GetSlotName( m_iSlot ), iPreset );
		GetTFInventory()->SetInventory( filesystem, pInventoryKeys );

		// Are we in-game?
		C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
		if( pPlayer )
		{
			// tell the server that we've updated our weapon preset
			char szCmd[64];
			Q_snprintf( szCmd, sizeof( szCmd ), "weaponpresetnow %d %d %d", m_iClass, m_iSlot, iPreset );
			engine->ExecuteClientCmd( szCmd );
		}
		
		// tell the parent we're done. I'm sorry
		GetParent()->OnCommand( VarArgs( "slot%d", m_iSlot ) ); // bad
		SetVisible( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Actually create all the controls for the current selected slot
//-----------------------------------------------------------------------------
bool CPFItemSelectionPanel::SetTeamClassSlot( int iTeam, int iClass, int iSlot )
{
	if( iTeam != TEAM_UNASSIGNED && iTeam < LAST_SHARED_TEAM || iTeam >= TF_TEAM_COUNT )
		return false;

	if( iClass < TF_FIRST_NORMAL_CLASS || iClass > TF_LAST_PLAYABLE_CLASS )
		return false;

	m_iItemCount = GetTFInventory()->CheckValidSlot( iClass, iSlot );
	if( m_iItemCount <= 0 )
		return false;

	m_iTeam = iTeam;
	m_iClass = iClass;
	m_iSlot = iSlot;

	// remove any previous controls
	m_pItemModelPanels.PurgeAndDeleteElements();
	m_pItemEquipButtons.PurgeAndDeleteElements();

	InvalidateLayout( true, true );

	for( int i = 0; i < m_iItemCount; ++i )
	{
		PFLoadoutWeaponInfo *wep = GetTFInventory()->GetWeapon( iClass, m_iSlot, i );
		if( wep )
		{
			// create a new model panel
			CPFItemModelPanel *pItemModelPanel = new CPFItemModelPanel( m_pItemContainer, VarArgs( "itempanel%d", i ) );
			pItemModelPanel->SetModel( wep->m_szInventoryModel, iTeam - 2 );
			pItemModelPanel->SetDetails( wep->m_szItemName, wep->m_szItemTypeName );
			m_pItemModelPanels.AddToTail( pItemModelPanel );
			
			// create a new equip button
			CExButton *pItemButton = new CExButton( m_pItemContainer, VarArgs( "item%d", i ), "" );
			pItemButton->AddActionSignalTarget( this );
			pItemButton->SetCommand( VarArgs("preset%d", i) );
			m_pItemEquipButtons.AddToTail( pItemButton );
		}
	}

	// cache our currently selected slot preset
	m_iSelectedPreset = GetTFInventory()->GetWeaponPreset( filesystem, m_iClass, m_iSlot );

	InvalidateLayout( true, true );
	SetVisible( true );

	return true;
}

//=============================================================================
// Loadout panel
//=============================================================================

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CPFFullLoadoutPanel::CPFFullLoadoutPanel( vgui::Panel *parent ) : BaseClass( parent, "full_loadout",
	vgui::scheme()->LoadSchemeFromFile( "Resource/ClientScheme.res", "ClientScheme" ) )
{
	m_iClass = 0;
	m_iTeam = TF_TEAM_RED;

	m_pModelPanel = new CBaseModelPanel( this, "classmodelpanel" );
	m_pItemSelectionPanel = new CPFItemSelectionPanel( this );

	m_pBracketImage = new ImagePanel( this, "BracketImage" );
	m_pNoneAvailableLabel = new CExLabel(this, "NoneAvailableTitle", "");
	m_pNoneAvailable2Label = new CExLabel( this, "NoneAvailableTitle2", "" );
	m_pNoneAvailableReasonLabel = new CExLabel( this, "NoneAvailableReason", "" );

	for( int i = 0; i < 3; ++i )
	{
		m_pItemModelPanels[i] = new CPFItemModelPanel( this, VarArgs( "modelpanel%d", i ) );
		m_pInventoryCountLabels[i] = new CExLabel( this, VarArgs( "InventoryCount%d", i ), "" );
		m_pInventoryChangeButtons[i] = new CExButton( this, VarArgs( "ChangeButton%d", i ), "" );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CPFFullLoadoutPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	SetProportional( true );
	LoadControlSettings( "Resource/UI/FullLoadoutPanel.res" );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CPFFullLoadoutPanel::PerformLayout()
{
	if( GetVParent() )
	{
		int w, h;
		vgui::ipanel()->GetSize( GetVParent(), w, h );
		SetBounds( 0, 0, w, h );
	}

	BaseClass::PerformLayout();

	// Weapon model preview locations
	int nBaseY = m_nItemY;
	for( int i = 0; i < ARRAYSIZE( m_pItemModelPanels ); ++i )
	{
		int x, y;
		m_pItemModelPanels[i]->GetPos( x, y );
		m_pItemModelPanels[i]->SetPos( x, nBaseY );
		nBaseY += m_nItemYDelta;
	}

	// Weapon change button locations
	nBaseY = m_nButtonY;
	bool bHasExtraPresets = false;
	for( int i = 0; i < ARRAYSIZE( m_pInventoryChangeButtons ); ++i )
	{
		// Do we have more than 1 weapon in the given slot?
		int iPresets = GetTFInventory()->CheckValidSlot( m_iClass, i );
		if( iPresets > 1 )
		{
			bHasExtraPresets = true;

			// set the position of our change button
			int x, y;
			m_pInventoryChangeButtons[i]->GetPos( x, y );
			m_pInventoryChangeButtons[i]->SetPos( x, nBaseY );
			m_pInventoryChangeButtons[i]->SetVisible( true );

			// set the position of our slot count
			int w, h;
			m_pInventoryChangeButtons[i]->GetSize( w, h );
			m_pInventoryCountLabels[i]->SetPos( x, nBaseY + h );
			m_pInventoryCountLabels[i]->SetVisible( true );
			if( iPresets == 2 )
			{
				m_pInventoryCountLabels[i]->SetText( g_pVGuiLocalize->Find( "#InventoryCountOne" ) );
			}
			else
			{
				wchar_t szText[256] = L"";

				const wchar_t *pchFmt = g_pVGuiLocalize->Find( "#InventoryCountMany" );
				if( pchFmt && pchFmt[0] )
				{
					wchar_t wszPresets[4];
					_snwprintf( wszPresets, sizeof( wszPresets ) / sizeof( wchar_t ), L"%d", iPresets );
					g_pVGuiLocalize->ConstructString( szText, sizeof( szText ), pchFmt, 1, wszPresets );
					m_pInventoryCountLabels[i]->SetText( szText );
				}
			}
		}
		else
		{
			m_pInventoryCountLabels[i]->SetVisible( false );
			m_pInventoryChangeButtons[i]->SetVisible( false );
		}
		nBaseY += m_nButtonYDelta;
	}

	// Set nothing available controls on off
	m_pBracketImage->SetVisible( !bHasExtraPresets );
	m_pNoneAvailableLabel->SetVisible( !bHasExtraPresets );
	m_pNoneAvailable2Label->SetVisible( !bHasExtraPresets );
	m_pNoneAvailableReasonLabel->SetVisible( !bHasExtraPresets );
}

//-----------------------------------------------------------------------------
// Purpose: Command handler
//-----------------------------------------------------------------------------
void CPFFullLoadoutPanel::OnCommand( const char *command )
{
	if( !Q_stricmp( command, "back" ) )
	{
		m_pItemSelectionPanel->SetVisible( false );
		SetVisible( false );
	}
	// Have we clicked a change button
	else if( !Q_strnicmp( command, "change", 6 ) )
	{
		m_pItemSelectionPanel->SetTeamClassSlot( m_iTeam, m_iClass, Q_atoi( command + 6 ) );
	}
	// we have probably returned from a change button with a new slot
	else if( !Q_strnicmp( command, "slot", 4 ) )
	{
		// lazy but it works
		ShowLoadout( m_iClass, m_iTeam, Q_atoi( command + 4 ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Show loadout for our desired class. Will also handle team and slot, 
// slot specifically for if we've changed an item in a slot
//-----------------------------------------------------------------------------
void CPFFullLoadoutPanel::ShowLoadout( int iClass, int iTeam, int iSlot )
{
	m_pItemSelectionPanel->SetVisible( false );

	m_iClass = iClass;
	m_iTeam = iTeam;

	// clear out any previously merged models
	m_pModelPanel->ClearMergeMDLs();
	if( GetPlayerClassData( m_iClass ) )
	{
		m_pModelPanel->SetMDL( GetPlayerClassData( m_iClass )->GetModelName() );
		if( m_iTeam > LAST_SHARED_TEAM )
			m_pModelPanel->SetSkin( m_iTeam - 2 );
	}

	// Show the class name
	wchar_t *wzLocalizedClassName = g_pVGuiLocalize->Find( g_aPlayerClassNames[m_iClass] );
	SetDialogVariable( "loadoutclass", wzLocalizedClassName );

	// If I (you, we) ever want's to change the 4th slot increase this number
	for( int i = 0; i < 3; ++i )
	{
		int count = GetTFInventory()->CheckValidSlot( m_iClass, i );
		if( count > 0 )
		{
			int iPreset = GetTFInventory()->GetWeaponPreset( filesystem, m_iClass, i );
			PFLoadoutWeaponInfo *wep = GetTFInventory()->GetWeapon( m_iClass, i, iPreset );
			if( wep )
			{
				m_pInventoryCountLabels[i]->SetVisible( false ); // don't show this yet
				m_pItemModelPanels[i]->SetVisible( true );
				m_pItemModelPanels[i]->SetModel( wep->m_szInventoryModel, iTeam - 2 );
				m_pItemModelPanels[i]->SetDetails( wep->m_szItemName, wep->m_szItemTypeName );
			}
			else
			{
				m_pInventoryCountLabels[i]->SetVisible( false );
				m_pItemModelPanels[i]->SetVisible( false );
			}
		}
	}

	// Cyanide; hypothetically, let's say, you wanted to handle grenades, I'd recommend doing a new loop here

	// Merge a weapon with our class model
	SetMergedWeapon( iSlot );

	SetVisible( true );
	InvalidateLayout( false, false );
}

//-----------------------------------------------------------------------------
// Purpose: Merge a weapon from a slot with our class model
//-----------------------------------------------------------------------------
void CPFFullLoadoutPanel::SetMergedWeapon( int iSlot )
{
	// check there is actually a weapon to merge
	if( GetTFInventory()->CheckValidSlot( m_iClass, iSlot ) )
	{
		int iPreset = GetTFInventory()->GetWeaponPreset( filesystem, m_iClass, iSlot );
		PFLoadoutWeaponInfo *wep = GetTFInventory()->GetWeapon( m_iClass, iSlot, iPreset );
		if( wep )
		{
			m_pModelPanel->ClearMergeMDLs();
			m_pModelPanel->SetMergeMDL( wep->m_szPlayerModel, nullptr, m_iTeam - 2 );

			// does this weapon use a slot specific animation?
			if( wep->m_iAnimSlot != -1 )
				m_pModelPanel->SetModelAnim( wep->m_iAnimSlot );
			else
				m_pModelPanel->SetModelAnim( iSlot );
		}
	}
}

//=============================================================================
// Class loadout selection panel
//=============================================================================

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CPFCharInfoLoadoutSubPanel::CPFCharInfoLoadoutSubPanel( vgui::Panel *parent ) : PropertyPage( parent, "CharInfoLoadoutSubPanel" )
{
	m_classImageButtons[0] = new ImageButton( this, "scout" );
	m_classImageButtons[1] = new ImageButton( this, "soldier" );
	m_classImageButtons[2] = new ImageButton( this, "pyro" );
	m_classImageButtons[3] = new ImageButton( this, "demoman" );
	m_classImageButtons[4] = new ImageButton( this, "heavyweapons" );
	m_classImageButtons[5] = new ImageButton( this, "engineer" );
	m_classImageButtons[6] = new ImageButton( this, "medic" );
	m_classImageButtons[7] = new ImageButton( this, "sniper" );
	m_classImageButtons[8] = new ImageButton( this, "spy" );

	m_pLoadoutPanel = new CPFFullLoadoutPanel( this );

	m_pClassLabel = new CExLabel( this, "ClassLabel", "" );
	m_pItemsLabel = new CExLabel( this, "ItemsLabel", "" );

	m_pSelectLabel = new CExLabel( this, "SelectLabel", "" );
	m_pLoadoutChangesLabel = new CExLabel( this, "LoadoutChangesLabel", "" );

	m_iMouseX = m_iMouseY = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Applies scheme settings
//-----------------------------------------------------------------------------
void CPFCharInfoLoadoutSubPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	SetProportional( true );
	LoadControlSettings( "Resource/UI/CharInfoLoadoutSubPanel.res" );
}

void CPFCharInfoLoadoutSubPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );
	
	m_colItemColor = inResourceData->GetColor( "itemcountcolor" );
	m_colNoItemColor = inResourceData->GetColor( "itemcountcolor_noitems" );

}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CPFCharInfoLoadoutSubPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	// Show "loadout changes on respawn" label if we're in-game
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if( pPlayer )
	{
		m_pSelectLabel->SetPos(GetXPos(), m_iSelectLabelOnChangesY );
		m_pLoadoutChangesLabel->SetVisible( true );
	}
	else
	{
		m_pSelectLabel->SetPos( GetXPos(), m_iSelectLabelY );
		m_pLoadoutChangesLabel->SetVisible( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Command handler
//-----------------------------------------------------------------------------
void CPFCharInfoLoadoutSubPanel::OnCommand( const char *command )
{
	if( !Q_strnicmp( command, "loadout ", 8 ) )
	{
		const char *classname = command + 8;
		for( int i = 1; i < TF_CLASS_COUNT_ALL; ++i )
		{
			if( !Q_stricmp( classname, g_aPlayerClassNamesLower[i] ) )
			{
				OpenFullLoadout( i );
				break;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when we click on a class button
//-----------------------------------------------------------------------------
void CPFCharInfoLoadoutSubPanel::OpenFullLoadout( int iClass )
{
	int iTeam = 0;
	// check for our team
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if( pLocalPlayer )
	{
		iTeam = pLocalPlayer->GetTeamNumber();
		// we never specified a class like when we call open_charinfo_direct
		if( iClass == 0 )
			iClass = pLocalPlayer->GetPlayerClass()->GetClassIndex();
	}

	// Sanity check incase we somehow try to open the loadout for a non-existent class
	if( iClass > TF_CLASS_UNDEFINED && iClass < TF_LAST_PLAYABLE_CLASS  )
		m_pLoadoutPanel->ShowLoadout( iClass, iTeam );
}

// class ID's do not line up with buttons so they're remapped here
int iRemapIndexToClass[TF_LAST_NORMAL_CLASS] =
{
	TF_CLASS_SCOUT,
	TF_CLASS_SOLDIER,
	TF_CLASS_PYRO,
	TF_CLASS_DEMOMAN,
	TF_CLASS_HEAVYWEAPONS,
	TF_CLASS_ENGINEER,
	TF_CLASS_MEDIC,
	TF_CLASS_SNIPER,
	TF_CLASS_SPY,
	// TODO civ?
};

void CPFCharInfoLoadoutSubPanel::OnThink()
{
	int mouseX, mouseY;
	vgui::input()->GetCursorPos( mouseX, mouseY );

	// only updated the button layout if the mouse was moved
	if( mouseX != m_iMouseX || mouseY != m_iMouseY )
	{
		RecalculateTargetClassLayoutAtPos( mouseX, mouseY );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called whenever the mouse has been moved into order to update the 
// button scaling and show class details of the currently armed button 
//-----------------------------------------------------------------------------
void CPFCharInfoLoadoutSubPanel::RecalculateTargetClassLayoutAtPos( int x, int y )
{
	bool bArmed = false;
	int i = 0;
	for( i = 0; i < ARRAYSIZE( m_classImageButtons ); ++i )
	{
		// this is supposed to be Within() but that doesn't really work with how I've set it up
		if( m_classImageButtons[i]->IsArmed() )
		{
			bArmed = true;
			break;
		}
	}

	// Prepare to set up class name and details labels
	m_nClassDetails = bArmed ? i : -1;

	m_iMouseX = x;
	m_iMouseY = y;
	RecalculateTargetClassLayout();
}

//-----------------------------------------------------------------------------
// Purpose: horrifically unreadable subroutine to layout the class selection buttons.
// It would honestly be easier to mannually place them 
//-----------------------------------------------------------------------------
void CPFCharInfoLoadoutSubPanel::RecalculateTargetClassLayout( void )
{
	if( IsVisible() )
	{
		// Total width of the parent panel minus the total width of all the class icons divided by 2 to get the center
		int iDefXPos = (GetWide() - ((m_iClassWideMin + m_iClassXDelta) * 9)) * 0.5;
		//iDefXPos -= (GetWide() - iDefXPos) * 0.5;

		// Set the position and scale of each class button based on our cursor's distance from said buttons
		for( int i = 0; i < ARRAYSIZE( m_classImageButtons ); ++i )
		{
			int iButtonX, iButtonY, iButtonWide, iButtonTall;
			m_classImageButtons[i]->GetBounds( iButtonX, iButtonY, iButtonWide, iButtonTall );

			// Approx. center of button. Live works differently by only dealing with the horizonal distance.
			// I couldn't figure out how it actually worked so we have to use the vertical distance as well
			int xCenter = iButtonX + (iButtonWide * 0.5f);
			int yCenter = iButtonY + (iButtonTall * 0.75f);

			// distance between the cursor and the button center
			int xPosSqr = abs( (m_iMouseX - xCenter) * (m_iMouseX - xCenter) );
			int yPosSqr = abs( (m_iMouseY - yCenter) * (m_iMouseY - yCenter) );
			int iDistance = FastSqrt( xPosSqr + yPosSqr );

			// change the z pos so the largest button displays ontop
			float flScale = RemapValClamped( iDistance, m_iClassDistanceMin, m_iClassDistanceMax, 1.0f, 0.0f );
			m_classImageButtons[i]->SetZPos( 10 * flScale );

			// Calculate the button's new width and height based on the distance from the cusor and the button's center
			int iNewWide = RemapValClamped( iDistance, m_iClassDistanceMin, m_iClassDistanceMax, m_iClassWideMax, m_iClassWideMin );
			int iNewTall = RemapValClamped( iDistance, m_iClassDistanceMin, m_iClassDistanceMax, m_iClassTallMax, m_iClassTallMin );
			m_classImageButtons[i]->SetSize( iNewWide, iNewTall );

			// Adjust the X position based on how many proceeding class buttons there are
			int newX = iDefXPos + ((m_iClassWideMin + m_iClassXDelta) * i) - ((iNewWide - m_iClassWideMin)) * 0.5;
			int newY = m_iClassYPos - ((iNewTall - m_iClassTallMin) * 0.5f);
			m_classImageButtons[i]->SetPos( newX, newY );

			// It would be stupid to reformat the label everytime we moved the cursor, so we don't
			if( m_nOldClassDetails != m_nClassDetails )
			{
				// Are we no longer hovering over a button?
				if( m_nClassDetails == -1 )
				{
					m_pClassLabel->SetVisible( false );
					m_pItemsLabel->SetVisible( false );
					m_nOldClassDetails = m_nClassDetails;
				}
				// Show the class loadout details of the class button we're currently hovering over
				else if( m_nClassDetails == i )
				{
					m_pClassLabel->SetVisible( true );
					m_pClassLabel->SetPos( xCenter - (m_pClassLabel->GetWide() * 0.5), m_pClassLabel->GetYPos() );
					m_pClassLabel->SetText( g_aPlayerClassNames[iRemapIndexToClass[i]] );

					// PFTODO Cyanide; maybe resize this label if it goes off either edge
					m_pItemsLabel->SetVisible( true );
					m_pItemsLabel->SetPos( xCenter - (m_pItemsLabel->GetWide() * 0.5), m_pItemsLabel->GetYPos() );

					// Get number of non-stock weapons 
					int iExtraPresets = 0;
					for( int j = 0; j < 3; ++j )
					{
						int slotPresets = GetTFInventory()->CheckValidSlot( iRemapIndexToClass[i], j ) - 1;
						if( slotPresets > 0 )
						{
							iExtraPresets += slotPresets;
						}
					}

					// Did we have exactly 1 extra item?
					if( iExtraPresets == 1 )
					{
						m_pItemsLabel->SetText( "#ItemsFoundShortOne" );
						m_pItemsLabel->SetFgColor( m_colItemColor );
					}
					// did we have more?
					else if( iExtraPresets > 1 )
					{
						wchar_t szText[256] = L"";

						const wchar_t *pchFmt = g_pVGuiLocalize->Find( "#ItemsFoundShort" );
						if( pchFmt && pchFmt[0] )
						{
							g_pVGuiLocalize->ConstructString( szText, sizeof( szText ), pchFmt, 1, iExtraPresets );
							m_pItemsLabel->SetText( szText );
							m_pItemsLabel->SetFgColor( m_colItemColor );
						}
					}
					// There wasn't any at all
					else
					{
						m_pItemsLabel->SetText( "#NoItemsExistShort" );
						m_pItemsLabel->SetFgColor( m_colNoItemColor );

					}

					m_nOldClassDetails = m_nClassDetails;
				}
			}
		}
	}
}