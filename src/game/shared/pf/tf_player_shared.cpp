//====== Copyright © 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "tf_gamerules.h"
#include "tf_player_shared.h"
#include "takedamageinfo.h"
#include "tf_weaponbase.h"
#include "effect_dispatch_data.h"
#include "tf_item.h"
#include "entity_capture_flag.h"
#include "baseobject_shared.h"
#include "tf_weapon_medigun.h"
#include "tf_weapon_pipebomblauncher.h"
#include "in_buttons.h"
#ifdef PF2
#include "tf_weapon_grenade_caltrop.h"
#include "tf_weapon_knife.h"
#include "tf_weapon_tranq.h"
#include "tf_weaponbase_grenade.h"
#include "tf_weapon_wrench.h"
#include "pf_cvars.h"
#endif
// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "c_te_effect_dispatch.h"
#include "c_tf_fx.h"
#include "soundenvelope.h"
#include "c_tf_playerclass.h"
#include "iviewrender.h"
#include "c_tf_playerresource.h"
#include "c_tf_team.h"
#ifdef PF2_CLIENT
#include "prediction.h"
#include "ScreenSpaceEffects.h"

#ifdef _DEBUG
#include "engine/ivdebugoverlay.h"
#endif
#endif

#define CTFPlayerClass C_TFPlayerClass

// Server specific.
#else
#include "tf_player.h"
#include "te_effect_dispatch.h"
#include "tf_fx.h"
#include "util.h"
#include "tf_team.h"
#include "tf_gamestats.h"
#include "tf_playerclass.h"
#ifdef PF2_DLL
#include "tf_weapon_builder.h"
#endif
#endif

