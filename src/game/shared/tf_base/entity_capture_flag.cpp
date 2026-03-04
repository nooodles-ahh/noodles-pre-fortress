//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: CTF Flag.
//
//=============================================================================//
#include "cbase.h"
#include "entity_capture_flag.h"
#include "tf_gamerules.h"
#include "tf_shareddefs.h"
#ifdef PF2
#include "pf_cvars.h"
#endif

#ifdef CLIENT_DLL
#include <vgui_controls/Panel.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui/IScheme.h>
#include "hudelement.h"
#include "iclientmode.h"
#include "hud_numericdisplay.h"
#include "tf_imagepanel.h"
#include "c_tf_player.h"
#include "c_tf_team.h"
#include "tf_hud_objectivestatus.h"
#include "view.h"

extern CUtlVector<int>	g_Flags;

ConVar cl_flag_return_size( "cl_flag_return_size", "20", FCVAR_CHEAT );

#else
#include "tf_player.h"
#include "tf_team.h"
#include "tf_objective_resource.h"
#include "tf_gamestats.h"
#include "func_respawnroom.h"
#include "datacache/imdlcache.h"
#ifdef PF2_DLL
#include "func_respawnflag.h"
#endif

extern ConVar tf_flag_caps_per_round;

ConVar cl_flag_return_height( "cl_flag_return_height", "82", FCVAR_CHEAT );
ConVar pf_flag_allow_return_on_touch( "pf_flag_allow_return_on_touch", "0", FCVAR_SERVER_CAN_EXECUTE, "Allow the opposing team to reset the flag upon touch." );

#endif

#ifdef CLIENT_DLL

static void RecvProxy_IsDisabled( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	CCaptureFlag *pFlag = (CCaptureFlag *) pStruct;
	bool bIsDisabled = ( pData->m_Value.m_Int > 0 );

	if ( pFlag )
	{
		pFlag->SetDisabled( bIsDisabled ); 
	}
}

#endif

//=============================================================================
//
// CTF Flag tables.
//

IMPLEMENT_NETWORKCLASS_ALIASED( CaptureFlag, DT_CaptureFlag )

BEGIN_NETWORK_TABLE( CCaptureFlag, DT_CaptureFlag )

#ifdef GAME_DLL
	SendPropBool( SENDINFO( m_bDisabled ) ),
	SendPropInt( SENDINFO( m_nGameType ), 5, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nFlagStatus ), 3, SPROP_UNSIGNED ),
	SendPropTime( SENDINFO( m_flResetTime ) ),
	SendPropTime( SENDINFO( m_flNeutralTime ) ),
	SendPropTime( SENDINFO( m_flMaxResetTime ) ),
	SendPropEHandle( SENDINFO( m_hPrevOwner ) ),
#ifdef PF2_DLL
	SendPropBool( SENDINFO( m_bSlowPlayer ) ),
#endif
#else
	RecvPropInt( RECVINFO( m_bDisabled ), 0, RecvProxy_IsDisabled ),
	RecvPropInt( RECVINFO( m_nGameType ) ),
	RecvPropInt( RECVINFO( m_nFlagStatus ) ),
	RecvPropTime( RECVINFO( m_flResetTime ) ),
	RecvPropTime( RECVINFO( m_flNeutralTime ) ),
	RecvPropTime( RECVINFO( m_flMaxResetTime ) ),
	RecvPropEHandle( RECVINFO( m_hPrevOwner ) ),
#ifdef PF2_CLIENT
	RecvPropInt( RECVINFO( m_bSlowPlayer ) ),
#endif
#endif
END_NETWORK_TABLE()

BEGIN_DATADESC( CCaptureFlag )

	// Keyfields.
	DEFINE_KEYFIELD( m_nGameType, FIELD_INTEGER, "GameType" ),
#ifdef GAME_DLL
#ifdef PF2_DLL
	DEFINE_KEYFIELD( m_nReturnTime, FIELD_INTEGER, "ReturnTime" ),
	DEFINE_KEYFIELD( m_nNeutralType, FIELD_INTEGER, "NeutralType" ),
	DEFINE_KEYFIELD( m_bInvadeScoringType, FIELD_BOOLEAN, "ScoringType" ),

	DEFINE_KEYFIELD( m_bSlowPlayer, FIELD_BOOLEAN, "SlowPlayer" ),
	
	// Live Keyvalues
	DEFINE_KEYFIELD( m_FlagModelName, FIELD_MODELNAME, "flag_model" ),

	// Flagrun keyfields
	DEFINE_KEYFIELD( m_VecFlagRedHome, FIELD_VECTOR, "red_home" ),
	DEFINE_KEYFIELD( m_VecFlagBlueHome, FIELD_VECTOR, "blue_home"),

#endif
	// Inputs.
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "RoundActivate", InputRoundActivate ),

