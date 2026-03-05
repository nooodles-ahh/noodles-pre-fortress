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
#include "c_func_capture_zone.h"
#include "c_team_control_point.h"

using namespace vgui;

CUtlVector<int> g_Flags;
CUtlVector<int> g_CaptureZones;
CUtlVector<int> g_TeamControlPoints;

DECLARE_BUILD_FACTORY( CTFArrowPanel );
DECLARE_BUILD_FACTORY( CTFFlagStatus );
#ifdef PF2
DECLARE_BUILD_FACTORY( CPFArrowPanel );
#endif

extern ConVar tf_flag_caps_per_round;

void AddSubKeyNamed( KeyValues *pKeys, const char *pszName )
{
	//int __cdecl AddSubKeyNamed(KeyValues *a1, KeyValues *a2)
	KeyValues *v2 = nullptr; // edi
	v2 = new KeyValues( pszName );
	//result = KeyValues::KeyValues( v2, a2 );
	if (v2)
		pKeys->AddSubKey( v2 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFArrowPanel::CTFArrowPanel( Panel *parent, const char *name ) : vgui::Panel( parent, name )
{
	m_RedMaterial.Init( "hud/objectives_flagpanel_compass_red", TEXTURE_GROUP_VGUI );
	m_BlueMaterial.Init( "hud/objectives_flagpanel_compass_blue", TEXTURE_GROUP_VGUI );
	m_NeutralMaterial.Init( "hud/objectives_flagpanel_compass_grey", TEXTURE_GROUP_VGUI );

	m_RedMaterialNoArrow.Init( "hud/objectives_flagpanel_compass_red_noArrow", TEXTURE_GROUP_VGUI );
	m_BlueMaterialNoArrow.Init( "hud/objectives_flagpanel_compass_blue_noArrow", TEXTURE_GROUP_VGUI );
	m_NeutralMaterialNoArrow.Init( "hud/objectives_flagpanel_compass_grey_noArrow", TEXTURE_GROUP_VGUI );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFArrowPanel::GetAngleRotation( void )
{
	float flRetVal = 0.0f;

	C_TFPlayer *pPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
	C_BaseEntity *pEnt = m_hEntity.Get();

	if ( pPlayer && pEnt )
	{
		QAngle vangles;
		Vector eyeOrigin;
		float zNear, zFar, fov;

		pPlayer->CalcView( eyeOrigin, vangles, zNear, zFar, fov );

		Vector vecFlag = pEnt->WorldSpaceCenter() - eyeOrigin;
		vecFlag.z = 0;
		vecFlag.NormalizeInPlace();

		Vector forward, right, up;
		AngleVectors( vangles, &forward, &right, &up );
		forward.z = 0;
		right.z = 0;
		forward.NormalizeInPlace();
		right.NormalizeInPlace();

		float dot = DotProduct( vecFlag, forward );
		float angleBetween = acos(max(min(dot, 1.0f), -1.0f)); // clamp pls

		dot = DotProduct( vecFlag, right );

		if ( dot < 0.0f )
		{
			angleBetween *= -1;
		}

		flRetVal = RAD2DEG( angleBetween );
	}

	return flRetVal;
}

VMatrix CTFArrowPanel::GetPanelRotationMatrix(int x, int y, int wide, int tall)
{
	VMatrix panelRotation;
	panelRotation.Identity();
	MatrixRotate( panelRotation, Vector( 0, 0, 1 ), GetAngleRotation() );
	panelRotation.SetTranslation( Vector( x + wide / 2, y + tall / 2, 0 ) );
	return panelRotation;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFArrowPanel::Paint()
{
	if ( !m_hEntity.Get() )
		return;

	C_BaseEntity *pEnt = m_hEntity.Get();
	IMaterial *pMaterial = m_NeutralMaterial;

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	// figure out what material we need to use
	int iTeamNum = pEnt->GetTeamNumber();

	switch(iTeamNum)
	{
		case TF_TEAM_RED:
			pMaterial = m_RedMaterial;
			break;
		case TF_TEAM_BLUE:
			pMaterial = m_BlueMaterial;
			break;
	}

	if( pLocalPlayer )
	{
		C_BaseEntity *pTargetEnt = pLocalPlayer;
		if( pLocalPlayer->GetObserverMode() == OBS_MODE_IN_EYE )
		{
			pTargetEnt = pLocalPlayer->GetObserverTarget();
		}

		if( pTargetEnt && pTargetEnt->IsPlayer() )
		{
			// does our target have the flag and are they carrying the flag we're currently drawing?
			C_TFPlayer *pTarget = static_cast<C_TFPlayer *>(pTargetEnt);
			if( pTarget->HasTheFlag() && (pTarget->GetItem() == pEnt) )
			{
				switch( iTeamNum )
				{
				case TF_TEAM_RED:
					pMaterial = m_RedMaterialNoArrow;
					break;
				case TF_TEAM_BLUE:
					pMaterial = m_BlueMaterialNoArrow;
					break;
				default:
					pMaterial = m_NeutralMaterialNoArrow;
					break;
				}
			}
		}
	}

	BaseClass::Paint();

	int x = 0;
	int y = 0;
	ipanel()->GetAbsPos( GetVPanel(), x, y );
	int nWidth = GetWide();
	int nHeight = GetTall();

	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->MatrixMode( MATERIAL_MODEL );
	pRenderContext->PushMatrix();
	pRenderContext->LoadMatrix( GetPanelRotationMatrix( x, y, nWidth, nHeight ) );

	IMesh *pMesh = pRenderContext->GetDynamicMesh( true, NULL, NULL, pMaterial );

	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	meshBuilder.TexCoord2f( 0, 0, 0 );
	meshBuilder.Position3f( -nWidth / 2, -nHeight / 2, 0 );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.TexCoord2f( 0, 1, 0 );
	meshBuilder.Position3f( nWidth / 2, -nHeight / 2, 0 );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.TexCoord2f( 0, 1, 1 );
	meshBuilder.Position3f( nWidth / 2, nHeight / 2, 0 );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.TexCoord2f( 0, 0, 1 );
	meshBuilder.Position3f( -nWidth / 2, nHeight / 2, 0 );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();

	pMesh->Draw();
	pRenderContext->PopMatrix();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFArrowPanel::IsVisible( void )
{
	if( IsTakingAFreezecamScreenshot() )
		return false;

	return BaseClass::IsVisible();
}

#ifdef PF2
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPFArrowPanel::CPFArrowPanel( Panel *parent, const char *name ) : CTFArrowPanel(parent, name)
{
	m_RedMaterial.Init( "hud/beta_flag_hud/objectives_flagpanel_compass_red", TEXTURE_GROUP_VGUI );
	m_BlueMaterial.Init( "hud/beta_flag_hud/objectives_flagpanel_compass_blue", TEXTURE_GROUP_VGUI );
	m_NeutralMaterial.Init( "hud/beta_flag_hud/objectives_flagpanel_compass_grey", TEXTURE_GROUP_VGUI );

	m_RedMaterialNoArrow.Init( "hud/beta_flag_hud/objectives_flagpanel_compass_red_noArrow", TEXTURE_GROUP_VGUI );
	m_BlueMaterialNoArrow.Init( "hud/beta_flag_hud/objectives_flagpanel_compass_blue_noArrow", TEXTURE_GROUP_VGUI );

	m_RedMaterialBG.Init( "hud/beta_flag_hud/objectives_flagpanel_compass_red_bg", TEXTURE_GROUP_VGUI );
	m_BlueMaterialBG.Init( "hud/beta_flag_hud/objectives_flagpanel_compass_blue_bg", TEXTURE_GROUP_VGUI );
	m_NeutralMaterialBG.Init( "hud/beta_flag_hud/objectives_flagpanel_compass_grey_bg", TEXTURE_GROUP_VGUI );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPFArrowPanel::Paint()
{
	if( !m_hEntity.Get() )
		return;

	C_BaseEntity *pEnt = m_hEntity.Get();
	IMaterial *pMaterial = m_NeutralMaterial;
	IMaterial *pBGMaterial = m_NeutralMaterialBG;

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	bool bPaintArrow = true;

	// figure out what material we need to use
	if( pEnt->GetTeamNumber() == TF_TEAM_RED )
	{
		pMaterial = m_RedMaterial;
		pBGMaterial = m_RedMaterialBG;
	}
	else if( pEnt->GetTeamNumber() == TF_TEAM_BLUE )
	{
		pMaterial = m_BlueMaterial;
		pBGMaterial = m_BlueMaterialBG;
	}

	if( pLocalPlayer )
	{
		C_BaseEntity *pTargetEnt = pLocalPlayer;
		if( pLocalPlayer->GetObserverMode() == OBS_MODE_IN_EYE )
		{
			pTargetEnt = pLocalPlayer->GetObserverTarget();
		}

		if( pTargetEnt && pTargetEnt->IsPlayer() )
		{
			// does our target have the flag and are they carrying the flag we're currently drawing?
			C_TFPlayer *pTarget = static_cast<C_TFPlayer *>(pTargetEnt);
			if( pTarget->HasTheFlag() && (pTarget->GetItem() == pEnt) )
			{
				bPaintArrow = false;
			}
		}
	}

	BaseClass::BaseClass::Paint();

	if( bPaintArrow )
	{
		int x = 0;
		int y = 0;
		ipanel()->GetAbsPos( GetVPanel(), x, y );
		int nWidth = GetWide();
		int nHeight = GetTall();

		CMatRenderContextPtr pRenderContext( materials );

		pRenderContext->DrawScreenSpaceRectangle( pBGMaterial, x, y, nWidth, nHeight,
			0, 0, nWidth, nHeight, nWidth, nHeight );

		pRenderContext->MatrixMode( MATERIAL_MODEL );
		pRenderContext->PushMatrix();
		pRenderContext->LoadMatrix( GetPanelRotationMatrix( x, y, nWidth, nHeight ) );

		IMesh *pMesh = pRenderContext->GetDynamicMesh( true, NULL, NULL, pMaterial );

		CMeshBuilder meshBuilder;
		meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

		meshBuilder.TexCoord2f( 0, 0, 0 );
		meshBuilder.Position3f( -nWidth / 2, -nWidth / 2, 0 );
		meshBuilder.Color4ub( 255, 255, 255, 255 );
		meshBuilder.AdvanceVertex();

		meshBuilder.TexCoord2f( 0, 1, 0 );
		meshBuilder.Position3f( nWidth / 2, -nWidth / 2, 0 );
		meshBuilder.Color4ub( 255, 255, 255, 255 );
		meshBuilder.AdvanceVertex();

		meshBuilder.TexCoord2f( 0, 1, 1 );
		meshBuilder.Position3f( nWidth / 2, nWidth / 2, 0 );
		meshBuilder.Color4ub( 255, 255, 255, 255 );
		meshBuilder.AdvanceVertex();

		meshBuilder.TexCoord2f( 0, 0, 1 );
		meshBuilder.Position3f( -nWidth / 2, nWidth / 2, 0 );
		meshBuilder.Color4ub( 255, 255, 255, 255 );
		meshBuilder.AdvanceVertex();

		meshBuilder.End();

		pMesh->Draw();
		pRenderContext->PopMatrix();
	}
}

VMatrix CPFArrowPanel::GetPanelRotationMatrix( int x, int y, int wide, int tall )
{
	VMatrix panelRotation;
	panelRotation.Identity();
	MatrixBuildOrtho( panelRotation, -1, -1, 1, 6, 1, 0 );
	MatrixRotate( panelRotation, Vector( 0, 0, 1 ), GetAngleRotation() );
	panelRotation.SetTranslation( Vector( x + wide / 2, y + tall / 2, 0 ) );
	return panelRotation;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFFlagStatus::CTFFlagStatus( Panel *parent, const char *name ) : EditablePanel( parent, name )
{
	m_pArrow = NULL;
	m_pStatusIcon = NULL;
	m_pBriefcase = NULL;
	m_hEntity = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlagStatus::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// load control settings...
	LoadControlSettings( "resource/UI/FlagStatus.res" );

	m_pArrow = dynamic_cast<CTFArrowPanel *>( FindChildByName( "Arrow" ) );
	m_pStatusIcon = dynamic_cast<CTFImagePanel *>( FindChildByName( "StatusIcon" ) );
	m_pBriefcase = dynamic_cast<CTFImagePanel *>( FindChildByName( "Briefcase" ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFlagStatus::IsVisible( void )
{
	if( IsTakingAFreezecamScreenshot() )
		return false;

	return BaseClass::IsVisible();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlagStatus::UpdateStatus( void )
{
	if ( m_hEntity.Get() )
	{
		CCaptureFlag *pFlag = dynamic_cast<CCaptureFlag *>( m_hEntity.Get() );
	
		if ( pFlag )
		{
			const char *pszImage = "../hud/objectives_flagpanel_ico_flag_home";

			if ( pFlag->IsDropped() )
			{
				pszImage = "../hud/objectives_flagpanel_ico_flag_dropped";
			}
			else if ( pFlag->IsStolen() )
			{
				pszImage = "../hud/objectives_flagpanel_ico_flag_moving";
			}

			if ( m_pStatusIcon )
			{
				m_pStatusIcon->SetImage( pszImage );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFHudFlagObjectives::CTFHudFlagObjectives( Panel *parent, const char *name ) : EditablePanel( parent, name )
{
	m_pCarriedImage = NULL;
	m_pPlayingTo = NULL;
	m_bFlagAnimationPlayed = false;
	m_bCarryingFlag = false;
	m_pSpecCarriedImage = NULL;
	m_bHybridMode = false;

	// I hate nullptr checks. When is a player not going to have these?
	m_pCarriedImage = new CTFImagePanel(this, "CarriedImage");
	m_pPlayingTo = new CExLabel(this, "PlayingTo", "");
	m_pPlayingToBG = new CTFImagePanel(this, "PlayingToBG");

	//m_pRedFlag = new CTFArrowPanel(this, "RedFlag");
	//m_pBlueFlag = new CTFArrowPanel(this, "BlueFlag");

	// PFTODO Cyanide; do nullptr checks on this variables now lmao
	m_pRedFlag = nullptr;
	m_pBlueFlag = nullptr;
	m_pCapturePoint = nullptr;

	m_pSpecCarriedImage = new ImagePanel(this, "SpecCarriedImage" );

	// outline is always on, so we need to init the alpha to 0
	vgui::ivgui()->AddTickSignal( GetVPanel(), 100 );
	ListenForGameEvent( "flagstatus_update" );
	ListenForGameEvent( "teamplay_point_captured" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFHudFlagObjectives::IsVisible( void )
{
	if( IsTakingAFreezecamScreenshot() )
		return false;

	return BaseClass::IsVisible();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudFlagObjectives::ApplySchemeSettings( IScheme *pScheme )
{
	KeyValues *pConditions = nullptr;

	if (m_bHybridMode)
	{
		pConditions = new KeyValues( "conditions" );
		AddSubKeyNamed( pConditions, "if_hybrid" );
	}

	LoadControlSettings( "resource/UI/HudObjectiveFlagPanel.res", NULL, NULL, pConditions );

	if (pConditions)
	{
		pConditions->deleteThis();
	}

	BaseClass::ApplySchemeSettings( pScheme );

	m_pRedFlag = FindControl< CTFArrowPanel >( "RedFlag" );
	m_pBlueFlag = FindControl< CTFArrowPanel >( "BlueFlag" );
	m_pCapturePoint = FindControl< CTFArrowPanel >( "CaptureFlag" );

	// outline is always on, so we need to init the alpha to 0
	CTFImagePanel *pOutline = dynamic_cast<CTFImagePanel *>( FindChildByName( "OutlineImage" ) );
	if ( pOutline )
	{
		pOutline->SetAlpha( 0 );
	}

	m_bFlagAnimationPlayed = false;

	UpdateStatus();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudFlagObjectives::Reset()
{
	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "FlagOutlineHide" );
	UpdateStatus();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudFlagObjectives::SetPlayingToLabelVisible( bool bVisible )
{
	if ( m_pPlayingTo->IsVisible() != bVisible )
	{
		m_pPlayingTo->SetVisible( bVisible );
	}

	if ( m_pPlayingToBG->IsVisible() != bVisible )
	{
		m_pPlayingToBG->SetVisible( bVisible );
	}
}

void CTFHudFlagObjectives::CheckCapturePoints( C_TFPlayer *pLocalPlayer )
{
	if( !pLocalPlayer )
		return;

	bool bFoundZone = false;
	// go through all the capture zones and find ours
	for( int i = 0; i < g_CaptureZones.Count(); i++ )
	{
		C_CaptureZone *pZone = dynamic_cast< C_CaptureZone* >( ClientEntityList().GetEnt( g_CaptureZones[ i ] ) );

		if( pZone )
		{
			if( pZone->GetTeamNumber() == pLocalPlayer->GetTeamNumber() && !pZone->IsDisabled() )
			{
				m_pCapturePoint->SetEntity( pZone );
				bFoundZone = true;
			}
		}
	}

	if( bFoundZone )
		return;
	
	C_TeamControlPoint *currentpoint = dynamic_cast< C_TeamControlPoint * >( m_pCapturePoint->GetEntity().Get() );
	if( !currentpoint || (currentpoint && currentpoint->GetOwningTeam() == pLocalPlayer->GetTeamNumber()) )
	{
		m_pCapturePoint->SetEntity( nullptr );
		for( int i = 0; i < g_TeamControlPoints.Count(); i++ )
		{
			C_TeamControlPoint *pZone = dynamic_cast< C_TeamControlPoint* >( ClientEntityList().GetEnt( g_TeamControlPoints[ i ] ) );

			if( pZone )
			{
				if( pZone->IsActive() && pZone->GetOwningTeam() != pLocalPlayer->GetTeamNumber() )
				{
					C_TeamControlPoint *point = dynamic_cast< C_TeamControlPoint * >( m_pCapturePoint->GetEntity().Get() );
					if( point )
					{
						if( point->GetPointIndex() > pZone->GetPointIndex() )
							m_pCapturePoint->SetEntity( pZone );
					}
					else
						m_pCapturePoint->SetEntity( pZone );
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudFlagObjectives::OnTick()
{
	m_pRedFlag->SetEntity(nullptr);
	m_pBlueFlag->SetEntity(nullptr);

	// sanity check the capture areas
	if( m_bCarryingFlag )
	{
		C_TFPlayer *pLocalPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
		CheckCapturePoints( pLocalPlayer );
	}
	else
		m_pCapturePoint->SetEntity(nullptr);

	// iterate through the flags to set their position in our HUD
	for ( int i = 0; i < g_Flags.Count(); i++ )
	{
		CCaptureFlag *pFlag = dynamic_cast< CCaptureFlag* >( ClientEntityList().GetEnt( g_Flags[i] ) );

		if ( pFlag )
		{
			if ( pFlag->IsDisabled() )
				continue;

			if ( !m_bCarryingFlag && pFlag->GetTeamNumber() == TF_TEAM_RED )
			{
				m_pRedFlag->SetEntity( pFlag );
				if( !m_pRedFlag->IsVisible() )
					m_pRedFlag->SetVisible( true );
			}
			else if ( !m_bCarryingFlag && pFlag->GetTeamNumber() == TF_TEAM_BLUE )
			{
				m_pBlueFlag->SetEntity( pFlag );
				if( !m_pBlueFlag->IsVisible() )
					m_pBlueFlag->SetVisible( true );
			}
			else if ( !m_bCarryingFlag && pFlag->GetTeamNumber() == TEAM_UNASSIGNED )
			{
				m_pCapturePoint->SetEntity(pFlag);

				if(!m_pCapturePoint->IsVisible())
					m_pCapturePoint->SetVisible(true);
				
				if (m_pBlueFlag->IsVisible())
					m_pBlueFlag->SetVisible(false);

				if (m_pRedFlag->IsVisible())
					m_pRedFlag->SetVisible(false);
			}
		}
		else
		{
			// this isn't a valid index for a flag
			g_Flags.Remove( i );
		}
	}

	if( !m_pRedFlag->GetEntity() && m_pRedFlag->IsVisible() )
		m_pRedFlag->SetVisible( false );

	if( !m_pBlueFlag->GetEntity() && m_pBlueFlag->IsVisible() )
		m_pBlueFlag->SetVisible( false );

	// are we playing captures for rounds?
	if ( tf_flag_caps_per_round.GetInt() > 0 )
	{
		C_TFTeam *pTeam = GetGlobalTFTeam( TF_TEAM_BLUE );
		if ( pTeam )
		{
			SetDialogVariable( "bluescore", pTeam->GetFlagCaptures() );
		}

		pTeam = GetGlobalTFTeam( TF_TEAM_RED );
		if ( pTeam )
		{
			SetDialogVariable( "redscore", pTeam->GetFlagCaptures() );
		}

		SetPlayingToLabelVisible( true );
		SetDialogVariable( "rounds", tf_flag_caps_per_round.GetInt() );
	}
	else // we're just playing straight score
	{
		C_TFTeam *pTeam = GetGlobalTFTeam( TF_TEAM_BLUE );
		if ( pTeam )
		{
			SetDialogVariable( "bluescore", pTeam->Get_Score() );
		}

		pTeam = GetGlobalTFTeam( TF_TEAM_RED );
		if ( pTeam )
		{
			SetDialogVariable( "redscore", pTeam->Get_Score() );
		}

		SetPlayingToLabelVisible( false );
	}

	// check the local player to see if they're spectating, OBS_MODE_IN_EYE, and the target entity is carrying the flag
	bool bSpecCarriedImage = false;
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer && ( pPlayer->GetObserverMode() == OBS_MODE_IN_EYE ) )
	{
		// does our target have the flag?
		C_BaseEntity *pEnt = pPlayer->GetObserverTarget();
		if ( pEnt && pEnt->IsPlayer() )
		{
			C_TFPlayer *pTarget = static_cast< C_TFPlayer* >( pEnt );
			if ( pTarget->HasTheFlag() )
			{
				bSpecCarriedImage = true;
				if ( pTarget->GetTeamNumber() == TF_TEAM_RED )
				{
					m_pSpecCarriedImage->SetImage( "../hud/objectives_flagpanel_carried_blue" );
				}
				else
				{
					m_pSpecCarriedImage->SetImage( "../hud/objectives_flagpanel_carried_red" );
				}
			}
		}
	}

	if ( bSpecCarriedImage )
	{
		if ( m_pSpecCarriedImage->IsVisible() )
		{
			m_pSpecCarriedImage->SetVisible( true );
		}
	}
	else
	{
		if ( m_pSpecCarriedImage->IsVisible() )
		{
			m_pSpecCarriedImage->SetVisible( false );
		}
	}

	if (pPlayer && m_bHybridMode != TFGameRules()->IsCTFCPHybrid())
	{
		m_bHybridMode = TFGameRules()->IsCTFCPHybrid();
		InvalidateLayout( false, true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudFlagObjectives::UpdateStatus( void )
{
	C_TFPlayer *pLocalPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );

	// are we carrying a flag?
	CCaptureFlag *pPlayerFlag = NULL;
	if ( pLocalPlayer && pLocalPlayer->HasItem() && ( pLocalPlayer->GetItem()->GetItemID() == TF_ITEM_CAPTURE_FLAG ) )
	{
		pPlayerFlag = dynamic_cast<CCaptureFlag*>( pLocalPlayer->GetItem() );
	}

	if ( pPlayerFlag )
	{
		m_bCarryingFlag = true;
		SetCarriedFlag( pPlayerFlag );

		// make sure the panels are on, set the initial alpha values, 
		// set the color of the flag we're carrying, and start the animations
		if ( !m_bFlagAnimationPlayed )
		{
			m_bFlagAnimationPlayed = true;

			m_pBlueFlag->SetVisible( false );
			m_pRedFlag->SetVisible( false );
			m_pCarriedImage->SetVisible( true );

			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "FlagOutline" );

			m_pCapturePoint->SetVisible( true );

			if ( pLocalPlayer )
			{
				CheckCapturePoints( pLocalPlayer );
			}
		}
	}
	else
	{
		// were we carrying the flag?
		if ( m_bCarryingFlag )
		{
			m_bCarryingFlag = false;
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "FlagOutline" );
			SetCarriedFlag( nullptr );
		}

		m_bFlagAnimationPlayed = false;

		if ( m_pCarriedImage->IsVisible() )
		{
			m_pCarriedImage->SetVisible( false );
		}

		if ( m_pCapturePoint->IsVisible())
		{
			m_pCapturePoint->SetVisible( false );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudFlagObjectives::FireGameEvent( IGameEvent *event )
{
	const char *eventName = event->GetName();

	if( !Q_strcmp( eventName, "flagstatus_update" ) || !Q_strcmp( eventName, "teamplay_point_captured" ) )
	{
		UpdateStatus();
	}
}
