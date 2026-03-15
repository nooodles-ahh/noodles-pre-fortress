//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Teleporter Object
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"

#include "tf_obj_teleporter.h"
#include "engine/IEngineSound.h"
#include "tf_player.h"
#include "tf_team.h"
#include "tf_gamerules.h"
#include "world.h"
#include "explode.h"
#include "particle_parse.h"
#include "tf_gamestats.h"
#include "tf_weapon_sniperrifle.h"
#include "tf_fx.h"
#include "tf_shareddefs.h"
#include <hl2orange.spa.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Ground placed version
#define TELEPORTER_MODEL_ENTRANCE_PLACEMENT	"models/buildables/teleporter_blueprint_enter.mdl"
#define TELEPORTER_MODEL_EXIT_PLACEMENT		"models/buildables/teleporter_blueprint_exit.mdl"
#define TELEPORTER_MODEL_BUILDING			"models/buildables/teleporter.mdl"
#define TELEPORTER_MODEL_LIGHT				"models/buildables/teleporter_light.mdl"

#define TELEPORTER_MINS			Vector( -24, -24, 0)
#define TELEPORTER_MAXS			Vector( 24, 24, 12)	

IMPLEMENT_SERVERCLASS_ST( CObjectTeleporter, DT_ObjectTeleporter )
	SendPropInt( SENDINFO( m_iState ), 5 ),
	SendPropTime( SENDINFO( m_flRechargeTime ) ),
	SendPropInt( SENDINFO( m_iTimesUsed ), 6 ),
	SendPropFloat( SENDINFO( m_flYawToExit ), 8, 0, 0.0, 360.0f ),
END_SEND_TABLE()

BEGIN_DATADESC( CObjectTeleporter )
	DEFINE_KEYFIELD( m_iTeleporterType, FIELD_INTEGER, "teleporterType" ),
	DEFINE_KEYFIELD( m_szMatchingTeleporterName, FIELD_STRING, "matchingTeleporter" ),
	DEFINE_THINKFUNC( TeleporterThink ),
	DEFINE_ENTITYFUNC( TeleporterTouch ),
END_DATADESC()

PRECACHE_REGISTER( obj_teleporter );

#define TELEPORTER_MAX_HEALTH	150

#define TELEPORTER_THINK_CONTEXT				"TeleporterContext"

#define BUILD_TELEPORTER_DAMAGE					25		// how much damage an exploding teleporter can do

#define BUILD_TELEPORTER_FADEOUT_TIME			0.25	// time to teleport a player out (teleporter with full health)
#define BUILD_TELEPORTER_FADEIN_TIME			0.25	// time to teleport a player in (teleporter with full health)

#define BUILD_TELEPORTER_NEXT_THINK				0.05

#define BUILD_TELEPORTER_PLAYER_OFFSET			20		// how far above the origin of the teleporter to place a player

#define BUILD_TELEPORTER_EFFECT_TIME			12.0	// seconds that player glows after teleporting

