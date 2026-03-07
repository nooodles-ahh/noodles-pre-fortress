//=============================================================================
//
// Purpose: TODO TODO TODO TODO
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "c_baseanimating.h"
#include "c_team.h"
#include "c_team_control_point.h"
#include "c_team_objectiveresource.h"
#include "tf_gamerules.h"
#include "tf_shareddefs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_CLIENTCLASS_DT( C_TeamControlPoint, DT_TeamControlPoint, CTeamControlPoint )
	RecvPropBool( RECVINFO( m_bActive ) ),
	RecvPropInt( RECVINFO( m_iPointIndex ) ),
	RecvPropInt( RECVINFO( m_iTeam ) ),
END_RECV_TABLE()

void C_TeamControlPoint::Spawn( void )
{
	// add this element if it isn't already in the list
	if( g_TeamControlPoints.Find( entindex() ) == -1 )
	{
		g_TeamControlPoints.AddToTail( entindex() );
	}
	m_TeamData.SetSize( g_Teams.Size() );

	for ( int i = 0; i < m_TeamData.Count(); ++i )
	{
		m_TeamData[i].captureProgress = 0.f;
		m_TeamData[i].captureProgressLerped = 0.f;
	}

	SetSimulatedEveryTick(true);
	UseClientSideAnimation();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void C_TeamControlPoint::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );
	
	if( updateType == DATA_UPDATE_CREATED )
	{
		for ( int i = 0; i < m_TeamData.Count(); ++i )
		{
			m_TeamData[i].captureProgress = GetTeamCapPercentage( i );
			m_TeamData[i].captureProgressLerped = m_TeamData[i].captureProgress;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : currentTime - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_TeamControlPoint::Interpolate( float currentTime )
{
	return BaseClass::Interpolate( currentTime );
}

void C_TeamControlPoint::Simulate()
{
	BaseClass::Simulate();

	for( int i = LAST_SHARED_TEAM + 1; i < m_TeamData.Count(); i++ )
	{
		// Skip spectator
		if( i == TEAM_SPECTATOR )
			continue;

		m_TeamData[i].captureProgress = GetTeamCapPercentage( i );

		const float flDecay = 10.0f;
		m_TeamData[i].captureProgressLerped = Lerp(
			1.0f - expf( -flDecay * gpGlobals->frametime ),
			m_TeamData[i].captureProgressLerped,
			m_TeamData[i].captureProgress
		);

		float flPerc = m_TeamData[ i ].captureProgressLerped;

		if( m_TeamData[ i ].iTeamPoseParam != -1 )
		{
			SetPoseParameter( m_TeamData[ i ].iTeamPoseParam, flPerc );
		}

		if( m_TeamData[ i ].iModelBodygroup != -1 )
		{
			SetBodygroup( m_TeamData[ i ].iModelBodygroup, 0.0f == m_TeamData[ i ].captureProgress );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float C_TeamControlPoint::GetTeamCapPercentage( int iTeam )
{
	int iCappingTeam = ObjectiveResource()->GetCappingTeam( GetPointIndex() );
	if( iCappingTeam == TEAM_UNASSIGNED )
	{
		// No-one's capping this point.
		if( iTeam == m_iTeam )
			return 1.0;

		return 0.0;
	}

	// Cyanide; Ignore the cap percentage on payload because I don't really want to fix
	// the fundamental issue which is that there is a capture time for some reason
	if( TFGameRules()->GetGameType() == TF_GAMETYPE_ESCORT )
	{
		if( iTeam == m_iTeam )
			return 1.0;
	}
	else
	{
		float flCapPerc = ObjectiveResource()->GetCPCapPercentage( GetPointIndex() );
		if( iTeam == iCappingTeam )
			return flCapPerc;
		if( iTeam == m_iTeam )
			return ( 1.0 - flCapPerc );
	}

	return 0.0;
}

CStudioHdr * C_TeamControlPoint::OnNewModel( void )
{
	CStudioHdr *hdr = BaseClass::OnNewModel();

	for( int i = 0; i < m_TeamData.Count(); i++ )
	{
		if( GetModelPtr() && GetModelPtr()->SequencesAvailable() )
		{
			m_TeamData[ i ].iTeamPoseParam = LookupPoseParameter( VarArgs( "cappoint_%d_percentage", i ) );
			m_TeamData[ i ].iModelBodygroup = FindBodygroupByName( VarArgs( "cappoint_%d_bodygroup", i ) );
		}
		else
		{
			m_TeamData[ i ].iTeamPoseParam = -1;
		}
	}

	ResetSequence( LookupSequence( "idle" ) );

	return hdr;
}

bool C_TeamControlPoint::IsActive( void )
{
	return m_bActive;
}
int C_TeamControlPoint::GetOwningTeam( void )
{
	return m_iTeam;
}

int C_TeamControlPoint::GetPointIndex( void )
{
	return m_iPointIndex;
}

LINK_ENTITY_TO_CLASS( team_control_point, C_TeamControlPoint );

