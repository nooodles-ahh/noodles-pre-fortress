//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. ========//
//
// Purpose: 
//
//=============================================================================//
#include "cbase.h"
#include "tf_weaponbase_grenadeproj.h"
#include "tf_gamerules.h"
#ifdef GAME_DLL
#include "explode.h"
#endif // GAME_DLL


// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
// Server specific.
#else
#include "soundent.h"
#include "te_effect_dispatch.h"
#include "tf_player.h"
#include "func_break.h"
#include "func_nogrenades.h"
#include "func_respawnroom.h"
#include "Sprite.h"
#include "tf_fx.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar sv_gravity;

#ifdef GAME_DLL
static string_t s_iszTrainName;
#endif


//=============================================================================
//
// TF Grenade projectile tables.
//

// Server specific.
#ifdef GAME_DLL
BEGIN_DATADESC( CTFWeaponBaseGrenadeProj )
DEFINE_THINKFUNC( DetonateThink ),
DEFINE_THINKFUNC(BeepThink),
END_DATADESC()

ConVar tf_grenade_show_radius( "tf_grenade_show_radius", "0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Render radius of grenades" );
ConVar tf_grenade_show_radius_time( "tf_grenade_show_radius_time", "5.0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Time to show grenade radius" );
extern void SendProxy_Origin( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
extern void SendProxy_Angles( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );

ConVar pf_grenades_disarm_shots( "pf_grenades_disarm_shots", "1", FCVAR_NOTIFY );
ConVar pf_grenades_tfc("pf_grenades_tfc", "1", FCVAR_NOTIFY);
ConVar pf_grenades_bonk( "pf_grenades_bonk", "1", FCVAR_NOTIFY );

#endif

IMPLEMENT_NETWORKCLASS_ALIASED( TFWeaponBaseGrenadeProj, DT_TFWeaponBaseGrenadeProj )

LINK_ENTITY_TO_CLASS( tf_weaponbase_grenade_proj, CTFWeaponBaseGrenadeProj );
PRECACHE_REGISTER( tf_weaponbase_grenade_proj );

BEGIN_NETWORK_TABLE( CTFWeaponBaseGrenadeProj, DT_TFWeaponBaseGrenadeProj )
#ifdef CLIENT_DLL
	RecvPropVector( RECVINFO( m_vInitialVelocity ) ),
	RecvPropBool( RECVINFO( m_bCritical ) ),
	RecvPropBool( RECVINFO( m_bGrenade ) ),

	RecvPropVector( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin ) ),
	RecvPropQAngles( RECVINFO_NAME( m_angNetworkAngles, m_angRotation ) ),

#else
	SendPropVector( SENDINFO( m_vInitialVelocity ), 20 /*nbits*/, 0 /*flags*/, -3000 /*low value*/, 3000 /*high value*/	),
	SendPropBool( SENDINFO( m_bCritical ) ),
	SendPropBool( SENDINFO( m_bGrenade ) ),

	SendPropExclude( "DT_BaseEntity", "m_vecOrigin" ),
	SendPropExclude( "DT_BaseEntity", "m_angRotation" ),

	SendPropVector	(SENDINFO(m_vecOrigin), -1,  SPROP_COORD_MP_INTEGRAL|SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin ),
	SendPropQAngles	(SENDINFO(m_angRotation), 6, SPROP_CHANGES_OFTEN, SendProxy_Angles ),
