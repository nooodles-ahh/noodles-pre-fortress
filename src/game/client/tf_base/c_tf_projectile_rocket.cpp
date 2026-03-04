//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "c_tf_projectile_rocket.h"
#include "particles_new.h"
#include "tempent.h"
#include "iefx.h"
#include "dlight.h"
#include "c_te_legacytempents.h"
#include "pf_cvars.h"

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_Rocket, DT_TFProjectile_Rocket )

BEGIN_NETWORK_TABLE( C_TFProjectile_Rocket, DT_TFProjectile_Rocket )
	RecvPropBool( RECVINFO( m_bCritical ) ),
END_NETWORK_TABLE()

IMPLEMENT_NETWORKCLASS_ALIASED(TFFlameRocket, DT_TFFlameRocket)

BEGIN_NETWORK_TABLE(C_TFFlameRocket, DT_TFFlameRocket)
	RecvPropBool(RECVINFO(m_bCritical)),
END_NETWORK_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TFProjectile_Rocket::C_TFProjectile_Rocket( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TFProjectile_Rocket::~C_TFProjectile_Rocket( void )
{
	ParticleProp()->StopEmission();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFProjectile_Rocket::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged(updateType);

	if ( updateType == DATA_UPDATE_CREATED )
	{
		CreateRocketTrails();		
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFProjectile_Rocket::CreateRocketTrails( void )
{
	if ( IsDormant() )
		return;

	if ( enginetrace->GetPointContents( GetAbsOrigin() ) & MASK_WATER )
	{
		ParticleProp()->Create( "rockettrail_underwater", PATTACH_POINT_FOLLOW, "trail" );
	}
	else
	{
		ParticleProp()->Create( GetTrailParticleName(), PATTACH_POINT_FOLLOW, "trail" );
	}

	if ( m_bCritical )
	{
		switch( GetTeamNumber() )
		{
		case TF_TEAM_BLUE:
			ParticleProp()->Create( "critical_rocket_blue", PATTACH_ABSORIGIN_FOLLOW );
			break;
		case TF_TEAM_RED:
			ParticleProp()->Create( "critical_rocket_red", PATTACH_ABSORIGIN_FOLLOW );
			break;
		default:
			break;
		}
	}
}


void C_TFProjectile_Rocket::CreateLightEffects( void )
{
	// Handle the dynamic light
	if (pf_projectilelight.GetBool())
	{
		AddEffects( EF_DIMLIGHT );

		dlight_t *dl;
		if ( IsEffectActive( EF_DIMLIGHT ) )
		{	
			dl = effects->CL_AllocDlight( LIGHT_INDEX_TE_DYNAMIC + index );
			dl->flags = DLIGHT_NO_MODEL_ILLUMINATION;
			dl->origin = GetAbsOrigin();
			dl->color.r = 255;
			dl->color.g = 100;
			dl->color.b = 10;
			dl->die = gpGlobals->curtime + 0.05f;
			dl->radius = 192.f;
			dl->decay = 0.0f;
			dl->style = 1;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TFFlameRocket::C_TFFlameRocket( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TFFlameRocket::~C_TFFlameRocket( void )
{
	ParticleProp()->StopEmission();
}

void C_TFFlameRocket::CreateRocketTrails( void )
{
	if (IsDormant())
		return;

	if( m_bCritical )
	{
		ParticleProp()->Create( ConstructTeamParticle( "incendiaryrocket_crit_%s", GetTeamNumber() ), PATTACH_POINT_FOLLOW, "trail" );
	}
	else
	{
		ParticleProp()->Create( GetTrailParticleName(), PATTACH_POINT_FOLLOW, "trail" );
	}
}