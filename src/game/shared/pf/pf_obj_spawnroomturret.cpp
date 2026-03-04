//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Defense Turret
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"

#include "pf_obj_spawnroomturret.h"

#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "vgui_bitmapbutton.h"
#include "vgui/ILocalize.h"
#include "tf_fx_muzzleflash.h"
#include "eventlist.h"
#include "hintsystem.h"
#include <vgui_controls/ProgressBar.h>
#include "igameevents.h"
#else // GAME_DLL
#include "tf_obj_sentrygun.h"
#include "engine/IEngineSound.h"
#include "tf_player.h"
#include "tf_team.h"
#include "world.h"
#include "tf_projectile_rocket.h"
#include "te_effect_dispatch.h"
#include "tf_gamerules.h"
#include "ammodef.h"
#include "tf_weaponbase_grenadeproj.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern bool IsInCommentaryMode();

enum TurretAttachments
{
	TURRET_ATTACHMENT_MUZZLE = 0,
	TURRET_ATTACHMENT_MUZZLE_ALT
};

#define TURRET_MODEL			"models/buildables/spawnroom_turret_ceiling.mdl"
#define TURRET_GROUND_MODEL		"models/buildables/spawnroom_turret_floor.mdl"

#define TURRET_MAX_HEALTH		1000

#define TURRET_THINK_DELAY		0.05f

#define TURRET_CONTEXT			"SpawnroomTurretContext"

// CEILING TURRET MINS AND MAXS
#define TURRET_MINS_CEILING_RETRACTED				Vector( -25, -25, -10 )
#define	TURRET_MAXS_CEILING_RETRACTED				Vector( 25, 25, 10 )

#define TURRET_MINS_CEILING_DEPLOYED				Vector( -25, -25, -55 )		
#define TURRET_MAXS_CEILING_DEPLOYED				Vector( 25, 25, 10 )

// GROUND TURRET MINS AND MAXS
#define TURRET_MINS_GROUND_RETRACTED				Vector( -25, -25, 0 )
#define TURRET_MAXS_GROUND_RETRACTED				Vector( 25, 25, 5 )

#define TURRET_MINS_GROUND_DEPLOYED					Vector( -25, -25, 0 )
#define TURRET_MAXS_GROUND_DEPLOYED					Vector( 25, 25, 40 )

// Turn into cvar later
#define TURRET_DISABLED_TIME 10.0f


#ifdef GAME_DLL
ConVar pf_spawnroom_turret_damage( "pf_spawnroom_turret_damage", "15", FCVAR_CHEAT, "Adjusts the Spawnroom Turret's damage. " );
ConVar pf_spawnroom_turret_notarget( "pf_spawnroom_turret_notarget", "0", FCVAR_CHEAT, "If on, the turret will not target anybody." );
ConVar pf_spawnroom_turret_retract_time("pf_spawnroom_turret_retract_time", "4", FCVAR_CHEAT, "The time in seconds of how long it will take the turret to retract back.");
ConVar pf_spawnroom_turret_deploy_time( "pf_spawnroom_turret_deploy_time", "1", FCVAR_CHEAT, "The time in seconds it takes for the turret to deploy." );
#endif

IMPLEMENT_NETWORKCLASS_ALIASED( ObjectSpawnroomTurret, DT_ObjectSpawnroomTurret )

BEGIN_NETWORK_TABLE_NOBASE( CObjectSpawnroomTurret, DT_SpawnroomTurretLocalData )
// Client specific.
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO( m_iKills ) ),
	RecvPropInt( RECVINFO( m_iAssists ) ),
// Server specific.
#else
    SendPropInt( SENDINFO( m_iKills ), 12, SPROP_CHANGES_OFTEN ),
	SendPropInt(SENDINFO( m_iAssists ), 12, SPROP_CHANGES_OFTEN),
#endif
END_NETWORK_TABLE()

BEGIN_NETWORK_TABLE( CObjectSpawnroomTurret, DT_ObjectSpawnroomTurret )
// Client specific.
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO( m_iAmmoShells ) ),
	RecvPropInt( RECVINFO( m_iState ) ),
	RecvPropBool( RECVINFO( m_bGroundTurret ) ),