#endif
END_NETWORK_TABLE()

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CTFWeaponBaseGrenadeProj::CTFWeaponBaseGrenadeProj()
{
#ifndef CLIENT_DLL
	m_bUseImpactNormal = false;
	m_vecImpactNormal.Init();
#endif
#ifdef GAME_DLL
	s_iszTrainName = AllocPooledString( "models/props_vehicles/train_enginecar.mdl" );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
CTFWeaponBaseGrenadeProj::~CTFWeaponBaseGrenadeProj()
{
#ifdef CLIENT_DLL
	ParticleProp()->StopEmission();
#else
	if( VPhysicsGetObject() )
	{
		VPhysicsDestroyObject();
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFWeaponBaseGrenadeProj::GetDamageType() 
{ 
	int iDmgType = g_aWeaponDamageTypes[ GetWeaponID() ];
	if ( m_bCritical )
	{
		iDmgType |= DMG_CRITICAL;
	}

	return iDmgType;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBaseGrenadeProj::Precache( void )
{
	BaseClass::Precache();

#ifndef CLIENT_DLL
	PrecacheModel( NOGRENADE_SPRITE );
	PrecacheParticleSystem( "critical_grenade_blue" );
	PrecacheParticleSystem( "critical_grenade_red" );
	PrecacheParticleSystem("pipebombtrail_blue");
	PrecacheParticleSystem("pipebombtrail_red");
	PrecacheParticleSystem("nadepulse_blue");
	PrecacheParticleSystem("nadepulse_red");
	PrecacheParticleSystem("nadepulse_final_blue");
	PrecacheParticleSystem("nadepulse_final_red");
	PrecacheParticleSystem("stickybombtrail_blue");
	PrecacheParticleSystem("stickybombtrail_red");
#endif
	PrecacheScriptSound("Weapon_Grenade.Beep");
	PrecacheScriptSound("Weapon_Grenade.Disarm");
	PrecacheScriptSound("Weapon_Grenade.FinalBeep");
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char* CTFWeaponBaseGrenadeProj::GetTrailParticleName(void)
{
	if (GetTeamNumber() == TF_TEAM_BLUE)
	{
		return "stickybombtrail_blue";
	}
	else
	{
		return "stickybombtrail_red";
	}

}

const char* CTFWeaponBaseGrenadeProj::GetPulseParticleName(void)
{
	if (GetTeamNumber() == TF_TEAM_BLUE)
	{
		return "nadepulse_blue";
	}
	else
	{
		return "nadepulse_red";
	}
}

const char* CTFWeaponBaseGrenadeProj::GetFinalPulseParticleName(void)
{
	if (GetTeamNumber() == TF_TEAM_BLUE)
	{
		return "nadepulse_final_blue";
	}
	else
	{
		return "nadepulse_final_red";
	}
}

//=============================================================================
//
// Client specific functions.
//
#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBaseGrenadeProj::Spawn()
{
	m_flSpawnTime = gpGlobals->curtime;
	BaseClass::Spawn();
}



//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBaseGrenadeProj::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if ( type == DATA_UPDATE_CREATED )
	{
		// Now stick our initial velocity into the interpolation history 
		CInterpolatedVar< Vector > &interpolator = GetOriginInterpolator();

		interpolator.ClearHistory();
		float changeTime = GetLastChangeTime( LATCH_SIMULATION_VAR );

		// Add a sample 1 second back.
		Vector vCurOrigin = GetLocalOrigin() - m_vInitialVelocity;
		interpolator.AddToHead( changeTime - 1.0, &vCurOrigin, false );

		// Add the current sample.
		vCurOrigin = GetLocalOrigin();
		interpolator.AddToHead( changeTime, &vCurOrigin, false );

		if (m_bGrenade)
		{
			ParticleProp()->Create(GetTrailParticleName(), PATTACH_ABSORIGIN_FOLLOW);
		}
	}
}

//=============================================================================
//
// Server specific functions.
//
#else
unsigned int CTFWeaponBaseGrenadeProj::PhysicsSolidMaskForEntity( void ) const
{
	int teamContents = 0;

	//if( m_bCollideWithTeammates == false )
	{
		// Only collide with the other team
		teamContents = (GetTeamNumber() == TF_TEAM_RED) ? CONTENTS_BLUETEAM : CONTENTS_REDTEAM;
	}
	/*else
	{
		// Collide with both teams
		teamContents = CONTENTS_REDTEAM | CONTENTS_BLUETEAM;
	}*/

	return BaseClass::PhysicsSolidMaskForEntity() | teamContents;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFWeaponBaseGrenadeProj *CTFWeaponBaseGrenadeProj::Create( const char *szName, const Vector &position, const QAngle &angles, 
													   const Vector &velocity, const AngularImpulse &angVelocity, 
													   CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo, int iFlags )
{
	CTFWeaponBaseGrenadeProj *pGrenade = static_cast<CTFWeaponBaseGrenadeProj*>( CBaseEntity::Create( szName, position, angles, pOwner ) );
	if ( pGrenade )
	{
		pGrenade->InitGrenade( velocity, angVelocity, pOwner, weaponInfo );
	}

	return pGrenade;
}
CTFWeaponBaseGrenadeProj* CTFWeaponBaseGrenadeProj::Create(const char* szName, const Vector& position, const QAngle& angles,
	const Vector& velocity, const AngularImpulse& angVelocity,
	CBaseCombatCharacter* pOwner, const CTFWeaponInfo& weaponInfo, float timer, int iFlags)
{
	CTFWeaponBaseGrenadeProj* pGrenade = Create(szName, position, angles, velocity, angVelocity, pOwner, weaponInfo, iFlags);
	if (pGrenade)
	{
		pGrenade->SetDetonateTimerLength(timer);
	}
	return pGrenade;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBaseGrenadeProj::InitGrenade( const Vector &velocity, const AngularImpulse &angVelocity, 
									CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo )
{
	// We can't use OwnerEntity for grenades, because then the owner can't shoot them with his hitscan weapons (due to collide rules)
	// Thrower is used to store the person who threw the grenade, for damage purposes.
	SetOwnerEntity( NULL );
	SetThrower( pOwner ); 

	m_bGrenade = weaponInfo.m_bGrenade;

	SetupInitialTransmittedGrenadeVelocity( velocity );

	SetGravity( TF_WEAPON_GRENADE_GRAVITY /*BaseClass::GetGrenadeGravity()*/ );
	SetFriction( TF_WEAPON_GRENADE_FRICTION /*BaseClass::GetGrenadeFriction()*/ );
	SetElasticity( 0.45f/*BaseClass::GetGrenadeElasticity()*/ );

	SetDamage( weaponInfo.GetWeaponData( TF_WEAPON_PRIMARY_MODE ).m_nDamage );
	
	SetDamageRadius( weaponInfo.m_flDamageRadius );
	ChangeTeam( pOwner->GetTeamNumber() );

	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	if ( pPhysicsObject )
	{
		pPhysicsObject->AddVelocity( &velocity, &angVelocity );
	}
	
	if(pf_grenades_tfc.GetBool() && m_bGrenade && GetWeaponID() != TF_WEAPON_GRENADE_CALTROP)
	{
		SetAbsVelocity( velocity );
		SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_CUSTOM );
	}

}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBaseGrenadeProj::Spawn( void )
{
	// Base class spawn.
	BaseClass::Spawn();

	// So it will collide with physics props!
	SetSolidFlags( FSOLID_NOT_STANDABLE );

	if( IsPillGrenade() || GetWeaponID() == TF_WEAPON_GRENADE_CALTROP )
	{
		SetSolid( SOLID_BBOX );
		// Set the grenade size here.
		UTIL_SetSize( this, Vector( -2.0f, -2.0f, -2.0f ), Vector( 2.0f, 2.0f, 2.0f ) );
		VPhysicsInitNormal( SOLID_BBOX, 0, false );
	}
	else
	{
		VPhysicsInitNormal( SOLID_VPHYSICS, 0, false );
		SetSolid( SOLID_VPHYSICS );
	}

	SetCollisionGroup( TF_COLLISIONGROUP_GRENADES );
		

	// Don't collide with players on the owner's team for the first bit of our life
	if(TFGameRules()->FriendlyFireMode())
	{
		m_flCollideWithTeammatesTime = gpGlobals->curtime;
		m_bCollideWithTeammates = true;
	}
	else
	{
		m_flCollideWithTeammatesTime = gpGlobals->curtime + 0.25;
		m_bCollideWithTeammates = false;
	}

	m_takedamage = DAMAGE_YES;
	SetHealth( 900 );

	if(!IsPillGrenade() && pf_grenades_disarm_shots.GetBool())
	{
		m_nDisarmShots = pf_grenades_disarm_shots.GetInt();
	}

	// Set the team.
	ChangeTeam( GetThrower()->GetTeamNumber() );

	// Set skin based on team ( red = 1, blue = 2 )
	m_nSkin = ( GetTeamNumber() == TF_TEAM_BLUE ) ? 1 : 0;

	// Setup the think and touch functions (see CBaseEntity).
	SetThink( &CTFWeaponBaseGrenadeProj::DetonateThink );
	SetNextThink( gpGlobals->curtime + 0.2 );

	RegisterThinkContext("BeepThink");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBaseGrenadeProj::Explode( trace_t *pTrace, int bitsDamageType )
{
	SetModelName( NULL_STRING );//invisible
	AddSolidFlags( FSOLID_NOT_SOLID );

	if( VPhysicsGetObject() )
	{
		VPhysicsDestroyObject();
	}

	m_takedamage = DAMAGE_NO;

	// Pull out of the wall a bit
	if ( pTrace->fraction != 1.0 )
	{
		SetAbsOrigin( pTrace->endpos + ( pTrace->plane.normal * 1.0f ) );
	}

	CSoundEnt::InsertSound ( SOUND_COMBAT, GetAbsOrigin(), BASEGRENADE_EXPLOSION_VOLUME, 3.0 );

	// Explosion effect on client
	Vector vecOrigin = GetAbsOrigin();
	CPVSFilter filter( vecOrigin );
	if ( UseImpactNormal() )
	{
		if ( pTrace->m_pEnt && pTrace->m_pEnt->IsPlayer() )
		{
			TE_TFExplosion( filter, 0.0f, vecOrigin, GetImpactNormal(), GetWeaponID(), pTrace->m_pEnt->entindex() );
		}
		else
		{
			TE_TFExplosion( filter, 0.0f, vecOrigin, GetImpactNormal(), GetWeaponID(), -1 );
		}
	}
	else
	{
		if ( pTrace->m_pEnt && pTrace->m_pEnt->IsPlayer() )
		{
			TE_TFExplosion( filter, 0.0f, vecOrigin, pTrace->plane.normal, GetWeaponID(), pTrace->m_pEnt->entindex() );
		}
		else
		{
			TE_TFExplosion( filter, 0.0f, vecOrigin, pTrace->plane.normal, GetWeaponID(), -1 );
		}
	}



	// Use the thrower's position as the reported position
	Vector vecReported = GetThrower() ? GetThrower()->GetAbsOrigin() : vec3_origin;

	CTakeDamageInfo info( this, GetThrower(), GetBlastForce(), GetAbsOrigin(), m_flDamage, bitsDamageType, 0, &vecReported );

	float flRadius = GetDamageRadius();

	if ( tf_grenade_show_radius.GetBool() )
	{
		DrawRadius( flRadius, tf_grenade_show_radius_time.GetFloat() );
	}

	RadiusDamage( info, vecOrigin, flRadius, CLASS_NONE, NULL );

	// Don't decal players with scorch.
	if ( pTrace->m_pEnt && !pTrace->m_pEnt->IsPlayer() )
	{
		UTIL_DecalTrace( pTrace, "Scorch" );
	}

	SetThink( &CBaseGrenade::SUB_Remove );
	SetTouch( NULL );

	AddEffects( EF_NODRAW );
	SetAbsVelocity( vec3_origin );
	SetNextThink( gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFWeaponBaseGrenadeProj::OnTakeDamage( const CTakeDamageInfo &info )
{
	// don't do anything if you're damaging a friendly grenade
	if(info.GetAttacker() && InSameTeam(info.GetAttacker()))
		return 0;

	// Cyanide; durrr this seems fine
	if( !V_strcmp( info.GetInflictor()->GetClassname(), "trigger_hurt" ) )
		return 0;

	CTakeDamageInfo info2 = info;

	// Reduce explosion damage so that we don't get knocked too far
	if ( m_bGrenade || ( info.GetDamageType() & DMG_BLAST ) )
	{
		info2.ScaleDamageForce( 0.05f );
	}

	if(!IsPillGrenade() && pf_grenades_disarm_shots.GetBool())
	{
		// Melee disarming
		if(info.GetDamageType() & (DMG_CLUB | DMG_SLASH))
		{
			m_nDisarmShots--;
			if(m_nDisarmShots <= 0)
				Disarm(info2);
		}
		
	}


	// We need to skip back to the base entity take damage, because
	// CBaseCombatCharacter doesn't, which prevents us from reacting
	// to physics impact damage.
	return CBaseEntity::OnTakeDamage( info2 );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBaseGrenadeProj::DetonateThink( void )
{
	if( m_bRemoving )
		return;

	if ( !IsInWorld() )
	{
		Remove();
		return;
	}

	if ( gpGlobals->curtime > m_flCollideWithTeammatesTime && m_bCollideWithTeammates == false )
	{
		m_bCollideWithTeammates = true;
	}

	if ( gpGlobals->curtime > m_flDetonateTime )
	{
		Detonate();
		return;
	}


	SetNextThink( gpGlobals->curtime + 0.2 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBaseGrenadeProj::Detonate( void )
{
	trace_t		tr;
	Vector		vecSpot;// trace starts here!

	SetThink( NULL );

	vecSpot = GetAbsOrigin() + Vector ( 0 , 0 , 8 );
	UTIL_TraceLine ( vecSpot, vecSpot + Vector ( 0, 0, -32 ), MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, & tr);

	Explode( &tr, GetDamageType() );

	if ( GetShakeAmplitude() )
	{
		UTIL_ScreenShake( GetAbsOrigin(), GetShakeAmplitude(), 150.0, 1.0, GetShakeRadius(), SHAKE_START );
	}
}

void CTFWeaponBaseGrenadeProj::ExplodeInHand(CTFPlayer* pPlayer)
{
	Detonate();
}

void CTFWeaponBaseGrenadeProj::BeepThink(void)
{
	if(m_bRemoving)
		return;

	if ( m_flDetonateTime - 1.0 > gpGlobals->curtime )
	{
		EmitSound( "Weapon_Grenade.Beep" );
		DispatchParticleEffect( GetPulseParticleName(), PATTACH_ABSORIGIN_FOLLOW, this );
		SetNextThink( gpGlobals->curtime + 1.0f, "BeepThink" );
	}
	else
	{
		EmitSound("Weapon_Grenade.FinalBeep");
		DispatchParticleEffect(GetFinalPulseParticleName(), PATTACH_ABSORIGIN_FOLLOW, this);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets the time at which the grenade will explode.
//-----------------------------------------------------------------------------
void CTFWeaponBaseGrenadeProj::SetDetonateTimerLength( float timer )
{
	m_flDetonateTime = gpGlobals->curtime + timer;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBaseGrenadeProj::ResolveFlyCollisionCustom( trace_t &trace, Vector &vecVelocity )
{
	//Assume all surfaces have the same elasticity
	float flSurfaceElasticity = 1.0;

	//Don't bounce off of players with perfect elasticity
	if( trace.m_pEnt && trace.m_pEnt->IsPlayer() )
	{
		if( pf_grenades_bonk.GetBool() && m_flNextAttack < gpGlobals->curtime )
		{
			Vector vel = GetAbsVelocity();
			Vector dir = vel;
			VectorNormalize( dir );

			float flPercent = vel.Length() / 2000;
			float flDmg = 5 * flPercent;

			// send a tiny amount of damage so the character will react to getting bonked
			CTakeDamageInfo info( this, GetThrower(), 5 * vel, GetAbsOrigin(), flDmg, DMG_CLUB );
			info.SetIgnoreArmor( true );
			info.SetDamageCustom( TF_DMG_CUSTOM_GRENADE_BONK );
			trace.m_pEnt->DispatchTraceAttack( info, dir, &trace );
			ApplyMultiDamage();

			m_flNextAttack = gpGlobals->curtime + 1.0; // debounce
		}
		flSurfaceElasticity = 0.3;
	}

	// Train hack!
	// taken from the pipebomb code
	// -Nbc66
	if( trace.m_pEnt->GetModelName() == s_iszTrainName && (trace.m_pEnt->GetAbsVelocity().LengthSqr() > 1.0f) )
	{
		DevMsg( 2 , "WE HAVE TOUCHED A FUCKING TRAIN WITH A GRENADE" );
		Explode( &trace, GetDamageType() );
	}

#if 0
	// if its breakable glass and we kill it, don't bounce.
	// give some damage to the glass, and if it breaks, pass 
	// through it.
	bool breakthrough = false;

	if( trace.m_pEnt && FClassnameIs( trace.m_pEnt, "func_breakable" ) )
	{
		breakthrough = true;
	}

	if( trace.m_pEnt && FClassnameIs( trace.m_pEnt, "func_breakable_surf" ) )
	{
		breakthrough = true;
	}

	if (breakthrough)
	{
		CTakeDamageInfo info( this, this, 10, DMG_CLUB );
		trace.m_pEnt->DispatchTraceAttack( info, GetAbsVelocity(), &trace );

		ApplyMultiDamage();

		if( trace.m_pEnt->m_iHealth <= 0 )
		{
			// slow our flight a little bit
			Vector vel = GetAbsVelocity();

			vel *= 0.4;

			SetAbsVelocity( vel );
			return;
		}
	}
#endif

	float flTotalElasticity = GetElasticity() * flSurfaceElasticity;
	flTotalElasticity = clamp( flTotalElasticity, 0.0f, 0.9f );

	// NOTE: A backoff of 2.0f is a reflection
	Vector vecAbsVelocity;
	PhysicsClipVelocity( GetAbsVelocity(), trace.plane.normal, vecAbsVelocity, 2.0f );
	vecAbsVelocity *= flTotalElasticity;

	// Get the total velocity (player + conveyors, etc.)
	VectorAdd( vecAbsVelocity, GetBaseVelocity(), vecVelocity );
	float flSpeedSqr = DotProduct( vecVelocity, vecVelocity );

	// Reduce angular velocity specifically for the nail grenade
	Vector vecAngularVel;
	QAngleToAngularImpulse(GetLocalAngularVelocity(), vecAngularVel);
	vec_t oldY = GetLocalAngularVelocity().y;
	PhysicsClipVelocity(vecAngularVel, trace.plane.normal, vecAngularVel, 2.0f);

	QAngle angAngularVel;
	AngularImpulseToQAngle(vecAngularVel, angAngularVel );

	angAngularVel.y = oldY/2;
	SetLocalAngularVelocity(angAngularVel);

	// Stop if on ground.
	if ( trace.plane.normal.z > 0.7f )			// Floor
	{
		// Verify that we have an entity.
		CBaseEntity *pEntity = trace.m_pEnt;
		Assert( pEntity );

		// Are we on the ground?
		if( vecVelocity.z < (sv_gravity.GetFloat() * gpGlobals->frametime) )
		{
			if( pEntity->IsStandable() )
			{
				SetGroundEntity( pEntity );
			}

			vecAbsVelocity.z = 0.0f;
		}

		SetAbsVelocity( vecAbsVelocity );

		if ( flSpeedSqr < ( 30 * 30 ) )
		{
			if ( pEntity->IsStandable() )
			{
				SetGroundEntity( pEntity );
			}

			
			//if(pf_grenades_tfc.GetInt() > 1)
			{
				SetLocalAngularVelocity( angAngularVel/2 );
				VPhysicsInitNormal( SOLID_BBOX, 0, false );
				SetMoveType( MOVETYPE_VPHYSICS, MOVECOLLIDE_FLY_BOUNCE );
			}
			/*else
			{

				//align to the ground so we're not standing on end
				QAngle angle;
				VectorAngles( trace.plane.normal, angle );

				// rotate randomly in yaw
				//angle[1] = random->RandomFloat( 0, 360 );

				// TFTODO: rotate around trace.plane.normal
				SetLocalAngularVelocity( vec3_angle );

				SetAbsAngles( angle );
			}*/

			// Reset velocities.
			SetAbsVelocity( vec3_origin );		
		}
		else
		{
			if( vecVelocity.z < (sv_gravity.GetFloat() * gpGlobals->frametime) )
			{
				SetLocalAngularVelocity( angAngularVel / 2 );
				VPhysicsInitNormal( SOLID_BBOX, 0, false );
				SetMoveType( MOVETYPE_VPHYSICS, MOVECOLLIDE_FLY_BOUNCE );
			}
			else
			{

				Vector vecDelta = GetBaseVelocity() - vecAbsVelocity;
				Vector vecBaseDir = GetBaseVelocity();
				VectorNormalize( vecBaseDir );
				float flScale = vecDelta.Dot( vecBaseDir );

				VectorScale( vecAbsVelocity, (1.0f - trace.fraction) * gpGlobals->frametime, vecVelocity );
				VectorMA( vecVelocity, (1.0f - trace.fraction) * gpGlobals->frametime, GetBaseVelocity() * flScale, vecVelocity );
				PhysicsPushEntity( vecVelocity, &trace );
			}
		}
	}
	else
	{
		// If we get *too* slow, we'll stick without ever coming to rest because
		// we'll get pushed down by gravity faster than we can escape from the wall.
		if ( flSpeedSqr < ( 30 * 30 ) )
		{
			// Reset velocities.
			SetAbsVelocity( vec3_origin );
			SetLocalAngularVelocity( vec3_angle );
		}
		else
		{
			SetAbsVelocity( vecAbsVelocity );
		}
	}

	BounceSound();

#if 0
	// tell the bots a grenade has bounced
	CCSPlayer *player = ToCSPlayer(GetThrower());
	if ( player )
	{
		KeyValues *event = new KeyValues( "grenade_bounce" );
		event->SetInt( "userid", player->GetUserID() );
		gameeventmanager->FireEventServerOnly( event );
	}
#endif
}

bool CTFWeaponBaseGrenadeProj::ShouldNotDetonate( void )
{
	if( GrenadeInRespawnRoom( this, this->GetAbsOrigin() ) || InNoGrenadeZone( this ) )
	{
		return true;
	}
	else
	{
		return false;
	}
}

void CTFWeaponBaseGrenadeProj::RemoveGrenade( bool bBlinkOut )
{
	// Kill it
	SetThink( &BaseClass::SUB_Remove );
	SetNextThink( gpGlobals->curtime );
	SetTouch( NULL );
	AddEffects( EF_NODRAW );
	if( VPhysicsGetObject() )
	{
		VPhysicsDestroyObject();
	}

	if ( bBlinkOut )
	{
		// Sprite flash
		CSprite *pGlowSprite = CSprite::SpriteCreate( NOGRENADE_SPRITE, GetAbsOrigin(), false );
		if ( pGlowSprite )
		{
			pGlowSprite->SetTransparency( kRenderGlow, 255, 255, 255, 255, kRenderFxFadeFast );
			pGlowSprite->SetThink( &CSprite::SUB_Remove );
			pGlowSprite->SetNextThink( gpGlobals->curtime + 1.0 );
		}
	}
}

const float GRENADE_COEFFICIENT_OF_RESTITUTION = 0.2f;

//-----------------------------------------------------------------------------
// Purpose: Grenades aren't solid to players, so players don't get stuck on
//			them when they're lying on the ground. We still want thrown grenades
//			to bounce of players though, so manually trace ahead and see if we'd
//			hit something that we'd like the grenade to "collide" with.
//-----------------------------------------------------------------------------
void CTFWeaponBaseGrenadeProj::VPhysicsUpdate( IPhysicsObject *pPhysics )
{
	BaseClass::VPhysicsUpdate( pPhysics );

	Vector vel;
	AngularImpulse angVel;
	pPhysics->GetVelocity( &vel, &angVel );

	Vector start = GetAbsOrigin();

	// find all entities that my collision group wouldn't hit, but COLLISION_GROUP_NONE would and bounce off of them as a ray cast
	CTraceFilterCollisionGrenades filter( this, GetThrower() );
	trace_t tr;

	UTIL_TraceLine( start, start + vel * gpGlobals->frametime, CONTENTS_HITBOX|CONTENTS_MONSTER|CONTENTS_SOLID, &filter, &tr );

	bool bHitEnemy = false;

	if ( tr.m_pEnt )
	{
		bool bFF = !!TFGameRules()->FriendlyFireMode();
		if( !bFF && tr.m_pEnt->GetTeamNumber() != GetTeamNumber())
			bHitEnemy = true;
		else if( bFF && tr.m_pEnt->GetTeamNumber())
			bHitEnemy = true;
	}

	if ( tr.startsolid )
	{
		if ( (m_bInSolid == false && m_bCollideWithTeammates == true) || ( m_bInSolid == false  && bHitEnemy == true ) )
		{
			// UNDONE: Do a better contact solution that uses relative velocity?
			vel *= -GRENADE_COEFFICIENT_OF_RESTITUTION; // bounce backwards
			pPhysics->SetVelocity( &vel, NULL );
		}
		m_bInSolid = true;
		return;
	}

	m_bInSolid = false;

	if ( tr.DidHit() )
	{
		Touch( tr.m_pEnt );
		
		if ( m_bCollideWithTeammates == true || bHitEnemy == true )
		{
			// reflect velocity around normal
			vel = -2.0f * tr.plane.normal * DotProduct(vel,tr.plane.normal) + vel;

			// absorb 80% in impact
			vel *= GetElasticity();

			if ( bHitEnemy == true )
			{
				vel *= 0.5f;
			}

			angVel *= -0.5f;
			
			if( !IsPillGrenade() && pf_grenades_bonk.GetBool() && m_flNextAttack < gpGlobals->curtime )
			{
				Vector dir = vel;
				VectorNormalize( dir );

				float flPercent = vel.Length() / 2000;
				float flDmg = 5 * flPercent;

				// send a tiny amount of damage so the character will react to getting bonked
				CTakeDamageInfo info( this, GetThrower(), 5 * vel, GetAbsOrigin(), flDmg, DMG_CLUB );
				info.SetIgnoreArmor( true );
				info.SetDamageCustom( TF_DMG_CUSTOM_GRENADE_BONK );
				tr.m_pEnt->DispatchTraceAttack( info, dir, &tr );
				ApplyMultiDamage();

				m_flNextAttack = gpGlobals->curtime + 1.0; // debounce
			}

			pPhysics->SetVelocity( &vel, &angVel );
		}
	}
}


void CTFWeaponBaseGrenadeProj::DrawRadius( float flRadius, float flLifetime )
{
	NDebugOverlay::Sphere( GetAbsOrigin(), flRadius, 255, 0, 0, false, flLifetime );
}


//-----------------------------------------------------------------------------
// Purpose: Using this for some grenades. Check if we can actually hit a target
//-----------------------------------------------------------------------------
bool CTFWeaponBaseGrenadeProj::RadiusHit( const Vector &vecSrcIn, CBaseEntity *pInflictor, CBaseEntity *pInflicted )
{
	const int MASK_RADIUS_DAMAGE = MASK_SHOT&(~CONTENTS_HITBOX);
	trace_t		tr;
	Vector		vecSpot;

	Vector vecSrc = vecSrcIn;

	if (pInflicted->m_takedamage == DAMAGE_NO)
		return false;

	// Check that the explosion can 'see' this entity.
	vecSpot = pInflicted->BodyTarget( vecSrc, false );
	UTIL_TraceLine( vecSrc, vecSpot, MASK_RADIUS_DAMAGE, pInflictor, COLLISION_GROUP_DEBRIS, &tr );

	if (tr.fraction != 1.0 && tr.m_pEnt != pInflicted)
		return false;

	return true;
}

void CTFWeaponBaseGrenadeProj::PlayDisarmSound()
{
	CPASAttenuationFilter filter( GetAbsOrigin() );
	EmitSound( filter, entindex(), "Weapon_Grenade.Disarm" );
}

void CTFWeaponBaseGrenadeProj::Disarm( const CTakeDamageInfo &info )
{
	if(m_bRemoving)
		return;

	m_bRemoving = true;

	// play the disarm sound
	PlayDisarmSound();

	IGameEvent* event = gameeventmanager->CreateEvent( "grenade_disarmed" );
	if ( event )
	{
		event->SetInt( "index", this->entindex());
		int owner = -1;
		int attacker = -1;

		CTFPlayer* pTFThrower = nullptr;
		if(GetThrower())
		{
			pTFThrower = ToTFPlayer(GetThrower());
			if(pTFThrower)
			{
				owner = pTFThrower->entindex();
			}
		}
		CBaseEntity *pAttacker = info.GetAttacker();
		if(pAttacker && pAttacker->IsPlayer())
		{
			CTFPlayer *pTFDisarmer = ToTFPlayer(pAttacker);
			if(pTFDisarmer)
			{
				attacker = pTFDisarmer->entindex();
			}
		}
		event->SetInt( "player", owner );
		event->SetInt( "disarmer", attacker );

		const char *killer_weapon_name = TFGameRules()->GetKillingWeaponName( info, pTFThrower );
		event->SetString( "weapon", killer_weapon_name );

		gameeventmanager->FireEvent( event );
	}

	// set up removal
	SetDetonateTimerLength(2.0f);
	SetThink( &CTFWeaponBaseGrenadeProj::DisarmThink );
	SetNextThink(gpGlobals->curtime);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBaseGrenadeProj::DisarmThink( void )
{
	if ( !IsInWorld() )
	{
		Remove( );
		return;
	}

	if ( gpGlobals->curtime > m_flDetonateTime )
	{
		RemoveGrenade();
		return;
	}

	SetNextThink( gpGlobals->curtime + 0.2 );
}
#endif
