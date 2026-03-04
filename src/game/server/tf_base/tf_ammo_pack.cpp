//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "tf_ammo_pack.h"
#include "tf_shareddefs.h"
#include "ammodef.h"
#include "tf_gamerules.h"
#include "tf_powerup.h"
#include "explode.h"
#include "tf/tf_fx.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//----------------------------------------------

// Network table.
IMPLEMENT_SERVERCLASS_ST( CTFAmmoPack, DT_AmmoPack )
	SendPropVector( SENDINFO( m_vecInitialVelocity ), -1, SPROP_NOSCALE ),
END_SEND_TABLE()

BEGIN_DATADESC( CTFAmmoPack )
	DEFINE_THINKFUNC( FlyThink ),
	DEFINE_ENTITYFUNC( PackTouch ),
END_DATADESC();

LINK_ENTITY_TO_CLASS( tf_ammo_pack, CTFAmmoPack );

PRECACHE_REGISTER( tf_ammo_pack );

void CTFAmmoPack::Spawn( void )
{
	Precache();
	SetModel( STRING( GetModelName() ) );
	BaseClass::Spawn();

	SetNextThink( gpGlobals->curtime + 0.75f );
#ifdef PF2_DLL
	if(!m_bDisintegrate)
	{
		SetThink( &CTFAmmoPack::FlyThink );
		SetTouch( &CTFAmmoPack::PackTouch );
	}
#endif
	m_flCreationTime = gpGlobals->curtime;

	// no pickup until flythink
	m_bAllowOwnerPickup = false;

	// no ammo to start
	memset( m_iAmmo, 0, sizeof(m_iAmmo) );

	// Die in 30 seconds
	SetContextThink( &CBaseEntity::SUB_Remove, gpGlobals->curtime + 30, "DieContext" );

	if ( IsX360() )
	{
		RemoveEffects( EF_ITEM_BLINK );
	}
}

void CTFAmmoPack::Precache( void )
{
}

// extra disintegrate parameter
CTFAmmoPack *CTFAmmoPack::Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, const char *pszModelName, bool bDisintegrate )
{
	CTFAmmoPack *pAmmoPack = static_cast<CTFAmmoPack*>( CBaseAnimating::CreateNoSpawn( "tf_ammo_pack", vecOrigin, vecAngles, pOwner ) );
	if ( pAmmoPack )
	{
		pAmmoPack->SetModelName( AllocPooledString( pszModelName ) );
#ifdef PF2_DLL
		pAmmoPack->Disintegrate(bDisintegrate, NULL /*set this later*/);
#endif
		DispatchSpawn( pAmmoPack );
	}

	return pAmmoPack;
}

void CTFAmmoPack::SetInitialVelocity( Vector &vecVelocity )
{ 
	m_vecInitialVelocity = vecVelocity;
}

int CTFAmmoPack::GiveAmmo( int iCount, int iAmmoType )
{
	if (iAmmoType == -1 || iAmmoType >= TF_AMMO_COUNT )
	{
		Msg("ERROR: Attempting to give unknown ammo type (%d)\n", iAmmoType);
		return 0;
	}

	m_iAmmo[iAmmoType] += iCount;

	return iCount;
}

void CTFAmmoPack::FlyThink( void )
{
	m_bAllowOwnerPickup = true;
}