#ifdef PF2_DLL
	// Live Inputs.
	DEFINE_INPUTFUNC( FIELD_VOID, "ForceDrop", InputForceDrop ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ForceReset", InputForceReset ),
	DEFINE_INPUTFUNC( FIELD_VOID, "SetReturnTime", InputSetReturnTime ),
#endif

	// Outputs.
	DEFINE_OUTPUT( m_outputOnReturn, "OnReturn" ),
	DEFINE_OUTPUT( m_outputOnPickUp, "OnPickUp" ),
	DEFINE_OUTPUT( m_outputOnDrop, "OnDrop" ),
	DEFINE_OUTPUT( m_outputOnCapture, "OnCapture" ),

#ifdef PF2_DLL
	// Live Outputs.
	DEFINE_OUTPUT( m_outputOnCapTeam1, "OnCapTeam1" ),
	DEFINE_OUTPUT( m_outputOnCapTeam2, "OnCapTeam2" ),
	DEFINE_OUTPUT( m_outputOnPickUpTeam1, "OnPickUpTeam1" ),
	DEFINE_OUTPUT( m_outputOnPickUpTeam2, "OnPickUpTeam2" ),
	DEFINE_OUTPUT( m_outputOnTouchSameTeam, "OnTouchSameTeam" ),
	
	// PF2 Outputs.
	DEFINE_OUTPUT( m_outputOnThrow, "OnThrow" ),
	DEFINE_OUTPUT( m_outputOnTouchOpposingTeam, "OnTouchOpposingTeam" )
#endif
#endif

END_DATADESC();

LINK_ENTITY_TO_CLASS( item_teamflag, CCaptureFlag );

//=============================================================================
//
// CTF Flag functions.
//

CCaptureFlag::CCaptureFlag()
{
#ifdef CLIENT_DLL
	m_pGlowTrailEffect = NULL;
	m_pPaperTrailEffect = NULL;
#else
	m_hReturnIcon = NULL;
#ifdef PF2_DLL
	m_nReturnTime = 60;
	m_nNeutralType = TF_INVADE_RETURN_DEFAULT;
	m_bInvadeScoringType = false; // increment team score by default
#endif
#endif

#ifdef PF2
	m_bSlowPlayer = false;
#endif
	UseClientSideAnimation();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
unsigned int CCaptureFlag::GetItemID( void )
{
	return TF_ITEM_CAPTURE_FLAG;
}

//-----------------------------------------------------------------------------
// Purpose: Precache the model and sounds.
//-----------------------------------------------------------------------------
void CCaptureFlag::Precache( void )
{
	PrecacheModel( TF_FLAG_MODEL );
#ifdef GAME_DLL
	PrecacheModel( m_FlagModelName.ToCStr() );
#endif
	PrecacheScriptSound( TF_CTF_FLAGSPAWN );
	PrecacheScriptSound( TF_CTF_ENEMY_STOLEN );
	PrecacheScriptSound( TF_CTF_ENEMY_DROPPED );
	PrecacheScriptSound( TF_CTF_ENEMY_CAPTURED );
	PrecacheScriptSound( TF_CTF_ENEMY_RETURNED );
	PrecacheScriptSound( TF_CTF_TEAM_STOLEN );
	PrecacheScriptSound( TF_CTF_TEAM_DROPPED );
	PrecacheScriptSound( TF_CTF_TEAM_CAPTURED );
	PrecacheScriptSound( TF_CTF_TEAM_RETURNED );

	PrecacheScriptSound( TF_AD_CAPTURED_SOUND );
	PrecacheScriptSound( TF_AD_ENEMY_STOLEN );
	PrecacheScriptSound( TF_AD_ENEMY_DROPPED );
	PrecacheScriptSound( TF_AD_ENEMY_CAPTURED );
	PrecacheScriptSound( TF_AD_ENEMY_RETURNED );
	PrecacheScriptSound( TF_AD_TEAM_STOLEN );
	PrecacheScriptSound( TF_AD_TEAM_DROPPED );
	PrecacheScriptSound( TF_AD_TEAM_CAPTURED );
	PrecacheScriptSound( TF_AD_TEAM_RETURNED );

	PrecacheScriptSound( TF_INVADE_ENEMY_STOLEN );
	PrecacheScriptSound( TF_INVADE_ENEMY_DROPPED );
	PrecacheScriptSound( TF_INVADE_ENEMY_CAPTURED );
	PrecacheScriptSound( TF_INVADE_TEAM_STOLEN );
	PrecacheScriptSound( TF_INVADE_TEAM_DROPPED );
	PrecacheScriptSound( TF_INVADE_TEAM_CAPTURED );
	PrecacheScriptSound( TF_INVADE_FLAG_RETURNED );

	PrecacheParticleSystem( "player_intel_trail_blue" );
	PrecacheParticleSystem( "player_intel_trail_red" );
	PrecacheParticleSystem( "player_intel_papertrail" );
}

#ifndef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCaptureFlag::OnPreDataChanged( DataUpdateType_t updateType )
{
	m_nOldFlagStatus = m_nFlagStatus;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCaptureFlag::OnDataChanged( DataUpdateType_t updateType )
{
	if ( m_nOldFlagStatus != m_nFlagStatus )
	{
		IGameEvent *pEvent = gameeventmanager->CreateEvent( "flagstatus_update" );
		if ( pEvent )
		{
			gameeventmanager->FireEventClientSide( pEvent );
		}
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureFlag::Spawn( void )
{
	// Precache the model and sounds.  Set the flag model.
	Precache();
#ifdef GAME_DLL
	if( m_FlagModelName.ToCStr() )
		SetModel( m_FlagModelName.ToCStr() );
	else
		SetModel( TF_FLAG_MODEL );
#endif
	// Set the flag solid and the size for touching.
	SetSolid( SOLID_BBOX );
	SetSolidFlags( FSOLID_NOT_SOLID | FSOLID_TRIGGER );
	SetSize( vec3_origin, vec3_origin );

	// Bloat the box for player pickup
	CollisionProp()->UseTriggerBounds( true, 24 );
	// use the initial dynamic prop "m_bStartDisabled" setting to set our own m_bDisabled flag
#ifdef GAME_DLL
	m_bDisabled = m_bStartDisabled;
	m_bStartDisabled = false;

	// Don't allow the intelligence to fade.
	m_flFadeScale = 0.0f;
#else
	m_bDisabled = false;
#endif

	// Base class spawn.
	BaseClass::Spawn();

#ifdef GAME_DLL
	// Save the starting position, so we can reset the flag later if need be.
	m_vecResetPos = GetAbsOrigin();
	m_vecResetAng = GetAbsAngles();

	SetFlagStatus( TF_FLAGINFO_NONE );
	ResetFlagReturnTime();
	ResetFlagNeutralTime();

	m_bAllowOwnerPickup = true;
	m_hPrevOwner = NULL;

	m_bCaptured = false;
	m_bWasReturnedByOpposingTeam = false;
	m_bWasThrown = false;

#else

	// add this element if it isn't already in the list
	if ( g_Flags.Find( entindex() ) == -1 )
	{
		g_Flags.AddToTail( entindex() );
	}

#endif

	if ( m_bDisabled )
	{
		SetDisabled( true );
	}
	else
	{
		SetDisabled( false );
	}
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCaptureFlag::Activate( void )
{
	BaseClass::Activate();

	m_iOriginalTeam = GetTeamNumber();
	m_nSkin = ( GetTeamNumber() == TEAM_UNASSIGNED ) ? 2 : (GetTeamNumber() - 2);
}
#endif


//-----------------------------------------------------------------------------
// Purpose: Reset the flag position state.
//-----------------------------------------------------------------------------
void CCaptureFlag::Reset( void )
{
#ifdef GAME_DLL
	// Set the flag position.
	SetAbsOrigin( m_vecResetPos );
	SetAbsAngles( m_vecResetAng );

	// No longer dropped, if it was.
	SetFlagStatus( TF_FLAGINFO_NONE );
	ResetFlagReturnTime();
	ResetFlagNeutralTime();

	m_bAllowOwnerPickup = true;
	m_hPrevOwner = NULL;
#ifdef PF2_DLL
	m_bWasThrown = false;
#endif
	if ( m_nGameType == TF_FLAGTYPE_INVADE || TFGameRules()->IsCTFFlagrun() )
	{
		ChangeTeam( m_iOriginalTeam );
		m_nSkin = ( GetTeamNumber() == TEAM_UNASSIGNED ) ? 2 : (GetTeamNumber() - 2);
	}

	// Fire event
	if( m_bWasReturnedByOpposingTeam )
	{
		m_outputOnTouchOpposingTeam.FireOutput( this, this );
	}

	PhysicsToggle( false );
#endif 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCaptureFlag::ResetMessage( void )
{
#ifdef GAME_DLL
	if ( m_nGameType == TF_FLAGTYPE_CTF )
	{
		for ( int iTeam = TF_TEAM_RED; iTeam < TF_TEAM_COUNT; ++iTeam )
		{
			if ( iTeam == GetTeamNumber() )
			{
				CTeamRecipientFilter filter( iTeam, true );
				EmitSound( filter, entindex(), TF_CTF_ENEMY_RETURNED );

				TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_YOUR_FLAG_RETURNED );
			}
			else
			{
				CTeamRecipientFilter filter( iTeam, true );
				EmitSound( filter, entindex(), TF_CTF_TEAM_RETURNED );

				TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_ENEMY_FLAG_RETURNED );
			}
		}

		// Returned sound
		CPASAttenuationFilter filter( this, TF_CTF_FLAGSPAWN );
		EmitSound( filter, entindex(), TF_CTF_FLAGSPAWN );
	}
	else if ( m_nGameType == TF_FLAGTYPE_ATTACK_DEFEND )
	{
		for ( int iTeam = TF_TEAM_RED; iTeam < TF_TEAM_COUNT; ++iTeam )
		{
			if ( iTeam == GetTeamNumber() )
			{
				TFTeamMgr()->TeamCenterPrint( iTeam, "#TF_AD_FlagReturned" );

				CTeamRecipientFilter filter( iTeam, true );
				EmitSound( filter, entindex(), TF_AD_TEAM_RETURNED );
			}
			else
			{
				TFTeamMgr()->TeamCenterPrint( iTeam, "#TF_AD_FlagReturned" );

				CTeamRecipientFilter filter( iTeam, true );
				EmitSound( filter, entindex(), TF_AD_ENEMY_RETURNED );
			}
		}
	}
	else if ( m_nGameType == TF_FLAGTYPE_INVADE )
	{
		for ( int iTeam = TF_TEAM_RED; iTeam < TF_TEAM_COUNT; ++iTeam )
		{
			CTeamRecipientFilter filter( iTeam, true );
			EmitSound( filter, entindex(), TF_INVADE_FLAG_RETURNED );
			TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_ENEMY_FLAG_RETURNED ); // #TF_Invade_FlagReturned
		}
		
		// Returned sound
		CPASAttenuationFilter filter( this, TF_CTF_FLAGSPAWN );
		EmitSound( filter, entindex(), TF_CTF_FLAGSPAWN );
	}

	// Output.
	m_outputOnReturn.FireOutput( this, this );

	if ( m_hReturnIcon.Get() )
	{
		UTIL_Remove( m_hReturnIcon );
		m_hReturnIcon = NULL;
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureFlag::FlagTouch( CBaseEntity *pOther )
{
	// Is the flag enabled?
	if ( IsDisabled() )
	{
		return;
	}

	// The touch from a live player.
	if ( !pOther->IsPlayer() || !pOther->IsAlive() )
	{
		return;
	}

#ifdef GAME_DLL
	// Don't let the person who threw this flag pick it up until it hits the ground.
	// This way we can throw the flag to people, but not touch it as soon as we throw it ourselves
	if(  m_hPrevOwner.Get() && m_hPrevOwner.Get() == pOther && m_bAllowOwnerPickup == false )
	{
		return;
	}

	if( pOther->GetTeamNumber() == GetTeamNumber() )
	{
		m_outputOnTouchSameTeam.FireOutput( this, this );
	}
#endif

	// Does my team own this flag? If so, no touch.
	if ( m_nGameType == TF_FLAGTYPE_CTF && pOther->GetTeamNumber() == GetTeamNumber() )
	{
		return;
	}

	if ( ( m_nGameType == TF_FLAGTYPE_ATTACK_DEFEND || m_nGameType == TF_FLAGTYPE_TERRITORY_CONTROL ) &&
		   pOther->GetTeamNumber() != GetTeamNumber() )
	{
#ifdef GAME_DLL
		// Reset the flag upon the enemy touching it if this command is on.
		if( pf_flag_allow_return_on_touch.GetBool() )
		{
			if( m_nFlagStatus & (TF_FLAGINFO_DROPPED | TF_FLAGINFO_THROWN) )
			{
				// do the typical resetting
				m_bWasReturnedByOpposingTeam = true;
				Reset();
				ResetMessage();

				// remove the capture flag meter
				if( m_hReturnIcon.Get() )
				{
					UTIL_Remove( m_hReturnIcon );
					m_hReturnIcon = NULL;
				}
				// event
				IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_flag_event" );
				if( event )
				{
					event->SetInt( "player", pOther->entindex() );
					event->SetInt( "eventtype", TF_FLAGEVENT_RETURNED ); // by opposing team
					event->SetInt( "priority", 7 );
					gameeventmanager->FireEvent( event );
				}
			}
		}
#endif
		return;
	}

	if ( m_nGameType == TF_FLAGTYPE_INVADE && GetTeamNumber() != TEAM_UNASSIGNED )
	{
		if ( pOther->GetTeamNumber() != GetTeamNumber() )
		{
			return;
		}
	}

	// Can't pickup flags during WaitingForPlayers
	if ( TFGameRules()->IsInWaitingForPlayers() )
		return;

	// Get the touching player.
	CTFPlayer *pPlayer = ToTFPlayer( pOther );
	if ( !pPlayer )
	{
		return;
	}

	// Is the touching player about to teleport?
	if ( pPlayer->m_Shared.InCond( TF_COND_SELECTED_TO_TELEPORT ) )
		return;

	// Don't let invulnerable players pickup flags
	if ( pPlayer->m_Shared.InCond( TF_COND_INVULNERABLE ) )
		return;

	// Don't let cloaked players pickup flags
	if ( pPlayer->m_Shared.InCond( TF_COND_STEALTHED ) || pPlayer->m_Shared.InCond( TF_COND_SMOKE_BOMB ) )
		return;

	// Quality of Life Feature: Don't let disguised spies pick up the intelligence
	if ( pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) )
		return;

	// don't let players pick up more than 1 flag
	if( pPlayer->HasTheFlag() )
		return;

#ifdef GAME_DLL
	if ( PointInRespawnRoom( pPlayer, pPlayer->WorldSpaceCenter() ) )
		return;
#endif

	// Pick up the flag.
	PickUp( pPlayer, true );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureFlag::PickUp( CTFPlayer *pPlayer, bool bInvisible )
{
	// Is the flag enabled?
	if ( IsDisabled() )
		return;

	if ( !TFGameRules()->FlagsMayBeCapped() )
		return;

#ifdef GAME_DLL
#ifdef PF2_DLL
	if( m_bWasThrown )
	{
		PhysicsToggle( false );
	}

	m_bWasThrown = false;
	m_bWasReturnedByOpposingTeam = false;
#endif

	if ( !m_bAllowOwnerPickup )
	{
		if ( m_hPrevOwner.Get() && m_hPrevOwner.Get() == pPlayer ) 
		{
			return;
		}
	}

#endif


	// Call into the base class pickup.
	BaseClass::PickUp( pPlayer, false );

	pPlayer->TeamFortress_SetSpeed();

#ifdef GAME_DLL 
	// what the fuck is this

	/*	DISABLED - Waiting for a proper model first.
	CTFWeaponBase *pMelee = pPlayer->Weapon_GetWeaponByType(TF_WPN_TYPE_MELEE);
	if (pMelee)
	{
		pPlayer->Weapon_Detach(pMelee);
		UTIL_Remove(pMelee);
	}
	CTFWeaponBase *pFlagWeapon = static_cast<CTFWeaponBase*>(pPlayer->GiveNamedItem("tf_weapon_flag"));
	if (pFlagWeapon)
	{
		pFlagWeapon->DefaultTouch(pPlayer);
		pPlayer->SetActiveWeapon(pFlagWeapon);
	}
	*/
	
	// Update the parent to set the correct place on the model to attach the flag.
	int iAttachment = pPlayer->LookupAttachment( "flag" );
	if( iAttachment != -1 )
	{
		SetParent( pPlayer, iAttachment );
		SetLocalOrigin( vec3_origin );
		SetLocalAngles( vec3_angle );
	}

	// Remove the player's disguse if they're a spy
	if ( pPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_SPY )
	{
		if ( pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) ||
			pPlayer->m_Shared.InCond( TF_COND_DISGUISING ))
		{
			pPlayer->m_Shared.RemoveDisguise();
		}
	}

	// Remove the touch function.
	SetTouch( NULL );

	m_hPrevOwner = pPlayer;
	m_bAllowOwnerPickup = true;

	if ( m_nGameType == TF_FLAGTYPE_CTF )
	{
		for ( int iTeam = TF_TEAM_RED; iTeam < TF_TEAM_COUNT; ++iTeam )
		{
			if ( iTeam != pPlayer->GetTeamNumber() )
			{
				CTeamRecipientFilter filter( iTeam, true );
				EmitSound( filter, entindex(), TF_CTF_ENEMY_STOLEN );

				TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_YOUR_FLAG_TAKEN );
			}
			else
			{
				CTeamRecipientFilter filter( iTeam, true );
				EmitSound( filter, entindex(), TF_CTF_TEAM_STOLEN );

				// exclude the guy who just picked it up
				filter.RemoveRecipient( pPlayer );

				TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_ENEMY_FLAG_TAKEN );
			}
		}
	}
	else if ( m_nGameType == TF_FLAGTYPE_ATTACK_DEFEND )
	{
		// Handle messages to the screen.
		TFTeamMgr()->PlayerCenterPrint( pPlayer, "#TF_AD_TakeFlagToPoint" );

		for ( int iTeam = TF_TEAM_RED; iTeam < TF_TEAM_COUNT; ++iTeam )
		{
			if ( iTeam != pPlayer->GetTeamNumber() )
			{
				CTeamRecipientFilter filter( iTeam, true );
				EmitSound( filter, entindex(), TF_AD_ENEMY_STOLEN );
			}
			else
			{
				CTeamRecipientFilter filter( iTeam, true );
				EmitSound( filter, entindex(), TF_AD_TEAM_STOLEN );
			}
		}
	}
	else if ( m_nGameType == TF_FLAGTYPE_INVADE )
	{
		for ( int iTeam = TF_TEAM_RED; iTeam < TF_TEAM_COUNT; ++iTeam )
		{
			if ( iTeam != pPlayer->GetTeamNumber() )
			{
				CTeamRecipientFilter filter( iTeam, true );
				EmitSound( filter, entindex(), TF_INVADE_ENEMY_STOLEN );

				TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_YOUR_FLAG_TAKEN ); // #TF_Invade_PlayerTeamPickup
			}
			else
			{
				CTeamRecipientFilter filter( iTeam, true );
				EmitSound( filter, entindex(), TF_INVADE_TEAM_STOLEN );

				// exclude the guy who just picked it up
				filter.RemoveRecipient( pPlayer );

				TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_ENEMY_FLAG_TAKEN ); // #TF_Invade_OtherTeamPickup
			}
		}
		
		// set the flag's team to match the player's team
		ChangeTeam( pPlayer->GetTeamNumber() );
		m_nSkin = ( GetTeamNumber() == TEAM_UNASSIGNED ) ? 2 : (GetTeamNumber() - 2);
	}

	SetFlagStatus( TF_FLAGINFO_STOLEN );
	ResetFlagReturnTime();
	ResetFlagNeutralTime();

	IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_flag_event" );
	if ( event )
	{
		event->SetInt( "player", pPlayer->entindex() );
		event->SetInt( "eventtype", TF_FLAGEVENT_PICKUP );
		event->SetInt( "priority", 8 );
		gameeventmanager->FireEvent( event );
	}

	pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_FLAGPICKUP );

	// Output.
	m_outputOnPickUp.FireOutput( this, this );

	switch(pPlayer->GetTeamNumber())
	{
		case TF_TEAM_RED:
			m_outputOnPickUpTeam1.FireOutput( this, this );
			break;
		case TF_TEAM_BLUE:
			m_outputOnPickUpTeam2.FireOutput( this, this );
			break;
	}

	if ( m_hReturnIcon.Get() )
	{
		UTIL_Remove( m_hReturnIcon );
		m_hReturnIcon = NULL;
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureFlag::Capture( CTFPlayer *pPlayer, int nCapturePoint, bool reset )
{
	// PFTODO: Make it so that you can throw the flag into a capture point and capture.

	// Is the flag enabled?
	if ( IsDisabled() )
		return;

#ifdef GAME_DLL

	if ( m_nGameType == TF_FLAGTYPE_CTF )
	{
		bool bFlagrunMode = TFGameRules()->IsCTFFlagrun();

		bool bNotify = true;
		
		// don't play any sounds if this is going to win the round for one of the teams (victory sound will be played instead)
		if ( !bFlagrunMode && tf_flag_caps_per_round.GetInt() > 0 )
		{
			int nCaps = TFTeamMgr()->GetFlagCaptures( pPlayer->GetTeamNumber() );
			
			if ( ( nCaps >= 0 ) && ( tf_flag_caps_per_round.GetInt() - nCaps <= 1 ) )
			{
				// this cap is going to win, so don't play a sound
				bNotify = false;
			}
		}

		if ( bNotify )
		{
			for ( int iTeam = TF_TEAM_RED; iTeam < TF_TEAM_COUNT; ++iTeam )
			{
				if ( iTeam != pPlayer->GetTeamNumber() )
				{
					CTeamRecipientFilter filter( iTeam, true );
					EmitSound( filter, entindex(), TF_CTF_ENEMY_CAPTURED );

					TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_YOUR_FLAG_CAPTURED );
				}
				else
				{
					CTeamRecipientFilter filter( iTeam, true );
					EmitSound( filter, entindex(), TF_CTF_TEAM_CAPTURED );

					TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_ENEMY_FLAG_CAPTURED );
				}
			}
		}

		// Reward the player
		CTF_GameStats.Event_PlayerCapturedPoint( pPlayer );

		if( bFlagrunMode )
		{
			int iCapTeam = pPlayer->GetTeamNumber();

			m_iOriginalTeam = iCapTeam;
			if( iCapTeam == TF_TEAM_RED )
			{
				m_vecResetPos = m_VecFlagRedHome;
			}
			else if( iCapTeam == TF_TEAM_BLUE )
			{
				m_vecResetPos = m_VecFlagBlueHome;
			}
		}
		// Reward the team
		else if( tf_flag_caps_per_round.GetInt() > 0 )
		{
			TFTeamMgr()->IncrementFlagCaptures( pPlayer->GetTeamNumber() );
		}
		else
		{
			TFTeamMgr()->AddTeamScore( pPlayer->GetTeamNumber(), TF_CTF_CAPTURED_TEAM_FRAGS );
		}
	}
	else if ( m_nGameType == TF_FLAGTYPE_ATTACK_DEFEND )
	{
		char szNumber[64];
		Q_snprintf( szNumber, sizeof(szNumber), "%d", nCapturePoint );

		// Handle messages to the screen.
		TFTeamMgr()->PlayerCenterPrint( pPlayer, "#TF_AD_YouSecuredPoint", szNumber );
		TFTeamMgr()->PlayerTeamCenterPrint( pPlayer, "TF_AD_AttackersSecuredPoint", szNumber );
		for ( int iTeam = TF_TEAM_RED; iTeam < TF_TEAM_COUNT; ++iTeam )
		{
			if ( iTeam != pPlayer->GetTeamNumber() )
			{
				TFTeamMgr()->TeamCenterPrint( iTeam, "#TF_AD_AttackersSecuredPoint" );

				CTeamRecipientFilter filter( iTeam, true );
				EmitSound( filter, entindex(), TF_AD_ENEMY_CAPTURED );
			}
			else
			{
				CTeamRecipientFilter filter( iTeam, true );
				EmitSound( filter, entindex(), TF_AD_TEAM_CAPTURED );
			}
		}

		// Capture sound
		CBroadcastRecipientFilter filter;
		EmitSound( filter, entindex(), TF_AD_CAPTURED_SOUND );

		// Reward the player
		CTF_GameStats.Event_PlayerCapturedPoint( pPlayer );

		// TFTODO:: Reward the team	
	}
	else if ( m_nGameType == TF_FLAGTYPE_INVADE )
	{
		bool bNotify = true;

		// don't play any sounds if this is going to win the round for one of the teams (victory sound will be played instead)
		if ( tf_flag_caps_per_round.GetInt() > 0 )
		{
			int nCaps = TFTeamMgr()->GetFlagCaptures( pPlayer->GetTeamNumber() );

			if ( ( nCaps >= 0 ) && ( tf_flag_caps_per_round.GetInt() - nCaps <= 1 ) )
			{
				// this cap is going to win, so don't play a sound
				bNotify = false;
			}
		}

		if ( bNotify )
		{
			for ( int iTeam = TF_TEAM_RED; iTeam < TF_TEAM_COUNT; ++iTeam )
			{
				if ( iTeam != pPlayer->GetTeamNumber() )
				{
					CTeamRecipientFilter filter( iTeam, true );
					EmitSound( filter, entindex(), TF_INVADE_ENEMY_CAPTURED); // TF_INVADE_ENEMY_CAPTURED

					TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_YOUR_FLAG_CAPTURED ); // #TF_Invade_PlayerTeamCapture
				}
				else
				{
					CTeamRecipientFilter filter( iTeam, true );
					EmitSound( filter, entindex(), TF_INVADE_TEAM_CAPTURED); // TF_INVADE_TEAM_CAPTURED

					TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_ENEMY_FLAG_CAPTURED ); // #TF_Invade_OtherTeamCapture"
				}
			}
		}

		// Reward the player
		CTF_GameStats.Event_PlayerCapturedPoint( pPlayer );

		// Reward the team
		if ( tf_flag_caps_per_round.GetInt() > 0 && m_bInvadeScoringType )
		{
			TFTeamMgr()->IncrementFlagCaptures( pPlayer->GetTeamNumber() );
		}
		else
		{
			TFTeamMgr()->AddTeamScore( pPlayer->GetTeamNumber(), TF_INVADE_CAPTURED_TEAM_FRAGS );
		}
	}

	IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_flag_event" );
	if ( event )
	{
		event->SetInt( "player", pPlayer->entindex() );
		event->SetInt( "eventtype", TF_FLAGEVENT_CAPTURE );
		event->SetInt( "priority", 9 );
		gameeventmanager->FireEvent( event );
	}

	if (reset || TFGameRules()->GetGameType() != TF_GAMETYPE_CP)
	{
		SetFlagStatus( TF_FLAGINFO_NONE );
		ResetFlagReturnTime();
		ResetFlagNeutralTime();

		// Reset the flag.
		BaseClass::Drop(pPlayer, true);

		Reset();

		pPlayer->TeamFortress_SetSpeed();
	}
	pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_FLAGCAPTURED );
	
	// Output.
	m_outputOnCapture.FireOutput( this, this );

	switch(pPlayer->GetTeamNumber())
	{
		case TF_TEAM_RED:
			m_outputOnCapTeam1.FireOutput( this, this );
			break;
		case TF_TEAM_BLUE:
			m_outputOnCapTeam2.FireOutput( this, this );
			break;
	}

	m_bCaptured = true;
	SetNextThink( gpGlobals->curtime + TF_FLAG_THINK_TIME );

	if ( TFGameRules()->InStalemate() )
	{
		// whoever capped the flag is the winner, give them enough caps to win
		CTFTeam *pTeam = pPlayer->GetTFTeam();
		if ( !pTeam )
			return;

		// if we still need more caps to trigger a win, give them to us
		if ( pTeam->GetFlagCaptures() < tf_flag_caps_per_round.GetInt() )
		{
			pTeam->SetFlagCaptures( tf_flag_caps_per_round.GetInt() );
		}	
	}

