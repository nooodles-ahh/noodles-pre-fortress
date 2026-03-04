//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "hudelement.h"
#include <vgui_controls/Panel.h>
#include "c_tf_player.h"
#include "clientmode_tf.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ProgressBar.h>


class CHudDetpackConstruction : public CHudElement, public EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE( CHudDetpackConstruction, EditablePanel );

	CHudDetpackConstruction( const char *name );

	virtual bool ShouldDraw();	
	virtual void Init();
	virtual void ApplySchemeSettings( IScheme* scheme );

private:
	CHudTexture *m_pIcon;
	vgui::ContinuousProgressBar *m_pSmokeBombMeter;
};

DECLARE_HUDELEMENT( CHudDetpackConstruction );
CHudDetpackConstruction::CHudDetpackConstruction( const char *pName ) : CHudElement( pName ), BaseClass( NULL, "HudDetpackConstruction" )
{
	SetParent( g_pClientMode->GetViewport() );
	SetHiddenBits( HIDEHUD_PLAYERDEAD );
}


void CHudDetpackConstruction::Init()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudDetpackConstruction::ApplySchemeSettings( IScheme* pScheme )
{
	// load control settings...
	LoadControlSettings( "resource/UI/HudDetpackConstruction.res" );

	BaseClass::ApplySchemeSettings( pScheme );
}

bool CHudDetpackConstruction::ShouldDraw()
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	// if we are spectating another player first person, check this player
	if ( pPlayer && ( pPlayer->GetObserverMode() == OBS_MODE_IN_EYE ) )
	{
		pPlayer = ToTFPlayer( pPlayer->GetObserverTarget() );
	}

	return ( pPlayer && pPlayer->IsAlive() && pPlayer->m_Shared.InCond( TF_COND_BUILDING_DETPACK ) );
}
