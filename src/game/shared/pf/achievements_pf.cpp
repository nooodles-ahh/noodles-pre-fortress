//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================


#include "cbase.h"

#ifdef CLIENT_DLL

#include "achievementmgr.h"
#include "baseachievement.h"
#include "tf_hud_statpanel.h"
#include "c_tf_team.h"
#include "c_tf_player.h"

CAchievementMgr g_AchievementMgrTF;	// global achievement mgr for TF

bool CheckWinNoEnemyCaps( IGameEvent *event, int iRole );

// Grace period that we allow a player to start after level init and still consider them to be participating for the full round.  This is fairly generous
// because it can in some cases take a client several minutes to connect with respect to when the server considers the game underway
#define TF_FULL_ROUND_GRACE_PERIOD	( 4 * 60.0f )

bool IsLocalTFPlayerClass( int iClass );

// helper class for achievements that check that the player was playing on a game team for the full round
class CTFAchievementFullRound : public CBaseAchievement
{
	DECLARE_CLASS( CTFAchievementFullRound, CBaseAchievement );
public:
	void Init() 
	{
		m_iFlags |= ACH_FILTER_FULL_ROUND_ONLY;		
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_round_win" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( 0 == Q_strcmp( event->GetName(), "teamplay_round_win" ) )
		{
			C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
			if ( pLocalPlayer )
			{
				// is the player currently on a game team?
				int iTeam = pLocalPlayer->GetTeamNumber();
				if ( iTeam >= FIRST_GAME_TEAM ) 
				{
					float flRoundTime = event->GetFloat( "round_time", 0 );
					if ( flRoundTime > 0 )
					{
						Event_OnRoundComplete( flRoundTime, event );
					}
				}
			}
		}
	}

	virtual void Event_OnRoundComplete( float flRoundTime, IGameEvent *event ) = 0 ;

};

//==============================================================
// TF2 Achievements
//==============================================================
class CAchievementTFWinMultipleGames : public CTFAchievementFullRound
{
	DECLARE_CLASS( CAchievementTFWinMultipleGames, CTFAchievementFullRound );
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_FILTER_FULL_ROUND_ONLY );
		SetGoal( 20 );
		BaseClass::Init();
	}

	virtual void Event_OnRoundComplete( float flRoundTime, IGameEvent* event )
	{
		C_TFPlayer* pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pLocalPlayer )
		{
			// was the player on the winning team?
			int iPlayerTeam = pLocalPlayer->GetTeamNumber();
			int iWinningTeam = event->GetInt( "team" );
			if ( ( iWinningTeam >= FIRST_GAME_TEAM ) && ( iPlayerTeam == iWinningTeam ) )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFWinMultipleGames, ACHIEVEMENT_TF_WIN_MULTIPLEGAMES, "TF_WIN_MULTIPLEGAMES", 10 );