void CTFAmmoPack::PackTouch( CBaseEntity *pOther )
{
	Assert( pOther );

	if( !pOther->IsPlayer() )
		return;

	if( !pOther->IsAlive() )
		return;

	//Don't let the person who threw this ammo pick it up until it hits the ground.
	//This way we can throw ammo to people, but not touch it as soon as we throw it ourselves
	if( GetOwnerEntity() == pOther && m_bAllowOwnerPickup == false )
		return;

	CTFPlayer *pPlayer = ToTFPlayer( pOther );

	Assert( pPlayer );

	bool bSuccess = false;
	int iAmmoTaken = 0;
	for (int i = 0; i < TF_AMMO_COUNT; ++i)
	{
		iAmmoTaken += pPlayer->GiveAmmo(m_iAmmo[i], i);
	}
	bSuccess = iAmmoTaken > 0;

	float flCloak = pPlayer->m_Shared.GetSpyCloakMeter();
	if( flCloak < 100.0f )
	{
		pPlayer->m_Shared.SetSpyCloakMeter( min( flCloak + ( 100.0f * PackRatios[POWERUP_MEDIUM] ), 100.0f ) );

		if( !bSuccess )
			EmitSound( "BaseCombatCharacter.AmmoPickup" );

		bSuccess = true;
	}

	if ( bSuccess )
	{
		UTIL_Remove( this );
	}
}


void CTFAmmoPack::UpdateOnRemove( )
{
	if(m_bDisintegrate)
		Explode(m_pExplosionOwner);
	BaseClass::UpdateOnRemove();
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
unsigned int CTFAmmoPack::PhysicsSolidMaskForEntity( void ) const
{ 
	return BaseClass::PhysicsSolidMaskForEntity() | CONTENTS_DEBRIS;
}

#ifdef PF2_DLL
void CTFAmmoPack::Disintegrate(bool bDisintegrate, CBaseEntity* pOwner)
{ 
	m_bDisintegrate = bDisintegrate;
	m_pExplosionOwner = pOwner;
	if(bDisintegrate)
		GetBaseAnimating()->Dissolve( "", gpGlobals->curtime, false, ENTITY_DISSOLVE_NORMAL );
}

ConVar tf_ammo_pack_explode_damage( "tf_ammo_pack_explode_damage", "80", FCVAR_GAMEDLL );
void CTFAmmoPack::Explode( CBaseEntity *pOwner )
{
	trace_t		tr;
	Vector		vecSpot;// trace starts here!

	SetThink( NULL );

	vecSpot = GetAbsOrigin() + Vector( 0, 0, 8 );
	UTIL_TraceLine( vecSpot, vecSpot + Vector( 0, 0, -32 ), MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, &tr );

	SetModelName( NULL_STRING );//invisible
	AddSolidFlags( FSOLID_NOT_SOLID );

	m_takedamage = DAMAGE_NO;

	// Pull out of the wall a bit
	if (tr.fraction != 1.0)
	{
		SetAbsOrigin( tr.endpos + ( tr.plane.normal * 1.0f ) );
	}

	// Explosion effect on client
	Vector vecOrigin = GetAbsOrigin();
	CPVSFilter filter( vecOrigin );
	TE_TFExplosion( filter, 0.0f, vecOrigin, tr.plane.normal, TF_WEAPON_NONE, tr.GetEntityIndex() );

	float flDamage = tf_ammo_pack_explode_damage.GetFloat(); // replace with how much ammo we actually have later

	// Use the thrower's position as the reported position
	Vector vecReported = pOwner ? pOwner->GetAbsOrigin() : vec3_origin;
	CTakeDamageInfo info( this, pOwner, vec3_origin, GetAbsOrigin(), flDamage, DMG_BLAST | DMG_HALF_FALLOFF, 0, &vecReported );
	RadiusDamage( info, vecOrigin, flDamage, CLASS_NONE, NULL );

	// Don't decal players with scorch.
	if (tr.m_pEnt && !tr.m_pEnt->IsPlayer())
	{
		UTIL_DecalTrace( &tr, "Scorch" );
	}

	SetTouch( NULL );

	AddEffects( EF_NODRAW );
	SetAbsVelocity( vec3_origin );
	SetNextThink( gpGlobals->curtime );

	if (GetShakeAmplitude())
	{
		UTIL_ScreenShake( GetAbsOrigin(), GetShakeAmplitude(), 150.0, 1.0, GetShakeRadius(), SHAKE_START );
	}
}
#endif