// Server specific.
#else
	SendPropInt( SENDINFO( m_iAmmoShells ), 9, SPROP_CHANGES_OFTEN ),
	SendPropInt( SENDINFO( m_iState ), Q_log2( TURRET_NUM_STATES ) + 1, SPROP_UNSIGNED ),
	SendPropBool( SENDINFO( m_bGroundTurret ) ),
#endif
END_NETWORK_TABLE()

// Server specific.
#ifdef GAME_DLL
BEGIN_DATADESC( CObjectSpawnroomTurret )
    DEFINE_KEYFIELD( m_bGroundTurret, FIELD_BOOLEAN, "GroundTurret" ),
	DEFINE_KEYFIELD( m_iszRespawnRoomName, FIELD_STRING, "respawnroomname" ),
	DEFINE_KEYFIELD( m_bHasHealth, FIELD_BOOLEAN, "HasHealth" ),
	DEFINE_KEYFIELD( m_iMaxHealth, FIELD_INTEGER, "Health" ),
	DEFINE_INPUTFUNC( FIELD_VOID, "RoundActivate", InputRoundActivate ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( obj_srt, CObjectSpawnroomTurret );
PRECACHE_REGISTER( obj_srt );
#endif

CObjectSpawnroomTurret::CObjectSpawnroomTurret()
{
#ifdef CLIENT_DLL
    m_pDamageEffects = NULL;
	m_iOldUpgradeLevel = 1;
	m_iMaxAmmoShells = 1000;
#else
	SetMaxHealth( TURRET_MAX_HEALTH );
	m_iHealth = TURRET_MAX_HEALTH;
	m_iState = TURRET_STATE_INACTIVE;

	m_bDeploy = false;
	SetType( OBJ_SRT );
	m_flNextAttack = 1e16; // sometime in the not too distant future...

	m_hEnemy = NULL;
	m_hRespawnRoom = NULL;
	m_flTimeToHeal = 1e16;
#endif
}

CObjectSpawnroomTurret::~CObjectSpawnroomTurret()
{
}

#ifdef CLIENT_DLL
//=============================================================
//  Client functions
//=============================================================

void CObjectSpawnroomTurret::GetStatusText( void )
{
	// line 182 in tf_hud_target_id.cpp to get target id back
	// do nothing
}

// PFTODO: have special targetid strings for the spawnroom turret
void CObjectSpawnroomTurret::GetTargetIDString( wchar_t* sIDString, int iMaxLenInBytes )
{
	// FINAL RESULT: TEAM Defense Turret.
	// Doesn't show health value.
	// a stripped version of C_BaseObject's function 

	sIDString[0] = '\0';

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pLocalPlayer )
		return;

	if( InSameTeam( pLocalPlayer ) || pLocalPlayer->IsPlayerClass( TF_CLASS_SPY ) || pLocalPlayer->GetTeamNumber() == TEAM_SPECTATOR )
	{
		const char *pszStatusName = GetStatusName();
		wchar_t *wszObjectName = g_pVGuiLocalize->Find( pszStatusName );

		if( !wszObjectName )
		{
			wszObjectName = L"";
		}
		// new stuff here
		const char *pszTeamName = GetTeam() ? g_aTeamNames[GetTeamNumber()] : nullptr;

		wchar_t wszTeamName[sizeof(pszTeamName)];

		if( pszTeamName )
		{
			g_pVGuiLocalize->ConvertANSIToUnicode( pszTeamName, wszTeamName, sizeof( wszTeamName ) );
		}
		else
		{
			wszTeamName[0] = '\0';
		}

		const char* pszFormatString = "TF_playerid_srt";

		g_pVGuiLocalize->ConstructString( sIDString, iMaxLenInBytes, g_pVGuiLocalize->Find( pszFormatString ),
			2,
			wszTeamName
			);

	}
}

void CObjectSpawnroomTurret::GetTargetIDDataString( wchar_t* sDataString, int iMaxLenInBytes )
{
	// do nothing as the turret does not have levels
}
#else
//=============================================================
//  Server functions
//=============================================================

