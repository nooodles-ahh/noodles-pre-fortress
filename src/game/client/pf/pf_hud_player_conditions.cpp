//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "hudelement.h"
#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>
#include "c_tf_player.h"
#include "clientmode_tf.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ImagePanel.h>

using namespace vgui;

struct s_Conditions
{
	const char *pImageName;
	int			iCondition;
};

static s_Conditions s_CondImages[] =
{
	{ "ConditionInfectedImage", TF_COND_INFECTED },
	{ "ConditionDizzyImage", TF_COND_DIZZY },
	{ "ConditionPoisonedImage", TF_COND_HALLUCINATING },
	{ "ConditionTranqedImage", TF_COND_TRANQUILIZED },
	{ "ConditionSlowedImage", TF_COND_LEG_DAMAGED },
	//{ "ConditionOnFireImage", TF_COND_BURNING }
};

class CHudPlayerConditions : public CHudElement, public EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE( CHudPlayerConditions, EditablePanel );

	CHudPlayerConditions( const char *name );

	virtual void ApplySchemeSettings( IScheme *pScheme );
	virtual void LevelInit( void );
	virtual bool ShouldDraw();
	virtual void Paint();
	virtual void OnThink();

private:
	ImagePanel *m_pCondImages[ARRAYSIZE( s_CondImages )];
	bool m_bCondStates[ARRAYSIZE( s_CondImages )];

	float m_flNextThink;
};

DECLARE_HUDELEMENT( CHudPlayerConditions );
CHudPlayerConditions::CHudPlayerConditions( const char *pName ) : CHudElement( pName ), BaseClass( NULL, "HudPlayerConditions" )
{
	SetParent( g_pClientMode->GetViewport() );
	SetHiddenBits( HIDEHUD_PLAYERDEAD );
	for( int i = 0; i < ARRAYSIZE( s_CondImages ); ++i )
	{
		m_pCondImages[i] = new ImagePanel( this, s_CondImages[i].pImageName );
	}

	m_flNextThink = 0.0f;
}

void CHudPlayerConditions::LevelInit()
{
	m_flNextThink = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHudPlayerConditions::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	LoadControlSettings( "resource/UI/HudPlayerConditions.res" );

	BaseClass::ApplySchemeSettings( pScheme );
}

bool CHudPlayerConditions::ShouldDraw()
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	// if we are spectating another player first person, check this player
	if( pPlayer && (pPlayer->GetObserverMode() == OBS_MODE_IN_EYE) )
	{
		pPlayer = ToTFPlayer( pPlayer->GetObserverTarget() );
	}

	return (pPlayer && pPlayer->IsAlive());
}

void CHudPlayerConditions::OnThink()
{
	C_TFPlayer *pPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );

	if( pPlayer )
	{
		for( int i = 0; i < ARRAYSIZE( s_CondImages ); ++i )
			m_bCondStates[i] = pPlayer->m_Shared.InCond( s_CondImages[i].iCondition );
	}
	m_flNextThink = gpGlobals->curtime + 0.1f;
}


void CHudPlayerConditions::Paint()
{
	int i;
	int total = 0;
	for( i = 0; i < ARRAYSIZE( s_CondImages ); ++i )
	{
		if( m_bCondStates[i] != m_pCondImages[i]->IsVisible() )
			m_pCondImages[i]->SetVisible( m_bCondStates[i] );
		total += m_bCondStates[i];
	}
	if( total > 0 )
	{
		int _x, _y, _width, _tall;
		GetBounds( _x, _y, _width, _tall );

		// int iStart = ( int )( ( _width / 2.0f ) - ( ( _tall * total ) * 0.5f ) ); // Horizonal
		// Start from the bottom of the panel and fill up
		int iStart = _tall - _width; // icons are squares and should be the same width as the panel
		for( i = 0; i < ARRAYSIZE( s_CondImages ); ++i )
		{
			if( m_bCondStates[i] )
			{
				m_pCondImages[i]->SetPos( 0, iStart );
				// always want it to be at least slightly visible
				int iAlpha = (int)(fabs( sin( gpGlobals->curtime * 2 ) ) * 246.0) + 10;
				m_pCondImages[i]->SetAlpha( iAlpha );
				iStart -= _width;
			}
		}
	}
}