class CAchievementTFPlayGameEveryMap : public CTFAchievementFullRound
{
	DECLARE_CLASS( CAchievementTFPlayGameEveryMap, CTFAchievementFullRound );
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_HAS_COMPONENTS | ACH_FILTER_FULL_ROUND_ONLY );

		static const char* szComponents[] =
		{
			"ad_dustbowl", "ad_dustbowl2", "cp_granary", "ad_gravelpit", "cp_well", "ctf_2fort", "tc_hydro", "cp_powerhouse"
		};
		m_pszComponentNames = szComponents;
		m_iNumComponents = ARRAYSIZE( szComponents );
		SetGoal( m_iNumComponents );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_round_win" );
	}

	virtual void Event_OnRoundComplete( float flRoundTime, IGameEvent* event )
	{
		float flTeamplayStartTime = m_pAchievementMgr->GetTeamplayStartTime();
		if ( flTeamplayStartTime > 0 )
		{
			// has the player been present and on a game team since the start of this round (minus a grace period)?
			if ( flTeamplayStartTime < ( gpGlobals->curtime - flRoundTime ) + TF_FULL_ROUND_GRACE_PERIOD )
			{
				// yes, the achievement is satisfied for this map, set the corresponding bit
				OnComponentEvent( m_pAchievementMgr->GetMapName() );
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFPlayGameEveryMap, ACHIEVEMENT_TF_PLAY_GAME_EVERYMAP, "TF_PLAY_GAME_EVERYMAP", 8 );



//==============================================================
// PF2 Achievements
//==============================================================

class CAchievementPFGetTranqKill : public CBaseAchievement
{
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}
	// server fires an event for this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementPFGetTranqKill, ACHIEVEMENT_PF_GET_TRANQKILL, "PF_GET_TRANQKILL", 1 );

class CAchievementTFGetArmorRepairPoints : public CBaseAchievement
{
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 500 );
	}

	void OnPlayerStatsUpdate()
	{
		ClassStats_t& classStats = CTFStatPanel::GetClassStats( TF_CLASS_ENGINEER );
		RoundStats_t& lifeStats = GetStatPanel()->GetRoundStatsCurrentLife();
		int iOldCount = m_iCount;
		m_iCount = classStats.accumulated.m_iStat[ TFSTAT_ARMORREPAIRED ] + lifeStats.m_iStat[ TFSTAT_ARMORREPAIRED ];
		if ( m_iCount != iOldCount )
		{
			m_pAchievementMgr->SetDirty( true );
		}

		if ( IsLocalTFPlayerClass( TF_CLASS_ENGINEER ) )
		{
			IncrementCount();
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFGetArmorRepairPoints, ACHIEVEMENT_PF_GET_ARMORREPAIRPOINTS, "PF_GET_ARMORREPAIRPOINTS", 10 );

/*
class CAchievementPFDestroySapperWithGun : public CBaseAchievement
{
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 5 );
	}
	// server fires an event for this achievement, no other code within achievement necessary
};

DECLARE_ACHIEVEMENT( CAchievementPFDestroySapperWithGun, ACHIEVEMENT_PF_DESTROY_SAPPER_GUN, "PF_DESTROY_SAPPER_GUN", 5 );
*/

class CAchievementPFUseEnemyTeleporter : public CBaseAchievement
{
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}
	// server fires an event for this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementPFUseEnemyTeleporter, ACHIEVEMENT_PF_USE_ENEMY_TELEPORTER, "PF_USE_ENEMY_TELEPORTER", 10 );

class CAchievementPFEMPBlastAmmoKill : public CBaseAchievement
{
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}
	// server fires an event for this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementPFEMPBlastAmmoKill, ACHIEVEMENT_PF_EMP_BLAST_AMMOKILL, "PF_EMP_BLAST_AMMOKILL", 5 );



class CAchievementpfBonk : public CBaseAchievement
{
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}
	// server fires an event for this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementpfBonk, ACHIEVEMENT_PF_BONK_BY_NADE, "PF_BONK_BY_NADE", 1 );

class CAchievementTFGetAltHealingPoints : public CBaseAchievement
{
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 500 );
	}

	void OnPlayerStatsUpdate()
	{
		ClassStats_t& classStats = CTFStatPanel::GetClassStats( TF_CLASS_MEDIC );
		RoundStats_t& lifeStats = GetStatPanel()->GetRoundStatsCurrentLife();
		int iOldCount = m_iCount;
		m_iCount = classStats.accumulated.m_iStat[ TFSTAT_HEALING_OTHER ] + lifeStats.m_iStat[ TFSTAT_HEALING_OTHER ];
		if ( m_iCount != iOldCount )
		{
			m_pAchievementMgr->SetDirty( true );
		}

		if ( IsLocalTFPlayerClass( TF_CLASS_MEDIC ) )
		{
			IncrementCount();
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFGetAltHealingPoints, ACHIEVEMENT_PF_GET_ALT_HEALPOINTS, "PF_GET_ALT_HEALPOINTS", 10 );

class CAchievementPFNapalmBurn : public CBaseAchievement
{
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}
	// server fires an event for this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementPFNapalmBurn, ACHIEVEMENT_PF_NAPALM_BURN, "PF_NAPALM_BURN", 5 );

class CAchievementPFHealNadeMultiPlayers : public CBaseAchievement
{
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}
	// server fires an event for this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementPFHealNadeMultiPlayers, ACHIEVEMENT_PF_NADE_MULTI_HEAL_PLAYER, "PF_NADE_MULTI_HEAL_PLAYER", 1)

class CAchievementPFFatFingers : public CBaseAchievement
{
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}
	// server fires an event for this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementPFFatFingers, ACHIEVEMENT_PF_FATFINGERS, "PF_FATFINGERS", 1 );

class CAchievementPFPipeBounce : public CBaseAchievement
{
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}
	// server fires an event for this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT_( CAchievementPFPipeBounce, ACHIEVEMENT_PF_KILL_BY_PIPEBOUNCE, "PF_KILL_BY_PIPEBOUNCE", NULL, 1, true );


class CAchievementPFOutWithBang : public CBaseAchievement
{
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}
	// server fires an event for this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementPFOutWithBang, ACHIEVEMENT_PF_NADE_KILL_SELF_AND_OTHER, "PF_NADE_KILL_SELF_AND_OTHER", 1 );

class CAchievementPFNadeTrophyKill : public CBaseAchievement
{
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 5 );
	}
	// server fires an event for this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementPFNadeTrophyKill, ACHIEVEMENT_PF_NADE_TROPHY_KILL, "PF_NADE_TROPHY_KILL", 5 );

//-----------------------------------------------------------------------------
// Purpose: see if a round win was a win for the local player with no enemy caps
//-----------------------------------------------------------------------------
bool CheckWinNoEnemyCaps( IGameEvent *event, int iRole )
{
	if ( 0 == Q_strcmp( event->GetName(), "teamplay_round_win" ) )
	{
		if ( event->GetInt( "team" ) == GetLocalPlayerTeam() )
		{
			int iLosingTeamCaps = event->GetInt( "losing_team_num_caps" );
			if ( 0 == iLosingTeamCaps )
			{
				C_TFTeam *pLocalTeam = GetGlobalTFTeam( GetLocalPlayerTeam() );
				if ( pLocalTeam )
				{
					int iRolePlayer = pLocalTeam->GetRole();
					if ( iRole > TEAM_ROLE_NONE && ( iRolePlayer != iRole ) )
						return false;
					return true;
				}
			}
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Helper function to determine if local player is specified class
//-----------------------------------------------------------------------------
bool IsLocalTFPlayerClass( int iClass )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	return( pLocalPlayer && pLocalPlayer->IsPlayerClass( iClass ) );
}


#endif // CLIENT_DLL