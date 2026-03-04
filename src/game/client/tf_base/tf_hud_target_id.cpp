//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: HUD Target ID element
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "tf_hud_target_id.h"
#include "c_tf_playerresource.h"
#include "iclientmode.h"
#include "vgui/ILocalize.h"
#include "c_baseobject.h"
#include "c_team.h"
#include "tf_gamerules.h"
#include "tf_hud_statpanel.h"
#include "game_controls/IconPanel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DECLARE_HUDELEMENT( CMainTargetID );
DECLARE_HUDELEMENT( CSpectatorTargetID );
DECLARE_HUDELEMENT( CSecondaryTargetID );

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTargetID::CTargetID( const char *pElementName ) :
	CHudElement( pElementName ), BaseClass( NULL, pElementName )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_hFont = g_hFontTrebuchet24;
	m_flLastChangeTime = 0;
	m_iLastEntIndex = 0;

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	m_pTargetNameLabel = NULL;
	m_pTargetDataLabel = NULL;
	m_pBGPanel = NULL;
	m_pTargetHealth = new CTFSpectatorGUIHealth( this, "SpectatorGUIHealth" );
	m_bLayoutOnUpdate = false;
#ifdef PF2_CLIENT
	m_pTargetArmor = new CTFSpectatorGUIArmor( this, "SpectatorGUIArmor" );
	m_bShowArmor = false;
#endif
	m_pMoveablePanel = nullptr;
	m_pMoveablePanelBackground = nullptr;
	m_pMoveableKeyLabel = nullptr;
	m_pMoveableIcon = nullptr;

	RegisterForRenderGroup( "mid" );
	RegisterForRenderGroup( "commentary" );

	m_iRenderPriority = 5;
}