#endif
}

//-----------------------------------------------------------------------------
// Purpose: A player drops the flag.
//-----------------------------------------------------------------------------
void CCaptureFlag::Drop( CTFPlayer *pPlayer, bool bVisible, bool bThrown /*= false*/, bool bMessage /*= true*/
#ifdef PF2
, bool bThrowingFlag /*= false */
#endif
)
{
	// Is the flag enabled?
	if ( IsDisabled() )
		return;

	// Call into the base class drop.
	BaseClass::Drop( pPlayer, bVisible );

	pPlayer->TeamFortress_SetSpeed();

#ifdef GAME_DLL
	// Will be removed if it's scrapped, this code currently causes the Minigun Spawning bug due to
	// missing TF_WEAPON_FLAG

	/* 
	pPlayer->TFWeaponRemove( TF_WEAPON_FLAG );
	CTFWeaponBase *pMelee = pPlayer->Weapon_GetWeaponByType( TF_WPN_TYPE_MELEE );
	if (!pMelee)
	{
		int id = pPlayer->GetPlayerClass()->GetData()->m_aWeapons[2];
		CTFWeaponBase *pCreateMelee = static_cast<CTFWeaponBase*>(pPlayer->Weapon_Create(WeaponIdToClassname(id)));
		if ( pCreateMelee )
		{
			pCreateMelee->DefaultTouch(pPlayer);
		}
		pPlayer->SwitchToNextBestWeapon(pPlayer->Weapon_OwnsThisID(TF_WEAPON_FLAG));
	}*/
	if ( bThrown )
	{
		m_bAllowOwnerPickup = false;
		m_flOwnerPickupTime = gpGlobals->curtime + TF_FLAG_OWNER_PICKUP_TIME;
	}

#ifdef PF2_DLL
	if ( pf_flag_allow_throwing.GetBool() && !m_bWasThrown && bThrowingFlag )
	{
		Throw( pPlayer );
		m_bWasThrown = true;
	}
	else
#endif
	{
		Vector vecTelePoint;
		Vector vecStart = pPlayer->WorldSpaceCenter();
		Vector vecEnd = vecStart;
		vecEnd.z -= 8000.0f;
		trace_t trace;
		UTIL_TraceHull( vecStart, vecEnd, WorldAlignMins(), WorldAlignMaxs(), MASK_SOLID, this, COLLISION_GROUP_DEBRIS, &trace );
		vecTelePoint = trace.endpos;
		SetAbsOrigin( vecTelePoint );
	}
	
	if ( m_nGameType == TF_FLAGTYPE_CTF )
	{
		if ( bMessage )
		{
			for ( int iTeam = TF_TEAM_RED; iTeam < TF_TEAM_COUNT; ++iTeam )
			{
				if ( iTeam != pPlayer->GetTeamNumber() )
				{
					CTeamRecipientFilter filter( iTeam, true );
					EmitSound( filter, entindex(), TF_CTF_ENEMY_DROPPED );

					TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_YOUR_FLAG_DROPPED );
				}
				else
				{
					CTeamRecipientFilter filter( iTeam, true );
					EmitSound( filter, entindex(), TF_CTF_TEAM_DROPPED );

					TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_ENEMY_FLAG_DROPPED );
				}
			}
		}

		SetFlagReturnIn( m_nReturnTime );
	}
	else if ( m_nGameType == TF_FLAGTYPE_INVADE )
	{
		if ( bMessage )
		{
			for ( int iTeam = TF_TEAM_RED; iTeam < TF_TEAM_COUNT; ++iTeam )
			{
				if ( iTeam != pPlayer->GetTeamNumber() )
				{
					CTeamRecipientFilter filter( iTeam, true );
					EmitSound( filter, entindex(), TF_INVADE_ENEMY_DROPPED );

					TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_YOUR_FLAG_DROPPED ); // #TF_Invade_FlagDrop
				}
				else
				{
					CTeamRecipientFilter filter( iTeam, true );
					EmitSound( filter, entindex(), TF_INVADE_TEAM_DROPPED );

					TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_ENEMY_FLAG_DROPPED ); // #TF_Invade_FlagDrop
				}
			}
		}

		SetFlagReturnIn( m_nReturnTime );
		switch ( m_nNeutralType )
		{
			case TF_INVADE_RETURN_HALF:
				SetFlagNeutralIn( m_nReturnTime/2 );
				break;
			case TF_INVADE_RETURN_NONE:
				break;
			default:
				SetFlagNeutralIn( TF_INVADE_NEUTRAL_TIME );
				break;
		}
	}
	else if ( m_nGameType == TF_FLAGTYPE_ATTACK_DEFEND )
	{
		if ( bMessage )
		{
			for ( int iTeam = TF_TEAM_RED; iTeam < TF_TEAM_COUNT; ++iTeam )
			{
				if ( iTeam != pPlayer->GetTeamNumber() )
				{
					CTeamRecipientFilter filter( iTeam, true );
					EmitSound( filter, entindex(), TF_AD_ENEMY_DROPPED );
				}
				else
				{
					CTeamRecipientFilter filter( iTeam, true );
					EmitSound( filter, entindex(), TF_AD_TEAM_DROPPED );
				}
			}
		}

		SetFlagReturnIn( m_nReturnTime );
	}

	// Reset the flag's angles.
	SetAbsAngles( m_vecResetAng );

	// Reset the touch function.
	SetTouch( &CCaptureFlag::FlagTouch );

