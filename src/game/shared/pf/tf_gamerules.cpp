//========= Copyright © 1996-2004, Valve LLC, All rights reserved. ============
//
// Purpose: The TF Game rules 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tf_gamerules.h"
#include "ammodef.h"
#include "KeyValues.h"
#include "tf_weaponbase.h"
#include "time.h"
#include "pf_cvars.h"
#ifdef CLIENT_DLL
	#include <game/client/iviewport.h>
	#include "c_tf_player.h"
	#include "c_tf_objective_resource.h"
#else
	#include "basemultiplayerplayer.h"
	#include "voice_gamemgr.h"
	#include "items.h"
	#include "team.h"
	#include "tf_bot_temp.h"
	#include "tf_player.h"
	#include "tf_team.h"
	#include "player_resource.h"
	#include "entity_tfstart.h"
	#include "filesystem.h"
	#include "tf_obj.h"
	#include "tf_objective_resource.h"
	#include "tf_player_resource.h"
	#include "team_control_point_master.h"
	#include "playerclass_info_parse.h"
	#include "team_control_point_master.h"
	#include "coordsize.h"
	#include "entity_healthkit.h"
	#include "tf_gamestats.h"
	#include "entity_capture_flag.h"
	#include "tf_player_resource.h"
	#include "tf_obj_sentrygun.h"
	#include "tier0/icommandline.h"
	#include "activitylist.h"
	#include "AI_ResponseSystem.h"
	#include "hl2orange.spa.h"
	#include "hltvdirector.h"
	#include "vote_controller.h"
	#include "tf_voteissues.h"
	#include "tf_logic_hybrid_ctf_cp.h"
	#include "pf_logic_multiflag.h"
	#include "tf_weaponbase_grenadeproj.h"
	#include "eventqueue.h"
	#include "team_train_watcher.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"
#include <tf/tf_weapon_grenade_pipebomb.h>

#define ITEM_RESPAWN_TIME	10.0f
#define LIVE_TF2_MAX_INTERP_RATIO "5"

enum
{
	BIRTHDAY_RECALCULATE,
	BIRTHDAY_OFF,
	BIRTHDAY_ON,
};

static int g_TauntCamAchievements[] = 
{
	0,		// TF_CLASS_UNDEFINED

	0,		// TF_CLASS_SCOUT,	
	0,		// TF_CLASS_SNIPER,
	0,		// TF_CLASS_SOLDIER,
	0,		// TF_CLASS_DEMOMAN,
	0,		// TF_CLASS_MEDIC,
	0,		// TF_CLASS_HEAVYWEAPONS,
	0,		// TF_CLASS_PYRO,
	0,		// TF_CLASS_SPY,
	0,		// TF_CLASS_ENGINEER,

	0,		// TF_CLASS_CIVILIAN,
	0,		// TF_CLASS_COUNT_ALL,
};

extern ConVar mp_capstyle;
extern ConVar sv_turbophysics;
extern ConVar sv_vote_kick_ban_duration;
extern ConVar tf_arena_max_streak;

ConVar tf_caplinear( "tf_caplinear", "1", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "If set to 1, teams must capture control points linearly." );
ConVar tf_stalematechangeclasstime( "tf_stalematechangeclasstime", "20", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "Amount of time that players are allowed to change class in stalemates." );
ConVar tf_birthday( "tf_birthday", "0", FCVAR_NOTIFY | FCVAR_REPLICATED );

#ifdef GAME_DLL
// TF overrides the default value of this convar