void CObjectSpawnroomTurret::Spawn()
{
	m_iPitchPoseParameter = -1;
	m_iYawPoseParameter = -1;

	SetModel( m_bGroundTurret ? TURRET_GROUND_MODEL : TURRET_MODEL );
	
	m_takedamage = m_bHasHealth ? DAMAGE_YES : DAMAGE_EVENTS_ONLY;

	m_iUpgradeLevel = 1;
	m_iUpgradeMetal = 0;

	SetMaxHealth( m_bHasHealth ? m_iMaxHealth : TURRET_MAX_HEALTH );
	SetHealth( m_bHasHealth ? m_iMaxHealth : TURRET_MAX_HEALTH );

	// Rotate Details
	m_iRightBound = 45;
	m_iLeftBound = 315;
	m_iBaseTurnRate = 8;
	m_flFieldOfView = VIEW_FIELD_FULL;

	m_nSkin = ( GetTeam() ) ? ( GetTeamNumber() == TF_TEAM_RED ? 0 : 1 ) : ( 2 );

	// Give the Gun some ammo
	m_iMaxAmmoShells = 1000;
	m_iAmmoShells = m_iMaxAmmoShells;

	m_iAmmoType = GetAmmoDef()->Index( "TF_AMMO_PRIMARY" );

	// Start searching for enemies
	m_hEnemy = NULL;

	m_flLastAttackedTime = 0;

	m_bTracerTick = false;
    m_bDeploy = false;
	m_iState = TURRET_STATE_DORMANT;

	CBaseObject::Spawn();
    SetActivity( ACT_OBJ_IDLE ); // Immediately set this to idle

	SetViewOffset( m_bGroundTurret ? TURRET_GROUND_EYE_OFFSET : TURRET_EYE_OFFSET );

	VPhysicsInitNormal( SOLID_BBOX, GetSolidFlags() | FSOLID_NOT_STANDABLE, false );
	SetMoveType( MOVETYPE_NONE );
	SetCollisionGroup( TFCOLLISION_GROUP_COMBATOBJECT );

	if( m_bGroundTurret )
		UTIL_SetSize( this, TURRET_MINS_GROUND_RETRACTED, TURRET_MAXS_GROUND_RETRACTED );
	else
		UTIL_SetSize( this, TURRET_MINS_CEILING_RETRACTED, TURRET_MAXS_CEILING_RETRACTED );

	m_iAttachments[ TURRET_ATTACHMENT_MUZZLE ] = LookupAttachment( m_bGroundTurret ? "muzzle" : "muzzle_l" );
	m_iAttachments[ TURRET_ATTACHMENT_MUZZLE_ALT ] = m_bGroundTurret ? 0 : LookupAttachment( "muzzle_r" );
	
	// Orient it
	QAngle angles = GetAbsAngles();

	m_vecCurAngles.y = UTIL_AngleMod( angles.y );
	m_iRightBound = UTIL_AngleMod( (int)angles.y - 50 );
	m_iLeftBound = UTIL_AngleMod( (int)angles.y + 50 );
	if ( m_iRightBound > m_iLeftBound )
	{
		m_iRightBound = m_iLeftBound;
		m_iLeftBound = UTIL_AngleMod( (int)angles.y - 50);
	}

	SetContextThink( &CObjectSpawnroomTurret::SentryThink, gpGlobals->curtime + TURRET_THINK_DELAY, TURRET_CONTEXT );
}

void CObjectSpawnroomTurret::Precache()
{
	// Models
	PrecacheModel( TURRET_MODEL );
	PrecacheModel( TURRET_GROUND_MODEL );

	PrecacheModel( "models/effects/sentry1_muzzle/sentry1_muzzle.mdl" );

	// Sounds
	PrecacheScriptSound( GetIdleSound() );
	PrecacheScriptSound( "Building_SpawnroomTurret.Deploy" );
	PrecacheScriptSound( "Building_SpawnroomTurret.Retract" );
	PrecacheScriptSound( "Building_SpawnroomTurret.Fire" );
	PrecacheScriptSound( "Building_SpawnroomTurret.Alert" );


	//PrecacheParticleSystem( "sentrydamage_1" );
}

void CObjectSpawnroomTurret::PlayStartupAnimation()
{
	// Do nothing
}

void CObjectSpawnroomTurret::OnGoActive()
{
	// Do nothing
}

