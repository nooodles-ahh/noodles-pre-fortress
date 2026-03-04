//========= Copyright � 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_HUD_AMMOSTATUS_H
#define TF_HUD_AMMOSTATUS_H
#ifdef _WIN32
#pragma once
#endif
#ifdef USE_INVENTORY
#include "tf_inventory.h"
#endif

#define TF_MAX_GRENADES			4
#define TF_MAX_FILENAME_LENGTH	128

//-----------------------------------------------------------------------------
// Purpose:  Displays weapon ammo data
//-----------------------------------------------------------------------------
class CTFHudWeaponAmmo : public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTFHudWeaponAmmo, vgui::EditablePanel );

public:

	CTFHudWeaponAmmo( const char *pElementName );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void Reset();

	virtual bool ShouldDraw( void );

protected:

	virtual void OnThink();

private:
	
	void UpdateAmmoLabels( bool bPrimary, bool bReserve, bool bNoClip );
	void ShowLowAmmoIndicator();

private:

	float							m_flNextThink;
	CTFInventory					*Inventory;

	CHandle<C_BaseCombatWeapon>		m_hCurrentActiveWeapon;
	int								m_nAmmo;
	int								m_nAmmo2;

	CExLabel						*m_pInClip;
	CExLabel						*m_pInClipShadow;
	CExLabel						*m_pInReserve;
	CExLabel						*m_pInReserveShadow;
	CExLabel						*m_pNoClip;
	CExLabel						*m_pNoClipShadow;

	vgui::ImagePanel						*m_pLowAmmoImage;
	Rect_t							rectAmmoBounds;
};

//-----------------------------------------------------------------------------
// Purpose:  Displays grenade ammo data
//-----------------------------------------------------------------------------
class CTFHudGrenadeAmmo : public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTFHudGrenadeAmmo, vgui::EditablePanel );

public:

	CTFHudGrenadeAmmo( const char *pElementName );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void Reset();

	virtual bool ShouldDraw( void );

protected:

	virtual void OnThink();

private:
	void UpdateGrenadesLabels( bool bGrenades1, bool bGrenades2 );

private:

	float							m_flNextThink;
	CTFInventory					*Inventory;

	int								m_nGrenadeCount[2];
	int								m_nGrenadeIDs[2];
	int								m_nGrenadeOldIDs[2];

	CExLabel						*m_pGrenade[2];
	CExLabel						*m_pGrenadeShadow[2];
	CTFImagePanel					*m_pGrenadeBG[2];
	vgui::ImagePanel				*m_pGrenadeImage[2];
	vgui::ImagePanel				*m_pGrenadeImageShadow[2];
};

//-----------------------------------------------------------------------------
// Purpose:  Displays grenade ammo regeneration data
//-----------------------------------------------------------------------------
class CTFHudGrenadeAmmoRegen : public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTFHudGrenadeAmmoRegen, vgui::EditablePanel );

public:

	CTFHudGrenadeAmmoRegen( const char *pElementName );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void Reset();
	virtual void Paint();

	virtual bool ShouldDraw( void );

protected:

	virtual void OnThink();

private:
	void UpdateGrenadeMeters( bool bGrenades1, bool bGrenades2 );

private:

	float							m_flNextThink;
	CTFInventory					*Inventory;

	vgui::ContinuousProgressBar *m_pGrenades1Meter;
	vgui::ContinuousProgressBar *m_pGrenades2Meter;
};

#endif	// TF_HUD_AMMOSTATUS_H