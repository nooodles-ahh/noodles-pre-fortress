//========= Copyright � 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "hud_numericdisplay.h"
#include <KeyValues.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui_controls/AnimationController.h>
#include "iclientmode.h"
#include "tf_shareddefs.h"
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui/ISurface.h>
#include <vgui/IImage.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/ProgressBar.h>

#include "tf_controls.h"
#include "in_buttons.h"
#include "tf_imagepanel.h"
#include "c_team.h"
#include "c_tf_player.h"
#include "ihudlcd.h"
#include "tf_hud_ammostatus.h"
#include "pf_cvars.h"
#include "tf_gamerules.h"
#include "tf_weaponbase_grenade.h"

using namespace vgui;

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


DECLARE_HUDELEMENT( CTFHudWeaponAmmo );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFHudWeaponAmmo::CTFHudWeaponAmmo( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudWeaponAmmo" ) 
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD );

	hudlcd->SetGlobalStat( "(ammo_primary)", "0" );
	hudlcd->SetGlobalStat( "(ammo_secondary)", "0" );
	hudlcd->SetGlobalStat( "(weapon_print_name)", "" );
	hudlcd->SetGlobalStat( "(weapon_name)", "" );

	m_pInClip = NULL;
	m_pInClipShadow = NULL;
	m_pInReserve = NULL;
	m_pInReserveShadow = NULL;
	m_pNoClip = NULL;
	m_pNoClipShadow = NULL;
	m_pLowAmmoImage = nullptr;

	m_nAmmo	= -1;
	m_nAmmo2 = -1;
	m_hCurrentActiveWeapon = NULL;
	m_flNextThink = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudWeaponAmmo::Reset()
{
	m_flNextThink = gpGlobals->curtime + 0.05f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudWeaponAmmo::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// load control settings...
	LoadControlSettings( "resource/UI/HudAmmoWeapons.res" );

	m_pInClip = dynamic_cast<CExLabel *>( FindChildByName( "AmmoInClip" ) );
	m_pInClipShadow = dynamic_cast<CExLabel *>( FindChildByName( "AmmoInClipShadow" ) );

	m_pInReserve = dynamic_cast<CExLabel *>( FindChildByName( "AmmoInReserve" ) );
	m_pInReserveShadow = dynamic_cast<CExLabel *>( FindChildByName( "AmmoInReserveShadow" ) );

	m_pNoClip = dynamic_cast<CExLabel *>( FindChildByName( "AmmoNoClip" ) );
	m_pNoClipShadow = dynamic_cast<CExLabel *>( FindChildByName( "AmmoNoClipShadow" ) );

	m_pLowAmmoImage = dynamic_cast<vgui::ImagePanel*>(FindChildByName( "HudWeaponLowAmmoImage" ));
	if( m_pLowAmmoImage )
	{
		m_pLowAmmoImage->GetBounds( rectAmmoBounds.x, rectAmmoBounds.y, rectAmmoBounds.width, rectAmmoBounds.height );
	}

	m_nAmmo	= -1;
	m_nAmmo2 = -1;
	m_hCurrentActiveWeapon = NULL;
	m_flNextThink = 0.0f;

	UpdateAmmoLabels( false, false, false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFHudWeaponAmmo::ShouldDraw( void )
{
	// Get the player and active weapon.
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pPlayer )
	{
		return false;
	}

	CTFWeaponBase *pWeapon = pPlayer->GetActiveTFWeapon();

	if ( !pWeapon )
	{
		return false;
	}

	if ( pWeapon->GetWeaponID() == TF_WEAPON_MEDIGUN )
	{
		return false;
	}

	return CHudElement::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudWeaponAmmo::UpdateAmmoLabels( bool bPrimary, bool bReserve, bool bNoClip )
{
	if ( m_pInClip && m_pInClipShadow )
	{
		if ( m_pInClip->IsVisible() != bPrimary )
		{
			m_pInClip->SetVisible( bPrimary );
			m_pInClipShadow->SetVisible( bPrimary );
		}
	}

	if ( m_pInReserve && m_pInReserveShadow )
	{
		if ( m_pInReserve->IsVisible() != bReserve )
		{
			m_pInReserve->SetVisible( bReserve );
			m_pInReserveShadow->SetVisible( bReserve );
		}
	}

	if ( m_pNoClip && m_pNoClipShadow )
	{
		if ( m_pNoClip->IsVisible() != bNoClip )
		{
			m_pNoClip->SetVisible( bNoClip );
			m_pNoClipShadow->SetVisible( bNoClip );
		}
	}
}

// These are either hidden or development only in Live
ConVar hud_lowammowarning_threshold( "hud_lowammowarning_threshold", "0.40", FCVAR_ARCHIVE | FCVAR_DONTRECORD, "Percentage threshold at which the low ammo warning will become visible.");
ConVar hud_lowammowarning_maxposadjust( "hud_lowammowarning_maxposadjust", "4", FCVAR_ARCHIVE | FCVAR_DONTRECORD, "Maximum pixel amount to increase the low ammo warning image." ); // 4 via guess work, maybe a bit lower?
//-----------------------------------------------------------------------------
// Purpose: Get ammo info from the weapon and update the displays.
//-----------------------------------------------------------------------------
void CTFHudWeaponAmmo::OnThink()
{
	// Get the player and active weapon.
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	C_BaseCombatWeapon *pWeapon = GetActiveWeapon();

	if ( m_flNextThink < gpGlobals->curtime )
	{
		hudlcd->SetGlobalStat( "(weapon_print_name)", pWeapon ? pWeapon->GetPrintName() : " " );
		hudlcd->SetGlobalStat( "(weapon_name)", pWeapon ? pWeapon->GetName() : " " );

		if ( !pPlayer || !pWeapon || !pWeapon->UsesPrimaryAmmo() )
		{
			hudlcd->SetGlobalStat( "(ammo_primary)", "n/a" );
			hudlcd->SetGlobalStat( "(ammo_secondary)", "n/a" );

			// turn off our ammo counts
			UpdateAmmoLabels( false, false, false );

			m_nAmmo = -1;
			m_nAmmo2 = -1;

			if( m_pLowAmmoImage )
			{
				m_pLowAmmoImage->SetBounds( rectAmmoBounds.x, rectAmmoBounds.y, rectAmmoBounds.width, rectAmmoBounds.height );
				m_pLowAmmoImage->SetVisible( false );
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "HudLowAmmoPulseStop" );
			}
		}
		else
		{
			// Get the ammo in our clip.
			int nAmmo1 = pWeapon->Clip1();
			int nAmmo2 = 0;
			// Clip ammo not used, get total ammo count.
			if ( nAmmo1 < 0 )
			{
				nAmmo1 = pPlayer->GetAmmoCount( pWeapon->GetPrimaryAmmoType() );
			}
			// Clip ammo, so the second ammo is the total ammo.
			else
			{
				nAmmo2 = pPlayer->GetAmmoCount( pWeapon->GetPrimaryAmmoType() );
			}
			
			hudlcd->SetGlobalStat( "(ammo_primary)", VarArgs( "%d", nAmmo1 ) );
			hudlcd->SetGlobalStat( "(ammo_secondary)", VarArgs( "%d", nAmmo2 ) );

			if ( m_nAmmo != nAmmo1 || m_nAmmo2 != nAmmo2 || m_hCurrentActiveWeapon.Get() != pWeapon )
			{
				m_nAmmo = nAmmo1;
				m_nAmmo2 = nAmmo2;
				m_hCurrentActiveWeapon = pWeapon;

				if ( m_hCurrentActiveWeapon.Get()->UsesClipsForAmmo1() )
				{
					UpdateAmmoLabels( true, true, false );

					SetDialogVariable( "Ammo", m_nAmmo );
					SetDialogVariable( "AmmoInReserve", m_nAmmo2 );

				}
				else
				{
					UpdateAmmoLabels( false, false, true );
					SetDialogVariable( "Ammo", m_nAmmo );
				}

				C_TFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
				int iMaxAmmo = pTFPlayer->m_Shared.MaxAmmo( pWeapon->GetPrimaryAmmoType() );
				int iLowAmmo = RoundFloatToInt( iMaxAmmo * hud_lowammowarning_threshold.GetFloat() );
				if( m_nAmmo + m_nAmmo2 < iLowAmmo )
				{
					if( m_pLowAmmoImage )
					{
						if( !m_pLowAmmoImage->IsVisible() )
							ShowLowAmmoIndicator();
					
						int iScale = RoundFloatToInt( ((iLowAmmo - (float)(m_nAmmo + m_nAmmo2)) / iLowAmmo) *
							hud_lowammowarning_maxposadjust.GetFloat() );

						// Adjust the low ammo image size 
						m_pLowAmmoImage->SetBounds( 
							rectAmmoBounds.x - iScale,
							rectAmmoBounds.y - iScale,
							rectAmmoBounds.width + (iScale * 2),
							rectAmmoBounds.height + (iScale * 2)
						);
					}
				}
				else
				{
					if( m_pLowAmmoImage )
					{
						m_pLowAmmoImage->SetBounds( rectAmmoBounds.x, rectAmmoBounds.y, rectAmmoBounds.width, rectAmmoBounds.height );
						m_pLowAmmoImage->SetVisible( false );
						g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "HudLowAmmoPulseStop" );
					}
				}
			}
		}

		m_flNextThink = gpGlobals->curtime + 0.1f;
	}
}

