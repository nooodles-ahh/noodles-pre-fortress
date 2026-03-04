//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======
//
// TF Nail
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_tranq.h"
#include "tf_projectile_nail.h"

#ifdef CLIENT_DLL
#include "c_basetempentity.h"
#include "c_te_legacytempents.h"
#include "c_te_effect_dispatch.h"
#include "input.h"
#include "c_tf_player.h"
#include "cliententitylist.h"
#else
#include "tf_player.h"
#endif

#define SYRINGE_MODEL				"models/weapons/w_models/w_syringe_proj.mdl"
#define SYRINGE_DISPATCH_EFFECT		"ClientProjectile_Syringe"

#define NAIL_MODEL				"models/weapons/w_models/w_nail.mdl"
#define NAIL_DISPATCH_EFFECT	"ClientProjectile_Nail"

#define DART_MODEL				"models/weapons/w_models/w_dart_proj.mdl"
#define DART_DISPATCH_EFFECT	"ClientProjectile_Dart"

//=============================================================================
//
// TF Syringe Projectile functions (Server specific).
//

LINK_ENTITY_TO_CLASS(tf_projectile_syringe, CTFProjectile_Syringe);
PRECACHE_REGISTER(tf_projectile_syringe);

short g_sModelIndexSyringe;
void PrecacheSyringe(void* pUser)
{
	g_sModelIndexSyringe = modelinfo->GetModelIndex(SYRINGE_MODEL);
}