#ifdef PF2_DLL
	// Outputs.
	if ( m_bWasThrown )
	{
		SetFlagStatus( TF_FLAGINFO_THROWN );
		m_outputOnThrow.FireOutput( this, this );
	}
	else
#endif
	{
		SetFlagStatus( TF_FLAGINFO_DROPPED );
		m_outputOnDrop.FireOutput( this, this );
	}
	m_hReturnIcon = CBaseEntity::Create( "item_teamflag_return_icon", GetAbsOrigin() + Vector(0,0,cl_flag_return_height.GetFloat()), vec3_angle, this );

	if ( m_hReturnIcon )
	{
		m_hReturnIcon->SetParent( this );
	}

	if ( PointInRespawnFlagZone( GetAbsOrigin() ) )
	{
		Reset();
		ResetMessage();
	}

#endif
}

#ifdef PF2
void CCaptureFlag::Throw( CTFPlayer* pPlayer )
{
#ifdef GAME_DLL
	// set flag status to dropped
	SetGroundEntity( NULL );

	// turn on physics
	PhysicsToggle( true );

	// do an animation
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_GRENADE );

	Vector vecSrc, vecThrow;
	Vector vForward, vRight, vUp;
	QAngle angEyes = pPlayer->LocalEyeAngles();

	// Use the player shoot position so we don't throw it through walls
	vecSrc = pPlayer->Weapon_ShootPosition();

	// Lower the throw angle slightly
	angEyes.x -= 5;

	// Some maths to determine the velocity based on how high up we're looking
	// PFTODO - Multiplier convar?
	float flVelocity = (90.0f - angEyes.x) * 5.0f;
	// Max flag throwing force
	if( flVelocity > pf_flag_throw_force.GetFloat() )
		flVelocity = pf_flag_throw_force.GetFloat();

	// Get the forward vector from our eye angle
	AngleVectors( angEyes, &vForward, &vRight, &vUp );

	// Multiply the forward vector by our desired velocity
	vecThrow = vForward * flVelocity;

	// Set throw starting position 
	SetAbsOrigin( vecSrc );

	// Set the velocity
	SetAbsVelocity( vecThrow );