//-----------------------------------------------------------------------------
// Purpose: Setup
//-----------------------------------------------------------------------------
void CTargetID::Reset( void )
{
	m_pTargetHealth->Reset();
#ifdef PF2_CLIENT
	m_pTargetArmor->Reset();
#endif
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTargetID::ApplySchemeSettings( vgui::IScheme *scheme )
{
	LoadControlSettings( "resource/UI/TargetID.res" );

	BaseClass::ApplySchemeSettings( scheme );

	m_pTargetNameLabel = dynamic_cast<Label *>(FindChildByName("TargetNameLabel"));
	m_pTargetDataLabel = dynamic_cast<Label *>(FindChildByName("TargetDataLabel"));

	m_pBGPanel = dynamic_cast<CTFImagePanel*>(FindChildByName("TargetIDBG"));
	m_cBlueColor = scheme->GetColor( "TeamBlue", Color( 255, 64, 64, 255 ) );
	m_cRedColor = scheme->GetColor( "TeamRed", Color( 255, 64, 64, 255 ) );
	m_cSpecColor = scheme->GetColor( "TeamSpec", Color( 255, 160, 0, 255 ) );
	m_hFont = scheme->GetFont( "TargetID", true );

	m_pMoveablePanel = FindControl<vgui::EditablePanel>( "MoveableSubPanel", false );
	m_pMoveablePanelBackground = FindControl<CIconPanel>( "MoveableIconBG", true );
	m_pMoveableKeyLabel = FindControl<vgui::Label>( "MoveableKeyLabel", true );
	m_pMoveableIcon = FindControl<CIconPanel>( "MoveableIcon", true );
	m_pMoveableSymbolIcon = FindControl<vgui::ImagePanel>( "MoveableSymbolIcon", true );

	//SetPaintBackgroundEnabled( true );
	//SetBgColor( Color( 0, 0, 0, 90 ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTargetID::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	m_iRenderPriority = inResourceData->GetInt( "priority" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTargetID::GetRenderGroupPriority( void )
{
	return m_iRenderPriority;
}

//-----------------------------------------------------------------------------
// Purpose: clear out string etc between levels
//-----------------------------------------------------------------------------
void CTargetID::VidInit()
{
	CHudElement::VidInit();

	m_flLastChangeTime = 0;
	m_iLastEntIndex = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTargetID::ShouldDraw( void )
{
	if ( !CHudElement::ShouldDraw() )
		return false;

	C_TFPlayer *pLocalTFPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalTFPlayer )
		return false;

	// Get our target's ent index
	m_iTargetEntIndex = CalculateTargetIndex(pLocalTFPlayer);
	if ( !m_iTargetEntIndex )
	{
		// Check to see if we should clear our ID
		if ( m_flLastChangeTime && ( gpGlobals->curtime > m_flLastChangeTime ) )
		{
			m_flLastChangeTime = 0;
			m_iLastEntIndex = 0;
		}
		else
		{
			// Keep re-using the old one
			m_iTargetEntIndex = m_iLastEntIndex;
		}
	}
	else
	{
		m_flLastChangeTime = gpGlobals->curtime;
	}

	bool bReturn = false;
	if ( m_iTargetEntIndex )
	{
		C_BaseEntity *pEnt = cl_entitylist->GetEnt( m_iTargetEntIndex );
		if ( pEnt )
		{
			if ( IsPlayerIndex( m_iTargetEntIndex ) )
			{
				C_TFPlayer *pPlayer = static_cast<C_TFPlayer*>( pEnt );
				if ( pPlayer && !pPlayer->InSameTeam(pLocalTFPlayer) && 
					(pPlayer->m_Shared.InCond(TF_COND_SMOKE_BOMB) || pPlayer->m_Shared.InCond(TF_COND_STEALTHED)) )
					return bReturn;
					
				bool bDisguisedAsFriendly = false;
				if ( pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) && // they're disguised
					!pPlayer->m_Shared.InCond( TF_COND_DISGUISING ) && // they're not in the process of disguising
					!(pPlayer->m_Shared.InCond( TF_COND_STEALTHED ) ||
					pPlayer->m_Shared.InCond( TF_COND_SMOKE_BOMB ))) // they're not cloaked
				{
					bDisguisedAsFriendly = ( pPlayer->m_Shared.GetDisguiseTeam() == pLocalTFPlayer->GetTeamNumber() );
				}

				bReturn = ( pLocalTFPlayer->GetTeamNumber() == TEAM_SPECTATOR || 
							pLocalTFPlayer->InSameTeam( pEnt ) || 
							bDisguisedAsFriendly || 
							pLocalTFPlayer->IsPlayerClass(TF_CLASS_SPY) );
			}
			else if ( pEnt->IsBaseObject() )
			{
				C_BaseObject* pObj = dynamic_cast<C_BaseObject*>( pEnt );

				if( ( pLocalTFPlayer->InSameTeam( pObj ) || pLocalTFPlayer->IsPlayerClass( TF_CLASS_SPY ) ) )
				{
					// Don't show target id if it has no team.
					if( pObj->GetType() == OBJ_SRT && pObj->GetTeamNumber() < TF_TEAM_RED )
						bReturn = false;
					else
						bReturn = true;
				}
			}
		}
	}

	if ( bReturn )
	{
		if ( !IsVisible() || (m_iTargetEntIndex != m_iLastEntIndex) )
		{
			m_iLastEntIndex = m_iTargetEntIndex;
			m_bLayoutOnUpdate = true;
		}

		UpdateID();
	}

	return bReturn;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTargetID::PerformLayout( void )
{
	//int iXIndent = XRES(5);
	//int iXPostdent = XRES(0);
	int iWidth;

	m_pTargetNameLabel->SetVisible( true );
	m_pTargetDataLabel->SetVisible( true );

	int iTextW, iTextH;
	int iDataW, iDataH;

#ifdef PF2_CLIENT
	if ( m_bShowArmor )
	{
		iWidth = m_pTargetArmor->GetXPos() + m_pTargetArmor->GetWide();
	}
	else if( m_bShowHealth )
#endif
	{
		iWidth = m_pTargetHealth->GetXPos() + m_pTargetHealth->GetWide();
	}
	else
	{
		iWidth = XRES( 10 );
	}

	m_pTargetNameLabel->SetPos( iWidth, m_pTargetNameLabel->GetYPos() );
	m_pTargetDataLabel->SetPos( iWidth, m_pTargetDataLabel->GetYPos() );

	m_pTargetNameLabel->GetContentSize( iTextW, iTextH );
	m_pTargetDataLabel->GetContentSize( iDataW, iDataH );

	iWidth += max(iTextW,iDataW) + XRES(10);

	int iMoveableWidth = 0;
	if( m_pMoveablePanel && m_pMoveablePanel->IsVisible() && 
		m_pMoveableKeyLabel && m_pMoveablePanelBackground && m_pMoveableIcon && m_pMoveableSymbolIcon )
	{
		// set the %movekey% dialog var to our binding
		const char *key = engine->Key_LookupBinding( "+attack2" );
		if( !key )
		{
			key = "< not bound >";
		}

		// not recursive I guess?
		m_pMoveablePanel->SetDialogVariable( "movekey", key );
		m_pMoveableKeyLabel->GetContentSize( iTextW, iTextH ); // reuse these

		// awesome hardcoding
		iMoveableWidth = max( XRES( 32 ), iTextW + XRES(16) );
		m_pMoveablePanel->SetWide( iMoveableWidth );
		m_pMoveablePanel->SetPos( iWidth - XRES( 5 ), 0 );
		m_pMoveablePanelBackground->SetSize( iMoveableWidth, GetTall() );
		m_pMoveableKeyLabel->SetWide( iMoveableWidth );


		m_pMoveableIcon->SetPos( (iMoveableWidth - m_pMoveableIcon->GetWide()) * 0.5f, YRES(1) );
		m_pMoveableSymbolIcon->SetPos( (iMoveableWidth - m_pMoveableSymbolIcon->GetWide()) * 0.5f, m_pMoveableIcon->GetTall() );
	}

	SetSize( iWidth + iMoveableWidth, GetTall() );

	int iX,iY;
	GetPos( iX, iY );
	SetPos( (ScreenWidth() - (iWidth + iMoveableWidth)) * 0.5, iY );

	if ( m_pBGPanel )
	{
		m_pBGPanel->SetSize( iWidth, GetTall() );
	}
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Color CTargetID::GetColorForTargetTeam( int iTeamNumber )
{
	switch( iTeamNumber )
	{
	case TF_TEAM_BLUE:
		return m_cBlueColor;
		break;

	case TF_TEAM_RED:
		return m_cRedColor;
		break;

	default:
		return m_cSpecColor;
		break;
	}
} 

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTargetID::CalculateTargetIndex( C_TFPlayer *pLocalTFPlayer ) 
{ 
	int iIndex = pLocalTFPlayer->GetIDTarget(); 

	// If our target entity is already in our secondary ID, don't show it in primary.
	CSecondaryTargetID *pSecondaryID = GET_HUDELEMENT( CSecondaryTargetID );
	if ( pSecondaryID && pSecondaryID != this && pSecondaryID->GetTargetIndex() == iIndex )
	{
		iIndex = 0;
	}

	return iIndex;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTargetID::UpdateID( void )
{
	wchar_t sIDString[ MAX_ID_STRING ] = L"";
	wchar_t sDataString[ MAX_ID_STRING ] = L"";

	C_TFPlayer *pLocalTFPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalTFPlayer )
		return;

	// Get our target's ent index
	Assert( m_iTargetEntIndex );

	// Is this an entindex sent by the server?
	if ( m_iTargetEntIndex )
	{
		C_BaseEntity *pEnt = cl_entitylist->GetEnt( m_iTargetEntIndex );
		if ( !pEnt )
			return;

		m_bShowHealth = false;

		float flHealth = 0;
		float flMaxHealth = 1;
		int iMaxBuffedHealth = 0;
#ifdef PF2_CLIENT
		float flArmor = 0;
		float flMaxArmor = 1;
#endif
		// Some entities we always want to check, cause the text may change
		// even while we're looking at it
		// Is it a player?
		if ( IsPlayerIndex( m_iTargetEntIndex ) )
		{
			const char *printFormatString = NULL;
			wchar_t wszPlayerName[ MAX_PLAYER_NAME_LENGTH ];
			bool bDisguisedTarget = false;
			bool bDisguisedEnemy = false;
#ifdef PF2_CLIENT
			m_bShowArmor = true;
#endif
			C_TFPlayer *pPlayer = static_cast<C_TFPlayer*>( pEnt );
			if ( !pPlayer )
				return;

			if( m_pMoveablePanel )
				m_pMoveablePanel->SetVisible( false );

			C_TFPlayer *pDisguiseTarget = NULL;
			g_pVGuiLocalize->ConvertANSIToUnicode( pPlayer->GetPlayerName(), wszPlayerName, sizeof(wszPlayerName) );

			// determine if the target is a disguised spy (either friendly or enemy)
			if ( pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) && // they're disguised
				//!pPlayer->m_Shared.InCond( TF_COND_DISGUISING ) && // they're not in the process of disguising
				(!pPlayer->m_Shared.InCond( TF_COND_STEALTHED ) 
#ifdef PF2_CLIENT
				|| !pPlayer->m_Shared.InCond( TF_COND_SMOKE_BOMB ))
#endif
				) // they're not cloaked
			{
				bDisguisedTarget = true;
				pDisguiseTarget = ToTFPlayer( pPlayer->m_Shared.GetDisguiseTarget() );
			}

			if ( bDisguisedTarget )
			{
				// is the target a disguised enemy spy?
				if ( pPlayer->IsEnemyPlayer() )
				{
					if ( pDisguiseTarget )
					{
						bDisguisedEnemy = true;
						// change the player name
						g_pVGuiLocalize->ConvertANSIToUnicode( pDisguiseTarget->GetPlayerName(), wszPlayerName, sizeof(wszPlayerName) );
						// change the team  / team color
					}
				}
				else
				{
					// The target is a disguised friendly spy.  They appear to the player with no disguise.  Add the disguise
					// team & class to the target ID element.
					bool bDisguisedAsEnemy = ( pPlayer->m_Shared.GetDisguiseTeam() != pPlayer->GetTeamNumber() );
					const wchar_t *wszAlignment = g_pVGuiLocalize->Find( bDisguisedAsEnemy ? "#TF_enemy" : "#TF_friendly" );
					
					int classindex = pPlayer->m_Shared.GetDisguiseClass();
					const wchar_t *wszClassName = g_pVGuiLocalize->Find( g_aPlayerClassNames[classindex] );

					// build a string with disguise information
					g_pVGuiLocalize->ConstructString( sDataString, sizeof(sDataString), g_pVGuiLocalize->Find( "#TF_playerid_friendlyspy_disguise" ), 
						2, wszAlignment, wszClassName );
				}
			}

			if ( pPlayer->Weapon_OwnsThisID( TF_WEAPON_MEDIGUN ) )
			{
				wchar_t wszChargeLevel[ 10 ];
				_snwprintf( wszChargeLevel, ARRAYSIZE(wszChargeLevel) - 1, L"%.0f", pPlayer->MedicGetChargeLevel() * 100 );
				wszChargeLevel[ ARRAYSIZE(wszChargeLevel)-1 ] = '\0';
				g_pVGuiLocalize->ConstructString( sDataString, sizeof(sDataString), g_pVGuiLocalize->Find( "#TF_playerid_mediccharge" ), 1, wszChargeLevel );
			}

			int iTeamNum;
			int iClassNum;
			if ( bDisguisedEnemy )
			{
				iTeamNum = pDisguiseTarget->GetTeamNumber();
				iClassNum = pPlayer->m_Shared.GetDisguiseClass();
			}
			else
			{
				iTeamNum = pPlayer->GetTeamNumber();
				iClassNum = pPlayer->GetPlayerClass()->GetClassIndex();
			}

			m_pBGPanel->SetImage( m_pBGPanel->m_szTeamBG[ iTeamNum ] );
#ifdef PF2_CLIENT
			int iArmorType = GetPlayerClassData(iClassNum)->m_nArmorType;
			if( iArmorType != ARMOR_NONE )
				m_pTargetArmor->SetArmorImageTeam( iTeamNum, iArmorType);
			else
				m_bShowArmor = false;
#endif
			if ( pLocalTFPlayer->GetTeamNumber() == TEAM_SPECTATOR || pPlayer->InSameTeam( pLocalTFPlayer ) || bDisguisedEnemy || pLocalTFPlayer->IsPlayerClass(TF_CLASS_SPY) )
			{
				printFormatString = "#TF_playerid_sameteam";
				m_bShowHealth = true;
			}
			else if ( pLocalTFPlayer->m_Shared.GetState() == TF_STATE_DYING )
			{
				// We're looking at an enemy who killed us.
				printFormatString = "#TF_playerid_diffteam";
				m_bShowHealth = true;
			}			

			if ( m_bShowHealth )
			{
				C_TF_PlayerResource *tf_PR = dynamic_cast<C_TF_PlayerResource *>(g_PR);

				if ( tf_PR )
				{
					flMaxHealth = tf_PR->GetMaxHealth( m_iTargetEntIndex );
					iMaxBuffedHealth = pPlayer->m_Shared.GetMaxBuffedHealth();
#ifdef PF2_CLIENT
					flMaxArmor = pPlayer->GetPlayerClass()->GetMaxArmor();
#endif
					if ( bDisguisedEnemy )
					{
						flHealth = (float)pPlayer->m_Shared.GetDisguiseHealth();
#ifdef PF2_CLIENT
						flArmor = ( float )pPlayer->m_Shared.GetDisguiseArmor();
#endif
					}
					else
					{
						flHealth = (float)pPlayer->GetHealth();
#ifdef PF2_CLIENT
						flArmor = ( float )pPlayer->ArmorValue();
#endif
					}
					
				}
				else
				{
					m_bShowHealth = false;
				}
			}

			if ( printFormatString )
			{
				wchar_t *pszPrepend = GetPrepend();
				if ( !pszPrepend || !pszPrepend[0] )
				{
					pszPrepend = L"";
				}
				g_pVGuiLocalize->ConstructString( sIDString, sizeof(sIDString), g_pVGuiLocalize->Find(printFormatString), 2, pszPrepend, wszPlayerName );
			}
		}
		else	
		{
			// see if it is an object
			if ( pEnt->IsBaseObject() )
			{
				C_BaseObject *pObj = assert_cast<C_BaseObject *>( pEnt );
#ifdef PF2_CLIENT
				m_bShowArmor = false;
				// m_takedamage isn't currently networked
				m_bShowHealth = pObj->GetType() != OBJ_SRT /* && pObj->m_takedamage > DAMAGE_EVENTS_ONLY*/;
#endif
				pObj->GetTargetIDString( sIDString, sizeof(sIDString) );
				pObj->GetTargetIDDataString( sDataString, sizeof(sDataString) );
				if (m_pBGPanel->m_szTeamBG[pObj->GetTeamNumber()][0])
					m_pBGPanel->SetImage(m_pBGPanel->m_szTeamBG[pObj->GetTeamNumber()]);
				flHealth = pObj->GetHealth();
				flMaxHealth = pObj->GetMaxHealth();
				
				// Looking at an object we own?
				if( pObj->GetType() != OBJ_SRT && pObj->GetOwner() == pLocalTFPlayer )
				{
					bool bPrevVisiblity = m_pMoveablePanel->IsVisible();
					// do a thing?
					// also don't draw if we are dead
					// -Nbc66
					if( !pLocalTFPlayer->CanPickupBuilding( pObj ) || pLocalTFPlayer->GetObserverMode() > OBS_MODE_NONE )
					{
						m_pMoveablePanel->SetVisible( false );
					}
					else
					{
						m_pMoveablePanel->SetVisible( true );

						const char *pObjectIcon = nullptr;
						switch( pObj->GetType() )
						{
						case OBJ_DISPENSER:
							pObjectIcon = "obj_status_dispenser";
							break;
						case OBJ_TELEPORTER:
							if( pObj->GetObjectMode() == TELEPORTER_TYPE_ENTRANCE )
								pObjectIcon = "obj_status_tele_entrance";
							else
								pObjectIcon = "obj_status_tele_exit";
							break;
						case OBJ_SENTRYGUN:
							if( pObj->GetUpgradeLevel() == 3 )
								pObjectIcon = "obj_status_sentrygun_3";
							else if( pObj->GetUpgradeLevel() == 2 )
								pObjectIcon = "obj_status_sentrygun_2";
							else
								pObjectIcon = "obj_status_sentrygun_1";
							break;
						default:
							break;
						}

						Assert( pObjectIcon );
						// I should hope so
						if( pObjectIcon )
							m_pMoveableIcon->SetIcon( pObjectIcon );
					}
					
					if( bPrevVisiblity != m_pMoveablePanel->IsVisible() )
						m_bLayoutOnUpdate = true;
				}
				else
				{
					m_pMoveablePanel->SetVisible( false );
				}
			}
		}

		// Setup health icon
		if ( !pEnt->IsAlive() )
		{
			flHealth = 0;	// fixup for health being 1 when dead
		}


		m_pTargetHealth->SetHealth( flHealth, flMaxHealth, iMaxBuffedHealth );
		m_pTargetHealth->SetVisible( m_bShowHealth );
#ifdef PF2_CLIENT
		m_pTargetArmor->SetArmor( flArmor, flMaxArmor );
		m_pTargetArmor->SetVisible( m_bShowHealth && m_bShowArmor );
#endif
		int iNameW, iDataW, iIgnored;

		m_pTargetNameLabel->GetContentSize( iNameW, iIgnored );
		m_pTargetDataLabel->GetContentSize( iDataW, iIgnored );

		// Target name
		if ( sIDString[0] )
		{
			sIDString[ ARRAYSIZE(sIDString)-1 ] = '\0';
			m_pTargetNameLabel->SetVisible(true);

			// TODO: Support	if( hud_centerid.GetInt() == 0 )
			SetDialogVariable( "targetname", sIDString );
		}
		else
		{
			m_pTargetNameLabel->SetVisible(false);
			m_pTargetNameLabel->SetText("");
		}

		// Extra target data
		if ( sDataString[0] )
		{
			sDataString[ ARRAYSIZE(sDataString)-1 ] = '\0';
			m_pTargetDataLabel->SetVisible(true);
			SetDialogVariable( "targetdata", sDataString );
		}
		else
		{
			m_pTargetDataLabel->SetVisible(false);
			m_pTargetDataLabel->SetText("");
		}

		int iPostNameW, iPostDataW;
		m_pTargetNameLabel->GetContentSize( iPostNameW, iIgnored );
		m_pTargetDataLabel->GetContentSize( iPostDataW, iIgnored );

		if ( m_bLayoutOnUpdate || (iPostDataW != iDataW) || (iPostNameW != iNameW) )
		{
			InvalidateLayout( true );
			m_bLayoutOnUpdate = false;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSecondaryTargetID::CSecondaryTargetID( const char *pElementName ) : CTargetID( pElementName )
{
	m_wszPrepend[0] = '\0';

	RegisterForRenderGroup( "mid" );

	m_bWasHidingLowerElements = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSecondaryTargetID::ShouldDraw( void )
{
	bool bDraw = BaseClass::ShouldDraw();

	if ( bDraw )
	{
		if ( !m_bWasHidingLowerElements )
		{
			HideLowerPriorityHudElementsInGroup( "mid" );
			m_bWasHidingLowerElements = true;
		}
	}
	else 
	{
		if ( m_bWasHidingLowerElements )
		{
			UnhideLowerPriorityHudElementsInGroup( "mid" );
			m_bWasHidingLowerElements = false;
		}
	}

	return bDraw;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CSecondaryTargetID::CalculateTargetIndex( C_TFPlayer *pLocalTFPlayer )
{
	// If we're a medic & we're healing someone, target him.
	CBaseEntity *pHealTarget = pLocalTFPlayer->MedicGetHealTarget();
	if ( pHealTarget )
	{
		if ( pHealTarget->entindex() != m_iTargetEntIndex )
		{
			g_pVGuiLocalize->ConstructString( m_wszPrepend, sizeof(m_wszPrepend), g_pVGuiLocalize->Find("#TF_playerid_healtarget" ), 0 );
		}
		return pHealTarget->entindex();
	}

	// If we have a healer, target him.
	C_TFPlayer *pHealer;
	float flHealerChargeLevel;
	pLocalTFPlayer->GetHealer( &pHealer, &flHealerChargeLevel );
	if ( pHealer )
	{
		if ( pHealer->entindex() != m_iTargetEntIndex )
		{
			g_pVGuiLocalize->ConstructString( m_wszPrepend, sizeof(m_wszPrepend), g_pVGuiLocalize->Find("#TF_playerid_healer" ), 0 );
		}
		return pHealer->entindex();
	}

	if ( m_iTargetEntIndex )
	{
		m_wszPrepend[0] = '\0';
	}
	return 0;
}

// Separately declared versions of the hud element for alive and dead so they
// can have different positions

bool CMainTargetID::ShouldDraw( void )
{
	C_TFPlayer *pLocalTFPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalTFPlayer )
		return false;

	if ( pLocalTFPlayer->GetObserverMode() > OBS_MODE_NONE )
		return false;

	return BaseClass::ShouldDraw();
}

bool CSpectatorTargetID::ShouldDraw( void )
{
	C_TFPlayer *pLocalTFPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalTFPlayer )
		return false;

	if ( pLocalTFPlayer->GetObserverMode() <= OBS_MODE_NONE ||
		 pLocalTFPlayer->GetObserverMode() == OBS_MODE_FREEZECAM )
		return false;

	return BaseClass::ShouldDraw();
}

int	CSpectatorTargetID::CalculateTargetIndex( C_TFPlayer *pLocalTFPlayer ) 
{ 
	int iIndex = BaseClass::CalculateTargetIndex( pLocalTFPlayer );

	if ( pLocalTFPlayer->GetObserverMode() == OBS_MODE_IN_EYE && pLocalTFPlayer->GetObserverTarget() )
	{
		iIndex = pLocalTFPlayer->GetObserverTarget()->entindex();
	}

	return iIndex;
}