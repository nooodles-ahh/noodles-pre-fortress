//====== Copyright � 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: Shared player code.
//
//=============================================================================
#ifndef TF_PLAYER_SHARED_H
#define TF_PLAYER_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#include "networkvar.h"
#include "tf_shareddefs.h"
#include "tf_weaponbase.h"
#include "basegrenade_shared.h"

// Client specific.
#ifdef CLIENT_DLL
class C_TFPlayer;
// Server specific.
#else
class CTFPlayer;
#endif

//=============================================================================
//
// Tables.
//

// Client specific.
#ifdef CLIENT_DLL

	EXTERN_RECV_TABLE( DT_TFPlayerShared );

// Server specific.
#else

	EXTERN_SEND_TABLE( DT_TFPlayerShared );

#endif


//=============================================================================

#define PERMANENT_CONDITION		-1

// Damage storage for crit multiplier calculation
class CTFDamageEvent
{
	DECLARE_EMBEDDED_NETWORKVAR()

public:
	float flDamage;
	float flTime;
	bool bKill;
};

//=============================================================================
//
// Shared player class.
//
class CTFPlayerShared
{
public:

// Client specific.
#ifdef CLIENT_DLL

	friend class C_TFPlayer;
	typedef C_TFPlayer OuterClass;
	DECLARE_PREDICTABLE();

// Server specific.
#else

	friend class CTFPlayer;
	typedef CTFPlayer OuterClass;

#endif
	
	DECLARE_EMBEDDED_NETWORKVAR()
	DECLARE_CLASS_NOBASE( CTFPlayerShared );

	// Initialization.
	CTFPlayerShared();
	void Init( OuterClass *pOuter );

	// State (TF_STATE_*).
	int		GetState() const					{ return m_nPlayerState; }
	void	SetState( int nState )				{ m_nPlayerState = nState; }
	bool	InState( int nState )				{ return ( m_nPlayerState == nState ); }

	// Condition (TF_COND_*).
	int		GetCond() const						{ return m_nPlayerCond; }
	void	SetCond( int nCond )				{ m_nPlayerCond = nCond; }
	void	AddCond( int nCond, float flDuration = PERMANENT_CONDITION );
	void	RemoveCond( int nCond );
	bool	InCond( int nCond );
	void	RemoveAllCond( CTFPlayer *pPlayer );
	void	OnConditionAdded( int nCond );
	void	OnConditionRemoved( int nCond );
	void	ConditionThink( void );
	float	GetConditionDuration( int nCond );
	bool	IsHallucinating( void ) { return InCond( TF_COND_HALLUCINATING ); }
	void	StopHallucinating() { RemoveCond( TF_COND_HALLUCINATING ); }
	bool	IsCritBoosted( void ) { return InCond( TF_COND_CRITBOOSTED ); }

	bool	IsStealthed( void );

	void	ConditionGameRulesThink( void );

	void	InvisibilityThink( void );

	int		GetMaxBuffedHealth( void );

#ifdef CLIENT_DLL
	CNewParticleEffect *m_pSparkleProp;
	// This class only receives calls for these from C_TFPlayer, not
	// natively from the networking system
	virtual void OnPreDataChanged( void );
	virtual void OnDataChanged( void );

	// check the newly networked conditions for changes
	void	UpdateConditions( void );
#endif