#endif
}

#ifdef PF2
void CCaptureFlag::PhysicsToggle( bool bToggle )
{
	if( bToggle )
	{
		SetCollisionGroup( TFCOLLISION_GROUP_FLAG );
		SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE );
	}
	else
	{
		SetCollisionGroup( COLLISION_GROUP_NONE );
		SetMoveType( MOVETYPE_NONE );
	}
}
#endif // PF2
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CCaptureFlag::IsThrown( void )
{
	return ( m_nFlagStatus == TF_FLAGINFO_THROWN );
}
#endif

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CCaptureFlag::IsDropped( void )
{ 
	return ( m_nFlagStatus == TF_FLAGINFO_DROPPED ); 
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CCaptureFlag::IsHome( void )
{ 
	return ( m_nFlagStatus == TF_FLAGINFO_NONE ); 
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CCaptureFlag::IsStolen( void )
{ 
	return ( m_nFlagStatus == TF_FLAGINFO_STOLEN ); 
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CCaptureFlag::IsDisabled( void )
{
	return m_bDisabled;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureFlag::SetDisabled( bool bDisabled )
{
	m_bDisabled = bDisabled;

	if ( bDisabled )
	{
		AddEffects( EF_NODRAW );

		SetTouch( NULL );
		SetThink( NULL );
	}
	else
	{
		RemoveEffects( EF_NODRAW );

		SetTouch( &CCaptureFlag::FlagTouch );
		SetThink( &CCaptureFlag::Think );
		SetNextThink( gpGlobals->curtime );
	}
}

//-----------------------------------------------------------------------------------------------
// GAME DLL Functions
//-----------------------------------------------------------------------------------------------
#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureFlag::Think( void )
{
	// Is the flag enabled?
	if ( IsDisabled() )
		return;

	if ( !TFGameRules()->FlagsMayBeCapped() )
	{
		SetNextThink( gpGlobals->curtime + TF_FLAG_THINK_TIME );
		return;
	}

	if ( m_bCaptured )
	{
		m_bCaptured = false;
		SetTouch( &CCaptureFlag::FlagTouch );
	}

#ifdef PF2_DLL
	if ( IsDropped() || IsThrown() )
#else
	if ( IsDropped() )
#endif
	{
		if ( !m_bAllowOwnerPickup )
		{
			if ( m_flOwnerPickupTime && gpGlobals->curtime > m_flOwnerPickupTime )
			{
				m_bAllowOwnerPickup = true;
			}
		}

		if ( m_nGameType == TF_FLAGTYPE_INVADE )
		{
			if ( m_flResetTime && gpGlobals->curtime > m_flResetTime )
			{
				Reset();
				ResetMessage();
			}
			else if ( m_flNeutralTime && gpGlobals->curtime > m_flNeutralTime )
			{
				for ( int iTeam = TF_TEAM_RED; iTeam < TF_TEAM_COUNT; ++iTeam )
				{
					TFTeamMgr()->TeamCenterPrint( iTeam, "#TF_Invade_FlagNeutral" );
				}

				// reset the team to the original team setting (when it spawned)
				ChangeTeam( m_iOriginalTeam );
				m_nSkin = ( GetTeamNumber() == TEAM_UNASSIGNED ) ? 2 : (GetTeamNumber() - 2);

				ResetFlagNeutralTime();
			}
		}
		else
		{
			if ( m_flResetTime && gpGlobals->curtime > m_flResetTime )
			{
				Reset();
				ResetMessage();
			}
		}
	}

	SetNextThink( gpGlobals->curtime + TF_FLAG_THINK_TIME );
}

//-----------------------------------------------------------------------------
// Purpose: Sets the flag status
//-----------------------------------------------------------------------------
void CCaptureFlag::SetFlagStatus( int iStatus )
{ 
	MDLCACHE_CRITICAL_SECTION();

	m_nFlagStatus = iStatus; 

	switch ( m_nFlagStatus )
	{
	case TF_FLAGINFO_NONE:
	case TF_FLAGINFO_DROPPED:
#ifdef PF2_DLL
	case TF_FLAGINFO_THROWN:
#endif
		ResetSequence( LookupSequence("spin") );	// set spin animation if it's not being held
		break;
	case TF_FLAGINFO_STOLEN:
		ResetSequence( LookupSequence("idle") );	// set idle animation if it is being held
		break;
	default:
		AssertOnce( false );	// invalid stats
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureFlag::InputEnable( inputdata_t &inputdata )
{
	SetDisabled( false );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureFlag::InputDisable( inputdata_t &inputdata )
{
	SetDisabled( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCaptureFlag::InputRoundActivate( inputdata_t &inputdata )
{
	CTFPlayer *pPlayer = ToTFPlayer( m_hPrevOwner.Get() );

	// If the player has a capture flag, drop it.
	if ( pPlayer && pPlayer->HasItem() && ( pPlayer->GetItem() == this ) )
	{
		Drop( pPlayer, true, false, false );
	}

	Reset();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureFlag::InputForceDrop( inputdata_t &inputdata )
{
	CTFPlayer *pPlayer = ToTFPlayer( m_hPrevOwner.Get() );

	// If the player has a capture flag, drop it.
	if ( pPlayer && pPlayer->HasItem() && ( pPlayer->GetItem() == this ) )
	{
		Drop( pPlayer, true, false, false );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureFlag::InputForceReset( inputdata_t &inputdata )
{
	CTFPlayer *pPlayer = ToTFPlayer( m_hPrevOwner.Get() );

	// If the player has a capture flag, drop it.
	if ( pPlayer && pPlayer->HasItem() && ( pPlayer->GetItem() == this ) )
	{
		Drop( pPlayer, true, false, false );
	}

	Reset();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureFlag::InputSetReturnTime( inputdata_t &inputdata )
{
	m_nReturnTime = inputdata.value.Int();
	if(IsDropped())
		SetFlagReturnIn(m_nReturnTime);
}

//-----------------------------------------------------------------------------
// Purpose: Always transmitted to clients
//-----------------------------------------------------------------------------
int CCaptureFlag::UpdateTransmitState()
{
	// ALWAYS transmit to all clients.
	return SetTransmitState( FL_EDICT_ALWAYS );
}


#else

float CCaptureFlag::GetReturnProgress()
{
	float flEventTime = max( m_flResetTime.m_Value, m_flNeutralTime.m_Value );

	return ( 1.0 - ( ( flEventTime - gpGlobals->curtime ) / m_flMaxResetTime ) );
}


void CCaptureFlag::Simulate( void )
{
	BaseClass::Simulate();

	ManageTrailEffects();
}

void CCaptureFlag::ManageTrailEffects( void )
{
	if ( m_nFlagStatus == TF_FLAGINFO_STOLEN )
	{
		if ( m_pGlowTrailEffect == NULL )
		{
			char *pEffectName = NULL;

			if ( GetPrevOwner() && GetPrevOwner() != CBasePlayer::GetLocalPlayer() )
			{
				switch( GetPrevOwner()->GetTeamNumber() )
				{
				case TF_TEAM_BLUE:
					pEffectName = "player_intel_trail_blue";
					break;
				case TF_TEAM_RED:
					pEffectName = "player_intel_trail_red";
					break;
				default:
					pEffectName = "player_intel_trail_blue";
					break;
				}



				m_pGlowTrailEffect = ParticleProp()->Create( pEffectName, PATTACH_ABSORIGIN_FOLLOW );
			}
		}

		if ( GetPrevOwner() )
		{
			CTFPlayer *pPlayer = ToTFPlayer( GetPrevOwner() );

			if ( pPlayer )
			{
				if ( pPlayer->GetAbsVelocity().Length() >= pPlayer->MaxSpeed() * 0.5f )
				{
					if ( m_pPaperTrailEffect == NULL )
					{
						m_pPaperTrailEffect = ParticleProp()->Create( "player_intel_papertrail", PATTACH_ABSORIGIN_FOLLOW );
					}
				}
				else
				{
					if ( m_pPaperTrailEffect )
					{
						ParticleProp()->StopEmission( m_pPaperTrailEffect );
						m_pPaperTrailEffect = NULL;
					}
				}
			}
		}

	}
	else
	{
		if ( m_pGlowTrailEffect )
		{
			ParticleProp()->StopEmission( m_pGlowTrailEffect );
			m_pGlowTrailEffect = NULL;
		}

		if ( m_pPaperTrailEffect )
		{
			ParticleProp()->StopEmission( m_pPaperTrailEffect );
			m_pPaperTrailEffect = NULL;
		}
	}
}


#endif


LINK_ENTITY_TO_CLASS( item_teamflag_return_icon, CCaptureFlagReturnIcon );

IMPLEMENT_NETWORKCLASS_ALIASED( CaptureFlagReturnIcon, DT_CaptureFlagReturnIcon )

BEGIN_NETWORK_TABLE( CCaptureFlagReturnIcon, DT_CaptureFlagReturnIcon )
END_NETWORK_TABLE()

CCaptureFlagReturnIcon::CCaptureFlagReturnIcon()
{
#ifdef CLIENT_DLL
	m_pReturnProgressMaterial_Empty = NULL;
	m_pReturnProgressMaterial_Full = NULL;
#endif
}

#ifdef GAME_DLL

void CCaptureFlagReturnIcon::Spawn( void )
{
	BaseClass::Spawn();

	UTIL_SetSize( this, Vector(-8,-8,-8), Vector(8,8,8) );

	CollisionProp()->SetCollisionBounds( Vector( -50, -50, -50 ), Vector( 50, 50, 50 ) );
}

int CCaptureFlagReturnIcon::UpdateTransmitState( void )
{
	return SetTransmitState( FL_EDICT_PVSCHECK );
}
#endif

#ifdef CLIENT_DLL

typedef struct
{
	float maxProgress;

	float vert1x;
	float vert1y;
	float vert2x;
	float vert2y;

	int swipe_dir_x;
	int swipe_dir_y;
} progress_segment_t;


// This defines the properties of the 8 circle segments
// in the circular progress bar.
progress_segment_t Segments[8] = 
{
	{ 0.125, 0.5, 0.0, 1.0, 0.0, 1, 0 },
	{ 0.25,	 1.0, 0.0, 1.0, 0.5, 0, 1 },
	{ 0.375, 1.0, 0.5, 1.0, 1.0, 0, 1 },
	{ 0.50,	 1.0, 1.0, 0.5, 1.0, -1, 0 },
	{ 0.625, 0.5, 1.0, 0.0, 1.0, -1, 0 },
	{ 0.75,	 0.0, 1.0, 0.0, 0.5, 0, -1 },
	{ 0.875, 0.0, 0.5, 0.0, 0.0, 0, -1 },
	{ 1.0,	 0.0, 0.0, 0.5, 0.0, 1, 0 },
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
RenderGroup_t CCaptureFlagReturnIcon::GetRenderGroup( void ) 
{	
	return RENDER_GROUP_TRANSLUCENT_ENTITY;	
}

void CCaptureFlagReturnIcon::GetRenderBounds( Vector& theMins, Vector& theMaxs )
{
	theMins.Init( -20, -20, -20 );
	theMaxs.Init(  20,  20,  20 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CCaptureFlagReturnIcon::DrawModel( int flags )
{
	int nRetVal = BaseClass::DrawModel( flags );
	
	DrawReturnProgressBar();

	return nRetVal;
}

//-----------------------------------------------------------------------------
// Purpose: Draw progress bar above the flag indicating when it will return
//-----------------------------------------------------------------------------
void CCaptureFlagReturnIcon::DrawReturnProgressBar( void )
{
	CCaptureFlag *pFlag = dynamic_cast< CCaptureFlag * > ( GetOwnerEntity() );

	if ( !pFlag )
		return;

	if (pFlag->IsDisabled())
		return;

	// Don't draw if this flag is not going to reset
	if ( pFlag->GetMaxResetTime() <= 0 )
		return;

	if ( !TFGameRules()->FlagsMayBeCapped() )
		return;

	if ( !m_pReturnProgressMaterial_Full )
	{
		m_pReturnProgressMaterial_Full = materials->FindMaterial( "VGUI/flagtime_full", TEXTURE_GROUP_VGUI );
	}

	if ( !m_pReturnProgressMaterial_Empty )
	{
		m_pReturnProgressMaterial_Empty = materials->FindMaterial( "VGUI/flagtime_empty", TEXTURE_GROUP_VGUI );
	}

	if ( !m_pReturnProgressMaterial_Full || !m_pReturnProgressMaterial_Empty )
	{
		return;
	}

	CMatRenderContextPtr pRenderContext( materials );

	Vector vOrigin = GetAbsOrigin();
	QAngle vAngle = vec3_angle;

	// Align it towards the viewer
	Vector vUp = CurrentViewUp();
	Vector vRight = CurrentViewRight();
	if ( fabs( vRight.z ) > 0.95 )	// don't draw it edge-on
		return;

	vRight.z = 0;
	VectorNormalize( vRight );

	float flSize = cl_flag_return_size.GetFloat();

	unsigned char ubColor[4];
	ubColor[3] = 255;

	switch( pFlag->GetTeamNumber() )
	{
	case TF_TEAM_RED:
		ubColor[0] = 255;
		ubColor[1] = 0;
		ubColor[2] = 0;
		break;
	case TF_TEAM_BLUE:
		ubColor[0] = 0;
		ubColor[1] = 0;
		ubColor[2] = 255;
		break;
	default:
		ubColor[0] = 100;
		ubColor[1] = 100;
		ubColor[2] = 100;
		break;
	}

	// First we draw a quad of a complete icon, background
	CMeshBuilder meshBuilder;

	pRenderContext->Bind( m_pReturnProgressMaterial_Empty );
	IMesh *pMesh = pRenderContext->GetDynamicMesh();

	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	meshBuilder.Color4ubv( ubColor );
	meshBuilder.TexCoord2f( 0,0,0 );
	meshBuilder.Position3fv( (vOrigin + (vRight * -flSize) + (vUp * flSize)).Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ubv( ubColor );
	meshBuilder.TexCoord2f( 0,1,0 );
	meshBuilder.Position3fv( (vOrigin + (vRight * flSize) + (vUp * flSize)).Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ubv( ubColor );
	meshBuilder.TexCoord2f( 0,1,1 );
	meshBuilder.Position3fv( (vOrigin + (vRight * flSize) + (vUp * -flSize)).Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ubv( ubColor );
	meshBuilder.TexCoord2f( 0,0,1 );
	meshBuilder.Position3fv( (vOrigin + (vRight * -flSize) + (vUp * -flSize)).Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();

	pMesh->Draw();

	float flProgress = pFlag->GetReturnProgress();

	pRenderContext->Bind( m_pReturnProgressMaterial_Full );
	pMesh = pRenderContext->GetDynamicMesh();

	vRight *= flSize * 2;
	vUp *= flSize * -2;

	// Next we're drawing the circular progress bar, in 8 segments
	// For each segment, we calculate the vertex position that will draw
	// the slice.
	int i;
	for ( i=0;i<8;i++ )
	{
		if ( flProgress < Segments[i].maxProgress )
		{
			CMeshBuilder meshBuilder_Full;

			meshBuilder_Full.Begin( pMesh, MATERIAL_TRIANGLES, 3 );

			// vert 0 is ( 0.5, 0.5 )
			meshBuilder_Full.Color4ubv( ubColor );
			meshBuilder_Full.TexCoord2f( 0, 0.5, 0.5 );
			meshBuilder_Full.Position3fv( vOrigin.Base() );
			meshBuilder_Full.AdvanceVertex();

			// Internal progress is the progress through this particular slice
			float internalProgress = RemapVal( flProgress, Segments[i].maxProgress - 0.125, Segments[i].maxProgress, 0.0, 1.0 );
			internalProgress = clamp( internalProgress, 0.0, 1.0 );

			// Calculate the x,y of the moving vertex based on internal progress
			float swipe_x = Segments[i].vert2x - ( 1.0 - internalProgress ) * 0.5 * Segments[i].swipe_dir_x;
			float swipe_y = Segments[i].vert2y - ( 1.0 - internalProgress ) * 0.5 * Segments[i].swipe_dir_y;

			// vert 1 is calculated from progress
			meshBuilder_Full.Color4ubv( ubColor );
			meshBuilder_Full.TexCoord2f( 0, swipe_x, swipe_y );
			meshBuilder_Full.Position3fv( (vOrigin + (vRight * ( swipe_x - 0.5 ) ) + (vUp *( swipe_y - 0.5 ) ) ).Base() );
			meshBuilder_Full.AdvanceVertex();

			// vert 2 is ( Segments[i].vert1x, Segments[i].vert1y )
			meshBuilder_Full.Color4ubv( ubColor );
			meshBuilder_Full.TexCoord2f( 0, Segments[i].vert2x, Segments[i].vert2y );
			meshBuilder_Full.Position3fv( (vOrigin + (vRight * ( Segments[i].vert2x - 0.5 ) ) + (vUp *( Segments[i].vert2y - 0.5 ) ) ).Base() );
			meshBuilder_Full.AdvanceVertex();

			meshBuilder_Full.End();

			pMesh->Draw();
		}
	}
}

#endif
