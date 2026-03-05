//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TF_HUD_MEDICCALLERS_H
#define TF_HUD_MEDICCALLERS_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include <game/client/iviewport.h>
#include <vgui/IScheme.h>
#include "hud.h"
#include "hudelement.h"
#include "tf_imagepanel.h"
#include "c_tf_player.h"

enum
{
	DRAW_ARROW_UP,
	DRAW_ARROW_LEFT,
	DRAW_ARROW_RIGHT
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTFMedicCallerPanel : public vgui::EditablePanel, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CTFMedicCallerPanel, vgui::EditablePanel );
public:
	CTFMedicCallerPanel( vgui::Panel *parent, const char *name, int iClass );
	~CTFMedicCallerPanel( void );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void PerformLayout( void );
	virtual void OnTick( void );
	virtual void PaintBackground( void );
	virtual void Paint( void );

	void	GetCallerPosition( const Vector &vecDelta, float flRadius, float *xpos, float *ypos, float *flRotation );
	void	SetPlayer( C_TFPlayer *pPlayer, float flDuration, Vector &vecOffset );
	void	SetType( int iType ) { m_iType = iType; }
	static void AddMedicCaller( C_TFPlayer *pPlayer, float flDuration, Vector &vecOffset, int iType );
	static void AddEngiCaller( C_TFPlayer *pPlayer, float flDuration, Vector &vecOffset, int iType );
	virtual void FireGameEvent( IGameEvent* event );
	void	SetClass( int iClass ) { m_iClass = iClass; }

private:
	IMaterial		*m_pArrowMaterial;
	float			m_flRemoveAt;
	Vector			m_vecOffset;
	CHandle<C_TFPlayer> m_hPlayer;
	int				m_iDrawArrow;
	bool			m_bOnscreen;
	int				m_iType;
	int				m_iClass;
};

#endif // TF_HUD_MEDICCALLERS_H