int CObjectSpawnroomTurret::OnTakeDamage( const CTakeDamageInfo& info )
{
	// If on the same team it cannot be damaged
	if(	InSameTeam( info.GetAttacker() ) )
		return 0;

	// Do damage if limited health is on, DON'T KILL THE TURRET
	if ( m_bHasHealth )
	{
		// Turret cannot be attacked when it isn't deployed.
		if( !m_bDeploy )
			return 0;

		// This allows the turret to regenerate health while it's disabled.
		if( IsDisabled() )
			return 0;

		if( m_iHealth - info.GetDamage() <= 0 )
		{
			m_iHealth = 1;
			if( !IsDisabled() )
			{
				Disable( TURRET_DISABLED_TIME );
				SetContextThink( &CObjectSpawnroomTurret::DisabledThink, gpGlobals->curtime + TURRET_THINK_DELAY, TURRET_CONTEXT );
			}
			return 0;
		}	

		return BaseClass::OnTakeDamage( info );
	}
	return 0;
}

void CObjectSpawnroomTurret::Disable( float flTime )
{
	// Call CBaseObject's Disable so that we can put our own turret disabled sounds in.
	CBaseObject::Disable( flTime );
	// PFTODO: turret disabled sound
	EmitSound( "Building_Sentry.Damaged" );
}

void CObjectSpawnroomTurret::SentryThink()
{
	switch( m_iState )
	{
		case TURRET_STATE_INACTIVE:
			break;

		case TURRET_STATE_SEARCHING:
			if( RetractCheck() )
				Retract();
		case TURRET_STATE_DORMANT:
			SentryRotate();
			break;

		case TURRET_STATE_ATTACKING:
			Attack();
			break;

		default:
			Assert( 0 );
			break;

	}
	SetContextThink( &CObjectSpawnroomTurret::SentryThink, gpGlobals->curtime + TURRET_THINK_DELAY, TURRET_CONTEXT );
}

void CObjectSpawnroomTurret::FoundTarget( CBaseEntity *pTarget, const Vector &vecSoundCenter )
{
	if ( IsTurretDormant() )
	{
		Deploy();
	}
	
	if ( m_flNextAttack < gpGlobals->curtime )
	{ 
		m_hEnemy = pTarget;
		if ( m_iAmmoShells > 0 )
		{
			// Play one sound to everyone but the target.
			CPASFilter filter( vecSoundCenter );

			if ( pTarget->IsPlayer() )
			{
				CTFPlayer *pPlayer = ToTFPlayer( pTarget );

				// Play a specific sound just to the target and remove it from the genral recipient list.
				CSingleUserRecipientFilter singleFilter( pPlayer );
				EmitSound( singleFilter, entindex(), "Building_SpawnroomTurret.Alert" );
				filter.RemoveRecipient( pPlayer );
			}

			EmitSound( filter, entindex(), "Building_SpawnroomTurret.Alert" );
		}

		// Update timers, we are attacking now!
		m_iState.Set( TURRET_STATE_ATTACKING );
		m_flNextAttack = gpGlobals->curtime + TURRET_THINK_DELAY;

	}
}

Vector CObjectSpawnroomTurret::EyePosition( void )
{
	Vector offset = m_bGroundTurret ? TURRET_GROUND_EYE_OFFSET : TURRET_EYE_OFFSET;

	return BaseClass::EyePosition() + offset;
}