	void	Disguise( int nTeam, int nClass );
	void	CompleteDisguise( void );
	void	RemoveDisguise( void );
	void	FindDisguiseTarget( void );
	int		GetDisguiseTeam( void )				{ return m_nDisguiseTeam; }
	int		GetDisguiseClass( void ) 			{ return m_nDisguiseClass; }
	int		GetDesiredDisguiseClass( void )		{ return m_nDesiredDisguiseClass; }
	int		GetDesiredDisguiseTeam( void )		{ return m_nDesiredDisguiseTeam; }
	CBaseEntity *GetDisguiseTarget( void )
	{
#ifdef CLIENT_DLL
		if ( m_iDisguiseTargetIndex == TF_DISGUISE_TARGET_INDEX_NONE )
			return NULL;
		return cl_entitylist->GetBaseEntity( m_iDisguiseTargetIndex );
#else
		return m_hDisguiseTarget.Get();
#endif
	}
	int		GetDisguiseHealth( void )			{ return m_iDisguiseHealth; }
	void	SetDisguiseHealth( int iDisguiseHealth );
	int		GetDisguiseArmor( void )			{ return m_iDisguiseArmor; }
	void	SetDisguiseArmor( int iDisguiseArmor );
	int		GetDisguiseWeaponSlot( void )		{ return m_iDisguiseWeaponSlot;	}
	void	RecalcDisguiseWeapon( void );

#ifdef CLIENT_DLL
	void	OnDisguiseChanged( void );
	int		GetDisguiseWeaponModelIndex( void ) { return m_iDisguiseWeaponModelIndex; }
	CTFWeaponInfo *GetDisguiseWeaponInfo( void );

	void UpdateCritBoostEffect( bool bForceHide = false );
#endif

#ifdef GAME_DLL
	void	Heal( CTFPlayer *pPlayer, float flAmount, bool bDispenserHeal = false );
	void	StopHealing( CTFPlayer *pPlayer );

	void	RegenerateArmor( CTFPlayer* pPlayer, float flAmountArmor );
	void	StopRegeneratingArmor( CTFPlayer* pPlayer );

	void	RecalculateInvuln( bool bInstantRemove = false );
	int		FindHealerIndex( CTFPlayer *pPlayer );
	int		FindArmorerIndex(CTFPlayer *pPlayer );

	EHANDLE	GetFirstHealer();
	EHANDLE GetFirstArmorer();

#endif
	int		GetNumHealers( void ) { return m_nNumHealers; }
	int		GetNumArmorers( void ) { return m_nNumArmorers; }

	void	Burn( CTFPlayer *pPlayer, bool bNapalm = false );
	void	Infect(CTFPlayer* pPlayer);

	// Weapons.
	CTFWeaponBase *GetActiveTFWeapon() const;
	bool			IsActiveTFWeapon( int iWeaponID );

	// Utility.
	bool	IsAlly( CBaseEntity *pEntity );
	bool	IsLoser( void );
	int		MaxAmmo( int ammoType );

	// Separation force
	bool	IsSeparationEnabled( void ) const	{ return m_bEnableSeparation; }
	void	SetSeparation( bool bEnable )		{ m_bEnableSeparation = bEnable; }
	const Vector &GetSeparationVelocity( void ) const { return m_vSeparationVelocity; }
	void	SetSeparationVelocity( const Vector &vSeparationVelocity ) { m_vSeparationVelocity = vSeparationVelocity; }

	void	FadeInvis( float flInvisFadeTime );
	float	GetPercentInvisible( void );
	void	NoteLastDamageTime( int nDamage );
	void	OnSpyTouchedByEnemy( void );
	float	GetLastStealthExposedTime( void ) { return m_flLastStealthExposeTime; }

	int		GetDesiredPlayerClassIndex( void );

	float	GetSpyCloakMeter() const		{ return m_flCloakMeter; }
	float	GetSmokeBombExpireTime() const	{ return m_flSmokeBombExpire; }
	void	SetSpyCloakMeter( float val ) { m_flCloakMeter = val; }

	void	SetDetpackUsed( bool val ) { m_bUsedDetpack = val; }
	bool	GetDetpackUsed( void ) { return m_bUsedDetpack; }

	void	StartGrenadeReneration(int nGrenade);
	float	GetGrenadeRegenerationTime(int nGrenade) const { return nGrenade == 0 ? (float)m_flGrenade1RegenTime : (float)m_flGrenade2RegenTime; }

	bool	IsJumping( void ) { return m_bJumping; }
	void	SetJumping( bool bJumping );
	bool    IsAirDashing( void ) { return m_bAirDash; }
	void    SetAirDash( bool bAirDash );

	void	DebugPrintConditions( void );

	float	GetStealthNoAttackExpireTime( void );

