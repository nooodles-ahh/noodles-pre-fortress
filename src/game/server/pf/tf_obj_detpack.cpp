
#include "cbase.h"
#include "tf_obj_detpack.h"
#include "tf_player.h"
#include "explode.h"
#include "tf_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar tf_obj_detpack_damage( "tf_obj_detpack_damage", "300", FCVAR_DEVELOPMENTONLY );
ConVar tf_obj_detpack_radius( "tf_obj_detpack_radius", "300", FCVAR_DEVELOPMENTONLY );
ConVar tf_obj_detpack_show_radius( "tf_obj_detpack_show_radius", "0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
//ConVar tf_obj_detpack_radius_wall( "tf_obj_detpack_radius_player", "600", FCVAR_DEVELOPMENTONLY );

#define DETPACK_MAX_HEALTH			100
#define DETPACK_MODEL				"models/buildables/detpack.mdl"	//TODO: Get ourselves a detpack model
#define DETPACK_MODEL_PLACEMENT		"models/buildables/detpack_blueprint.mdl"


IMPLEMENT_SERVERCLASS_ST(CObjectDetpack, DT_TFObjectDetpack)
////TODO: Countdown stuff sent to client, maybe?
END_SEND_TABLE()

BEGIN_DATADESC( CObjectDetpack )
DEFINE_THINKFUNC( DetpackThink ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( obj_detpack, CObjectDetpack );
PRECACHE_REGISTER( obj_detpack );

CObjectDetpack::CObjectDetpack()
{
	SetMaxHealth( DETPACK_MAX_HEALTH );
	m_iHealth = DETPACK_MAX_HEALTH;

	m_takedamage = DAMAGE_NO;
	SetType( OBJ_DETPACK );
}

CObjectDetpack::~CObjectDetpack()
{
	//StopSound( "misc/teleport_ready" );
}

void CObjectDetpack::Spawn( void )
{
	BaseClass::Spawn();

	SetSolid( SOLID_BBOX );
	SetModel( DETPACK_MODEL_PLACEMENT );

	//m_takedamage = DAMAGE_YES;
}

void CObjectDetpack::Precache( void )
{
	PrecacheScriptSound( "Weapon_Grenade_Det_Pack.Plant" );
	PrecacheScriptSound( "Weapon_Grenade_Det_Pack.Explode" );
	PrecacheScriptSound( "Weapon_Grenade_Det_Pack.Timer" );

	PrecacheModel( DETPACK_MODEL );
	PrecacheModel( DETPACK_MODEL_PLACEMENT );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Start building the object
//-----------------------------------------------------------------------------
bool CObjectDetpack::StartBuilding( CBaseEntity *pBuilder )
{
	SetModel( DETPACK_MODEL );

	CreateBuildPoints();

	CTFPlayer* pTFPlayer = ToTFPlayer( pBuilder );
	if ( pTFPlayer )
	{
		m_vecBuilderCoords = pTFPlayer->GetAbsOrigin();
		pTFPlayer->m_Shared.AddCond( TF_COND_BUILDING_DETPACK );
	}
	else
	{
		m_vecBuilderCoords = GetAbsOrigin();
	}

	return BaseClass::StartBuilding( pBuilder );
}

//-----------------------------------------------------------------------------
// Purpose: Finished building
//-----------------------------------------------------------------------------
void CObjectDetpack::OnGoActive( void )
{
	CTFPlayer *pBuilder = GetBuilder();

	Assert( pBuilder );
	
	if (!pBuilder)
		return;

	pBuilder->m_Shared.RemoveCond( TF_COND_BUILDING_DETPACK );
	pBuilder->m_Shared.SetDetpackUsed( true ); // Move this to StartBuilding once we've made sure the distance check is fair 

	SetModel( DETPACK_MODEL );
	
	//disable damage after build time
	m_takedamage = DAMAGE_NO;

	// Begin thinking
	switch (m_iMode)
	{
	case 1:
		SetDetTime( 20.0f );
		break;
	case 2:
		SetDetTime( 50.0f );
		break;
	case 0:
	default:
		SetDetTime( 5.0f );
	}
	SetThink( &CObjectDetpack::DetpackThink );
	SetNextThink( gpGlobals->curtime );

	EmitSound( "misc/teleport_ready" ); //replace me

	BaseClass::OnGoActive();
}

//-----------------------------------------------------------------------------
// Purpose: Continue construction of this object
//-----------------------------------------------------------------------------
void CObjectDetpack::BuildingThink( void )
{
	BaseClass::BuildingThink();

	CTFPlayer* pBuilder = GetBuilder();
	if ( !pBuilder )
		return;

	if ( !pBuilder->IsAlive() || !pBuilder->GetGroundEntity())
	{
		pBuilder->m_Shared.RemoveCond( TF_COND_BUILDING_DETPACK );
		Explode();
		UTIL_Remove( this );
	}

	// Check if the builder has moved
	if ( m_vecBuilderCoords == pBuilder->GetAbsOrigin() )
		return;

	CBaseEntity* pEntity = NULL;
	const int MASK_RADIUS_DAMAGE = MASK_SHOT & ( ~CONTENTS_HITBOX );
	trace_t		tr;
	Vector		vecSpot;
	Vector vecSrc = GetAbsOrigin();

	// Check if the detpack can 'see' the builder.
	vecSpot = pBuilder->BodyTarget( vecSrc, false );
	bool bLostBuilder = false;
	for ( CEntitySphereQuery sphere( vecSrc, 120 ); ( pEntity = sphere.GetCurrentEntity() ) != NULL; sphere.NextEntity() )
	{
		CTFPlayer* pTFPlayer = ToTFPlayer( pEntity );
		if ( !pEntity )
			continue;

		if ( pTFPlayer == pBuilder )
		{
			UTIL_TraceLine( vecSrc, vecSpot, MASK_RADIUS_DAMAGE, this, COLLISION_GROUP_DEBRIS, &tr );
			if ( tr.fraction != 1.0 && tr.m_pEnt != pBuilder )
			{
				bLostBuilder = true;
			}
			break;
		}
		bLostBuilder = true;
	}
	if( bLostBuilder )
	{
		pBuilder->m_Shared.RemoveCond( TF_COND_BUILDING_DETPACK );
		Explode();
		UTIL_Remove( this );
	}
}

void CObjectDetpack::DetpackThink( void )
{
	if ( gpGlobals->curtime >= (m_flDetTime - 5.0f) && gpGlobals->curtime > m_flNextBeep)
	{
		//the percent complete of the bomb timer
		float fComplete = ( ( m_flDetTime - gpGlobals->curtime ) / m_flSetTime );
		fComplete = clamp( fComplete, 0.0f, 1.0f );
		float freq = max( 0.1 + 0.9 * fComplete, 0.15 );

		m_flNextBeep = gpGlobals->curtime + freq;
		EmitSound( "Weapon_Grenade_Det_Pack.Timer" );
	}

	//for now, just make sure we detonate on time
	if (gpGlobals->curtime < m_flDetTime)
	{
		SetNextThink( gpGlobals->curtime + 0.1 );
		return;
	}

	DetonateObject();
}

// KABOOM!
void CObjectDetpack::DetonateObject( void )
{
	ExplosionCreate(
		GetAbsOrigin(),
		GetAbsAngles(),
		GetBuilder(),
		tf_obj_detpack_damage.GetFloat(),
		tf_obj_detpack_radius.GetFloat(),
		0,
		tf_obj_detpack_radius.GetFloat(),
		this,
		DMG_BLAST
	);

	if (tf_obj_detpack_show_radius.GetBool())
		DrawRadius( tf_obj_detpack_radius.GetFloat() );

	//Get rid of this when it explodes
	UTIL_Remove( this );
}

void CObjectDetpack::SetModel( const char *pModel )
{
	BaseClass::SetModel( pModel );
}

void CObjectDetpack::DrawRadius( float flRadius )
{
	Vector pos = GetAbsOrigin();
	int r = 255;
	int g = 0, b = 0;
	float flLifetime = 5.0f;
	bool bDepthTest = true;

	Vector edge, lastEdge;
	NDebugOverlay::Line( pos, pos + Vector( 0, 0, 50 ), r, g, b, !bDepthTest, flLifetime );

	lastEdge = Vector( flRadius + pos.x, pos.y, pos.z );
	float angle;
	for (angle = 0.0f; angle <= 360.0f; angle += 22.5f)
	{
		edge.x = flRadius * cos( angle ) + pos.x;
		edge.y = pos.y;
		edge.z = flRadius * sin( angle ) + pos.z;

		NDebugOverlay::Line( edge, lastEdge, r, g, b, !bDepthTest, flLifetime );

		lastEdge = edge;
	}

	lastEdge = Vector( pos.x, flRadius + pos.y, pos.z );
	for (angle = 0.0f; angle <= 360.0f; angle += 22.5f)
	{
		edge.x = pos.x;
		edge.y = flRadius * cos( angle ) + pos.y;
		edge.z = flRadius * sin( angle ) + pos.z;

		NDebugOverlay::Line( edge, lastEdge, r, g, b, !bDepthTest, flLifetime );

		lastEdge = edge;
	}

	lastEdge = Vector( pos.x, flRadius + pos.y, pos.z );
	for (angle = 0.0f; angle <= 360.0f; angle += 22.5f)
	{
		edge.x = flRadius * cos( angle ) + pos.x;
		edge.y = flRadius * sin( angle ) + pos.y;
		edge.z = pos.z;

		NDebugOverlay::Line( edge, lastEdge, r, g, b, !bDepthTest, flLifetime );

		lastEdge = edge;
	}
}