bool CObjectSpawnroomTurret::FindTarget() 
{
	// Don't target when this is on.
	if ( pf_spawnroom_turret_notarget.GetBool() )
		return false;

	if ( IsInCommentaryMode() )
		return false;

	// TODO - override
	if ( IsDisabled() )
		return false;

	// Check if the opposing team won, if so then don't engage. Any Team Turrets still work.
	if ( GetTeam() && TFGameRules()->RoundHasBeenWon() && TFGameRules()->GetWinningTeam() != GetTeamNumber() )
		return false;

	Vector vecSentryOrigin = EyePosition();

	CUtlVector<CTFTeam *> pTeamList;
	GetOpposingTFTeamList( &pTeamList );

	// If we have an enemy get his minimum distance to check against.
	Vector vecSegment;
	Vector vecTargetCenter;
	float flMinDist2 = 1100.0f * 1100.0f;
	CBaseEntity *pTargetCurrent = NULL;
	CBaseEntity *pTargetOld = m_hEnemy.Get();
	float flOldTargetDist2 = FLT_MAX;

	for( int i = 0; i < pTeamList.Count(); ++i)
	{
		int nTeamCount = pTeamList[i]->GetNumPlayers();
		for ( int iPlayer = 0; iPlayer < nTeamCount; ++iPlayer )
		{
			CTFPlayer *pTargetPlayer = static_cast<CTFPlayer*>( pTeamList[i]->GetPlayer( iPlayer ) );
			if ( pTargetPlayer == NULL )
				continue;

			if (pTargetPlayer == NULL)
				continue;

			// Make sure the player is alive.
			if (!pTargetPlayer->IsAlive())
				continue;

			if (pTargetPlayer->GetFlags() & FL_NOTARGET)
				continue;

			if (pTargetPlayer->m_lifeState == LIFE_DYING || pTargetPlayer->m_lifeState == LIFE_DEAD)
				continue;

			vecTargetCenter = pTargetPlayer->GetAbsOrigin();

			// check that we even have a respawn room associated first, when neutral we probably wont have one.
			if (GetRespawnRoom() != NULL)
			{
				// is the respawn room active?
				if (GetRespawnRoom()->GetActive())
				{
					// check if the player is in our respawn room
					// we don't want to attack any player outside the associated spawn if we have one
					if (!GetRespawnRoom()->PointIsWithin(vecTargetCenter) &&
						!GetRespawnRoom()->IsTouching(pTargetPlayer))
					{
						continue;
					}
				}
			}

			vecTargetCenter += pTargetPlayer->GetViewOffset();
			VectorSubtract(vecTargetCenter, vecSentryOrigin, vecSegment);
			float flDist2 = vecSegment.LengthSqr();

			// Store the current target distance if we come across it
			if (pTargetPlayer == pTargetOld)
			{
				flOldTargetDist2 = flDist2;
			}

			// Check to see if the target is closer than the already validated target.
			if (flDist2 > flMinDist2)
				continue;

			// It is closer, check to see if the target is valid.
			if (ValidTargetPlayer(pTargetPlayer, vecSentryOrigin, vecTargetCenter))
			{
				flMinDist2 = flDist2;
				pTargetCurrent = pTargetPlayer;
			}
		}

		// If we already have a target, don't check objects.
		if ( pTargetCurrent == NULL )
		{
			int nTeamObjectCount = pTeamList[i]->GetNumObjects();
			for ( int iObject = 0; iObject < nTeamObjectCount; ++iObject )
			{
				CBaseObject *pTargetObject = pTeamList[i]->GetObject( iObject );
				if ( !pTargetObject )
					continue;

				// UNCOMMENT IF WE GET CRASHES
				//if ( pTargetObject->m_lifeState == LIFE_DYING || pTargetObject->m_lifeState == LIFE_DEAD )
				//	continue;

				vecTargetCenter = pTargetObject->GetAbsOrigin();

				// check that we even have a respawn room associated first, when neutral we probably wont have one.
				if( GetRespawnRoom() != NULL )
				{
					// is the respawn room active?
					if ( GetRespawnRoom()->GetActive() )
					{
						// check if the player is in our respawn room
						// we don't want to attack any player outside the associated spawn if we have one
						if ( !GetRespawnRoom()->PointIsWithin( vecTargetCenter ) &&
							!GetRespawnRoom()->IsTouching(pTargetObject) )
						{
							continue;
						}
					}
				}
				vecTargetCenter += pTargetObject->GetViewOffset();
				VectorSubtract( vecTargetCenter, vecSentryOrigin, vecSegment );
				float flDist2 = vecSegment.LengthSqr();

				// Store the current target distance if we come across it
				if ( pTargetObject == pTargetOld )
				{
					flOldTargetDist2 = flDist2;
				}

				// Check to see if the target is closer than the already validated target.
				if ( flDist2 > flMinDist2 )
					continue;

				// It is closer, check to see if the target is valid.
				if ( ValidTargetObject( pTargetObject, vecSentryOrigin, vecTargetCenter ) )
				{
					flMinDist2 = flDist2;
					pTargetCurrent = pTargetObject;
				}
			}
		}
	}

	// We have a target.
	if ( pTargetCurrent )
	{
		if ( pTargetCurrent != pTargetOld )
		{
			// flMinDist2 is the new target's distance
			// flOldTargetDist2 is the old target's distance
			// Don't switch unless the new target is closer by some percentage
			if ( flMinDist2 < ( flOldTargetDist2 * 0.75f ) )
			{
				FoundTarget( pTargetCurrent, vecSentryOrigin );
			}
		}
		return true;
	}

	return false;
}