PRECACHE_REGISTER_FN(PrecacheSyringe);

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFProjectile_Syringe* CTFProjectile_Syringe::Create(const Vector& vecOrigin, const QAngle& vecAngles, CBaseEntity* pOwner, CBaseEntity* pScorer, bool bCritical, float flVelocity, float flGravity)
{
	return static_cast<CTFProjectile_Syringe*>(CTFBaseProjectile::Create("tf_projectile_syringe", vecOrigin, vecAngles, pOwner, flVelocity, g_sModelIndexSyringe, SYRINGE_DISPATCH_EFFECT, pScorer, bCritical, flGravity));
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char* CTFProjectile_Syringe::GetProjectileModelName(void)
{
	return SYRINGE_MODEL;
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char* GetSyringeTrailParticleName(int iTeamNumber, bool bCritical)
{
	if (iTeamNumber == TF_TEAM_BLUE)
		return (bCritical ? "nailtrails_medic_blue_crit" : "nailtrails_medic_blue");
	else
		return (bCritical ? "nailtrails_medic_red_crit" : "nailtrails_medic_red");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientsideProjectileSyringeCallback(const CEffectData & data)
{
	// Get the syringe and add it to the client entity list, so we can attach a particle system to it.
	C_BaseEntity* pEntity = ClientEntityList().GetBaseEntityFromHandle(data.m_hEntity);
	if (pEntity)
	{
		C_LocalTempEntity* pSyringe = ClientsideProjectileCallback(data);
		if (pSyringe)
		{
			pSyringe->m_nSkin = (pEntity->GetTeamNumber() == TF_TEAM_RED) ? 0 : 1;
			bool bCritical = ((data.m_nDamageType & DMG_CRITICAL) != 0);
			pSyringe->AddParticleEffect(GetSyringeTrailParticleName(pEntity->GetTeamNumber(), bCritical));
			pSyringe->AddEffects(EF_NOSHADOW);
			pSyringe->flags |= FTENT_USEFASTCOLLISIONS;
		}
	}
}

DECLARE_CLIENT_EFFECT( SYRINGE_DISPATCH_EFFECT, ClientsideProjectileSyringeCallback );

#endif

//=============================================================================
//
// TF Nail Projectile functions (Server specific).
//

LINK_ENTITY_TO_CLASS(tf_projectile_nail, CTFProjectile_Nail);
PRECACHE_REGISTER(tf_projectile_nail);

short g_sModelIndexNail;
void PrecacheNail(void* pUser)
{
	g_sModelIndexNail = modelinfo->GetModelIndex(NAIL_MODEL);
}

PRECACHE_REGISTER_FN(PrecacheNail);

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFProjectile_Nail* CTFProjectile_Nail::Create(const Vector & vecOrigin, const QAngle & vecAngles, CBaseEntity * pOwner, CBaseEntity * pScorer, bool bCritical, float flVelocity, float flGravity)
{
	return static_cast<CTFProjectile_Nail*>(CTFBaseProjectile::Create("tf_projectile_nail", vecOrigin, vecAngles, pOwner, flVelocity, g_sModelIndexNail, NAIL_DISPATCH_EFFECT, pScorer, bCritical, flGravity));
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char* CTFProjectile_Nail::GetProjectileModelName(void)
{
	return NAIL_MODEL;
}

#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char* GetNailTrailParticleName(int iTeamNumber, bool bCritical)
{
	if (iTeamNumber == TF_TEAM_BLUE)
		return (bCritical ? "nailtrails_medic_blue_crit" : "nailtrails_medic_blue");
	else
		return (bCritical ? "nailtrails_medic_red_crit" : "nailtrails_medic_red");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientsideProjectileNailCallback( const CEffectData &data )
{
	// Get the nail and add it to the client entity list, so we can attach a particle system to it.
	C_BaseEntity* pEntity = ClientEntityList().GetBaseEntityFromHandle(data.m_hEntity);
	if (pEntity)
	{
		C_LocalTempEntity* pNail = ClientsideProjectileCallback(data);
		if (pNail)
		{
			bool bCritical = ((data.m_nDamageType & DMG_CRITICAL) != 0);
			pNail->AddParticleEffect(GetNailTrailParticleName(pEntity->GetTeamNumber(), bCritical));
			pNail->AddEffects( EF_NOSHADOW );
			pNail->flags |= FTENT_USEFASTCOLLISIONS;
		}
	}
}

DECLARE_CLIENT_EFFECT( NAIL_DISPATCH_EFFECT, ClientsideProjectileNailCallback );

#endif

//=============================================================================
//
// TF Dart Projectile functions (Server specific).
//

LINK_ENTITY_TO_CLASS( tf_projectile_dart, CTFProjectile_Dart );
PRECACHE_REGISTER( tf_projectile_dart );

short g_sModelIndexDart;
void PrecacheDart( void* pUser )
{
	g_sModelIndexDart = modelinfo->GetModelIndex( DART_MODEL );
}

PRECACHE_REGISTER_FN( PrecacheDart );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFProjectile_Dart* CTFProjectile_Dart::Create( const Vector & vecOrigin, const QAngle & vecAngles, CBaseEntity * pOwner, CBaseEntity * pScorer, bool bCritical, float flVelocity, float flGravity )
{
	return static_cast< CTFProjectile_Dart* >( CTFBaseProjectile::Create( "tf_projectile_dart", vecOrigin, vecAngles, pOwner, flVelocity, g_sModelIndexDart, DART_DISPATCH_EFFECT, pScorer, bCritical, flGravity ) );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char* CTFProjectile_Dart::GetProjectileModelName( void )
{
	return DART_MODEL;
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char* GetDartTrailParticleName( int iTeamNumber, bool bCritical )
{
	if (iTeamNumber == TF_TEAM_BLUE)
		return ( bCritical ? "tranq_crit_blue" : "tranq_tracer_teamcolor_blue" );
	else
		return ( bCritical ? "tranq_crit_red" : "tranq_tracer_teamcolor_red" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientsideProjectileDartCallback( const CEffectData& data )
{
	// Get the dart and add it to the client entity list, so we can attach a particle system to it.
	C_BaseEntity* pEntity = ClientEntityList().GetBaseEntityFromHandle( data.m_hEntity );
	if ( pEntity )
	{
		C_LocalTempEntity* pDart = ClientsideProjectileCallback( data );
		if ( pDart )
		{
			pDart->m_nSkin = ( pEntity->GetTeamNumber() == TF_TEAM_RED ) ? 0 : 1;
			bool bCritical = ( ( data.m_nDamageType & DMG_CRITICAL ) != 0 );
			pDart->AddParticleEffect( GetDartTrailParticleName( pEntity->GetTeamNumber(), bCritical ) );
			pDart->AddEffects( EF_NOSHADOW );
			pDart->flags |= FTENT_USEFASTCOLLISIONS;
		}
	}
}

DECLARE_CLIENT_EFFECT( DART_DISPATCH_EFFECT, ClientsideProjectileDartCallback );
#endif