//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "hudelement.h"
#include <vgui_controls/Panel.h>
#include <vgui/isurface.h>
#include "c_tf_player.h"
#include "clientmode_tf.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ProgressBar.h>


class CHudSmokeBomb : public CHudElement, public EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE( CHudSmokeBomb, EditablePanel );

	CHudSmokeBomb( const char *name );

	virtual bool ShouldDraw();	
	virtual void Paint();
	virtual void Init();

private:
	CHudTexture *m_pIcon;
	vgui::ContinuousProgressBar *m_pSmokeBombMeter;
};

DECLARE_HUDELEMENT( CHudSmokeBomb );
CHudSmokeBomb::CHudSmokeBomb( const char *pName ) : CHudElement( pName ), BaseClass( NULL, "HudSmokeBomb" )
{
	SetParent( g_pClientMode->GetViewport() );
	m_pIcon = NULL;
	m_pSmokeBombMeter = new ContinuousProgressBar( this, "SmokeBombMeter" );
	SetHiddenBits( HIDEHUD_PLAYERDEAD );
}


void CHudSmokeBomb::Init()
{
}

bool CHudSmokeBomb::ShouldDraw()
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	// if we are spectating another player first person, check this player
	if ( pPlayer && ( pPlayer->GetObserverMode() == OBS_MODE_IN_EYE ) )
	{
		pPlayer = ToTFPlayer( pPlayer->GetObserverTarget() );
	}

	return ( pPlayer && pPlayer->IsAlive() && pPlayer->m_Shared.InCond( TF_COND_SMOKE_BOMB ) );
}

extern ConVar tf_smoke_bomb_time;

void CHudSmokeBomb::Paint()
{
	if ( !m_pIcon )
	{
		m_pIcon = gHUD.GetIcon( "cond_smoke_bomb" );
	}

	int x, y, w, h;
	GetBounds( x, y, w, h );

	if ( m_pIcon )
	{
		m_pIcon->DrawSelf( 0, 0, w, w, Color(255,255,255,255) );
	}

	

	// Draw a progress bar for time remaining
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pPlayer )
		return;
    
	float flExpireTime = pPlayer->m_Shared.GetSmokeBombExpireTime();
	float flPercent = ( flExpireTime - gpGlobals->curtime ) / tf_smoke_bomb_time.GetFloat();
	
	if (m_pSmokeBombMeter)
	{
		m_pSmokeBombMeter->SetProgress( flPercent );
	}
}