void CObjectSpawnroomTurret::Deploy()
{
	m_bDeploy = true;
	if ( m_bGroundTurret )
		UTIL_SetSize( this, TURRET_MINS_GROUND_DEPLOYED, TURRET_MAXS_GROUND_DEPLOYED );
	else
		UTIL_SetSize( this, TURRET_MINS_CEILING_DEPLOYED, TURRET_MAXS_CEILING_DEPLOYED );

	EmitSound( "Building_SpawnroomTurret.Deploy" );
	m_flNextAttack = gpGlobals->curtime + pf_spawnroom_turret_deploy_time.GetFloat();
	m_flSearchingGracePeriod = gpGlobals->curtime + pf_spawnroom_turret_retract_time.GetFloat();
	m_iState.Set( TURRET_STATE_SEARCHING );
}

void CObjectSpawnroomTurret::Retract()
{
	m_bDeploy = false;

	if ( m_bGroundTurret )
		UTIL_SetSize( this, TURRET_MINS_GROUND_RETRACTED, TURRET_MAXS_GROUND_RETRACTED );
	else
		UTIL_SetSize( this, TURRET_MINS_CEILING_RETRACTED, TURRET_MAXS_CEILING_RETRACTED );

	EmitSound( "Building_SpawnroomTurret.Retract" );
	m_flNextAttack = gpGlobals->curtime + 1e16;
	m_iState.Set( TURRET_STATE_DORMANT );
}

void CObjectSpawnroomTurret::DisabledThink()
{
	// this should be moved to SentryThink 
	
	if( !IsDisabled() )
	{ 
		// Go back to thinking normally.
		SetContextThink( &CObjectSpawnroomTurret::SentryThink, gpGlobals->curtime + TURRET_THINK_DELAY, TURRET_CONTEXT );
		return;
	}

	// PFTODO: gradual increase over time
	// It doesn't heal to max health sometimes.
	if( m_flTimeToHeal < gpGlobals->curtime )
	{
		if( m_iHealth < m_iMaxHealth )
		{
			int iHealthToAdd = min( ( m_iMaxHealth / TURRET_DISABLED_TIME - 1 ), m_iMaxHealth - m_iHealth );
			SetHealth( m_iHealth + iHealthToAdd );
		}

		//DevMsg( "The health of the turret is: %d\n", GetHealth() );
	}

	m_flTimeToHeal = gpGlobals->curtime + 1.0f;

	SetNextThink( gpGlobals->curtime + 1.0f, TURRET_CONTEXT );
}

//-----------------------------------------------------------------------------
// Make sure our target is still valid, and if so, fire at it
//-----------------------------------------------------------------------------
void CObjectSpawnroomTurret::Attack()
{
	StudioFrameAdvance( );

	if ( !FindTarget() || m_hEnemy.Get() == NULL )
	{
		m_flSearchingGracePeriod = gpGlobals->curtime + pf_spawnroom_turret_retract_time.GetFloat();
		m_iState.Set( TURRET_STATE_SEARCHING );
		m_hEnemy = NULL;
		return;
	}

	// Track enemy
	Vector vecMid = EyePosition();
	Vector vecMidEnemy = m_hEnemy->WorldSpaceCenter();
	Vector vecDirToEnemy = vecMidEnemy - vecMid;

	QAngle angToTarget;
	VectorAngles( vecDirToEnemy, angToTarget );

	angToTarget.y = UTIL_AngleMod( angToTarget.y );
	if (angToTarget.x < -180)
		angToTarget.x += 360;
	if (angToTarget.x > 180)
		angToTarget.x -= 360;

	// now all numbers should be in [1...360]
	// pin to turret limitations to [-40...40]
	if (angToTarget.x > 40)
		angToTarget.x = 40;
	else if (angToTarget.x < -40)
		angToTarget.x = -40;
	m_vecGoalAngles.y = angToTarget.y;
	m_vecGoalAngles.x = angToTarget.x;

	if(MoveTurret())
		EmitSound( GetIdleSound() );

	// Fire on the target if it's within 10 units of being aimed right at it
	if ( m_flNextAttack <= gpGlobals->curtime && (m_vecGoalAngles - m_vecCurAngles).Length() <= 10 )
	{
		Fire();
		m_flNextAttack = gpGlobals->curtime + 0.1;
	}
}

