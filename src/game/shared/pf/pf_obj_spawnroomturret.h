//========= Copyright � 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: Spawnroom Turret
//
// $NoKeywords: $
//=============================================================================//

#ifndef PF_OBJ_SPAWNROOMTURRET_H
#define PF_OBJ_SPAWNROOMTURRET_H
#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
#include "c_obj_sentrygun.h"
#else
#include "tf_obj_sentrygun.h"
#include "func_respawnroom.h"
#endif

#ifdef CLIENT_DLL
#define CObjectSpawnroomTurret C_ObjectSpawnroomTurret
#define CObjectSentrygun C_ObjectSentrygun
#endif

class CObjectSpawnroomTurret : public CObjectSentrygun
{
    DECLARE_CLASS( CObjectSpawnroomTurret, CObjectSentrygun );
    DECLARE_NETWORKCLASS();
public:
    CObjectSpawnroomTurret();
	~CObjectSpawnroomTurret();

	// Check used for determining EyePosition and bounding box
	bool IsGroundTurret() { return m_bGroundTurret; }

protected:
	CNetworkVar( bool, m_bGroundTurret );

#ifdef CLIENT_DLL
public:
	virtual void GetStatusText( void );
	virtual void GetTargetIDString( wchar_t* sIDString, int iMaxLenInBytes );
	virtual void GetTargetIDDataString( wchar_t* sDataString, int iMaxLenInBytes );
private:
	// not defined, not accessible
	CObjectSpawnroomTurret( const CObjectSpawnroomTurret & );

#else // GAME_DLL
public:
    DECLARE_DATADESC();
	static CObjectSpawnroomTurret* Create(const Vector &vOrigin, const QAngle &vAngles);

	// overrides
	virtual void Precache();
	virtual void Spawn();
	virtual void PlayStartupAnimation( void );
	virtual void SentryThink( void );
	virtual void OnGoActive( void );
	virtual int	 OnTakeDamage( const CTakeDamageInfo& info );
	virtual void Disable( float flTime );
	virtual void FoundTarget( CBaseEntity *pTarget, const Vector &vecSoundCenter );
	virtual Vector EyePosition( void );
	virtual bool FindTarget( void );	
	virtual void StartUpgrading( void ) {}
	virtual void FinishUpgrading( void ) {}
	virtual bool RetractCheck( void );
	virtual void SentryRotate( void );
	virtual void Attack();
	virtual bool Fire();
	virtual int GetMaxUpgradeLevel( void ) { return 1; }
    virtual bool CanBeUpgraded() { return false; }
	virtual const char* GetIdleSound() { return "Building_SpawnroomTurret.Idle"; }
	virtual bool ValidTargetPlayer( CTFPlayer *pPlayer, const Vector &vecStart, const Vector &vecEnd );

	void Deploy();
	void Retract();
	void DisabledThink();

	// helper functions
	bool IsTurretDormant() const { return m_iState == TURRET_STATE_DORMANT; }
	bool IsAttacking() const { return m_iState == TURRET_STATE_ATTACKING; }
	bool IsSearching() const { return m_iState == TURRET_STATE_SEARCHING; }

	virtual void DetermineAnimation( void );
	void	InputRoundActivate( inputdata_t &inputdata );

private:
	CFuncRespawnRoom *GetRespawnRoom( void ) { return m_hRespawnRoom; }

private:
	bool						m_bDeploy;
	float						m_flSearchingGracePeriod;
	float						m_flTimeToHeal;				// Used when the turret is disabled and needs to regenerate
	bool						m_bTracerTick;				// have to use this because sentry did it based on decreasing ammo shells
	string_t					m_iszRespawnRoomName;
	bool						m_bHasHealth;				// tell the turret to use limited health
	CHandle<CFuncRespawnRoom>	m_hRespawnRoom;

#endif

};
#endif // PF_OBJ_SPAWNROOMTURRET_H