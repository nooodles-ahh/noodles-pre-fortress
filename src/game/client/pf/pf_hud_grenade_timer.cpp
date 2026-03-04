//========= Copyright LOLOLOL =================================================//
//
// Purpose: A timer that appears in the same screenspace as the crosshair
//
//=============================================================================//

#include "cbase.h"
#include "hudelement.h"
#include <vgui_controls/Panel.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui/ISurface.h>
#include "clientmode.h"
#include "c_tf_player.h"
#include "mathlib/mathlib.h"
#include "tf_imagepanel.h"
#include "utlvector.h"
#include "view.h"
#include <vgui/IVGui.h>
#include <vgui_controls/ImagePanel.h>
#include "pf_cvars.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define TIMER_FADE_TIME 0.6f

using namespace vgui;

typedef struct
{
	float	m_flDuration;
	float	m_flStartTime;
	int		m_iIndex;
	bool	m_bDropped;
	bool	m_bFading;
}
grenade_timer;


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudPFGrenadeProgressTimer : public CHudElement, public vgui::EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE( CHudPFGrenadeProgressTimer, vgui::EditablePanel );

	CHudPFGrenadeProgressTimer( const char *pElementName );
	~CHudPFGrenadeProgressTimer();

	virtual void OnTick();

	virtual void LevelShutdown( void );

	void SetIcon( const char *szIcon );
	virtual bool ShouldDraw();

	virtual void FireGameEvent( IGameEvent *event );
	void AddDetonationTimer( IGameEvent *event );
	void UpdateTimerIndex( IGameEvent *event );
	void StartTimerFade( IGameEvent *event );

	virtual void Paint();

private:
	int		m_iTexture;

	CUtlVector<grenade_timer> m_vecTimers;
};

ConVar pf_grenade_hud_timer( "pf_grenade_hud_timer", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE );
ConVar pf_grenade_hud_timer_scale( "pf_grenade_hud_timer_scale", "64", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Set the scale of the hud timer", true, 8, true, 128 );

/*#if defined(CLIENT_DLL)
int iPrimed = -1;
CON_COMMAND( pf_grenade_timer_demo, "Creates a fake grenade timer event" )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if( !pPlayer )
		return;
	if( iPrimed < 0 )
	{
		iPrimed = RandomInt( 2, 100 );
		IGameEvent *event = gameeventmanager->CreateEvent( "grenade_primed" );
		if( event )
		{
			event->SetInt( "index", iPrimed );
			event->SetFloat( "time", 3.8f );
			event->SetInt( "player", pPlayer->entindex() );
			gameeventmanager->FireEventClientSide( event );
		}
	}
	else
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "grenade_thrown" );
		if( event )
		{
			event->SetInt( "oldindex", iPrimed );
			event->SetInt( "newindex", iPrimed );
			event->SetInt( "player", pPlayer->entindex() );
			gameeventmanager->FireEventClientSide( event );
		}
		iPrimed = -1;
	}
}
#endif*/

DECLARE_HUDELEMENT( CHudPFGrenadeProgressTimer );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudPFGrenadeProgressTimer::CHudPFGrenadeProgressTimer( const char *pElementName ) :
	CHudElement( pElementName ), BaseClass( NULL, "HudPFGrenadeTimer" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_PLAYERDEAD | HIDEHUD_CROSSHAIR );

	SetPaintBackgroundEnabled( false );

	vgui::ivgui()->AddTickSignal( GetVPanel() );

	SetIcon( "hud/grenade_timer" );
	SetPaintEnabled( false );

	SetForceStereoRenderToFrameBuffer( true );

	int screenWide, screenTall;
	GetHudSize( screenWide, screenTall );
	float timerScale = pf_grenade_hud_timer_scale.GetFloat();
	SetPos( (screenWide / 2) - timerScale / 2, (screenTall / 2) - timerScale / 2 );

	ListenForGameEvent( "grenade_primed" );
	ListenForGameEvent( "grenade_thrown" );
	ListenForGameEvent( "grenade_disarmed" );
	ListenForGameEvent( "localplayer_changeclass" );
	ListenForGameEvent( "localplayer_changeteam" );
	ListenForGameEvent( "player_death" );
}

CHudPFGrenadeProgressTimer::~CHudPFGrenadeProgressTimer()
{
	m_vecTimers.RemoveAll();
}

void CHudPFGrenadeProgressTimer::OnTick( void )
{
	if( m_vecTimers.Count() == 0 )
	{
		SetPaintEnabled( false );
	}
	else
	{
		SetPaintEnabled( true );
	}
}