ConVar tf_arena_force_class( "tf_arena_force_class", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Force random classes in arena." );
ConVar tf_arena_first_blood( "tf_arena_first_blood", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Toggles first blood criticals" );
ConVar tf_arena_first_blood_length( "tf_arena_first_blood_length", "5.0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Duration of first blood criticals" );
ConVar mp_waitingforplayers_time( "mp_waitingforplayers_time", (IsX360()?"15":"30"), FCVAR_GAMEDLL | FCVAR_DEVELOPMENTONLY, "WaitingForPlayers time length in seconds" );
ConVar tf_gravetalk( "tf_gravetalk", "1", FCVAR_NOTIFY, "Allows living players to hear dead players using text/voice chat." );
ConVar tf_spectalk( "tf_spectalk", "1", FCVAR_NOTIFY, "Allows living players to hear spectators using text chat." );

ConVar tf_arena_override_cap_enable_time( "tf_arena_override_cap_enable_time", "-1", FCVAR_REPLICATED | FCVAR_NOTIFY, "Overrides the time (in seconds) it takes for the capture point to become enable, -1 uses the level designer specified time." );

ConVar tf_gamemode_arena( "tf_gamemode_arena", "0", FCVAR_NOTIFY | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );

ConVar pf_civ_death_ends_round( "pf_civ_death_ends_round", "1", FCVAR_NOTIFY, "Civilian death causes a round loss for the bodyguards." );
ConVar pf_use_escort_class_restrictions( "pf_use_escort_class_restrictions", "1", FCVAR_NOTIFY, "Use old class limits for escort." );
#endif

#ifdef GAME_DLL
void pf_grenades_regenerate_helper( IConVar* var, const char* pOldValue, float flOldValue )
{
	if ( TFGameRules() )
		TFGameRules()->UpdateGrenadeRegenMode();
}
#endif
ConVar pf_grenades_regenerate( "pf_grenades_regenerate", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Use grenade regeneration"
#ifdef GAME_DLL
	, pf_grenades_regenerate_helper
#endif
);

#ifdef GAME_DLL
void ExtendLevel(const CCommand& args)
{
	if (!UTIL_IsCommandIssuedByServerAdmin())
		return;
	TFGameRules()->ExtendLevel(atof(args[1])*60);
}


ConCommand cc_extendlevel("extendlevel", ExtendLevel, "Extend the level in minutes");

void ValidateCapturesPerRound( IConVar *pConVar, const char *oldValue, float flOldValue )
{
	ConVarRef var( pConVar );

	if ( var.GetInt() <= 0 )
	{
		// reset the flag captures being played in the current round
		int nTeamCount = TFTeamMgr()->GetTeamCount();
		for ( int iTeam = FIRST_GAME_TEAM; iTeam < nTeamCount; ++iTeam )
		{
			CTFTeam *pTeam = GetGlobalTFTeam( iTeam );
			if ( !pTeam )
				continue;

			pTeam->SetFlagCaptures( 0 );
		}
	}
}
#endif	

ConVar tf_flag_caps_per_round( "tf_flag_caps_per_round", "3", FCVAR_REPLICATED, "Number of flag captures per round on CTF maps. Set to 0 to disable.", true, 0, true, 9
#ifdef GAME_DLL
							  , ValidateCapturesPerRound
#endif
							  );


/**
 * Player hull & eye position for standing, ducking, etc.  This version has a taller
 * player height, but goldsrc-compatible collision bounds.
 */
static CViewVectors g_TFViewVectors(
	Vector( 0, 0, 72 ),		//VEC_VIEW (m_vView) eye position
							
	Vector(-24, -24, 0 ),	//VEC_HULL_MIN (m_vHullMin) hull min
	Vector( 24,  24, 82 ),	//VEC_HULL_MAX (m_vHullMax) hull max
												
	Vector(-24, -24, 0 ),	//VEC_DUCK_HULL_MIN (m_vDuckHullMin) duck hull min
	Vector( 24,  24, 55 ),	//VEC_DUCK_HULL_MAX	(m_vDuckHullMax) duck hull max
	Vector( 0, 0, 45 ),		//VEC_DUCK_VIEW		(m_vDuckView) duck view
												
	Vector( -10, -10, -10 ),	//VEC_OBS_HULL_MIN	(m_vObsHullMin) observer hull min
	Vector(  10,  10,  10 ),	//VEC_OBS_HULL_MAX	(m_vObsHullMax) observer hull max
												
	Vector( 0, 0, 14 )		//VEC_DEAD_VIEWHEIGHT (m_vDeadViewHeight) dead view height
);							

Vector g_TFClassViewVectors[11] =
{
	Vector( 0, 0, 72 ),		// TF_CLASS_UNDEFINED

	Vector( 0, 0, 65 ),		// TF_CLASS_SCOUT,			// TF_FIRST_NORMAL_CLASS
	Vector( 0, 0, 75 ),		// TF_CLASS_SNIPER,
	Vector( 0, 0, 68 ),		// TF_CLASS_SOLDIER,
	Vector( 0, 0, 68 ),		// TF_CLASS_DEMOMAN,
	Vector( 0, 0, 75 ),		// TF_CLASS_MEDIC,
	Vector( 0, 0, 75 ),		// TF_CLASS_HEAVYWEAPONS,
	Vector( 0, 0, 68 ),		// TF_CLASS_PYRO,
	Vector( 0, 0, 75 ),		// TF_CLASS_SPY,
	Vector( 0, 0, 68 ),		// TF_CLASS_ENGINEER,		// TF_LAST_NORMAL_CLASS
	Vector( 0, 0, 70)		// TF_CLASS_CIVILIAN
};

const CViewVectors *CTFGameRules::GetViewVectors() const
{
	return &g_TFViewVectors;
}

REGISTER_GAMERULES_CLASS( CTFGameRules );

BEGIN_NETWORK_TABLE_NOBASE( CTFGameRules, DT_TFGameRules )
#ifdef CLIENT_DLL

	RecvPropInt( RECVINFO( m_nGameType ) ),
	RecvPropBool( RECVINFO( m_bCTFCPHybrid ) ),
	RecvPropInt( RECVINFO( m_nHudType ) ),
	RecvPropTime( RECVINFO( m_flCapturePointEnableTime ) ),
	RecvPropBool( RECVINFO( m_bPlayingKoth ) ),
	RecvPropEHandle( RECVINFO( m_hRedKothTimer ) ), 
	RecvPropEHandle( RECVINFO( m_hBlueKothTimer ) ),
	RecvPropString( RECVINFO( m_pszTeamGoalStringRed ) ),
	RecvPropString( RECVINFO( m_pszTeamGoalStringBlue ) ),
	RecvPropInt( RECVINFO( m_bGrenadesRegenerate ) ),
	RecvPropBool( RECVINFO( m_bCTFMultiFlag )),
	RecvPropArray3( RECVINFO_ARRAY( m_bControlPointsNeedingFlags ), RecvPropBool( RECVINFO( m_bControlPointsNeedingFlags[0] ) ) ),
	RecvPropBool( RECVINFO( m_bIsPayloadRace )),

#else

	SendPropInt( SENDINFO( m_nGameType ), 3, SPROP_UNSIGNED ),
	SendPropBool( SENDINFO( m_bCTFCPHybrid ) ),
	SendPropInt( SENDINFO( m_nHudType ) ),	
	SendPropString( SENDINFO( m_pszTeamGoalStringRed ) ),
	SendPropString( SENDINFO( m_pszTeamGoalStringBlue ) ),
	SendPropTime( SENDINFO( m_flCapturePointEnableTime ) ),
	SendPropBool( SENDINFO( m_bPlayingKoth ) ),
	SendPropEHandle( SENDINFO( m_hRedKothTimer ) ), 
	SendPropEHandle( SENDINFO( m_hBlueKothTimer ) ),
	SendPropInt( SENDINFO( m_bGrenadesRegenerate ) ),
	SendPropBool( SENDINFO( m_bCTFMultiFlag )),
	SendPropArray3( SENDINFO_ARRAY3( m_bControlPointsNeedingFlags ), SendPropBool( SENDINFO_ARRAY( m_bControlPointsNeedingFlags ) ) ),
	SendPropBool( SENDINFO( m_bIsPayloadRace ) ),

#endif
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_gamerules, CTFGameRulesProxy );
IMPLEMENT_NETWORKCLASS_ALIASED( TFGameRulesProxy, DT_TFGameRulesProxy )

#ifdef CLIENT_DLL
	void RecvProxy_TFGameRules( const RecvProp *pProp, void **pOut, void *pData, int objectID )
	{
		CTFGameRules *pRules = TFGameRules();
		Assert( pRules );
		*pOut = pRules;
	}

	BEGIN_RECV_TABLE( CTFGameRulesProxy, DT_TFGameRulesProxy )
		RecvPropDataTable( "tf_gamerules_data", 0, 0, &REFERENCE_RECV_TABLE( DT_TFGameRules ), RecvProxy_TFGameRules )
	END_RECV_TABLE()
#else
	void *SendProxy_TFGameRules( const SendProp *pProp, const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID )
	{
		CTFGameRules *pRules = TFGameRules();
		Assert( pRules );
		pRecipients->SetAllRecipients();
		return pRules;
	}

	BEGIN_SEND_TABLE( CTFGameRulesProxy, DT_TFGameRulesProxy )
		SendPropDataTable( "tf_gamerules_data", 0, &REFERENCE_SEND_TABLE( DT_TFGameRules ), SendProxy_TFGameRules )
	END_SEND_TABLE()
#endif

#ifdef GAME_DLL
BEGIN_DATADESC( CTFGameRulesProxy )

	DEFINE_KEYFIELD(m_iHud_Type, FIELD_INTEGER, "hud_type"),

	// Inputs.
	DEFINE_KEYFIELD( m_nDisableGrenadesResupplying, FIELD_INTEGER, "disablegrenaderegen" ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetRedTeamRespawnWaveTime", InputSetRedTeamRespawnWaveTime ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetBlueTeamRespawnWaveTime", InputSetBlueTeamRespawnWaveTime ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "AddRedTeamRespawnWaveTime", InputAddRedTeamRespawnWaveTime ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "AddBlueTeamRespawnWaveTime", InputAddBlueTeamRespawnWaveTime ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetRedTeamGoalString", InputSetRedTeamGoalString ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetBlueTeamGoalString", InputSetBlueTeamGoalString ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetRedTeamRole", InputSetRedTeamRole ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetBlueTeamRole", InputSetBlueTeamRole ),

	DEFINE_INPUTFUNC( FIELD_VOID, "SetRedKothClockActive", InputSetRedKothClockActive ),
	DEFINE_INPUTFUNC( FIELD_VOID, "SetBlueKothClockActive", InputSetBlueKothClockActive ),
	DEFINE_INPUTFUNC(FIELD_STRING, "PlayVORed", InputPlayVORed),
	DEFINE_INPUTFUNC(FIELD_STRING, "PlayVOBlue", InputPlayVOBlue),
	DEFINE_INPUTFUNC( FIELD_STRING, "PlayVO", InputPlayVO ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetRedTeamRespawnWaveTime( inputdata_t &inputdata )
{
	TFGameRules()->SetTeamRespawnWaveTime( TF_TEAM_RED, inputdata.value.Float() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetBlueTeamRespawnWaveTime( inputdata_t &inputdata )
{
	TFGameRules()->SetTeamRespawnWaveTime( TF_TEAM_BLUE, inputdata.value.Float() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputAddRedTeamRespawnWaveTime( inputdata_t &inputdata )
{
	TFGameRules()->AddTeamRespawnWaveTime( TF_TEAM_RED, inputdata.value.Float() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputAddBlueTeamRespawnWaveTime( inputdata_t &inputdata )
{
	TFGameRules()->AddTeamRespawnWaveTime( TF_TEAM_BLUE, inputdata.value.Float() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetRedTeamGoalString( inputdata_t &inputdata )
{
	TFGameRules()->SetTeamGoalString( TF_TEAM_RED, inputdata.value.String() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetBlueTeamGoalString( inputdata_t &inputdata )
{
	TFGameRules()->SetTeamGoalString( TF_TEAM_BLUE, inputdata.value.String() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetRedTeamRole( inputdata_t &inputdata )
{
	CTFTeam *pTeam = TFTeamMgr()->GetTeam( TF_TEAM_RED );
	if ( pTeam )
	{
		pTeam->SetRole( inputdata.value.Int() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetBlueTeamRole( inputdata_t &inputdata )
{
	CTFTeam *pTeam = TFTeamMgr()->GetTeam( TF_TEAM_BLUE );
	if ( pTeam )
	{
		pTeam->SetRole( inputdata.value.Int() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetRedKothClockActive(inputdata_t& inputdata)
{
	variant_t sVariant;
	if (TFGameRules() && TFGameRules()->GetRedKothRoundTimer())
	{
		TFGameRules()->GetRedKothRoundTimer()->AcceptInput( "Resume", NULL, NULL, sVariant, 0 );
	}
	if( TFGameRules()->GetBlueKothRoundTimer() )
	{
		TFGameRules()->GetBlueKothRoundTimer()->AcceptInput( "Pause", NULL, NULL, sVariant, 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetBlueKothClockActive(inputdata_t& inputdata)
{
	variant_t sVariant;
	if( TFGameRules() && TFGameRules()->GetBlueKothRoundTimer() )
	{
		TFGameRules()->GetBlueKothRoundTimer()->AcceptInput( "Resume", NULL, NULL, sVariant, 0 );
	}
	if( TFGameRules()->GetRedKothRoundTimer() )
	{
		TFGameRules()->GetRedKothRoundTimer()->AcceptInput( "Pause", NULL, NULL, sVariant, 0 );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputPlayVO(inputdata_t& inputdata)
{
	if (TFGameRules())
	{
		TFGameRules()->BroadcastSound(255, inputdata.value.String());
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputPlayVORed(inputdata_t& inputdata)
{
	if (TFGameRules())
	{
		TFGameRules()->BroadcastSound(TF_TEAM_RED, inputdata.value.String());
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputPlayVOBlue(inputdata_t& inputdata)
{
	if (TFGameRules())
	{
		TFGameRules()->BroadcastSound(TF_TEAM_BLUE, inputdata.value.String());
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::Activate()
{
	TFGameRules()->Activate();
	
	TFGameRules()->SetGrenadesResupply( m_nDisableGrenadesResupplying );
	TFGameRules()->SetHudType(m_iHud_Type);

	BaseClass::Activate();
}



class CKothLogic : public CPointEntity
{
public:
	DECLARE_CLASS(CKothLogic, CPointEntity);
	DECLARE_DATADESC();

	CKothLogic();

	virtual void	InputAddBlueTimer(inputdata_t& inputdata);
	virtual void	InputAddRedTimer(inputdata_t& inputdata);
	virtual void	InputSetBlueTimer(inputdata_t& inputdata);
	virtual void	InputSetRedTimer(inputdata_t& inputdata);
	virtual void	InputRoundSpawn(inputdata_t& inputdata);
	virtual void	InputRoundActivate(inputdata_t& inputdata);

private:
	int m_iTimerLength;
	int m_iUnlockPoint;

};

BEGIN_DATADESC( CArenaLogic )
DEFINE_KEYFIELD( m_flTimeToEnableCapPoint, FIELD_FLOAT, "CapEnableDelay" ),

DEFINE_INPUTFUNC( FIELD_VOID, "RoundActivate", InputRoundActivate ),

DEFINE_OUTPUT( m_OnArenaRoundStart, "OnArenaRoundStart" ),
DEFINE_OUTPUT( m_OnCapEnabled, "OnCapEnabled" ),

//DEFINE_THINKFUNC( ArenaLogicThink ),
END_DATADESC()


BEGIN_DATADESC(CKothLogic)
DEFINE_KEYFIELD(m_iTimerLength, FIELD_INTEGER, "timer_length"),
DEFINE_KEYFIELD(m_iUnlockPoint, FIELD_INTEGER, "unlock_point"),

// Inputs.
DEFINE_INPUTFUNC(FIELD_INTEGER, "AddBlueTimer", InputAddBlueTimer),
DEFINE_INPUTFUNC(FIELD_INTEGER, "AddRedTimer", InputAddRedTimer),
DEFINE_INPUTFUNC(FIELD_INTEGER, "SetBlueTimer", InputSetBlueTimer),
DEFINE_INPUTFUNC(FIELD_INTEGER, "SetRedTimer", InputSetRedTimer),
DEFINE_INPUTFUNC(FIELD_VOID, "RoundSpawn", InputRoundSpawn),
DEFINE_INPUTFUNC(FIELD_VOID, "RoundActivate", InputRoundActivate),
END_DATADESC()

LINK_ENTITY_TO_CLASS( tf_logic_arena, CArenaLogic );
LINK_ENTITY_TO_CLASS(tf_logic_koth, CKothLogic);


//=============================================================================
// Arena Logic
//=============================================================================

void CArenaLogic::Spawn( void )
{
	BaseClass::Spawn();
	//SetThink( &CArenaLogic::ArenaLogicThink );
	//SetNextThink( gpGlobals->curtime );
}

void CArenaLogic::OnCapEnabled( void )
{
	if( m_bCapUnlocked == false )
	{
		m_bCapUnlocked = true;
		m_OnCapEnabled.FireOutput( this, this );
	}
}

/*void CArenaLogic::ArenaLogicThink( void )
{
	// Live TF2 checks m_fCapEnableTime from TFGameRules here.
	SetNextThink( gpGlobals->curtime + 0.1 );

#ifdef GAME_DLL
	if ( TFGameRules()->State_Get() == GR_STATE_STALEMATE )
	{
		m_bCapUnlocked = true;
		m_OnCapEnabled.FireOutput(this, this);
	}
#endif
}*/

void CArenaLogic::InputRoundActivate( inputdata_t &inputdata )
{
	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
	if( !pMaster )
		return;

	for( int i = 0; i < pMaster->GetNumPoints(); i++ )
	{
		CTeamControlPoint *pPoint = pMaster->GetControlPoint( i );

		variant_t sVariant;
		sVariant.SetInt( m_flTimeToEnableCapPoint );
		pPoint->AcceptInput( "SetLocked", NULL, NULL, sVariant, 0 );
		g_EventQueue.AddEvent( pPoint, "SetUnlockTime", sVariant, 0.1, NULL, NULL );
	}
}

CKothLogic::CKothLogic()
{
	m_iTimerLength = 180;
	m_iUnlockPoint = 30;
}

void CKothLogic::InputRoundSpawn(inputdata_t& inputdata)
{
	variant_t sVariant;

	if (TFGameRules())
	{
		sVariant.SetInt(m_iTimerLength);

		TFGameRules()->SetBlueKothRoundTimer((CTeamRoundTimer*)CBaseEntity::Create("team_round_timer", vec3_origin, vec3_angle));

		if (TFGameRules()->GetBlueKothRoundTimer())
		{
			TFGameRules()->GetBlueKothRoundTimer()->SetName(MAKE_STRING("zz_blue_koth_timer"));
			TFGameRules()->GetBlueKothRoundTimer()->SetShowInHud(true);
			TFGameRules()->GetBlueKothRoundTimer()->AcceptInput("SetTime", NULL, NULL, sVariant, 0);
			TFGameRules()->GetBlueKothRoundTimer()->AcceptInput("Pause", NULL, NULL, sVariant, 0);
			TFGameRules()->GetBlueKothRoundTimer()->ChangeTeam(TF_TEAM_BLUE);
		}

		TFGameRules()->SetRedKothRoundTimer((CTeamRoundTimer*)CBaseEntity::Create("team_round_timer", vec3_origin, vec3_angle));

		if (TFGameRules()->GetRedKothRoundTimer())
		{
			TFGameRules()->GetRedKothRoundTimer()->SetName(MAKE_STRING("zz_red_koth_timer"));
			TFGameRules()->GetRedKothRoundTimer()->SetShowInHud(true);
			TFGameRules()->GetRedKothRoundTimer()->AcceptInput("SetTime", NULL, NULL, sVariant, 0);
			TFGameRules()->GetRedKothRoundTimer()->AcceptInput("Pause", NULL, NULL, sVariant, 0);
			TFGameRules()->GetRedKothRoundTimer()->ChangeTeam(TF_TEAM_RED);
		}

	}
}

void CKothLogic::InputRoundActivate(inputdata_t& inputdata)
{
	CTeamControlPointMaster* pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
	if (!pMaster)
		return;

	for (int i = 0; i < pMaster->GetNumPoints(); i++)
	{
		CTeamControlPoint* pPoint = pMaster->GetControlPoint(i);

		variant_t sVariant;
		sVariant.SetInt(m_iUnlockPoint);
		pPoint->AcceptInput("SetLocked", NULL, NULL, sVariant, 0);
		g_EventQueue.AddEvent(pPoint, "SetUnlockTime", sVariant, 0.1, NULL, NULL);
	}
}

void CKothLogic::InputAddBlueTimer(inputdata_t& inputdata)
{
	if (TFGameRules() && TFGameRules()->GetBlueKothRoundTimer())
	{
		TFGameRules()->GetBlueKothRoundTimer()->AddTimerSeconds(inputdata.value.Int());
	}
}

void CKothLogic::InputAddRedTimer(inputdata_t& inputdata)
{
	if (TFGameRules() && TFGameRules()->GetRedKothRoundTimer())
	{
		TFGameRules()->GetRedKothRoundTimer()->AddTimerSeconds(inputdata.value.Int());
	}
}

void CKothLogic::InputSetBlueTimer(inputdata_t& inputdata)
{
	if (TFGameRules() && TFGameRules()->GetBlueKothRoundTimer())
	{
		TFGameRules()->GetBlueKothRoundTimer()->SetTimeRemaining(inputdata.value.Int());
	}
}

void CKothLogic::InputSetRedTimer(inputdata_t& inputdata)
{
	if (TFGameRules() && TFGameRules()->GetRedKothRoundTimer())
	{
		TFGameRules()->GetRedKothRoundTimer()->SetTimeRemaining(inputdata.value.Int());
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::RestoreActiveTimer( void )
{
	BaseClass::RestoreActiveTimer();

	// We need to restore both of our koth timers to be "Shown in the HUD"
	// as waiting for players turns them off and only the first timer, blue, would get restored
	if( !IsInKothMode() )
		return;

	if( GetBlueKothRoundTimer() )
	{
		GetBlueKothRoundTimer()->SetShowInHud( true );
	}

	if( GetRedKothRoundTimer() )
	{
		GetRedKothRoundTimer()->SetShowInHud( true );
	}
}

class CMultipleEscort : public CPointEntity
{
public:
	DECLARE_CLASS(CMultipleEscort, CPointEntity);
};

LINK_ENTITY_TO_CLASS(tf_logic_multiple_escort, CMultipleEscort);
#endif

// (We clamp ammo ourselves elsewhere).
ConVar ammo_max( "ammo_max", "5000", FCVAR_REPLICATED );

#ifndef CLIENT_DLL
ConVar sk_plr_dmg_grenade( "sk_plr_dmg_grenade","0");		// Very lame that the base code needs this defined
#endif

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iDmgType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFGameRules::Damage_IsTimeBased( int iDmgType )
{
	// Damage types that are time-based.
	return ( ( iDmgType & ( DMG_PARALYZE | DMG_NERVEGAS | DMG_DROWNRECOVER ) ) != 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iDmgType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFGameRules::Damage_ShowOnHUD( int iDmgType )
{
	// Damage types that have client HUD art.
	return ( ( iDmgType & ( DMG_DROWN | DMG_BURN | DMG_NERVEGAS | DMG_SHOCK ) ) != 0 );
}
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iDmgType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFGameRules::Damage_ShouldNotBleed( int iDmgType )
{
	// Should always bleed currently.
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGameRules::Damage_GetTimeBased( void )
{
	int iDamage = ( DMG_PARALYZE | DMG_NERVEGAS | DMG_DROWNRECOVER );
	return iDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGameRules::Damage_GetShowOnHud( void )
{
	int iDamage = ( DMG_DROWN | DMG_BURN | DMG_NERVEGAS | DMG_SHOCK );
	return iDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFGameRules::Damage_GetShouldNotBleed( void )
{
	return 0;
}

#ifdef PF2_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGameRules::GrenadesResupply()
{
	return m_nDisableGrenadesResupplying;
}
#endif
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFGameRules::CTFGameRules()
{
#ifdef GAME_DLL
	// Create teams.
	TFTeamMgr()->Init();

	ResetMapTime();

	// Create the team managers
//	for ( int i = 0; i < ARRAYSIZE( teamnames ); i++ )
//	{
//		CTeam *pTeam = static_cast<CTeam*>(CreateEntityByName( "tf_team" ));
//		pTeam->Init( sTeamNames[i], i );
//
//		g_Teams.AddToTail( pTeam );
//	}

	m_flIntermissionEndTime = 0.0f;
	m_flNextPeriodicThink = 0.0f;

	ListenForGameEvent( "teamplay_point_captured" );
	ListenForGameEvent( "teamplay_capture_blocked" );	
	ListenForGameEvent( "teamplay_round_win" );
	ListenForGameEvent( "teamplay_flag_event" );
	ListenForGameEvent( "teamplay_point_unlocked" );

	Q_memset( m_vecPlayerPositions,0, sizeof(m_vecPlayerPositions) );

	m_iPrevRoundState = -1;
	m_iCurrentRoundState = -1;
	m_iCurrentMiniRoundMask = 0;
	m_flTimerMayExpireAt = -1.0f;

	m_bFirstBlood = false;
	m_iArenaTeamCount = 0;

	m_flCapturePointEnableTime = 0;

	// Lets execute a map specific cfg file
	// ** execute this after server.cfg!
	char szCommand[32];
	Q_snprintf( szCommand, sizeof( szCommand ), "exec %s.cfg\n", STRING( gpGlobals->mapname ) );
	engine->ServerCommand( szCommand );

#else // GAME_DLL

	ListenForGameEvent( "game_newmap" );
	
#endif

	// Initialize the game type
	m_nGameType.Set( TF_GAMETYPE_UNDEFINED );

#ifndef PF2
	// Initialize the classes here.
	InitPlayerClasses();
#endif

	// Set turbo physics on.  Do it here for now.
	sv_turbophysics.SetValue( 1 );

	// Initialize the team manager here, etc...

	// If you hit these asserts its because you added or removed a weapon type 
	// and didn't also add or remove the weapon name or damage type from the
	// arrays defined in tf_shareddefs.cpp
	Assert( g_aWeaponDamageTypes[TF_WEAPON_COUNT] == TF_DMG_SENTINEL_VALUE );
	Assert( FStrEq( g_aWeaponNames[TF_WEAPON_COUNT], "TF_WEAPON_COUNT" ) );	

	m_iPreviousRoundWinners = TEAM_UNASSIGNED;
	m_iBirthdayMode = BIRTHDAY_RECALCULATE;

	m_pszTeamGoalStringRed.GetForModify()[0] = '\0';
	m_pszTeamGoalStringBlue.GetForModify()[0] = '\0';

	m_nDisableGrenadesResupplying = 0;

#ifdef PF2_DLL
	m_bCheckForAchievements = false;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::FlagsMayBeCapped( void )
{
	if ( State_Get() != GR_STATE_TEAM_WIN )
		return true;

	return false;
}

#ifdef GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: Determines whether we should allow mp_timelimit to trigger a map change
//-----------------------------------------------------------------------------
bool CTFGameRules::CanChangelevelBecauseOfTimeLimit( void )
{
	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;

	// we only want to deny a map change triggered by mp_timelimit if we're not forcing a map reset,
	// we're playing mini-rounds, and the master says we need to play all of them before changing (for maps like Dustbowl)
	if ( !m_bForceMapReset && pMaster && pMaster->PlayingMiniRounds() && pMaster->ShouldPlayAllControlPointRounds() )
	{
		if ( pMaster->NumPlayableControlPointRounds() )
		{
			return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::CanGoToStalemate( void )
{
	// In CTF, don't go to stalemate if one of the flags isn't at home
	if ( m_nGameType == TF_GAMETYPE_CTF )
	{
		CCaptureFlag *pFlag = dynamic_cast<CCaptureFlag*> ( gEntList.FindEntityByClassname( NULL, "item_teamflag" ) );
		while( pFlag )
		{
			if ( pFlag->IsDropped() || pFlag->IsStolen() )
				return false;

			pFlag = dynamic_cast<CCaptureFlag*> ( gEntList.FindEntityByClassname( pFlag, "item_teamflag" ) );
		}

		// check that one team hasn't won by capping
		if ( CheckCapsPerRound() )
			return false;
	}

	return BaseClass::CanGoToStalemate();
}

// Classnames of entities that are preserved across round restarts
static const char *s_PreserveEnts[] =
{
	"tf_gamerules",
	"tf_team_manager",
	"tf_player_manager",
	"tf_team",
	"tf_objective_resource",
	"keyframe_rope",
	"move_rope",
	"tf_viewmodel",
	"vote_controller",
	"", // END Marker
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::Activate()
{
	m_iBirthdayMode = BIRTHDAY_RECALCULATE;

	m_nGameType.Set( TF_GAMETYPE_UNDEFINED );

	tf_gamemode_arena.SetValue( 0 );

#ifdef PF2_DLL
	UpdateGrenadeRegenMode();
#endif

	CArenaLogic *pArena = dynamic_cast<CArenaLogic *>(gEntList.FindEntityByClassname( NULL, "tf_logic_arena" ));
	if( pArena )
	{
		m_nGameType.Set( TF_GAMETYPE_ARENA );
		
		tf_gamemode_arena.SetValue( 1 );
		Msg( "Executing server arena config file\n", 1 );
		engine->ServerCommand( "exec config_arena.cfg \n" );
		engine->ServerExecute();
		return;
	}

	CCaptureFlag *pFlag = dynamic_cast<CCaptureFlag*> ( gEntList.FindEntityByClassname( NULL, "item_teamflag" ) );
	if ( pFlag )
	{
		m_nGameType.Set( TF_GAMETYPE_CTF );
	}

	if ( g_hControlPointMasters.Count() )
	{
		m_nGameType.Set( TF_GAMETYPE_CP );
	}

	CHybridMap_CTF_CP *pHybrid = dynamic_cast<CHybridMap_CTF_CP*> (gEntList.FindEntityByClassname( NULL, "tf_logic_hybrid_ctf_cp" ));
	if (pHybrid)
	{
		m_bCTFCPHybrid.Set(true);
	}

	CKothLogic *pKoth = dynamic_cast< CKothLogic* > ( gEntList.FindEntityByClassname( NULL, "tf_logic_koth" ) );
	if ( pKoth )
	{
		m_nGameType.Set( TF_GAMETYPE_CP );
		m_bPlayingKoth = true;
		return;
	}

	CTeamTrainWatcher* pTrain = dynamic_cast<CTeamTrainWatcher*> (gEntList.FindEntityByClassname(NULL, "team_train_watcher"));
	if (pTrain)
	{
		m_nGameType.Set(TF_GAMETYPE_ESCORT);

		if( gEntList.FindEntityByClassname( NULL, "tf_logic_multiple_escort" ) )
		{
			SetMultipleTrains( true );
			m_bIsPayloadRace = true;
		}
		return;
	}

	CPFLogic_MultiFlag *pMultiFlag = dynamic_cast<CPFLogic_MultiFlag *> (gEntList.FindEntityByClassname( NULL, "pf_logic_multiflag" ));
	if( pMultiFlag )
	{
		m_bCTFMultiFlag.Set( true );
	}
}

#ifdef PF2_DLL
void CTFGameRules::UpdateGrenadeRegenMode( void )
{
	m_bGrenadesRegenerate.Set( pf_grenades_regenerate.GetBool() );
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::AllowDamage( CBaseEntity *pVictim, const CTakeDamageInfo &info )
{
	bool bRetVal = true;

	if ( ( State_Get() == GR_STATE_TEAM_WIN ) && pVictim )
	{
		if ( pVictim->GetTeamNumber() == GetWinningTeam() )
		{
			CBaseTrigger *pTrigger = dynamic_cast< CBaseTrigger *>( info.GetInflictor() );

			// we don't want players on the winning team to be
			// hurt by team-specific trigger_hurt entities during the bonus time
			if ( pTrigger && pTrigger->UsesFilter() )
			{
				bRetVal = false;
			}
		}
	}

	return bRetVal;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SetTeamGoalString( int iTeam, const char *pszGoal )
{
	if ( iTeam == TF_TEAM_RED )
	{
		if ( !pszGoal || !pszGoal[0] )
		{
			m_pszTeamGoalStringRed.GetForModify()[0] = '\0';
		}
		else
		{
			if ( Q_stricmp( m_pszTeamGoalStringRed.Get(), pszGoal ) )
			{
				Q_strncpy( m_pszTeamGoalStringRed.GetForModify(), pszGoal, MAX_TEAMGOAL_STRING );
			}
		}
	}
	else if ( iTeam == TF_TEAM_BLUE )
	{
		if ( !pszGoal || !pszGoal[0] )
		{
			m_pszTeamGoalStringBlue.GetForModify()[0] = '\0';
		}
		else
		{
			if ( Q_stricmp( m_pszTeamGoalStringBlue.Get(), pszGoal ) )
			{
				Q_strncpy( m_pszTeamGoalStringBlue.GetForModify(), pszGoal, MAX_TEAMGOAL_STRING );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::RoundCleanupShouldIgnore( CBaseEntity *pEnt )
{
	if ( FindInList( s_PreserveEnts, pEnt->GetClassname() ) )
		return true;

	//There has got to be a better way of doing this.
	if ( Q_strstr( pEnt->GetClassname(), "tf_weapon_" ) )
		return true;

	return BaseClass::RoundCleanupShouldIgnore( pEnt );
}

//-------------------------------------------------iFov = clamp( iFov, 75, 90 );----------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::ShouldCreateEntity( const char *pszClassName )
{
	if ( FindInList( s_PreserveEnts, pszClassName ) )
		return false;

	return BaseClass::ShouldCreateEntity( pszClassName );
}

ConVar mp_waitingforplayers_opengates( "mp_waitingforplayers_opengates", "0", FCVAR_GAMEDLL, "Open setup gates while waiting for players");
void CTFGameRules::CleanUpMap( void )
{
	BaseClass::CleanUpMap();

	if ( HLTVDirector() )
	{
		HLTVDirector()->BuildCameraList();
	}

#ifdef PF2_DLL
	CreateSoldierStatue();
	MapEntsSpawnAll();

	// are we automatically determining health pickups giving armor?
	if( pf_healthkit_armor_repair.GetInt() == -1 )
	{
		// check if we have any armor pickups in the level
		CBaseEntity *pPickup = gEntList.FindEntityByClassname( NULL, "item_armor" );
		m_bHealthkitArmorRepair = !pPickup;
	}
	else
	{
		m_bHealthkitArmorRepair = pf_healthkit_armor_repair.GetBool();
	}

	if( mp_waitingforplayers_opengates.GetBool() && IsInWaitingForPlayers() )
	{
		CTeamRoundTimer *pRoundTimer = (CTeamRoundTimer *)gEntList.FindEntityByClassname( NULL, "team_round_timer" );
		if( pRoundTimer )
		{
			variant_t emptyVariant;
			pRoundTimer->FireNamedOutput( "OnSetupFinished", emptyVariant, nullptr, nullptr, 0.0f );
		}
	}
#endif
}

#ifdef PF2_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::CreateSoldierStatue( void )
{
	if( TFGameRules()->IsApril() )
	{
		Vector vecSoldierOrigin;
		QAngle angSoldierAngles;
		KeyValues *pKeyValue = new KeyValues( "SoldierMemorial" );
		if( pKeyValue && pKeyValue->LoadFromFile( filesystem, "scripts/soldiermemorial.txt", "MOD" ) )
		{
			bool bGotOrigin = false, bGotAngles = false;
			KeyValues *pMap = pKeyValue->FindKey( STRING( gpGlobals->mapname ) );
			if( pMap )
			{
				FOR_EACH_VALUE( pMap, pValue )
				{
					if( FStrEq( pValue->GetName(), "origin" ) )
					{
						UTIL_StringToVector( vecSoldierOrigin.Base(), pValue->GetString() );
						bGotOrigin = true;
					}
					else if( FStrEq( pValue->GetName(), "angles" ) )
					{
						UTIL_StringToVector( angSoldierAngles.Base(), pValue->GetString() );
						bGotAngles = true;
					}
				}
			}
			if( bGotOrigin && bGotAngles )
			{
				CBaseEntity *pSoldierMemorial = CreateEntityByName( "prop_dynamic" );
				if( pSoldierMemorial )
				{
					pSoldierMemorial->SetAbsOrigin( vecSoldierOrigin );
					pSoldierMemorial->SetAbsAngles( angSoldierAngles );
					pSoldierMemorial->KeyValue( "model", "models/soldier_statue/soldier_statue.mdl" );
					pSoldierMemorial->KeyValue( "health", 9999999999 );
					pSoldierMemorial->KeyValue( "minhealthdmg", 999999999999 );
					DispatchSpawn( pSoldierMemorial );
					pSoldierMemorial->SetSolid( SOLID_VPHYSICS );
					variant_t sVariant;
					pSoldierMemorial->AcceptInput( "EnableCollision", NULL, NULL, sVariant, NULL );
				}
			}
		}
	}
}

static const char *s_ValidMapEnts[] =
{
	"item_armor",
	"item_grenadepack",
	"prop_dynamic",
	"item_healthkit_full",
	"item_healthkit_small",
	"item_healthkit_medium",
	"item_ammopack_full",
	"item_ammopack_small",
	"item_ammopack_medium",
	"", // END Marker
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::MapEntsSpawnAll( void )
{
	KeyValues *pKeyValue = new KeyValues( "map_ents" );
	char szMapSpeicificEnts[MAX_PATH];
	Q_snprintf( szMapSpeicificEnts, sizeof( szMapSpeicificEnts ), "maps/%s_map_ents.txt", STRING( gpGlobals->mapname ) );

	// override rather than add keyvalues as conflicts would otherwise be unavoidable 
	if( pKeyValue && (pKeyValue->LoadFromFile( filesystem, szMapSpeicificEnts, "BSP" ) ||
		pKeyValue->LoadFromFile( filesystem, szMapSpeicificEnts, "MOD" ) ||
		pKeyValue->LoadFromFile( filesystem, "scripts/map_ents.txt", "MOD" )) )
	{
		KeyValues *pMap = pKeyValue->FindKey( STRING( gpGlobals->mapname ) );
		if( pMap )
		{
			FOR_EACH_SUBKEY( pMap, pSubKey )
			{
				//Invalid entity listed
				if( !FindInList( s_ValidMapEnts, pSubKey->GetName() ) )
				{
					continue;
				}

				bool bIsProp = false;
				if( FStrEq( pSubKey->GetName(), "prop_dynamic" ) )
					bIsProp = true;

				Vector vecEntOrigin = Vector( 0, 0, 0 );
				QAngle angEntAngles = QAngle( 0, 0, 0 );
				char cModelName[512];
				int nCollision = 0;
				CBaseEntity *pNewMapEnt = CreateEntityByName( pSubKey->GetName() );
				if( pNewMapEnt )
				{
					FOR_EACH_VALUE( pSubKey, pValue )
					{
						if( FStrEq( pValue->GetName(), "origin" ) )
						{
							UTIL_StringToVector( vecEntOrigin.Base(), pValue->GetString() );
						}
						else if( FStrEq( pValue->GetName(), "angles" ) )
						{
							UTIL_StringToVector( angEntAngles.Base(), pValue->GetString() );
						}
						else if( bIsProp && FStrEq( pValue->GetName(), "model" ) )
						{
							Q_snprintf( cModelName, sizeof( cModelName ), "%s", pValue->GetString() );
						}
						else if( bIsProp && FStrEq( pValue->GetName(), "solid" ) )
						{
							nCollision = pValue->GetInt();
						}
					}
					pNewMapEnt->SetAbsOrigin( vecEntOrigin );
					pNewMapEnt->SetAbsAngles( angEntAngles );
					if( bIsProp )
					{
						pNewMapEnt->KeyValue( "model", cModelName );
						if( nCollision > 0 )
						{
							pNewMapEnt->KeyValue( "health", 9999999999 );
							pNewMapEnt->KeyValue( "minhealthdmg", 999999999999 );
							pNewMapEnt->SetSolid( nCollision == 1 ? SOLID_VPHYSICS : SOLID_BBOX );
							variant_t sVariant;
							pNewMapEnt->AcceptInput( "EnableCollision", NULL, NULL, sVariant, NULL );
						}
					}
					pNewMapEnt->SetName( MAKE_STRING( "temp_map_ent" ) );
					DispatchSpawn( pNewMapEnt );
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------	
void cc_ReloadMapEnts( const CCommand &args )
{
	if( UTIL_IsCommandIssuedByServerAdmin() )
	{
		CTFGameRules *pRules = dynamic_cast<CTFGameRules *>(GameRules());

		if( pRules )
		{
			pRules->MapEntsReload();
		}
	}
}
static ConCommand reload_map_ents_file( "reload_map_ents_file", cc_ReloadMapEnts, "Reloads map ents loaded from map_ents.txt file" );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::MapEntsReload( void )
{
	// Search through all entities and remove the ones with the name "temp_map_ent"
	CBaseEntity *pCur = gEntList.FirstEnt();
	while( pCur )
	{
		if( pCur->NameMatches( "temp_map_ent" ) )
		{
			UTIL_Remove( pCur );
		}

		pCur = gEntList.NextEnt( pCur );
	}

	// Respawn all custom entities
	MapEntsSpawnAll();
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------ob
void CTFGameRules::RecalculateControlPointState( void )
{
	Assert( ObjectiveResource() );

	if ( !g_hControlPointMasters.Count() )
		return;

	if ( g_pObjectiveResource && g_pObjectiveResource->PlayingMiniRounds() )
		return;

	for ( int iTeam = LAST_SHARED_TEAM+1; iTeam < GetNumberOfTeams(); iTeam++ )
	{
		int iFarthestPoint = GetFarthestOwnedControlPoint( iTeam, true );
		if ( iFarthestPoint == -1 )
			continue;

		// Now enable all spawn points for that spawn point
		CBaseEntity *pSpot = gEntList.FindEntityByClassname( NULL, "info_player_teamspawn" );
		while( pSpot )
		{
			CTFTeamSpawn *pTFSpawn = assert_cast<CTFTeamSpawn*>(pSpot);
			if ( pTFSpawn->GetControlPoint() )
			{
				if ( pTFSpawn->GetTeamNumber() == iTeam )
				{
					if ( pTFSpawn->GetControlPoint()->GetPointIndex() == iFarthestPoint )
					{
						pTFSpawn->SetDisabled( false );
					}
					else
					{
						pTFSpawn->SetDisabled( true );
					}
				}
			}

			pSpot = gEntList.FindEntityByClassname( pSpot, "info_player_teamspawn" );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when a new round is being initialized
//-----------------------------------------------------------------------------
void CTFGameRules::SetupOnRoundStart( void )
{
	for ( int i = 0; i < MAX_TEAMS; i++ )
	{
		ObjectiveResource()->SetBaseCP( -1, i );
	}

	for ( int i = 0; i < TF_TEAM_COUNT; i++ )
	{
		m_iNumCaps[i] = 0;
	}

	// Let all entities know that a new round is starting
	CBaseEntity *pEnt = gEntList.FirstEnt();
	while( pEnt )
	{
		variant_t emptyVariant;
		pEnt->AcceptInput( "RoundSpawn", NULL, NULL, emptyVariant, 0 );

		pEnt = gEntList.NextEnt( pEnt );
	}

	// All entities have been spawned, now activate them
	pEnt = gEntList.FirstEnt();
	while( pEnt )
	{
		variant_t emptyVariant;
		pEnt->AcceptInput( "RoundActivate", NULL, NULL, emptyVariant, 0 );

		pEnt = gEntList.NextEnt( pEnt );
	}

	if ( g_pObjectiveResource && !g_pObjectiveResource->PlayingMiniRounds() )
	{
		// Find all the control points with associated spawnpoints
		memset( m_bControlSpawnsPerTeam, 0, sizeof(bool) * MAX_TEAMS * MAX_CONTROL_POINTS );
		CBaseEntity *pSpot = gEntList.FindEntityByClassname( NULL, "info_player_teamspawn" );
		while( pSpot )
		{
			CTFTeamSpawn *pTFSpawn = assert_cast<CTFTeamSpawn*>(pSpot);
			if ( pTFSpawn->GetControlPoint() )
			{
				m_bControlSpawnsPerTeam[ pTFSpawn->GetTeamNumber() ][ pTFSpawn->GetControlPoint()->GetPointIndex() ] = true;
				pTFSpawn->SetDisabled( true );
			}

			pSpot = gEntList.FindEntityByClassname( pSpot, "info_player_teamspawn" );
		}

		RecalculateControlPointState();

		SetRoundOverlayDetails();
	}

#ifdef GAME_DLL
	m_szMostRecentCappers[0] = 0;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Called when a new round is off and running
//-----------------------------------------------------------------------------
void CTFGameRules::SetupOnRoundRunning( void )
{
	// Let out control point masters know that the round has started
	for ( int i = 0; i < g_hControlPointMasters.Count(); i++ )
	{
		variant_t emptyVariant;
		if ( g_hControlPointMasters[i] )
		{
			g_hControlPointMasters[i]->AcceptInput( "RoundStart", NULL, NULL, emptyVariant, 0 );
		}
	}

	// Reset player speeds after preround lock
	CTFPlayer *pPlayer;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

		if ( !pPlayer )
			continue;

		pPlayer->TeamFortress_SetSpeed();
		pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_ROUND_START );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called before a new round is started (so the previous round can end)
//-----------------------------------------------------------------------------
void CTFGameRules::PreviousRoundEnd( void )
{
	// before we enter a new round, fire the "end output" for the previous round
	if ( g_hControlPointMasters.Count() && g_hControlPointMasters[0] )
	{
		g_hControlPointMasters[0]->FireRoundEndOutput();
	}

	m_iPreviousRoundWinners = GetWinningTeam();
}

//-----------------------------------------------------------------------------
// Purpose: Called when a round has entered stalemate mode (timer has run out)
//-----------------------------------------------------------------------------
void CTFGameRules::SetupOnStalemateStart( void )
{
	// Remove everyone's objects
	for ( int i = 1 ; i <= gpGlobals->maxClients ; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pPlayer )
		{
			pPlayer->TeamFortress_RemoveEverythingFromWorld();
		}
	}

	// Respawn all the players
	RespawnPlayers( true );

	if( IsInArenaMode() )
	{
		if( m_hArenaLogic.IsValid() )
		{
			m_hArenaLogic->m_OnArenaRoundStart.FireOutput( m_hArenaLogic.Get(), m_hArenaLogic.Get() );

			IGameEvent *event = gameeventmanager->CreateEvent( "arena_round_start" );
			if( event )
			{
				gameeventmanager->FireEvent( event );
			}

			if( tf_arena_override_cap_enable_time.GetFloat() > 0 )
				m_flCapturePointEnableTime = gpGlobals->curtime + tf_arena_override_cap_enable_time.GetFloat();
			else
				m_flCapturePointEnableTime = gpGlobals->curtime + m_hArenaLogic->m_flTimeToEnableCapPoint;

			BroadcastSound( 255, "Announcer.AM_RoundStartRandom" );
			BroadcastSound( 255, "Ambient.Siren" );
		}
	}

	// Disable all the active health packs in the world
	m_hDisabledHealthKits.Purge();
	CHealthKit *pHealthPack = gEntList.NextEntByClass( (CHealthKit *)NULL );
	while ( pHealthPack )
	{
		if ( !pHealthPack->IsDisabled() )
		{
			pHealthPack->SetDisabled( true );
			m_hDisabledHealthKits.AddToTail( pHealthPack );
		}
		pHealthPack = gEntList.NextEntByClass( pHealthPack );
	}

	CTFPlayer *pPlayer;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

		if ( !pPlayer )
			continue;

		pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_SUDDENDEATH_START );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SetupOnStalemateEnd( void )
{
	// Reenable all the health packs we disabled
	for ( int i = 0; i < m_hDisabledHealthKits.Count(); i++ )
	{
		if ( m_hDisabledHealthKits[i] )
		{
			m_hDisabledHealthKits[i]->SetDisabled( false );
		}
	}

	m_hDisabledHealthKits.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::InitTeams( void )
{
	BaseClass::InitTeams();

	// clear the player class data
	ResetFilePlayerClassInfoDatabase();
}


ConVar tf_fixedup_damage_radius ( "tf_fixedup_damage_radius", "1", FCVAR_DEVELOPMENTONLY );
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
//			&vecSrcIn - 
//			flRadius - 
//			iClassIgnore - 
//			*pEntityIgnore - 
//-----------------------------------------------------------------------------
void CTFGameRules::RadiusDamage( const CTakeDamageInfo &info, const Vector &vecSrcIn, float flRadius, int iClassIgnore, CBaseEntity *pEntityIgnore )
{
	const int MASK_RADIUS_DAMAGE = MASK_SHOT&(~CONTENTS_HITBOX);
	CBaseEntity *pEntity = NULL;
	trace_t		tr;
	float		falloff;
	Vector		vecSpot;

	Vector vecSrc = vecSrcIn;

	if ( info.GetDamageType() & DMG_RADIUS_MAX )
		falloff = 0.0;
	else if ( info.GetDamageType() & DMG_HALF_FALLOFF )
		falloff = 0.5;
	else if ( flRadius )
		falloff = info.GetDamage() / flRadius;
	else
		falloff = 1.0;

	CBaseEntity *pInflictor = info.GetInflictor();
#ifdef PF2_DLL
	CTFWeaponBaseGrenadeProj* pGrenade = dynamic_cast<CTFWeaponBaseGrenadeProj*>( pInflictor );
	bool bHandGrenade = pGrenade && !pGrenade->IsPillGrenade();
#endif
//	float flHalfRadiusSqr = Square( flRadius / 2.0f );

	// iterate on all entities in the vicinity.
	for ( CEntitySphereQuery sphere( vecSrc, flRadius ); (pEntity = sphere.GetCurrentEntity()) != NULL; sphere.NextEntity() )
	{
		// This value is used to scale damage when the explosion is blocked by some other object.
		float flBlockedDamagePercent = 0.0f;

		if ( pEntity == pEntityIgnore )
			continue;

		if ( pEntity->m_takedamage == DAMAGE_NO )
			continue;

		// UNDONE: this should check a damage mask, not an ignore
		if ( iClassIgnore != CLASS_NONE && pEntity->Classify() == iClassIgnore )
		{// houndeyes don't hurt other houndeyes with their attack
			continue;
		}

		// Check that the explosion can 'see' this entity.
		vecSpot = pEntity->BodyTarget( vecSrc, false );
#ifdef PF2_DLL
		UTIL_TraceLine( vecSrc, vecSpot, MASK_RADIUS_DAMAGE, info.GetInflictor(), TF_COLLISIONGROUP_GRENADES, &tr );
#else
		UTIL_TraceLine( vecSrc, vecSpot, MASK_RADIUS_DAMAGE, info.GetInflictor(), COLLISION_GROUP_PROJECTILE, &tr );
#endif
		if ( tr.fraction != 1.0 && tr.m_pEnt != pEntity )
			continue;

		// Adjust the damage - apply falloff.
		float flAdjustedDamage = 0.0f;

		float flDistanceToEntity;

		// Rockets store the ent they hit as the enemy and have already
		// dealt full damage to them by this time
		if ( pInflictor && ( pEntity == pInflictor->GetEnemy() ) )
		{
			// Full damage, we hit this entity directly
			flDistanceToEntity = 0;
		}
		else if ( pEntity->IsPlayer() )
		{
			// Use whichever is closer, absorigin or worldspacecenter
			float flToWorldSpaceCenter = ( vecSrc - pEntity->WorldSpaceCenter() ).Length();
			float flToOrigin = ( vecSrc - pEntity->GetAbsOrigin() ).Length();

			flDistanceToEntity = min( flToWorldSpaceCenter, flToOrigin );
#ifdef PF2_DLL
			// If a grenade explodes close enough to a player do full damage
			if (flDistanceToEntity < 80.0f && pInflictor && bHandGrenade)
			{
				flDistanceToEntity = 0;
			}
#endif
		}
		else
		{
			flDistanceToEntity = ( vecSrc - tr.endpos ).Length();
		}

		if ( tf_fixedup_damage_radius.GetBool() )
		{
			flAdjustedDamage = RemapValClamped( flDistanceToEntity, 0, flRadius, info.GetDamage(), info.GetDamage() * falloff );
		}
		else
		{
			flAdjustedDamage = flDistanceToEntity * falloff;
			flAdjustedDamage = info.GetDamage() - flAdjustedDamage;
		}
		
		// Take a little less damage from yourself
		if ( tr.m_pEnt == info.GetAttacker())
		{
			flAdjustedDamage = flAdjustedDamage * 0.75;
		}
	
		if ( flAdjustedDamage <= 0 )
			continue;

		// the explosion can 'see' this entity, so hurt them!
		if (tr.startsolid)
		{
			// if we're stuck inside them, fixup the position and distance
			tr.endpos = vecSrc;
			tr.fraction = 0.0;
		}
		
		CTakeDamageInfo adjustedInfo = info;
		//Msg("%s: Blocked damage: %f percent (in:%f  out:%f)\n", pEntity->GetClassname(), flBlockedDamagePercent * 100, flAdjustedDamage, flAdjustedDamage - (flAdjustedDamage * flBlockedDamagePercent) );
		adjustedInfo.SetDamage( flAdjustedDamage - (flAdjustedDamage * flBlockedDamagePercent) );

#ifdef PF2_DLL
		// We need to ignore armor on a direct hit with a flamerocket
		CTFFlameRocket* pFlameRocket = dynamic_cast< CTFFlameRocket* >( pInflictor );
		if ( pFlameRocket && flDistanceToEntity == 0 )
		{
			adjustedInfo.SetIgnoreArmor( true );
		}
#endif

		// Now make a consideration for skill level!
		if( info.GetAttacker() && info.GetAttacker()->IsPlayer() && pEntity->IsNPC() )
		{
			// An explosion set off by the player is harming an NPC. Adjust damage accordingly.
			adjustedInfo.AdjustPlayerDamageInflictedForSkillLevel();
		}

		Vector dir = vecSpot - vecSrc;
		VectorNormalize( dir );

		// If we don't have a damage force, manufacture one
		if ( adjustedInfo.GetDamagePosition() == vec3_origin || adjustedInfo.GetDamageForce() == vec3_origin )
		{
			CalculateExplosiveDamageForce( &adjustedInfo, dir, vecSrc );
		}
		else
		{
			// Assume the force passed in is the maximum force. Decay it based on falloff.
			float flForce = adjustedInfo.GetDamageForce().Length() * falloff;
			adjustedInfo.SetDamageForce( dir * flForce );
			adjustedInfo.SetDamagePosition( vecSrc );
		}

		if ( tr.fraction != 1.0 && pEntity == tr.m_pEnt )
		{
			ClearMultiDamage( );
			pEntity->DispatchTraceAttack( adjustedInfo, dir, &tr );
			ApplyMultiDamage();
		}
		else
		{
			pEntity->TakeDamage( adjustedInfo );
		}

		// Now hit all triggers along the way that respond to damage... 
		pEntity->TraceAttackToTriggers( adjustedInfo, vecSrc, tr.endpos, dir );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::Arena_CleanupPlayerQueue( void )
{
	for( int i = 0; i < MAX_PLAYERS; i++ )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if( pTFPlayer && pTFPlayer->IsReadyToPlay() && !pTFPlayer->IsHLTV() && !pTFPlayer->IsReplay() && pTFPlayer->GetTeamNumber() != TEAM_SPECTATOR )
		{
			RemovePlayerFromQueue( pTFPlayer );
			pTFPlayer->m_bInArenaQueue = false;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::Arena_ClientDisconnect( const char *pszPlayerName )
{
	int iLightTeam = TEAM_UNASSIGNED, iHeavyTeam = TEAM_UNASSIGNED, iPlayers;
	CTeam *pLightTeam = NULL, *pHeavyTeam = NULL;
	CTFPlayer *pTFPlayer = NULL;

	if( !TFGameRules()->IsInArenaMode() )
		return;

	if( IsInWaitingForPlayers() || State_Get() != GR_STATE_PREROUND )
		return;

	if( IsInTournamentMode() || !AreTeamsUnbalanced( iHeavyTeam, iLightTeam ) )
		return;

	pHeavyTeam = GetGlobalTeam( iHeavyTeam );
	pLightTeam = GetGlobalTeam( iLightTeam );
	if( pHeavyTeam && pLightTeam )
	{
		iPlayers = pHeavyTeam->GetNumPlayers() - pLightTeam->GetNumPlayers();

		// Are there players waiting to play?
		if( m_hArenaQueue.Count() > 0 && iPlayers > 0 )
		{
			for( int i = 0; i < iPlayers; i++ )
			{
				pTFPlayer = m_hArenaQueue.Head();
				if( pTFPlayer && !pTFPlayer->IsHLTV() && !pTFPlayer->IsReplay() )
				{
					pTFPlayer->ForceChangeTeam( TF_TEAM_AUTOASSIGN );

					const char *pszTeamName;

					// Figure out what team the player got balanced to
					if( pTFPlayer->GetTeam() == pLightTeam )
						pszTeamName = pLightTeam->GetName();
					else
						pszTeamName = pHeavyTeam->GetName();

					UTIL_ClientPrintAll( HUD_PRINTTALK, "#TF_Arena_ClientDisconnect", pTFPlayer->GetPlayerName(), pszTeamName, pszPlayerName );

					if( !pTFPlayer->m_bInArenaQueue )
						pTFPlayer->m_flArenaQueueTime = gpGlobals->curtime;

					RemovePlayerFromQueue( pTFPlayer );

					pTFPlayer->m_bInArenaQueue = false;
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::Arena_ResetLosersScore( bool bStreakReached )
{
	if( bStreakReached )
	{
		for( int i = 0; i < GetNumberOfTeams(); i++ )
		{
			if( i != GetWinningTeam() )
				GetWinningTeam();

			CTeam *pTeam = GetGlobalTeam( i );

			if( pTeam )
				pTeam->ResetScores();
		}
	}
	else
	{
		for( int i = 0; i < GetNumberOfTeams(); i++ )
		{
			if( i != GetWinningTeam() && GetWinningTeam() > TEAM_SPECTATOR )
			{
				CTeam *pTeam = GetGlobalTeam( i );

				if( pTeam )
					pTeam->ResetScores();
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::Arena_NotifyTeamSizeChange( void )
{
	CTeam *pTeam = GetGlobalTFTeam( TF_TEAM_BLUE );
	int iTeamCount = pTeam->GetNumPlayers();
	if( iTeamCount != m_iArenaTeamCount )
	{
		if( m_iArenaTeamCount )
		{
			if( iTeamCount >= m_iArenaTeamCount )
			{
				UTIL_ClientPrintAll( HUD_PRINTTALK, "#TF_Arena_TeamSizeIncreased", UTIL_VarArgs( "%d", iTeamCount ) );
			}
			else
			{
				UTIL_ClientPrintAll( HUD_PRINTTALK, "#TF_Arena_TeamSizeDecreased", UTIL_VarArgs( "%d", iTeamCount ) );
			}
		}
		m_iArenaTeamCount = iTeamCount;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGameRules::Arena_PlayersNeededForMatch( void )
{
	int i = 0, j = 0, iReadyToPlay = 0, iNumPlayers = 0, iPlayersNeeded = 0, iMaxTeamSize = 0, iDesiredTeamSize = 0;
	CTFPlayer *pTFPlayer = NULL, *pTFPlayerToBalance = NULL;

	// 1/3 of the players in a full server spectate
	iMaxTeamSize = gpGlobals->maxClients;
	if( HLTVDirector()->IsActive() )
		iMaxTeamSize--;
	iMaxTeamSize = (iMaxTeamSize / 3.0) + 0.5;

	for( i = 1; i <= MAX_PLAYERS; i++ )
	{
		pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if( pTFPlayer && pTFPlayer->IsReadyToPlay() )
		{
			iReadyToPlay++;
		}
	}

	// **NOTE: live TF2 sets iDesiredTeamSize 
	// to ( iPlayersNeeded + iReadyToPlay ) / 2
	// but this isn't working due to overlap with 
	// the arena queue and IsReadyToPlay()

	iPlayersNeeded = m_hArenaQueue.Count();
	iDesiredTeamSize = (iReadyToPlay) / 2;

	// keep the teams even 
	if( iReadyToPlay % 2 != 0 )
		iPlayersNeeded--;

	if( iDesiredTeamSize > iMaxTeamSize )
	{
		iPlayersNeeded += (2 * iMaxTeamSize) - iReadyToPlay;

		if( iPlayersNeeded < 0 )
			iPlayersNeeded = 0;
	}

	if( GetWinningTeam() > TEAM_SPECTATOR )
	{
		iNumPlayers = GetGlobalTFTeam( GetWinningTeam() )->GetNumPlayers();
		if( iNumPlayers <= iMaxTeamSize )
		{
			// Is the winning team larger than the desired team size?
			if( iNumPlayers <= iDesiredTeamSize )
				return iPlayersNeeded;
			iMaxTeamSize = iDesiredTeamSize;
		}
	}
	else
	{
		return iPlayersNeeded;
	}

	// Keep going until teams are balanced
	while( iNumPlayers >= iMaxTeamSize )
	{
		// Shortest queue time starts very large
		float flShortestQueueTime = 9999.9f, flTimeQueued = 0.0f;

		for( j = 1; i < MAX_PLAYERS; i++ )
		{
			pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
			if( pTFPlayer && pTFPlayer->GetTeamNumber() == GetWinningTeam() && !pTFPlayer->IsHLTV() && !pTFPlayer->IsReplay() )
			{
				flTimeQueued = gpGlobals->curtime - pTFPlayer->m_flArenaQueueTime;
				if( flShortestQueueTime > flTimeQueued )
				{
					// Save the player who has been on the team for the shortest time
					flShortestQueueTime = flTimeQueued;
					pTFPlayerToBalance = pTFPlayer;
				}
			}
		}

		if( pTFPlayerToBalance )
		{
			// Move the player into the front of the queue
			pTFPlayerToBalance->ForceChangeTeam( TEAM_SPECTATOR );

			pTFPlayerToBalance->m_flArenaQueueTime = gpGlobals->curtime;
			if( iPlayersNeeded < iMaxTeamSize )
				++iPlayersNeeded;

			// Balance the player
			IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_teambalanced_player" );
			if( event )
			{
				event->SetInt( "team", GetWinningTeam() );
				event->SetInt( "player", pTFPlayerToBalance->GetUserID() );
				gameeventmanager->FireEvent( event );
			}
			UTIL_ClientPrintAll( HUD_PRINTTALK, "#game_player_was_team_balanced", UTIL_VarArgs( "%s", pTFPlayerToBalance->GetPlayerName() ) );

			pTFPlayerToBalance = NULL;
		}
		iNumPlayers = GetGlobalTFTeam( GetWinningTeam() )->GetNumPlayers();
	}

	return iPlayersNeeded;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::Arena_PrepareNewPlayerQueue( bool bScramble )
{
	int i = 0;
	CTFPlayer *pTFPlayer = NULL;

	for( i = 0; i < MAX_PLAYERS; i++ )
	{
		pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

		// I really don't like how this is formatted
		if( pTFPlayer && !pTFPlayer->IsHLTV() && !pTFPlayer->IsReplay() )
		{
			if( GetWinningTeam() > TEAM_SPECTATOR && pTFPlayer->IsReadyToPlay() && (pTFPlayer->GetTeamNumber() != GetWinningTeam() || bScramble) )
			{
				AddPlayerToQueue( pTFPlayer );
			}
		}
	}

	if( bScramble )
		m_hArenaQueue.Sort( ScramblePlayersSort );
	else
		m_hArenaQueue.Sort( SortPlayerSpectatorQueue );

	for( i = 0; i < m_hArenaQueue.Count(); ++i )
	{
		pTFPlayer = m_hArenaQueue.Element( i );
		if( pTFPlayer && pTFPlayer->IsReadyToPlay() )
		{
			pTFPlayer->ChangeTeam( TEAM_SPECTATOR );
			pTFPlayer->m_bInArenaQueue = true;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::Arena_RunTeamLogic( void )
{
	if( !TFGameRules()->IsInArenaMode() )
		return;

	bool bStreakReached = false;
	int i = 0, iHeavyCount = 0, iLightCount = 0, iPlayersNeeded = 0, iTeam = TEAM_UNASSIGNED, iActivePlayers = CountActivePlayers();
	CTFTeam *pTeam = GetGlobalTFTeam( GetWinningTeam() );

	if( !pTeam )
		return;

	if( tf_arena_use_queue.GetBool() )
	{
		if( tf_arena_max_streak.GetInt() > 0 && pTeam->GetScore() >= tf_arena_max_streak.GetInt() )
		{
			bStreakReached = true;
			IGameEvent *event = gameeventmanager->CreateEvent( "arena_match_maxstreak" );
			if( event )
			{
				event->SetInt( "team", iTeam );
				event->SetInt( "streak", tf_arena_max_streak.GetInt() );
				gameeventmanager->FireEvent( event );
			}
		}

		if( bStreakReached /* && (!IsInVSHMode() || !IsInDRMode() )*/ )
		{
			for( i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
			{
				BroadcastSound( i, "Announcer.AM_TeamScrambleRandom" );
			}
		}

		if( !IsInTournamentMode() /* && (!IsInVSHMode() || !IsInDRMode() )*/ )
		{
			if( iActivePlayers > 0 )
				Arena_ResetLosersScore( bStreakReached );
			else
				Arena_ResetLosersScore( true );
		}

		/*if( IsInVSHMode() || IsInDRMode() )
			bStreakReached = true;*/

		if( iActivePlayers <= 0 )
		{
			Arena_PrepareNewPlayerQueue( true );
			State_Transition( GR_STATE_PREGAME );
		}
		else
		{
			Arena_PrepareNewPlayerQueue( bStreakReached );

			iPlayersNeeded = Arena_PlayersNeededForMatch();

			if( AreTeamsUnbalanced( iHeavyCount, iLightCount ) && iPlayersNeeded > 0 )
			{
				if( iPlayersNeeded > m_hArenaQueue.Count() )
					iPlayersNeeded = m_hArenaQueue.Count();

				if( pTeam )
				{
					// Everyone not on the winning team should be in spectate already
					int iUnknown = pTeam->GetNumPlayers() + iPlayersNeeded;

					iUnknown = (iUnknown * 0.5) - pTeam->GetNumPlayers();
					iTeam = pTeam->GetTeamNumber();

					if( iTeam == TEAM_UNASSIGNED )
						iTeam = TF_TEAM_AUTOASSIGN;

					for( i = 0; i < iPlayersNeeded; ++i )
					{
						CTFPlayer *pTFPlayer = m_hArenaQueue.Element( i );
						if( i >= iUnknown )
							iTeam = TF_TEAM_AUTOASSIGN;

						if( pTFPlayer )
						{
							pTFPlayer->ForceChangeTeam( iTeam );
							if( !pTFPlayer->m_bInArenaQueue )
								pTFPlayer->m_flArenaQueueTime = gpGlobals->curtime;
						}
					}
				}
			}
			m_flArenaNotificationSend = gpGlobals->curtime + 1.0f;
			Arena_CleanupPlayerQueue();
			Arena_NotifyTeamSizeChange();
		}
	}
	else if( iActivePlayers > 0
		|| (GetGlobalTFTeam( TF_TEAM_RED ) && GetGlobalTFTeam( TF_TEAM_RED )->GetNumPlayers() < 1)
		|| (GetGlobalTFTeam( TF_TEAM_BLUE ) && GetGlobalTFTeam( TF_TEAM_BLUE )->GetNumPlayers() < 1) )
	{
		State_Transition( GR_STATE_PREGAME );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::Arena_SendPlayerNotifications( void )
{
	CUtlVector< CTFTeam * > pTeam;
	pTeam.AddToTail( GetGlobalTFTeam( TF_TEAM_RED ) );
	pTeam.AddToTail( GetGlobalTFTeam( TF_TEAM_BLUE ) );

	if( !pTeam[0] || !pTeam[1] )
		return;

	int iPlaying = pTeam[0]->GetNumPlayers() + pTeam[1]->GetNumPlayers(), iReady = 0, iQueued;
	CTFPlayer *pTFPlayer = NULL;

	m_flArenaNotificationSend = 0.0f;

	for( int i = 0; i < MAX_PLAYERS; i++ )
	{
		pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

		if( pTFPlayer && pTFPlayer->IsReadyToPlay() && !pTFPlayer->IsHLTV() && !pTFPlayer->IsReplay() )
		{
			if( pTFPlayer->GetTeamNumber() == TEAM_SPECTATOR )
			{
				// Player is in spectator. Send a notification to the hud
				CRecipientFilter filter;
				filter.AddRecipient( pTFPlayer );
				UserMessageBegin( filter, "HudArenaNotify" );
				WRITE_BYTE( pTFPlayer->entindex() );
				WRITE_BYTE( 1 );
				MessageEnd();
			}

			iReady++;
		}
	}

	if( iPlaying != iReady )
	{
		iQueued = iReady - iPlaying;
		for( int i = 0; i < pTeam.Count(); i++ )
		{
			if( pTeam.Element( i ) )
			{
				for( int j = 0; j < pTeam[i]->GetNumPlayers(); j++ )
				{
					CBasePlayer *pPlayer = pTeam[i]->GetPlayer( j );
					pTFPlayer = ToTFPlayer( pPlayer );
					if( pTFPlayer && pTFPlayer->IsReadyToPlay() && !pTFPlayer->IsHLTV() && !pTFPlayer->IsReplay() )
					{
						AddPlayerToQueue( pTFPlayer );
					}
				}
			}

			m_hArenaQueue.Sort( SortPlayerSpectatorQueue );

			//TODO: This is not alerting the correct players 
			// who might sit out the next match
			for( int j = 0; j <= m_hArenaQueue.Count(); j++ )
			{
				if( j < iQueued )
				{
					pTFPlayer = m_hArenaQueue.Element( j );

					if( !pTFPlayer )
						continue;

					//Msg("Player Might Have to Sit Out: %s\n", pTFPlayer->GetPlayerName() );
					// This player might sit out the next game if they lose
					CRecipientFilter filter;
					filter.AddRecipient( pTFPlayer );
					UserMessageBegin( filter, "HudArenaNotify" );
					WRITE_BYTE( pTFPlayer->entindex() );
					WRITE_BYTE( 0 );
					MessageEnd();
				}
			}
		}
	}

	// Clean up the player queue
	Arena_CleanupPlayerQueue();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::AddPlayerToQueue( CTFPlayer *pPlayer )
{
	if( !pPlayer )
		return;

	if( !pPlayer->IsArenaSpectator() )
	{
		// Don't know if this is alright
		RemovePlayerFromQueue( pPlayer );
		m_hArenaQueue.AddToTail( pPlayer );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::AddPlayerToQueueHead( CTFPlayer *pPlayer )
{
	if( !pPlayer )
		return;

	if( !m_hArenaQueue.HasElement( pPlayer ) && !pPlayer->IsArenaSpectator() )
	{
		m_hArenaQueue.AddToHead( pPlayer );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::RemovePlayerFromQueue( CTFPlayer *pPlayer )
{
	if( !pPlayer )
		return;

	m_hArenaQueue.FindAndRemove( pPlayer );
}
	// --------------------------------------------------------------------------------------------------- //
	// Voice helper
	// --------------------------------------------------------------------------------------------------- //

	class CVoiceGameMgrHelper : public IVoiceGameMgrHelper
	{
	public:
		virtual bool		CanPlayerHearPlayer( CBasePlayer *pListener, CBasePlayer *pTalker, bool &bProximity )
		{
			// Dead players can only be heard by other dead team mates but only if a match is in progress
			if ( TFGameRules()->State_Get() != GR_STATE_TEAM_WIN && TFGameRules()->State_Get() != GR_STATE_GAME_OVER ) 
			{
				if ( pTalker->IsAlive() == false )
				{
					if ( pListener->IsAlive() == false || tf_gravetalk.GetBool() )
						return ( pListener->InSameTeam( pTalker ) );

					return false;
				}
			}

			return ( pListener->InSameTeam( pTalker ) );
		}
	};
	CVoiceGameMgrHelper g_VoiceGameMgrHelper;
	IVoiceGameMgrHelper *g_pVoiceGameMgrHelper = &g_VoiceGameMgrHelper;

	// Load the objects.txt file.
	class CObjectsFileLoad : public CAutoGameSystem
	{
	public:
		virtual bool Init()
		{
			LoadObjectInfos( filesystem );
			return true;
		}
	} g_ObjectsFileLoad;

	// --------------------------------------------------------------------------------------------------- //
	// Globals.
	// --------------------------------------------------------------------------------------------------- //
/*
	// NOTE: the indices here must match TEAM_UNASSIGNED, TEAM_SPECTATOR, TF_TEAM_RED, TF_TEAM_BLUE, etc.
	char *sTeamNames[] =
	{
		"Unassigned",
		"Spectator",
		"Red",
		"Blue"
	};
*/



	// --------------------------------------------------------------------------------------------------- //
	// Global helper functions.
	// --------------------------------------------------------------------------------------------------- //
	
	// World.cpp calls this but we don't use it in TF.
	void InitBodyQue()
	{
	}

	//-----------------------------------------------------------------------------
	// Purpose: 
	//-----------------------------------------------------------------------------
	CTFGameRules::~CTFGameRules()
	{
		// Note, don't delete each team since they are in the gEntList and will 
		// automatically be deleted from there, instead.
		TFTeamMgr()->Shutdown();
		ShutdownCustomResponseRulesDicts();
	}

	//-----------------------------------------------------------------------------
	// Purpose: TF2 Specific Client Commands
	// Input  :
	// Output :
	//-----------------------------------------------------------------------------
	bool CTFGameRules::ClientCommand( CBaseEntity *pEdict, const CCommand &args )
	{
		CTFPlayer *pPlayer = ToTFPlayer( pEdict );

		const char *pcmd = args[0];
		if ( FStrEq( pcmd, "objcmd" ) )
		{
			if ( args.ArgC() < 3 )
				return true;

			int entindex = atoi( args[1] );
			edict_t* pEdict = INDEXENT(entindex);
			if ( pEdict )
			{
				CBaseEntity* pBaseEntity = GetContainingEntity(pEdict);
				CBaseObject* pObject = dynamic_cast<CBaseObject*>(pBaseEntity);

				if ( pObject )
				{
					// We have to be relatively close to the object too...

					// BUG! Some commands need to get sent without the player being near the object, 
					// eg delayed dismantle commands. Come up with a better way to ensure players aren't
					// entering these commands in the console.

					//float flDistSq = pObject->GetAbsOrigin().DistToSqr( pPlayer->GetAbsOrigin() );
					//if (flDistSq <= (MAX_OBJECT_SCREEN_INPUT_DISTANCE * MAX_OBJECT_SCREEN_INPUT_DISTANCE))
					{
						// Strip off the 1st two arguments and make a new argument string
						CCommand objectArgs( args.ArgC() - 2, &args.ArgV()[2]);
						pObject->ClientCommand( pPlayer, objectArgs );
					}
				}
			}

			return true;
		}

		// Handle some player commands here as they relate more directly to gamerules state
		if ( FStrEq( pcmd, "nextmap" ) )
		{
			if ( pPlayer->m_flNextTimeCheck < gpGlobals->curtime )
			{
				char szNextMap[32];

				if ( nextlevel.GetString() && *nextlevel.GetString() && engine->IsMapValid( nextlevel.GetString() ) )
				{
					Q_strncpy( szNextMap, nextlevel.GetString(), sizeof( szNextMap ) );
				}
				else
				{
					GetNextLevelName( szNextMap, sizeof( szNextMap ) );
				}

				ClientPrint( pPlayer, HUD_PRINTTALK, "#TF_nextmap", szNextMap);

				pPlayer->m_flNextTimeCheck = gpGlobals->curtime + 1;
			}

			return true;
		}
		else if ( FStrEq( pcmd, "timeleft" ) )
		{	
			if ( pPlayer->m_flNextTimeCheck < gpGlobals->curtime )
			{
				if ( mp_timelimit.GetInt() > 0 )
				{
					int iTimeLeft = GetTimeLeft();

					char szMinutes[5];
					char szSeconds[3];

					if ( iTimeLeft <= 0 )
					{
						Q_snprintf( szMinutes, sizeof(szMinutes), "0" );
						Q_snprintf( szSeconds, sizeof(szSeconds), "00" );
					}
					else
					{
						Q_snprintf( szMinutes, sizeof(szMinutes), "%d", iTimeLeft / 60 );
						Q_snprintf( szSeconds, sizeof(szSeconds), "%02d", iTimeLeft % 60 );
					}				

					ClientPrint( pPlayer, HUD_PRINTTALK, "#TF_timeleft", szMinutes, szSeconds );
				}
				else
				{
					ClientPrint( pPlayer, HUD_PRINTTALK, "#TF_timeleft_nolimit" );
				}

				pPlayer->m_flNextTimeCheck = gpGlobals->curtime + 1;
			}
			return true;
		}
		else if( pPlayer->ClientCommand( args ) )
		{
            return true;
		}

		return BaseClass::ClientCommand( pEdict, args );
	}

	// Add the ability to ignore the world trace
	void CTFGameRules::Think()
	{
		if ( !g_fGameOver )
		{
			if ( gpGlobals->curtime > m_flNextPeriodicThink )
			{
				if ( State_Get() != GR_STATE_TEAM_WIN )
				{
					if ( CheckCapsPerRound() )
						return;
				}
			}
		}

		if( IsInArenaMode() && m_flArenaNotificationSend > 0.0 && gpGlobals->curtime >= m_flArenaNotificationSend )
		{
			Arena_SendPlayerNotifications();
		}

		BaseClass::Think();
	}

	//Runs think for all player's conditions
	//Need to do this here instead of the player so players that crash still run their important thinks
	void CTFGameRules::RunPlayerConditionThink ( void )
	{
		for ( int i = 1 ; i <= gpGlobals->maxClients ; i++ )
		{
			CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

			if ( pPlayer )
			{
				pPlayer->m_Shared.ConditionGameRulesThink();
			}
		}
	}

#ifdef PF2_DLL
	void CTFGameRules::PlayerAchievementCheck( void )
	{
		// do we have an achievement we need to look for?
		if( m_bCheckForAchievements )
		{
			for( int i = 1; i <= gpGlobals->maxClients; i++ )
			{
				CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
				if( pPlayer )
				{
					// Cyanide; Expandable in the future
					// are we looking for the kamikaze achievement?
					if( pPlayer->m_bKamikazeByFrag )
					{
						// cycle through all the players to look for one we might have also killed
						for( int j = 1; j <= gpGlobals->maxClients; j++ )
						{
							CTFPlayer *pKilledPlayer = ToTFPlayer( UTIL_PlayerByIndex( j ) );
							if( pKilledPlayer )
							{
								// Skip yourself
								if( pKilledPlayer == pPlayer )
									continue;

								// does our frag entindex match the one that killed the other player
								if( pKilledPlayer->m_iKamikazeFragEntIndex == pPlayer->m_iKamikazeFragEntIndex )
								{
									CSingleUserRecipientFilter filter( pPlayer );
									UserMessageBegin( filter, "AchievementEvent" );
									WRITE_SHORT( ACHIEVEMENT_PF_NADE_KILL_SELF_AND_OTHER );
									MessageEnd();

									// reset our vars so we don't attempt to award the achievement in the future
									pPlayer->m_bKamikazeByFrag = false;
									pPlayer->m_iKamikazeFragEntIndex = pKilledPlayer->m_iKamikazeFragEntIndex = 0;
									break;
								}
							}
						}
					}
				}
			}
			m_bCheckForAchievements = false;
		}
	}
#endif

	void CTFGameRules::FrameUpdatePostEntityThink()
	{
		BaseClass::FrameUpdatePostEntityThink();

		RunPlayerConditionThink();

#ifdef PF2_DLL
		// PFTODO; Is this the best place to put this?
		PlayerAchievementCheck();
#endif
	}

	bool CTFGameRules::CheckCapsPerRound()
	{
		if( IsCTFFlagrun() )
		{
			// Can't win if a flag is out
			CCaptureFlag *pFlag = dynamic_cast<CCaptureFlag *> (gEntList.FindEntityByClassname( NULL, "item_teamflag" ));
			while( pFlag )
			{
				if( pFlag->IsDropped() || pFlag->IsStolen() || pFlag->IsThrown() )
					return false;

				pFlag = dynamic_cast<CCaptureFlag *> (gEntList.FindEntityByClassname( pFlag, "item_teamflag" ));
			}

			int iMaxCaps = -1;
			CTFTeam *pMaxTeam = NULL;

			// check to see if any team has won a "round"
			int nTeamCount = TFTeamMgr()->GetTeamCount();
			for( int iTeam = FIRST_GAME_TEAM; iTeam < nTeamCount; ++iTeam )
			{
				CTFTeam *pTeam = GetGlobalTFTeam( iTeam );
				if( !pTeam )
					continue;

				// we might have more than one team over the caps limit (if the server op lowered the limit)
				// so loop through to see who has the most among teams over the limit
				// PFTODO Cyanide; save a win limit value from logic_multiflag 
				if( pTeam->GetFlagCaptures() >= 3 )
				{
					if( pTeam->GetFlagCaptures() > iMaxCaps )
					{
						iMaxCaps = pTeam->GetFlagCaptures();
						pMaxTeam = pTeam;
					}
				}
			}

			if( iMaxCaps != -1 && pMaxTeam != NULL )
			{
				SetWinningTeam( pMaxTeam->GetTeamNumber(), WINREASON_FLAG_CAPTURE_LIMIT );
				return true;
			}
		}
		else if ( tf_flag_caps_per_round.GetInt() > 0 )
		{
			int iMaxCaps = -1;
			CTFTeam *pMaxTeam = NULL;

			// check to see if any team has won a "round"
			int nTeamCount = TFTeamMgr()->GetTeamCount();
			for ( int iTeam = FIRST_GAME_TEAM; iTeam < nTeamCount; ++iTeam )
			{
				CTFTeam *pTeam = GetGlobalTFTeam( iTeam );
				if ( !pTeam )
					continue;

				// we might have more than one team over the caps limit (if the server op lowered the limit)
				// so loop through to see who has the most among teams over the limit
				if ( pTeam->GetFlagCaptures() >= tf_flag_caps_per_round.GetInt() )
				{
					if ( pTeam->GetFlagCaptures() > iMaxCaps )
					{
						iMaxCaps = pTeam->GetFlagCaptures();
						pMaxTeam = pTeam;
					}
				}
			}

			if ( iMaxCaps != -1 && pMaxTeam != NULL )
			{
				SetWinningTeam( pMaxTeam->GetTeamNumber(), WINREASON_FLAG_CAPTURE_LIMIT );
				return true;
			}
		}

		return false;
	}

	bool CTFGameRules::CheckWinLimit()
	{
		if ( mp_winlimit.GetInt() != 0 )
		{
			bool bWinner = false;

			if ( TFTeamMgr()->GetTeam( TF_TEAM_BLUE )->GetScore() >= mp_winlimit.GetInt() )
			{
				UTIL_LogPrintf( "Team \"BLUE\" triggered \"Intermission_Win_Limit\"\n" );
				bWinner = true;
			}
			else if ( TFTeamMgr()->GetTeam( TF_TEAM_RED )->GetScore() >= mp_winlimit.GetInt() )
			{
				UTIL_LogPrintf( "Team \"RED\" triggered \"Intermission_Win_Limit\"\n" );
				bWinner = true;
			}

			if ( bWinner )
			{
				IGameEvent *event = gameeventmanager->CreateEvent( "tf_game_over" );
				if ( event )
				{
					event->SetString( "reason", "Reached Win Limit" );
					gameeventmanager->FireEvent( event );
				}

				GoToIntermission();
				return true;
			}
		}

		return false;
	}

	bool CTFGameRules::IsInPreMatch() const
	{
		// TFTODO    return (cb_prematch_time > gpGlobals->time)
		return false;
	}

	float CTFGameRules::GetPreMatchEndTime() const
	{
		//TFTODO: implement this.
		return gpGlobals->curtime;
	}

	void CTFGameRules::GoToIntermission( void )
	{
		CTF_GameStats.Event_GameEnd();

		BaseClass::GoToIntermission();
	}

	bool CTFGameRules::FPlayerCanTakeDamage(CBasePlayer *pPlayer, CBaseEntity *pAttacker, const CTakeDamageInfo &info)
	{
		// guard against NULL pointers if players disconnect
		if ( !pPlayer || !pAttacker )
			return false;

		// if pAttacker is an object, we can only do damage if pPlayer is our builder
		if ( pAttacker->IsBaseObject() )
		{
			CBaseObject *pObj = ( CBaseObject *)pAttacker;

			if ( pObj->GetBuilder() == pPlayer || pPlayer->GetTeamNumber() != pObj->GetTeamNumber() )
			{
				// Builder and enemies
				return true;
			}
			else
			{
				// Teammates of the builder
				return false;
			}
		}

		return BaseClass::FPlayerCanTakeDamage(pPlayer, pAttacker, info);
	}

Vector DropToGround( 
	CBaseEntity *pMainEnt, 
	const Vector &vPos, 
	const Vector &vMins, 
	const Vector &vMaxs )
{
	trace_t trace;
	UTIL_TraceHull( vPos, vPos + Vector( 0, 0, -500 ), vMins, vMaxs, MASK_SOLID, pMainEnt, COLLISION_GROUP_NONE, &trace );
	return trace.endpos;
}


void TestSpawnPointType( const char *pEntClassName )
{
	// Find the next spawn spot.
	CBaseEntity *pSpot = gEntList.FindEntityByClassname( NULL, pEntClassName );

	while( pSpot )
	{
		// trace a box here
		Vector vTestMins = pSpot->GetAbsOrigin() + VEC_HULL_MIN;
		Vector vTestMaxs = pSpot->GetAbsOrigin() + VEC_HULL_MAX;

		if ( UTIL_IsSpaceEmpty( pSpot, vTestMins, vTestMaxs ) )
		{
			// the successful spawn point's location
			NDebugOverlay::Box( pSpot->GetAbsOrigin(), VEC_HULL_MIN, VEC_HULL_MAX, 0, 255, 0, 100, 60 );

			// drop down to ground
			Vector GroundPos = DropToGround( NULL, pSpot->GetAbsOrigin(), VEC_HULL_MIN, VEC_HULL_MAX );

			// the location the player will spawn at
			NDebugOverlay::Box( GroundPos, VEC_HULL_MIN, VEC_HULL_MAX, 0, 0, 255, 100, 60 );

			// draw the spawn angles
			QAngle spotAngles = pSpot->GetLocalAngles();
			Vector vecForward;
			AngleVectors( spotAngles, &vecForward );
			NDebugOverlay::HorzArrow( pSpot->GetAbsOrigin(), pSpot->GetAbsOrigin() + vecForward * 32, 10, 255, 0, 0, 255, true, 60 );
		}
		else
		{
			// failed spawn point location
			NDebugOverlay::Box( pSpot->GetAbsOrigin(), VEC_HULL_MIN, VEC_HULL_MAX, 255, 0, 0, 100, 60 );
		}

		// increment pSpot
		pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );
	}
}

// -------------------------------------------------------------------------------- //

void TestSpawns()
{
	TestSpawnPointType( "info_player_teamspawn" );
}
ConCommand cc_TestSpawns( "map_showspawnpoints", TestSpawns, "Dev - test the spawn points, draws for 60 seconds", FCVAR_CHEAT );

// -------------------------------------------------------------------------------- //

void cc_ShowRespawnTimes()
{
	CTFGameRules *pRules = TFGameRules();
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );

	if ( pRules && pPlayer )
	{
		float flRedMin = ( pRules->m_TeamRespawnWaveTimes[TF_TEAM_RED] >= 0 ? pRules->m_TeamRespawnWaveTimes[TF_TEAM_RED] : mp_respawnwavetime.GetFloat() );
		float flRedScalar = pRules->GetRespawnTimeScalar( TF_TEAM_RED );
		float flNextRedRespawn = pRules->GetNextRespawnWave( TF_TEAM_RED, NULL ) - gpGlobals->curtime;

		float flBlueMin = ( pRules->m_TeamRespawnWaveTimes[TF_TEAM_BLUE] >= 0 ? pRules->m_TeamRespawnWaveTimes[TF_TEAM_BLUE] : mp_respawnwavetime.GetFloat() );
		float flBlueScalar = pRules->GetRespawnTimeScalar( TF_TEAM_BLUE );
		float flNextBlueRespawn = pRules->GetNextRespawnWave( TF_TEAM_BLUE, NULL ) - gpGlobals->curtime;

		char tempRed[128];
		Q_snprintf( tempRed, sizeof( tempRed ),   "Red:  Min Spawn %2.2f, Scalar %2.2f, Next Spawn In: %.2f\n", flRedMin, flRedScalar, flNextRedRespawn );

		char tempBlue[128];
		Q_snprintf( tempBlue, sizeof( tempBlue ), "Blue: Min Spawn %2.2f, Scalar %2.2f, Next Spawn In: %.2f\n", flBlueMin, flBlueScalar, flNextBlueRespawn );

		ClientPrint( pPlayer, HUD_PRINTTALK, tempRed );
		ClientPrint( pPlayer, HUD_PRINTTALK, tempBlue );
	}
}

ConCommand mp_showrespawntimes( "mp_showrespawntimes", cc_ShowRespawnTimes, "Show the min respawn times for the teams" );

// -------------------------------------------------------------------------------- //

CBaseEntity *CTFGameRules::GetPlayerSpawnSpot( CBasePlayer *pPlayer )
{
	// get valid spawn point
	CBaseEntity *pSpawnSpot = pPlayer->EntSelectSpawnPoint();

	// drop down to ground
	Vector GroundPos = DropToGround( pPlayer, pSpawnSpot->GetAbsOrigin(), VEC_HULL_MIN, VEC_HULL_MAX );

	// Move the player to the place it said.
	pPlayer->SetLocalOrigin( GroundPos + Vector(0,0,1) );
	pPlayer->SetAbsVelocity( vec3_origin );
	pPlayer->SetLocalAngles( pSpawnSpot->GetLocalAngles() );
	pPlayer->m_Local.m_vecPunchAngle = vec3_angle;
	pPlayer->m_Local.m_vecPunchAngleVel = vec3_angle;
	pPlayer->SnapEyeAngles( pSpawnSpot->GetLocalAngles() );

	return pSpawnSpot;
}

//-----------------------------------------------------------------------------
// Purpose: Checks to see if the player is on the correct team and whether or
//          not the spawn point is available.
//-----------------------------------------------------------------------------
bool CTFGameRules::IsSpawnPointValid( CBaseEntity *pSpot, CBasePlayer *pPlayer, bool bIgnorePlayers )
{
	// Check the team.
	if ( pSpot->GetTeamNumber() != pPlayer->GetTeamNumber() )
		return false;

	if ( !pSpot->IsTriggered( pPlayer ) )
		return false;

	CTFTeamSpawn *pCTFSpawn = dynamic_cast<CTFTeamSpawn*>( pSpot );
	if ( pCTFSpawn )
	{
		if ( pCTFSpawn->IsDisabled() )
			return false;
	}

	Vector mins = GetViewVectors()->m_vHullMin;
	Vector maxs = GetViewVectors()->m_vHullMax;

	if ( !bIgnorePlayers )
	{
		Vector vTestMins = pSpot->GetAbsOrigin() + mins;
		Vector vTestMaxs = pSpot->GetAbsOrigin() + maxs;
		return UTIL_IsSpaceEmpty( pPlayer, vTestMins, vTestMaxs );
	}

	trace_t trace;
	UTIL_TraceHull( pSpot->GetAbsOrigin(), pSpot->GetAbsOrigin(), mins, maxs, MASK_PLAYERSOLID, pPlayer, COLLISION_GROUP_PLAYER_MOVEMENT, &trace );
	return ( trace.fraction == 1 && trace.allsolid != 1 && (trace.startsolid != 1) );
}

Vector CTFGameRules::VecItemRespawnSpot( CItem *pItem )
{
	return pItem->GetOriginalSpawnOrigin();
}

QAngle CTFGameRules::VecItemRespawnAngles( CItem *pItem )
{
	return pItem->GetOriginalSpawnAngles();
}

float CTFGameRules::FlItemRespawnTime( CItem *pItem )
{
	return ITEM_RESPAWN_TIME;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFGameRules::GetChatFormat( bool bTeamOnly, CBasePlayer *pPlayer )
{
	if ( !pPlayer )  // dedicated server output
	{
		return NULL;
	}

	const char *pszFormat = NULL;
	
	// team only
	if ( bTeamOnly == true )
	{
		if ( pPlayer->GetTeamNumber() == TEAM_SPECTATOR )
		{
			pszFormat = "TF_Chat_Spec";
		}
		else
		{
			if ( pPlayer->IsAlive() == false && State_Get() != GR_STATE_TEAM_WIN )
			{
				pszFormat = "TF_Chat_Team_Dead";
			}
			else
			{
				const char *chatLocation = GetChatLocation( bTeamOnly, pPlayer );
				if ( chatLocation && *chatLocation )
				{
					pszFormat = "TF_Chat_Team_Loc";
				}
				else
				{
					pszFormat = "TF_Chat_Team";
				}
			}
		}
	}
	// everyone
	else
	{	
		if ( pPlayer->GetTeamNumber() == TEAM_SPECTATOR )
		{
			pszFormat = "TF_Chat_AllSpec";	
		}
		else
		{
			if ( pPlayer->IsAlive() == false && State_Get() != GR_STATE_TEAM_WIN )
			{
				pszFormat = "TF_Chat_AllDead";
			}
			else
			{
				pszFormat = "TF_Chat_All";	
			}
		}
	}
#ifdef PF2_DLL
	if (pPlayer->IsDeveloper())
	{
		if (pPlayer->GetTeamNumber() == TEAM_SPECTATOR)
		{
			pszFormat = "TF_Chat_DevSpec";
		}
		else
		{
			if (pPlayer->IsAlive() == false && State_Get() != GR_STATE_TEAM_WIN)
			{
				pszFormat = "TF_Chat_DevDead";
			}
			else
			{
				pszFormat = "TF_Chat_Dev";
			}
		}
		if (bTeamOnly && pPlayer->IsAlive())
		{
			pszFormat = "TF_Chat_DevTeam";
		}
		else if (bTeamOnly && !pPlayer->IsAlive())
		{
			pszFormat = "TF_Chat_DevTeam_Dead";
		}
	}
#endif

	return pszFormat;
}

VoiceCommandMenuItem_t *CTFGameRules::VoiceCommand( CBaseMultiplayerPlayer *pPlayer, int iMenu, int iItem )
{
	VoiceCommandMenuItem_t *pItem = BaseClass::VoiceCommand( pPlayer, iMenu, iItem );

	if ( pItem )
	{
		int iActivity = ActivityList_IndexForName( pItem->m_szGestureActivity );

		if ( iActivity != ACT_INVALID )
		{
			CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
			if ( pTFPlayer && !pTFPlayer->m_Shared.InCond( TF_COND_TAUNTING ) )
			{
				pTFPlayer->DoAnimationEvent( PLAYERANIMEVENT_VOICE_COMMAND_GESTURE, iActivity );
			}
		}
	}

	return pItem;
}

//-----------------------------------------------------------------------------
// Purpose: Actually change a player's name.  
//-----------------------------------------------------------------------------
void CTFGameRules::ChangePlayerName( CTFPlayer *pPlayer, const char *pszNewName )
{
	const char *pszOldName = pPlayer->GetPlayerName();

	CReliableBroadcastRecipientFilter filter;
	UTIL_SayText2Filter( filter, pPlayer, false, "#TF_Name_Change", pszOldName, pszNewName );

	IGameEvent * event = gameeventmanager->CreateEvent( "player_changename" );
	if ( event )
	{
		event->SetInt( "userid", pPlayer->GetUserID() );
		event->SetString( "oldname", pszOldName );
		event->SetString( "newname", pszNewName );
		gameeventmanager->FireEvent( event );
	}

	pPlayer->SetPlayerName( pszNewName );

	pPlayer->m_flNextNameChangeTime = gpGlobals->curtime + 10.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::ClientSettingsChanged( CBasePlayer *pPlayer )
{
	const char *pszName = engine->GetClientConVarValue( pPlayer->entindex(), "name" );

	const char *pszOldName = pPlayer->GetPlayerName();

	CTFPlayer *pTFPlayer = (CTFPlayer*)pPlayer;

	// msg everyone if someone changes their name,  and it isn't the first time (changing no name to current name)
	// Note, not using FStrEq so that this is case sensitive
	if ( pszOldName[0] != 0 && Q_strncmp( pszOldName, pszName, MAX_PLAYER_NAME_LENGTH-1 ) )		
	{
		if ( pTFPlayer->m_flNextNameChangeTime < gpGlobals->curtime )
		{
			ChangePlayerName( pTFPlayer, pszName );
		}
		else
		{
			// no change allowed, force engine to use old name again
			engine->ClientCommand( pPlayer->edict(), "name \"%s\"", pszOldName );

			// tell client that he hit the name change time limit
			ClientPrint( pTFPlayer, HUD_PRINTTALK, "#Name_change_limit_exceeded" );
		}
	}

	// keep track of their hud_classautokill value
	int nClassAutoKill = Q_atoi( engine->GetClientConVarValue( pPlayer->entindex(), "hud_classautokill" ) );
	pTFPlayer->SetHudClassAutoKill( nClassAutoKill > 0 ? true : false );

	// keep track of their tf_medigun_autoheal value
	pTFPlayer->SetMedigunAutoHeal( Q_atoi( engine->GetClientConVarValue( pPlayer->entindex(), "tf_medigun_autoheal" ) ) > 0 );

	// keep track of their cl_autoreload value
	pTFPlayer->SetAutoReload( Q_atoi( engine->GetClientConVarValue( pPlayer->entindex(), "cl_autoreload" ) ) > 0 );

#ifdef PF2_DLL
	pTFPlayer->SetGrenadePressThrow( Q_atoi( engine->GetClientConVarValue( pPlayer->entindex(), "pf_grenade_press_throw" ) ) > 0 );
	pTFPlayer->SetHoldZoom( Q_atoi( engine->GetClientConVarValue( pPlayer->entindex(), "pf_holdzoom" ) ) > 0 );

	pTFPlayer->SetToggleSniperCharge( Q_atoi( engine->GetClientConVarValue( pPlayer->entindex(), "pf_sniper_toggle_charge" ) ) > 0 );
	pTFPlayer->SetViewmodelFlipped( Q_atoi( engine->GetClientConVarValue( pPlayer->entindex(), "cl_flipviewmodels" ) ) > 0 );
#endif

	const char *pszFov = engine->GetClientConVarValue( pPlayer->entindex(), "fov_desired" );
	int iFov = atoi(pszFov);
#ifdef PF2_DLL
	iFov = clamp( iFov, MIN_FOV, MAX_FOV );
#else
	iFov = clamp( iFov, 75, 90 );
#endif
	pTFPlayer->SetDefaultFOV( iFov );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGameRules::ClientCommandKeyValues( edict_t *pEntity, KeyValues *pKeyValues )
{
	BaseClass::ClientCommandKeyValues( pEntity, pKeyValues );

	CTFPlayer *pPlayer = ToTFPlayer( CBaseEntity::Instance( pEntity ) );
	if ( !pPlayer )
		return;

	if ( FStrEq( pKeyValues->GetName(), "FreezeCamTaunt" ) )
	{
		int iAchieverIndex = pKeyValues->GetInt( "achiever" );
		CTFPlayer *pAchiever = ToTFPlayer( UTIL_PlayerByIndex( iAchieverIndex ) );

		if ( pAchiever && pAchiever != pPlayer )
		{
			int iClass = pAchiever->GetPlayerClass()->GetClassIndex();

			if ( g_TauntCamAchievements[iClass] != 0 )
			{
				pAchiever->AwardAchievement( g_TauntCamAchievements[iClass] );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Return true if the specified player can carry any more of the ammo type
//-----------------------------------------------------------------------------
bool CTFGameRules::CanHaveAmmo( CBaseCombatCharacter *pPlayer, int iAmmoIndex )
{
	//literally just CanHaveAmmo stuff from gamerules.cpp
	if (iAmmoIndex > -1)
	{
			// Get the max carrying capacity for this ammo
			int iMaxCarry = GetAmmoDef()->MaxCarry(iAmmoIndex);

			// Does the player have room for more of this type of ammo?
			if (pPlayer->GetAmmoCount(iAmmoIndex) < iMaxCarry)
				return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::PlayerKilled( CBasePlayer *pVictim, const CTakeDamageInfo &info )
{
	// Find the killer & the scorer
	CBaseEntity *pInflictor = info.GetInflictor();
	CBaseEntity *pKiller = info.GetAttacker();
	CBaseMultiplayerPlayer *pScorer = ToBaseMultiplayerPlayer( GetDeathScorer( pKiller, pInflictor, pVictim ) );
	CTFPlayer *pAssister = NULL;
	CBaseObject *pObject = NULL;

	// if inflictor or killer is a base object, tell them that they got a kill
	// ( depends if a sentry rocket got the kill, sentry may be inflictor or killer )
	if ( pInflictor )
	{
		if ( pInflictor->IsBaseObject() )
		{
			pObject = dynamic_cast<CBaseObject *>( pInflictor );
		}
		else 
		{
			CBaseEntity *pInflictorOwner = pInflictor->GetOwnerEntity();
			if ( pInflictorOwner && pInflictorOwner->IsBaseObject() )
			{
				pObject = dynamic_cast<CBaseObject *>( pInflictorOwner );
			}
		}
		
	}
	else if( pKiller && pKiller->IsBaseObject() )
	{
		pObject = dynamic_cast<CBaseObject *>( pKiller );
	}

	if ( pObject )
	{
		pObject->IncrementKills();
		pInflictor = pObject;

		if ( pObject->ObjectType() == OBJ_SENTRYGUN )
		{
			CTFPlayer *pOwner = pObject->GetOwner();
			if ( pOwner )
			{
				int iKills = pObject->GetKills();

				// keep track of max kills per a single sentry gun in the player object
				if ( pOwner->GetMaxSentryKills() < iKills )
				{
					pOwner->SetMaxSentryKills( iKills );
					CTF_GameStats.Event_MaxSentryKills( pOwner, iKills );
				}

				// if we just got 10 kills with one sentry, tell the owner's client, which will award achievement if it doesn't have it already
				if ( iKills == 10 )
				{
					pOwner->AwardAchievement( ACHIEVEMENT_TF_GET_TURRETKILLS );
				}
			}
		}
	}

	// if not killed by  suicide or killed by world, see if the scorer had an assister, and if so give the assister credit
	if ( ( pVictim != pScorer ) && pKiller )
	{
		pAssister = ToTFPlayer( GetAssister( pVictim, pScorer, pInflictor ) );
	}	

	//find the area the player is in and see if his death causes a block
	CTriggerAreaCapture *pArea = dynamic_cast<CTriggerAreaCapture *>(gEntList.FindEntityByClassname( NULL, "trigger_capture_area" ) );
	while( pArea )
	{
		if ( pArea->CheckIfDeathCausesBlock( ToBaseMultiplayerPlayer(pVictim), pScorer ) )
			break;

		pArea = dynamic_cast<CTriggerAreaCapture *>( gEntList.FindEntityByClassname( pArea, "trigger_capture_area" ) );
	}

	// determine if this kill affected a nemesis relationship
	int iDeathFlags = 0;
	CTFPlayer *pTFPlayerVictim = ToTFPlayer( pVictim );
	CTFPlayer *pTFPlayerScorer = ToTFPlayer( pScorer );
	if ( pScorer )
	{	
		CalcDominationAndRevenge( pTFPlayerScorer, pTFPlayerVictim, false, &iDeathFlags );
		if ( pAssister )
		{
			CalcDominationAndRevenge( pAssister, pTFPlayerVictim, true, &iDeathFlags );
		}
	}
	pTFPlayerVictim->SetDeathFlags( iDeathFlags );	

	if ( pAssister )
	{
		CTF_GameStats.Event_AssistKill( ToTFPlayer( pAssister ), pVictim );
		if (pObject)
			pObject->IncrementAssists();
	}

#ifdef PF2_DLL
	if ( pTFPlayerVictim->IsPlayerClass( TF_CLASS_CIVILIAN ) && pf_civ_death_ends_round.GetBool() )
	{
		if ( IsEscortMode() )
		{
			SetWinningTeam( TF_TEAM_RED, WINREASON_OPPONENTS_DEAD, true, g_hControlPointMasters[0]->ShouldSwitchTeamsOnRoundWin() );
			PreviousRoundEnd();
		}
	}
#endif

	BaseClass::PlayerKilled( pVictim, info );
}

//-----------------------------------------------------------------------------
// Purpose: Determines if attacker and victim have gotten domination or revenge
//-----------------------------------------------------------------------------
void CTFGameRules::CalcDominationAndRevenge( CTFPlayer *pAttacker, CTFPlayer *pVictim, bool bIsAssist, int *piDeathFlags )
{
	PlayerStats_t *pStatsVictim = CTF_GameStats.FindPlayerStats( pVictim );

	// calculate # of unanswered kills between killer & victim - add 1 to include current kill
	int iKillsUnanswered = pStatsVictim->statsKills.iNumKilledByUnanswered[pAttacker->entindex()] + 1;		
	if ( TF_KILLS_DOMINATION == iKillsUnanswered )
	{			
		// this is the Nth unanswered kill between killer and victim, killer is now dominating victim
		*piDeathFlags |= ( bIsAssist ? TF_DEATH_ASSISTER_DOMINATION : TF_DEATH_DOMINATION );
		// set victim to be dominated by killer
		pAttacker->m_Shared.SetPlayerDominated( pVictim, true );
		// record stats
		CTF_GameStats.Event_PlayerDominatedOther( pAttacker );
	}
	else if ( pVictim->m_Shared.IsPlayerDominated( pAttacker->entindex() ) )
	{
		// the killer killed someone who was dominating him, gains revenge
		*piDeathFlags |= ( bIsAssist ? TF_DEATH_ASSISTER_REVENGE : TF_DEATH_REVENGE );
		// set victim to no longer be dominating the killer
		pVictim->m_Shared.SetPlayerDominated( pAttacker, false );
		// record stats
		CTF_GameStats.Event_PlayerRevenge( pAttacker );
	}

}

//-----------------------------------------------------------------------------
// Purpose: create some proxy entities that we use for transmitting data */
//-----------------------------------------------------------------------------
void CTFGameRules::CreateStandardEntities()
{
	// Create the player resource
	g_pPlayerResource = (CPlayerResource*)CBaseEntity::Create( "tf_player_manager", vec3_origin, vec3_angle );

	// Create the objective resource
	g_pObjectiveResource = (CTFObjectiveResource *)CBaseEntity::Create( "tf_objective_resource", vec3_origin, vec3_angle );

	Assert( g_pObjectiveResource );

	// Create the entity that will send our data to the client.
	CBaseEntity *pEnt = CBaseEntity::Create( "tf_gamerules", vec3_origin, vec3_angle );
	Assert( pEnt );
	pEnt->SetName( AllocPooledString("tf_gamerules" ) );

	CBaseEntity::Create("vote_controller", vec3_origin, vec3_angle);

	new CKickIssue();
	new CRestartGameIssue();
	new CScrambleTeams();
	new CChangeLevelIssue();
	new CNextLevelIssue();
	new CExtendLevelIssue();
}

//-----------------------------------------------------------------------------
// Purpose: determine the class name of the weapon that got a kill
//-----------------------------------------------------------------------------
const char *CTFGameRules::GetKillingWeaponName( const CTakeDamageInfo &info, CTFPlayer *pVictim )
{
	CBaseEntity *pInflictor = info.GetInflictor();
	CBaseEntity *pKiller = info.GetAttacker();
	CBasePlayer *pScorer = TFGameRules()->GetDeathScorer( pKiller, pInflictor, pVictim );
#ifdef PF2_DLL
	CBaseEntity *pWeapon = info.GetWeapon();
#endif
	const char *killer_weapon_name = "world";

	if ( info.GetDamageCustom() == TF_DMG_CUSTOM_BURNING )
	{
		// special-case burning damage, since persistent burning damage may happen after attacker has switched weapons
		killer_weapon_name = "tf_weapon_flamethrower";
	}
#ifdef PF2_DLL
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_NAPALM_BURNING )
	{
		// special-case burning damage, since persistent burning damage may happen after attacker has switched weapons
		killer_weapon_name = "tf_weapon_grenade_napalm_fire";
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_INFECTION )
	{
		killer_weapon_name = "infection";
	}
	else if ( pWeapon )
	{
		killer_weapon_name = STRING( pWeapon->m_iClassname );
	}
#endif
	else if ( pScorer && pInflictor && ( pInflictor == pScorer ) )
	{
		// If the inflictor is the killer,  then it must be their current weapon doing the damage
		if ( pScorer->GetActiveWeapon() )
		{
			killer_weapon_name = pScorer->GetActiveWeapon()->GetClassname(); 
		}
	}
	else if ( pInflictor )
	{
		killer_weapon_name = STRING( pInflictor->m_iClassname );
	}
	else if ( pKiller && pKiller->IsBaseObject())
	{
		killer_weapon_name = STRING( pKiller->m_iClassname );
	}

	// strip certain prefixes from inflictor's classname
	const char *prefix[] = {"tf_weapon_grenade_", "TF_WEAPON_GRENADE_", "tf_weapon_", "NPC_", "func_" };
	for ( int i = 0; i< ARRAYSIZE( prefix ); i++ )
	{
		// if prefix matches, advance the string pointer past the prefix
		int len = Q_strlen( prefix[i] );
		if ( strncmp( killer_weapon_name, prefix[i], len ) == 0 )
		{
			killer_weapon_name += len;
			if( info.GetDamageCustom() == TF_DMG_CUSTOM_GRENADE_BONK )
			{
				killer_weapon_name = UTIL_VarArgs( "%s_bonk", killer_weapon_name );
			}
			break;
		}
	}

	// look out for sentry rocket as weapon and map it to sentry gun, so we get the sentry death icon
	if( 0 == Q_strcmp( killer_weapon_name, "tf_projectile_sentryrocket" ) )
	{
		killer_weapon_name = "obj_sentrygun3";
	}
	else if( 0 == Q_strcmp( killer_weapon_name, "obj_sentrygun" ) )
	{
		CObjectSentrygun *pSentrygun = assert_cast<CObjectSentrygun *>(pInflictor);
		if( pSentrygun )
		{

			int iSentryLevel = pSentrygun->GetUpgradeLevel();
			switch( iSentryLevel )
			{
			case 1:
				killer_weapon_name = "obj_sentrygun";
				break;
			case 2:
				killer_weapon_name = "obj_sentrygun2";
				break;
			case 3:
				killer_weapon_name = "obj_sentrygun3";
				break;
			default:
				killer_weapon_name = "obj_sentrygun";
				break;
			
			}
		}
	}

#ifdef PF2_DLL
	// lazy code for flamerocket
	if ( !Q_strcmp( killer_weapon_name, "tf_projectile_flame_rocket" ) )
	{
		killer_weapon_name = "flame_rocket";
	}

#endif

	return killer_weapon_name;
}

//-----------------------------------------------------------------------------
// Purpose: returns the player who assisted in the kill, or NULL if no assister
//-----------------------------------------------------------------------------
CBasePlayer *CTFGameRules::GetAssister( CBasePlayer *pVictim, CBasePlayer *pScorer, CBaseEntity *pInflictor )
{
	CTFPlayer *pTFScorer = ToTFPlayer( pScorer );
	CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
	if ( pTFScorer && pTFVictim )
	{
		// if victim killed himself, don't award an assist to anyone else, even if there was a recent damager
		if ( pTFScorer == pTFVictim )
			return NULL;

		// If a player is healing the scorer, give that player credit for the assist
		CTFPlayer *pHealer = ToTFPlayer( static_cast<CBaseEntity *>( pTFScorer->m_Shared.GetFirstHealer() ) );
		// Must be a medic to receive a healing assist, otherwise engineers get credit for assists from dispensers doing healing.
		// Also don't give an assist for healing if the inflictor was a sentry gun, otherwise medics healing engineers get assists for the engineer's sentry kills.
		if ( pHealer && ( pHealer->Weapon_OwnsThisID( TF_WEAPON_MEDIGUN ) ) && ( NULL == dynamic_cast<CObjectSentrygun *>( pInflictor ) ) )
		{
			return pHealer;
		}

		// See who has damaged the victim 2nd most recently (most recent is the killer), and if that is within a certain time window.
		// If so, give that player an assist.  (Only 1 assist granted, to single other most recent damager.)
		CTFPlayer *pRecentDamager = GetRecentDamager( pTFVictim, 1, TF_TIME_ASSIST_KILL );
		if ( pRecentDamager && ( pRecentDamager != pScorer ) )
			return pRecentDamager;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Returns specifed recent damager, if there is one who has done damage
//			within the specified time period.  iDamager=0 returns the most recent
//			damager, iDamager=1 returns the next most recent damager.
//-----------------------------------------------------------------------------
CTFPlayer *CTFGameRules::GetRecentDamager( CTFPlayer *pVictim, int iDamager, float flMaxElapsed )
{
	Assert( iDamager < MAX_DAMAGER_HISTORY );

	DamagerHistory_t &damagerHistory = pVictim->GetDamagerHistory( iDamager );
	if ( ( NULL != damagerHistory.hDamager ) && ( gpGlobals->curtime - damagerHistory.flTimeDamage <= flMaxElapsed ) )
	{
		CTFPlayer *pRecentDamager = ToTFPlayer( damagerHistory.hDamager );
		if ( pRecentDamager )
			return pRecentDamager;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Returns who should be awarded the kill
//-----------------------------------------------------------------------------
CBasePlayer *CTFGameRules::GetDeathScorer( CBaseEntity *pKiller, CBaseEntity *pInflictor, CBaseEntity *pVictim )
{
	if ( ( pKiller == pVictim ) && ( pKiller == pInflictor ) )
	{
		// If this was an explicit suicide, see if there was a damager within a certain time window.  If so, award this as a kill to the damager.
		CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
		if ( pTFVictim )
		{
			CTFPlayer *pRecentDamager = GetRecentDamager( pTFVictim, 0, TF_TIME_SUICIDE_KILL_CREDIT );
			if ( pRecentDamager )
				return pRecentDamager;
		}
	}

	return BaseClass::GetDeathScorer( pKiller, pInflictor, pVictim );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pVictim - 
//			*pKiller - 
//			*pInflictor - 
//-----------------------------------------------------------------------------
void CTFGameRules::DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &info )
{
	int killer_ID = 0;

	// Find the killer & the scorer
	CTFPlayer *pTFPlayerVictim = ToTFPlayer( pVictim );
	CBaseEntity *pInflictor = info.GetInflictor();
	CBaseEntity *pKiller = info.GetAttacker();
	CBasePlayer *pScorer = GetDeathScorer( pKiller, pInflictor, pVictim );
	CTFPlayer *pAssister = ToTFPlayer( GetAssister( pVictim, pScorer, pInflictor ) );
	//get pipebomb info for achivement
	CTFGrenadePipebombProjectile *pPipebomb = dynamic_cast<CTFGrenadePipebombProjectile*>( pInflictor );
	// Work out what killed the player, and send a message to all clients about it
	const char *killer_weapon_name = GetKillingWeaponName( info, pTFPlayerVictim );
	
	
	if ( pScorer )	// Is the killer a client?
	{
		killer_ID = pScorer->GetUserID();
#ifdef PF2_DLL
		// Did you kill yourself with a frag
		if( pVictim && pTFPlayerVictim && pScorer == pVictim && !Q_strcmp( killer_weapon_name, "normal_projectile" ) )
		{
			pTFPlayerVictim->m_bKamikazeByFrag = true;
			// Save the frag entindex so we don't get achievement from a different frag
			pTFPlayerVictim->m_iKamikazeFragEntIndex = pInflictor->entindex();
			// run the check for a potential achievement
			m_bCheckForAchievements = true;
		}

		if ( !Q_stricmp( killer_weapon_name, "tranq" ) )
		{
			CSingleUserRecipientFilter filter( pScorer );
			UserMessageBegin( filter, "AchievementEvent" );
			WRITE_SHORT( ACHIEVEMENT_PF_GET_TRANQKILL );
			MessageEnd();
		}
		else if ( !Q_stricmp( killer_weapon_name, "tf_ammo_pack" ) )
		{
			CSingleUserRecipientFilter filter( pScorer );
			UserMessageBegin( filter, "AchievementEvent" );
			WRITE_SHORT( ACHIEVEMENT_PF_EMP_BLAST_AMMOKILL );
			MessageEnd();
		}

		else if ( !Q_stricmp( killer_weapon_name, "obj_teleporter" ) || !Q_stricmp( killer_weapon_name, "obj_dispenser" ) )
		{
			if(pTFPlayerVictim && pTFPlayerVictim->IsPlayerClass( TF_CLASS_SPY ))
			{
				CSingleUserRecipientFilter filter( pScorer );
				UserMessageBegin( filter, "AchievementEvent" );
				WRITE_SHORT( ACHIEVEMENT_PF_FATFINGERS );
				MessageEnd();
			}
		}

		else if( info.GetDamageCustom() == TF_DMG_CUSTOM_GRENADE_BONK )
		{
			ConDColorMsg( Color( 246, 255, 0, 255 ), "killed a player with a nade bonk!\n" );
			CSingleUserRecipientFilter filter( pScorer );
			UserMessageBegin( filter, "AchievementEvent" );
			WRITE_SHORT( ACHIEVEMENT_PF_BONK_BY_NADE );
			MessageEnd();
		}

	
		else if( pPipebomb && pPipebomb->m_bTouchedWallDirectHitOnPlayer && pVictim != pScorer )
		{
			DevMsg( 2, "Killed Player with a grenade from a wall bounce\n" );
			CSingleUserRecipientFilter filter( pScorer );
			UserMessageBegin( filter, "AchievementEvent" );
			WRITE_SHORT( ACHIEVEMENT_PF_KILL_BY_PIPEBOUNCE );
			MessageEnd();
		}

		// Did we die to a frag grenade that wasn't our own?
		if( (pVictim != pScorer) && pTFPlayerVictim && pInflictor && !Q_strcmp( killer_weapon_name, "normal_projectile" ))
		{
			// Set up for out with a bang achievement 
			pTFPlayerVictim->m_iKamikazeFragEntIndex = pInflictor->entindex();
			// run the check for a potential achievement here too incase we somehow missed it
			m_bCheckForAchievements = true;
		}

		
#endif
	}

	if( IsInArenaMode() && tf_arena_first_blood.GetBool() && !m_bFirstBlood && pScorer && pScorer != pTFPlayerVictim )
	{
		m_bFirstBlood = true;
		float flElapsedTime = gpGlobals->curtime - m_flStalemateStartTime;

		if( flElapsedTime <= 20.0 )
		{
			for( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
			{
				BroadcastSound( i, "Announcer.AM_FirstBloodFast" );
			}
		}
		else if( flElapsedTime < 50.0 )
		{
			for( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
			{
				BroadcastSound( i, "Announcer.AM_FirstBloodRandom" );
			}
		}
		else
		{
			for( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
			{
				BroadcastSound( i, "Announcer.AM_FirstBloodFinally" );
			}
		}

		//TODO: Add actual first blood crits back

		float flFirstBloodDuration = tf_arena_first_blood_length.GetFloat();

		CTFPlayer *pTFScorer = ToTFPlayer( pScorer );
		if( flFirstBloodDuration > 0.0f )
			pTFScorer->m_Shared.AddCond( TF_COND_CRITBOOSTED, flFirstBloodDuration );
	}


	IGameEvent * event = gameeventmanager->CreateEvent( "player_death" );

	if ( event )
	{
		event->SetInt( "userid", pVictim->GetUserID() );
		event->SetInt( "attacker", killer_ID );
		event->SetInt( "assister", pAssister ? pAssister->GetUserID() : -1 );
		event->SetString( "weapon", killer_weapon_name );
		event->SetInt( "damagebits", info.GetDamageType() );
		event->SetInt( "customkill", info.GetDamageCustom() );
		event->SetInt( "priority", 7 );	// HLTV event priority, not transmitted
		if ( pTFPlayerVictim->GetDeathFlags() & TF_DEATH_DOMINATION )
		{
			event->SetInt( "dominated", 1 );
		}
		if ( pTFPlayerVictim->GetDeathFlags() & TF_DEATH_ASSISTER_DOMINATION )
		{
			event->SetInt( "assister_dominated", 1 );
		}
		if ( pTFPlayerVictim->GetDeathFlags() & TF_DEATH_REVENGE )
		{
			event->SetInt( "revenge", 1 );
		}
		if ( pTFPlayerVictim->GetDeathFlags() & TF_DEATH_ASSISTER_REVENGE )
		{
			event->SetInt( "assister_revenge", 1 );
		}

		gameeventmanager->FireEvent( event );
	}		
}

void CTFGameRules::ClientDisconnected( edict_t *pClient )
{
	// clean up anything they left behind
	CTFPlayer *pPlayer = ToTFPlayer( GetContainingEntity( pClient ) );
	
	if ( pPlayer )
	{
		if (g_voteController->IsPlayerBeingKicked(pPlayer))
		{
			engine->ServerCommand(UTIL_VarArgs("banid %i %i\n", sv_vote_kick_ban_duration.GetInt()*2, pPlayer->GetUserID()));
			engine->ServerCommand("writeip\n");
			engine->ServerCommand("writeid\n");
		}
		pPlayer->TeamFortress_ClientDisconnected();
	}

	// are any of the spies disguising as this player?
	for ( int i = 1 ; i <= gpGlobals->maxClients ; i++ )
	{
		CTFPlayer *pTemp = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pTemp && pTemp != pPlayer )
		{
			if ( pTemp->m_Shared.GetDisguiseTarget() == pPlayer )
			{
				// choose someone else...
				pTemp->m_Shared.FindDisguiseTarget();
			}
		}
	}

	BaseClass::ClientDisconnected( pClient );
}

// Falling damage stuff.
#define TF_PLAYER_MAX_SAFE_FALL_SPEED	650		

float CTFGameRules::FlPlayerFallDamage( CBasePlayer *pPlayer )
{
	if ( pPlayer->m_Local.m_flFallVelocity > TF_PLAYER_MAX_SAFE_FALL_SPEED )
	{
		// Old TFC damage formula
		float flFallDamage = 5 * (pPlayer->m_Local.m_flFallVelocity / 300);

		// Fall damage needs to scale according to the player's max health, or
		// it's always going to be much more dangerous to weaker classes than larger.
		float flRatio = (float)pPlayer->GetMaxHealth() / 100.0;
		flFallDamage *= flRatio;

		flFallDamage *= random->RandomFloat( 0.8, 1.2 );

		return flFallDamage;
	}

	// Fall caused no damage
	return 0;
}

void CTFGameRules::SendWinPanelInfo( void )
{
	IGameEvent *winEvent = gameeventmanager->CreateEvent( "teamplay_win_panel" );

	if ( winEvent )
	{
		int iBlueScore = GetGlobalTeam( TF_TEAM_BLUE )->GetScore();
		int iRedScore = GetGlobalTeam( TF_TEAM_RED )->GetScore();
		int iBlueScorePrev = iBlueScore;
		int iRedScorePrev = iRedScore;

		bool bRoundComplete = m_bForceMapReset || ( IsGameUnderTimeLimit() && ( GetTimeLeft() <= 0 ) );

		CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
		bool bScoringPerCapture = ( pMaster ) ? ( pMaster->ShouldScorePerCapture() ) : false;

		if ( bRoundComplete && !bScoringPerCapture )
		{
			// if this is a complete round, calc team scores prior to this win
			switch ( m_iWinningTeam )
			{
			case TF_TEAM_BLUE:
				iBlueScorePrev = ( iBlueScore - TEAMPLAY_ROUND_WIN_SCORE >= 0 ) ? ( iBlueScore - TEAMPLAY_ROUND_WIN_SCORE ) : 0;
				break;
			case TF_TEAM_RED:
				iRedScorePrev = ( iRedScore - TEAMPLAY_ROUND_WIN_SCORE >= 0 ) ? ( iRedScore - TEAMPLAY_ROUND_WIN_SCORE ) : 0;
				break;
			case TEAM_UNASSIGNED:
				break;	// stalemate; nothing to do
			}
		}
			
		winEvent->SetInt( "panel_style", WINPANEL_BASIC );
		winEvent->SetInt( "winning_team", m_iWinningTeam );
		winEvent->SetInt( "winreason", m_iWinReason );
		winEvent->SetString( "cappers",  ( m_iWinReason == WINREASON_ALL_POINTS_CAPTURED || m_iWinReason == WINREASON_FLAG_CAPTURE_LIMIT ) ?
			m_szMostRecentCappers : "" );
		winEvent->SetInt( "flagcaplimit", tf_flag_caps_per_round.GetInt() );
		winEvent->SetInt( "blue_score", iBlueScore );
		winEvent->SetInt( "red_score", iRedScore );
		winEvent->SetInt( "blue_score_prev", iBlueScorePrev );
		winEvent->SetInt( "red_score_prev", iRedScorePrev );
		winEvent->SetInt( "round_complete", bRoundComplete );

		CTFPlayerResource *pResource = dynamic_cast< CTFPlayerResource * >( g_pPlayerResource );
		if ( !pResource )
			return;
 
		// determine the 3 players on winning team who scored the most points this round

		// build a vector of players & round scores
		CUtlVector<PlayerRoundScore_t> vecPlayerScore;
		int iPlayerIndex;
		for( iPlayerIndex = 1 ; iPlayerIndex <= MAX_PLAYERS; iPlayerIndex++ )
		{
			CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
			if ( !pTFPlayer || !pTFPlayer->IsConnected() )
				continue;
			// filter out spectators and, if not stalemate, all players not on winning team
			int iPlayerTeam = pTFPlayer->GetTeamNumber();
			if ( ( iPlayerTeam < FIRST_GAME_TEAM ) || ( m_iWinningTeam != TEAM_UNASSIGNED && ( m_iWinningTeam != iPlayerTeam ) ) )
				continue;

			int iRoundScore = 0, iTotalScore = 0;
			PlayerStats_t *pStats = CTF_GameStats.FindPlayerStats( pTFPlayer );
			if ( pStats )
			{
				iRoundScore = CalcPlayerScore( &pStats->statsCurrentRound );
				iTotalScore = CalcPlayerScore( &pStats->statsAccumulated );
			}
			PlayerRoundScore_t &playerRoundScore = vecPlayerScore[vecPlayerScore.AddToTail()];
			playerRoundScore.iPlayerIndex = iPlayerIndex;
			playerRoundScore.iRoundScore = iRoundScore;
			playerRoundScore.iTotalScore = iTotalScore;
		}
		// sort the players by round score
		vecPlayerScore.Sort( PlayerRoundScoreSortFunc );

		// set the top (up to) 3 players by round score in the event data
		int numPlayers = min( 3, vecPlayerScore.Count() );
		for ( int i = 0; i < numPlayers; i++ )
		{
			// only include players who have non-zero points this round; if we get to a player with 0 round points, stop
			if ( 0 == vecPlayerScore[i].iRoundScore )
				break;

			// set the player index and their round score in the event
			char szPlayerIndexVal[64]="", szPlayerScoreVal[64]="";
			Q_snprintf( szPlayerIndexVal, ARRAYSIZE( szPlayerIndexVal ), "player_%d", i+ 1 );
			Q_snprintf( szPlayerScoreVal, ARRAYSIZE( szPlayerScoreVal ), "player_%d_points", i+ 1 );
			winEvent->SetInt( szPlayerIndexVal, vecPlayerScore[i].iPlayerIndex );
			winEvent->SetInt( szPlayerScoreVal, vecPlayerScore[i].iRoundScore );				
		}

		if ( !bRoundComplete && ( TEAM_UNASSIGNED != m_iWinningTeam ) )
		{
			// if this was not a full round ending, include how many mini-rounds remain for winning team to win
			if ( g_hControlPointMasters.Count() && g_hControlPointMasters[0] )
			{
				winEvent->SetInt( "rounds_remaining", g_hControlPointMasters[0]->CalcNumRoundsRemaining( m_iWinningTeam ) );
			}
		}

		// Send the event
		gameeventmanager->FireEvent( winEvent );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sorts players by round score
//-----------------------------------------------------------------------------
int CTFGameRules::PlayerRoundScoreSortFunc( const PlayerRoundScore_t *pRoundScore1, const PlayerRoundScore_t *pRoundScore2 )
{
	// sort first by round score	
	if ( pRoundScore1->iRoundScore != pRoundScore2->iRoundScore )
		return pRoundScore2->iRoundScore - pRoundScore1->iRoundScore;

	// if round scores are the same, sort next by total score
	if ( pRoundScore1->iTotalScore != pRoundScore2->iTotalScore )
		return pRoundScore2->iTotalScore - pRoundScore1->iTotalScore;

	// if scores are the same, sort next by player index so we get deterministic sorting
	return ( pRoundScore2->iPlayerIndex - pRoundScore1->iPlayerIndex );
}

//-----------------------------------------------------------------------------
// Purpose: Called when the teamplay_round_win event is about to be sent, gives
//			this method a chance to add more data to it
//-----------------------------------------------------------------------------
void CTFGameRules::FillOutTeamplayRoundWinEvent( IGameEvent *event )
{
	// determine the losing team
	int iLosingTeam;

	switch( event->GetInt( "team" ) )
	{
	case TF_TEAM_RED:
		iLosingTeam = TF_TEAM_BLUE;
		break;
	case TF_TEAM_BLUE:
		iLosingTeam = TF_TEAM_RED;
		break;
	case TEAM_UNASSIGNED:
	default:
		iLosingTeam = TEAM_UNASSIGNED;
		break;
	}

	// set the number of caps that team got any time during the round
	event->SetInt( "losing_team_num_caps", m_iNumCaps[iLosingTeam] );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SetupSpawnPointsForRound( void )
{
	if ( !g_hControlPointMasters.Count() || !g_hControlPointMasters[0] || !g_hControlPointMasters[0]->PlayingMiniRounds() )
		return;

	CTeamControlPointRound *pCurrentRound = g_hControlPointMasters[0]->GetCurrentRound();
	if ( !pCurrentRound )
	{
		return;
	}

	// loop through the spawn points in the map and find which ones are associated with this round or the control points in this round
	CBaseEntity *pSpot = gEntList.FindEntityByClassname( NULL, "info_player_teamspawn" );
	while( pSpot )
	{
		CTFTeamSpawn *pTFSpawn = assert_cast<CTFTeamSpawn*>(pSpot);

		if ( pTFSpawn )
		{
			CHandle<CTeamControlPoint> hControlPoint = pTFSpawn->GetControlPoint();
			CHandle<CTeamControlPointRound> hRoundBlue = pTFSpawn->GetRoundBlueSpawn();
			CHandle<CTeamControlPointRound> hRoundRed = pTFSpawn->GetRoundRedSpawn();

			if ( hControlPoint && pCurrentRound->IsControlPointInRound( hControlPoint ) )
			{
				// this spawn is associated with one of our control points
				pTFSpawn->SetDisabled( false );
				pTFSpawn->ChangeTeam( hControlPoint->GetOwner() );
			}
			else if ( hRoundBlue && ( hRoundBlue == pCurrentRound ) )
			{
				pTFSpawn->SetDisabled( false );
				pTFSpawn->ChangeTeam( TF_TEAM_BLUE );
			}
			else if ( hRoundRed && ( hRoundRed == pCurrentRound ) )
			{
				pTFSpawn->SetDisabled( false );
				pTFSpawn->ChangeTeam( TF_TEAM_RED );
			}
			else
			{
				// this spawn isn't associated with this round or the control points in this round
				pTFSpawn->SetDisabled( true );
			}
		}

		pSpot = gEntList.FindEntityByClassname( pSpot, "info_player_teamspawn" );
	}
}


int CTFGameRules::SetCurrentRoundStateBitString( void )
{
	m_iPrevRoundState = m_iCurrentRoundState;

	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;

	if ( !pMaster )
	{
		return 0;
	}

	int iState = 0;

	for ( int i=0; i<pMaster->GetNumPoints(); i++ )
	{
		CTeamControlPoint *pPoint = pMaster->GetControlPoint( i );

		if ( pPoint->GetOwner() == TF_TEAM_BLUE )
		{
			// Set index to 1 for the point being owned by blue
			iState |= ( 1<<i );
		}
	}

	m_iCurrentRoundState = iState;

	return iState;
}


void CTFGameRules::SetMiniRoundBitMask( int iMask )
{
	m_iCurrentMiniRoundMask = iMask;
}

//-----------------------------------------------------------------------------
// Purpose: NULL pPlayer means show the panel to everyone
//-----------------------------------------------------------------------------
void CTFGameRules::ShowRoundInfoPanel( CTFPlayer *pPlayer /* = NULL */ )
{
	KeyValues *data = new KeyValues( "data" );

	if ( m_iCurrentRoundState < 0 )
	{
		// Haven't set up the round state yet
		return;
	}

	// if prev and cur are equal, we are starting from a fresh round
	if ( m_iPrevRoundState >= 0 && pPlayer == NULL )	// we have data about a previous state
	{
		data->SetInt( "prev", m_iPrevRoundState );
	}
	else
	{
		// don't send a delta if this is just to one player, they are joining mid-round
		data->SetInt( "prev", m_iCurrentRoundState );	
	}

	data->SetInt( "cur", m_iCurrentRoundState );

	// get bitmask representing the current miniround
	data->SetInt( "round", m_iCurrentMiniRoundMask );

	if ( pPlayer )
	{
		pPlayer->ShowViewPortPanel( PANEL_ROUNDINFO, true, data );
	}
	else
	{
		for ( int i = 1;  i <= MAX_PLAYERS; i++ )
		{
			CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

			if ( pTFPlayer && pTFPlayer->IsReadyToPlay() )
			{
				pTFPlayer->ShowViewPortPanel( PANEL_ROUNDINFO, true, data );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::TimerMayExpire( void )
{
	// Prevent timers expiring while control points are contested
	int iNumControlPoints = ObjectiveResource()->GetNumControlPoints();
	for ( int iPoint = 0; iPoint < iNumControlPoints; iPoint++ )
	{
		if ( ObjectiveResource()->GetCappingTeam( iPoint ) )
		{
			// HACK: Fix for some maps adding time to the clock 0.05s after CP is capped.
			m_flTimerMayExpireAt = gpGlobals->curtime + 0.1f;
			return false;
		}
	}

	if ( m_flTimerMayExpireAt >= gpGlobals->curtime )
		return false;

	return BaseClass::TimerMayExpire();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::RoundRespawn( void )
{
	// remove any buildings, grenades, rockets, etc. the player put into the world
	for ( int i = 1;  i <= MAX_PLAYERS; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

		if ( pPlayer )
		{
			pPlayer->TeamFortress_RemoveEverythingFromWorld();
		}
	}
	if(!IsInTournamentMode())
		Arena_RunTeamLogic();

	// reset the flag captures
	int nTeamCount = TFTeamMgr()->GetTeamCount();
	for ( int iTeam = FIRST_GAME_TEAM; iTeam < nTeamCount; ++iTeam )
	{
		CTFTeam *pTeam = GetGlobalTFTeam( iTeam );
		if ( !pTeam )
			continue;

		pTeam->SetFlagCaptures( 0 );
	}

	CTF_GameStats.ResetRoundStats();

	BaseClass::RoundRespawn();

	// ** AFTER WE'VE BEEN THROUGH THE ROUND RESPAWN, SHOW THE ROUNDINFO PANEL
	if ( !IsInWaitingForPlayers() )
	{
		ShowRoundInfoPanel();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::InternalHandleTeamWin( int iWinningTeam )
{
	// remove any spies' disguises and make them visible (for the losing team only)
	// and set the speed for both teams (winners get a boost and losers have reduced speed)
	for ( int i = 1;  i <= MAX_PLAYERS; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

		if ( pPlayer )
		{
			if ( pPlayer->GetTeamNumber() > LAST_SHARED_TEAM )
			{
				if ( pPlayer->GetTeamNumber() != iWinningTeam )
				{
					pPlayer->RemoveInvisibility();
//					pPlayer->RemoveDisguise();

					if ( pPlayer->HasTheFlag() )
					{
						pPlayer->DropFlag();
					}
					// Hide their weapon.
					CTFWeaponBase *pWeapon = pPlayer->GetActiveTFWeapon();
					if ( pWeapon )
					{
						pWeapon->SetWeaponVisible( false );
					}
				}

				pPlayer->TeamFortress_SetSpeed();
			}
		}
	}

	// disable any sentry guns the losing team has built
	CBaseEntity *pEnt = NULL;
	while ( ( pEnt = gEntList.FindEntityByClassname( pEnt, "obj_sentrygun" ) ) != NULL )
	{
		CObjectSentrygun *pSentry = dynamic_cast<CObjectSentrygun *>( pEnt );
		if ( pSentry )
		{
			if ( pSentry->GetTeamNumber() != iWinningTeam )
			{
				pSentry->SetDisabled( true );
			}
		}
	}

	if ( m_bForceMapReset )
	{
		m_iPrevRoundState = -1;
		m_iCurrentRoundState = -1;
		m_iCurrentMiniRoundMask = 0;
	}
}

// sort function for the list of players that we're going to use to scramble the teams
int ScramblePlayersSort( CTFPlayer *const *p1, CTFPlayer *const *p2 )
{
	CTFPlayerResource *pResource = dynamic_cast<CTFPlayerResource *>(g_pPlayerResource);
	if( pResource )
	{
		// check the priority
		if( pResource->GetTotalScore( (*p2)->entindex() ) > pResource->GetTotalScore( (*p1)->entindex() ) )
		{
			return 1;
		}
	}

	return -1;
}

// sort function for the player spectator queue
int SortPlayerSpectatorQueue( CTFPlayer *const *p1, CTFPlayer *const *p2 )
{
	// get the queue times of both players
	float flQueueTime1 = 0.0f, flQueueTime2 = 0.0f;
	flQueueTime1 = (*p1)->m_flArenaQueueTime;
	flQueueTime2 = (*p2)->m_flArenaQueueTime;

	if( flQueueTime1 == flQueueTime2 )
	{
		// if both players queued at the same time see who's been in the server longer
		flQueueTime1 = (*p1)->GetConnectionTime();
		flQueueTime2 = (*p2)->GetConnectionTime();
		if( flQueueTime1 < flQueueTime2 )
			return -1;
	}
	else if( flQueueTime1 > flQueueTime2 )
	{
		// the player with the higher queue time queued more recently (gpGlobals->curtime)
		return -1;
	}

	return flQueueTime1 != flQueueTime2;
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::HandleScrambleTeams( void )
{
	int i = 0;
	CTFPlayer *pTFPlayer = NULL;
	CUtlVector<CTFPlayer *> pListPlayers;

	// add all the players (that are on blue or red) to our temp list
	for ( i = 1 ; i <= gpGlobals->maxClients ; i++ )
	{
		pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pTFPlayer && ( pTFPlayer->GetTeamNumber() >= TF_TEAM_RED ) )
		{
			pListPlayers.AddToHead( pTFPlayer );
		}
	}

	// sort the list
	pListPlayers.Sort( ScramblePlayersSort );

	// loop through and put everyone on Spectator to clear the teams (or the autoteam step won't work correctly)
	for ( i = 0 ; i < pListPlayers.Count() ; i++ )
	{
		pTFPlayer = pListPlayers[i];

		if ( pTFPlayer )
		{
			pTFPlayer->ForceChangeTeam( TEAM_SPECTATOR );
		}
	}

	// loop through and auto team everyone
	for ( i = 0 ; i < pListPlayers.Count() ; i++ )
	{
		pTFPlayer = pListPlayers[i];

		if ( pTFPlayer )
		{
			pTFPlayer->ForceChangeTeam( TF_TEAM_AUTOASSIGN );
		}
	}

	ResetTeamsRoundWinTracking();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::HandleSwitchTeams( void )
{
	int i = 0;

	// respawn the players
	for ( i = 1 ; i <= gpGlobals->maxClients ; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pPlayer )
		{
			pPlayer->TeamFortress_RemoveEverythingFromWorld();

			// Ignore players who aren't on an active team
			if ( pPlayer->GetTeamNumber() != TF_TEAM_RED && pPlayer->GetTeamNumber() != TF_TEAM_BLUE )
			{
				continue;
			}

			if ( pPlayer->GetTeamNumber() == TF_TEAM_RED )
			{
				pPlayer->ForceChangeTeam( TF_TEAM_BLUE, true );
			}
			else if ( pPlayer->GetTeamNumber() == TF_TEAM_BLUE )
			{
				pPlayer->ForceChangeTeam( TF_TEAM_RED, true );
			}
#ifdef PF2_DLL
			// If we're in escort mode, force everyone onto class_undefined
			if ( IsEscortMode() )
			{
				if (pf_use_escort_class_restrictions.GetBool())
				{
					pPlayer->SetDesiredPlayerClassIndex( TF_CLASS_UNDEFINED );

					if (pPlayer->GetTeamNumber() == TF_TEAM_RED)
					{
						pPlayer->SetDesiredPlayerClassIndex( TF_CLASS_SNIPER );
					}
				}
				else
				{
					if ( pPlayer->IsPlayerClass( TF_CLASS_CIVILIAN ) )
					{
						pPlayer->SetDesiredPlayerClassIndex( TF_CLASS_UNDEFINED );
					}
				}
			}
#endif
		}
	}

	// switch the team scores
	CTFTeam *pRedTeam = GetGlobalTFTeam( TF_TEAM_RED );
	CTFTeam *pBlueTeam = GetGlobalTFTeam( TF_TEAM_BLUE );
	if ( pRedTeam && pBlueTeam )
	{
		int nRed = pRedTeam->GetScore();
		int nBlue = pBlueTeam->GetScore();

		pRedTeam->SetScore( nBlue );
		pBlueTeam->SetScore( nRed );
	}
}

bool CTFGameRules::CanChangeClassInStalemate( void ) 
{ 
	return (gpGlobals->curtime < (m_flStalemateStartTime + tf_stalematechangeclasstime.GetFloat())); 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SetRoundOverlayDetails( void )
{
	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;

	if ( pMaster && pMaster->PlayingMiniRounds() )
	{
		CTeamControlPointRound *pRound = pMaster->GetCurrentRound();

		if ( pRound )
		{
			CHandle<CTeamControlPoint> pRedPoint = pRound->GetPointOwnedBy( TF_TEAM_RED );
			CHandle<CTeamControlPoint> pBluePoint = pRound->GetPointOwnedBy( TF_TEAM_BLUE );

			// do we have opposing points in this round?
			if ( pRedPoint && pBluePoint )
			{
				int iMiniRoundMask = ( 1<<pBluePoint->GetPointIndex() ) | ( 1<<pRedPoint->GetPointIndex() );
				SetMiniRoundBitMask( iMiniRoundMask );
			}
			else
			{
				SetMiniRoundBitMask( 0 );
			}

			SetCurrentRoundStateBitString();
		}
	}

	BaseClass::SetRoundOverlayDetails();
}

#ifdef PF2_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::IsEscortMode( void )
{
	if ( g_hControlPointMasters.Count() )
	{
		return g_hControlPointMasters[0]->IsEscort();
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::CanJoinClass( CTFPlayer *pTFPlayer, int iClass )
{
	// Escort mode
	// Red team may only have snipers
	// Blue team may only have civilians, soldiers, hwguys, and medics
	
	if ( IsEscortMode() )
	{
		if ( pTFPlayer->GetTeamNumber() == TF_TEAM_RED )
		{
			if (iClass != TF_CLASS_SNIPER && pf_use_escort_class_restrictions.GetBool())
			{
				return false;
			}
			else
			{
				if (iClass == TF_CLASS_CIVILIAN)
				{
					return false;
				}
			}
		}
		
		if (pTFPlayer->GetTeamNumber() == TF_TEAM_BLUE && pf_use_escort_class_restrictions.GetBool() )
		{
			if ( !( iClass == TF_CLASS_SOLDIER || iClass == TF_CLASS_MEDIC || iClass == TF_CLASS_HEAVYWEAPONS || iClass == TF_CLASS_CIVILIAN ) )
			{
				return false;
			}
		}
		else
		{
			return true;
		}
	}

	else if ( iClass == TF_CLASS_CIVILIAN )
	{
		if ( !pf_allow_special_class.GetBool() )
		{
			return false;
		}
	}
	
	return true;
}
#endif
//-----------------------------------------------------------------------------
// Purpose: Returns whether a team should score for each captured point
//-----------------------------------------------------------------------------
bool CTFGameRules::ShouldScorePerRound( void )
{ 
	bool bRetVal = true;

	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
	if ( pMaster && pMaster->ShouldScorePerCapture() )
	{
		bRetVal = false;
	}

	return bRetVal;
}

#endif  // GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGameRules::GetFarthestOwnedControlPoint( int iTeam, bool bWithSpawnpoints )
{
	int iOwnedEnd = ObjectiveResource()->GetBaseControlPointForTeam( iTeam );
	if ( iOwnedEnd == -1 )
		return -1;

	int iNumControlPoints = ObjectiveResource()->GetNumControlPoints();
	int iWalk = 1;
	int iEnemyEnd = iNumControlPoints-1;
	if ( iOwnedEnd != 0 )
	{
		iWalk = -1;
		iEnemyEnd = 0;
	}

	// Walk towards the other side, and find the farthest owned point that has spawn points
	int iFarthestPoint = iOwnedEnd;
	for ( int iPoint = iOwnedEnd; iPoint != iEnemyEnd; iPoint += iWalk )
	{
		// If we've hit a point we don't own, we're done
		if ( ObjectiveResource()->GetOwningTeam( iPoint ) != iTeam )
			break;

		if ( bWithSpawnpoints && !m_bControlSpawnsPerTeam[iTeam][iPoint] )
			continue;

		iFarthestPoint = iPoint;
	}

	return iFarthestPoint;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::TeamMayCapturePoint( int iTeam, int iPointIndex ) 
{ 
	// Is point capturing allowed at all?
	if (IsInKothMode() && IsInWaitingForPlayers())
		return false;

	// If the point is explicitly locked it can't be capped.
	if (ObjectiveResource()->GetCPLocked(iPointIndex))
		return false;

	if ( !tf_caplinear.GetBool() )
		return true; 

	// Any previous points necessary?
	int iPointNeeded = ObjectiveResource()->GetPreviousPointForPoint( iPointIndex, iTeam, 0 );

	// Points set to require themselves are always cappable 
	if ( iPointNeeded == iPointIndex )
		return true;

	// No required points specified? Require all previous points.
	if ( iPointNeeded == -1 )
	{
		if ( !ObjectiveResource()->PlayingMiniRounds() )
		{
			// No custom previous point, team must own all previous points
			int iFarthestPoint = GetFarthestOwnedControlPoint( iTeam, false );
			return (abs(iFarthestPoint - iPointIndex) <= 1);
		}
		else
		{
			// No custom previous point, team must own all previous points in the current mini-round
			//tagES TFTODO: need to figure out a good algorithm for this


			if( IsInArenaMode() )
				return State_Get() == GR_STATE_STALEMATE;
			return true;
		}
	}

	// Loop through each previous point and see if the team owns it
	for ( int iPrevPoint = 0; iPrevPoint < MAX_PREVIOUS_POINTS; iPrevPoint++ )
	{
		int iPointNeeded = ObjectiveResource()->GetPreviousPointForPoint( iPointIndex, iTeam, iPrevPoint );
		if ( iPointNeeded != -1 )
		{
			if ( ObjectiveResource()->GetOwningTeam( iPointNeeded ) != iTeam )
				return false;
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::PlayerMayCapturePoint( CBasePlayer *pPlayer, int iPointIndex, char *pszReason /* = NULL */, int iMaxReasonLength /* = 0 */ )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );

	if ( !pTFPlayer )
	{
		return false;
	}

#ifdef PF2
	if (m_bControlPointsNeedingFlags.Get(iPointIndex))
	{
		if (!pTFPlayer->HasTheFlag())
		{
			if (pszReason)
			{
				Q_snprintf(pszReason, iMaxReasonLength, "#Cant_cap_flag");
			}
			return false;
		}
	}

	// Only civilian can capture on escort maps
	if ( !pTFPlayer->IsPlayerClass( TF_CLASS_CIVILIAN ) && m_iCivilianGoals.Get( iPointIndex ) )
	{
		if ( pszReason )
		{
			Q_snprintf( pszReason, iMaxReasonLength, "#Cant_cap_stealthed" );	// PFTODO: Change reason string to something else
		}
		return false;
	}
#endif

	// Disguised and invisible spies cannot capture points
#ifdef PF2
	if ( pTFPlayer->m_Shared.InCond( TF_COND_STEALTHED ) || pTFPlayer->m_Shared.InCond( TF_COND_SMOKE_BOMB ) )
#else
	if ( pTFPlayer->m_Shared.InCond( TF_COND_STEALTHED ) )
#endif
	{
		if ( pszReason )
		{
			Q_snprintf( pszReason, iMaxReasonLength, "#Cant_cap_stealthed" );
		}
		return false;
	}

	if ( pTFPlayer->m_Shared.InCond( TF_COND_INVULNERABLE ) )
	{
		if ( pszReason )
		{
			Q_snprintf( pszReason, iMaxReasonLength, "#Cant_cap_invuln" );
		}
		return false;
	}

	if( pTFPlayer->m_Shared.InCond( TF_COND_DISGUISED ) && (pTFPlayer->m_Shared.GetDisguiseTeam() != pTFPlayer->GetTeamNumber()) )
	{
		if ( pszReason )
		{
			Q_snprintf( pszReason, iMaxReasonLength, "#Cant_cap_disguised" );
		}
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::PlayerMayBlockPoint( CBasePlayer *pPlayer, int iPointIndex, char *pszReason, int iMaxReasonLength )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	if ( !pTFPlayer )
		return false;

#ifdef PF2
	if (m_bControlPointsNeedingFlags.Get(iPointIndex))
	{
		if (!pTFPlayer->HasTheFlag())
		{
			return true;
		}
	}
#endif

	// Invuln players can block points
	if ( pTFPlayer->m_Shared.InCond( TF_COND_INVULNERABLE ) )
	{
		if ( pszReason )
		{
			Q_snprintf( pszReason, iMaxReasonLength, "#Cant_cap_invuln" );
		}
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Calculates score for player
//-----------------------------------------------------------------------------
int CTFGameRules::CalcPlayerScore( RoundStats_t *pRoundStats )
{
	int iScore =	( pRoundStats->m_iStat[TFSTAT_KILLS] * TF_SCORE_KILL ) + 
					( pRoundStats->m_iStat[TFSTAT_CAPTURES] * TF_SCORE_CAPTURE ) + 
					( pRoundStats->m_iStat[TFSTAT_DEFENSES] * TF_SCORE_DEFEND ) + 
					( pRoundStats->m_iStat[TFSTAT_BUILDINGSDESTROYED] * TF_SCORE_DESTROY_BUILDING ) + 
					( pRoundStats->m_iStat[TFSTAT_HEADSHOTS] * TF_SCORE_HEADSHOT ) + 
					( pRoundStats->m_iStat[TFSTAT_BACKSTABS] * TF_SCORE_BACKSTAB ) + 
					( pRoundStats->m_iStat[TFSTAT_HEALING] / TF_SCORE_HEAL_HEALTHUNITS_PER_POINT ) +  
					( pRoundStats->m_iStat[TFSTAT_KILLASSISTS] / TF_SCORE_KILL_ASSISTS_PER_POINT ) + 
					( pRoundStats->m_iStat[TFSTAT_TELEPORTS] / TF_SCORE_TELEPORTS_PER_POINT ) +
					( pRoundStats->m_iStat[TFSTAT_INVULNS] / TF_SCORE_INVULN ) +
					( pRoundStats->m_iStat[TFSTAT_REVENGE] / TF_SCORE_REVENGE );
	return max( iScore, 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::IsBirthday( void )
{
	if ( IsX360() )
		return false;

	if ( m_iBirthdayMode == BIRTHDAY_RECALCULATE )
	{
		m_iBirthdayMode = BIRTHDAY_OFF;
		if ( tf_birthday.GetBool() )
		{
			m_iBirthdayMode = BIRTHDAY_ON;
		}
		else
		{
			time_t ltime = time(0);
			const time_t *ptime = &ltime;
			struct tm *today = localtime( ptime );
			if ( today )
			{
				if ( today->tm_mon == 7 && today->tm_mday == 24 )
				{
					m_iBirthdayMode = BIRTHDAY_ON;
				}
			}
		}
	}

	return ( m_iBirthdayMode == BIRTHDAY_ON );
}

#ifdef PF2
bool CTFGameRules::IsApril()
{
	time_t ltime = time( 0 );
	const time_t *ptime = &ltime;
	struct tm* today = localtime( ptime );
	if ( today )
	{
		if ( today->tm_mon == 3 && today->tm_mday >= 12 )
			return true;
	}

	return false;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::ShouldCollide( int collisionGroup0, int collisionGroup1 )
{
	if ( collisionGroup0 > collisionGroup1 )
	{
		// swap so that lowest is always first
		V_swap( collisionGroup0, collisionGroup1 );
	}
	
	//Don't stand on COLLISION_GROUP_WEAPONs
	if( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT &&
		collisionGroup1 == COLLISION_GROUP_WEAPON )
	{
		return false;
	}

	// Don't stand on projectiles
	if( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT &&
		collisionGroup1 == COLLISION_GROUP_PROJECTILE )
	{
		return false;
	}

	// Rockets need to collide with players when they hit, but
	// be ignored by player movement checks
	if ( ( collisionGroup0 == COLLISION_GROUP_PLAYER ) && 
		( collisionGroup1 == TFCOLLISION_GROUP_ROCKETS ) )
		return true;

	if ( ( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT ) && 
		( collisionGroup1 == TFCOLLISION_GROUP_ROCKETS ) )
		return false;

	if ( ( collisionGroup0 == COLLISION_GROUP_WEAPON ) && 
		( collisionGroup1 == TFCOLLISION_GROUP_ROCKETS ) )
		return false;

	if ( ( collisionGroup0 == TF_COLLISIONGROUP_GRENADES ) && 
		( collisionGroup1 == TFCOLLISION_GROUP_ROCKETS ) )
		return false;

	if ((collisionGroup0 == TFCOLLISION_GROUP_ROCKETS) &&
		(collisionGroup1 == TFCOLLISION_GROUP_ROCKETS))
		return false;

	if ((collisionGroup0 == COLLISION_GROUP_PROJECTILE) &&
		(collisionGroup1 == TFCOLLISION_GROUP_ROCKETS))
		return false;

	if ( ( collisionGroup0 == COLLISION_GROUP_PLAYER ) && 
		( collisionGroup1 == TF_COLLISIONGROUP_GRENADES ) )
		return true;

	if ( ( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT ) && 
		( collisionGroup1 == TF_COLLISIONGROUP_GRENADES ) )
		return false;

	// Respawn rooms only collide with players
	if ( collisionGroup1 == TFCOLLISION_GROUP_RESPAWNROOMS )
		return ( collisionGroup0 == COLLISION_GROUP_PLAYER ) || ( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT );
	
/*	if ( collisionGroup0 == COLLISION_GROUP_PLAYER )
	{
		// Players don't collide with objects or other players
		if ( collisionGroup1 == COLLISION_GROUP_PLAYER  )
			 return false;
 	}

	if ( collisionGroup1 == COLLISION_GROUP_PLAYER_MOVEMENT )
	{
		// This is only for probing, so it better not be on both sides!!!
		Assert( collisionGroup0 != COLLISION_GROUP_PLAYER_MOVEMENT );

		// No collide with players any more
		// Nor with objects or grenades
		switch ( collisionGroup0 )
		{
		default:
			break;
		case COLLISION_GROUP_PLAYER:
			return false;
		}
	}
*/
	// don't want caltrops and other grenades colliding with each other
	// caltops getting stuck on other caltrops, etc.)
	if ( ( collisionGroup0 == TF_COLLISIONGROUP_GRENADES ) && 
		 ( collisionGroup1 == TF_COLLISIONGROUP_GRENADES ) )
	{
		return false;
	}


	if ( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT &&
		collisionGroup1 == TFCOLLISION_GROUP_COMBATOBJECT )
	{
		return false;
	}

	if ( collisionGroup0 == COLLISION_GROUP_PLAYER &&
		collisionGroup1 == TFCOLLISION_GROUP_COMBATOBJECT )
	{
		return false;
	}

	if( collisionGroup0 == TF_COLLISIONGROUP_GRENADES &&
		collisionGroup1 == TFCOLLISION_GROUP_COMBATOBJECT )
	{
		return true;
	}

	return BaseClass::ShouldCollide( collisionGroup0, collisionGroup1 ); 
}

//-----------------------------------------------------------------------------
// Purpose: Return the value of this player towards capturing a point
//-----------------------------------------------------------------------------
int	CTFGameRules::GetCaptureValueForPlayer( CBasePlayer *pPlayer )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	if ( pTFPlayer->IsPlayerClass( TF_CLASS_SCOUT ) )
	{
		if ( mp_capstyle.GetInt() == 1 )
		{
			// Scouts count for 2 people in timebased capping.
			return 2;
		}
		else
		{
			// Scouts can cap all points on their own.
			return 10;
		}
	}

	return BaseClass::GetCaptureValueForPlayer( pPlayer );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGameRules::GetTimeLeft( void )
{
	float flTimeLimit = mp_timelimit.GetInt() * 60;

	Assert( flTimeLimit > 0 && "Should not call this function when !IsGameUnderTimeLimit" );

	float flMapChangeTime = m_flMapResetTime + flTimeLimit;

	return ( (int)(flMapChangeTime - gpGlobals->curtime) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::FireGameEvent( IGameEvent *event )
{
	const char *eventName = event->GetName();

	if ( !Q_strcmp( eventName, "teamplay_point_captured" ) )
	{
#ifdef GAME_DLL
		RecalculateControlPointState();

		// keep track of how many times each team caps
		int iTeam = event->GetInt( "team" );
		Assert( iTeam >= FIRST_GAME_TEAM && iTeam < TF_TEAM_COUNT );
		m_iNumCaps[iTeam]++;

		// award a capture to all capping players
		const char *cappers = event->GetString( "cappers" );

		Q_strncpy( m_szMostRecentCappers, cappers, ARRAYSIZE( m_szMostRecentCappers ) );	
		for ( int i =0; i < Q_strlen( cappers ); i++ )
		{
			int iPlayerIndex = (int) cappers[i];
			CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
			if ( pPlayer )
			{
				CTF_GameStats.Event_PlayerCapturedPoint( pPlayer );				
			}
		}
#endif
	}
	else if ( !Q_strcmp( eventName, "teamplay_capture_blocked" ) )
	{
#ifdef GAME_DLL
		int iPlayerIndex = event->GetInt( "blocker" );
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
		CTF_GameStats.Event_PlayerDefendedPoint( pPlayer );
#endif
	}	
	else if ( !Q_strcmp( eventName, "teamplay_round_win" ) )
	{
#ifdef GAME_DLL
		int iWinningTeam = event->GetInt( "team" );
		bool bFullRound = event->GetBool( "full_round" );
		float flRoundTime = event->GetFloat( "round_time" );
		bool bWasSuddenDeath = event->GetBool( "was_sudden_death" );
		CTF_GameStats.Event_RoundEnd( iWinningTeam, bFullRound, flRoundTime, bWasSuddenDeath );
#endif
	}
	else if ( !Q_strcmp( eventName, "teamplay_flag_event" ) )
	{
#ifdef GAME_DLL
		// if this is a capture event, remember the player who made the capture		
		int iEventType = event->GetInt( "eventtype" );
		if ( TF_FLAGEVENT_CAPTURE == iEventType )
		{
			int iPlayerIndex = event->GetInt( "player" );
			m_szMostRecentCappers[0] = iPlayerIndex;
			m_szMostRecentCappers[1] = 0;
		}
#endif
	}
	else if( !Q_strcmp( eventName, "teamplay_point_unlocked" ) )
	{
#if defined( GAME_DLL )
		// if this is an unlock event and we're in arena, fire OnCapEnabled		
		CArenaLogic *pArena = dynamic_cast<CArenaLogic *>(gEntList.FindEntityByClassname( NULL, "tf_logic_arena" ));
		if( pArena )
		{
			pArena->OnCapEnabled();
		}
#endif
	}
#ifdef CLIENT_DLL
	else if ( !Q_strcmp( eventName, "game_newmap" ) )
	{
		m_iBirthdayMode = BIRTHDAY_RECALCULATE;
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Init ammo definitions
//-----------------------------------------------------------------------------

// shared ammo definition
// JAY: Trying to make a more physical bullet response
#define BULLET_MASS_GRAINS_TO_LB(grains)	(0.002285*(grains)/16.0f)
#define BULLET_MASS_GRAINS_TO_KG(grains)	lbs2kg(BULLET_MASS_GRAINS_TO_LB(grains))

// exaggerate all of the forces, but use real numbers to keep them consistent
#define BULLET_IMPULSE_EXAGGERATION			1	

// convert a velocity in ft/sec and a mass in grains to an impulse in kg in/s
#define BULLET_IMPULSE(grains, ftpersec)	((ftpersec)*12*BULLET_MASS_GRAINS_TO_KG(grains)*BULLET_IMPULSE_EXAGGERATION)


CAmmoDef* GetAmmoDef()
{
	static CAmmoDef def;
	static bool bInitted = false;

	if ( !bInitted )
	{
		bInitted = true;
		
		// Start at 1 here and skip the dummy ammo type to make CAmmoDef use the same indices
		// as our #defines.
		for ( int i=1; i < TF_AMMO_COUNT; i++ )
		{
			def.AddAmmoType( g_aAmmoNames[i], DMG_BULLET, TRACER_LINE, 0, 0, "ammo_max", 2400, 10, 14 );
			Assert( def.Index( g_aAmmoNames[i] ) == i );
		}
	}

	return &def;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFGameRules::GetTeamGoalString( int iTeam )
{
	if ( iTeam == TF_TEAM_RED )
		return m_pszTeamGoalStringRed.Get();
	if ( iTeam == TF_TEAM_BLUE )
		return m_pszTeamGoalStringBlue.Get();
	return NULL;
}

bool CTFGameRules::IsConnectedUserInfoChangeAllowed( CBasePlayer *pPlayer )
{
	CBasePlayer *pBasePlayer = pPlayer;
#ifdef CLIENT_DLL
	// CHLClient calls this with NULL so we need to get the local player
	pBasePlayer = CBasePlayer::GetLocalPlayer();
#endif
	if( pBasePlayer )
	{
		// Allow players to change shit if they're spectating or dead
		if( pBasePlayer->GetTeamNumber() <= LAST_SHARED_TEAM )
		{
			return true;
		}
		if( !pBasePlayer->IsAlive() )
		{
			return true;
		}
	}
	return false;
}

#ifdef PF2
void CTFGameRules::SetControlPointNeedingFlag(int index, bool flagneeding)
{
	m_bControlPointsNeedingFlags.Set(index, flagneeding);
}

void CTFGameRules::SetCivilianGoals( int iIndex, bool bCivilianGoal )
{
	m_iCivilianGoals.Set( iIndex, bCivilianGoal );
}
#endif

#ifdef GAME_DLL

	Vector MaybeDropToGround( 
							CBaseEntity *pMainEnt, 
							bool bDropToGround, 
							const Vector &vPos, 
							const Vector &vMins, 
							const Vector &vMaxs )
	{
		if ( bDropToGround )
		{
			trace_t trace;
			UTIL_TraceHull( vPos, vPos + Vector( 0, 0, -500 ), vMins, vMaxs, MASK_SOLID, pMainEnt, COLLISION_GROUP_NONE, &trace );
			return trace.endpos;
		}
		else
		{
			return vPos;
		}
	}

	//-----------------------------------------------------------------------------
	// Purpose: This function can be used to find a valid placement location for an entity.
	//			Given an origin to start looking from and a minimum radius to place the entity at,
	//			it will sweep out a circle around vOrigin and try to find a valid spot (on the ground)
	//			where mins and maxs will fit.
	// Input  : *pMainEnt - Entity to place
	//			&vOrigin - Point to search around
	//			fRadius - Radius to search within
	//			nTries - Number of tries to attempt
	//			&mins - mins of the Entity
	//			&maxs - maxs of the Entity
	//			&outPos - Return point
	// Output : Returns true and fills in outPos if it found a spot.
	//-----------------------------------------------------------------------------
	bool EntityPlacementTest( CBaseEntity *pMainEnt, const Vector &vOrigin, Vector &outPos, bool bDropToGround )
	{
		// This function moves the box out in each dimension in each step trying to find empty space like this:
		//
		//											  X  
		//							   X			  X  
		// Step 1:   X     Step 2:    XXX   Step 3: XXXXX
		//							   X 			  X  
		//											  X  
		//

		Vector mins, maxs;
		pMainEnt->CollisionProp()->WorldSpaceAABB( &mins, &maxs );
		mins -= pMainEnt->GetAbsOrigin();
		maxs -= pMainEnt->GetAbsOrigin();

		// Put some padding on their bbox.
		float flPadSize = 5;
		Vector vTestMins = mins - Vector( flPadSize, flPadSize, flPadSize );
		Vector vTestMaxs = maxs + Vector( flPadSize, flPadSize, flPadSize );

		// First test the starting origin.
		if ( UTIL_IsSpaceEmpty( pMainEnt, vOrigin + vTestMins, vOrigin + vTestMaxs ) )
		{
			outPos = MaybeDropToGround( pMainEnt, bDropToGround, vOrigin, vTestMins, vTestMaxs );
			return true;
		}

		Vector vDims = vTestMaxs - vTestMins;


		// Keep branching out until we get too far.
		int iCurIteration = 0;
		int nMaxIterations = 15;

		int offset = 0;
		do
		{
			for ( int iDim=0; iDim < 3; iDim++ )
			{
				float flCurOffset = offset * vDims[iDim];

				for ( int iSign=0; iSign < 2; iSign++ )
				{
					Vector vBase = vOrigin;
					vBase[iDim] += (iSign*2-1) * flCurOffset;

					if ( UTIL_IsSpaceEmpty( pMainEnt, vBase + vTestMins, vBase + vTestMaxs ) )
					{
						// Ensure that there is a clear line of sight from the spawnpoint entity to the actual spawn point.
						// (Useful for keeping things from spawning behind walls near a spawn point)
						trace_t tr;
						UTIL_TraceLine( vOrigin, vBase, MASK_SOLID, pMainEnt, COLLISION_GROUP_NONE, &tr );

						if ( tr.fraction != 1.0 )
						{
							continue;
						}

						outPos = MaybeDropToGround( pMainEnt, bDropToGround, vBase, vTestMins, vTestMaxs );
						return true;
					}
				}
			}

			++offset;
		} while ( iCurIteration++ < nMaxIterations );

		//	Warning( "EntityPlacementTest for ent %d:%s failed!\n", pMainEnt->entindex(), pMainEnt->GetClassname() );
		return false;
	}

#else // GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( State_Get() == GR_STATE_STARTGAME )
	{
		m_iBirthdayMode = BIRTHDAY_RECALCULATE;
	}
}

void CTFGameRules::HandleOvertimeBegin()
{
	C_TFPlayer *pTFPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pTFPlayer )
	{
		pTFPlayer->EmitSound( "Game.Overtime" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::ShouldShowTeamGoal( void )
{
	if ( State_Get() == GR_STATE_PREROUND || State_Get() == GR_STATE_RND_RUNNING || InSetup() )
		return true;

	return false;
}

#endif

#ifdef GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::ShutdownCustomResponseRulesDicts()
{
	DestroyCustomResponseSystems();

	if ( m_ResponseRules.Count() != 0 )
	{
		int nRuleCount = m_ResponseRules.Count();
		for ( int iRule = 0; iRule < nRuleCount; ++iRule )
		{
			m_ResponseRules[iRule].m_ResponseSystems.Purge();
		}
		m_ResponseRules.Purge();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::InitCustomResponseRulesDicts()
{
	MEM_ALLOC_CREDIT();

	// Clear if necessary.
	ShutdownCustomResponseRulesDicts();

	// Initialize the response rules for TF.
	m_ResponseRules.AddMultipleToTail( TF_CLASS_COUNT_ALL );

	char szName[512];
	for ( int iClass = TF_FIRST_NORMAL_CLASS; iClass < TF_CLASS_COUNT_ALL; ++iClass )
	{
		m_ResponseRules[iClass].m_ResponseSystems.AddMultipleToTail( MP_TF_CONCEPT_COUNT );

		for ( int iConcept = 0; iConcept < MP_TF_CONCEPT_COUNT; ++iConcept )
		{
			AI_CriteriaSet criteriaSet;
			criteriaSet.AppendCriteria( "playerclass", g_aPlayerClassNames_NonLocalized[iClass] );
			criteriaSet.AppendCriteria( "Concept", g_pszMPConcepts[iConcept] );

			// 1 point for player class and 1 point for concept.
			float flCriteriaScore = 2.0f;

			// Name.
			V_snprintf( szName, sizeof( szName ), "%s_%s\n", g_aPlayerClassNames_NonLocalized[iClass], g_pszMPConcepts[iConcept] );
			m_ResponseRules[iClass].m_ResponseSystems[iConcept] = BuildCustomResponseSystemGivenCriteria( "scripts/talker/response_rules.txt", szName, criteriaSet, flCriteriaScore );
		}		
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SendHudNotification( IRecipientFilter &filter, HudNotification_t iType )
{
	UserMessageBegin( filter, "HudNotify" );
		WRITE_BYTE( iType );
	MessageEnd();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SendHudNotification( IRecipientFilter &filter, const char *pszText, const char *pszIcon, int iTeam /*= TEAM_UNASSIGNED*/ )
{
	UserMessageBegin( filter, "HudNotifyCustom" );
		WRITE_STRING( pszText );
		WRITE_STRING( pszIcon );
		WRITE_BYTE( iTeam );
	MessageEnd();
}

void CTFGameRules::ExtendLevel(float t)
{
	m_flMapResetTime += t;
}

//-----------------------------------------------------------------------------
// Purpose: Is the player past the required delays for spawning
//-----------------------------------------------------------------------------
bool CTFGameRules::HasPassedMinRespawnTime( CBasePlayer *pPlayer )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );

	if ( pTFPlayer && pTFPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_UNDEFINED )
		return true;

	float flMinSpawnTime = GetMinTimeWhenPlayerMaySpawn( pPlayer ); 

	return ( gpGlobals->curtime > flMinSpawnTime );
}

// PFTODO - Need to figure out what the other conditionals are calling
void CTFGameRules::PlaySpecialCapSounds( int iCappingTeam, CTeamControlPoint *pPoint )
{
	//cVar1 = (**(code **)(*(int *)this + 0x29c))(this);
	//if( cVar1 != '\0' ) 
	{
		//iVar2 = (**(code **)(*(int *)this + 0x214))(this);
		//if( iVar2 == 2 )

		if( IsInKothMode() )
		{
			int iVar2 = TF_TEAM_RED;
			while( true ) 
			{
				if( GetNumberOfTeams() <= iVar2 )
					break;

				//CTeamplayRoundBasedRules::BroadcastSound
				//( (CTeamplayRoundBasedRules *)this, iVar2, (char *)(unaff_EBX + 0x782c47) );
				if( iVar2 == iCappingTeam ) 
				{
					BroadcastSound( iVar2, "Announcer.Success" );
				}
				else 
				{
					BroadcastSound( iVar2, "Announcer.Failure" );
					
				}
				++iVar2;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGameRules::CountActivePlayers( void )
{
	int iActivePlayers = 0;

	if( IsInArenaMode() )
	{
		// Keep adding ready players as long as they're not HLTV or a replay
		for( int i = 1; i < MAX_PLAYERS; i++ )
		{
			CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
			if( pTFPlayer && pTFPlayer->IsReadyToPlay() && !pTFPlayer->IsHLTV() && !pTFPlayer->IsReplay() )
			{
				iActivePlayers++;
			}
		}


		if( m_hArenaQueue.Count() > 1 )
			return iActivePlayers;

		if( iActivePlayers <= 1 || (GetGlobalTFTeam( TF_TEAM_BLUE ) && GetGlobalTFTeam( TF_TEAM_BLUE )->GetNumPlayers() <= 0) || (!GetGlobalTFTeam( TF_TEAM_RED ) && GetGlobalTFTeam( TF_TEAM_RED )->GetNumPlayers() <= 0) )
			return 0;
	}

	return BaseClass::CountActivePlayers();
}
#endif


#ifdef CLIENT_DLL
const char *CTFGameRules::GetVideoFileForMap( bool bWithExtension /*= true*/ )
{
	char mapname[MAX_MAP_NAME];

	Q_FileBase( engine->GetLevelName(), mapname, sizeof( mapname ) );
	Q_strlower( mapname );

#ifdef _X360
	// need to remove the .360 extension on the end of the map name
	char *pExt = Q_stristr( mapname, ".360" );
	if ( pExt )
	{
		*pExt = '\0';
	}
#endif

	static char strFullpath[MAX_PATH];
	Q_strncpy( strFullpath, "media/", MAX_PATH );	// Assume we must play out of the media directory
	Q_strncat( strFullpath, mapname, MAX_PATH );

	if ( bWithExtension )
	{
		Q_strncat( strFullpath, ".bik", MAX_PATH );		// Assume we're a .bik extension type
	}

	return strFullpath;
}

// I don't even like live's colours so we're using different ones
void CTFGameRules::GetTeamGlowColor( int nTeam, float &r, float &g, float &b )
{
	switch (nTeam)
	{
	case TF_TEAM_BLUE:
		r = 0.59f;
		g = 0.74f;
		b = 0.83f;
		break;
	case TF_TEAM_RED:
		r = 0.84f;
		g = 0.42f;
		b = 0.42f;
		break;
	default:
		r = 0.76f; 
		g = 0.76f; 
		b = 0.76f;
		break;
	}
}
#endif

#ifdef PF2
ConVar pf_round_end_friendlyfire( "pf_round_end_friendlyfire", "0", FCVAR_REPLICATED | FCVAR_NOTIFY );
int CTFGameRules::FriendlyFireMode( void )
{
	if( pf_round_end_friendlyfire.GetBool() && RoundHasBeenWon() )
		return FFMODE_FORCED;

	if( friendlyfire.GetInt() > 0 )
	{
		switch( friendlyfire.GetInt() )
		{
		case 1:
			return FFMODE_ON;
		case 2:
			return FFMODE_EVENTS;
		case 3:
			return FFMODE_FORCED;
		}
	}

	return FFMODE_OFF;
}
#endif