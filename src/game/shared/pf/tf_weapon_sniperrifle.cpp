//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF Sniper Rifle
//
//=============================================================================//
#include "cbase.h" 
#include "tf_fx_shared.h"
#include "tf_weapon_sniperrifle.h"
#include "in_buttons.h"
#include "tf_weaponbase_grenade.h"
#include "tf_gamerules.h"

// Client specific.
#ifdef CLIENT_DLL
#include "view.h"
#include "beamdraw.h"
#include "vgui/ISurface.h"
#include <vgui/ILocalize.h>
#include "vgui_controls/Controls.h"
#include "hud_crosshair.h"
#include "functionproxy.h"
#include "materialsystem/imaterialvar.h"
#include "toolframework_client.h"
#include "input.h"

// forward declarations
void ToolFramework_RecordMaterialParams( IMaterial *pMaterial );
#endif

#define TF_WEAPON_SNIPERRIFLE_CHARGE_PER_SEC	50.0f
#define TF_WEAPON_SNIPERRIFLE_UNCHARGE_PER_SEC	75.0f
#define	TF_WEAPON_SNIPERRIFLE_DAMAGE_MIN		50
#define TF_WEAPON_SNIPERRIFLE_DAMAGE_MAX		150
#define TF_WEAPON_SNIPERRIFLE_RELOAD_TIME		1.5f
#define TF_WEAPON_SNIPERRIFLE_ZOOM_TIME			0.3f

#define TF_WEAPON_SNIPERRIFLE_NO_CRIT_AFTER_ZOOM_TIME	0.2f

#define SNIPER_DOT_SPRITE_RED		"effects/sniperdot_red.vmt"
#define SNIPER_DOT_SPRITE_BLUE		"effects/sniperdot_blue.vmt"
#define SNIPER_CHARGE_BEAM_RED		"tfc_sniper_charge_red"
#define SNIPER_CHARGE_BEAM_BLUE		"tfc_sniper_charge_blue"

//=============================================================================
//
// Weapon Sniper Rifles tables.
//

IMPLEMENT_NETWORKCLASS_ALIASED( TFSniperRifle, DT_TFSniperRifle )

BEGIN_NETWORK_TABLE_NOBASE( CTFSniperRifle, DT_SniperRifleLocalData )
#if !defined( CLIENT_DLL )
	SendPropFloat( SENDINFO(m_flChargedDamage), 0, SPROP_NOSCALE | SPROP_CHANGES_OFTEN ),
	SendPropFloat( SENDINFO(m_flChargeStarted), 0, SPROP_NOSCALE | SPROP_CHANGES_OFTEN ),
#else
	RecvPropFloat( RECVINFO(m_flChargedDamage) ),
	RecvPropFloat( RECVINFO( m_flChargeStarted ) ),
#endif
END_NETWORK_TABLE()

BEGIN_NETWORK_TABLE( CTFSniperRifle, DT_TFSniperRifle )
#if !defined( CLIENT_DLL )
	SendPropDataTable( "SniperRifleLocalData", 0, &REFERENCE_SEND_TABLE( DT_SniperRifleLocalData ), SendProxy_SendLocalWeaponDataTable ),
#else
	RecvPropDataTable( "SniperRifleLocalData", 0, 0, &REFERENCE_RECV_TABLE( DT_SniperRifleLocalData ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFSniperRifle )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_flChargedDamage, FIELD_FLOAT, 0 ),
	DEFINE_PRED_FIELD( m_flChargeStarted, FIELD_FLOAT, 0 ),
	DEFINE_PRED_FIELD( m_bInAttack, FIELD_BOOLEAN, 0 ),
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_sniperrifle, CTFSniperRifle );
PRECACHE_WEAPON_REGISTER( tf_weapon_sniperrifle );