//-----------------------------------------------------------------------------
// Fire on our target
//-----------------------------------------------------------------------------
bool CObjectSpawnroomTurret::Fire()
{
	//NDebugOverlay::Cross3D( EyePosition(), 10, 255, 0, 0, false, 0.1 );

	Vector vecAimDir;

	// All turrets fire shells
	if ( m_iAmmoShells > 0 )
	{
		if ( !IsPlayingGesture( ACT_OBJ_ATTACK ) )
		{
			RemoveGesture( ACT_OBJ_ATTACK_DRY );
			AddGesture( ACT_OBJ_ATTACK );
		}

		Vector vecSrc;
		QAngle vecAng;

		int iAttachment;
		
		// this way kinda sucks but since ammo shells don't deplete oh well
		if ( !m_bGroundTurret )
		{ 
			m_bTracerTick = !m_bTracerTick;

			if ( m_bTracerTick )
			{
				// level 2 and 3 turrets alternate muzzles each time they fizzy fizzy fire.
				iAttachment = m_iAttachments[ TURRET_ATTACHMENT_MUZZLE_ALT ];
			}
			else
			{
				iAttachment = m_iAttachments[ TURRET_ATTACHMENT_MUZZLE ];
			}
		}
		else
		{
			iAttachment = m_iAttachments[ TURRET_ATTACHMENT_MUZZLE ];
		}

		GetAttachment( iAttachment, vecSrc, vecAng );

		Vector vecMidEnemy = m_hEnemy->WorldSpaceCenter();

		// If we cannot see their WorldSpaceCenter ( possible, as we do our target finding based
		// on the eye position of the target ) then fire at the eye position
		trace_t tr;
		UTIL_TraceLine( vecSrc, vecMidEnemy, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );

		if ( !tr.m_pEnt || tr.m_pEnt->IsWorld() )
		{
			// Hack it lower a little bit..
			// The eye position is not always within the hitboxes for a standing TF Player
			vecMidEnemy = m_hEnemy->EyePosition() + Vector( 0, 0, -5 );
		}

		vecAimDir = vecMidEnemy - vecSrc;

		float flDistToTarget = vecAimDir.Length();

		vecAimDir.NormalizeInPlace();

		//NDebugOverlay::Cross3D( vecSrc, 10, 255, 0, 0, false, 0.1 );

		FireBulletsInfo_t info;

		info.m_vecSrc = vecSrc;
		info.m_vecDirShooting = vecAimDir;
		info.m_iTracerFreq = 1;
		info.m_iShots = 1;
		info.m_pAttacker = this;
		info.m_vecSpread = vec3_origin/*vec3_spread*/;
		info.m_flDistance = flDistToTarget + 100;
		info.m_iAmmoType = m_iAmmoType;
		info.m_flDamage = pf_spawnroom_turret_damage.GetFloat();

		FireBullets( info );

		//NDebugOverlay::Line( vecSrc, vecSrc + vecAimDir * 1000, 255, 0, 0, false, 0.1 );

		CEffectData data;
		data.m_nEntIndex = entindex();
		data.m_nAttachmentIndex = iAttachment;
		data.m_fFlags = m_iUpgradeLevel;
		data.m_vOrigin = vecSrc;
		DispatchEffect( "TF_3rdPersonMuzzleFlash_SentryGun", data );

		EmitSound( "Building_SpawnroomTurret.Fire" );

		// uncomment this if you want the turret to have limited ammo
		//m_iAmmoShells--;
	}
	else
	{
		if ( !IsPlayingGesture( ACT_OBJ_ATTACK_DRY ) )
		{
			RemoveGesture( ACT_OBJ_ATTACK_DRY );
			AddGesture( ACT_OBJ_ATTACK_DRY );
		}
		
		// Out of ammo, play a click
		EmitSound( "Building_Sentrygun.Empty" );
		m_flNextAttack = gpGlobals->curtime + 0.2;
	}

	return true;
}

