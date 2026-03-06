//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: HUD Target ID element
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "tf_imagepanel.h"
#include "tf_spectatorgui.h"
#include "c_tf_player.h"
#include "iclientmode.h"
#include "ienginevgui.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui_controls/EditablePanel.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CDisguiseStatus : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CDisguiseStatus, EditablePanel );

public:
	CDisguiseStatus( const char *pElementName );
	void			Init( void );
	virtual void	ApplySchemeSettings( vgui::IScheme *scheme );
	virtual void	Paint( void );
	virtual bool	ShouldDraw( void );

	void ShowAndUpdateStatus( void );

private:
	CTFImagePanel *m_StatusBG;
	Label *m_NameLabel;
	Label *m_WeaponLabel;
	CTFSpectatorGUIHealth *m_pTargetHealth;
	CTFSpectatorGUIArmor *m_pTargetArmor;

	int m_iTeam;
	int m_iClass;
	int m_iSlot;


	CPanelAnimationVar( vgui::HFont, m_hFont, "TextFont", "TargetID" );
};

DECLARE_HUDELEMENT( CDisguiseStatus );

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CDisguiseStatus::CDisguiseStatus( const char *pElementName ) :
	CHudElement( pElementName ), BaseClass( NULL, "DisguiseStatus" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pTargetHealth = new CTFSpectatorGUIHealth( this, "SpectatorGUIHealth" );
	m_pTargetArmor = new CTFSpectatorGUIArmor( this, "SpectatorGUIArmor" );

	m_iTeam = TEAM_UNASSIGNED;
	m_iClass = 0;
	m_iSlot = -1;

	SetHiddenBits( HIDEHUD_MISCSTATUS );
}

//-----------------------------------------------------------------------------
// Purpose: Setup
//-----------------------------------------------------------------------------
void CDisguiseStatus::Init( void )
{
}

void CDisguiseStatus::ApplySchemeSettings( vgui::IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );

	// load control settings...
	LoadControlSettings( "resource/UI/DisguiseStatusPanel.res" );
	
	// ???
	//m_itemModelPanel = FindControl<CEmbeddedItemModelPanel>( "itemmodelpanel", false );
	m_StatusBG = FindControl<CTFImagePanel>( "DisguiseStatusBG", false );
	m_NameLabel = FindControl<Label>( "DisguiseNameLabel", false );
	m_WeaponLabel = FindControl<Label>( "WeaponNameLabel", false );

	// reset these so we reapply dialogue vars and such
	m_iTeam = TEAM_UNASSIGNED;
	m_iClass = 0;
	m_iSlot = -1;

	SetPaintBackgroundEnabled( false );
}

extern ConVar tf_max_health_boost;
//-----------------------------------------------------------------------------
// Purpose: Draw function for the element
//-----------------------------------------------------------------------------
void CDisguiseStatus::Paint()
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if( !pLocalPlayer )
		return;

	if( pLocalPlayer->m_Shared.InCond( TF_COND_DISGUISED ) )
	{
		C_TFPlayer *pTarget = ToTFPlayer( pLocalPlayer->m_Shared.GetDisguiseTarget() );
		if( pTarget )
		{
			TFPlayerClassData_t *pData = GetPlayerClassData( pLocalPlayer->m_Shared.GetDisguiseClass() );
			if( pData )
			{
				int iHealth = pLocalPlayer->m_Shared.GetDisguiseHealth();
				int iHealthMax = pData->m_nMaxHealth;
				int iHealthMaxBuffed = iHealthMax + tf_max_health_boost.GetInt();

				m_pTargetHealth->SetHealth( iHealth, iHealthMax, iHealthMaxBuffed );

				int iArmor = pLocalPlayer->m_Shared.GetDisguiseArmor();
				int iArmorMax = pData->m_nMaxArmor;
				int iArmorType = pData->m_nArmorType;

				m_pTargetArmor->SetArmorImageTeam( m_iTeam, iArmorType );
				m_pTargetArmor->SetArmor( iArmor, iArmorMax, false );
			}
		}
	}
}

bool CDisguiseStatus::ShouldDraw( void )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if( pLocalPlayer )
	{
		if( pLocalPlayer->m_Shared.InCond( TF_COND_DISGUISED ) )
		{
			m_iTeam = pLocalPlayer->m_Shared.GetDisguiseTeam();
			ShowAndUpdateStatus();
			return true;
		}
		else
		{
			m_iTeam = TEAM_UNASSIGNED;
			m_iClass = 0;
			m_iSlot = -1;
		}
	}
	return false;
}

void CDisguiseStatus::ShowAndUpdateStatus( void )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if( pLocalPlayer )
	{
		if( m_StatusBG && m_StatusBG->m_iBGTeam != m_iTeam ) 
		{
			m_StatusBG->SetTeamBG( m_iTeam );
		}

		if( m_iClass != pLocalPlayer->m_Shared.GetDisguiseClass() )
		{
			m_iClass = pLocalPlayer->m_Shared.GetDisguiseClass();
			m_iSlot = -1; // we changed class so reset me
			C_TFPlayer *pTarget = ToTFPlayer( pLocalPlayer->m_Shared.GetDisguiseTarget() );
			if( pTarget )
			{
				const char *playerName = pTarget->GetPlayerName();
				SetDialogVariable( "disguisename", playerName );
			}
			else
			{
				SetDialogVariable( "disguisename", "" );
			}
		}

		CTFWeaponInfo *weaponInfo = pLocalPlayer->m_Shared.GetDisguiseWeaponInfo();
		if( weaponInfo )
		{
			if( m_iSlot != weaponInfo->iSlot )
			{
				m_iSlot = weaponInfo->iSlot;
				const wchar_t *pchLocalizedWeaponName = g_pVGuiLocalize->Find( weaponInfo->szPrintName );
				if( pchLocalizedWeaponName )
					SetDialogVariable( "weaponname", pchLocalizedWeaponName );
				else
					SetDialogVariable( "weaponname", weaponInfo->szPrintName );
			}
		}
		else
		{
			SetDialogVariable( "weaponname", "" );
		}
	}
}