	void	SetPlayerDominated( CTFPlayer *pPlayer, bool bDominated );
	bool	IsPlayerDominated( int iPlayerIndex );
	bool	IsPlayerDominatingMe( int iPlayerIndex );
	void	SetPlayerDominatingMe( CTFPlayer *pPlayer, bool bDominated );

	bool	IsCarryingObject( void ) { return m_bCarryingObject; }

#ifdef GAME_DLL
	void			SetCarriedObject( CBaseObject* pObj );
	CBaseObject*	GetCarriedObject( void );

	void	Concussion( void );
#endif

	int		GetTeleporterEffectColor( void ) { return m_nTeamTeleporterUsed; }
	void	SetTeleporterEffectColor( int iTeam ) { m_nTeamTeleporterUsed = iTeam; }
#ifdef CLIENT_DLL
	bool	ShouldShowRecentlyTeleported( void );
#endif

	CNetworkVar( float, m_flConcussionTime );
	CNetworkArray( float, m_flSlowedTime, 3 );
	CNetworkVar( float, m_flNextThrowTime );
	CNetworkVar( int, m_nAirDuckCount );
	CNetworkVar( bool, m_bBlockJump );
	CNetworkVar( bool, m_bUsedDetpack );
	CNetworkVar( int, m_iPrimed);
	CNetworkVar( float, m_flGrenade1RegenTime );
	CNetworkVar( float, m_flGrenade2RegenTime );
	CNetworkVar( float, m_flHallucinationTime );
	CNetworkVar( bool, m_bExecutedCritsForceVar);

	int GetSequenceForDeath( CBaseAnimating *pAnim, int iDamageCustom );

private:

	void ImpactWaterTrace( trace_t &trace, const Vector &vecStart );

	void OnAddStealthed( void );
	void OnAddInvulnerable( void );
	void OnAddTeleported( void );
	void OnAddBurning( void );
	void OnAddDisguising( void );
	void OnAddDisguised( void );
	void OnAddSmokeBomb( void );
	void OnAddCrits( void );

	void OnRemoveZoomed( void );
	void OnRemoveBurning( void );
	void OnRemoveStealthed( void );
	void OnRemoveDisguised( void );
	void OnRemoveDisguising( void );
	void OnRemoveInvulnerable( void );
	void OnRemoveTeleported( void );
	void OnRemoveSmokeBomb( void );
	void OnRemoveCrits( void );

	float GetCritMult( void );

#ifdef GAME_DLL
	void  UpdateCritMult( void );
	void  RecordDamageEvent( const CTakeDamageInfo &info, bool bKill );
	void  ClearDamageEvents( void ) { m_DamageEvents.Purge(); }
	int	  GetNumKillsInTime( float flTime );

	// Invulnerable.
	bool  IsProvidingInvuln( CTFPlayer *pPlayer );
	void  SetInvulnerable( bool bState, bool bInstant = false );
#endif

private:

	// Vars that are networked.
	CNetworkVar( int, m_nPlayerState );			// Player state.
	CNetworkVar( int, m_nPlayerCond );			// Player condition flags.
	float m_flCondExpireTimeLeft[TF_COND_LAST];		// Time until each condition expires

	// Arena spectators
	CNetworkVar( bool, m_bArenaSpectator ); // Arena Spectator

//TFTODO: What if the player we're disguised as leaves the server?
//...maybe store the name instead of the index?
	CNetworkVar( int, m_nDisguiseTeam );		// Team spy is disguised as.
	CNetworkVar( int, m_nDisguiseClass );		// Class spy is disguised as.
	EHANDLE m_hDisguiseTarget;					// Playing the spy is using for name disguise.
	CNetworkVar( int, m_iDisguiseTargetIndex );
	CNetworkVar( int, m_iDisguiseHealth );		// Health to show our enemies in player id
	CNetworkVar( int, m_iDisguiseArmor );		// Armor to show our enemies in player id
	CNetworkVar( int, m_nDesiredDisguiseClass );
	CNetworkVar( int, m_nDesiredDisguiseTeam );
	CNetworkVar( int, m_iDisguiseWeaponSlot );

	bool m_bEnableSeparation;		// Keeps separation forces on when player stops moving, but still penetrating
	Vector m_vSeparationVelocity;	// Velocity used to keep player seperate from teammates