ConVar tf_spy_invis_time( "tf_spy_invis_time", "1.0", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Transition time in and out of spy invisibility", true, 0.1, true, 5.0 );
ConVar tf_spy_invis_unstealth_time( "tf_spy_invis_unstealth_time", "2.0", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Transition time in and out of spy invisibility", true, 0.1, true, 5.0 );

ConVar tf_spy_max_cloaked_speed( "tf_spy_max_cloaked_speed", "999", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED );	// no cap
#ifdef PF2
ConVar tf_max_health_boost( "tf_max_health_boost", "50", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Max health factor that players can be boosted to by healers.", true, 1.0, false, 0 );
#else
ConVar tf_max_health_boost( "tf_max_health_boost", "1.5", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Max health factor that players can be boosted to by healers.", true, 1.0, false, 0 );
#endif
ConVar tf_invuln_time( "tf_invuln_time", "1.0", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Time it takes for invulnerability to wear off." );

#ifdef GAME_DLL
#ifdef PF2_DLL
// Reduce one point per second as it reduces too quickly with the release drain time, and generally lower health values
ConVar tf_boost_drain_time( "tf_boost_drain_time", "50.0", FCVAR_DEVELOPMENTONLY, "Time is takes for a full health boost to drain away from a player.", true, 0.1, false, 0 );
#else
ConVar tf_boost_drain_time( "tf_boost_drain_time", "15.0", FCVAR_DEVELOPMENTONLY, "Time is takes for a full health boost to drain away from a player.", true, 0.1, false, 0 );
#endif
ConVar tf_debug_bullets( "tf_debug_bullets", "0", FCVAR_DEVELOPMENTONLY, "Visualize bullet traces." );
ConVar tf_damage_events_track_for( "tf_damage_events_track_for", "30",  FCVAR_DEVELOPMENTONLY );
#endif

ConVar tf_useparticletracers( "tf_useparticletracers", "1", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Use particle tracers instead of old style ones." );
ConVar tf_spy_cloak_consume_rate( "tf_spy_cloak_consume_rate", "10.0", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "cloak to use per second while cloaked, from 100 max )" );	// 10 seconds of invis
ConVar tf_spy_cloak_regen_rate( "tf_spy_cloak_regen_rate", "3.3", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "cloak to regen per second, up to 100 max" );		// 30 seconds to full charge
ConVar tf_spy_cloak_no_attack_time( "tf_spy_cloak_no_attack_time", "2.0", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "time after uncloaking that the spy is prohibited from attacking" );
#ifdef PF2
ConVar tf_smoke_bomb_time("tf_smoke_bomb_time", "10.0", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED);
ConVar pf_grenades_regenerate_time("pf_grenades_regenerate_time", "8", FCVAR_NOTIFY | FCVAR_REPLICATED);
#ifdef _DEBUG
ConVar sv_showimpacts( "sv_showimpacts", "0", FCVAR_REPLICATED, "Shows client (red) and server (blue) bullet impact point (1=both, 2=client-only, 3=server-only)" );
ConVar sv_showplayerhitboxes( "sv_showplayerhitboxes", "0", FCVAR_REPLICATED, "Show lag compensated hitboxes for the specified player index whenever a player fires." );
#endif
#endif

#ifdef PF2
#if defined (GAME_DLL)
void FlagSpeed_ChangeCallback( IConVar *var, const char *pOldValue, float flOldValue )
{
	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

		if ( pPlayer )
		{
			if ( pPlayer->GetTeamNumber() > LAST_SHARED_TEAM )
			{
				pPlayer->TeamFortress_SetSpeed();
			}
		}
	}
}
#else
#define FlagSpeed_ChangeCallback nullptr
#endif
ConVar pf_force_flag_speed_penalty( "pf_force_flag_speed_penalty", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Speed penalty while carrying the flag.", FlagSpeed_ChangeCallback );
#endif
//ConVar tf_spy_stealth_blink_time( "tf_spy_stealth_blink_time", "0.3", FCVAR_DEVELOPMENTONLY, "time after being hit the spy blinks into view" );
//ConVar tf_spy_stealth_blink_scale( "tf_spy_stealth_blink_scale", "0.85", FCVAR_DEVELOPMENTONLY, "percentage visible scalar after being hit the spy blinks into view" );

ConVar tf_damage_disablespread( "tf_damage_disablespread", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Toggles the random damage spread applied to all player damage." );
ConVar tf_always_loser( "tf_always_loser", "0", FCVAR_REPLICATED | FCVAR_CHEAT, "Force loserstate to true." );
#ifdef PF2

#endif // pf2




#define TF_SPY_STEALTH_BLINKTIME   0.3f
#define TF_SPY_STEALTH_BLINKSCALE  0.85f

#define TF_PLAYER_CONDITION_CONTEXT	"TFPlayerConditionContext"

#define MAX_DAMAGE_EVENTS		128

const char *g_pszBDayGibs[22] = 
{
	"models/effects/bday_gib01.mdl",
	"models/effects/bday_gib02.mdl",
	"models/effects/bday_gib03.mdl",
	"models/effects/bday_gib04.mdl",
	"models/player/gibs/gibs_balloon.mdl",
	"models/player/gibs/gibs_burger.mdl",
	"models/player/gibs/gibs_boot.mdl",
	"models/player/gibs/gibs_bolt.mdl",
	"models/player/gibs/gibs_can.mdl",
	"models/player/gibs/gibs_clock.mdl",
	"models/player/gibs/gibs_fish.mdl",
	"models/player/gibs/gibs_gear1.mdl",
	"models/player/gibs/gibs_gear2.mdl",
	"models/player/gibs/gibs_gear3.mdl",
	"models/player/gibs/gibs_gear4.mdl",
	"models/player/gibs/gibs_gear5.mdl",
	"models/player/gibs/gibs_hubcap.mdl",
	"models/player/gibs/gibs_licenseplate.mdl",
	"models/player/gibs/gibs_spring1.mdl",
	"models/player/gibs/gibs_spring2.mdl",
	"models/player/gibs/gibs_teeth.mdl",
	"models/player/gibs/gibs_tire.mdl"
};

//=============================================================================
//
// Tables.
//

// Client specific.
#ifdef CLIENT_DLL

BEGIN_RECV_TABLE_NOBASE( CTFPlayerShared, DT_TFPlayerSharedLocal )
	RecvPropInt( RECVINFO( m_nDesiredDisguiseTeam ) ),
	RecvPropInt( RECVINFO( m_nDesiredDisguiseClass ) ),
	RecvPropTime( RECVINFO( m_flStealthNoAttackExpire ) ),
	RecvPropTime( RECVINFO( m_flStealthNextChangeTime ) ),
	RecvPropFloat( RECVINFO( m_flCloakMeter) ),
	RecvPropBool( RECVINFO( m_bBlockJump ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_bPlayerDominated ), RecvPropBool( RECVINFO( m_bPlayerDominated[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_bPlayerDominatingMe ), RecvPropBool( RECVINFO( m_bPlayerDominatingMe[0] ) ) ),
#ifdef PF2_CLIENT
	RecvPropFloat( RECVINFO( m_flSmokeBombExpire) ),
	RecvPropFloat(RECVINFO(m_flConcussionTime)),
	RecvPropArray3( RECVINFO_ARRAY( m_flSlowedTime ), RecvPropFloat( RECVINFO( m_flSlowedTime[0] ) ) ),
	RecvPropFloat( RECVINFO( m_flNextThrowTime ) ),
	RecvPropBool( RECVINFO( m_bUsedDetpack ) ),
	RecvPropInt( RECVINFO( m_iPrimed ) ),
	RecvPropFloat( RECVINFO( m_flGrenade1RegenTime) ),
	RecvPropFloat( RECVINFO( m_flGrenade2RegenTime) ),
	RecvPropTime( RECVINFO( m_flHallucinationTime) ),
#endif
END_RECV_TABLE()

BEGIN_RECV_TABLE_NOBASE( CTFPlayerShared, DT_TFPlayerShared )
	RecvPropInt( RECVINFO( m_nPlayerCond ) ),
	RecvPropInt( RECVINFO( m_bJumping) ),
	RecvPropInt( RECVINFO( m_nNumHealers ) ),
	RecvPropInt( RECVINFO( m_iCritMult) ),
	RecvPropInt( RECVINFO( m_bAirDash) ),
	RecvPropInt( RECVINFO( m_nAirDuckCount ) ),
	RecvPropInt( RECVINFO( m_nPlayerState ) ),
	RecvPropInt( RECVINFO( m_iDesiredPlayerClass ) ),
	RecvPropEHandle(RECVINFO(m_hCarriedObject)),
	RecvPropBool(RECVINFO(m_bCarryingObject)),
	RecvPropInt( RECVINFO( m_nTeamTeleporterUsed ) ),
	RecvPropBool( RECVINFO( m_bArenaSpectator ) ),
#ifdef PF2_CLIENT
	RecvPropInt( RECVINFO( m_nNumArmorers ) ),
#endif
	// Spy.
	RecvPropTime( RECVINFO( m_flInvisChangeCompleteTime ) ),
	RecvPropInt( RECVINFO( m_nDisguiseTeam ) ),
	RecvPropInt( RECVINFO( m_nDisguiseClass ) ),
	RecvPropInt( RECVINFO( m_iDisguiseTargetIndex ) ),
	RecvPropInt( RECVINFO( m_iDisguiseHealth ) ),
#ifdef PF2_CLIENT
	RecvPropInt( RECVINFO( m_iDisguiseArmor ) ),
#endif
	RecvPropInt( RECVINFO( m_iDisguiseWeaponSlot ) ),
	// Local Data.
	RecvPropDataTable( "tfsharedlocaldata", 0, 0, &REFERENCE_RECV_TABLE(DT_TFPlayerSharedLocal) ),
END_RECV_TABLE()

BEGIN_PREDICTION_DATA_NO_BASE( CTFPlayerShared )
	DEFINE_PRED_FIELD( m_nPlayerState, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nPlayerCond, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD_TOL(m_flCloakMeter, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, 0.5f), /*it works but probably isn't what I should be doing*/
	DEFINE_PRED_FIELD( m_bJumping, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bAirDash, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flInvisChangeCompleteTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nAirDuckCount, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bBlockJump, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
#ifdef PF2_CLIENT
	DEFINE_PRED_FIELD( m_flNextThrowTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bUsedDetpack, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_iPrimed, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flGrenade1RegenTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flGrenade2RegenTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
#endif
END_PREDICTION_DATA()

// Server specific.
#else

BEGIN_SEND_TABLE_NOBASE( CTFPlayerShared, DT_TFPlayerSharedLocal )
	SendPropInt( SENDINFO( m_nDesiredDisguiseTeam ), 3, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nDesiredDisguiseClass ), 4, SPROP_UNSIGNED ),
	SendPropTime( SENDINFO( m_flStealthNoAttackExpire ) ),
	SendPropTime( SENDINFO( m_flStealthNextChangeTime ) ),
	SendPropFloat( SENDINFO( m_flCloakMeter ), 0, SPROP_NOSCALE | SPROP_CHANGES_OFTEN, 0.0, 100.0 ),
	SendPropBool( SENDINFO( m_bBlockJump ) ), 
	SendPropArray3( SENDINFO_ARRAY3( m_bPlayerDominated ), SendPropBool( SENDINFO_ARRAY( m_bPlayerDominated ) ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_bPlayerDominatingMe ), SendPropBool( SENDINFO_ARRAY( m_bPlayerDominatingMe ) ) ),
#ifdef PF2_DLL
	SendPropTime( SENDINFO( m_flSmokeBombExpire ) ),
	SendPropTime( SENDINFO( m_flConcussionTime ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_flSlowedTime ), SendPropTime( SENDINFO_ARRAY( m_flSlowedTime ) ) ),
	SendPropTime( SENDINFO( m_flNextThrowTime ) ),
	SendPropBool( SENDINFO( m_bUsedDetpack ) ),
	SendPropInt( SENDINFO( m_iPrimed ) ),
	SendPropTime( SENDINFO( m_flGrenade1RegenTime ) ),
	SendPropTime( SENDINFO( m_flGrenade2RegenTime ) ),
	SendPropTime( SENDINFO( m_flHallucinationTime ) ),

#endif
END_SEND_TABLE()

BEGIN_SEND_TABLE_NOBASE( CTFPlayerShared, DT_TFPlayerShared )
	SendPropInt( SENDINFO( m_nPlayerCond ), TF_COND_LAST, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
	SendPropInt( SENDINFO( m_bJumping ), 1, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
	SendPropInt( SENDINFO( m_nNumHealers ), 5, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
	SendPropInt( SENDINFO( m_iCritMult ), 8, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
	SendPropInt( SENDINFO( m_bAirDash ), 1, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
	SendPropInt( SENDINFO( m_nAirDuckCount ) ),
	SendPropInt( SENDINFO( m_nPlayerState ), Q_log2( TF_STATE_COUNT )+1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iDesiredPlayerClass ), Q_log2( TF_CLASS_COUNT_ALL )+1, SPROP_UNSIGNED ),
	SendPropEHandle(SENDINFO(m_hCarriedObject)),
	SendPropBool(SENDINFO(m_bCarryingObject)),
	SendPropInt( SENDINFO( m_nTeamTeleporterUsed ), 3, SPROP_UNSIGNED ),
	SendPropBool( SENDINFO( m_bArenaSpectator ) ),
#ifdef PF2_DLL
	SendPropInt( SENDINFO( m_nNumArmorers), 5, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
#endif
	// Spy
	SendPropTime( SENDINFO( m_flInvisChangeCompleteTime ) ),
	SendPropInt( SENDINFO( m_nDisguiseTeam ), 3, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nDisguiseClass ), 4, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iDisguiseTargetIndex ), 7, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iDisguiseHealth ), 10 ),
#ifdef PF2_DLL
	SendPropInt( SENDINFO( m_iDisguiseArmor ), 11 ),
#endif
	SendPropInt( SENDINFO( m_iDisguiseWeaponSlot ), 12 ),
	// Local Data.
	SendPropDataTable( "tfsharedlocaldata", 0, &REFERENCE_SEND_TABLE( DT_TFPlayerSharedLocal ), SendProxy_SendLocalDataTable ),	
END_SEND_TABLE()

#endif


// --------------------------------------------------------------------------------------------------- //
// Shared CTFPlayer implementation.
// --------------------------------------------------------------------------------------------------- //

// --------------------------------------------------------------------------------------------------- //
// CTFPlayerShared implementation.
// --------------------------------------------------------------------------------------------------- //

CTFPlayerShared::CTFPlayerShared()
{
	m_nPlayerState.Set( TF_STATE_WELCOME );
	m_bJumping = false;
	m_bAirDash = false;
	m_flStealthNoAttackExpire = 0.0f;
	m_flStealthNextChangeTime = 0.0f;
	m_iCritMult = 0;
	m_flInvisibility = 0.0f;

	m_bArenaSpectator = false;

	m_nTeamTeleporterUsed = TEAM_UNASSIGNED;
#ifdef PF2
	m_flGrenade1RegenTime = 0.0f;
	m_flGrenade2RegenTime = 0.0f;

	m_bExecutedCritsForceVar = false;

	m_flConcussionTime = 0.0f;
	m_flHallucinationTime = 0.0f;
#endif
#ifdef CLIENT_DLL
	m_iDisguiseWeaponModelIndex = -1;
	m_pDisguiseWeaponInfo = NULL;
	m_pCritSound = NULL;
	m_pCritEffect = NULL;
#endif
}

void CTFPlayerShared::Init( CTFPlayer *pPlayer )
{
	m_pOuter = pPlayer;

	m_flNextBurningSound = 0;
	m_flInvisibility = 0.0f;
#ifdef PF2
	//m_iPrimed = PRIME_STATE_NONE;
	m_flConcussionTime = 0.0f;
	m_flHallucinationTime = 0.0f;
	m_flSlowedTime.Set( 0, 0.0f );
	m_flSlowedTime.Set( 1, 0.0f );
	m_flSlowedTime.Set( 2, 0.0f );
#endif
	m_nDesiredDisguiseTeam = TF_SPY_UNDEFINED;
	SetJumping( false );
}

//-----------------------------------------------------------------------------
// Purpose: Add a condition and duration
// duration of PERMANENT_CONDITION means infinite duration
//-----------------------------------------------------------------------------
void CTFPlayerShared::AddCond( int nCond, float flDuration /* = PERMANENT_CONDITION */ )
{
	Assert( nCond >= 0 && nCond < TF_COND_LAST );
	m_nPlayerCond |= (1<<nCond);
	m_flCondExpireTimeLeft[nCond] = flDuration;
	OnConditionAdded( nCond );
}

//-----------------------------------------------------------------------------
// Purpose: Forcibly remove a condition
//-----------------------------------------------------------------------------
void CTFPlayerShared::RemoveCond( int nCond )
{
	Assert( nCond >= 0 && nCond < TF_COND_LAST );

	m_nPlayerCond &= ~(1<<nCond);
	m_flCondExpireTimeLeft[nCond] = 0;

	OnConditionRemoved( nCond );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPlayerShared::InCond( int nCond )
{
	Assert( nCond >= 0 && nCond < TF_COND_LAST );

	return ( ( m_nPlayerCond & (1<<nCond) ) != 0 );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CTFPlayerShared::GetConditionDuration( int nCond )
{
	Assert( nCond >= 0 && nCond < TF_COND_LAST );

	if ( InCond( nCond ) )
	{
		return m_flCondExpireTimeLeft[nCond];
	}
	
	return 0.0f;
}

void CTFPlayerShared::DebugPrintConditions( void )
{
#ifndef CLIENT_DLL
	const char *szDll = "Server";
#else
	const char *szDll = "Client";
#endif

	Msg( "( %s ) Conditions for player ( %d )\n", szDll, m_pOuter->entindex() );

	int i;
	int iNumFound = 0;
	for ( i=0;i<TF_COND_LAST;i++ )
	{
		if ( m_nPlayerCond & (1<<i) )
		{
			if ( m_flCondExpireTimeLeft[i] == PERMANENT_CONDITION )
			{
				Msg( "( %s ) Condition %d - ( permanent cond )\n", szDll, i );
			}
			else
			{
				Msg( "( %s ) Condition %d - ( %.1f left )\n", szDll, i, m_flCondExpireTimeLeft[i] );
			}

			iNumFound++;
		}
	}

	if ( iNumFound == 0 )
	{
		Msg( "( %s ) No active conditions\n", szDll );
	}
}

#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnPreDataChanged( void )
{
	m_nOldConditions = m_nPlayerCond;
	m_nOldDisguiseClass = GetDisguiseClass();
	m_iOldDisguiseWeaponModelIndex = m_iDisguiseWeaponModelIndex;
	m_bWasCritBoosted = IsCritBoosted();
	m_nOldDisguiseWeaponSlot = m_iDisguiseWeaponSlot;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnDataChanged( void )
{
	// Update conditions from last network change
	if ( m_nOldConditions != m_nPlayerCond )
	{
		UpdateConditions();

		m_nOldConditions = m_nPlayerCond;
	}	

	if( m_bWasCritBoosted != IsCritBoosted() )
	{
		UpdateCritBoostEffect();
	}

	if ( m_nOldDisguiseClass != GetDisguiseClass() || m_nOldDisguiseWeaponSlot != m_iDisguiseWeaponSlot )
	{
		OnDisguiseChanged();
	}

	if ( m_iDisguiseWeaponModelIndex != m_iOldDisguiseWeaponModelIndex )
	{
		C_BaseCombatWeapon *pWeapon = m_pOuter->GetActiveWeapon();

		if( pWeapon )
		{
			pWeapon->SetModelIndex( pWeapon->GetWorldModelIndex() );
		}
	}

	//check for local player too since the loser function doesn't do that
	//-Nbc66
	if ( m_pOuter->IsLocalPlayer() && IsLoser() )
	{
		C_BaseCombatWeapon *pWeapon = GetActiveWeapon();
		if ( pWeapon && !pWeapon->IsEffectActive( EF_NODRAW ) )
		{
			pWeapon->SetWeaponVisible( false );
			pWeapon->UpdateVisibility();
		}

		if( InCond( TF_COND_STEALTHED ) )
		{
			RemoveCond( TF_COND_STEALTHED );
		}
	}

}

//-----------------------------------------------------------------------------
// Purpose: check the newly networked conditions for changes
//-----------------------------------------------------------------------------
void CTFPlayerShared::UpdateConditions( void )
{
	int nCondChanged = m_nPlayerCond ^ m_nOldConditions;
	int nCondAdded = nCondChanged & m_nPlayerCond;
	int nCondRemoved = nCondChanged & m_nOldConditions;

	int i;
	for ( i=0;i<TF_COND_LAST;i++ )
	{
		if ( nCondAdded & (1<<i) )
		{
			OnConditionAdded( i );
		}
		else if ( nCondRemoved & (1<<i) )
		{
			OnConditionRemoved( i );
		}
	}
}

#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: Remove any conditions affecting players
//-----------------------------------------------------------------------------
void CTFPlayerShared::RemoveAllCond( CTFPlayer *pPlayer )
{
	int i;
	for ( i=0;i<TF_COND_LAST;i++ )
	{
		if ( m_nPlayerCond & (1<<i) )
		{
			RemoveCond( i );
		}
	}

	// Now remove all the rest
	m_nPlayerCond = 0;
}


//-----------------------------------------------------------------------------
// Purpose: Called on both client and server. Server when we add the bit,
// and client when it recieves the new cond bits and finds one added
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnConditionAdded( int nCond )
{
	switch( nCond )
	{
	case TF_COND_HEALTH_BUFF:
#ifdef GAME_DLL
		m_flHealFraction = 0;
		m_flDisguiseHealFraction = 0;
#endif
		break;

	case TF_COND_STEALTHED:
		OnAddStealthed();
		break;
#ifdef PF2
	case TF_COND_AIMING:
#ifdef GAME_DLL
		m_pOuter->TeamFortress_SetSpeed();
#endif // GAME_DLL
		break;
#ifdef CLIENT_DLL
	case TF_COND_SELECTED_TO_TELEPORT:
		if (m_pSparkleProp)
		{
			m_pOuter->ParticleProp()->StopEmissionAndDestroyImmediately(m_pSparkleProp);
			m_pSparkleProp = NULL;
		}
		switch (m_pOuter->GetTeamNumber())
		{
		case TF_TEAM_RED:
			m_pSparkleProp = m_pOuter->ParticleProp()->Create("player_sparkles_red", PATTACH_ABSORIGIN);
			break;
		case TF_TEAM_BLUE:
			m_pSparkleProp = m_pOuter->ParticleProp()->Create("player_sparkles_blue", PATTACH_ABSORIGIN);
			break;
		default:
			break;

		}
		if (m_pSparkleProp)
			m_pSparkleProp->SetControlPointEntity(0, m_pOuter);
		break;

#endif
#endif

	case TF_COND_INVULNERABLE:
		OnAddInvulnerable();
		break;

	case TF_COND_TELEPORTED:
		OnAddTeleported();
		break;

	case TF_COND_BURNING:
		OnAddBurning();
		break;

	case TF_COND_DISGUISING:
		OnAddDisguising();
		break;

	case TF_COND_DISGUISED:
		OnAddDisguised();
		break;

	case TF_COND_TAUNTING:
		{
			CTFWeaponBase *pWpn = m_pOuter->GetActiveTFWeapon();
			if ( pWpn )
			{
				// cancel any reload in progress.
				pWpn->AbortReload();
			}
		}
		break;

#ifdef PF2
	case TF_COND_ARMOR_BUFF:
#ifdef GAME_DLL
		m_flArmorFraction = 0;
#endif
		break;

	case TF_COND_SMOKE_BOMB:
		OnAddSmokeBomb();
		break;

	case TF_COND_TRANQUILIZED:
		m_flSlowedTime.Set( 0, gpGlobals->curtime + TF_TRANQUILLIZED_DURATION );
		break;
	case TF_COND_LEG_DAMAGED:
		m_flSlowedTime.Set( 1, gpGlobals->curtime + TF_BROKEN_LEG_DURATION );
		break;
	case TF_COND_LEG_DAMAGED_GUNSHOT:
		m_flSlowedTime.Set( 2, gpGlobals->curtime + TF_BROKEN_LEG_GUNSHOT_DURATION );
		break;
	case TF_COND_BUILDING_DETPACK:
		m_pOuter->TeamFortress_SetSpeed();
		break;

	case TF_COND_DIZZY:
		m_flConcussionTime = gpGlobals->curtime + TF_CONCUSSION_DURATION;
		break;

	case TF_COND_HALLUCINATING:
#ifdef CLIENT_DLL
		if ( m_pOuter->IsLocalPlayer() )
		{
			g_pScreenSpaceEffects->EnableScreenSpaceEffect( "pf_drugged_effect" );
		}	
#endif
		break;
	case TF_COND_CRITBOOSTED:
		OnAddCrits();
		break;
#endif // PF2

	default:
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called on both client and server. Server when we remove the bit,
// and client when it recieves the new cond bits and finds one removed
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnConditionRemoved( int nCond )
{
	switch( nCond )
	{
	case TF_COND_ZOOMED:
		OnRemoveZoomed();
		break;

	case TF_COND_BURNING:
		OnRemoveBurning();
		break;

	case TF_COND_HEALTH_BUFF:
#ifdef GAME_DLL
		m_flHealFraction = 0;
		m_flDisguiseHealFraction = 0;
#endif
		break;

	case TF_COND_STEALTHED:
		OnRemoveStealthed();
		break;

#ifdef PF2
	case TF_COND_AIMING:
#ifdef GAME_DLL
		m_pOuter->TeamFortress_SetSpeed();
#endif // GAME_DLL
		break;

#ifdef CLIENT_DLL
	case TF_COND_SELECTED_TO_TELEPORT:
		if (m_pSparkleProp)
		{
			m_pOuter->ParticleProp()->StopEmission(m_pSparkleProp);
			m_pSparkleProp = NULL;
		}
		break;
#endif
#endif

	case TF_COND_DISGUISED:
		OnRemoveDisguised();
		break;

	case TF_COND_DISGUISING:
		OnRemoveDisguising();
		break;

	case TF_COND_INVULNERABLE:
		OnRemoveInvulnerable();
		break;

	case TF_COND_TELEPORTED:
		OnRemoveTeleported();
		break;

#ifdef PF2
	case TF_COND_ARMOR_BUFF:
#ifdef GAME_DLL
		m_flArmorFraction = 0;
#endif
		break;
	case TF_COND_SMOKE_BOMB:
		OnRemoveSmokeBomb();
		break;

	case TF_COND_BUILDING_DETPACK:
		m_pOuter->TeamFortress_SetSpeed();
		break;
	case TF_COND_HALLUCINATING:
#ifdef CLIENT_DLL
		if (m_pOuter->IsLocalPlayer())
			view->SetScreenOverlayMaterial(NULL);
#endif
		break;
	case TF_COND_CRITBOOSTED:
		OnRemoveCrits();
		break;
#endif // PF2

	default:
		break;
	}
}

int CTFPlayerShared::GetMaxBuffedHealth( void )
{
#ifdef PF2
	// Same as QWTF/TFC
	return m_pOuter->GetMaxHealth() + tf_max_health_boost.GetInt();
#else
	float flBoostMax = m_pOuter->GetMaxHealth() * tf_max_health_boost.GetFloat();

	int iRoundDown = floor( flBoostMax / 5 );
	iRoundDown = iRoundDown * 5;

	return iRoundDown;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Runs SERVER SIDE only Condition Think
// If a player needs something to be updated no matter what do it here (invul, etc).
//-----------------------------------------------------------------------------
void CTFPlayerShared::ConditionGameRulesThink( void )
{
#ifdef GAME_DLL
	if ( m_flNextCritUpdate < gpGlobals->curtime )
	{
		UpdateCritMult();
		m_flNextCritUpdate = gpGlobals->curtime + 0.5;
	}

	int i;
	for ( i=0;i<TF_COND_LAST;i++ )
	{
		if ( m_nPlayerCond & (1<<i) )
		{
			// Ignore permanent conditions
			if ( m_flCondExpireTimeLeft[i] != PERMANENT_CONDITION )
			{
				float flReduction = gpGlobals->frametime;

				// If we're being healed, we reduce bad conditions faster
				if ( i > TF_COND_HEALTH_BUFF && m_aHealers.Count() > 0 )
				{
					flReduction += (m_aHealers.Count() * flReduction * 4);
				}

				m_flCondExpireTimeLeft[i] = max( m_flCondExpireTimeLeft[i] - flReduction, 0.f );

				if ( m_flCondExpireTimeLeft[i] == 0 )
				{
					RemoveCond( i );
				}
			}
		}
	}

	// Our health will only decay ( from being medic buffed ) if we are not being healed by a medic
	// Dispensers can give us the TF_COND_HEALTH_BUFF, but will not maintain or give us health above 100%s
	bool bDecayHealth = true;

	// If we're being healed, heal ourselves
	if ( InCond( TF_COND_HEALTH_BUFF ) )
	{
		// Heal faster if we haven't been in combat for a while
		float flTimeSinceDamage = gpGlobals->curtime - m_pOuter->GetLastDamageTime();
		float flScale = RemapValClamped( flTimeSinceDamage, 10, 15, 1.0, 3.0 );

		bool bHasFullHealth = m_pOuter->GetHealth() >= m_pOuter->GetMaxHealth();

		float fTotalHealAmount = 0.0f;
		for ( int i = 0; i < m_aHealers.Count(); i++ )
		{
			//Assert( m_aHealers[i].pPlayer );

			// Dispensers don't heal above 100%
			if ( bHasFullHealth && m_aHealers[i].bDispenserHeal )
			{
				continue;
			}

			// Being healed by a medigun, don't decay our health
			bDecayHealth = false;

			// Dispensers heal at a constant rate
			if ( m_aHealers[i].bDispenserHeal )
			{
				// Dispensers heal at a slower rate, but ignore flScale
				m_flHealFraction += gpGlobals->frametime * m_aHealers[i].flAmount;
			}
			else	// player heals are affected by the last damage time
			{
				m_flHealFraction += gpGlobals->frametime * m_aHealers[i].flAmount * flScale;
			}

			fTotalHealAmount += m_aHealers[i].flAmount;
		}

		int nHealthToAdd = (int)m_flHealFraction;
		if ( nHealthToAdd > 0 )
		{
			m_flHealFraction -= nHealthToAdd;

			int iBoostMax = GetMaxBuffedHealth();

			if ( InCond( TF_COND_DISGUISED ) )
			{
				// Separate cap for disguised health
				int nFakeHealthToAdd = clamp( nHealthToAdd, 0, iBoostMax - m_iDisguiseHealth );
				m_iDisguiseHealth += nFakeHealthToAdd;
			}

			// Cap it to the max we'll boost a player's health
			nHealthToAdd = clamp( nHealthToAdd, 0, iBoostMax - m_pOuter->GetHealth() );


			m_pOuter->TakeHealth( nHealthToAdd, DMG_IGNORE_MAXHEALTH );

			// split up total healing based on the amount each healer contributes
			for ( int i = 0; i < m_aHealers.Count(); i++ )
			{
				//Assert( m_aHealers[i].pPlayer );
				if ( m_aHealers[i].pPlayer.IsValid() )
				{
					CTFPlayer *pPlayer = static_cast<CTFPlayer *>( static_cast<CBaseEntity *>( m_aHealers[i].pPlayer ) );
					if ( IsAlly( pPlayer ) )
					{
						CTF_GameStats.Event_PlayerHealedOther( pPlayer, nHealthToAdd * ( m_aHealers[i].flAmount / fTotalHealAmount ) );
					}
					else
					{
						CTF_GameStats.Event_PlayerLeachedHealth( m_pOuter, m_aHealers[i].bDispenserHeal, nHealthToAdd * ( m_aHealers[i].flAmount / fTotalHealAmount ) );
					}
					// Store off how much this guy healed.
					m_aHealers[ i ].iRecentAmount += nHealthToAdd;

					// Show how much this player healed every second.
					if ( gpGlobals->curtime >= m_aHealers[ i ].flNextNotifyTime )
					{
						if ( m_aHealers[ i ].iRecentAmount > 0 )
						{
							IGameEvent* event = gameeventmanager->CreateEvent( "player_healed" );
							if ( event )
							{
								event->SetInt( "priority", 1 );
								event->SetInt( "patient", m_pOuter->GetUserID() );
								event->SetInt( "healer", pPlayer->GetUserID() );
								event->SetInt( "amount", m_aHealers[ i ].iRecentAmount );

								gameeventmanager->FireEvent( event );
							}
						}

						m_aHealers[ i ].iRecentAmount = 0;
						m_aHealers[ i ].flNextNotifyTime = gpGlobals->curtime + 1.0f;
					}

				}
			}
		}

		if ( InCond( TF_COND_BURNING ) )
		{
			// Reduce the duration of this burn 
			float flReduction = 2;	 // ( flReduction + 1 ) x faster reduction
			m_flFlameRemoveTime -= flReduction * gpGlobals->frametime;
		}
#ifdef PF2_DLL
		if ( InCond( TF_COND_HALLUCINATING ) )
		{
			// Reduce the duration of this burn 
			float flReduction = 2;	 // ( flReduction + 1 ) x faster reduction
			m_flHallucinationTime -= flReduction * gpGlobals->frametime;
		}

		if( InCond( TF_COND_TRANQUILIZED ) )
		{
			m_flSlowedTime.Set( 0, m_flSlowedTime.Get( 0 ) - (gpGlobals->frametime * 2) );
		}
		if ( InCond( TF_COND_LEG_DAMAGED ) )
		{
			m_flSlowedTime.Set( 1, m_flSlowedTime.Get( 1 ) - (gpGlobals->frametime * 2) );
		}
		if( InCond( TF_COND_LEG_DAMAGED_GUNSHOT ) )
		{
			m_flSlowedTime.Set( 2, m_flSlowedTime.Get( 2 ) - (gpGlobals->frametime * 2) );
		}
#endif
	}

	if ( bDecayHealth )
	{
		// If we're not being buffed, our health drains back to our max
		if ( m_pOuter->GetHealth() > m_pOuter->GetMaxHealth() )
		{
			float flBoostMaxAmount = GetMaxBuffedHealth() - m_pOuter->GetMaxHealth();
			m_flHealFraction += (gpGlobals->frametime * (flBoostMaxAmount / tf_boost_drain_time.GetFloat()));

			int nHealthToDrain = (int)m_flHealFraction;
			if ( nHealthToDrain > 0 )
			{
				m_flHealFraction -= nHealthToDrain;

				// Manually subtract the health so we don't generate pain sounds / etc
				m_pOuter->m_iHealth -= nHealthToDrain;
			}
		}

		if ( InCond( TF_COND_DISGUISED ) && m_iDisguiseHealth > m_pOuter->GetMaxHealth() )
		{
			float flBoostMaxAmount = GetMaxBuffedHealth() - m_pOuter->GetMaxHealth();
			m_flDisguiseHealFraction += (gpGlobals->frametime * (flBoostMaxAmount / tf_boost_drain_time.GetFloat()));

			int nHealthToDrain = (int)m_flDisguiseHealFraction;
			if ( nHealthToDrain > 0 )
			{
				m_flDisguiseHealFraction -= nHealthToDrain;

				// Reduce our fake disguised health by roughly the same amount
				m_iDisguiseHealth -= nHealthToDrain;
			}
		}
	}

	// Taunt
	if ( InCond( TF_COND_TAUNTING ) )
	{
		if ( gpGlobals->curtime > m_flTauntRemoveTime )
		{
			m_pOuter->ResetTauntHandle();

			//m_pOuter->SnapEyeAngles( m_pOuter->m_angTauntCamera );
			//m_pOuter->SetAbsAngles( m_pOuter->m_angTauntCamera );
			//m_pOuter->SetLocalAngles( m_pOuter->m_angTauntCamera );

			RemoveCond( TF_COND_TAUNTING );
		}
	}

	if ( InCond( TF_COND_BURNING ) && ( m_pOuter->m_flPowerPlayTime < gpGlobals->curtime ) )
	{
		// If we're underwater, put the fire out
		if ( gpGlobals->curtime > m_flFlameRemoveTime || m_pOuter->GetWaterLevel() >= WL_Waist )
		{
			RemoveCond( TF_COND_BURNING );
#ifdef PF2_DLL
			RemoveCond( TF_COND_NAPALM_BURNING );
#endif
		}
		else if ( ( gpGlobals->curtime >= m_flFlameBurnTime ) && ( TF_CLASS_PYRO != m_pOuter->GetPlayerClass()->GetClassIndex() ) )
		{
#ifdef PF2_DLL
			// Very nice hack to have the napalm kill icon work
			bool bNapalm = InCond( TF_COND_NAPALM_BURNING );
			// Burn the player (if not pyro, who does not take persistent burning damage)
			CTakeDamageInfo info( m_hBurnAttacker, m_hBurnAttacker, TF_BURNING_DMG, DMG_BURN | DMG_PREVENT_PHYSICS_FORCE, bNapalm ? TF_DMG_CUSTOM_NAPALM_BURNING : TF_DMG_CUSTOM_BURNING );
#else
			CTakeDamageInfo info( m_hBurnAttacker, m_hBurnAttacker, TF_BURNING_DMG, DMG_BURN | DMG_PREVENT_PHYSICS_FORCE, TF_DMG_CUSTOM_BURNING );
#endif
			m_pOuter->TakeDamage( info );
			m_flFlameBurnTime = gpGlobals->curtime + TF_BURNING_FREQUENCY;
		}

		if ( m_flNextBurningSound < gpGlobals->curtime )
		{
			m_pOuter->SpeakConceptIfAllowed( MP_CONCEPT_ONFIRE );
			m_flNextBurningSound = gpGlobals->curtime + 2.5;
		}
	}

	if ( InCond( TF_COND_DISGUISING ) )
	{
		if ( gpGlobals->curtime > m_flDisguiseCompleteTime )
		{
			CompleteDisguise();
		}
	}

	// stops medigun drain hack
	CWeaponMedigun *pWeapon = ( CWeaponMedigun* )m_pOuter->Weapon_OwnsThisID( TF_WEAPON_MEDIGUN );
	if ( pWeapon && pWeapon->IsReleasingCharge() )
	{
		pWeapon->DrainCharge();
	}

	if ( InCond( TF_COND_INVULNERABLE )  )
	{
		bool bRemoveInvul = false;

		if ( ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN ) && ( TFGameRules()->GetWinningTeam() != m_pOuter->GetTeamNumber() ) )
		{
			bRemoveInvul = true;
		}
		
		if ( m_flInvulnerableOffTime )
		{
			if ( gpGlobals->curtime > m_flInvulnerableOffTime )
			{
				bRemoveInvul = true;
			}
		}

		if ( bRemoveInvul == true )
		{
			m_flInvulnerableOffTime = 0;
			RemoveCond( TF_COND_INVULNERABLE_WEARINGOFF );
			RemoveCond( TF_COND_INVULNERABLE );
		}
	}

	if ( InCond( TF_COND_STEALTHED_BLINK ) )
	{
		if ( TF_SPY_STEALTH_BLINKTIME/*tf_spy_stealth_blink_time.GetFloat()*/ < ( gpGlobals->curtime - m_flLastStealthExposeTime ) )
		{
			RemoveCond( TF_COND_STEALTHED_BLINK );
		}
	}

#ifdef PF2_DLL
	if ( InCond( TF_COND_ARMOR_BUFF ) )
	{

		bool bHasFullArmor = m_pOuter->ArmorValue() >= m_pOuter->GetPlayerClass()->GetMaxArmor();
		float fTotalArmorAmount = 0.0f;
		int nArmorToAdd = (int)m_flArmorFraction;
		for (int i = 0; i < m_aArmorers.Count(); i++)
		{
			//Assert( m_aArmorers[i].pPlayer );
			
			// NO OVERARMORING!!!
			if ( bHasFullArmor )
			{
				continue;
			}

			// Heal armor at a constant rate always; not the same as health
			m_flArmorFraction += gpGlobals->frametime * m_aArmorers[i].flAmountArmor;

			fTotalArmorAmount += m_aArmorers[i].flAmountArmor;
		}

		//for (int i = 0; i < m_aArmorers.Count(); i++)
		//{
		//	Assert( m_aArmorers[i].pPlayer );
		//}

		if ( nArmorToAdd > 0 )
		{
			m_flArmorFraction -= nArmorToAdd;
			int iMaxArmor = m_pOuter->GetPlayerClass()->GetMaxArmor();

			if ( InCond( TF_COND_DISGUISED ) )
			{
				// Separate cap for disguised armor
				int nFakeArmorToAdd = clamp( nArmorToAdd, 0, iMaxArmor - m_iDisguiseArmor );
				m_iDisguiseArmor += nFakeArmorToAdd;
			}
			// Add the armor value until it's full!
			nArmorToAdd = clamp( nArmorToAdd, 0, iMaxArmor );
			m_pOuter->IncrementArmorValue( nArmorToAdd, iMaxArmor );
			// split up total healing based on the amount each healer contributes
			for ( int i = 0; i < m_aArmorers.Count(); i++ )
			{
				//Assert( m_aArmorers[i].pPlayer );
				if ( m_aArmorers[i].pPlayer.IsValid() )
				{
					CTFPlayer *pPlayer = static_cast<CTFPlayer *>( static_cast<CBaseEntity *>( m_aArmorers[i].pPlayer ) );
					if ( IsAlly( pPlayer ) )
					{
						CTF_GameStats.Event_PlayerArmorRepairedOther( pPlayer, nArmorToAdd * ( m_aArmorers[i].flAmountArmor / fTotalArmorAmount ) );
					}
					else
					{
					//	CTF_GameStats.Event_PlayerLeachedHealth( m_pOuter, m_aArmorers[i].bDispenserHeal, nArmorToAdd * ( m_aHealers[i].flAmountArmor / fTotalArmorAmount ) );
					}
					// Store off how much this guy healed.
					m_aArmorers[ i ].iRecentArmorAmount += nArmorToAdd;

					// Show how much this player healed every second.
					if ( gpGlobals->curtime >= m_aArmorers[ i ].flNextNotifyArmorTime )
					{
						if ( m_aArmorers[ i ].iRecentArmorAmount > 0 )
						{
							IGameEvent* event = gameeventmanager->CreateEvent( "player_repaired" );
							if ( event )
							{
								event->SetInt( "patient", m_pOuter->GetUserID() );
								event->SetInt( "repairer", GetBuildableId( "OBJ_DISPENSER" ) );
								event->SetInt( "amount", m_aArmorers[ i ].iRecentArmorAmount );

								gameeventmanager->FireEvent( event );
							}
						}

						m_aArmorers[ i ].iRecentArmorAmount = 0;
						m_aArmorers[ i ].flNextNotifyArmorTime = gpGlobals->curtime + 1.0f;
					}

				}
			}
		}
		
	}

	if (InCond(TF_COND_INFECTED))
	{
		if (GetNumHealers() > 0)
		{
			RemoveCond(TF_COND_INFECTED);
		}
		else if ( (gpGlobals->curtime >= m_flInfectionTime) && ( TF_CLASS_MEDIC != m_pOuter->GetPlayerClass()->GetClassIndex() ) )
		{
			if ( m_hInfectionAttacker.Get() )
			{
				CTakeDamageInfo info( m_hInfectionAttacker, m_hInfectionAttacker, TF_INFECTION_DMG, DMG_PREVENT_PHYSICS_FORCE | DMG_INFECTION, TF_DMG_CUSTOM_INFECTION );
				m_pOuter->TakeDamage( info );
			}
			else
			{
				CTakeDamageInfo info( m_pOuter, m_pOuter, TF_INFECTION_DMG, DMG_PREVENT_PHYSICS_FORCE | DMG_INFECTION, TF_DMG_CUSTOM_INFECTION );
				m_pOuter->TakeDamage( info );
			}
			m_flInfectionTime = gpGlobals->curtime + TF_INFECTION_FREQUENCY;
		}
	}

	if ( InCond( TF_COND_HALLUCINATING ) )
	{
		m_flHallucinationTime = gpGlobals->curtime + GetConditionDuration( TF_COND_HALLUCINATING );

		// If we're underwater, put the fire out
		if ( gpGlobals->curtime > m_flHallucinationTime || m_pOuter->GetWaterLevel() >= WL_Waist )
		{
			RemoveCond( TF_COND_HALLUCINATING );
		}
	}

	if( InCond( TF_COND_ARMOR_MELTING ) )
	{
		if( gpGlobals->curtime > m_flNextArmorMelt )
		{
			int currentArmor = m_pOuter->ArmorValue();
			if( currentArmor > 0 )
			{
				m_pOuter->SetArmorValue( currentArmor - 1 );
			}
			m_flNextArmorMelt = gpGlobals->curtime + 0.1;
		}
	}

	if( InCond( TF_COND_TRANQUILIZED ) )
	{
		if( m_flSlowedTime.Get( 0 ) < gpGlobals->curtime )
			RemoveCond( TF_COND_TRANQUILIZED );
	}

	if( InCond( TF_COND_LEG_DAMAGED ) )
	{
		if( m_flSlowedTime.Get( 1 ) < gpGlobals->curtime )
			RemoveCond( TF_COND_LEG_DAMAGED );
	}

	if( InCond( TF_COND_LEG_DAMAGED_GUNSHOT ) )
	{
		if( m_flSlowedTime.Get( 2 ) < gpGlobals->curtime )
			RemoveCond( TF_COND_LEG_DAMAGED_GUNSHOT );
	}
	
#endif // PF2_DLL
#endif // GAME_DLL
}

//-----------------------------------------------------------------------------
// Purpose: Do CLIENT/SERVER SHARED condition thinks.
//-----------------------------------------------------------------------------
void CTFPlayerShared::ConditionThink( void )
{
	bool bIsLocalPlayer = false;
#ifdef CLIENT_DLL
	bIsLocalPlayer = m_pOuter->IsLocalPlayer();
#else
	bIsLocalPlayer = true;
#endif

	if( !pf_force_crits.GetBool() && m_bExecutedCritsForceVar && bIsLocalPlayer )
	{
		RemoveCond( TF_COND_CRITBOOSTED );
		m_bExecutedCritsForceVar = false;
	}

	if( pf_force_crits.GetBool() && !m_bExecutedCritsForceVar && bIsLocalPlayer )
	{
		AddCond( TF_COND_CRITBOOSTED );
		m_bExecutedCritsForceVar = true;
	}

	

	if( m_pOuter->IsPlayerClass( TF_CLASS_SPY ) && bIsLocalPlayer )
	{
		if( InCond( TF_COND_STEALTHED ) )
		{
			m_flCloakMeter -= gpGlobals->frametime * tf_spy_cloak_consume_rate.GetFloat();

			if( m_flCloakMeter <= 0.0f )
			{
				FadeInvis( tf_spy_invis_unstealth_time.GetFloat() );
			}
		}
		else
		{
			m_flCloakMeter += gpGlobals->frametime * tf_spy_cloak_regen_rate.GetFloat();

			if( m_flCloakMeter >= 100.0f )
			{
				m_flCloakMeter = 100.0f;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveZoomed( void )
{
#ifdef GAME_DLL
	m_pOuter->SetFOV( m_pOuter, 0, 0.1f );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddDisguising( void )
{
#ifdef CLIENT_DLL
	if ( m_pOuter->m_pDisguisingEffect )
	{
//		m_pOuter->ParticleProp()->StopEmission( m_pOuter->m_pDisguisingEffect );
	}

	if ( !m_pOuter->IsLocalPlayer() && ( !InCond( TF_COND_STEALTHED ) || !m_pOuter->IsEnemyPlayer() ) )
	{
		const char *pEffectName = ( m_pOuter->GetTeamNumber() == TF_TEAM_RED ) ? "spy_start_disguise_red" : "spy_start_disguise_blue";
		m_pOuter->m_pDisguisingEffect = m_pOuter->ParticleProp()->Create( pEffectName, PATTACH_ABSORIGIN_FOLLOW );
		m_pOuter->m_flDisguiseEffectStartTime = gpGlobals->curtime;
	}

	m_pOuter->EmitSound( "Player.Spy_Disguise" );

#endif
}

//-----------------------------------------------------------------------------
// Purpose: set up effects for when player finished disguising
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddDisguised( void )
{
#ifdef CLIENT_DLL
	if ( m_pOuter->m_pDisguisingEffect )
	{
		// turn off disguising particles
//		m_pOuter->ParticleProp()->StopEmission( m_pOuter->m_pDisguisingEffect );
		m_pOuter->m_pDisguisingEffect = NULL;
	}
	m_pOuter->m_flDisguiseEndEffectStartTime = gpGlobals->curtime;
#endif
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: start, end, and changing disguise classes
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnDisguiseChanged( void )
{
	// recalc disguise model index
	RecalcDisguiseWeapon();
#ifdef PF2_CLIENT
	UpdateCritBoostEffect();
#endif
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddInvulnerable( void )
{
#ifdef CLIENT_DLL

	if ( m_pOuter->IsLocalPlayer() )
	{
		char *pEffectName = NULL;

		switch( m_pOuter->GetTeamNumber() )
		{
		case TF_TEAM_BLUE:
			pEffectName = "effects/invuln_overlay_blue";
			break;
		case TF_TEAM_RED:
			pEffectName =  "effects/invuln_overlay_red";
			break;
		default:
			pEffectName = "effects/invuln_overlay_blue";
			break;
		}

		IMaterial *pMaterial = materials->FindMaterial( pEffectName, TEXTURE_GROUP_CLIENT_EFFECTS, false );
		if ( !IsErrorMaterial( pMaterial ) )
		{
			view->SetScreenOverlayMaterial( pMaterial );
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveInvulnerable( void )
{
#ifdef CLIENT_DLL
	if ( m_pOuter->IsLocalPlayer() )
	{
		view->SetScreenOverlayMaterial( NULL );
	}
#endif
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayerShared::ShouldShowRecentlyTeleported( void )
{
	if( m_pOuter->IsPlayerClass( TF_CLASS_SPY ) )
	{
		if( InCond( TF_COND_STEALTHED ) )
			return false;

		if( InCond( TF_COND_DISGUISED ) && (m_pOuter->IsLocalPlayer() || m_pOuter->IsEnemyPlayer()) )
		{
			if( GetDisguiseTeam() != m_nTeamTeleporterUsed )
				return false;
		}
		else if( m_pOuter->GetTeamNumber() != m_nTeamTeleporterUsed )
			return false;
	}

	return ( InCond( TF_COND_TELEPORTED ) );
}
#endif


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddTeleported( void )
{
#ifdef CLIENT_DLL
	m_pOuter->UpdateRecentlyTeleportedEffect();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveTeleported( void )
{
#ifdef CLIENT_DLL
	m_pOuter->UpdateRecentlyTeleportedEffect();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#ifdef PF2
void CTFPlayerShared::Burn( CTFPlayer *pAttacker, bool bNapalm /*= false*/ )
#else
void CTFPlayerShared::Burn( CTFPlayer *pAttacker )
#endif
{
#ifdef GAME_DLL
	// Don't bother igniting players who have just been killed by the fire damage.
	if ( !m_pOuter->IsAlive() )
		return;

	// pyros don't burn persistently or take persistent burning damage, but we show brief burn effect so attacker can tell they hit
	bool bVictimIsPyro = ( TF_CLASS_PYRO ==  m_pOuter->GetPlayerClass()->GetClassIndex() );

#ifdef PF2_DLL
	// "sub" condition that tells us if the burner originally comes from a napalm grenade
	if ( bNapalm )
	{
		if ( !InCond( TF_COND_NAPALM_BURNING ) )
		{
			AddCond( TF_COND_NAPALM_BURNING );
			if ( pAttacker && !bVictimIsPyro )
			{
				pAttacker->OnBurnOther( m_pOuter );
			}
		}
	}
	// if it's not from napalm, remove the condition as the new damage is coming from the flamethrower
	else if ( InCond( TF_COND_NAPALM_BURNING ) )
	{
		RemoveCond( TF_COND_NAPALM_BURNING );
	}
#endif

	if ( !InCond( TF_COND_BURNING ) )
	{
		// Start burning
		AddCond( TF_COND_BURNING );
		m_flFlameBurnTime = gpGlobals->curtime;	//asap
		// let the attacker know he burned me
#ifndef PF2_DLL
		if ( pAttacker && !bVictimIsPyro )
		{
			pAttacker->OnBurnOther( m_pOuter );
		}
#endif
	}
	
	float flFlameLife = bVictimIsPyro ? TF_BURNING_FLAME_LIFE_PYRO : TF_BURNING_FLAME_LIFE;
	m_flFlameRemoveTime = gpGlobals->curtime + flFlameLife;
	m_hBurnAttacker = pAttacker;

#endif
}

#ifdef PF2
void CTFPlayerShared::Infect(CTFPlayer* pPlayer)
{
#ifdef GAME_DLL
	if (!m_pOuter->IsAlive())
		return;

	// medics don't get infected
	if (m_pOuter->IsPlayerClass(TF_CLASS_MEDIC))
		return;

	if (InCond( TF_COND_INVULNERABLE ))
		return;

	if (!InCond(TF_COND_INFECTED))
	{
		AddCond(TF_COND_INFECTED, 12.0f);
		m_flInfectionTime = gpGlobals->curtime;
	}
	m_hInfectionAttacker = pPlayer;
#endif
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveBurning( void )
{
#ifdef CLIENT_DLL
	m_pOuter->StopBurningSound();

	if ( m_pOuter->m_pBurningEffect )
	{
		m_pOuter->ParticleProp()->StopEmission( m_pOuter->m_pBurningEffect );
		m_pOuter->m_pBurningEffect = NULL;
	}

	if ( m_pOuter->IsLocalPlayer() )
	{
		view->SetScreenOverlayMaterial( NULL );
	}

	m_pOuter->m_flBurnEffectStartTime = 0;
	m_pOuter->m_flBurnEffectEndTime = 0;
#else
	m_hBurnAttacker = NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddStealthed( void )
{
#ifdef CLIENT_DLL
	m_pOuter->EmitSound( "Player.Spy_Cloak" );
#ifdef PF2_CLIENT
	UpdateCritBoostEffect();
#endif
	m_pOuter->RemoveAllDecals();
	
#else
#endif

	m_flInvisChangeCompleteTime = gpGlobals->curtime + tf_spy_invis_time.GetFloat();

	// set our offhand weapon to be the invis weapon
	int i;
	for (i = 0; i < m_pOuter->WeaponCount(); i++) 
	{
		CTFWeaponBase *pWpn = ( CTFWeaponBase *)m_pOuter->GetWeapon(i);
		if ( !pWpn )
			continue;

		if ( pWpn->GetWeaponID() != TF_WEAPON_INVIS )
			continue;

		// try to switch to this weapon
		m_pOuter->SetOffHandWeapon( pWpn );
		break;
	}

	m_pOuter->TeamFortress_SetSpeed();
}

#ifdef PF2
ConVar tf_smoke_bomb_transition_time( "tf_smoke_bomb_transition_time", "0.0", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Transition time in of spy invisibility", true, 0.0, true, 5.0 );
ConVar tf_smoke_bomb_transition_out_time( "tf_smoke_bomb_transition_out_time", "1.0", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Transition time in of spy invisibility", true, 0.0, true, 5.0 );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddSmokeBomb( void )
{
#ifdef CLIENT_DLL
	//m_pOuter->EmitSound( "Player.Spy_Cloak" );
	m_pOuter->RemoveAllDecals();
#endif

	m_flInvisChangeCompleteTime = gpGlobals->curtime + tf_smoke_bomb_transition_time.GetFloat();
	m_flSmokeBombExpire = gpGlobals->curtime + tf_smoke_bomb_time.GetFloat();

	m_pOuter->TeamFortress_SetSpeed();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveSmokeBomb( void )
{
	// How have I fucked it up to get to this point
	//if (!InCond( TF_COND_SMOKE_BOMB ))
	//	return;
#ifdef CLIENT_DLL
	m_pOuter->EmitSound( "Player.Spy_UnCloak" );
#endif

	m_flInvisChangeCompleteTime = gpGlobals->curtime + tf_smoke_bomb_transition_out_time.GetFloat();
	m_flStealthNoAttackExpire = gpGlobals->curtime + tf_smoke_bomb_transition_out_time.GetFloat();

	m_pOuter->TeamFortress_SetSpeed();
}
#endif

void CTFPlayerShared::OnAddCrits( void )
{
#ifdef CLIENT_DLL
	UpdateCritBoostEffect();
#endif

}

void CTFPlayerShared::OnRemoveCrits( void )
{

	if( !pf_force_crits.GetBool() && !m_bExecutedCritsForceVar )
	{
#ifdef CLIENT_DLL
		UpdateCritBoostEffect();
#endif
	}

}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveStealthed( void )
{
#ifdef CLIENT_DLL
	m_pOuter->EmitSound( "Player.Spy_UnCloak" );
#ifdef PF2_CLIENT
	UpdateCritBoostEffect();
#endif
	m_pOuter->UpdateRecentlyTeleportedEffect();
#endif

	m_pOuter->HolsterOffHandWeapon();

	m_pOuter->TeamFortress_SetSpeed();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveDisguising( void )
{
#ifdef CLIENT_DLL
	if ( m_pOuter->m_pDisguisingEffect )
	{
//		m_pOuter->ParticleProp()->StopEmission( m_pOuter->m_pDisguisingEffect );
		m_pOuter->m_pDisguisingEffect = NULL;
	}
#else
	// Cyanide; Don't reset this, see below
	//m_nDesiredDisguiseTeam = TF_SPY_UNDEFINED;

	// Do not reset this value, we use the last desired disguise class for the
	// 'lastdisguise' command

	//m_nDesiredDisguiseClass = TF_CLASS_UNDEFINED;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveDisguised( void )
{
#ifdef CLIENT_DLL

	// if local player is on the other team, reset the model of this player
	CTFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !m_pOuter->InSameTeam( pLocalPlayer ) )
	{
		TFPlayerClassData_t *pData = GetPlayerClassData( TF_CLASS_SPY );
		int iIndex = modelinfo->GetModelIndex( pData->GetModelName() );

		m_pOuter->SetModelIndex( iIndex );
	}

	m_pOuter->EmitSound( "Player.Spy_Disguise" );

	// They may have called for medic and created a visible medic bubble
	m_pOuter->ParticleProp()->StopParticlesNamed( "speech_mediccall", true );

	UpdateCritBoostEffect();

#else
	m_nDisguiseTeam  = TF_SPY_UNDEFINED;
	m_nDisguiseClass.Set( TF_CLASS_UNDEFINED );
	m_hDisguiseTarget.Set( NULL );
	m_iDisguiseTargetIndex = TF_DISGUISE_TARGET_INDEX_NONE;
	m_iDisguiseHealth = 0;
	m_iDisguiseWeaponSlot = -1;
#ifdef PF2_DLL
	m_iDisguiseArmor = 0;
#endif
	// Update the player model and skin.
	m_pOuter->UpdateModel();

	m_pOuter->TeamFortress_SetSpeed();

	m_pOuter->ClearExpression();

#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddBurning( void )
{
#ifdef CLIENT_DLL
	// Start the burning effect
	if ( !m_pOuter->m_pBurningEffect )
	{
		const char *pEffectName = ( m_pOuter->GetTeamNumber() == TF_TEAM_RED ) ? "burningplayer_red" : "burningplayer_blue";
		m_pOuter->m_pBurningEffect = m_pOuter->ParticleProp()->Create( pEffectName, PATTACH_ABSORIGIN_FOLLOW );

		m_pOuter->m_flBurnEffectStartTime = gpGlobals->curtime;
		m_pOuter->m_flBurnEffectEndTime = gpGlobals->curtime + TF_BURNING_FLAME_LIFE;
	}
	// set the burning screen overlay
	if ( m_pOuter->IsLocalPlayer() )
	{
		IMaterial *pMaterial = materials->FindMaterial( "effects/imcookin", TEXTURE_GROUP_CLIENT_EFFECTS, false );
		if ( !IsErrorMaterial( pMaterial ) )
		{
			view->SetScreenOverlayMaterial( pMaterial );
		}
	}
#endif

	/*
#ifdef GAME_DLL
	
	if ( player == robin || player == cook )
	{
		CSingleUserRecipientFilter filter( m_pOuter );
		TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_SPECIAL );
	}

#endif
	*/

	// play a fire-starting sound
	m_pOuter->EmitSound( "Fire.Engulf" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFPlayerShared::GetStealthNoAttackExpireTime( void )
{
	return m_flStealthNoAttackExpire;
}

//-----------------------------------------------------------------------------
// Purpose: Sets whether this player is dominating the specified other player
//-----------------------------------------------------------------------------
void CTFPlayerShared::SetPlayerDominated( CTFPlayer *pPlayer, bool bDominated )
{
	int iPlayerIndex = pPlayer->entindex();
	m_bPlayerDominated.Set( iPlayerIndex, bDominated );
	pPlayer->m_Shared.SetPlayerDominatingMe( m_pOuter, bDominated );
}

#ifdef PF2_DLL
void CTFPlayerShared::Concussion( void )
{
	if (!pf_concuss_effect_disable.GetBool() && !InCond(TF_COND_INVULNERABLE))
	{
		AddCond(TF_COND_DIZZY, TF_CONCUSSION_DURATION );
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Sets whether this player is being dominated by the other player
//-----------------------------------------------------------------------------
void CTFPlayerShared::SetPlayerDominatingMe( CTFPlayer *pPlayer, bool bDominated )
{
	int iPlayerIndex = pPlayer->entindex();
	m_bPlayerDominatingMe.Set( iPlayerIndex, bDominated );
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether this player is dominating the specified other player
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsPlayerDominated( int iPlayerIndex )
{
#ifdef CLIENT_DLL
	// On the client, we only have data for the local player.
	// As a result, it's only valid to ask for dominations related to the local player
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return false;

	Assert( m_pOuter->IsLocalPlayer() || pLocalPlayer->entindex() == iPlayerIndex );

	if ( m_pOuter->IsLocalPlayer() )
		return m_bPlayerDominated.Get( iPlayerIndex );

	return pLocalPlayer->m_Shared.IsPlayerDominatingMe( m_pOuter->entindex() );
#else
	// Server has all the data.
	return m_bPlayerDominated.Get( iPlayerIndex );
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsPlayerDominatingMe( int iPlayerIndex )
{
	return m_bPlayerDominatingMe.Get( iPlayerIndex );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayerShared::NoteLastDamageTime( int nDamage )
{
	// we took damage
	if ( nDamage > 5 )
	{
		m_flLastStealthExposeTime = gpGlobals->curtime;
		AddCond( TF_COND_STEALTHED_BLINK );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsStealthed(void)
{
	if (InCond(TF_COND_STEALTHED))
		return true;
	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnSpyTouchedByEnemy( void )
{
	m_flLastStealthExposeTime = gpGlobals->curtime;
	AddCond( TF_COND_STEALTHED_BLINK );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayerShared::FadeInvis( float flInvisFadeTime )
{
	RemoveCond( TF_COND_STEALTHED );
#ifdef PF2
	RemoveCond( TF_COND_SMOKE_BOMB );
#endif
	if ( flInvisFadeTime < 0.15 )
	{
		// this was a force respawn, they can attack whenever
	}
	else
	{
		// next attack in some time
		m_flStealthNoAttackExpire = gpGlobals->curtime + tf_spy_cloak_no_attack_time.GetFloat();
	}

	m_flInvisChangeCompleteTime = gpGlobals->curtime + flInvisFadeTime;
}

//-----------------------------------------------------------------------------
// Purpose: Approach our desired level of invisibility
//-----------------------------------------------------------------------------
void CTFPlayerShared::InvisibilityThink( void )
{
	float flTargetInvis = 0.0f;
	float flTargetInvisScale = 1.0f;
	if ( InCond( TF_COND_STEALTHED_BLINK ) )
	{
		// We were bumped into or hit for some damage.
		flTargetInvisScale = TF_SPY_STEALTH_BLINKSCALE;/*tf_spy_stealth_blink_scale.GetFloat();*/
	}

	// Go invisible or appear.
	if ( m_flInvisChangeCompleteTime > gpGlobals->curtime )
	{
		if (InCond( TF_COND_STEALTHED ) || InCond( TF_COND_SMOKE_BOMB ))
		{
			flTargetInvis = 1.0f - ( ( m_flInvisChangeCompleteTime - gpGlobals->curtime ) );
		}
		else
		{
			flTargetInvis = ( ( m_flInvisChangeCompleteTime - gpGlobals->curtime ) * 0.5f );
		}
	}
	else
	{
		if (InCond( TF_COND_STEALTHED ) || InCond( TF_COND_SMOKE_BOMB ))
		{
			flTargetInvis = 1.0f;
		}
		else
		{
			flTargetInvis = 0.0f;
		}
	}

	flTargetInvis *= flTargetInvisScale;
	m_flInvisibility = clamp( flTargetInvis, 0.0f, 1.0f );
}


//-----------------------------------------------------------------------------
// Purpose: How invisible is the player [0..1]
//-----------------------------------------------------------------------------
float CTFPlayerShared::GetPercentInvisible( void )
{
	return m_flInvisibility;
}

//-----------------------------------------------------------------------------
// Purpose: Start the process of disguising
//-----------------------------------------------------------------------------
void CTFPlayerShared::Disguise( int nTeam, int nClass )
{
#ifndef CLIENT_DLL
	int nRealTeam = m_pOuter->GetTeamNumber();
	int nRealClass = m_pOuter->GetPlayerClass()->GetClassIndex();

	Assert ( ( nClass >= TF_CLASS_SCOUT ) && ( nClass <= TF_CLASS_ENGINEER ) );

	// we're not a spy
	if ( nRealClass != TF_CLASS_SPY )
	{
		return;
	}

	// we're not disguising as anything but ourselves (so reset everything)
	if ( nRealTeam == nTeam && nRealClass == nClass )
	{
		RemoveDisguise();
		return;
	}

	// Ignore disguise of the same type
	if ( nTeam == m_nDisguiseTeam && nClass == m_nDisguiseClass )
	{
		// Update the weapon slot
		int iWeaponSlot = GetActiveTFWeapon() ? GetActiveTFWeapon()->GetSlot() : 0;
		if( m_iDisguiseWeaponSlot != iWeaponSlot )
		{
			if( iWeaponSlot >= 0 && iWeaponSlot <= 2 )
			{
				m_iDisguiseWeaponSlot = iWeaponSlot;
			}
		}
		return;
	}

	// invalid team
	if ( nTeam <= TEAM_SPECTATOR || nTeam >= TF_TEAM_COUNT )
	{
		return;
	}

	// invalid class
	if ( nClass <= TF_CLASS_UNDEFINED || nClass >= TF_CLASS_COUNT )
	{
		return;
	}

	m_nDesiredDisguiseClass = nClass;
	m_nDesiredDisguiseTeam = nTeam;

	AddCond( TF_COND_DISGUISING );

	// Start the think to complete our disguise
	m_flDisguiseCompleteTime = gpGlobals->curtime + TF_TIME_TO_DISGUISE;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Set our target with a player we've found to emulate
//-----------------------------------------------------------------------------
#ifndef CLIENT_DLL
void CTFPlayerShared::FindDisguiseTarget( void )
{
	m_hDisguiseTarget = m_pOuter->TeamFortress_GetDisguiseTarget( m_nDisguiseTeam, m_nDisguiseClass );
	if ( m_hDisguiseTarget )
	{
		m_iDisguiseTargetIndex.Set( m_hDisguiseTarget.Get()->entindex() );
		Assert( m_iDisguiseTargetIndex >= 1 && m_iDisguiseTargetIndex <= MAX_PLAYERS );
	}
	else
	{
		m_iDisguiseTargetIndex.Set( TF_DISGUISE_TARGET_INDEX_NONE );
	}
}
#endif

#ifdef PF2_DLL
extern ConVar pf_armor_ratio;
extern ConVar pf_armor_bonus;
#endif
//-----------------------------------------------------------------------------
// Purpose: Complete our disguise
//-----------------------------------------------------------------------------
void CTFPlayerShared::CompleteDisguise( void )
{
#ifndef CLIENT_DLL
	AddCond( TF_COND_DISGUISED );

	m_nDisguiseClass = m_nDesiredDisguiseClass;
	m_nDisguiseTeam = m_nDesiredDisguiseTeam;

	m_iDisguiseWeaponSlot = 0;

	RemoveCond( TF_COND_DISGUISING );

	FindDisguiseTarget();

	
#ifdef PF2
	// Get a health value roughly what would be expected with such an armor value
	int iMaxArmor = GetPlayerClassData( m_nDisguiseClass )->m_nMaxArmor;
	int iMaxHealth = GetPlayerClassData( m_nDisguiseClass )->m_nMaxHealth;
	int iArmor = ( int )random->RandomInt( 10, iMaxArmor );
	m_iDisguiseArmor = iArmor;
	int iHealth = (int)( ( ( iMaxArmor - iArmor ) / 80.0f ) * 20);
	m_iDisguiseHealth = iMaxHealth - iHealth;
#else
	int iMaxHealth = m_pOuter->GetMaxHealth();
	m_iDisguiseHealth = (int)random->RandomInt( iMaxHealth / 2, iMaxHealth );
#endif

	// Update the player model and skin.
	m_pOuter->UpdateModel();

	m_pOuter->TeamFortress_SetSpeed();

	m_pOuter->ClearExpression();
#endif
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::SetDisguiseHealth( int iDisguiseHealth )
{
	m_iDisguiseHealth = iDisguiseHealth;
}

#ifdef PF2
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::SetDisguiseArmor( int iDisguiseArmor )
{
	m_iDisguiseArmor = iDisguiseArmor;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::RemoveDisguise( void )
{
#ifdef CLIENT_DLL


#else
	RemoveCond( TF_COND_DISGUISED );
	RemoveCond( TF_COND_DISGUISING );
#endif
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::RecalcDisguiseWeapon( void )
{
	if( !InCond( TF_COND_DISGUISED ) )
	{
		m_iDisguiseWeaponModelIndex = -1;
		m_pDisguiseWeaponInfo = NULL;
		return;
	}

	Assert( m_pOuter->GetPlayerClass()->GetClassIndex() == TF_CLASS_SPY );

	CTFWeaponInfo *pDisguiseWeaponInfo = NULL;

	TFPlayerClassData_t *pData = GetPlayerClassData( m_nDisguiseClass );

	Assert( pData );

	// Find the weapon in the same slot
	int i;
	for( i = 0; i<TF_PLAYER_WEAPON_COUNT; i++ )
	{
		if( pData->m_aWeapons[i] != TF_WEAPON_NONE )
		{
			const char *pWpnName = WeaponIdToAlias( pData->m_aWeapons[i] );

			WEAPON_FILE_INFO_HANDLE	hWpnInfo = LookupWeaponInfoSlot( pWpnName );
			Assert( hWpnInfo != GetInvalidWeaponInfoHandle() );
			CTFWeaponInfo *pWeaponInfo = dynamic_cast<CTFWeaponInfo*>(GetFileWeaponInfoFromHandle( hWpnInfo ));

			// find the primary weapon
			if( pWeaponInfo && pWeaponInfo->iSlot == m_iDisguiseWeaponSlot )
			{
				pDisguiseWeaponInfo = pWeaponInfo;
				break;
			}
		}
	}

	Assert( pDisguiseWeaponInfo != NULL && "Cannot find slot 0 primary weapon for desired disguise class\n" );

	m_pDisguiseWeaponInfo = pDisguiseWeaponInfo;
	m_iDisguiseWeaponModelIndex = -1;

	if( pDisguiseWeaponInfo )
	{
		m_iDisguiseWeaponModelIndex = modelinfo->GetModelIndex( pDisguiseWeaponInfo->szWorldModel );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Crit effects handling.
//-----------------------------------------------------------------------------
void CTFPlayerShared::UpdateCritBoostEffect( bool bForceHide /*= false*/ )
{
	bool bShouldShow = !bForceHide;

	if ( bShouldShow )
	{
		if ( m_pOuter->IsDormant() )
		{
			bShouldShow = false;
		}
		else if ( !IsCritBoosted() )
		{
			bShouldShow = false;
		}
		else if ( IsStealthed() )
		{
			bShouldShow = false;
		}
		else if( !m_pOuter->IsAlive() )
		{
			bShouldShow = false;
		}
		else if ( InCond( TF_COND_DISGUISED ) &&
			m_pOuter->IsEnemyPlayer() &&
			m_pOuter->GetTeamNumber() != GetDisguiseTeam() )
		{
			// Don't show crit effect for disguised enemy spies unless they're disguised
			// as their own team.
			bShouldShow = false;
		}
	}

	if ( bShouldShow )
	{
		// Update crit effect model.
		if ( m_pCritEffect )
		{
			if ( m_hCritEffectHost.Get() )
				m_hCritEffectHost->ParticleProp()->StopEmission( m_pCritEffect );

			m_pCritEffect = NULL;
		}

		bool bShouldDrawPlayer = m_pOuter->ShouldDrawThisPlayer();
		if ( !bShouldDrawPlayer )
		{
			m_hCritEffectHost = m_pOuter->GetViewModel();
		}
		else
		{
			C_BaseCombatWeapon *pWeapon = m_pOuter->GetActiveWeapon();

			// Don't add crit effect to weapons without a model.
			if ( pWeapon && pWeapon->GetWorldModelIndex() != 0 )
			{
				m_hCritEffectHost = pWeapon;
			}
			else
			{
				m_hCritEffectHost = m_pOuter;
			}
		}

		if ( m_hCritEffectHost.Get() )
		{
			const char *pszEffect = ConstructTeamParticle( bShouldDrawPlayer ? "critgun_weaponmodel_%s" : "critgun_weaponmodel_viewmodel_%s", m_pOuter->GetTeamNumber(), g_aTeamNamesShort );
			m_pCritEffect = m_hCritEffectHost->ParticleProp()->Create( pszEffect, PATTACH_ABSORIGIN_FOLLOW );
		}

		if ( !m_pCritSound )
		{
			CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
			CLocalPlayerFilter filter;
			m_pCritSound = controller.SoundCreate( filter, m_pOuter->entindex(), "Weapon_General.CritPower" );
			controller.Play( m_pCritSound, 1.0, 100 );
		}
	}
	else
	{
		if ( m_pCritEffect )
		{
			if ( m_hCritEffectHost.Get() )
				m_hCritEffectHost->ParticleProp()->StopEmission( m_pCritEffect );

			m_pCritEffect = NULL;
		}

		m_hCritEffectHost = NULL;

		if ( m_pCritSound )
		{
			CSoundEnvelopeController::GetController().SoundDestroy( m_pCritSound );
			m_pCritSound = NULL;
		}
	}
}

CTFWeaponInfo *CTFPlayerShared::GetDisguiseWeaponInfo( void )
{
	if ( InCond( TF_COND_DISGUISED ) && m_pDisguiseWeaponInfo == NULL )
	{
		RecalcDisguiseWeapon();
	}

	return m_pDisguiseWeaponInfo;
}

#endif

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: Heal players.
// pPlayer is person who healed us
//-----------------------------------------------------------------------------
void CTFPlayerShared::Heal( CTFPlayer *pPlayer, float flAmount, /* = 0*/ bool bDispenserHeal /* = false */)
{
	Assert( FindHealerIndex(pPlayer) == m_aHealers.InvalidIndex() );

	healers_t newHealer;
	newHealer.pPlayer = pPlayer;
	newHealer.flAmount = flAmount;
	newHealer.bDispenserHeal = bDispenserHeal;
	newHealer.iRecentAmount = 0;
	newHealer.flNextNotifyTime = gpGlobals->curtime + 1.0f;

	m_aHealers.AddToTail( newHealer );

	AddCond( TF_COND_HEALTH_BUFF, PERMANENT_CONDITION );

	RecalculateInvuln();

	m_nNumHealers = m_aHealers.Count();
}


//-----------------------------------------------------------------------------
// Purpose: Stop healing!
// pPlayer is person who healed us
//-----------------------------------------------------------------------------
void CTFPlayerShared::StopHealing( CTFPlayer *pPlayer )
{
	int iIndex = FindHealerIndex( pPlayer );
	Assert( iIndex != m_aHealers.InvalidIndex() );

	m_aHealers.Remove( iIndex );

	if ( !m_aHealers.Count() )
	{
		RemoveCond( TF_COND_HEALTH_BUFF );
	}

	RecalculateInvuln();

	m_nNumHealers = m_aHealers.Count();
}

#ifdef PF2_DLL
//-----------------------------------------------------------------------------
// Purpose: Regenerate armor on players.
// pPlayer is person who armored us
//-----------------------------------------------------------------------------
void CTFPlayerShared::RegenerateArmor( CTFPlayer* pPlayer, float flAmountArmor )
{
	Assert( FindArmorerIndex( pPlayer ) == m_aArmorers.InvalidIndex() );

	armorers_t newArmorer;
	newArmorer.pPlayer = pPlayer;
	newArmorer.flAmountArmor = flAmountArmor;
	newArmorer.iRecentArmorAmount = 0;
	newArmorer.flNextNotifyArmorTime = gpGlobals->curtime + 1.0f;

	m_aArmorers.AddToTail( newArmorer );

	AddCond( TF_COND_ARMOR_BUFF, PERMANENT_CONDITION );

	// This is here to stop a bug where the class would emit an invis proxy.
	RecalculateInvuln();

	m_nNumArmorers = m_aArmorers.Count();
}

void CTFPlayerShared::StopRegeneratingArmor( CTFPlayer* pPlayer )
{
	int iIndex = FindArmorerIndex( pPlayer );
	Assert( iIndex != m_aArmorers.InvalidIndex() );

	m_aArmorers.Remove( iIndex );

	if ( !m_aArmorers.Count() )
	{
		RemoveCond( TF_COND_ARMOR_BUFF );
	}

	m_nNumArmorers = m_aArmorers.Count();
}
#endif
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsProvidingInvuln( CTFPlayer *pPlayer )
{
	CTFWeaponBase *pWpn = pPlayer->GetActiveTFWeapon();
	if ( !pWpn )
		return false;

	CWeaponMedigun *pMedigun = dynamic_cast< CWeaponMedigun* >( pWpn );
	if ( pMedigun && pMedigun->IsReleasingCharge() )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::RecalculateInvuln( bool bInstantRemove )
{
	bool bShouldBeInvuln = false;

	if ( m_pOuter->m_flPowerPlayTime > gpGlobals->curtime )
	{
		bShouldBeInvuln = true;
	}

	// If we're not carrying the flag, and we're being healed by a medic 
	// who's generating invuln, then we should get invuln.
	if ( !m_pOuter->HasTheFlag() )
	{
		if ( IsProvidingInvuln( m_pOuter ) )
		{
			bShouldBeInvuln = true;
		}
		else
		{
			for ( int i = 0; i < m_aHealers.Count(); i++ )
			{
				if ( !m_aHealers[i].pPlayer )
					continue;

				CTFPlayer *pPlayer = ToTFPlayer( m_aHealers[i].pPlayer );
				if ( !pPlayer )
					continue;

				if ( IsProvidingInvuln( pPlayer ) )
				{
					bShouldBeInvuln = true;
					break;
				}
			}
		}
	}

	SetInvulnerable( bShouldBeInvuln, bInstantRemove );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::SetInvulnerable( bool bState, bool bInstant )
{
	bool bCurrentState = InCond( TF_COND_INVULNERABLE );
	if ( bCurrentState == bState )
	{
		if ( bState && m_flInvulnerableOffTime )
		{
			m_flInvulnerableOffTime = 0;
			RemoveCond( TF_COND_INVULNERABLE_WEARINGOFF );
		}
		return;
	}

	if ( bState )
	{
		Assert( !m_pOuter->HasTheFlag() );

		if ( m_flInvulnerableOffTime )
		{
			m_pOuter->StopSound( "TFPlayer.InvulnerableOff" );

			m_flInvulnerableOffTime = 0;
			RemoveCond( TF_COND_INVULNERABLE_WEARINGOFF );
		}

		// Invulnerable turning on
		AddCond( TF_COND_INVULNERABLE );

		// remove any persistent damaging conditions
		if ( InCond( TF_COND_BURNING ) )
		{
			RemoveCond( TF_COND_BURNING );
#ifdef PF2_DLL
			RemoveCond( TF_COND_NAPALM_BURNING );
#endif
		}

		CSingleUserRecipientFilter filter( m_pOuter );
		m_pOuter->EmitSound( filter, m_pOuter->entindex(), "TFPlayer.InvulnerableOn" );
	}
	else
	{
		if ( !m_flInvulnerableOffTime )
		{
			CSingleUserRecipientFilter filter( m_pOuter );
			m_pOuter->EmitSound( filter, m_pOuter->entindex(), "TFPlayer.InvulnerableOff" );
		}

		if ( bInstant )
		{
			m_flInvulnerableOffTime = 0;
			RemoveCond( TF_COND_INVULNERABLE );
			RemoveCond( TF_COND_INVULNERABLE_WEARINGOFF );
		}
		else
		{
			// We're already in the process of turning it off
			if ( m_flInvulnerableOffTime )
				return;

			AddCond( TF_COND_INVULNERABLE_WEARINGOFF );
			m_flInvulnerableOffTime = gpGlobals->curtime + tf_invuln_time.GetFloat();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFPlayerShared::FindHealerIndex( CTFPlayer *pPlayer )
{
	for ( int i = 0; i < m_aHealers.Count(); i++ )
	{
		if ( m_aHealers[i].pPlayer == pPlayer )
			return i;
	}

	return m_aHealers.InvalidIndex();
}

//-----------------------------------------------------------------------------
// Purpose: Returns the first healer in the healer array.  Note that this
//		is an arbitrary healer.
//-----------------------------------------------------------------------------
EHANDLE CTFPlayerShared::GetFirstHealer()
{
	if ( m_aHealers.Count() > 0 )
		return m_aHealers.Head().pPlayer;

	return NULL;
}

#ifdef PF2_DLL
int CTFPlayerShared::FindArmorerIndex( CTFPlayer *pPlayer )
{
	for ( int i = 0; i < m_aArmorers.Count(); i++ )
	{
		if ( m_aArmorers[i].pPlayer == pPlayer )
			return i;
	}
	return m_aArmorers.InvalidIndex();
}

//-----------------------------------------------------------------------------
// Purpose: Returns the first armorer in the healer array.  Note that this
//		is an arbitrary armorers.
//-----------------------------------------------------------------------------
EHANDLE CTFPlayerShared::GetFirstArmorer()
{
	if ( m_aArmorers.Count() > 0 )
		return m_aArmorers.Head().pPlayer;

	return NULL;
}
#endif
#endif // GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFWeaponBase *CTFPlayerShared::GetActiveTFWeapon() const
{
	return m_pOuter->GetActiveTFWeapon();
}

//-----------------------------------------------------------------------------
// Purpose: Team check.
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsAlly( CBaseEntity *pEntity )
{
	return ( pEntity->GetTeamNumber() == m_pOuter->GetTeamNumber() );
}

//-----------------------------------------------------------------------------
// Purpose: Used to determine if player should do loser animations.
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsLoser( void )
{
	if ( !m_pOuter->IsAlive() )
		return false;

	if ( tf_always_loser.GetBool() )
		return true;

	if ( TFGameRules() && TFGameRules()->State_Get() == GR_STATE_TEAM_WIN )
	{
		int iWinner = TFGameRules()->GetWinningTeam();
		if ( iWinner != m_pOuter->GetTeamNumber() )
		{
			if ( m_pOuter->IsPlayerClass( TF_CLASS_SPY ) )
			{
				if ( InCond( TF_COND_DISGUISED ) )
				{
					return ( iWinner != GetDisguiseTeam() );
				}		
			}

			//just in case we get it by mistake somehow
			// -Nbc66
			if( InCond( TF_COND_CRITBOOSTED ) && !pf_force_crits.GetBool() )
			{
				RemoveCond( TF_COND_CRITBOOSTED );
			}

			return true;
		}


		//if we are on the winning team give us crits
		// -Nbc66
		if( iWinner == m_pOuter->GetTeamNumber() && !InCond( TF_COND_CRITBOOSTED ) && !pf_force_crits.GetBool() )
		{
			AddCond( TF_COND_CRITBOOSTED );
		}
	}
	return false;
}

int CTFPlayerShared::MaxAmmo(int ammoType)
{
	for (int i = 0; i < TF_PLAYER_WEAPON_COUNT + TF_PLAYER_GRENADE_COUNT; i++)
	{
		int iWeapon = i;
		bool grenade = false;
		if (i >= TF_PLAYER_WEAPON_COUNT)
		{
			grenade = true;
			iWeapon -= TF_PLAYER_WEAPON_COUNT;
		}
		CBaseCombatWeapon* pw = m_pOuter->GetWeapon( i );
		if ( !pw ) continue;
		CTFWeaponBase* ptfw = static_cast<CTFWeaponBase*>( pw );
		if (!ptfw) continue;
		int ammo = 0;
		CTFWeaponInfo pwi = ptfw->GetTFWpnData();

		if ( pwi.iAmmoType == ammoType )
		{
			ammo = pwi.GetWeaponData( TF_WEAPON_PRIMARY_MODE ).m_iMaxAmmo;
		}

		else if (pwi.iAmmo2Type == ammoType)
		{
			ammo = pwi.GetWeaponData( TF_WEAPON_SECONDARY_MODE ).m_iMaxAmmo;
		}
		if ( ammo > 0 )
			return ammo;
	}
	return m_pOuter->GetPlayerClass()->GetData()->m_aAmmoMax[ammoType];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFPlayerShared::GetDesiredPlayerClassIndex( void )
{
	return m_iDesiredPlayerClass;
}

#ifdef PF2
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::StartGrenadeReneration(int nGrenade)
{
	if(nGrenade == 0)
	{
		m_flGrenade1RegenTime = gpGlobals->curtime + pf_grenades_regenerate_time.GetFloat();
	}
	else
	{
		m_flGrenade2RegenTime = gpGlobals->curtime + pf_grenades_regenerate_time.GetFloat();
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::SetJumping( bool bJumping )
{
	m_bJumping = bJumping;
}

void CTFPlayerShared::SetAirDash( bool bAirDash )
{
	m_bAirDash = bAirDash;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFPlayerShared::GetSequenceForDeath( CBaseAnimating *pAnim, int iDamageCustom )
{
	const char *pszSequence = NULL;

	switch( iDamageCustom )
	{
	case TF_DMG_CUSTOM_BACKSTAB:
		pszSequence = "primary_death_backstab";
		break;
	case TF_DMG_CUSTOM_HEADSHOT:
		pszSequence = "primary_death_headshot";
		break;
	// They're annoying in TF2C they'll be annoying here
	// If you want you can impliment a convar to enable it clientside
	/*case TF_DMG_CUSTOM_BURNING:
		pszSequence = "primary_death_burning";
		break;
	*/
	case TF_DMG_CUSTOM_EMP_EXPLOSION:
		pszSequence = "dieviolent";
		break;
	}

	if ( pszSequence != NULL )
	{
		return pAnim->LookupSequence( pszSequence );
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFPlayerShared::GetCritMult( void )
{
	float flRemapCritMul = RemapValClamped( m_iCritMult, 0, 255, 1.0, 4.0 );
/*#ifdef CLIENT_DLL
	Msg("CLIENT: Crit mult %.2f - %d\n",flRemapCritMul, m_iCritMult);
#else
	Msg("SERVER: Crit mult %.2f - %d\n", flRemapCritMul, m_iCritMult );
#endif*/

	return flRemapCritMul;
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::UpdateCritMult( void )
{
	const float flMinMult = 1.0;
	const float flMaxMult = TF_DAMAGE_CRITMOD_MAXMULT;

	if ( m_DamageEvents.Count() == 0 )
	{
		m_iCritMult = RemapValClamped( flMinMult, 1.0, 4.0, 0, 255 );
		return;
	}

	//Msg( "Crit mult update for %s\n", m_pOuter->GetPlayerName() );
	//Msg( "   Entries: %d\n", m_DamageEvents.Count() );

	// Go through the damage multipliers and remove expired ones, while summing damage of the others
	float flTotalDamage = 0;
	for ( int i = m_DamageEvents.Count() - 1; i >= 0; i-- )
	{
		float flDelta = gpGlobals->curtime - m_DamageEvents[i].flTime;
		if ( flDelta > tf_damage_events_track_for.GetFloat() )
		{
			//Msg( "      Discarded (%d: time %.2f, now %.2f)\n", i, m_DamageEvents[i].flTime, gpGlobals->curtime );
			m_DamageEvents.Remove(i);
			continue;
		}

		// Ignore damage we've just done. We do this so that we have time to get those damage events
		// to the client in time for using them in prediction in this code.
		if ( flDelta < TF_DAMAGE_CRITMOD_MINTIME )
		{
			//Msg( "      Ignored (%d: time %.2f, now %.2f)\n", i, m_DamageEvents[i].flTime, gpGlobals->curtime );
			continue;
		}

		if ( flDelta > TF_DAMAGE_CRITMOD_MAXTIME )
			continue;

		//Msg( "      Added %.2f (%d: time %.2f, now %.2f)\n", m_DamageEvents[i].flDamage, i, m_DamageEvents[i].flTime, gpGlobals->curtime );

		flTotalDamage += m_DamageEvents[i].flDamage;
	}

	float flMult = RemapValClamped( flTotalDamage, 0, TF_DAMAGE_CRITMOD_DAMAGE, flMinMult, flMaxMult );

	//Msg( "   TotalDamage: %.2f   -> Mult %.2f\n", flTotalDamage, flMult );

	m_iCritMult = (int)RemapValClamped( flMult, 1.0, 4.0, 0, 255 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::RecordDamageEvent( const CTakeDamageInfo &info, bool bKill )
{
	if ( m_DamageEvents.Count() >= MAX_DAMAGE_EVENTS )
	{
		// Remove the oldest event
		m_DamageEvents.Remove( m_DamageEvents.Count()-1 );
	}

	int iIndex = m_DamageEvents.AddToTail();
	m_DamageEvents[iIndex].flDamage = info.GetDamage();
	m_DamageEvents[iIndex].flTime = gpGlobals->curtime;
	m_DamageEvents[iIndex].bKill = bKill;

	// Don't count critical damage
	if ( info.GetDamageType() & DMG_CRITICAL )
	{
		m_DamageEvents[iIndex].flDamage /= TF_DAMAGE_CRIT_MULTIPLIER;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFPlayerShared::GetNumKillsInTime( float flTime )
{
	if ( tf_damage_events_track_for.GetFloat() < flTime )
	{
		Warning("Player asking for damage events for time %.0f, but tf_damage_events_track_for is only tracking events for %.0f\n", flTime, tf_damage_events_track_for.GetFloat() );
	}

	int iKills = 0;
	for ( int i = m_DamageEvents.Count() - 1; i >= 0; i-- )
	{
		float flDelta = gpGlobals->curtime - m_DamageEvents[i].flTime;
		if ( flDelta < flTime )
		{
			if ( m_DamageEvents[i].bKill )
			{
				iKills++;
			}
		}
	}

	return iKills;
}

#endif

//=============================================================================
//
// Shared player code that isn't CTFPlayerShared
//

//-----------------------------------------------------------------------------
// Purpose:
//   Input: info
//          bDoEffects - effects (blood, etc.) should only happen client-side.
//-----------------------------------------------------------------------------
void CTFPlayer::FireBullet( const FireBulletsInfo_t &info, bool bDoEffects, int nDamageType, int nCustomDamageType /*= TF_DMG_CUSTOM_NONE*/ )
{
	// Fire a bullet (ignoring the shooter).
	Vector vecStart = info.m_vecSrc;
	Vector vecEnd = vecStart + info.m_vecDirShooting * info.m_flDistance;
	trace_t trace;
	UTIL_TraceLine( vecStart, vecEnd, ( MASK_SOLID | CONTENTS_HITBOX ), this, COLLISION_GROUP_NONE, &trace );

#ifdef GAME_DLL
	if ( tf_debug_bullets.GetBool() )
	{
		NDebugOverlay::Line( vecStart, trace.endpos, 0,255,0, true, 30 );
	}
#endif

#if defined(_DEBUG) && defined(PF2)
#ifdef CLIENT_DLL
	if (sv_showimpacts.GetInt() == 1 || sv_showimpacts.GetInt() == 2)
	{
		// draw red client impact markers
		debugoverlay->AddBoxOverlay( trace.endpos, Vector( -2, -2, -2 ), Vector( 2, 2, 2 ), QAngle( 0, 0, 0 ), 255, 0, 0, 127, 4 );

		if (trace.m_pEnt && trace.m_pEnt->IsPlayer())
		{
			C_BasePlayer *player = ToBasePlayer( trace.m_pEnt );
			player->DrawClientHitboxes( 4, true );
		}
	}
#else
	if (sv_showimpacts.GetInt() == 1 || sv_showimpacts.GetInt() == 3)
	{
		// draw blue server impact markers
		NDebugOverlay::Box( trace.endpos, Vector( -2, -2, -2 ), Vector( 2, 2, 2 ), 0, 0, 255, 127, 4 );

		if (trace.m_pEnt && trace.m_pEnt->IsPlayer())
		{
			CBasePlayer *player = ToBasePlayer( trace.m_pEnt );
			player->DrawServerHitboxes( 4, true );
		}
	}
#endif

	if (sv_showplayerhitboxes.GetInt() > 0)
	{
		CBasePlayer *lagPlayer = UTIL_PlayerByIndex( sv_showplayerhitboxes.GetInt() );
		if (lagPlayer)
		{
#ifdef CLIENT_DLL
			lagPlayer->DrawClientHitboxes( 4, true );
#else
			lagPlayer->DrawServerHitboxes( 4, true );
#endif
		}
	}
#endif

	if( trace.fraction < 1.0 )
	{
		// Verify we have an entity at the point of impact.
		Assert( trace.m_pEnt );

		if( bDoEffects )
		{
			// If shot starts out of water and ends in water
			if ( !( enginetrace->GetPointContents( trace.startpos ) & ( CONTENTS_WATER | CONTENTS_SLIME ) ) &&
				( enginetrace->GetPointContents( trace.endpos ) & ( CONTENTS_WATER | CONTENTS_SLIME ) ) )
			{	
				// Water impact effects.
				ImpactWaterTrace( trace, vecStart );
			}
			else
			{
				// Regular impact effects.

				// don't decal your teammates or objects on your team
				if ( trace.m_pEnt->GetTeamNumber() != GetTeamNumber() )
				{
					UTIL_ImpactTrace( &trace, nDamageType );
				}
			}

#ifdef CLIENT_DLL
			static int	tracerCount;
			if ( ( info.m_iTracerFreq != 0 ) && ( tracerCount++ % info.m_iTracerFreq ) == 0 )
			{
				// if this is a local player, start at attachment on view model
				// else start on attachment on weapon model

				int iEntIndex = entindex();
				int iUseAttachment = TRACER_DONT_USE_ATTACHMENT;
				int iAttachment = 1;

				C_BaseCombatWeapon *pWeapon = GetActiveWeapon();

				if( pWeapon )
				{
					iAttachment = pWeapon->LookupAttachment( "muzzle" );
				}

				bool bInToolRecordingMode = clienttools->IsInRecordingMode();

				// try to align tracers to actual weapon barrel if possible
				if ( !ShouldDrawThisPlayer() && !bInToolRecordingMode )
				{
					C_BaseViewModel *pViewModel = dynamic_cast< C_BaseViewModel * >( GetViewModel() );

					if ( pViewModel )
					{
						iEntIndex = pViewModel->entindex();
						pViewModel->GetAttachment( iAttachment, vecStart );
					}
				}
				else if ( !IsDormant() )
				{
					// fill in with third person weapon model index
					C_BaseCombatWeapon *pWeapon = GetActiveWeapon();

					if( pWeapon )
					{
						iEntIndex = pWeapon->entindex();

						int nModelIndex = pWeapon->GetModelIndex();
						int nWorldModelIndex = pWeapon->GetWorldModelIndex();
						if ( bInToolRecordingMode && nModelIndex != nWorldModelIndex )
						{
							pWeapon->SetModelIndex( nWorldModelIndex );
						}

						pWeapon->GetAttachment( iAttachment, vecStart );

						if ( bInToolRecordingMode && nModelIndex != nWorldModelIndex )
						{
							pWeapon->SetModelIndex( nModelIndex );
						}
					}
				}

				if ( tf_useparticletracers.GetBool() )
				{
					const char *pszTracerEffect = GetTracerType();
					if ( pszTracerEffect && pszTracerEffect[0] )
					{
						char szTracerEffect[128];
						if ( nDamageType & DMG_CRITICAL )
						{
							Q_snprintf( szTracerEffect, sizeof(szTracerEffect), "%s_crit", pszTracerEffect );
							pszTracerEffect = szTracerEffect;
						}

						UTIL_ParticleTracer( pszTracerEffect, vecStart, trace.endpos, entindex(), iUseAttachment, true );
					}
				}
				else
				{
					UTIL_Tracer( vecStart, trace.endpos, entindex(), iUseAttachment, 5000, true, GetTracerType() );
				}
			}
#endif

		}

		// Server specific.
#ifndef CLIENT_DLL
		// See what material we hit.
		CTakeDamageInfo dmgInfo( this, info.m_pAttacker, GetActiveWeapon(), info.m_flDamage, nDamageType );
		dmgInfo.SetDamageCustom( nCustomDamageType );
		CalculateBulletDamageForce( &dmgInfo, info.m_iAmmoType, info.m_vecDirShooting, trace.endpos, 1.0 );	//MATTTODO bullet forces
		trace.m_pEnt->DispatchTraceAttack( dmgInfo, info.m_vecDirShooting, &trace );
#endif
	}
}

#ifdef CLIENT_DLL
static ConVar tf_impactwatertimeenable( "tf_impactwatertimeenable", "0", FCVAR_CHEAT, "Draw impact debris effects." );
static ConVar tf_impactwatertime( "tf_impactwatertime", "1.0f", FCVAR_CHEAT, "Draw impact debris effects." );
#endif

//-----------------------------------------------------------------------------
// Purpose: Trace from the shooter to the point of impact (another player,
//          world, etc.), but this time take into account water/slime surfaces.
//   Input: trace - initial trace from player to point of impact
//          vecStart - starting point of the trace 
//-----------------------------------------------------------------------------
void CTFPlayer::ImpactWaterTrace( trace_t &trace, const Vector &vecStart )
{
#ifdef CLIENT_DLL
	if ( tf_impactwatertimeenable.GetBool() )
	{
		if ( m_flWaterImpactTime > gpGlobals->curtime )
			return;
	}
#endif 

	trace_t traceWater;
	UTIL_TraceLine( vecStart, trace.endpos, ( MASK_SHOT | CONTENTS_WATER | CONTENTS_SLIME ), 
		this, COLLISION_GROUP_NONE, &traceWater );
	if( traceWater.fraction < 1.0f )
	{
		CEffectData	data;
		data.m_vOrigin = traceWater.endpos;
		data.m_vNormal = traceWater.plane.normal;
		data.m_flScale = random->RandomFloat( 8, 12 );
		if ( traceWater.contents & CONTENTS_SLIME )
		{
			data.m_fFlags |= FX_WATER_IN_SLIME;
		}

		const char *pszEffectName = "tf_gunshotsplash";
		CTFWeaponBase *pWeapon = GetActiveTFWeapon();
		if ( pWeapon && ( TF_WEAPON_MINIGUN == pWeapon->GetWeaponID() ) )
		{
			// for the minigun, use a different, cheaper splash effect because it can create so many of them
			pszEffectName = "tf_gunshotsplash_minigun";
		}		
		DispatchEffect( pszEffectName, data );

#ifdef CLIENT_DLL
		if ( tf_impactwatertimeenable.GetBool() )
		{
			m_flWaterImpactTime = gpGlobals->curtime + tf_impactwatertime.GetFloat();
		}
#endif
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFWeaponBase *CTFPlayer::GetActiveTFWeapon( void ) const
{
	CBaseCombatWeapon *pRet = GetActiveWeapon();
	if ( pRet )
	{
		Assert( dynamic_cast< CTFWeaponBase* >( pRet ) != NULL );
		return static_cast< CTFWeaponBase * >( pRet );
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Check if passed weapon is active
//-----------------------------------------------------------------------------
bool CTFPlayer::IsActiveTFWeapon( int iWeaponID )
{
	CTFWeaponBase* pWeapon = GetActiveTFWeapon();
	if ( pWeapon )
	{
		return pWeapon->GetWeaponID() == iWeaponID;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: How much build resource ( metal ) does this player have
//-----------------------------------------------------------------------------
int CTFPlayer::GetBuildResources( void )
{
	return GetAmmoCount( TF_AMMO_METAL );
}
#ifdef PF2
extern ConVar tf_flamethrower_speed_penalty;
#endif
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::TeamFortress_SetSpeed()
{
	int playerclass = GetPlayerClass()->GetClassIndex();
	float maxfbspeed;

	// Spectators can move while in Classic Observer mode
	if ( IsObserver() )
	{
		if ( GetObserverMode() == OBS_MODE_ROAMING )
			SetMaxSpeed( GetPlayerClassData( TF_CLASS_SCOUT )->m_flMaxSpeed );
		else
			SetMaxSpeed( 0 );
		return;
	}

	// Check for any reason why they can't move at all
	if ( playerclass == TF_CLASS_UNDEFINED || GameRules()->InRoundRestart() )
	{
		if (!m_Shared.InCond( TF_COND_AIMING ))
			SetAbsVelocity( vec3_origin );
		SetMaxSpeed( 1 );
		return;
	}
#ifdef PF2
	if ( m_Shared.InCond( TF_COND_BUILDING_DETPACK ) )
	{
		SetAbsVelocity( vec3_origin );
		SetMaxSpeed( 1 );
		return;
	}
#endif

	// First, get their max class speed
	maxfbspeed = GetPlayerClassData( playerclass )->m_flMaxSpeed;

	// Slow us down if we're disguised as a slower class
	// unless we're cloaked..
	if ( m_Shared.InCond( TF_COND_DISGUISED ) && !m_Shared.InCond( TF_COND_STEALTHED ) )
	{
		float flMaxDisguiseSpeed = GetPlayerClassData( m_Shared.GetDisguiseClass() )->m_flMaxSpeed;
		maxfbspeed = min( flMaxDisguiseSpeed, maxfbspeed );
	}

// Expecting this to not exist in Live anymore
#ifdef PF2
	// Second, see if any flags are slowing them down
	if ( HasItem() && GetItem()->GetItemID() == TF_ITEM_CAPTURE_FLAG )
	{
		CCaptureFlag *pFlag = dynamic_cast<CCaptureFlag*>( GetItem() );

		if ( pFlag )
		{
			if ( pf_force_flag_speed_penalty.GetBool() || pFlag->SlowCarrier() || pFlag->GetGameType() == TF_FLAGTYPE_ATTACK_DEFEND || pFlag->GetGameType() == TF_FLAGTYPE_TERRITORY_CONTROL )
			{
				maxfbspeed = GetPlayerClass()->GetMaxFlagSpeed();
			}
		}
	}
#endif

	// if they're a sniper, and they're aiming, their speed must be 80 or less
	if ( m_Shared.InCond( TF_COND_AIMING ) )
	{
		// Pyro's move faster while firing their flamethrower
		if ( playerclass == TF_CLASS_PYRO )
		{
#ifdef PF2
			if (maxfbspeed > tf_flamethrower_speed_penalty.GetFloat() )
				maxfbspeed = tf_flamethrower_speed_penalty.GetFloat();
#else
			if (maxfbspeed > 200)
				maxfbspeed = 200;
#endif
		}
		else
		{
			if (maxfbspeed > 80)
				maxfbspeed = 80;
		}
	}

	// Engineer moves slower while a hauling an object.
	if (playerclass == TF_CLASS_ENGINEER && m_Shared.IsCarryingObject())
	{
		maxfbspeed *= 0.9f;
	}

	if ( m_Shared.InCond( TF_COND_STEALTHED ) )
	{
		if (maxfbspeed > tf_spy_max_cloaked_speed.GetFloat() )
			maxfbspeed = tf_spy_max_cloaked_speed.GetFloat();
	}

	// Handle these speed changes in GetPlayerMaxSpeed now
#if 0
	if (m_Shared.InCond(TF_COND_TRANQUILIZED))
	{
		maxfbspeed *= TRANQ_MOVEMENT_CHANGE;
	}

	if (m_Shared.InCond(TF_COND_LEG_DAMAGED))
	{
		maxfbspeed *= CALTROP_MOVEMENT_CHANGE;
	}
#endif

	// if we're in bonus time because a team has won, give the winners 110% speed and the losers 90% speed
	if ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN )
	{
		int iWinner = TFGameRules()->GetWinningTeam();
		
		if ( iWinner != TEAM_UNASSIGNED )
		{
			if ( iWinner == GetTeamNumber() )
			{
				maxfbspeed *= 1.1f;
			}
			else
			{
				maxfbspeed *= 0.9f;
			}
		}
	}

	// Set the speed
	SetMaxSpeed( maxfbspeed );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPlayer::HasItem( void )
{
	return ( m_hItem != NULL );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayer::SetItem( CTFItem *pItem )
{
	m_hItem = pItem;

#ifndef CLIENT_DLL
	if ( pItem && pItem->GetItemID() == TF_ITEM_CAPTURE_FLAG )
	{
		RemoveInvisibility();
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFItem	*CTFPlayer::GetItem( void )
{
	return m_hItem;
}

//-----------------------------------------------------------------------------
// Purpose: Is the player allowed to use a teleporter ?
//-----------------------------------------------------------------------------
bool CTFPlayer::HasTheFlag( void )
{
	if ( HasItem() && GetItem()->GetItemID() == TF_ITEM_CAPTURE_FLAG )
	{
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Return true if this player's allowed to build another one of the specified object
//-----------------------------------------------------------------------------
int CTFPlayer::CanBuild( int iObjectType, int iObjectMode )
{
	if ( iObjectType < 0 || iObjectType >= OBJ_LAST )
		return CB_UNKNOWN_OBJECT;

#ifndef CLIENT_DLL
	CTFPlayerClass *pCls = GetPlayerClass();

	if ( m_Shared.IsCarryingObject() )
	{
		CBaseObject* pObject = m_Shared.GetCarriedObject();
		if ( pObject && pObject->GetType() == iObjectType && pObject->GetObjectMode() == iObjectMode )
		{
			return CB_CAN_BUILD;
		}
		else
		{
			Assert( 0 );
		}
	}

	if ( pCls && pCls->CanBuildObject( iObjectType ) == false )
	{
		return CB_CANNOT_BUILD;
	}

	if( GetObjectInfo( iObjectType )->m_AltModes.Count() != 0
		&& GetObjectInfo( iObjectType )->m_AltModes.Count() <= iObjectMode * 3 )
	{
		return CB_CANNOT_BUILD;
	}
	else if( GetObjectInfo( iObjectType )->m_AltModes.Count() == 0 && iObjectMode != 0 )
	{
		return CB_CANNOT_BUILD;
	}

#endif

	int iObjectCount = GetNumObjects( iObjectType, iObjectMode );

	// Make sure we haven't hit maximum number
	if ( iObjectCount >= GetObjectInfo( iObjectType )->m_nMaxObjects && GetObjectInfo( iObjectType )->m_nMaxObjects != -1 )
	{
		return CB_LIMIT_REACHED;
	}

#ifdef PF2
	if ( iObjectType == OBJ_DETPACK )
	{
		if ( m_Shared.GetDetpackUsed() )
		{
			return CB_NEED_RESOURCES;
		}
		return CB_CAN_BUILD;
	}
#endif

	// Find out how much the object should cost
	int iCost = CalculateObjectCost( iObjectType );

	// Make sure we have enough resources
	if ( GetBuildResources() < iCost )
	{
		return CB_NEED_RESOURCES;
	}

	return CB_CAN_BUILD;
}

//-----------------------------------------------------------------------------
// Purpose: Get the number of objects of the specified type that this player has
//-----------------------------------------------------------------------------
int CTFPlayer::GetNumObjects( int iObjectType, int iObjectMode )
{
	int iCount = 0;
	for ( int i = 0; i < GetObjectCount(); i++ )
	{
		if ( !GetObject(i) )
			continue;

		if ( GetObject(i)->GetType() == iObjectType && 
			GetObject(i)->GetObjectMode() == iObjectMode && 
			!GetObject(i)->IsBeingCarried())
		{
			iCount++;
		}
	}

	return iCount;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ItemPostFrame()
{
	HandleGrenades();

	if ( m_hOffHandWeapon.Get() )
	{
		if ( gpGlobals->curtime < m_flNextAttack )
		{
			m_hOffHandWeapon->ItemBusyFrame();
		}
		else
		{
#if defined( CLIENT_DLL )
			// Not predicting this weapon
			if ( m_hOffHandWeapon->IsPredicted() )
#endif
			{
				m_hOffHandWeapon->ItemPostFrame();
			}
		}
	}

#ifdef PF2_DLL
	if (m_Shared.InCond( TF_COND_SMOKE_BOMB ))
	{
		if (m_afButtonPressed & IN_ATTACK)
		{
			m_Shared.RemoveCond( TF_COND_SMOKE_BOMB );
		}

	}
#endif

	BaseClass::ItemPostFrame();
}

#ifdef PF2
void CTFPlayer::HandleGrenades()
{
	// Cyanide; Avoid having to check if it's the invis watch (or potentially something else)
	CTFWeaponBaseGrenade *pOffHandGrenade = dynamic_cast<CTFWeaponBaseGrenade *>(m_hOffHandWeapon.Get());

	if( !GetPrimedState() && !pOffHandGrenade )
	{
		// are we pressing a grenade button?
		if( !(m_afButtonPressed & (IN_GRENADE1 | IN_GRENADE2)) )
			return;

		while( true )
		{
			if( !CanAttack() )
				break;

			if( !GetActiveTFWeapon() )
				break;

			if( GetActiveTFWeapon()->IsLowered() || !GetActiveTFWeapon()->GetTFWpnData().m_bCanThrowGrenade )
				break;

			if( m_Shared.m_flNextThrowTime > gpGlobals->curtime )
				break;

			if( m_Shared.InCond( TF_COND_ZOOMED ) || m_Shared.InCond( TF_COND_AIMING ) )
				break;

			// if we pressed a grenade button we can assume it's one or the other
			int iGrenade = m_afButtonPressed & IN_GRENADE1 ? 0 : 1;
			TFPlayerClassData_t *pData = m_PlayerClass.GetData();
			CTFWeaponBaseGrenade *pGrenade = (CTFWeaponBaseGrenade *)Weapon_OwnsThisID( pData->m_aGrenades[iGrenade] );
			if( pGrenade && pGrenade->HasAmmo() )
			{
				if( TFGameRules()->GrenadesRegenerate() )
				{
					float flRegenTime = iGrenade == 0 ? m_Shared.m_flGrenade1RegenTime.Get() : m_Shared.m_flGrenade2RegenTime.Get();
					if( flRegenTime > gpGlobals->curtime )
						break;
				}

				MSG_NOTIFY_ON_HIT;
				bool bLowered = true;
				if( pf_grenades_holstering.GetBool() && pGrenade->ShouldLowerMainWeapon() )
				{
					bLowered = false;
					if( GetActiveTFWeapon() && !GetActiveTFWeapon()->IsLowered() )
					{
						GetActiveTFWeapon()->Lower();
						// Remove the grenade buttons from recently pressed
						m_afButtonPressed &= ~(IN_GRENADE1 | IN_GRENADE2);
					}
				}

#ifdef CLIENT_DLL
				if( prediction && !prediction->IsFirstTimePredicted() )
					return;
#endif
				pGrenade->SetWeaponVisible( false );
				pGrenade->Prime();
				SetPrimedState( PRIME_STATE_PRE_DEPLOY );
				m_hOffHandWeapon = pGrenade;
				return;
			}
			break;
		}

#ifdef CLIENT_DLL
		if( m_flNextDenySound < gpGlobals->curtime )
		{
			EmitSound( "Player.DenyWeaponSelection" );
			m_flNextDenySound = gpGlobals->curtime + 0.5;
		}
#endif
	}
	else if( GetPrimedState() == PRIME_STATE_PRE_DEPLOY && pOffHandGrenade )
	{
		if( GetActiveTFWeapon()->HasWeaponIdleTimeElapsed() )
		{
			if( pf_grenades_holstering.GetBool() )
			{
				m_hOffHandWeapon->SetWeaponVisible( true );
				CBaseViewModel *vm = GetViewModel( TF_VM_OFFHAND );
				if( vm )
					vm->RemoveEffects( EF_NODRAW );
				m_hOffHandWeapon->Deploy();
			}
			SetPrimedState( PRIME_STATE_DEPLOYED );
			return;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Hides the grenade viewmodel and readys the our active weapon
//-----------------------------------------------------------------------------
void CTFPlayer::FinishThrowGrenade(void)
{
#if defined( CLIENT_DLL )
	if( prediction && !prediction->IsFirstTimePredicted() )
		return;
#endif

	// Cyanide; Bad catch so we don't spam this out
	if( GetPrimedState() != PRIME_STATE_DEPLOYED )
		return;

	MSG_NOTIFY_ON_HIT;

	m_hOffHandWeapon->SetWeaponVisible( false );
	m_hOffHandWeapon = NULL;
	CBaseViewModel *vm = GetViewModel( TF_VM_OFFHAND );
	if( vm )
		vm->AddEffects( EF_NODRAW );

	if( pf_grenades_holstering.GetBool() && GetActiveTFWeapon() )
	{
		GetActiveTFWeapon()->Ready();
		// don't allow m_flNextThrowTime to be longer than the normal deploy time
		m_Shared.m_flNextThrowTime = gpGlobals->curtime + min(GetActiveTFWeapon()->SequenceDuration(), 0.40f);
	}
	SetPrimedState( PRIME_STATE_NONE );
}
#endif

void CTFPlayer::SetOffHandWeapon( CTFWeaponBase *pWeapon )
{
	m_hOffHandWeapon = pWeapon;
	if ( m_hOffHandWeapon.Get() )
	{
		m_hOffHandWeapon->Deploy();
	}
}

// Set to NULL at the end of the holster?
void CTFPlayer::HolsterOffHandWeapon( void )
{
	if ( m_hOffHandWeapon.Get() )
	{
		m_hOffHandWeapon->Holster();
	}
#ifdef PF2
	m_hOffHandWeapon = NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Return true if we should record our last weapon when switching between the two specified weapons
//-----------------------------------------------------------------------------
bool CTFPlayer::Weapon_ShouldSetLast( CBaseCombatWeapon *pOldWeapon, CBaseCombatWeapon *pNewWeapon )
{
	// if the weapon doesn't want to be auto-switched to, don't!	
	CTFWeaponBase *pTFWeapon = dynamic_cast< CTFWeaponBase * >( pOldWeapon );
	
	if ( pTFWeapon->AllowsAutoSwitchTo() == false )
	{
		return false;
	}

	return BaseClass::Weapon_ShouldSetLast( pOldWeapon, pNewWeapon );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPlayer::Weapon_Switch( CBaseCombatWeapon *pWeapon, int viewmodelindex )
{
	m_PlayerAnimState->ResetGestureSlot( GESTURE_SLOT_ATTACK_AND_RELOAD );
#ifdef PF2
	if (GetActiveTFWeapon())
	{
		if (GetActiveTFWeapon()->IsLowered()) 
			return false;

		if (pWeapon)
		{	
			CTFWeaponBase* ptfWeapon = static_cast<CTFWeaponBase*>(pWeapon);
			if (ptfWeapon && ptfWeapon->GetTFWpnData().m_bGrenade)
			{
				return false;
			}
		}
	}
#endif
	
	return BaseClass::Weapon_Switch( pWeapon, viewmodelindex );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::GetStepSoundVelocities( float *velwalk, float *velrun )
{
	float flMaxSpeed = MaxSpeed();

	if ( ( GetFlags() & FL_DUCKING ) || ( GetMoveType() == MOVETYPE_LADDER ) )
	{
		*velwalk = flMaxSpeed * 0.25;
		*velrun = flMaxSpeed * 0.3;		
	}
	else
	{
		*velwalk = flMaxSpeed * 0.3;
		*velrun = flMaxSpeed * 0.8;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::SetStepSoundTime( stepsoundtimes_t iStepSoundTime, bool bWalking )
{
	float flMaxSpeed = MaxSpeed();

	switch ( iStepSoundTime )
	{
	case STEPSOUNDTIME_NORMAL:
	case STEPSOUNDTIME_WATER_FOOT:
		m_flStepSoundTime = RemapValClamped( flMaxSpeed, 200, 450, 400, 200 );
		if ( bWalking )
		{
			m_flStepSoundTime += 100;
		}
		break;

	case STEPSOUNDTIME_ON_LADDER:
		m_flStepSoundTime = 350;
		break;

	case STEPSOUNDTIME_WATER_KNEE:
		m_flStepSoundTime = RemapValClamped( flMaxSpeed, 200, 450, 600, 400 );
		break;

	default:
		Assert(0);
		break;
	}

	if ( ( GetFlags() & FL_DUCKING) || ( GetMoveType() == MOVETYPE_LADDER ) )
	{
		m_flStepSoundTime += 100;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::CanAttack( void )
{
	CTFGameRules *pRules = TFGameRules();

	Assert( pRules );

	if (m_Shared.GetStealthNoAttackExpireTime() > gpGlobals->curtime || m_Shared.InCond( TF_COND_STEALTHED ) || m_Shared.InCond( TF_COND_SMOKE_BOMB ))
	{
#ifdef CLIENT_DLL
		HintMessage( HINT_CANNOT_ATTACK_WHILE_CLOAKED, true, true );
#endif
		return false;
	}

	if (m_Shared.InCond( TF_COND_TAUNTING ))
	{
		return false;
	}

	if ( ( pRules->State_Get() == GR_STATE_TEAM_WIN ) && ( pRules->GetWinningTeam() != GetTeamNumber() ) )
	{
		return false;
	}

	return true;
}

bool CTFPlayer::CanPickupBuilding( CBaseObject *pObject )
{
	if( !pObject )
		return false;

	if( !pf_haul_buildings.GetBool() )
		return false;

	//if ( m_Shared.IsLoser() )
	//	return false;

	if( IsActiveTFWeapon( TF_WEAPON_BUILDER ) )
		return false;

	if( pObject->GetBuilder() != this )
		return false;

	if( pObject->IsBuilding() || pObject->IsUpgrading() || pObject->IsRedeploying() || pObject->IsDisabled() )
		return false;

	float flObjDistance = VectorLength( pObject->GetAbsOrigin() - EyePosition() );
	// Close enough for now
	if( flObjDistance > OBJECT_PICKUP_DISTANCE )
		return false;

	return true;
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::TryToDismantleBuilding( void )
{
	Vector vecForward;
	AngleVectors( EyeAngles(), &vecForward );
	Vector vecSwingStart = Weapon_ShootPosition();
	float flRange = OBJECT_PICKUP_DISTANCE;
	Vector vecSwingEnd = vecSwingStart + vecForward * flRange;

	// only trace against objects
	trace_t trace;
	CTraceFilterIgnorePlayers traceFilter( NULL, COLLISION_GROUP_NONE );
	UTIL_TraceLine( vecSwingStart, vecSwingEnd, MASK_SOLID, &traceFilter, &trace );

	if ( trace.fraction < 1.0f &&
		trace.m_pEnt &&
		trace.m_pEnt->IsBaseObject() &&
		trace.m_pEnt->GetTeamNumber() == GetTeamNumber() )
	{
		CBaseObject *pObject = static_cast<CBaseObject *>( trace.m_pEnt );
		if ( pObject->GetBuilder() != this )
			return false;

		if ( pObject->IsBuilding() || pObject->IsDisabled() )
			return false;
		
		// collect resources from this building and destroy
		pObject->DismantleObject( this );

		SpeakConceptIfAllowed( MP_CONCEPT_PICKUP_BUILDING );

		m_flNextCarryTalkTime = gpGlobals->curtime + RandomFloat( 6.0f, 12.0f );

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::TryToPickupBuilding( void )
{
	Vector vecForward;
	AngleVectors( EyeAngles(), &vecForward );
	Vector vecSwingStart = Weapon_ShootPosition();
	// 5500 with Rescue Ranger.
	float flRange = OBJECT_PICKUP_DISTANCE;
	Vector vecSwingEnd = vecSwingStart + vecForward * flRange;

	// only trace against objects

	// See if we hit anything.
	trace_t trace;

	CTraceFilterIgnorePlayers traceFilter( NULL, COLLISION_GROUP_NONE );
	UTIL_TraceLine( vecSwingStart, vecSwingEnd, MASK_SOLID, &traceFilter, &trace );

	if( trace.fraction < 1.0f &&
		trace.m_pEnt &&
		trace.m_pEnt->IsBaseObject() &&
		trace.m_pEnt->GetTeamNumber() == GetTeamNumber() )
	{
		CBaseObject *pObject = dynamic_cast<CBaseObject *>(trace.m_pEnt);
		if( CanPickupBuilding( pObject ) )
		{
			CTFWeaponBase *pWpn = Weapon_OwnsThisID( TF_WEAPON_BUILDER );

			if( pWpn )
			{
				CTFWeaponBuilder *pBuilder = dynamic_cast<CTFWeaponBuilder *>(pWpn);

				// Is this the builder that builds the object we're looking for?
				if( pBuilder )
				{
					pObject->MakeCarriedObject( this );

					pBuilder->SetSubType( pObject->ObjectType() );
					pBuilder->SetObjectMode( pObject->GetObjectMode() );

					SpeakConceptIfAllowed( MP_CONCEPT_PICKUP_BUILDING );

					// try to switch to this weapon
					Weapon_Switch( pBuilder );

					m_flNextCarryTalkTime = gpGlobals->curtime + RandomFloat( 6.0f, 12.0f );

					return true;
				}
			}
		}
	}
	return false;
}

void CTFPlayerShared::SetCarriedObject(CBaseObject* pObj)
{
	if (pObj)
	{
		m_bCarryingObject = true;
		m_hCarriedObject = pObj;
	}
	else
	{
		m_bCarryingObject = false;
		m_hCarriedObject = NULL;
	}

	m_pOuter->TeamFortress_SetSpeed();
}

CBaseObject* CTFPlayerShared::GetCarriedObject(void)
{
	CBaseObject* pObj = m_hCarriedObject.Get();
	return pObj;
}

#endif

#ifdef PF2
int CTFPlayer::GetPrimedState( void )
{
	return m_Shared.m_iPrimed;
}
void CTFPlayer::SetPrimedState( int iState )
{
	m_Shared.m_iPrimed.Set( iState );
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Weapons can call this on secondary attack and it will link to the class
// ability
//-----------------------------------------------------------------------------
// PF NOTE - If we're keeping the delayed knife convar we'll need to reset the knife
bool CTFPlayer::DoClassSpecialSkill( void )
{
	bool bDoSkill = false;

	switch( GetPlayerClass()->GetClassIndex() )
	{
	case TF_CLASS_SPY:
		{
			if (Weapon_OwnsThisID( TF_WEAPON_INVIS ))
			{
				if (m_Shared.m_flStealthNextChangeTime <= gpGlobals->curtime)
				{
					// Toggle invisibility
					if (m_Shared.InCond( TF_COND_STEALTHED ))
					{
#ifdef GAME_DLL
						m_Shared.FadeInvis( tf_spy_invis_unstealth_time.GetFloat() );
#endif
						bDoSkill = true;
					}
					else if (CanGoInvisible() && (m_Shared.GetSpyCloakMeter() > 8.0f))	// must have over 10% cloak to start
					{
#ifdef GAME_DLL
						m_Shared.AddCond( TF_COND_STEALTHED );
#endif
						bDoSkill = true;
						CTFKnife *pKnife = dynamic_cast<CTFKnife*>(Weapon_OwnsThisID( TF_WEAPON_KNIFE ));
						if (pKnife)
						{
							pKnife->WeaponReset();
						}
					}

					if (bDoSkill)
						m_Shared.m_flStealthNextChangeTime = gpGlobals->curtime + 0.5;
				}
			}
		}
		break;

	case TF_CLASS_DEMOMAN:
		{
			CTFPipebombLauncher *pPipebombLauncher = static_cast<CTFPipebombLauncher*>( Weapon_OwnsThisID( TF_WEAPON_PIPEBOMBLAUNCHER ) );

			if ( pPipebombLauncher )
			{
				pPipebombLauncher->SecondaryAttack();
			}
		}
		bDoSkill = true;
		break;
	case TF_CLASS_ENGINEER:
	{
		bDoSkill = false;
#ifdef GAME_DLL
		if ( pf_dismantle_buildings.GetBool() )
			bDoSkill = TryToDismantleBuilding();
		else if ( pf_haul_buildings.GetBool() )
			bDoSkill = TryToPickupBuilding();
#endif
	}
	break;

	default:
		break;
	}

	return bDoSkill;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::CanGoInvisible( void )
{
	if ( HasItem() && GetItem()->GetItemID() == TF_ITEM_CAPTURE_FLAG )
	{
		HintMessage( HINT_CANNOT_CLOAK_WITH_FLAG );
		return false;
	}
#ifdef PF2
	if (GetPrimedState())
		return false;
#endif
	
	CTFGameRules *pRules = TFGameRules();

	Assert( pRules );

	if ( ( pRules->State_Get() == GR_STATE_TEAM_WIN ) && ( pRules->GetWinningTeam() != GetTeamNumber() ) )
	{
		return false;
	}

	return true;
}

//ConVar testclassviewheight( "testclassviewheight", "0", FCVAR_DEVELOPMENTONLY );
//Vector vecTestViewHeight(0,0,0);

//-----------------------------------------------------------------------------
// Purpose: Return class-specific standing eye height
//-----------------------------------------------------------------------------
const Vector& CTFPlayer::GetClassEyeHeight( void )
{
	CTFPlayerClass *pClass = GetPlayerClass();

	if ( !pClass )
		return VEC_VIEW;

	//if ( testclassviewheight.GetFloat() > 0 )
	//{
	//	vecTestViewHeight.z = test.GetFloat();
	//	return vecTestViewHeight;
	//}

	int iClassIndex = pClass->GetClassIndex();

	if ( iClassIndex < TF_FIRST_NORMAL_CLASS || iClassIndex > TF_LAST_NORMAL_CLASS )
		return VEC_VIEW;

	return g_TFClassViewVectors[pClass->GetClassIndex()];
}


CTFWeaponBase *CTFPlayer::Weapon_OwnsThisID( int iWeaponID )
{
	for (int i = 0;i < WeaponCount(); i++) 
	{
		CTFWeaponBase *pWpn = ( CTFWeaponBase *)GetWeapon( i );

		if ( pWpn == NULL )
			continue;

		if ( pWpn->GetWeaponID() == iWeaponID )
		{
			return pWpn;
		}
	}

	return NULL;
}

CTFWeaponBase *CTFPlayer::Weapon_GetWeaponByType( int iType )
{
	for (int i = 0;i < WeaponCount(); i++) 
	{
		CTFWeaponBase *pWpn = ( CTFWeaponBase *)GetWeapon( i );

		if ( pWpn == NULL )
			continue;

		int iWeaponRole = pWpn->GetTFWpnData().m_iWeaponType;
		int	iClass = GetPlayerClass()->GetClassIndex();

		if (pWpn->GetTFWpnData().m_iClassWeaponType[iClass] >= 0)
			iWeaponRole = pWpn->GetTFWpnData().m_iClassWeaponType[iClass];

		if ( iWeaponRole == iType )
		{
			return pWpn;
		}
	}

	return NULL;

}

void CTFPlayer::PlayStepSound( Vector &vecOrigin, surfacedata_t *psurface, float fvol, bool force )
{
#ifdef CLIENT_DLL
	// Don't make predicted footstep sounds in third person, animevents will take care of that.
	if ( prediction->InPrediction() && C_BasePlayer::ShouldDrawLocalPlayer() )
		return;
#endif

	BaseClass::PlayStepSound( vecOrigin, psurface, fvol, force );
}

#ifdef PF2
#define CONC_CLAMP_MAX 4.0f
#define CONC_MULTIPLIER 9
//-----------------------------------------------------------------------------
// Purpose: Return an angle for use when concussed
// Notes: Called in CBasePlayer::CalcPlayerView
//-----------------------------------------------------------------------------
QAngle CTFPlayer::ConcAngles( float flMultiOverride /*= -1*/)
{
	// tfPlayer->EyeAngles x max 89, min -89 (up and down)
	QAngle qConcAngles = QAngle(0, 0, 0);
	
	if( m_Shared.InCond( TF_COND_DIZZY ) )
	{
		float flDizzyTimeLeft = m_Shared.m_flConcussionTime - gpGlobals->curtime;
		float multiplier = flMultiOverride >= 0.0f ? flMultiOverride : CONC_MULTIPLIER;
		VectorAdd( qConcAngles, QAngle(
			sin( flDizzyTimeLeft ) * (multiplier * Clamp<float>( flDizzyTimeLeft, 0.0f, multiplier/2.0f )),
			cos( flDizzyTimeLeft ) * (multiplier * Clamp<float>( flDizzyTimeLeft, 0.0f, multiplier/2.0f )),
			0 ),
			qConcAngles );
	}
	return qConcAngles;
}

extern ConVar tf_maxspeed;
//-----------------------------------------------------------------------------
// Purpose: So we can smoothly and predition friendly-ly adjust the player speed
//-----------------------------------------------------------------------------
float CTFPlayer::GetPlayerMaxSpeed()
{
	// player max speed is the lower limit of m_flMaxSpeed and tf_maxspeed
	float fMaxSpeed = tf_maxspeed.GetFloat();
	if( MaxSpeed() > 0.0f && MaxSpeed() < fMaxSpeed )
		fMaxSpeed = MaxSpeed();

	if( m_Shared.InCond( TF_COND_TRANQUILIZED ) )
	{
		float flTranqTime = m_Shared.m_flSlowedTime.Get(0) - gpGlobals->curtime;
		float flSpeedMulti = flTranqTime / TF_TRANQUILLIZED_DURATION;
		if( flSpeedMulti > pf_tranq_speed_ratio.GetFloat() )
			flSpeedMulti = pf_tranq_speed_ratio.GetFloat();

		fMaxSpeed -= fMaxSpeed*flSpeedMulti;
	}

	if( m_Shared.InCond( TF_COND_LEG_DAMAGED ) )
	{
		float flTranqTime = m_Shared.m_flSlowedTime.Get( 1 ) - gpGlobals->curtime;
		float flSpeedMulti = flTranqTime / TF_BROKEN_LEG_DURATION;
		if( flSpeedMulti > pf_brokenleg_speed_ratio.GetFloat() )
			flSpeedMulti = pf_brokenleg_speed_ratio.GetFloat();

		fMaxSpeed -= fMaxSpeed * flSpeedMulti;
	}

	if( m_Shared.InCond( TF_COND_LEG_DAMAGED_GUNSHOT ) )
	{
		float flTranqTime = m_Shared.m_flSlowedTime.Get( 2 ) - gpGlobals->curtime;
		float flSpeedMulti = flTranqTime / TF_BROKEN_LEG_GUNSHOT_DURATION;
		if( flSpeedMulti > pf_brokenleg_speed_ratio.GetFloat() )
			flSpeedMulti = pf_brokenleg_speed_ratio.GetFloat();

		fMaxSpeed -= fMaxSpeed * flSpeedMulti;
	}

	return fMaxSpeed;
}
#endif