ConVar tf_sniper_rifle_charge_multiplier( "tf_sniper_rifle_charge_damage_multiplier", "3", FCVAR_DEVELOPMENTONLY | FCVAR_NOTIFY | FCVAR_REPLICATED );
ConVar pf_sniper_rifle_tfc_style( "pf_sniper_rifle_tfc_style", "0", FCVAR_DEVELOPMENTONLY | FCVAR_NOTIFY | FCVAR_REPLICATED );
ConVar pf_sniper_rifle_tfc_noair( "pf_sniper_rifle_tfc_noair", "1", FCVAR_DEVELOPMENTONLY | FCVAR_NOTIFY | FCVAR_REPLICATED );
ConVar pf_sniper_rifle_tfc_nofire( "pf_sniper_rifle_tfc_nofire", "1", FCVAR_DEVELOPMENTONLY | FCVAR_NOTIFY | FCVAR_REPLICATED );
#ifdef CLIENT_DLL
ConVar pf_sniper_toggle_charge( "pf_sniper_toggle_charge", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE | FCVAR_USERINFO );
ConVar tf_sniper_fullcharge_bell( "tf_sniper_fullcharge_bell", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE | FCVAR_DONTRECORD, "Play a sound when the sniper rifle is full charged" );
#endif

//=============================================================================
//
// Weapon Sniper Rifles funcions.
//

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CTFSniperRifle::CTFSniperRifle()
{
// Server specific.
#ifdef GAME_DLL
	m_hSniperDot = NULL;
#endif
#ifdef CLIENT_DLL
	m_pLaserEffect = NULL;
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
CTFSniperRifle::~CTFSniperRifle()
{
// Server specific.
#ifdef GAME_DLL
	DestroySniperDot();
#endif

#ifdef CLIENT_DLL
	m_pLaserEffect = NULL;
#endif // CLIENT_DLL

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSniperRifle::Spawn()
{
	m_iAltFireHint = HINT_ALTFIRE_SNIPERRIFLE;
	BaseClass::Spawn();

	ResetTimers();
	m_flDamageMin = GetTFWeaponInfo(TF_WEAPON_SNIPERRIFLE)->GetWeaponDamage();
	m_flDamageMax = m_flDamageMin * tf_sniper_rifle_charge_multiplier.GetFloat();
	m_bInAttack = false;
#ifdef CLIENT_DLL
	m_bPlayedDing = false;
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFSniperRifle::Precache()
{
	BaseClass::Precache();
	PrecacheModel( SNIPER_DOT_SPRITE_RED );
	PrecacheModel( SNIPER_DOT_SPRITE_BLUE );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSniperRifle::ResetTimers( void )
{
	m_bInAttack = false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFSniperRifle::Reload( void )
{
	// We currently don't reload.
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFSniperRifle::CanHolster( void ) const
{
 	CTFPlayer *pPlayer = GetTFPlayerOwner();
 	if ( pPlayer )
	{
		// don't allow us to holster this weapon if we're in the process of zooming and 
		// we've just fired the weapon (next primary attack is only 1.5 seconds after firing)
		if ( ( pPlayer->GetFOV() < pPlayer->GetDefaultFOV() ) && ( m_flNextPrimaryAttack > gpGlobals->curtime ) )
		{
			return false;
		}
	}

	return BaseClass::CanHolster();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFSniperRifle::Holster( CBaseCombatWeapon *pSwitchingTo )
{
// Server specific.
#ifdef GAME_DLL
	// Destroy the sniper dot.
	DestroySniperDot();
#else // CLIENT_DLL
	if( m_pLaserEffect && m_hLaserEffectHost.Get() )
	{
		m_hLaserEffectHost.Get()->ParticleProp()->StopEmissionAndDestroyImmediately( m_pLaserEffect );
		m_pLaserEffect = NULL;
		m_hLaserEffectHost = NULL;
	}
#endif

	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_ZOOMED ) )
	{
		ZoomOut();
	}
	if (pPlayer && pPlayer->m_Shared.InCond(TF_COND_AIMING))
	{
		AimOut();
	}

	m_flChargedDamage = 0.0f;
	m_flChargeStarted = 0.0f;
	ResetTimers();

	return BaseClass::Holster( pSwitchingTo );
}

void CTFSniperRifle::WeaponReset( void )
{
	BaseClass::WeaponReset();
	ZoomOut();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFSniperRifle::HandleZooms( void )
{
	// Get the owning player.
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if ( !pPlayer )
		return;

	// Handle the zoom when taunting.
	if ( pPlayer->m_Shared.InCond( TF_COND_TAUNTING ) )
	{
		if ( pPlayer->m_Shared.InCond( TF_COND_AIMING ) )
		{
			ToggleZoom();
		}

		//Don't rezoom in the middle of a taunt.
		ResetTimers();
	}

	bool bDoZoom = false;
	if ( pPlayer->ShouldHoldZoom() )
	{
		if ( ( pPlayer->m_nButtons & IN_ATTACK2 ? !IsZoomed() : IsZoomed() ) )
		{
			bDoZoom = true;
		}
	}
	else
	{
		if ( ( pPlayer->m_nButtons & IN_ATTACK2 ) )
		{
			bDoZoom = true;
		}
	}

	if ( bDoZoom && m_flNextSecondaryAttack <= gpGlobals->curtime )
	{
		// Don't zoom if we're out of ammo, we're about to switch
		if ( pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 && !pPlayer->m_Shared.InCond( TF_COND_ZOOMED ) )
		{
			return;
		}

		Zoom();
	}
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSniperRifle::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	ManageChargeBeam();
}

extern ConVar r_drawviewmodel;
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSniperRifle::ManageChargeBeam( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();

	C_BaseEntity *pAttachEnt = GetWeaponForEffect();

	if( m_hLaserEffectHost.Get() && m_hLaserEffectHost.Get() != pAttachEnt )
	{
		DestroyChargeBeam();
	}

	if( pPlayer->m_Shared.InCond( TF_COND_AIMING ) && CanAttack() && (m_flNextPrimaryAttack <= gpGlobals->curtime) )
	{
		if( !m_pLaserEffect )
		{
			int iAttachment = pAttachEnt->LookupAttachment( "laser" );
			if( iAttachment > 0 )
			{
				const char *pBeamEffectName = (GetTeamNumber() == TF_TEAM_RED) ? SNIPER_CHARGE_BEAM_RED : SNIPER_CHARGE_BEAM_BLUE;
				// Create at model attachment
				if( !UsingViewModel() )
				{
					m_pLaserEffect = pAttachEnt->ParticleProp()->Create( pBeamEffectName, PATTACH_POINT_FOLLOW, iAttachment );
				}
				else
				{
					m_pLaserEffect = pAttachEnt->ParticleProp()->Create( VarArgs("%s_vm", pBeamEffectName ), PATTACH_POINT_FOLLOW, iAttachment);
				}
				m_hLaserEffectHost = pAttachEnt;
			}
		}
	}
	else
	{
		DestroyChargeBeam();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSniperRifle::DestroyChargeBeam( void )
{
	if( m_pLaserEffect && m_hLaserEffectHost.Get() )
	{
		m_hLaserEffectHost.Get()->ParticleProp()->StopEmissionAndDestroyImmediately( m_pLaserEffect );
		m_pLaserEffect = NULL;
		m_hLaserEffectHost = NULL;
	}
}
#endif


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFSniperRifle::ItemPostFrame( void )
{
	// Get the owning player.
	CTFPlayer *pPlayer = ToTFPlayer(GetOwner());
	if (!pPlayer)
		return;

	if ( m_bLowered )
		return;

	if ( !CanAttack() )
	{
		if ( IsZoomed() )
		{
			ToggleZoom();
			AimOut();
		}
		return;
	}

	HandleZooms();

#ifdef GAME_DLL
	// Update the sniper dot position if we have one
	if ( m_hSniperDot && m_flNextPrimaryAttack <= gpGlobals->curtime )
	{
		UpdateSniperDot();
	}
#endif

	// Start charging when we're zoomed in, and allowed to fire
	if ( m_flNextPrimaryAttack <= gpGlobals->curtime )
	{
		// Don't start charging in the time just after a shot before we unzoom to play rack anim.
		if ( pPlayer->m_Shared.InCond( TF_COND_AIMING ) )
		{
			m_flChargedDamage = min( m_flChargedDamage + gpGlobals->frametime * m_flDamageMin, m_flDamageMax );
#ifdef CLIENT_DLL
			if( tf_sniper_fullcharge_bell.GetBool() )
			{
				if( m_flChargedDamage >= m_flDamageMax && !m_bPlayedDing )
				{
					C_TFPlayer::GetLocalTFPlayer()->EmitSound( "TFPlayer.ReCharged" );
					m_bPlayedDing = true;
				}
			}
#endif

		}
		else
		{	
			m_flChargedDamage = max( 0.f, m_flChargedDamage - gpGlobals->frametime * TF_WEAPON_SNIPERRIFLE_UNCHARGE_PER_SEC );
		}
	}

	bool bTFCMode = pf_sniper_rifle_tfc_style.GetBool();
	bool bTFCNoAir = pf_sniper_rifle_tfc_noair.GetBool();
	bool bTFCNoFire = pf_sniper_rifle_tfc_nofire.GetBool();

	bool bInAttack = false;
	if(pPlayer->ShouldToggleSniperCharge())
	{
		// did the player press the attack key
		if(pPlayer->m_afButtonPressed & IN_ATTACK)
		{
			// if we were already attacking we setup to fire, otherwise begin to charge
			if(m_bInAttack)
				bInAttack = false;
			else
				bInAttack = true;
		}
		// if we were already attacking and we haven't pressed anything keep charging
		else if (m_bInAttack)
		{
			bInAttack = true;
		}
	}
	else
	{
		bInAttack = (pPlayer->m_nButtons & IN_ATTACK) != 0;
	}

	// Fire.
	if ( bInAttack )
	{
		m_bInAttack = true;
		if ( !bTFCMode )
			AimIn();
		else
		{
			// I think friction is a bit lower in TF2 than in TFC, so we'll be a little lenient
			Vector vec = pPlayer->GetLocalVelocity();
			if ( m_flNextPrimaryAttack <= gpGlobals->curtime && vec.Length() < 80.0f /*50.0f */ )
				AimIn();
		}
	}
	else
	{
		if ( pPlayer->m_Shared.InCond( TF_COND_AIMING ) || ( bTFCMode && !bTFCNoFire ) )
		{
			if ( m_bInAttack )
			{
				m_bInAttack = false;
				if ( bTFCMode && ( bTFCNoAir && !pPlayer->GetGroundEntity() ) )
				{
					ClientPrint( pPlayer, HUD_PRINTCENTER, "You can't fire the sniper rifle without\nboth feet on the ground." );
				}
				else
				{
					Fire( pPlayer );
				}
				AimOut();
			}
		}
	}

	// Idle.
	if ( !( bInAttack || ( pPlayer->m_nButtons & IN_ATTACK2 ) ) )
	{
		m_bInAttack = false;
		// No fire buttons down or reloading
		if ( !ReloadOrSwitchWeapons() && ( m_bInReload == false ) )
		{
			WeaponIdle();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFSniperRifle::Lower( void )
{
	if ( BaseClass::Lower() )
	{
		if ( IsZoomed() )
		{
			ToggleZoom();
		}

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Secondary attack.
//-----------------------------------------------------------------------------
void CTFSniperRifle::Zoom( void )
{
	CTFPlayer* pPlayer = ToTFPlayer( GetOwner() );
	if ( !pPlayer )
		return;

	if ( pPlayer->GetFOV() >= 75 )
	{
		ZoomIn();
		if( pPlayer->ShouldHoldZoom() )
			m_flNextSecondaryAttack = gpGlobals->curtime + 0.1f;
		else
			m_flNextSecondaryAttack = gpGlobals->curtime + TF_WEAPON_SNIPERRIFLE_ZOOM_TIME;
	}
	else
	{
		ZoomOut();
		m_flNextSecondaryAttack = gpGlobals->curtime + TF_WEAPON_SNIPERRIFLE_ZOOM_TIME;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSniperRifle::AimIn( void )
{
	// Start aiming.
	CTFPlayer *pPlayer = GetTFPlayerOwner();

	if ( !pPlayer )
		return;

	if ( pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
		return;

	if ( m_flNextPrimaryAttack > gpGlobals->curtime )
		return;

	if( !pPlayer->m_Shared.InCond( TF_COND_AIMING ) )
	{
		m_flChargeStarted = gpGlobals->curtime;
#ifdef CLIENT_DLL
		m_bPlayedDing = false;
#endif
	}

	pPlayer->m_Shared.AddCond( TF_COND_AIMING );
	pPlayer->TeamFortress_SetSpeed();

#ifdef GAME_DLL
	// Create the sniper dot.
	CreateSniperDot();
	pPlayer->ClearExpression();
#endif
}

bool CTFSniperRifle::IsZoomed( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();

	if ( pPlayer )
	{
		return pPlayer->m_Shared.InCond( TF_COND_ZOOMED );
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSniperRifle::AimOut( void )
{
	// Stop aiming
	CTFPlayer *pPlayer = GetTFPlayerOwner();

	if ( !pPlayer )
		return;

	pPlayer->m_Shared.RemoveCond( TF_COND_AIMING );
	pPlayer->TeamFortress_SetSpeed();
	m_bInAttack = false;

#ifdef GAME_DLL
	// Destroy the sniper dot.
	DestroySniperDot();
	pPlayer->ClearExpression();
#endif

	// if we are thinking about zooming, cancel it
	m_flChargedDamage = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFSniperRifle::Fire( CTFPlayer *pPlayer )
{
	// Check the ammo.  We don't use clip ammo, check the primary ammo type.
	if ( pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
	{
		HandleFireOnEmpty();
		return;
	}

	if ( m_flNextPrimaryAttack > gpGlobals->curtime )
		return;

	// Fire the sniper shot.
	PrimaryAttack();

	m_flChargedDamage = 0.0f;
	m_flChargeStarted = -1;

#ifdef GAME_DLL
	if ( m_hSniperDot )
	{
		m_hSniperDot->ResetChargeTime();
	}
#else // CLIENT_DLL
	DestroyChargeBeam();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CTFSniperRifle::GetProjectileDamage( void )
{
	// Uncharged? Min damage.
	return max( m_flChargedDamage.Get(), m_flDamageMin);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFSniperRifle::GetDamageType( void ) const
{
	// Only do hit location damage if we're zoomed
	/*CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_ZOOMED ) )
		return BaseClass::GetDamageType();*/

	return ( BaseClass::GetDamageType() & DMG_USE_HITLOCATIONS );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFSniperRifle::CreateSniperDot( void )
{
// Server specific.
#ifdef GAME_DLL

	// Check to see if we have already been created?
	if ( m_hSniperDot )
		return;

	if (m_flNextPrimaryAttack > gpGlobals->curtime)
		return;

	// Get the owning player (make sure we have one).
	CBaseCombatCharacter *pPlayer = GetOwner();
	if ( !pPlayer )
		return;

	// Create the sniper dot, but do not make it visible yet.
	m_hSniperDot = CSniperDot::Create( GetAbsOrigin(), pPlayer, true, (m_flDamageMax/m_flDamageMin) );
	m_hSniperDot->ChangeTeam( pPlayer->GetTeamNumber() );

#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFSniperRifle::DestroySniperDot( void )
{
// Server specific.
#ifdef GAME_DLL

	// Destroy the sniper dot.
	if ( m_hSniperDot )
	{
		UTIL_Remove( m_hSniperDot );
		m_hSniperDot = NULL;
	}

#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFSniperRifle::UpdateSniperDot( void )
{
// Server specific.
#ifdef GAME_DLL

	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if ( !pPlayer )
		return;

	// Get the start and endpoints.
	Vector vecMuzzlePos = pPlayer->Weapon_ShootPosition();
	Vector forward;
	pPlayer->EyeVectors( &forward );
	Vector vecEndPos = vecMuzzlePos + ( forward * MAX_TRACE_LENGTH );

	trace_t	trace;
	UTIL_TraceLine( vecMuzzlePos, vecEndPos, ( MASK_SHOT & ~CONTENTS_WINDOW ), GetOwner(), COLLISION_GROUP_NONE, &trace );

	// Update the sniper dot.
	if ( m_hSniperDot )
	{
		CBaseEntity *pEntity = NULL;
		if ( trace.DidHitNonWorldEntity() )
		{
			pEntity = trace.m_pEnt;
			if ( !pEntity || !pEntity->m_takedamage )
			{
				pEntity = NULL;
			}
		}

		m_hSniperDot->Update( pEntity, trace.endpos, trace.plane.normal );
	}

#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFSniperRifle::CanFireCriticalShot( bool bIsHeadshot )
{
	// can only fire a crit shot if this is a headshot
	if ( pf_sniper_rifle_tfc_style.GetBool() )
	{
		if ( m_flChargeStarted == -1 )
			return false;
		return bIsHeadshot;
	}

	if (gpGlobals->curtime >= m_flChargeStarted + TF_WEAPON_SNIPERRIFLE_NO_CRIT_AFTER_ZOOM_TIME)
		return bIsHeadshot;
	return false;
}

//=============================================================================
//
// Client specific functions.
//
#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFSniperRifle::GetHUDDamagePerc( void )
{
	return (m_flChargedDamage / m_flDamageMax);
}

//-----------------------------------------------------------------------------
// Returns the sniper chargeup from 0 to 1
//-----------------------------------------------------------------------------
class CProxySniperRifleCharge : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity );
};

void CProxySniperRifleCharge::OnBind( void *pC_BaseEntity )
{
	Assert( m_pResult );

	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( GetSpectatorTarget() != 0 && GetSpectatorMode() == OBS_MODE_IN_EYE )
	{
		pPlayer = (C_TFPlayer *)UTIL_PlayerByIndex( GetSpectatorTarget() );
	}

	if ( pPlayer )
	{
		CTFSniperRifle *pWeapon = assert_cast<CTFSniperRifle*>(pPlayer->GetActiveTFWeapon());
		if ( pWeapon )
		{
			float flChargeValue = ( ( 1.0 - pWeapon->GetHUDDamagePerc() ) * 0.8 ) + 0.6;

			VMatrix mat, temp;

			Vector2D center( 0.5, 0.5 );
			MatrixBuildTranslation( mat, -center.x, -center.y, 0.0f );

			// scale
			{
				Vector2D scale( 1.0f, 0.25f );
				MatrixBuildScale( temp, scale.x, scale.y, 1.0f );
				MatrixMultiply( temp, mat, mat );
			}

			MatrixBuildTranslation( temp, center.x, center.y, 0.0f );
			MatrixMultiply( temp, mat, mat );

			// translation
			{
				Vector2D translation( 0.0f, flChargeValue );
				MatrixBuildTranslation( temp, translation.x, translation.y, 0.0f );
				MatrixMultiply( temp, mat, mat );
			}

			m_pResult->SetMatrixValue( mat );
		}
	}

	if ( ToolsEnabled() )
	{
		ToolFramework_RecordMaterialParams( GetMaterial() );
	}
}

EXPOSE_INTERFACE( CProxySniperRifleCharge, IMaterialProxy, "SniperRifleCharge" IMATERIAL_PROXY_INTERFACE_VERSION );
#endif

//=============================================================================
//
// Laser Dot functions.
//

IMPLEMENT_NETWORKCLASS_ALIASED( SniperDot, DT_SniperDot )

BEGIN_NETWORK_TABLE( CSniperDot, DT_SniperDot )
#ifdef CLIENT_DLL
	RecvPropFloat( RECVINFO( m_flChargeStartTime ) ),
	RecvPropFloat( RECVINFO( m_flChargeTime ) ),
#else
	SendPropTime( SENDINFO( m_flChargeStartTime ) ),
	SendPropTime( SENDINFO( m_flChargeTime ) ),
#endif
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( env_sniperdot, CSniperDot );

BEGIN_DATADESC( CSniperDot )
DEFINE_FIELD( m_vecSurfaceNormal,	FIELD_VECTOR ),
DEFINE_FIELD( m_hTargetEnt,			FIELD_EHANDLE ),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CSniperDot::CSniperDot( void )
{
	m_vecSurfaceNormal.Init();
	m_hTargetEnt = NULL;

#ifdef CLIENT_DLL
	m_hSpriteMaterial = NULL;
#endif

	ResetChargeTime();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
CSniperDot::~CSniperDot( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &origin - 
// Output : CSniperDot
//-----------------------------------------------------------------------------
CSniperDot *CSniperDot::Create( const Vector &origin, CBaseEntity *pOwner, bool bVisibleDot, float flChargeTime )
{
// Client specific.
#ifdef CLIENT_DLL

	return NULL;

// Server specific.
#else

	// Create the sniper dot entity.
	CSniperDot *pDot = static_cast<CSniperDot*>( CBaseEntity::Create( "env_sniperdot", origin, QAngle( 0.0f, 0.0f, 0.0f ) ) );
	if ( !pDot )
		return NULL;

	//Create the graphic
	pDot->SetMoveType( MOVETYPE_NONE );
	pDot->AddSolidFlags( FSOLID_NOT_SOLID );
	pDot->AddEffects( EF_NOSHADOW );
	UTIL_SetSize( pDot, -Vector( 4.0f, 4.0f, 4.0f ), Vector( 4.0f, 4.0f, 4.0f ) );

	// Set owner.
	pDot->SetOwnerEntity( pOwner );

	// Force updates even though we don't have a model.
	pDot->AddEFlags( EFL_FORCE_CHECK_TRANSMIT );

	// For dot intensity
	pDot->m_flChargeTime = flChargeTime;

	return pDot;

#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSniperDot::Update( CBaseEntity *pTarget, const Vector &vecOrigin, const Vector &vecNormal )
{
	SetAbsOrigin( vecOrigin );
	m_vecSurfaceNormal = vecNormal;
	m_hTargetEnt = pTarget;
}

//=============================================================================
//
// Client specific functions.
//
#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
// TFTODO: Make the sniper dot get brighter the more damage it will do.
//-----------------------------------------------------------------------------
int CSniperDot::DrawModel( int flags )
{
	// Get the owning player.
	C_TFPlayer *pPlayer = ToTFPlayer( GetOwnerEntity() );
	if ( !pPlayer )
		return -1;

	// Get the sprite rendering position.
	Vector vecEndPos;

	float flSize = 6.0;

	if ( !pPlayer->IsDormant() )
	{
		Vector vecAttachment, vecDir;
		QAngle angles;

		float flDist = MAX_TRACE_LENGTH;

		// Always draw the dot in front of our faces when in first-person.
		if ( pPlayer->IsLocalPlayer() )
		{
			// Take our view position and orientation
			vecAttachment = CurrentViewOrigin();
			vecDir = CurrentViewForward();

			// Clamp the forward distance for the sniper's firstperson
			flDist = 384;

			flSize = 2.0;
		}
		else
		{
			// Take the owning player eye position and direction.
			vecAttachment = pPlayer->EyePosition();
			QAngle angles = pPlayer->EyeAngles();
			AngleVectors( angles, &vecDir );
		}

		trace_t tr;
		UTIL_TraceLine( vecAttachment, vecAttachment + ( vecDir * flDist ), MASK_SHOT, pPlayer, COLLISION_GROUP_NONE, &tr );

		// Backup off the hit plane, towards the source
		vecEndPos = tr.endpos + vecDir * -4;
	}
	else
	{
		// Just use our position if we can't predict it otherwise.
		vecEndPos = GetAbsOrigin();
	}

	// Draw our laser dot in space.
	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->Bind( m_hSpriteMaterial, this );

	float flLifeTime = gpGlobals->curtime - m_flChargeStartTime;
	float flStrength = RemapValClamped( flLifeTime, 0.0, m_flChargeTime, 0.1, 1.0 );

	color32 innercolor = { 255, 255, 255, 255 };
	color32 outercolor = { 255, 255, 255, 128 };

	DrawSprite( vecEndPos, flSize, flSize, outercolor );
	DrawSprite( vecEndPos, flSize * flStrength, flSize * flStrength, innercolor );

	// Successful.
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSniperDot::ShouldDraw( void )			
{
	if ( IsEffectActive( EF_NODRAW ) )
		return false;

	// Don't draw the sniper dot when in thirdperson.
	if ( ::input->CAM_IsThirdPerson() )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSniperDot::OnDataChanged( DataUpdateType_t updateType )
{
	if ( updateType == DATA_UPDATE_CREATED )
	{
		if ( GetTeamNumber() == TF_TEAM_BLUE )
		{
			m_hSpriteMaterial.Init( SNIPER_DOT_SPRITE_BLUE, TEXTURE_GROUP_CLIENT_EFFECTS );
		}
		else
		{
			m_hSpriteMaterial.Init( SNIPER_DOT_SPRITE_RED, TEXTURE_GROUP_CLIENT_EFFECTS );
		}
	}
}

#endif