void CTFHudWeaponAmmo::ShowLowAmmoIndicator()
{
	if( m_pLowAmmoImage ) 
	{
		m_pLowAmmoImage->SetBounds( rectAmmoBounds.x, rectAmmoBounds.y, rectAmmoBounds.width, rectAmmoBounds.height);
		m_pLowAmmoImage->SetVisible( true );
		//(**(code **)(**(int **)(this + 0x1e4) + 0xf0))(*(int **)(this + 0x1e4), 0xff0000ff); ??????
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "HudLowAmmoPulse" );
	}
	return;
}

DECLARE_HUDELEMENT( CTFHudGrenadeAmmo );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFHudGrenadeAmmo::CTFHudGrenadeAmmo( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudGrenadeAmmo" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_PLAYERDEAD );

	m_pGrenade[0] = new CExLabel( this, "AmmoGrenades1", "" );
	m_pGrenadeShadow[0] = new CExLabel( this, "AmmoGrenades1Shadow", "" );
	m_pGrenade[1] = new CExLabel( this, "AmmoGrenades2", "" );
	m_pGrenadeShadow[1] = new CExLabel( this, "AmmoGrenades2Shadow", "" );

	m_pGrenadeBG[0] = new CTFImagePanel( this, "Grenade1BG" );
	m_pGrenadeImage[0] = new vgui::ImagePanel( this, "Grenade1Image" );
	m_pGrenadeImageShadow[0] = new vgui::ImagePanel( this, "Grenade1ImageShadow" );

	m_pGrenadeBG[1] = new CTFImagePanel( this, "Grenade2BG" );
	m_pGrenadeImage[1] = new vgui::ImagePanel( this, "Grenade2Image" );
	m_pGrenadeImageShadow[1] = new vgui::ImagePanel( this, "Grenade2ImageShadow" );

	m_flNextThink = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudGrenadeAmmo::Reset()
{
	m_flNextThink = gpGlobals->curtime + 0.05f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudGrenadeAmmo::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// load control settings...
	LoadControlSettings( "resource/UI/HudAmmoGrenades.res" );

	m_nGrenadeIDs[0] = m_nGrenadeIDs[1] = m_nGrenadeOldIDs[0] = m_nGrenadeOldIDs[1] = TF_WEAPON_NONE;
	m_nGrenadeCount[0] = m_nGrenadeCount[1] = 0;
	m_pGrenadeBG[0]->SetAlpha( 255 );
	m_pGrenadeBG[1]->SetAlpha( 255 );
	m_flNextThink = 0.0f;

	UpdateGrenadesLabels( false, false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFHudGrenadeAmmo::ShouldDraw( void )
{
	// Get the player and active weapon.
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if (!pPlayer || !pPlayer->IsAlive())
	{
		return false;
	}

	CTFWeaponBase *pWeapon = pPlayer->GetActiveTFWeapon();

	if (!pWeapon)
	{
		return false;
	}

	if( TFGameRules() && TFGameRules()->GrenadesRegenerate())
	{
		return false;
	}

	return CHudElement::ShouldDraw();
}

void CTFHudGrenadeAmmo::UpdateGrenadesLabels( bool bGrenades1, bool bGrenades2 )
{
	if( m_pGrenadeImage[0]->IsVisible() != bGrenades1 )
	{
		m_pGrenadeImage[0]->SetVisible( bGrenades1 );
		m_pGrenadeImageShadow[0]->SetVisible( bGrenades1 );
		m_pGrenade[0]->SetVisible( bGrenades1 );
		m_pGrenadeShadow[0]->SetVisible( bGrenades1 );

		if( !bGrenades1 )
		{
			m_pGrenadeBG[0]->SetAlpha( 128 );
		}
		else
		{
			m_pGrenadeBG[0]->SetAlpha( 255 );
		}
	}

	if( m_pGrenadeImage[1]->IsVisible() != bGrenades2 )
	{
		m_pGrenadeImage[1]->SetVisible( bGrenades2 );
		m_pGrenadeImageShadow[1]->SetVisible( bGrenades2 );
		m_pGrenade[1]->SetVisible( bGrenades2 );
		m_pGrenadeShadow[1]->SetVisible( bGrenades2 );

		if( !bGrenades2 )
		{
			m_pGrenadeBG[1]->SetAlpha( 128 );
		}
		else
		{
			m_pGrenadeBG[1]->SetAlpha( 255 );
		}
	}

	// Set grenade icons depending on the weaponID
	for( int i = 0; i < 2; ++i )
	{
		// if our weapon ID hasn't changed don't update
		if( m_nGrenadeIDs[i] != m_nGrenadeOldIDs[i] )
		{
			if( m_nGrenadeIDs[i] != TF_WEAPON_NONE )
			{
				// get the class name so "tf_weapon_grenade_normal"
				const char *pszWeaponName = WeaponIdToClassname( m_nGrenadeIDs[i] );
				if( pszWeaponName )
				{
					// cut off the first X many characters
					pszWeaponName += sizeof( "tf_weapon_grenade_" ) - 1;
					// Set our image
					m_pGrenadeImage[i]->SetImage( VarArgs( "../hud/grenade_icons/ico_%s", pszWeaponName ) );
					m_pGrenadeImageShadow[i]->SetImage( VarArgs( "../hud/grenade_icons/ico_%s", pszWeaponName ) );
				}
			}
			// Update the old ID to match the new
			m_nGrenadeOldIDs[i] = m_nGrenadeIDs[i];
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get ammo info from the weapon and update the displays.
//-----------------------------------------------------------------------------
void CTFHudGrenadeAmmo::OnThink()
{
	// Get the player and active weapon.
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	if (m_flNextThink < gpGlobals->curtime)
	{
		CTFPlayer* pTFPlayer = ToTFPlayer( pPlayer );
		if(pTFPlayer)
		{
			C_TFPlayerClass *pPlayerClass = pTFPlayer->GetPlayerClass();
			if( pPlayerClass )
			{
				TFPlayerClassData_t *pClassData = pPlayerClass->GetData();
				if( pClassData )
				{
					for( int i = 0; i < 2; ++i )
					{
						// Set our ID to the grenade in slot i
						m_nGrenadeIDs[i] = pClassData->m_aGrenades[i];
						m_nGrenadeCount[i] = i == 0 ?
							pTFPlayer->GetAmmoCount( TF_AMMO_GRENADES1 ) :
							pTFPlayer->GetAmmoCount( TF_AMMO_GRENADES2 );
					}
				}
			}
		}

		// Hide the grenade label and image if our ID is none
		UpdateGrenadesLabels( m_nGrenadeIDs[0] != TF_WEAPON_NONE, m_nGrenadeIDs[1] != TF_WEAPON_NONE );
		SetDialogVariable( "Grenades1", m_nGrenadeCount[0] );
		SetDialogVariable( "Grenades2", m_nGrenadeCount[1] );

		m_flNextThink = gpGlobals->curtime + 0.1f;
	}
}

DECLARE_HUDELEMENT( CTFHudGrenadeAmmoRegen );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFHudGrenadeAmmoRegen::CTFHudGrenadeAmmoRegen( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudGrenadeAmmoRegen" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_PLAYERDEAD );

	m_flNextThink = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudGrenadeAmmoRegen::Reset()
{
	m_flNextThink = gpGlobals->curtime + 0.05f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudGrenadeAmmoRegen::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// load control settings...
	LoadControlSettings( "resource/UI/HudAmmoGrenadesRegen.res" );

	m_pGrenades1Meter = dynamic_cast<ContinuousProgressBar*>(FindChildByName( "AmmoGrenades1Meter" ));
	m_pGrenades2Meter = dynamic_cast<ContinuousProgressBar*>(FindChildByName( "AmmoGrenades2Meter" ));
	m_flNextThink = 0.0f;

	UpdateGrenadeMeters( false, false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFHudGrenadeAmmoRegen::ShouldDraw( void )
{
	// Get the player and active weapon.
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if (!pPlayer || !pPlayer->IsAlive())
	{
		return false;
	}

	CTFWeaponBase *pWeapon = pPlayer->GetActiveTFWeapon();

	if (!pWeapon)
	{
		return false;
	}

	if( TFGameRules() && !TFGameRules()->GrenadesRegenerate() )
	{
		return false;
	}

	return CHudElement::ShouldDraw();
}

void CTFHudGrenadeAmmoRegen::UpdateGrenadeMeters( bool bGrenades1, bool bGrenades2 )
{
	if (m_pGrenades1Meter)
	{
		if (m_pGrenades1Meter->IsVisible() != bGrenades1)
		{
			m_pGrenades1Meter->SetVisible( bGrenades1 );
		}
	}
	if (m_pGrenades2Meter)
	{
		if (m_pGrenades2Meter->IsVisible() != bGrenades2)
		{
			m_pGrenades2Meter->SetVisible( bGrenades2 );
		}
	}
}



//-----------------------------------------------------------------------------
// Purpose: Get ammo info from the weapon and update the displays.
//-----------------------------------------------------------------------------
void CTFHudGrenadeAmmoRegen::OnThink()
{
	// Get the player and active weapon.
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	if (m_flNextThink < gpGlobals->curtime)
	{
		CTFPlayer* pTFPlayer = ToTFPlayer( pPlayer );
		
		if( pTFPlayer )
		{
			C_TFPlayerClass *pTFClass = pTFPlayer->GetPlayerClass();
			if (pTFClass)
			{
				TFPlayerClassData_t *pTFClassData = pTFClass->GetData();
				if (pTFClassData)
				{
					UpdateGrenadeMeters( pTFClassData->m_aAmmoMax[TF_AMMO_GRENADES1] > 0, 
								pTFClassData->m_aAmmoMax[TF_AMMO_GRENADES2] > 0 );
				}
			}
		}
		else
		{
			UpdateGrenadeMeters( false, false );
		}

		m_flNextThink = gpGlobals->curtime + 0.1f;
	}
}
extern ConVar pf_grenades_regenerate_time;
void CTFHudGrenadeAmmoRegen::Paint()
{
	// Draw a progress bar for time remaining
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pPlayer )
		return;

	BaseClass::Paint();
    
	//float flExpireTime = pPlayer->m_Shared.GetSmokeBombExpireTime();
	float flExpireTime = 1.0f;
	float flPercent = 1.0f;
	float flTime = pf_grenades_regenerate_time.GetFloat();

	if( TFGameRules()->GrenadesRegenerate() )
	{
		if (m_pGrenades1Meter)
		{
			flExpireTime = pPlayer->m_Shared.GetGrenadeRegenerationTime(0);
			if(flExpireTime > gpGlobals->curtime)
			{
				flPercent = (flTime - (flExpireTime - gpGlobals->curtime))/flTime;
				m_pGrenades1Meter->SetProgress( flPercent );
			}
			else
			{
				m_pGrenades1Meter->SetProgress( 1.0f );
			}
		}
		if (m_pGrenades2Meter)
		{
			flExpireTime = pPlayer->m_Shared.GetGrenadeRegenerationTime(1);
			if(flExpireTime > gpGlobals->curtime)
			{
				flPercent = (flTime - (flExpireTime - gpGlobals->curtime))/flTime;
				m_pGrenades2Meter->SetProgress( flPercent );
			}
			else
			{
				m_pGrenades2Meter->SetProgress( 1.0f );
			}
				
		}
	}
}
