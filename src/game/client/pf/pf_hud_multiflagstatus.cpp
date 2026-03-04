//========= Copyright � 1996-2007, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "hudelement.h"
#include "iclientmode.h"
#include <KeyValues.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui_controls/AnimationController.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui/IVGui.h>
#include <vgui/ISurface.h>
#include <vgui/IImage.h>
#include <vgui_controls/Label.h>

#include "c_playerresource.h"
#include "teamplay_round_timer.h"
#include "utlvector.h"
#include "entity_capture_flag.h"
#include "c_tf_player.h"
#include "c_team.h"
#include "c_tf_team.h"
#include "c_team_objectiveresource.h"
#include "tf_hud_objectivestatus.h"
#include "tf_spectatorgui.h"
#include "teamplayroundbased_gamerules.h"
#include "tf_gamerules.h"
#include "tf_hud_freezepanel.h"

using namespace vgui;

extern CUtlVector<int> g_Flags;
extern CUtlVector<int> g_CaptureZones;

extern ConVar tf_flag_caps_per_round;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPFHudMultiFlagObjectives::CPFHudMultiFlagObjectives( Panel *parent, const char *name ) : EditablePanel( parent, name )
{
	m_pCarriedImage = NULL;
	m_bFlagAnimationPlayed = false;
	m_bCarryingFlag = false;
	m_pSpecCarriedImage = NULL;
	
	m_pFlagStatuses.SetSize( 0 );

	vgui::ivgui()->AddTickSignal( GetVPanel() );

	ListenForGameEvent( "flagstatus_update" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CPFHudMultiFlagObjectives::IsVisible( void )
{
	if( IsTakingAFreezecamScreenshot() )
		return false;

	return BaseClass::IsVisible();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPFHudMultiFlagObjectives::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// load control settings...
	LoadControlSettings( "resource/UI/HudMultiFlagPanel.res" );

	m_pSpecCarriedImage = dynamic_cast<ImagePanel *>(FindChildByName( "SpecCarriedImage" ));
	m_pCarriedImage = dynamic_cast<CTFImagePanel *>(FindChildByName( "CarriedImage" ));

	// outline is always on, so we need to init the alpha to 0
	CTFImagePanel *pOutline = dynamic_cast<CTFImagePanel *>(FindChildByName( "OutlineImage" ));
	if( pOutline )
	{
		pOutline->SetAlpha( 0 );
	}

	UpdateStatus();
}

void CPFHudMultiFlagObjectives::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CPFHudMultiFlagObjectives::PerformLayout()
{
	BaseClass::PerformLayout();

	int nStartingXPos = (GetWide() - (m_pFlagStatuses.Count() * (m_nStatusWide + m_nStatusXDelta))) * 0.5;
	FOR_EACH_VEC( m_pFlagStatuses, i )
	{
		m_pFlagStatuses[i]->SetPos( nStartingXPos, m_nStatusYPos );
		nStartingXPos += m_nStatusWide + m_nStatusXDelta;
	}
}



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPFHudMultiFlagObjectives::Reset()
{
	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "FlagOutlineHide" );

	m_pFlagStatuses.PurgeAndDeleteElements();

	if( m_pCarriedImage && m_pCarriedImage->IsVisible() )
	{
		m_pCarriedImage->SetVisible( false );
	}

	if( m_pSpecCarriedImage && m_pSpecCarriedImage->IsVisible() )
	{
		m_pSpecCarriedImage->SetVisible( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPFHudMultiFlagObjectives::OnTick()
{
	if( m_pFlagStatuses.Size() != g_Flags.Size() )
	{
		m_pFlagStatuses.PurgeAndDeleteElements();
		for( int i = 0; i < g_Flags.Size(); ++i )
		{
			CCaptureFlag *pFlag = dynamic_cast<CCaptureFlag *>(ClientEntityList().GetEnt( g_Flags[i] ));
			if( !pFlag )
			{
				g_Flags.Remove( i-- );
				continue;
			}

			CTFFlagStatus *status = new CTFFlagStatus( this, VarArgs( "flagstatus%d", i ) );
			status->SetSize( m_nStatusWide, m_nStatusTall );
			status->InvalidateLayout( false, true );
			status->SetEntity( pFlag );
			m_pFlagStatuses.AddToTail( status );
		}

		InvalidateLayout( true, false );
		return;
	}

	// iterate through the flags to set their position in our HUD
	for( int i = 0; i < g_Flags.Count(); i++ )
	{
		CCaptureFlag *pFlag = dynamic_cast<CCaptureFlag *>(ClientEntityList().GetEnt( g_Flags[i] ));

		if( pFlag )
		{
			if( !pFlag->IsDisabled() )
			{
				if( m_pFlagStatuses[i] )
				{
					m_pFlagStatuses[i]->SetEntity( pFlag );
				}
			}
		}
		else
		{
			// this isn't a valid index for a flag
			g_Flags.Remove( i );
		}
	}


	// check the local player to see if they're spectating, OBS_MODE_IN_EYE, and the target entity is carrying the flag
	bool bSpecCarriedImage = false;
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if( pPlayer && (pPlayer->GetObserverMode() == OBS_MODE_IN_EYE) )
	{
		// does our target have the flag?
		C_BaseEntity *pEnt = pPlayer->GetObserverTarget();
		if( pEnt && pEnt->IsPlayer() )
		{
			C_TFPlayer *pTarget = static_cast<C_TFPlayer *>(pEnt);
			if( pTarget->HasTheFlag() )
			{
				bSpecCarriedImage = true;
				if( pTarget->GetTeamNumber() == TF_TEAM_RED )
				{
					if( m_pSpecCarriedImage )
					{
						m_pSpecCarriedImage->SetImage( "../hud/objectives_flagpanel_carried_blue" );
					}
				}
				else
				{
					if( m_pSpecCarriedImage )
					{
						m_pSpecCarriedImage->SetImage( "../hud/objectives_flagpanel_carried_red" );
					}
				}
			}
		}
	}

	if( bSpecCarriedImage )
	{
		if( m_pSpecCarriedImage && !m_pSpecCarriedImage->IsVisible() )
		{
			m_pSpecCarriedImage->SetVisible( true );
		}
	}
	else
	{
		if( m_pSpecCarriedImage && m_pSpecCarriedImage->IsVisible() )
		{
			m_pSpecCarriedImage->SetVisible( false );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPFHudMultiFlagObjectives::UpdateStatus( void )
{
	C_TFPlayer *pLocalPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );

	// are we carrying a flag?
	CCaptureFlag *pPlayerFlag = NULL;
	if( pLocalPlayer && pLocalPlayer->HasItem() && (pLocalPlayer->GetItem()->GetItemID() == TF_ITEM_CAPTURE_FLAG) )
	{
		pPlayerFlag = dynamic_cast<CCaptureFlag *>(pLocalPlayer->GetItem());
	}

	if( pPlayerFlag )
	{
		m_bCarryingFlag = true;

		// make sure the panels are on, set the initial alpha values, 
		// set the color of the flag we're carrying, and start the animations
		if( m_pCarriedImage && !m_bFlagAnimationPlayed )
		{
			m_bFlagAnimationPlayed = true;
			
			if( !m_pCarriedImage->IsVisible() )
			{
				m_pCarriedImage->SetVisible( true );
			}

			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "FlagOutline" );

		}
	}
	else
	{
		// were we carrying the flag?
		if( m_bCarryingFlag )
		{
			m_bCarryingFlag = false;
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "FlagOutline" );
		}

		m_bFlagAnimationPlayed = false;

		if( m_pCarriedImage && m_pCarriedImage->IsVisible() )
		{
			m_pCarriedImage->SetVisible( false );
		}
	}

	for( int i = 0; i < m_pFlagStatuses.Count(); i++ )
	{
		if( m_pFlagStatuses[i] )
		{
			m_pFlagStatuses[i]->UpdateStatus();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPFHudMultiFlagObjectives::FireGameEvent( IGameEvent *event )
{
	const char *eventName = event->GetName();

	if( !Q_strcmp( eventName, "flagstatus_update" ) )
	{
		UpdateStatus();
	}
}