	float m_flInvisibility;
	CNetworkVar( float, m_flInvisChangeCompleteTime );		// when uncloaking, must be done by this time
	float m_flLastStealthExposeTime;

	CNetworkVar( int, m_nNumHealers );
	CNetworkVar( int, m_nNumArmorers );
	// Vars that are not networked.
	OuterClass			*m_pOuter;					// C_TFPlayer or CTFPlayer (client/server).

#ifdef GAME_DLL
	// Healer handling 
	struct healers_t
	{
		EHANDLE	pPlayer;
		float	flAmount;
		bool	bDispenserHeal;
		float	iRecentAmount;
		float	flNextNotifyTime;
	};

	struct armorers_t
	{
		EHANDLE		pPlayer;
		float		flAmountArmor;
		int			iRecentArmorAmount;
		float		flNextNotifyArmorTime;
	};



	CUtlVector< healers_t >	m_aHealers;	
	CUtlVector< armorers_t >m_aArmorers;
	float					m_flHealFraction;	// Store fractional health amounts
	float					m_flDisguiseHealFraction;	// Same for disguised healing
	float					m_flArmorFraction;

	float m_flInvulnerableOffTime;
#endif

	// Burn handling
	CHandle<CTFPlayer>		m_hBurnAttacker;
	CHandle<CTFPlayer>		m_hInfectionAttacker;
	CNetworkVar( int,		m_nNumFlames );
	float					m_flFlameBurnTime;
	float					m_flFlameRemoveTime;
	float					m_flTauntRemoveTime;
	float					m_flInfectionTime;
	float					m_flNextArmorMelt;

	
	float m_flDisguiseCompleteTime;

	int	m_nOldConditions;
	int	m_nOldDisguiseClass;
	int	m_nOldDisguiseWeaponSlot;

	CNetworkVar( int, m_iDesiredPlayerClass );
	CNetworkVar(int, m_iDesiredWeaponID);
	CNetworkVar(int, m_iRespawnParticleID);

	float m_flNextBurningSound;

	CNetworkVar( float, m_flCloakMeter );	// [0,100]
	CNetworkVar( float, m_flSmokeBombExpire );

	CNetworkVar( bool, m_bJumping );
	CNetworkVar( bool, m_bAirDash );

	CNetworkVar( float, m_flStealthNoAttackExpire );
	CNetworkVar( float, m_flStealthNextChangeTime );

	CNetworkVar( int, m_iCritMult );

	CNetworkArray( bool, m_bPlayerDominated, MAX_PLAYERS+1 );		// array of state per other player whether player is dominating other players
	CNetworkArray( bool, m_bPlayerDominatingMe, MAX_PLAYERS+1 );	// array of state per other player whether other players are dominating this player

	CNetworkHandle( CBaseObject, m_hCarriedObject );
	CNetworkVar( bool, m_bCarryingObject );

	CNetworkVar( int, m_nTeamTeleporterUsed );

#ifdef GAME_DLL
	float	m_flNextCritUpdate;
	CUtlVector<CTFDamageEvent> m_DamageEvents;
#else
	int m_iDisguiseWeaponModelIndex;
	int m_iOldDisguiseWeaponModelIndex;
	CTFWeaponInfo *m_pDisguiseWeaponInfo;

	WEAPON_FILE_INFO_HANDLE	m_hDisguiseWeaponInfo;

	CNewParticleEffect *m_pCritEffect;
	EHANDLE m_hCritEffectHost;
	CSoundPatch *m_pCritSound;

	bool m_bWasCritBoosted;
#endif
};			   

#define TF_DEATH_DOMINATION				0x0001	// killer is dominating victim
#define TF_DEATH_ASSISTER_DOMINATION	0x0002	// assister is dominating victim
#define TF_DEATH_REVENGE				0x0004	// killer got revenge on victim
#define TF_DEATH_ASSISTER_REVENGE		0x0008	// assister got revenge on victim

extern const char *g_pszBDayGibs[22];

#endif // TF_PLAYER_SHARED_H