bool CObjectSpawnroomTurret::ValidTargetPlayer( CTFPlayer *pPlayer, const Vector &vecStart, const Vector &vecEnd )
{
	// Overrided to see thru enemy spies' disguises and cloak.
	return FVisible( pPlayer, MASK_SHOT | CONTENTS_GRATE );
}

bool CObjectSpawnroomTurret::RetractCheck( void )
{
	// Retract if the round has been won and you're not on the winning team 
	// No Team Turrets aren't affected by this.
	if( GetTeam() && TFGameRules()->RoundHasBeenWon() && TFGameRules()->GetWinningTeam() != GetTeamNumber() )
		return true;

	if ( ( m_flSearchingGracePeriod < gpGlobals->curtime && IsSearching() ) )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Rotate and scan for targets
//-----------------------------------------------------------------------------
void CObjectSpawnroomTurret::SentryRotate( void )
{
	// if we're playing a fire gesture, stop it
	if ( IsPlayingGesture( ACT_OBJ_ATTACK ) )
	{
		RemoveGesture( ACT_OBJ_ATTACK );
	}

	if ( IsPlayingGesture( ACT_OBJ_ATTACK_DRY ) )
	{
		RemoveGesture( ACT_OBJ_ATTACK_DRY );
	}

	// animate
	StudioFrameAdvance();

	// Look for a target
	if ( FindTarget() )
		return;

	// Rotate
	if ( !MoveTurret() )
	{
		// Change direction

		if ( IsDisabled() )
		{
			EmitSound( "Building_Sentrygun.Disabled" );
			m_vecGoalAngles.x = 30;
		}
		else
		{
			if ( m_bDeploy && IsSearching() )
				EmitSound( GetIdleSound() );

			// Switch rotation direction
			if ( m_bTurningRight )
			{
				m_bTurningRight = false;
				m_vecGoalAngles.y = m_iLeftBound;
			}
			else
			{
				m_bTurningRight = true;
				m_vecGoalAngles.y = m_iRightBound;
			}

			// Randomly look up and down a bit
			if ( random->RandomFloat( 0, 1 ) < 0.3 )
			{
				m_vecGoalAngles.x = ( int )random->RandomFloat( -10, 10 );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Override 
//-----------------------------------------------------------------------------
void CObjectSpawnroomTurret::DetermineAnimation( void )
{
	Activity desiredActivity = m_Activity;

	switch ( m_Activity )
	{
	default:
		{
			if ( m_bDeploy )
			{
				desiredActivity = ACT_OBJ_STARTUP;
			}
			else
			{
				desiredActivity = ACT_OBJ_IDLE;
			}
		}
		break;

	case ACT_OBJ_STARTUP:
		{
			if ( IsActivityFinished() )
			{
				desiredActivity = ACT_OBJ_RUNNING;
			}
		}
		break;

	case ACT_OBJ_RETRACT:
	{
		if ( IsActivityFinished() )
		{
			desiredActivity = ACT_OBJ_IDLE;
		}
	}
	break;
	case ACT_OBJ_RUNNING:
		if ( !m_bDeploy )
		{
			desiredActivity = ACT_OBJ_RETRACT;
		}
		break;
	}

	if ( desiredActivity == m_Activity )
		return;

	SetActivity( desiredActivity );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectSpawnroomTurret::InputRoundActivate( inputdata_t &inputdata )
{
	if ( m_iszRespawnRoomName != NULL_STRING )
	{
		ConMsg("Looking for respawn room\n");
		m_hRespawnRoom = dynamic_cast<CFuncRespawnRoom*>(gEntList.FindEntityByName( NULL, m_iszRespawnRoomName ));
		if ( m_hRespawnRoom )
		{
			// set the team to that of the respawnroom's
			ChangeTeam( m_hRespawnRoom->GetTeamNumber() );
			m_nSkin = ( GetTeam()) ? ( GetTeamNumber() == TF_TEAM_RED ? 0 : 1 ) : 2;
			m_iState = TURRET_STATE_DORMANT;
		}
		else
		{		
			Warning("%s(%s) was unable to find func_respawnroom named '%s'\n", GetClassname(), GetDebugName(), STRING(m_iszRespawnRoomName) );
		}
	}
}
#endif
