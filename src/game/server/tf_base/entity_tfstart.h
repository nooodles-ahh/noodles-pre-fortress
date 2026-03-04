//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: CTF Spawn Point.
//
//=============================================================================//
#ifndef ENTITY_TFSTART_H
#define ENTITY_TFSTART_H

#ifdef _WIN32
#pragma once
#endif

class CTeamControlPoint;
class CTeamControlPointRound;

//=============================================================================
//
// TF team spawning entity.
//

class CTFTeamSpawn : public CPointEntity
{
public:
	DECLARE_CLASS( CTFTeamSpawn, CPointEntity );

	CTFTeamSpawn();

	void Activate( void );

	bool IsDisabled( void ) { return m_bDisabled; }
	void SetDisabled( bool bDisabled ) { m_bDisabled = bDisabled; }

	// Inputs/Outputs.
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );
	void InputRoundSpawn( inputdata_t &inputdata );
#ifdef PF2_DLL
	void InputArmorSpawn( inputdata_t& inputdata );
	void InputGrenadeSpawn( inputdata_t& inputdata );
#endif
	int DrawDebugTextOverlays(void);

	CHandle<CTeamControlPoint> GetControlPoint( void ) { return m_hControlPoint; }
	CHandle<CTeamControlPointRound> GetRoundBlueSpawn( void ) { return m_hRoundBlueSpawn; }
	CHandle<CTeamControlPointRound> GetRoundRedSpawn( void ) { return m_hRoundRedSpawn; }

#ifdef PF2_DLL
	bool SpawnArmor( void ) { return m_bSpawnArmor; }
	bool SpawnGrenades( void ) { return m_bSpawnGrenades; }
#endif

private:
	bool	m_bDisabled;		// Enabled/Disabled?
#ifdef PF2_DLL
	bool	m_bSpawnArmor;
	bool	m_bSpawnGrenades;
#endif

	string_t						m_iszControlPointName;
	string_t						m_iszRoundBlueSpawn;
	string_t						m_iszRoundRedSpawn;



	CHandle<CTeamControlPoint>		m_hControlPoint;
	CHandle<CTeamControlPointRound>	m_hRoundBlueSpawn;
	CHandle<CTeamControlPointRound>	m_hRoundRedSpawn;

	DECLARE_DATADESC();
};

#endif // ENTITY_TFSTART_H


