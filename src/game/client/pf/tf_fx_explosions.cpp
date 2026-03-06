//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: Game-specific explosion effects
//
//=============================================================================//
#include "cbase.h"
#include "c_te_effect_dispatch.h"
#include "tempent.h"
#include "c_te_legacytempents.h"
#include "tf_shareddefs.h"
#include "engine/IEngineSound.h"
#include "tf_weapon_parse.h"
#include "c_basetempentity.h"
#include "tier0/vprof.h"
#include "dlight.h"
#include "iefx.h"
#include "pf_cvars.h"
#include <tempentity.h>
#include "ragdollexplosionenumerator.h"

//--------------------------------------------------------------------------------------------------------------
extern CTFWeaponInfo *GetTFWeaponInfo( int iWeapon );

ConVar pf_particle_explosions( "pf_particle_explosions", "0", FCVAR_CLIENTDLL | FCVAR_DONTRECORD | FCVAR_ARCHIVE );
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TFExplosionCallback(const Vector& vecOrigin, const Vector& vecNormal, int iWeaponID, ClientEntityHandle_t hEntity)
{
	// Get the weapon information.
	CTFWeaponInfo* pWeaponInfo = NULL;
	switch (iWeaponID)
	{
	case TF_WEAPON_GRENADE_PIPEBOMB:
	case TF_WEAPON_GRENADE_DEMOMAN:
		pWeaponInfo = GetTFWeaponInfo(TF_WEAPON_PIPEBOMBLAUNCHER);
		break;
	case TF_WEAPON_GRENADE_MIRV:
	case TF_WEAPON_GRENADE_MIRVBOMB:
		pWeaponInfo = GetTFWeaponInfo(TF_WEAPON_GRENADE_MIRV);
		break;
	case TF_WEAPON_FLAMETHROWER:
	case TF_WEAPON_FLAMETHROWER_ROCKET:
		pWeaponInfo = GetTFWeaponInfo( TF_WEAPON_FLAMETHROWER );
		break;
	default:
		pWeaponInfo = GetTFWeaponInfo(iWeaponID);
		break;
	}

	bool bIsPlayer = false;
	if (hEntity.Get())
	{
		C_BaseEntity* pEntity = C_BaseEntity::Instance(hEntity);
		if (pEntity && pEntity->IsPlayer())
		{
			bIsPlayer = true;
		}
	}

	// Calculate the angles, given the normal.
	bool bIsWater = (UTIL_PointContents(vecOrigin) & CONTENTS_WATER);
	bool bInAir = false;
	QAngle angExplosion(0.0f, 0.0f, 0.0f);

	// Cannot use zeros here because we are sending the normal at a smaller bit size.
	if (fabs(vecNormal.x) < 0.05f && fabs(vecNormal.y) < 0.05f && fabs(vecNormal.z) < 0.05f)
	{
		bInAir = true;
		angExplosion.Init();
	}
	else
	{
		VectorAngles(vecNormal, angExplosion);
		bInAir = false;
	}

	// Base explosion effect and sound.
	char* pszEffect = "explosion";
	char* pszSound = "BaseExplosionEffect.Sound";

	float flRadius = 121.0f;
	float flDamage = 100.f;

	if (pWeaponInfo)
	{
		// Explosions.
		if (bIsWater)
		{
			if (Q_strlen(pWeaponInfo->m_szExplosionWaterEffect) > 0)
			{
				pszEffect = pWeaponInfo->m_szExplosionWaterEffect;
			}
		}
		else
		{
			if (bIsPlayer || bInAir)
			{
				if (Q_strlen(pWeaponInfo->m_szExplosionPlayerEffect) > 0)
				{
					pszEffect = pWeaponInfo->m_szExplosionPlayerEffect;
				}
			}
			else
			{
				if (Q_strlen(pWeaponInfo->m_szExplosionEffect) > 0)
				{
					pszEffect = pWeaponInfo->m_szExplosionEffect;
				}
			}
		}

		// Sound.
		if (Q_strlen(pWeaponInfo->m_szExplosionSound) > 0)
		{
			pszSound = pWeaponInfo->m_szExplosionSound;
		}

		// Sad I have to even check this
		if( pWeaponInfo->m_flDamageRadius != 0.0f )
			flRadius = pWeaponInfo->m_flDamageRadius;

		// special case for the flamerocket
		flDamage = pWeaponInfo->GetWeaponDamage( iWeaponID == TF_WEAPON_FLAMETHROWER_ROCKET );
	}

	CLocalPlayerFilter filter;
	C_BaseEntity::EmitSound(filter, SOUND_FROM_WORLD, pszSound, &vecOrigin);

	if (pf_muzzlelight.GetBool())
	{
		dlight_t* dl = effects->CL_AllocDlight(LIGHT_INDEX_TE_DYNAMIC);

		dl->origin = vecOrigin;
		dl->flags = DLIGHT_NO_MODEL_ILLUMINATION;
		dl->decay = 200;
		dl->radius = 255;
		dl->color.r = 255;
		dl->color.g = 100;
		dl->color.b = 10;
		dl->die = gpGlobals->curtime + 0.1f;
	}

	// Cyanide; cus it looks funny
 	CRagdollExplosionEnumerator	ragdollEnum( vecOrigin, flRadius, flDamage );
	partition->EnumerateElementsInSphere( PARTITION_CLIENT_RESPONSIVE_EDICTS, vecOrigin, flRadius, false, &ragdollEnum );

	if( !pf_particle_explosions.GetBool() )
	{
		CEffectData	data;

		data.m_vOrigin = vecOrigin;
		data.m_flMagnitude = flDamage;
		data.m_flScale = 1.0f;
		data.m_fFlags = TE_EXPLFLAG_NOSOUND | TE_EXPLFLAG_ROTATE;

		DispatchEffect( "Explosion", data );
	}
	else
	{
		DispatchParticleEffect( pszEffect, vecOrigin, angExplosion );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_TETFExplosion : public C_BaseTempEntity
{
public:

	DECLARE_CLASS( C_TETFExplosion, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

	C_TETFExplosion( void );

	virtual void	PostDataUpdate( DataUpdateType_t updateType );

public:

	Vector		m_vecOrigin;
	Vector		m_vecNormal;
	int			m_iWeaponID;
	ClientEntityHandle_t m_hEntity;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TETFExplosion::C_TETFExplosion( void )
{
	m_vecOrigin.Init();
	m_vecNormal.Init();
	m_iWeaponID = TF_WEAPON_NONE;
	m_hEntity = INVALID_EHANDLE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TETFExplosion::PostDataUpdate( DataUpdateType_t updateType )
{
	VPROF( "C_TETFExplosion::PostDataUpdate" );

	TFExplosionCallback( m_vecOrigin, m_vecNormal, m_iWeaponID, m_hEntity );
}

static void RecvProxy_ExplosionEntIndex( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	int nEntIndex = pData->m_Value.m_Int;
	((C_TETFExplosion*)pStruct)->m_hEntity = (nEntIndex < 0) ? INVALID_EHANDLE : ClientEntityList().EntIndexToHandle( nEntIndex );
}

IMPLEMENT_CLIENTCLASS_EVENT_DT( C_TETFExplosion, DT_TETFExplosion, CTETFExplosion )
	RecvPropFloat( RECVINFO( m_vecOrigin[0] ) ),
	RecvPropFloat( RECVINFO( m_vecOrigin[1] ) ),
	RecvPropFloat( RECVINFO( m_vecOrigin[2] ) ),
	RecvPropVector( RECVINFO( m_vecNormal ) ),
	RecvPropInt( RECVINFO( m_iWeaponID ) ),
	RecvPropInt( "entindex", 0, SIZEOF_IGNORE, 0, RecvProxy_ExplosionEntIndex ),
END_RECV_TABLE()

