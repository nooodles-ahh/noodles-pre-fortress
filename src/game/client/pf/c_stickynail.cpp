//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Implements the Sticky Bolt code. This constraints ragdolls to the world
//			after being hit by a crossbow bolt. If something here is acting funny
//			let me know - Adrian.
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_basetempentity.h"
#include "fx.h"
#include "decals.h"
#include "iefx.h"
#include "engine/IEngineSound.h"
#include "materialsystem/imaterialvar.h"
#include "IEffects.h"
#include "engine/IEngineTrace.h"
#include "vphysics/constraints.h"
#include "engine/ivmodelinfo.h"
#include "tempent.h"
#include "c_te_legacytempents.h"
#include "engine/ivdebugoverlay.h"
#include "c_te_effect_dispatch.h"
#include "tf_shareddefs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Do we need this anymore?


extern IPhysicsSurfaceProps *physprops;
IPhysicsObject *GetWorldPhysObject( void );

extern ITempEnts* tempents;

class CRagdollNailEnumerator : public IPartitionEnumerator
{
public:
	//Forced constructor   
	CRagdollNailEnumerator(Ray_t& shot, Vector vOrigin)
	{
		m_rayShot = shot;
		m_vWorld = vOrigin;
	}

	//Actual work code
	IterationRetval_t EnumElement( IHandleEntity *pHandleEntity )
	{
		C_BaseEntity *pEnt = ClientEntityList().GetBaseEntityFromHandle( pHandleEntity->GetRefEHandle() );
		if ( pEnt == NULL )
			return ITERATION_CONTINUE;

		C_BaseAnimating *pModel = static_cast< C_BaseAnimating * >( pEnt );

		if ( pModel == NULL )
			return ITERATION_CONTINUE;

		trace_t tr;
		enginetrace->ClipRayToEntity( m_rayShot, MASK_SHOT, pModel, &tr );

		IPhysicsObject	*pPhysicsObject = NULL;
		
		//Find the real object we hit.
		if( tr.physicsbone >= 0 )
		{
			if ( pModel->m_pRagdoll )
			{
				CRagdoll *pCRagdoll = dynamic_cast < CRagdoll * > ( pModel->m_pRagdoll );

				if ( pCRagdoll )
				{
					ragdoll_t *pRagdollT = pCRagdoll->GetRagdoll();

					if ( tr.physicsbone < pRagdollT->listCount )
					{
						pPhysicsObject = pRagdollT->list[tr.physicsbone].pObject;
					}
				}
			}
		}

		if ( pPhysicsObject == NULL )
			return ITERATION_CONTINUE;

		if ( tr.fraction < 1.0 )
		{
			IPhysicsObject *pReference = GetWorldPhysObject();

			if ( pReference == NULL || pPhysicsObject == NULL )
				 return ITERATION_CONTINUE;
			
			float flMass = pPhysicsObject->GetMass();
			pPhysicsObject->SetMass( flMass * 2 );

			constraint_ballsocketparams_t ballsocket;
			ballsocket.Defaults();
		
			pReference->WorldToLocal( &ballsocket.constraintPosition[0], m_vWorld );
			pPhysicsObject->WorldToLocal( &ballsocket.constraintPosition[1], tr.endpos );
	
			physenv->CreateBallsocketConstraint( pReference, pPhysicsObject, NULL, ballsocket );
	
			return ITERATION_STOP;
		}

		return ITERATION_CONTINUE;
	}

private:
	Ray_t	m_rayShot;
	Vector  m_vWorld;
};

void StickNailRagdollNow(const Vector &vecOrigin, const Vector &vecDirection, int iProjectileType, int iTeam)
{
	Ray_t	shotRay;
	trace_t tr;
	
	UTIL_TraceLine( vecOrigin, vecOrigin + vecDirection, MASK_SOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &tr );

	if ( tr.surface.flags & SURF_SKY )
		return;

	Vector vecEnd = vecOrigin - vecDirection;

	shotRay.Init( vecOrigin, vecEnd );

	CRagdollNailEnumerator	ragdollEnum(shotRay, vecOrigin);
	partition->EnumerateElementsAlongRay( PARTITION_CLIENT_RESPONSIVE_EDICTS, shotRay, false, &ragdollEnum );
	
	model_t *pModel = nullptr;

	switch (iProjectileType)
	{
	case TF_PROJECTILE_NAIL:
		pModel = (model_t *)engine->LoadModel( "models/weapons/w_models/w_nail.mdl" );
		break;
	case TF_PROJECTILE_DART:
		pModel = (model_t *)engine->LoadModel( "models/weapons/w_models/w_dart_proj.mdl" );
		break;
	default:
		pModel = (model_t *)engine->LoadModel( "models/weapons/w_models/w_syringe_proj.mdl" );
		break;
	}
	QAngle vAngles;

	VectorAngles( vecDirection, vAngles );

	C_LocalTempEntity *pTemp = tempents->SpawnTempModel( pModel, vecOrigin + vecDirection, vAngles, Vector( 0, 0, 0 ), 8.0f, FTENT_NONE | FTENT_FADEOUT );
	if (pTemp == NULL)
		return;

	pTemp->m_nSkin = (iTeam == TF_TEAM_RED) ? 0 : 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &data - 
//-----------------------------------------------------------------------------
void StickyNailCallback( const CEffectData &data )
{
	StickNailRagdollNow( data.m_vOrigin, data.m_vNormal, TF_PROJECTILE_NAIL, data.m_nMaterial );
}
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &data - 
//-----------------------------------------------------------------------------
void StickySyringeCallback(const CEffectData &data)
{
	StickNailRagdollNow( data.m_vOrigin, data.m_vNormal, TF_PROJECTILE_SYRINGE, data.m_nMaterial);
}
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &data - 
//-----------------------------------------------------------------------------
void StickyDartCallback( const CEffectData &data )
{
	StickNailRagdollNow( data.m_vOrigin, data.m_vNormal, TF_PROJECTILE_DART, data.m_nMaterial );
}

DECLARE_CLIENT_EFFECT( "NailImpact", StickyNailCallback );
DECLARE_CLIENT_EFFECT( "SyringeImpact", StickySyringeCallback );
DECLARE_CLIENT_EFFECT( "DartImpact", StickyDartCallback );
