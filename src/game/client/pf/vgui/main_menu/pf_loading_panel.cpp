//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"

#include <vgui_controls/Label.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include <vgui/IInput.h>
#include <vgui_controls/ImagePanel.h>
#include "ienginevgui.h"
#include "pf_loading_panel.h"
#include "filesystem.h"
#include "fmtstr.h"
#include "pf_mainmenu_override.h"
#include "tf_statsummary.h"
#include "tf_tips.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

CPFLoadingPanel *g_pPFLoadingPanel = NULL;

const char *g_pszTipsClassImagesLoading[]
{
	"",
	"class_portraits/scout",
	"class_portraits/sniper",
	"class_portraits/soldier",
	"class_portraits/demoman",
	"class_portraits/medic",
	"class_portraits/heavy",
	"class_portraits/pyro",
	"class_portraits/spy",
	"class_portraits/engineer",
};
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPFLoadingPanel *GPFLoadingPanel()
{
	if( NULL == g_pPFLoadingPanel )
	{
		g_pPFLoadingPanel = new CPFLoadingPanel();
	}
	return g_pPFLoadingPanel;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CPFLoadingPanel::CPFLoadingPanel( vgui::Panel *parent ) : vgui::EditablePanel( parent, "PFLoadingPanel",
	vgui::scheme()->LoadSchemeFromFile( "Resource/ClientScheme.res", "ClientScheme" ) )
{
	SetParent( parent );
	SetProportional( true );
	SetVisible( true );

	m_pMapPanelBGs[0] = new vgui::EditablePanel( this, "BGTransparentPanel" );
	m_pMapPanelBGs[1] = new vgui::EditablePanel( this, "BGPanel" );
	m_pOnYourWayLabel = new CExLabel( this, "OnYourWayLabel", "" );
	
	m_pMapNameLabel = new CExLabel( this, "MapLabel", "" );
	m_pTipText = new vgui::Label( this, "TipText", "" );
	m_pTipImage = new vgui::ImagePanel( this, "TipImage" );

	ListenForGameEvent( "server_spawn" );
}

//-----------------------------------------------------------------------------
// Purpose: Applies scheme settings
//-----------------------------------------------------------------------------
void CPFLoadingPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	SetProportional( true );
	LoadControlSettings( "Resource/UI/main_menu/loadingpanel.res" );

	ImagePanel *pImagePanel = dynamic_cast<ImagePanel *>(FindChildByName( "MainBackground" ));
	if( pImagePanel )
	{
		char szBGName[128];
		engine->GetMainMenuBackgroundName( szBGName, sizeof( szBGName ) );
		char szImage[128];
		Q_snprintf( szImage, sizeof( szImage ), "../console/%s", szBGName );
		// determine if we're in widescreen or not and select the appropriate image
		int screenWide, screenTall;
		surface()->GetScreenSize( screenWide, screenTall );
		float aspectRatio = (float)screenWide / (float)screenTall;
		bool bIsWidescreen = aspectRatio >= 1.6f;
		if( bIsWidescreen )
			Q_strcat( szImage, "_widescreen", sizeof( szImage ) );

		pImagePanel->SetImage( szImage );
	}
}

extern const char *GetMapDisplayName( const char *mapName );

//-----------------------------------------------------------------------------
// Purpose: Event handler
//-----------------------------------------------------------------------------
void CPFLoadingPanel::FireGameEvent( IGameEvent *event )
{
	const char *pEventName = event->GetName();

	// when we are changing levels and 
	if( 0 == Q_strcmp( pEventName, "server_spawn" ) )
	{
		const char *pMapName = event->GetString( "mapname" );
		if( pMapName )
		{
			// If we're loading a background map, don't display anything
			// HACK: Client doesn't get gpGlobals->eLoadType, so just do string compare for now.
			if( Q_stristr( pMapName, "background" ) )
			{
				// ClearMapLabel();
			}
			else
			{
				// set the map name in the UI
				wchar_t wzMapName[255] = L"";
				g_pVGuiLocalize->ConvertANSIToUnicode( GetMapDisplayName( pMapName ), wzMapName, sizeof( wzMapName ) );

				SetDialogVariable( "maplabel", wzMapName );
				SetMapImage( pMapName );

				m_pOnYourWayLabel->SetVisible( true );
				m_pMapNameLabel->SetVisible( true );

			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Updates the dialog
//-----------------------------------------------------------------------------
void CPFLoadingPanel::ClearMapLabel()
{
	SetDialogVariable( "maplabel", "" );
	
	m_pOnYourWayLabel->SetVisible( false );
	m_pMapNameLabel->SetVisible( false );
}

//-----------------------------------------------------------------------------
// Purpose: Called when we are activated during level load
//-----------------------------------------------------------------------------
void CPFLoadingPanel::OnActivate()
{
	UpdateTip();
	ClearMapLabel();
}

//-----------------------------------------------------------------------------
// Purpose: Called when we are deactivated at end of level load
//-----------------------------------------------------------------------------
void CPFLoadingPanel::OnDeactivate()
{
	ClearMapLabel();
}

void CPFLoadingPanel::SetMapImage( const char *mapName )
{
	// load the map image (if it exists for the current map)
	/*char szMapImage[MAX_PATH];
	Q_snprintf( szMapImage, sizeof( szMapImage ), "vgui/maps/menu_thumb_%s", mapName );
	Q_strlower( szMapImage );

	IMaterial *pMapMaterial = materials->FindMaterial( szMapImage, TEXTURE_GROUP_VGUI, false );
	if( pMapMaterial && !IsErrorMaterial( pMapMaterial ) )
	{
		if( m_pMapImage )
		{
			if( !m_pMapImage->IsVisible() )
			{
				m_pMapImage->SetVisible( true );
			}

			// take off the vgui/ at the beginning when we set the image
			Q_snprintf( szMapImage, sizeof( szMapImage ), "maps/menu_thumb_%s", mapName );
			Q_strlower( szMapImage );

			m_pMapImage->SetImage( szMapImage );
		}
	}
	else
	{
		if( m_pMapImage && m_pMapImage->IsVisible() )
		{
			m_pMapImage->SetVisible( false );
		}
	}*/
}
void CPFLoadingPanel::UpdateTip()
{
	int iClass = TF_CLASS_UNDEFINED;
	SetDialogVariable( "tiptext", g_TFTips.GetRandomTip( iClass ) );
	
	m_pTipImage->SetImage( g_pszTipsClassImagesLoading[iClass] );
	m_pTipImage->SetVisible( true );
}