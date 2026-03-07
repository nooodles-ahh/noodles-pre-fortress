//=============================================================================
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#ifndef C_TEAM_CONTROL_POINT_H
#define C_TEAM_CONTROL_POINT_H
#ifdef _WIN32
#pragma once
#endif
#include "cbase.h"
#include "tf_shareddefs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern CUtlVector<int> g_TeamControlPoints;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_TeamControlPoint : public C_BaseAnimating
{
	DECLARE_CLASS( C_TeamControlPoint, C_BaseAnimating );

public:
	DECLARE_CLIENTCLASS();
	void Spawn( void );

	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual bool	Interpolate( float currentTime );
	virtual void	Simulate(void);
	float			GetTeamCapPercentage( int iTeam );
	virtual CStudioHdr *OnNewModel( void );

	bool IsActive( void );
	int GetOwningTeam( void );
	int GetPointIndex( void );

private:
	struct teamdata_t
	{
		int iTeamPoseParam;
		int iModelBodygroup;
		float captureProgress;
		float captureProgressLerped;
	};
	CUtlVector<teamdata_t>	m_TeamData;

	bool m_bActive;
	int m_iPointIndex;
	int m_iTeam;

};
#endif