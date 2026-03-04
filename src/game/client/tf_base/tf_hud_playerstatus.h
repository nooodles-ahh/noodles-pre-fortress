//========= Copyright � 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_HUD_PLAYERSTATUS_H
#define TF_HUD_PLAYERSTATUS_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/ImagePanel.h>
#include "tf_controls.h"
#include "tf_imagepanel.h"
#include "GameEventListener.h"

//-----------------------------------------------------------------------------
// Purpose:  
//-----------------------------------------------------------------------------
class CTFClassImage : public vgui::ImagePanel
{
public:
	DECLARE_CLASS_SIMPLE( CTFClassImage, vgui::ImagePanel );

	CTFClassImage( vgui::Panel *parent, const char *name ) : ImagePanel( parent, name )
	{
	}

	void SetClass( int iTeam, int iClass, int iCloakstate );
};

//-----------------------------------------------------------------------------
// Purpose:  Displays player class data
//-----------------------------------------------------------------------------
class CTFHudPlayerClass : public vgui::EditablePanel, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CTFHudPlayerClass, EditablePanel );

public:

	CTFHudPlayerClass( Panel *parent, const char *name );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void Reset();

public: // IGameEventListener Interface
	virtual void FireGameEvent( IGameEvent * event );

protected:

	virtual void OnThink();

private:

	float				m_flNextThink;

	CTFClassImage		*m_pClassImage;
	CTFImagePanel		*m_pSpyImage; // used when spies are disguised
	CTFImagePanel		*m_pSpyOutlineImage;

	int					m_nTeam;
	int					m_nClass;
	int					m_nDisguiseTeam;
	int					m_nDisguiseClass;
	int					m_nCloakLevel;
};

//-----------------------------------------------------------------------------
// Purpose:  Clips the health image to the appropriate percentage
//-----------------------------------------------------------------------------
class CTFHealthPanel : public vgui::Panel
{
public:
	DECLARE_CLASS_SIMPLE(CTFHealthPanel, vgui::Panel );

	CTFHealthPanel( vgui::Panel *parent, const char *name);
	virtual void Paint();
	void SetPercentage( float flPercent ){ m_flPercent = (flPercent <= 1.0 ) ? flPercent : 1.0f; }
	virtual const char *GetHealthMaterialName( void ) { return "hud/health_color"; }

private:

	float	m_flPercent; // percentage from 0.0 -> 1.0
	int		m_iMaterialIndex;
	int		m_iDeadMaterialIndex;
};

//-----------------------------------------------------------------------------
// Purpose:  Displays player health data
//-----------------------------------------------------------------------------
class CTFHudPlayerHealth : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTFHudPlayerHealth, EditablePanel );

public:

	CTFHudPlayerHealth( Panel *parent, const char *name );

	virtual const char *GetResFilename( void ) { return "resource/UI/HudPlayerHealth.res"; }
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void Reset();

	void	SetHealth( int iNewHealth, int iMaxHealth, int iMaxBuffedHealth );
	void	HideHealthBonusImage( void );

protected:

	virtual void OnThink();

protected:
	float				m_flNextThink;

private:
	CTFHealthPanel		*m_pHealthImage;
	vgui::ImagePanel	*m_pHealthBonusImage;
	vgui::ImagePanel	*m_pHealthImageBG;
	vgui::ImagePanel	*m_pHealthImageBuildingBG;

	int					m_nHealth;
	int					m_nMaxHealth;

	int					m_nBonusHealthOrigX;
	int					m_nBonusHealthOrigY;
	int					m_nBonusHealthOrigW;
	int					m_nBonusHealthOrigH;

	CPanelAnimationVar( int, m_nHealthBonusPosAdj, "HealthBonusPosAdj", "25" );
	CPanelAnimationVar( float, m_flHealthDeathWarning, "HealthDeathWarning", "0.49" );
	CPanelAnimationVar( Color, m_clrHealthDeathWarningColor, "HealthDeathWarningColor", "HUDDeathWarning" );
};

#ifdef PF2_CLIENT
//-----------------------------------------------------------------------------
// Purpose:  Clips the health image to the appropriate percentage
//-----------------------------------------------------------------------------
class CTFArmorPanel : public vgui::Panel
{
public:
	DECLARE_CLASS_SIMPLE(CTFArmorPanel, vgui::Panel );

	CTFArmorPanel(vgui::Panel* parent, const char* name, bool m_bInvert = false);
	virtual void Paint();
	void SetPercentage(float flPercent) { m_flPercent = (flPercent <= 1.0) ? flPercent : 1.0f; }
	virtual const char* GetArmorMaterialName(void) { return "hud/armor_color"; }
	void SetArmorMaterial( int iTeam, int iType );

private:

	bool	m_bInvert;
	float	m_flPercent; // percentage from 0.0 -> 1.0
	int		m_iMaterialIndex;
	const char* m_pszArmorMaterial;
};

//-----------------------------------------------------------------------------
// Purpose:  Displays player health data
//-----------------------------------------------------------------------------
class CTFHudPlayerArmor : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE(CTFHudPlayerArmor, EditablePanel);

public:

	CTFHudPlayerArmor(Panel* parent, const char* name);

	virtual const char* GetResFilename(void) { return "resource/UI/HudPlayerArmor.res"; }
	virtual void ApplySchemeSettings(vgui::IScheme* pScheme);
	virtual void Reset();

	void	SetArmor(int iNewArmor, int iMaxArmor, bool bMelting = false);
	void	SetArmorImageTeam( int iTeam, int iType );

protected:

	virtual void OnThink();

protected:
	float				m_flNextThink;

private:
	CTFArmorPanel* m_pArmorImage;
	CTFArmorPanel* m_pArmorWarningImage;
	vgui::ImagePanel* m_pArmorImageBG;

	int					m_nArmor;
	int					m_nMaxArmor;

	int					m_nBonusHealthOrigX;
	int					m_nBonusHealthOrigY;
	int					m_nBonusHealthOrigW;
	int					m_nBonusHealthOrigH;

	int					m_nTeam;
	int					m_nType;
	
	CPanelAnimationVar(Color, m_clrArmorDeathWarningColor, "ArmorDeathWarningColor", "HUDDeathWarning");
	CPanelAnimationVar(Color, m_clrArmorMeltingColor, "ArmorMeltingColor", "HUDDeathWarning");
};
#endif

//-----------------------------------------------------------------------------
// Purpose:  Parent panel for the player class/health displays
//-----------------------------------------------------------------------------
class CTFHudPlayerStatus : public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTFHudPlayerStatus, vgui::EditablePanel );

public:
	CTFHudPlayerStatus( const char *pElementName );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void Reset();

#ifdef PF2_CLIENT
public: // IGameEventListener Interface
	virtual void FireGameEvent( IGameEvent *event );
#endif

private:

	CTFHudPlayerClass	*m_pHudPlayerClass;
	CTFHudPlayerHealth	*m_pHudPlayerHealth;
#ifdef PF2_CLIENT
	CTFHudPlayerArmor*	m_pHudPlayerArmor;
#endif
};

#endif	// TF_HUD_PLAYERSTATUS_H