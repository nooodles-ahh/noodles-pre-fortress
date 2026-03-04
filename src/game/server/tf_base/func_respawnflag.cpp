//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "func_respawnflag.h"
#include "tf_team.h"
#include "ndebugoverlay.h"
#include "entity_capture_flag.h"
#include "triggers.h"
#include "tf_player.h"
#include "tf_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Defines an area where objects cannot be built
//-----------------------------------------------------------------------------
class CFuncRespawnFlagZone : public CBaseTrigger
{
	DECLARE_CLASS( CFuncRespawnFlagZone, CBaseTrigger );
public:
	CFuncRespawnFlagZone();

	DECLARE_DATADESC();

	virtual void Spawn( void );
	virtual void Precache( void );
	virtual void Activate( void );

	// Inputs
	void	InputSetActive( inputdata_t &inputdata );
	void	InputSetInactive( inputdata_t &inputdata );
	void	InputToggleActive( inputdata_t &inputdata );

	void	SetActive( bool bActive );
	bool	GetActive() const;

	//bool	IsEmpty( void );
	bool	PointIsWithin( const Vector &vecPoint );

private:
	bool	m_bActive;
};

LINK_ENTITY_TO_CLASS( func_respawnflag, CFuncRespawnFlagZone );

BEGIN_DATADESC( CFuncRespawnFlagZone )

	// inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "SetActive", InputSetActive ),
	DEFINE_INPUTFUNC( FIELD_VOID, "SetInactive", InputSetInactive ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ToggleActive", InputToggleActive ),

END_DATADESC()


PRECACHE_REGISTER( func_respawnflag );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CFuncRespawnFlagZone::CFuncRespawnFlagZone()
{
}

//-----------------------------------------------------------------------------
// Purpose: Initializes the resource zone
//-----------------------------------------------------------------------------
void CFuncRespawnFlagZone::Spawn( void )
{
	BaseClass::Spawn();
	InitTrigger();

	m_bActive = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncRespawnFlagZone::Precache( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncRespawnFlagZone::Activate( void )
{
	BaseClass::Activate();
	SetActive( true );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncRespawnFlagZone::InputSetActive( inputdata_t &inputdata )
{
	SetActive( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncRespawnFlagZone::InputSetInactive( inputdata_t &inputdata )
{
	SetActive( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncRespawnFlagZone::InputToggleActive( inputdata_t &inputdata )
{
	if ( m_bActive )
	{
		SetActive( false );
	}
	else
	{
		SetActive( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncRespawnFlagZone::SetActive( bool bActive )
{
	m_bActive = bActive;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CFuncRespawnFlagZone::GetActive() const
{
	return m_bActive;
}

//-----------------------------------------------------------------------------
// Purpose: Return true if the specified point is within this zone
//-----------------------------------------------------------------------------
bool CFuncRespawnFlagZone::PointIsWithin( const Vector &vecPoint )
{
	Ray_t ray;
	trace_t tr;
	ICollideable *pCollide = CollisionProp();
	ray.Init( vecPoint, vecPoint );
	enginetrace->ClipRayToCollideable( ray, MASK_ALL, pCollide, &tr );
	return ( tr.startsolid );
}

//-----------------------------------------------------------------------------
// Purpose: Does a nobuild zone prevent us from building?
//-----------------------------------------------------------------------------
bool PointInRespawnFlagZone( const Vector &vecFlagOrigin )
{
	// Find out whether we're in a resource zone or not
	CBaseEntity *pEntity = NULL;
	while ((pEntity = gEntList.FindEntityByClassname( pEntity, "func_respawnflag" )) != NULL)
	{
		CFuncRespawnFlagZone *pRespawnFlag = (CFuncRespawnFlagZone *)pEntity;

		// Is this within this respawn flag?
		if (pRespawnFlag->GetActive() && pRespawnFlag->PointIsWithin(vecFlagOrigin) )
		{
			return true;	// respawn flag.
		}
	}

	return false; // flag should be ok.
}


//-----------------------------------------------------------------------------
// Purpose: Defines an area where objects cannot be built
//-----------------------------------------------------------------------------
class CFuncFlagAlert : public CBaseTrigger
{
	DECLARE_CLASS( CFuncFlagAlert, CBaseTrigger );
public:
	CFuncFlagAlert();

	DECLARE_DATADESC();

	virtual void Spawn( void );

	virtual void StartTouch( CBaseEntity *pOther );

private:
	bool	m_bPlaySound;
	int		m_iAlertDelay;

	COutputEvent	m_outputOnTriggeredTeam1;
	COutputEvent	m_outputOnTriggeredTeam2;

	float m_flNextAlert;
};

LINK_ENTITY_TO_CLASS( func_flag_alert, CFuncFlagAlert );

BEGIN_DATADESC( CFuncFlagAlert )

DEFINE_KEYFIELD( m_bPlaySound, FIELD_BOOLEAN, "playsound" ),
DEFINE_KEYFIELD( m_iAlertDelay, FIELD_INTEGER, "alert_delay" ),

DEFINE_OUTPUT( m_outputOnTriggeredTeam1, "OnTriggeredByTeam1" ),
DEFINE_OUTPUT( m_outputOnTriggeredTeam2, "OnTriggeredByTeam2" ),

END_DATADESC()


PRECACHE_REGISTER( func_flag_alert );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CFuncFlagAlert::CFuncFlagAlert()
{
}

//-----------------------------------------------------------------------------
// Purpose: Initializes the resource zone
//-----------------------------------------------------------------------------
void CFuncFlagAlert::Spawn( void )
{
	m_flNextAlert = 0.0f;

	BaseClass::Spawn();
	InitTrigger();
}

void CFuncFlagAlert::StartTouch( CBaseEntity *pOther )
{
	if( pOther->IsPlayer() )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( pOther );
		if( pTFPlayer )
		{
			if( pTFPlayer->GetTeamNumber() != GetTeamNumber() )
			{
				if( pTFPlayer->HasTheFlag() && m_flNextAlert <= gpGlobals->curtime )
				{

					if( m_bPlaySound )
					{
						int iBroadcastTeam = (pTFPlayer->GetTeamNumber() == TF_TEAM_RED) ? TF_TEAM_BLUE : TF_TEAM_RED;
						TFGameRules()->BroadcastSound( iBroadcastTeam, "Announcer.SecurityAlert" );
					}

					switch( pTFPlayer->GetTeamNumber() )
					{
					case TF_TEAM_RED:
						m_outputOnTriggeredTeam1.FireOutput( this, this );
						break;
					case TF_TEAM_BLUE:
						m_outputOnTriggeredTeam2.FireOutput( this, this );
						break;
					}

					m_flNextAlert = gpGlobals->curtime + m_iAlertDelay;
				}
			}
		}
	}

	BaseClass::StartTouch( pOther );
}