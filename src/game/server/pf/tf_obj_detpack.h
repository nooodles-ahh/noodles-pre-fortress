#ifndef TF_OBJ_DETPACK_H
#define TF_OBJ_DETPACK_H
#ifdef WIN32
#pragma once
#endif

#include "tf_obj.h"

class CObjectDetpack : public CBaseObject
{
	DECLARE_CLASS( CObjectDetpack, CBaseObject );
public:
	DECLARE_SERVERCLASS();

	CObjectDetpack();
	~CObjectDetpack();

	virtual void Spawn();
	virtual void Precache();
	virtual bool StartBuilding( CBaseEntity *pBuilder );
	virtual void BuildingThink( void );
	void SetDetTime( float flDetTime ) 
	{ 
		m_flDetTime = gpGlobals->curtime + flDetTime; 
		m_flSetTime = flDetTime;

	}
	virtual void SetMode( int iMode ) { m_iMode = iMode; }

	virtual void OnGoActive();
	virtual void SetModel( const char *pModel );

	void DetpackThink( void );
	virtual void DetonateObject( void );

	void DrawRadius( float flRadius );
private:
	DECLARE_DATADESC();
	float m_flDetTime;
	float m_flSetTime;
	int	  m_iMode;
	Vector m_vecBuilderCoords;

	float m_flNextBeep;
};

#endif //TF_OBJ_DETPACK_H