void CHudPFGrenadeProgressTimer::LevelShutdown( void )
{
	m_vecTimers.RemoveAll();
	CHudElement::LevelShutdown();
}

void CHudPFGrenadeProgressTimer::SetIcon( const char *szIcon )
{
	m_iTexture = vgui::surface()->DrawGetTextureId( szIcon );
	if( m_iTexture == -1 ) // we didn't find it, so create a new one
	{
		m_iTexture = vgui::surface()->CreateNewTextureID();
	}

	vgui::surface()->DrawSetTextureFile( m_iTexture, szIcon, true, false );
}

bool CHudPFGrenadeProgressTimer::ShouldDraw()
{
	if( !pf_grenade_hud_timer.GetBool() )
		return false;
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if( !pPlayer || (pPlayer && !pPlayer->IsAlive()) )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHudPFGrenadeProgressTimer::FireGameEvent( IGameEvent *event )
{
	const char *type = event->GetName();

	if( !Q_strcmp( type, "grenade_primed" ) )
	{
		AddDetonationTimer( event );
	}
	else if( !Q_strcmp( type, "grenade_thrown" ) )
	{
		// need to update the entity so we can deal with disarming
		UpdateTimerIndex( event );
	}
	else if( !Q_strcmp( type, "grenade_disarmed" ) )
	{
		StartTimerFade( event );
	}
	else if( !Q_strcmp( type, "localplayer_changeclass" ) || 
		!Q_strcmp( type, "localplayer_changeteam" ) )
	{
		// remove all our timers if we've changed class or team
		m_vecTimers.RemoveAll();
	}
	else if( !Q_strcmp( type, "player_death" ) )
	{
		// remove all our timers if we died
		C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
		if( pPlayer && pPlayer->GetUserID() == event->GetInt( "userid" ) )
		{
			m_vecTimers.RemoveAll();
		}
	}
	else
	{
		CHudElement::FireGameEvent( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Adds a new timer to our vector
//-----------------------------------------------------------------------------
void CHudPFGrenadeProgressTimer::AddDetonationTimer( IGameEvent *event )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if( pPlayer && pPlayer->IsAlive() )
	{
		// don't create someone else's timer
		if( pPlayer->entindex() != event->GetInt( "player" ) )
			return;

		float flTimer = event->GetFloat( "time" );
		// don't create a timer this short
		if( flTimer <= 0.0f )
			return;

		int iIndex = event->GetInt( "index" );

		grenade_timer gtNew;
		gtNew.m_flDuration = flTimer;
		gtNew.m_flStartTime = gpGlobals->curtime;
		gtNew.m_iIndex = iIndex;
		gtNew.m_bDropped = false;
		gtNew.m_bFading = false;
		m_vecTimers.AddToTail( gtNew );
	}
}

//-----------------------------------------------------------------------------
// Purpose: When we throw the grenade we need to change the relevant timer
//-----------------------------------------------------------------------------
void CHudPFGrenadeProgressTimer::UpdateTimerIndex( IGameEvent *event )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if( pPlayer && pPlayer->IsAlive() )
	{
		// don't update someone else's timer
		if( pPlayer->entindex() != event->GetInt( "player" ) )
			return;

		int iOldIndex = event->GetInt( "oldindex" );
		int iNewIndex = event->GetInt( "newindex" );

		FOR_EACH_VEC( m_vecTimers, i )
		{
			if( m_vecTimers[i].m_iIndex == iOldIndex )
			{
				m_vecTimers[i].m_iIndex = iNewIndex;
				m_vecTimers[i].m_bDropped = true;
				break;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Specifically for fading out disarmed grenades
//-----------------------------------------------------------------------------
void CHudPFGrenadeProgressTimer::StartTimerFade( IGameEvent *event )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if( pPlayer && pPlayer->IsAlive() )
	{
		// don't update someone else's timer
		if( pPlayer->entindex() != event->GetInt( "player" ) )
			return;

		int iIndex = event->GetInt( "index" );

		FOR_EACH_VEC( m_vecTimers, i )
		{
			if( m_vecTimers[i].m_iIndex == iIndex )
			{
				m_vecTimers[i].m_bFading = true;
				m_vecTimers[i].m_flStartTime = gpGlobals->curtime;
				m_vecTimers[i].m_flDuration = TIMER_FADE_TIME;
				break;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudPFGrenadeProgressTimer::Paint()
{
	int wide, tall;
	wide = tall = pf_grenade_hud_timer_scale.GetFloat();
	//GetSize( wide, tall );

	// todo - accessibility conc positioning
	int screenWide, screenTall;
	GetHudSize( screenWide, screenTall );
	SetPos( (screenWide / 2) - wide / 2, (screenTall / 2) - tall / 2 );

	float uv1 = 0.0f, uv2 = 1.0f;
	Vector2D uv11( uv1, uv1 );
	Vector2D uv21( uv2, uv1 );
	Vector2D uv22( uv2, uv2 );
	Vector2D uv12( uv1, uv2 );

	vgui::Vertex_t verts[4];
	verts[0].Init( Vector2D( 0, 0 ), uv11 );
	verts[1].Init( Vector2D( wide, 0 ), uv21 );
	verts[2].Init( Vector2D( wide, tall ), uv22 );
	verts[3].Init( Vector2D( 0, tall ), uv12 );

	vgui::surface()->DrawSetTexture( m_iTexture );
	vgui::surface()->DrawSetColor( 128, 128, 128, 128 );
	vgui::surface()->DrawTexturedPolygon( 4, verts );

	FOR_EACH_VEC( m_vecTimers, i )
	{
		float localTime = gpGlobals->curtime - m_vecTimers[i].m_flStartTime;
		float flMaxTime = m_vecTimers[i].m_flDuration + TIMER_FADE_TIME;

		// if we've hit or passed the timer duration start fading out
		if( !m_vecTimers[i].m_bFading && localTime >= m_vecTimers[i].m_flDuration )
		{
			m_vecTimers[i].m_bFading = true;
		}

		if( (m_vecTimers[i].m_bFading && localTime > flMaxTime) ||
			(!m_vecTimers[i].m_bFading && !cl_entitylist->GetClientEntity( m_vecTimers[i].m_iIndex ) && 
			localTime < m_vecTimers[i].m_flDuration) )
		{
			// remove then decrement
			m_vecTimers.Remove( i-- );
			continue;
		}

		float percent = 0.0f;
		// get a percentage if we're not fading out
		if(!m_vecTimers[i].m_bFading)
		{
			percent = (m_vecTimers[i].m_flDuration - localTime) / m_vecTimers[i].m_flDuration;
			if( percent < 0 )
				percent = 0.0f;
		}

		int r, g, b, a;

		// TODO - add an alternative high contrast colour set
		//if(pf_grenade_hud_timer_accessibility.GetBool())
		//{
		//}
		//else
		{
			// green - yellow - red
			// starts at 89, 232, 31, blue isn't touched
			r = (int)RemapValClamped( percent, 0.6f, 1.0f, 232, 89 );
			g = (int)RemapValClamped( percent, 0.1f, 0.6f, 31, 232 );
			b = 31;
			// -0.15 is about TIMER_FADE_TIME/3.8
			if( m_vecTimers[i].m_bDropped )
				a = 96;
			else
				a = 255;
		}

		// if we're fading change the transparency
		if( m_vecTimers[i].m_bFading )
		{
			a = (int)RemapValClamped( localTime, m_vecTimers[i].m_flDuration, flMaxTime, a, 0 );
		}

		vgui::surface()->DrawSetColor( r, g, b, a );


		// we're going to do this using quadrants
		//  -------------------------
		//  |           |           |
		//  |           |           |
		//  |     4     |     1     |
		//  |           |           |
		//  |           |           |
		//  -------------------------
		//  |           |           |
		//  |           |           |
		//  |     3     |     2     |
		//  |           |           |
		//  |           |           |
		//  -------------------------

		float flCompleteCircle = (2.0f * M_PI);
		float fl90degrees = flCompleteCircle / 4.0f;

		float flEndAngle = flCompleteCircle * (1.0f - percent); // count DOWN (counter-clockwise)
		//float flEndAngle = flCompleteCircle * m_flPercent; // count UP (clockwise)

		float flHalfWide = (float)wide / 2.0f;
		float flHalfTall = (float)tall / 2.0f;

		if( flEndAngle >= fl90degrees * 3.0f ) // >= 270 degrees
		{
			// draw the first and second quadrants
			uv11.Init( 0.5f, 0.0f );
			uv21.Init( 1.0f, 0.0f );
			uv22.Init( 1.0f, 1.0f );
			uv12.Init( 0.5, 1.0f );

			verts[0].Init( Vector2D( flHalfWide, 0.0f ), uv11 );
			verts[1].Init( Vector2D( wide, 0.0f ), uv21 );
			verts[2].Init( Vector2D( wide, tall ), uv22 );
			verts[3].Init( Vector2D( flHalfWide, tall ), uv12 );

			vgui::surface()->DrawTexturedPolygon( 4, verts );

			// draw the third quadrant
			uv11.Init( 0.0f, 0.5f );
			uv21.Init( 0.5f, 0.5f );
			uv22.Init( 0.5f, 1.0f );
			uv12.Init( 0.0f, 1.0f );

			verts[0].Init( Vector2D( 0.0f, flHalfTall ), uv11 );
			verts[1].Init( Vector2D( flHalfWide, flHalfTall ), uv21 );
			verts[2].Init( Vector2D( flHalfWide, tall ), uv22 );
			verts[3].Init( Vector2D( 0.0f, tall ), uv12 );

			vgui::surface()->DrawTexturedPolygon( 4, verts );

			// draw the partial fourth quadrant
			if( flEndAngle > fl90degrees * 3.5f ) // > 315 degrees
			{
				uv11.Init( 0.0f, 0.0f );
				uv21.Init( 0.5f - (tan( fl90degrees * 4.0f - flEndAngle ) * 0.5), 0.0f );
				uv22.Init( 0.5f, 0.5f );
				uv12.Init( 0.0f, 0.5f );

				verts[0].Init( Vector2D( 0.0f, 0.0f ), uv11 );
				verts[1].Init( Vector2D( flHalfWide - (tan( fl90degrees * 4.0f - flEndAngle ) * flHalfTall), 0.0f ), uv21 );
				verts[2].Init( Vector2D( flHalfWide, flHalfTall ), uv22 );
				verts[3].Init( Vector2D( 0.0f, flHalfTall ), uv12 );

				vgui::surface()->DrawTexturedPolygon( 4, verts );
			}
			else // <= 315 degrees
			{
				uv11.Init( 0.0f, 0.5f );
				uv21.Init( 0.0f, 0.5f - (tan( flEndAngle - fl90degrees * 3.0f ) * 0.5) );
				uv22.Init( 0.5f, 0.5f );
				uv12.Init( 0.0f, 0.5f );

				verts[0].Init( Vector2D( 0.0f, flHalfTall ), uv11 );
				verts[1].Init( Vector2D( 0.0f, flHalfTall - (tan( flEndAngle - fl90degrees * 3.0f ) * flHalfWide) ), uv21 );
				verts[2].Init( Vector2D( flHalfWide, flHalfTall ), uv22 );
				verts[3].Init( Vector2D( 0.0f, flHalfTall ), uv12 );

				vgui::surface()->DrawTexturedPolygon( 4, verts );
			}
		}
		else if( flEndAngle >= fl90degrees * 2.0f ) // >= 180 degrees
		{
			// draw the first and second quadrants
			uv11.Init( 0.5f, 0.0f );
			uv21.Init( 1.0f, 0.0f );
			uv22.Init( 1.0f, 1.0f );
			uv12.Init( 0.5, 1.0f );

			verts[0].Init( Vector2D( flHalfWide, 0.0f ), uv11 );
			verts[1].Init( Vector2D( wide, 0.0f ), uv21 );
			verts[2].Init( Vector2D( wide, tall ), uv22 );
			verts[3].Init( Vector2D( flHalfWide, tall ), uv12 );

			vgui::surface()->DrawTexturedPolygon( 4, verts );

			// draw the partial third quadrant
			if( flEndAngle > fl90degrees * 2.5f ) // > 225 degrees
			{
				uv11.Init( 0.5f, 0.5f );
				uv21.Init( 0.5f, 1.0f );
				uv22.Init( 0.0f, 1.0f );
				uv12.Init( 0.0f, 0.5f + (tan( fl90degrees * 3.0f - flEndAngle ) * 0.5) );

				verts[0].Init( Vector2D( flHalfWide, flHalfTall ), uv11 );
				verts[1].Init( Vector2D( flHalfWide, tall ), uv21 );
				verts[2].Init( Vector2D( 0.0f, tall ), uv22 );
				verts[3].Init( Vector2D( 0.0f, flHalfTall + (tan( fl90degrees * 3.0f - flEndAngle ) * flHalfWide) ), uv12 );

				vgui::surface()->DrawTexturedPolygon( 4, verts );
			}
			else // <= 225 degrees
			{
				uv11.Init( 0.5f, 0.5f );
				uv21.Init( 0.5f, 1.0f );
				uv22.Init( 0.5f - (tan( flEndAngle - fl90degrees * 2.0f ) * 0.5), 1.0f );
				uv12.Init( 0.5f, 0.5f );

				verts[0].Init( Vector2D( flHalfWide, flHalfTall ), uv11 );
				verts[1].Init( Vector2D( flHalfWide, tall ), uv21 );
				verts[2].Init( Vector2D( flHalfWide - (tan( flEndAngle - fl90degrees * 2.0f ) * flHalfTall), tall ), uv22 );
				verts[3].Init( Vector2D( flHalfWide, flHalfTall ), uv12 );

				vgui::surface()->DrawTexturedPolygon( 4, verts );
			}
		}
		else if( flEndAngle >= fl90degrees ) // >= 90 degrees
		{
			// draw the first quadrant
			uv11.Init( 0.5f, 0.0f );
			uv21.Init( 1.0f, 0.0f );
			uv22.Init( 1.0f, 0.5f );
			uv12.Init( 0.5f, 0.5f );

			verts[0].Init( Vector2D( flHalfWide, 0.0f ), uv11 );
			verts[1].Init( Vector2D( wide, 0.0f ), uv21 );
			verts[2].Init( Vector2D( wide, flHalfTall ), uv22 );
			verts[3].Init( Vector2D( flHalfWide, flHalfTall ), uv12 );

			vgui::surface()->DrawTexturedPolygon( 4, verts );

			// draw the partial second quadrant
			if( flEndAngle > fl90degrees * 1.5f ) // > 135 degrees
			{
				uv11.Init( 0.5f, 0.5f );
				uv21.Init( 1.0f, 0.5f );
				uv22.Init( 1.0f, 1.0f );
				uv12.Init( 0.5f + (tan( fl90degrees * 2.0f - flEndAngle ) * 0.5f), 1.0f );

				verts[0].Init( Vector2D( flHalfWide, flHalfTall ), uv11 );
				verts[1].Init( Vector2D( wide, flHalfTall ), uv21 );
				verts[2].Init( Vector2D( wide, tall ), uv22 );
				verts[3].Init( Vector2D( flHalfWide + (tan( fl90degrees * 2.0f - flEndAngle ) * flHalfTall), tall ), uv12 );

				vgui::surface()->DrawTexturedPolygon( 4, verts );
			}
			else // <= 135 degrees
			{
				uv11.Init( 0.5f, 0.5f );
				uv21.Init( 1.0f, 0.5f );
				uv22.Init( 1.0f, 0.5f + (tan( flEndAngle - fl90degrees ) * 0.5f) );
				uv12.Init( 0.5f, 0.5f );

				verts[0].Init( Vector2D( flHalfWide, flHalfTall ), uv11 );
				verts[1].Init( Vector2D( wide, flHalfTall ), uv21 );
				verts[2].Init( Vector2D( wide, flHalfTall + (tan( flEndAngle - fl90degrees ) * flHalfWide) ), uv22 );
				verts[3].Init( Vector2D( flHalfWide, flHalfTall ), uv12 );

				vgui::surface()->DrawTexturedPolygon( 4, verts );
			}
		}
		else // > 0 degrees
		{
			if( flEndAngle > fl90degrees / 2.0f ) // > 45 degrees
			{
				uv11.Init( 0.5f, 0.0f );
				uv21.Init( 1.0f, 0.0f );
				uv22.Init( 1.0f, 0.5f - (tan( fl90degrees - flEndAngle ) * 0.5) );
				uv12.Init( 0.5f, 0.5f );

				verts[0].Init( Vector2D( flHalfWide, 0.0f ), uv11 );
				verts[1].Init( Vector2D( wide, 0.0f ), uv21 );
				verts[2].Init( Vector2D( wide, flHalfTall - (tan( fl90degrees - flEndAngle ) * flHalfWide) ), uv22 );
				verts[3].Init( Vector2D( flHalfWide, flHalfTall ), uv12 );

				vgui::surface()->DrawTexturedPolygon( 4, verts );
			}
			else // <= 45 degrees
			{
				uv11.Init( 0.5f, 0.0f );
				uv21.Init( 0.5 + (tan( flEndAngle ) * 0.5), 0.0f );
				uv22.Init( 0.5f, 0.5f );
				uv12.Init( 0.5f, 0.0f );

				verts[0].Init( Vector2D( flHalfWide, 0.0f ), uv11 );
				verts[1].Init( Vector2D( flHalfWide + (tan( flEndAngle ) * flHalfTall), 0.0f ), uv21 );
				verts[2].Init( Vector2D( flHalfWide, flHalfTall ), uv22 );
				verts[3].Init( Vector2D( flHalfWide, 0.0f ), uv12 );

				vgui::surface()->DrawTexturedPolygon( 4, verts );
			}
		}
	}
}