ConVar tf_teleporter_fov_start( "tf_teleporter_fov_start", "120", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Starting FOV for teleporter zoom.", true, 1, false, 0 );
ConVar tf_teleporter_fov_time( "tf_teleporter_fov_time", "0.5", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "How quickly to restore FOV after teleport.", true, 0.0, false, 0 );

extern ConVar pf_upgradable_buildings;

LINK_ENTITY_TO_CLASS( obj_teleporter, CObjectTeleporter );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CObjectTeleporter::CObjectTeleporter()
{
	SetMaxHealth( TELEPORTER_MAX_HEALTH );
	m_iHealth = TELEPORTER_MAX_HEALTH;
	UseClientSideAnimation();
	SetType( OBJ_TELEPORTER );
	m_iTeleporterType = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectTeleporter::Spawn()
{
	// Only used by teleporters placed in hammer
	if ( m_iTeleporterType == 1 )
		SetObjectMode( TELEPORTER_TYPE_ENTRANCE );
	else if ( m_iTeleporterType == 2 )
		SetObjectMode( TELEPORTER_TYPE_EXIT );

	SetSolid( SOLID_BBOX );

	m_takedamage = DAMAGE_NO;

	SetState( TELEPORTER_STATE_BUILDING );

	m_flNextEnemyTouchHint = gpGlobals->curtime;

	m_flYawToExit = 0;

	if ( GetObjectMode() == TELEPORTER_TYPE_ENTRANCE )
		SetModel(TELEPORTER_MODEL_ENTRANCE_PLACEMENT);
	else
		SetModel(TELEPORTER_MODEL_EXIT_PLACEMENT);

	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CObjectTeleporter::MakeCarriedObject(CTFPlayer* pPlayer)
{
	SetState( TELEPORTER_STATE_BUILDING );

	// Stop thinking.
	SetContextThink( NULL, 0, TELEPORTER_THINK_CONTEXT );
	SetTouch( NULL );

	ShowDirectionArrow( false );

	SetPlaybackRate( 0.0f );
	m_flLastStateChangeTime = 0.0f;

	BaseClass::MakeCarriedObject( pPlayer );
}


//-----------------------------------------------------------------------------
// Receive a teleporting player 
//-----------------------------------------------------------------------------
void CObjectTeleporter::TeleporterReceive( CTFPlayer* pPlayer, float flDelay )
{
	if ( !pPlayer )
		return;

	SetTeleportingPlayer( pPlayer );

	Vector origin = GetAbsOrigin();
	CPVSFilter filter( origin );

	int iTeam = pPlayer->GetTeamNumber();
	if ( pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) )
	{
		iTeam = pPlayer->m_Shared.GetDisguiseTeam();
	}

	const char *pszEffectName = ConstructTeamParticle( "teleportedin_%s", iTeam );
	TE_TFParticleEffect( filter, 0.0, pszEffectName, origin, vec3_angle );

	EmitSound( "Building_Teleporter.Receive" );

	SetState( TELEPORTER_STATE_RECEIVING );
	m_flMyNextThink = gpGlobals->curtime + BUILD_TELEPORTER_FADEOUT_TIME;

	if ( pPlayer != GetBuilder() )
		m_iTimesUsed++;
}


//-----------------------------------------------------------------------------
// Teleport the passed player to our destination
//-----------------------------------------------------------------------------
void CObjectTeleporter::TeleporterSend( CTFPlayer *pPlayer )
{
	if ( !pPlayer )
		return;

	SetTeleportingPlayer( pPlayer );
	pPlayer->m_Shared.AddCond( TF_COND_SELECTED_TO_TELEPORT );

	Vector origin = GetAbsOrigin();
	CPVSFilter filter( origin );

	int iTeam = pPlayer->GetTeamNumber();
	if ( pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) )
	{
		iTeam = pPlayer->m_Shared.GetDisguiseTeam();
	}

	const char *pszTeleportedEffect = ConstructTeamParticle( "teleported_%s", iTeam );
	TE_TFParticleEffect( filter, 0.0, pszTeleportedEffect, origin, vec3_angle );

	const char *pszSparklesEffect = ConstructTeamParticle( "player_sparkles_%s", iTeam );
	TE_TFParticleEffect( filter, 0.0, pszSparklesEffect, PATTACH_ABSORIGIN, pPlayer );

	EmitSound( "Building_Teleporter.Send" );

	SetState( TELEPORTER_STATE_SENDING );
	m_flMyNextThink = gpGlobals->curtime + 0.1;

	m_iTimesUsed++;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CObjectTeleporter::StartUpgrading(void)
{
	BaseClass::StartUpgrading();

	SetState( TELEPORTER_STATE_UPGRADING );
}

void CObjectTeleporter::FinishUpgrading(void)
{
	SetState( TELEPORTER_STATE_IDLE );

	BaseClass::FinishUpgrading();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectTeleporter::SetModel( const char *pModel )
{
	BaseClass::SetModel( pModel );

	// Reset this after model change
	UTIL_SetSize( this, TELEPORTER_MINS, TELEPORTER_MAXS );

	CreateBuildPoints();

	ReattachChildren();

	m_iDirectionBodygroup = FindBodygroupByName( "teleporter_direction" );
	m_iBlurBodygroup = FindBodygroupByName( "teleporter_blur" );

	if ( m_iBlurBodygroup >= 0 )
	{
		SetBodygroup( m_iBlurBodygroup, 0 );
	}
}

bool CObjectTeleporter::InputWrenchHit( CTFPlayer *pPlayer, CTFWrench *pWrench, Vector vecHitPos )
{
	if ( HasSapper() && GetMatchingTeleporter() )
	{
		CObjectTeleporter* pMatch = GetMatchingTeleporter();
		// do damage to any attached buildings
		CTakeDamageInfo info(pPlayer, pPlayer, 65, DMG_CLUB, TF_DMG_WRENCH_FIX);

		IHasBuildPoints* pBPInterface = dynamic_cast<IHasBuildPoints*>( pMatch );
		int iNumObjects = pBPInterface->GetNumObjectsOnMe();
		for ( int iPoint = 0; iPoint < iNumObjects; iPoint++ )
		{
			CBaseObject* pObject = pMatch->GetBuildPointObject( iPoint );

			if ( pObject && pObject->IsHostileUpgrade() )
				pObject->TakeDamage( info );
		}
	}

	return BaseClass::InputWrenchHit(pPlayer, pWrench, vecHitPos);
}

//-----------------------------------------------------------------------------
// Purpose: Start building the object
//-----------------------------------------------------------------------------
bool CObjectTeleporter::StartBuilding( CBaseEntity *pBuilder )
{
	SetModel( TELEPORTER_MODEL_BUILDING );

	return BaseClass::StartBuilding( pBuilder );
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
bool CObjectTeleporter::IsPlacementPosValid( void )
{
	bool bResult = BaseClass::IsPlacementPosValid();

	if ( !bResult )
	{
		return false;
	}

	// m_vecBuildOrigin is the proposed build origin

	// start above the teleporter position
	Vector vecTestPos = m_vecBuildOrigin;
	vecTestPos.z += TELEPORTER_MAXS.z;

	// make sure we can fit a player on top in this pos
	trace_t tr;
	UTIL_TraceHull( vecTestPos, vecTestPos, VEC_HULL_MIN, VEC_HULL_MAX, MASK_SOLID | CONTENTS_PLAYERCLIP, this, COLLISION_GROUP_PLAYER_MOVEMENT, &tr );

	return ( tr.fraction >= 1.0 );
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CObjectTeleporter::OnGoActive( void )
{
	SetModel( TELEPORTER_MODEL_LIGHT );
	SetActivity( ACT_OBJ_IDLE );

	SetContextThink( &CObjectTeleporter::TeleporterThink, gpGlobals->curtime + 0.1, TELEPORTER_THINK_CONTEXT );
	SetTouch( &CObjectTeleporter::TeleporterTouch );

	SetState( TELEPORTER_STATE_IDLE );

	BaseClass::OnGoActive();

	SetPlaybackRate( 0.0f );
	m_flLastStateChangeTime = 0.0f;	// used as a flag to initialize the playback rate to 0 in the first DeterminePlaybackRate
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectTeleporter::Precache()
{
	BaseClass::Precache();

	// Precache Object Models
	int iModelIndex;

	PrecacheModel( TELEPORTER_MODEL_ENTRANCE_PLACEMENT );
	PrecacheModel( TELEPORTER_MODEL_EXIT_PLACEMENT );	

	iModelIndex = PrecacheModel( TELEPORTER_MODEL_BUILDING );
	PrecacheGibsForModel( iModelIndex );

	iModelIndex = PrecacheModel( TELEPORTER_MODEL_LIGHT );
	PrecacheGibsForModel( iModelIndex );

	// Precache Sounds
	PrecacheScriptSound( "Building_Teleporter.Ready" );
	PrecacheScriptSound( "Building_Teleporter.Send" );
	PrecacheScriptSound( "Building_Teleporter.Receive" );
	PrecacheScriptSound( "Building_Teleporter.Spin1" );
	PrecacheScriptSound( "Building_Teleporter.Spin2" );
	PrecacheScriptSound( "Building_Teleporter.Spin3" );

	PrecacheTeamParticles( "teleporter_%s_charged" );
	PrecacheTeamParticles( "teleporter_%s_entrance" );
	PrecacheTeamParticles( "teleporter_%s_exit" );
	PrecacheTeamParticles( "teleporter_arms_circle_%s" );
	PrecacheTeamParticles( "teleported_%s" );
	PrecacheTeamParticles( "teleportedin_%s" );
	PrecacheTeamParticles( "player_sparkles_%s" );
	PrecacheParticleSystem( "tpdamage_1" );
	PrecacheParticleSystem( "tpdamage_2" );
	PrecacheParticleSystem( "tpdamage_3" );
	PrecacheParticleSystem( "tpdamage_4" );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CObjectTeleporter::TeleporterTouch( CBaseEntity *pOther )
{
	if ( IsDisabled() )
	{
		return;
	}

	// if it's not a player, ignore
	if ( !pOther->IsPlayer() )
		return;

	CTFPlayer *pPlayer = ToTFPlayer( pOther );

	CTFPlayer *pBuilder = GetBuilder();

	Assert( pBuilder );

	if ( !pBuilder )
	{
		return;
	}

#ifdef PF2_DLL
	// if its not a teammate of the builder, notify the builder
	//Lazy!
	if ( pBuilder->GetTeamNumber() != pPlayer->GetTeamNumber() && 
		 pPlayer->GetPlayerClass()->GetClassIndex() != TF_CLASS_SPY &&
		 IsMatchingTeleporterReady() )
	{
		SetEnemyUsing( true );
		m_flNextCheckEnemyUsing = gpGlobals->curtime + 1.0f;
	}
	else
	{
		SetEnemyUsing( false );
	}
#endif
	// is this an entrance and do we have an exit?
	if ( GetObjectMode() == TELEPORTER_TYPE_ENTRANCE )
	{		
		if ( ( m_iState == TELEPORTER_STATE_READY ) )
		{
			if ( !PlayerCanBeTeleported( pPlayer ) )
			{
				// are we able to teleport?
				if ( pPlayer->HasTheFlag() )
				{
					// If they have the flag, print a warning that you can't tele with the flag
					CSingleUserRecipientFilter filter( pPlayer );
					TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_NO_TELE_WITH_FLAG );
				}
				return;
			}
			// get the velocity of the player touching the teleporter
			if ( pPlayer->GetAbsVelocity().Length() < 5.0 )
			{
				CObjectTeleporter *pDest = GetMatchingTeleporter();

				if ( pDest )
				{
					TeleporterSend( pPlayer );
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Receive a teleporting player 
//-----------------------------------------------------------------------------
bool CObjectTeleporter::IsMatchingTeleporterReady( void )
{
	if ( m_hMatchingTeleporter.Get() == NULL )
	{
		m_hMatchingTeleporter = FindMatch();
	}

	CObjectTeleporter *pMatch = GetMatchingTeleporter();

	if ( pMatch &&
		 pMatch->GetState() != TELEPORTER_STATE_BUILDING && 
		!pMatch->IsDisabled() &&
		!pMatch->IsUpgrading() &&
		!pMatch->IsRedeploying() )
		return true;

	return false;
}

bool CObjectTeleporter::PlayerCanBeTeleported(CTFPlayer* pSender)
{
	bool bResult = false;

	if ( pSender )
	{
		if ( !pSender->HasTheFlag() )
		{
#ifdef PF2_DLL
			// is this even something in the tf2 code?
			return true;
#else
			int iTeamNumber = pSender->GetTeamNumber();

			// Don't teleport enemies (unless it's a spy)
			if (GetTeamNumber() != pSender->GetTeamNumber() && pSender->IsPlayerClass(TF_CLASS_SPY))
				iTeamNumber = pSender->m_Shared.GetDisguiseTeam();

			if (GetTeamNumber() == iTeamNumber)
			{
				bResult = true;
			}
#endif
		}
	}

	return bResult;
}

bool CObjectTeleporter::IsSendingPlayer(CTFPlayer* pSender)
{
	bool bResult = false;

	if ( pSender && m_hTeleportingPlayer.Get() )
	{
		bResult = m_hTeleportingPlayer.Get() == pSender;
	}
	return bResult;
}

void CObjectTeleporter::CopyUpgradeStateToMatch( CObjectTeleporter* pMatch, bool bCopyFrom )
{
	// I don't even know

	if ( !pMatch )
		return;

	CObjectTeleporter *pObjToCopyFrom = bCopyFrom ? pMatch : this;
	CObjectTeleporter *pObjToCopyTo = bCopyFrom ? this : pMatch;

	pObjToCopyTo->m_iUpgradeMetal = pObjToCopyFrom->m_iUpgradeMetal;
	pObjToCopyTo->m_iMaxHealth = pObjToCopyFrom->m_iMaxHealth;
	pObjToCopyTo->m_iUpgradeMetalRequired = pObjToCopyFrom->m_iUpgradeMetalRequired;
	pObjToCopyTo->m_iUpgradeLevel = pObjToCopyFrom->m_iUpgradeLevel;
	
}

bool CObjectTeleporter::CheckUpgradeOnHit( CTFPlayer* pPlayer )
{
	bool bUpgradeSuccessful = false;

	if ( BaseClass::CheckUpgradeOnHit( pPlayer ) )
	{
		CObjectTeleporter* pMatch = GetMatchingTeleporter();

		if ( pMatch )
		{
			//pMatchingTeleporter->m_iUpgradeMetal = m_iUpgradeMetal;
			if ( pMatch && pMatch->CanBeUpgraded( pPlayer ) && GetUpgradeLevel() > pMatch->GetUpgradeLevel() )
			{
				// This end just got upgraded so make another end play upgrade anim if possible.
				pMatch->StartUpgrading();
			}
			// Other end still needs to keep up even while hauled etc.
			CopyUpgradeStateToMatch( pMatch, false );
		}

		bUpgradeSuccessful = true;
	}

	return bUpgradeSuccessful;
}

void CObjectTeleporter::InitializeMapPlacedObject( void )
{
	BaseClass::InitializeMapPlacedObject();

	CObjectTeleporter *pMatch = dynamic_cast<CObjectTeleporter*> ( gEntList.FindEntityByName( NULL, m_szMatchingTeleporterName ) ) ;
	if ( pMatch )
	{
		// Copy upgrade state from higher level end.
		bool bCopyFrom = pMatch->GetUpgradeLevel() > GetUpgradeLevel();

		if ( pMatch->GetUpgradeLevel() == GetUpgradeLevel() )
		{
			// If same level use it if it has more metal.
			bCopyFrom = pMatch->m_iUpgradeMetal > m_iUpgradeMetal;
		}

		CopyUpgradeStateToMatch( pMatch, bCopyFrom );

		m_hMatchingTeleporter = pMatch;
	}
}


CObjectTeleporter *CObjectTeleporter::GetMatchingTeleporter( void )
{
	return m_hMatchingTeleporter.Get();
}

void CObjectTeleporter::DeterminePlaybackRate( void )
{
	float flPlaybackRate = GetPlaybackRate();

	bool bWasBelowFullSpeed = ( flPlaybackRate < 1.0f );

	if ( IsBuilding() )
	{
		// Default half rate, author build anim as if one player is building
		SetPlaybackRate( GetConstructionMultiplier() * 0.5 );	
	}
	else if ( IsPlacing() )
	{
		SetPlaybackRate( 1.0f );
	}
	else
	{
		float flFrameTime = 0.1;	// BaseObjectThink delay

		switch( m_iState )
		{
		case TELEPORTER_STATE_READY:	
			{
				// spin up to 1.0 from whatever we're at, at some high rate
				flPlaybackRate = Approach( 1.0f, flPlaybackRate, 0.5f * flFrameTime );
			}
			break;

		case TELEPORTER_STATE_RECHARGING:
			{
				// Recharge - spin down to low and back up to full speed over 10 seconds

				// 0 -> 4, spin to low
				// 4 -> 6, stay at low
				// 6 -> 10, spin to 1.0

				float flScale = g_flTeleporterRechargeTimes[GetUpgradeLevel() - 1] / g_flTeleporterRechargeTimes[0];
				
				float flToLow = 4.0f * flScale;
				float flToHigh = 6.0f * flScale;
				float flRechargeTime = g_flTeleporterRechargeTimes[GetUpgradeLevel() - 1];

				float flTimeSinceChange = gpGlobals->curtime - m_flLastStateChangeTime;

				float flLowSpinSpeed = 0.15f;

				if ( flTimeSinceChange <= flToLow )
				{
					flPlaybackRate = RemapVal( gpGlobals->curtime,
						m_flLastStateChangeTime,
						m_flLastStateChangeTime + flToLow,
						1.0f,
						flLowSpinSpeed );
				}
				else if ( flTimeSinceChange > flToLow && flTimeSinceChange <= flToHigh )
				{
					flPlaybackRate = flLowSpinSpeed;
				}
				else
				{
					flPlaybackRate = RemapVal( gpGlobals->curtime,
						m_flLastStateChangeTime + flToHigh,
						m_flLastStateChangeTime + flRechargeTime,
						flLowSpinSpeed,
						1.0f );
				}
			}		
			break;

		default:
			{
				if ( m_flLastStateChangeTime <= 0.0f )
				{
					flPlaybackRate = 0.0f;
				}
				else
				{
					// lost connect - spin down to 0.0 from whatever we're at, slowish rate
					flPlaybackRate = Approach( 0.0f, flPlaybackRate, 0.25f * flFrameTime );
				}
			}
			break;
		}

		SetPlaybackRate( flPlaybackRate );
	}

	bool bBelowFullSpeed = ( GetPlaybackRate() < 1.0f );

	if ( m_iBlurBodygroup >= 0 && bBelowFullSpeed != bWasBelowFullSpeed )
	{
		if ( bBelowFullSpeed )
		{
			SetBodygroup( m_iBlurBodygroup, 0 );	// turn off blur bodygroup
		}
		else
		{
			SetBodygroup( m_iBlurBodygroup, 1 );	// turn on blur bodygroup
		}
	}

	StudioFrameAdvance();
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CObjectTeleporter::TeleporterThink(void)
{
	SetContextThink(&CObjectTeleporter::TeleporterThink, gpGlobals->curtime + BUILD_TELEPORTER_NEXT_THINK, TELEPORTER_THINK_CONTEXT);

#ifdef PF2_DLL
	//Chris: Not the greatest way to do it, but without any other means of doing it, this is all I have for the time being
	if ( IsMatchingTeleporterReady() && IsEnemyUsing() && m_flNextCheckEnemyUsing <= gpGlobals->curtime )
	{
		SetEnemyUsing( false );
	}
#endif

	// At any point, if our match is not ready, revert to IDLE
	if ( IsDisabled() || IsRedeploying() || !IsMatchingTeleporterReady() )
	{
		ShowDirectionArrow( false );

		if ( GetState() != TELEPORTER_STATE_IDLE && !IsUpgrading() )
		{
			SetState( TELEPORTER_STATE_IDLE );

			CObjectTeleporter* pMatchingTeleporter = GetMatchingTeleporter();
			if ( !pMatchingTeleporter )
			{
				// The other end has been destroyed. Revert back to L1.
				m_iUpgradeLevel = 1;

				// We need to adjust for any damage received if we downgraded
				float iHealthPercentage = GetHealth() / GetMaxHealthForCurrentLevel();
				SetMaxHealth( GetMaxHealthForCurrentLevel() );
				SetHealth( (int)floorf(GetMaxHealthForCurrentLevel() * iHealthPercentage) );
				m_iUpgradeMetal = 0;
			}
		}
		return;
	}

		if ( m_flMyNextThink && m_flMyNextThink > gpGlobals->curtime )
			return;

		// pMatch is not NULL and is not building
		CObjectTeleporter* pMatch = GetMatchingTeleporter();

		Assert( pMatch );
		Assert( pMatch->m_iState != TELEPORTER_STATE_BUILDING );	
		switch ( m_iState )
		{
			// Teleporter is not yet active, do nothing
		case TELEPORTER_STATE_BUILDING:
		case TELEPORTER_STATE_UPGRADING:
			ShowDirectionArrow( false );
			break;

		default:
		case TELEPORTER_STATE_IDLE:
			// Do we have a match that is active?
			if ( IsMatchingTeleporterReady() && !IsUpgrading() && gpGlobals->curtime > m_flRechargeTime )
			{
				SetState( TELEPORTER_STATE_READY );
				EmitSound( "Building_Teleporter.Ready" );

				if ( GetObjectMode() == TELEPORTER_TYPE_ENTRANCE )
				{
					ShowDirectionArrow( true );
				}
			}
			break;

		case TELEPORTER_STATE_READY:
			break;

		case TELEPORTER_STATE_SENDING:
		{
			pMatch->TeleporterReceive( m_hTeleportingPlayer, 1.0 );

			m_flRechargeTime = gpGlobals->curtime + ( BUILD_TELEPORTER_FADEOUT_TIME + BUILD_TELEPORTER_FADEIN_TIME + g_flTeleporterRechargeTimes[GetUpgradeLevel() - 1] );

			// change state to recharging...
			SetState( TELEPORTER_STATE_RECHARGING );
		}
		break;

		case TELEPORTER_STATE_RECEIVING:
		{
			// get the position we'll move the player to
			Vector newPosition = GetAbsOrigin();
			newPosition.z += TELEPORTER_MAXS.z + 1;
			// Telefrag anyone in the way
			CBaseEntity* pEnts[ 256 ];
			Vector mins, maxs;
			Vector expand( 4, 4, 4 );
			mins = newPosition + VEC_HULL_MIN - expand;
			maxs = newPosition + VEC_HULL_MAX + expand;
			CTFPlayer* pTeleportingPlayer = m_hTeleportingPlayer.Get();

			// move the player
			if ( pTeleportingPlayer )
			{
				CUtlVector<CBaseEntity*> hPlayersToKill;
				bool bClear = true;
				// Telefrag any players in the way
				int numEnts = UTIL_EntitiesInBox( pEnts, 256, mins, maxs, 0 );
				if ( numEnts )
				{
					//Iterate through the list and check the results
					for ( int i = 0; i < numEnts && bClear; i++ )
					{
						if ( pEnts[i] == NULL )
							continue;
						if ( pEnts[i] == this )
							continue;

						// kill players
						if ( pEnts[i]->IsPlayer() )
						{
							if ( !pTeleportingPlayer->InSameTeam( pEnts[i] ) )
							{
								hPlayersToKill.AddToTail( pEnts[i] );
							}
							continue;
						}
						if ( pEnts[i]->IsBaseObject() )
							continue;

						// Solid entities will prevent a teleport
						if ( pEnts[ i ]->IsSolid() && pEnts[ i ]->ShouldCollide( pTeleportingPlayer->GetCollisionGroup(), MASK_ALL ) &&
							g_pGameRules->ShouldCollide( pTeleportingPlayer->GetCollisionGroup(), pEnts[ i ]->GetCollisionGroup() ) )
						{
							// We're going to teleport into something solid. Abort & destroy this exit.
							bClear = false;
						}
					}
				}
				if ( bClear )
				{
					// Telefrag all enemy players we've found
					for ( int player = 0; player < hPlayersToKill.Count(); player++ )
					{
						hPlayersToKill[ player ]->TakeDamage( CTakeDamageInfo( this, pTeleportingPlayer, 1000, DMG_CRUSH ) );
					}
					pTeleportingPlayer->Teleport(&newPosition, &(GetAbsAngles()), &vec3_origin);

#ifdef PF2_DLL
					if ( pTeleportingPlayer->GetTeamNumber() != GetTeamNumber() && 
						pTeleportingPlayer->GetPlayerClass()->GetClassIndex() != TF_CLASS_SPY )
					{
						pTeleportingPlayer->AwardAchievement( ACHIEVEMENT_PF_USE_ENEMY_TELEPORTER );
					}
#endif
					// Unzoom if we are a sniper zoomed!
					if ((pTeleportingPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_SNIPER) &&
						pTeleportingPlayer->m_Shared.InCond(TF_COND_AIMING))
					{
						CTFWeaponBase* pWpn = pTeleportingPlayer->GetActiveTFWeapon();
						if ( pWpn && pWpn->GetWeaponID() == TF_WEAPON_SNIPERRIFLE )
						{
							CTFSniperRifle* pRifle = static_cast<CTFSniperRifle*>(pWpn);
							pRifle->ToggleZoom();
						}
					}
					pTeleportingPlayer->SetFOV(pTeleportingPlayer, 0, tf_teleporter_fov_time.GetFloat(), tf_teleporter_fov_start.GetInt());
					color32 fadeColor = { 255,255,255,100 };
					UTIL_ScreenFade(pTeleportingPlayer, fadeColor, 0.25, 0.4, FFADE_IN);
				}
				else
				{
					DetonateObject();
				}
			}
			SetState( TELEPORTER_STATE_RECEIVING_RELEASE );

			m_flMyNextThink = gpGlobals->curtime + ( BUILD_TELEPORTER_FADEIN_TIME );
		}
		break;
		case TELEPORTER_STATE_RECEIVING_RELEASE:
		{
			CTFPlayer* pTeleportingPlayer = m_hTeleportingPlayer.Get();
			if ( pTeleportingPlayer )
			{
#ifndef PF2_DLL

				int iPlayerTeam = pTeleportingPlayer->GetTeamNumber();
				if ( pTeleportingPlayer->IsPlayerClass( TF_CLASS_SPY ) && pTeleportingPlayer->m_Shared.InCond( TF_COND_DISGUISED ) )
				{
					iPlayerTeam = pTeleportingPlayer->m_Shared.GetDisguiseTeam();
				}
				int iTeam = GetBuilder() ? GetBuilder()->GetTeamNumber() : GetTeamNumber();
				// Don't show teleporter trail for players with different color.
				if ( iPlayerTeam == iTeam )
#endif
				{
					pTeleportingPlayer->TeleportEffect();
				}
				pTeleportingPlayer->m_Shared.RemoveCond( TF_COND_SELECTED_TO_TELEPORT );
				CTF_GameStats.Event_PlayerUsedTeleport( GetBuilder(), pTeleportingPlayer );

				IGameEvent* event = gameeventmanager->CreateEvent( "player_teleported" );
				if ( event )
				{
					event->SetInt("userid", pTeleportingPlayer->GetUserID());

					if (GetBuilder())
						event->SetInt("builderid", GetBuilder()->GetUserID());

					Vector vecOrigin = GetAbsOrigin();
					Vector vecDestinationOrigin = GetMatchingTeleporter()->GetAbsOrigin();
					Vector vecDifference = Vector(vecOrigin.x - vecDestinationOrigin.x, vecOrigin.y - vecDestinationOrigin.y, vecOrigin.z - vecDestinationOrigin.z);

					float flDist = sqrtf( pow( vecDifference.x, 2 ) + pow( vecDifference.y, 2 ) + pow( vecDifference.z, 2 ) );

					event->SetFloat( "dist", flDist );

					gameeventmanager->FireEvent(event, true);
				}
				if ( pTeleportingPlayer != GetBuilder() )
					 pTeleportingPlayer->SpeakConceptIfAllowed( MP_CONCEPT_TELEPORTED );
			}

			// reset the pointers to the player now that we're done teleporting
			SetTeleportingPlayer( NULL );
			pMatch->SetTeleportingPlayer( NULL );

			SetState( TELEPORTER_STATE_RECHARGING );

			m_flMyNextThink = gpGlobals->curtime + ( g_flTeleporterRechargeTimes[ GetUpgradeLevel() - 1 ] );
		}
		break;

		case TELEPORTER_STATE_RECHARGING:
			// If we are finished recharging, go active
			if ( gpGlobals->curtime > m_flRechargeTime )
			{
				SetState( TELEPORTER_STATE_READY );
				EmitSound( "Building_Teleporter.Ready" );
			}
			break;
		}	
}


int CObjectTeleporter::GetBaseHealth(void)
{
	return 150;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
bool CObjectTeleporter::IsUpgrading( void ) const
{
	return ( m_iState == TELEPORTER_STATE_UPGRADING );
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
char* CObjectTeleporter::GetPlacementModel(void)
{
	if ( GetObjectMode() == TELEPORTER_TYPE_ENTRANCE )
		return TELEPORTER_MODEL_ENTRANCE_PLACEMENT;

	return TELEPORTER_MODEL_EXIT_PLACEMENT;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int CObjectTeleporter::GetMaxUpgradeLevel(void)
{
	return pf_upgradable_buildings.GetBool() ? 3 : 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectTeleporter::FinishedBuilding( void )
{
	BaseClass::FinishedBuilding();

	SetActivity( ACT_OBJ_RUNNING );
	SetPlaybackRate( 0.0f );
}

void CObjectTeleporter::SetState( int state )
{
	if ( state != m_iState )
	{
		m_iState = state;
		m_flLastStateChangeTime = gpGlobals->curtime;
	}
}

void CObjectTeleporter::ShowDirectionArrow( bool bShow )
{
	if ( bShow != m_bShowDirectionArrow )
	{
		if ( m_iDirectionBodygroup >= 0 )
		{
			SetBodygroup( m_iDirectionBodygroup, bShow ? 1 : 0 );
		}

		m_bShowDirectionArrow = bShow;

		if ( bShow )
		{

			CObjectTeleporter *pMatch = GetMatchingTeleporter();

			Assert( pMatch );

			Vector vecToOwner = pMatch->GetAbsOrigin() - GetAbsOrigin();
			QAngle angleToExit;
			VectorAngles( vecToOwner, Vector(0,0,1), angleToExit );
			angleToExit -= GetAbsAngles();

			// pose param is flipped and backwards, adjust.
			m_flYawToExit = anglemod( -angleToExit.y + 180.0 );
			// m_flYawToExit = AngleNormalize( -angleToExit.y + 180.0 );
			// For whatever reason the original code normalizes angle 0 to 360 while pose param
			// takes angle from -180 to 180. I have no idea how did this work properly
			// in official TF2 all this time. (Nicknine)
		}
	}
}

int CObjectTeleporter::DrawDebugTextOverlays(void)
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT)
	{
		CObjectTeleporter* pMatch = GetMatchingTeleporter();

		char tempstr[512];

		// match
		Q_snprintf(tempstr, sizeof(tempstr), "Match Found: %s", (pMatch != NULL) ? "Yes" : "No");
		EntityText(text_offset, tempstr, 0);
		text_offset++;

		// state
		Q_snprintf(tempstr, sizeof(tempstr), "State: %d", m_iState);
		EntityText(text_offset, tempstr, 0);
		text_offset++;

		// recharge time
		if ( gpGlobals->curtime < m_flRechargeTime )
		{
			float flPercent = (m_flRechargeTime - gpGlobals->curtime) / g_flTeleporterRechargeTimes[GetUpgradeLevel() - 1];

			Q_snprintf(tempstr, sizeof(tempstr), "Recharging: %.1f", flPercent);
			EntityText(text_offset, tempstr, 0);
			text_offset++;
		}
	}

	return text_offset;
}

bool CObjectTeleporter::Command_Repair(CTFPlayer* pActivator)
{
	bool bRepaired = false;
	int iAmountToHeal = 0;
	int iRepairCost = 0;

	if ( GetHealth() < GetMaxHealth() )
	{
		iAmountToHeal = min(100, GetMaxHealth() - GetHealth());

		// repair the building
		iRepairCost = ceil((float)(iAmountToHeal) * 0.2f);

		TRACE_OBJECT(UTIL_VarArgs("%0.2f CObjectDispenser::Command_Repair ( %d / %d ) - cost = %d\n", gpGlobals->curtime,
			GetHealth(),
			GetMaxHealth(),
			iRepairCost));

		if ( iRepairCost > 0 )
		{
			if ( iRepairCost > pActivator->GetBuildResources() )
			{
				iRepairCost = pActivator->GetBuildResources();
			}

			pActivator->RemoveBuildResources( iRepairCost );

			float flNewHealth = min( GetMaxHealth(), GetHealth() + ( iRepairCost * 5 ) );
			SetHealth(flNewHealth);

			bRepaired = ( iRepairCost > 0 );

			CObjectTeleporter* pMatchingTeleporter = GetMatchingTeleporter();

			if (pMatchingTeleporter && pMatchingTeleporter->GetState() != TELEPORTER_STATE_BUILDING && !pMatchingTeleporter->IsUpgrading() )
			{
				float flNewHealth = min( pMatchingTeleporter->GetMaxHealth(), pMatchingTeleporter->GetHealth() + ( iRepairCost * 5 ) );
				pMatchingTeleporter->SetHealth( flNewHealth );
			}
		}
	}
	else if ( GetMatchingTeleporter() ) // See if the other teleporter needs repairing
	{
		CObjectTeleporter* pMatchingTeleporter = GetMatchingTeleporter();
		if (pMatchingTeleporter->GetHealth() < pMatchingTeleporter->GetMaxHealth() && 
			pMatchingTeleporter->GetState() != TELEPORTER_STATE_BUILDING && !pMatchingTeleporter->IsUpgrading())
		{
			iAmountToHeal = min(100, pMatchingTeleporter->GetMaxHealth() - pMatchingTeleporter->GetHealth());

			// repair the building
			iRepairCost = ceil((float)(iAmountToHeal) * 0.2f);

			TRACE_OBJECT(UTIL_VarArgs("%0.2f CObjectDispenser::Command_Repair ( %d / %d ) - cost = %d\n", gpGlobals->curtime,
				pMatchingTeleporter->GetHealth(),
				pMatchingTeleporter->GetMaxHealth(),
				iRepairCost));

			if (iRepairCost > 0)
			{
				if (iRepairCost > pActivator->GetBuildResources())
				{
					iRepairCost = pActivator->GetBuildResources();
				}

				pActivator->RemoveBuildResources(iRepairCost);

				float flNewHealth = min(pMatchingTeleporter->GetMaxHealth(), pMatchingTeleporter->GetHealth() + (iRepairCost * 5));
				pMatchingTeleporter->SetHealth(flNewHealth);

				bRepaired = (iRepairCost > 0);
			}
		}
	}

	return bRepaired;
}

CObjectTeleporter* CObjectTeleporter::FindMatch( void )
{
	int iObjMode = GetObjectMode();
	int iOppositeMode = (iObjMode == TELEPORTER_TYPE_ENTRANCE) ? TELEPORTER_TYPE_EXIT : TELEPORTER_TYPE_ENTRANCE;

	CObjectTeleporter *pMatch = NULL;

	CTFPlayer *pBuilder = GetBuilder();

	Assert( pBuilder );

	if ( !pBuilder )
	{
		return NULL;
	}

	int i;
	int iNumObjects = pBuilder->GetObjectCount();

	for ( i=0;i<iNumObjects;i++ )
	{
		CBaseObject *pObj = pBuilder->GetObject( i );

		if ( pObj && pObj->GetType() == GetType() && pObj->GetObjectMode() == iOppositeMode && !pObj->IsDisabled() )
		{
			pMatch = ( CObjectTeleporter * )pObj;
			bool bCopyFrom = pMatch->GetUpgradeLevel() > GetUpgradeLevel();
			if ( pMatch->GetUpgradeLevel() == GetUpgradeLevel() )
			{
				// If same level use it if it has more metal.
				bCopyFrom = pMatch->m_iUpgradeMetal > m_iUpgradeMetal;
			}
			CopyUpgradeStateToMatch( pMatch, bCopyFrom );
			break;
		}
	}

	return pMatch;
}

//-----------------------------------------------------------------------------
// If detonated, do some damage
//-----------------------------------------------------------------------------
void CObjectTeleporter::DetonateObject(void)
{
	float flDamage = min(50 + m_iHealth, GetMaxHealth());

	// Play explosion sound and effect.
	Vector vecOrigin = GetAbsOrigin();
	CTakeDamageInfo info( this, GetBuilder(), vec3_origin, GetAbsOrigin(), flDamage, DMG_BLAST | DMG_HALF_FALLOFF );
	info.SetDamageCustom( TF_DMG_CUSTOM_TELEPORTER_EXPLOSION );
	RadiusDamage( info, vecOrigin, flDamage, CLASS_NONE, NULL );

	BaseClass::DetonateObject();
}

//-----------------------------------------------------------------------------
// Purpose: Destroy building and give builder metal invested
//-----------------------------------------------------------------------------
void CObjectTeleporter::DismantleObject( CTFPlayer *pPlayer )
{
	pPlayer->SetAllowAmmoOverdraw( true );

	int iMetal = m_iUpgradeMetal;
	iMetal += GetObjectInfo( ObjectType() )->m_Cost;
	iMetal += ( m_iUpgradeLevel - 1 ) * m_iUpgradeMetalRequired;
	iMetal *= pf_building_dismantle_factor.GetFloat();

	int iMetalGiven = pPlayer->GiveAmmo( iMetal, TF_AMMO_METAL, false );

	pPlayer->SetAllowAmmoOverdraw( false );
	BaseClass::DismantleObject( pPlayer );
}