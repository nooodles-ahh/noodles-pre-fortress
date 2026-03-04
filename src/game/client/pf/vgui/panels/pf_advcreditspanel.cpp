//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"

#include <vgui_controls/Label.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/PanelListPanel.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include <vgui/IInput.h>
#include "ienginevgui.h"
#include "controls/pf_scriptobject.h"
#include "pf_advcreditspanel.h"
#include "filesystem.h"
#include "fmtstr.h"
#include "pf_webstuff.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

CPFAdvCreditsPanel *g_pPFAdvCreditsPanel = NULL;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPFAdvCreditsPanel *GPFAdvCreditsPanel()
{
	if( NULL == g_pPFAdvCreditsPanel )
	{
		g_pPFAdvCreditsPanel = new CPFAdvCreditsPanel( nullptr );
	}
	return g_pPFAdvCreditsPanel;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CPFAdvCreditsPanel::CPFAdvCreditsPanel( vgui::Panel *parent ) : BaseClass( parent, "PFAdvCreditsPanel", false, true )
{
	vgui::VPANEL pParent = enginevgui->GetPanel( PANEL_GAMEUIDLL );
	SetParent( pParent );

	SetMoveable( false );
	SetSizeable( false );
	MakePopup( true );
	SetDeleteSelfOnClose(true);

	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme" );
	SetScheme( scheme );
	SetProportional( true );

	m_pListPanel = new PanelListPanel( this, "PanelListPanel" );

	m_pCreditsKV = nullptr;

	InvalidateLayout( true, true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPFAdvCreditsPanel::~CPFAdvCreditsPanel()
{
	g_pPFAdvCreditsPanel = nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPFAdvCreditsPanel::PerformLayout()
{
	BaseClass::PerformLayout();
}


//-----------------------------------------------------------------------------
// Purpose: Applies scheme settings
//-----------------------------------------------------------------------------
void CPFAdvCreditsPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	SetProportional( true );
	LoadControlSettings( "Resource/UI/creditsdialog.res" );
	m_pListPanel->SetFirstColumnWidth(0);

	
	if( GetWebStuff()->HasCredits() )
	{
		KeyValues *webCredits = GetWebStuff()->GetCredits();
		m_pCreditsKV = webCredits;
	}
	else
	{
		KeyValues *kv = new KeyValues( "Credits" );
		if( kv->LoadFromFile( filesystem, "scripts/credits.txt", "MOD" ) )
			m_pCreditsKV = kv;
	}

	CreateControls();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CPFAdvCreditsPanel::OnKeyCodeTyped(vgui::KeyCode code)
{
	if ( code == KEY_ESCAPE )
	{
		Close();
	}
	else
	{
		BaseClass::OnKeyCodeTyped( code );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPFAdvCreditsPanel::OnKeyCodePressed(vgui::KeyCode code)
{
	if ( GetBaseButtonCode( code ) == KEY_XBUTTON_B )
	{
		Close();
	}
	else if ( code == KEY_ENTER )
	{
		// do nothing, the default is to close the panel!
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPFAdvCreditsPanel::OnClose()
{
	BaseClass::OnClose();
	DestroyControls();
}

//-----------------------------------------------------------------------------
// Purpose: Command handler
//-----------------------------------------------------------------------------
void CPFAdvCreditsPanel::OnCommand( const char *command )
{
	if( 0 == Q_stricmp( command, "Close" ) )
	{
		Close();
	}
}

enum eCreditRoles
{
	CREDIT_DEVELOPERS = 0,
	CREDIT_CONTRIBUTORS,
	CREDIT_TESTERS,
	CREDIT_SPECIAL,
	CREDIT_SOFTWARE,
	CREDIT_COUNT
};

static const char *s_CreditRoles[CREDIT_COUNT][2] =
{
	{ "developers", "#PF_Credit_Title_Developers" },
	{ "contributors", "#PF_Credit_Title_Contributors" },
	{ "playtesters", "#PF_Credit_Title_Testers" },
	{ "special", "#PF_Credit_Title_Special" },
	{ "software", "#PF_Credit_Title_Software" },
};

// "other" and "localization" are handled seperately
static const char *s_CreditKeyStrings[][2] =
{
	{ "lead", "#PF_Credit_LeadProj" },
	{ "devlead ", "#PF_Credit_LeadDev" }, // Pb - like poison :)
	{ "code", "#PF_Credit_Code" },
	{ "sounds", "#PF_Credit_Sounds"},
	{ "models", "#PF_Credit_Models" },
	{ "textures", "#PF_Credit_Textures" },
	{ "animation", "#PF_Credit_Animation" },
	{ "particles", "#PF_Credit_Particles" },
	{ "mapper", "#PF_Credit_Mapper" },
	{ "concepts", "#PF_Credit_Concepts"},
	{ "sfm", "#PF_Credit_SFM"},
	{ "server", "#PF_Credit_Hosting"},
	{ "music", "#PF_Credit_Music"},
	{ "rig", "#PF_Credit_Rigging"},
	{ "icon", "#PF_Credit_Icons"},
	{ "flex", "#PF_Credit_Flexes"},
	{ "graphics", "#PF_Credit_GraphicDesign"}, // is my passion
	{ "promo", "#PF_Credit_Promo"},
	{ "tester", "#PF_Credit_Tester" },
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPFAdvCreditsPanel::CreateControls()
{
	DestroyControls();

	// Create credit entries
	if( m_pCreditsKV )
	{
		Label *pCreditEntry = nullptr;
		Label *pTitle = nullptr;
		IScheme *pScheme = scheme()->GetIScheme( GetScheme() );

		for( KeyValues *kv = m_pCreditsKV->GetFirstSubKey(); kv; kv = kv->GetNextKey() )
		{
			char szTitleName[32];
			bool bRoleFound = false;
			int i = 0;
			for( i = 0; i < CREDIT_COUNT && !bRoleFound; ++i )
			{
				if( !Q_stricmp( kv->GetName(), s_CreditRoles[i][0] ) )
				{
					bRoleFound = true;
					Q_snprintf( szTitleName, sizeof( szTitleName ), "%s", s_CreditRoles[i][1] );
					break;
				}
			}
			if( !bRoleFound )
				continue;

			pTitle = new Label( this, "Title", "" );
			pTitle->SetText( g_pVGuiLocalize->Find( szTitleName ) );

			pTitle->SetBorder( pScheme->GetBorder( "OptionsCategoryBorder" ) );
			pTitle->SetFont( pScheme->GetFont( "HudFontSmallBold", true ) );
			pTitle->SetFgColor( pScheme->GetColor( "AdvTextDefault", COLOR_WHITE ) );
			pTitle->SetSize( m_iControlWidth, m_iTitleHeight );
			m_pListPanel->AddItem( nullptr, pTitle );

			for( KeyValues *kvDevs = kv->GetFirstSubKey(); kvDevs; kvDevs = kvDevs->GetNextKey() )
			{
				const char *name = kvDevs->GetName();
				if( name )
				{
					char roles[1024] = "";
					bool bMultipleRoles = false;
					if( i != CREDIT_TESTERS && i != CREDIT_SOFTWARE )
					{
						for( int i = 0; i < ARRAYSIZE( s_CreditKeyStrings ); ++i )
						{
							if( kvDevs->GetString( s_CreditKeyStrings[i][0], nullptr ) )
							{
								char szStringToLocalize[64];
								Q_snprintf( szStringToLocalize, sizeof( szStringToLocalize ), "%s", s_CreditKeyStrings[i][1] );

								char szLocalized[512];
								g_pVGuiLocalize->ConvertUnicodeToANSI( g_pVGuiLocalize->Find( szStringToLocalize ), szLocalized, sizeof( szLocalized ) );
								Q_strcat( roles, bMultipleRoles ? VarArgs( ", %s", szLocalized ) : szLocalized, sizeof( roles ) );
								bMultipleRoles = true;
							}
						}
						// Check for special case subkeys
						for( KeyValues *pSubKey = kvDevs->GetFirstSubKey(); pSubKey; pSubKey = pSubKey->GetNextKey() )
						{
							if( !Q_stricmp( pSubKey->GetName(), "localization" ) )
							{
								bool bFound = false;
								for( KeyValues *kvLocalizations = pSubKey->GetFirstSubKey(); kvLocalizations; kvLocalizations = kvLocalizations->GetNextKey() )
								{
									const char *name = kvLocalizations->GetName();
									if( name )
									{
										// Construct a "#GameUI_Language_" string so we can find the translation
										char szUILanguageString[128] = "";
										Q_snprintf( szUILanguageString, sizeof( szUILanguageString ), "#GameUI_Language_%s", name );

										wchar_t wzLocalizationCredit[128];
										g_pVGuiLocalize->ConstructString( wzLocalizationCredit, sizeof( wzLocalizationCredit ), g_pVGuiLocalize->Find( "#PF_Credit_Localization_fmt" ), 1, g_pVGuiLocalize->Find( szUILanguageString ) );
										char szLocalizationCredit[128];
										g_pVGuiLocalize->ConvertUnicodeToANSI( wzLocalizationCredit, szLocalizationCredit, sizeof( szLocalizationCredit ) );

										Q_strcat( roles, bMultipleRoles ? VarArgs( ", %s", szLocalizationCredit ) : szLocalizationCredit, sizeof( roles ) );

										bMultipleRoles = true;
										bFound = true;
									}
								}
								// "localization" entry found, but no keyvalues
								if( !bFound )
								{
									char szLocalized[128];
									g_pVGuiLocalize->ConvertUnicodeToANSI( g_pVGuiLocalize->Find( "#PF_Credit_Localization" ), szLocalized, sizeof( szLocalized ) );
									Q_strcat( roles, bMultipleRoles ? VarArgs( ", %s", szLocalized ) : szLocalized, sizeof( roles ) );
								}
								bMultipleRoles = true;
							}
							else if( !Q_stricmp( pSubKey->GetName(), "other" ) )
							{
								for( KeyValues *kvOtherRoles = pSubKey->GetFirstSubKey(); kvOtherRoles; kvOtherRoles = kvOtherRoles->GetNextKey() )
								{
									const char *name = kvOtherRoles->GetName();
									if( name )
									{

										// Get our current language
										char uilanguage[64];
										engine->GetUILanguage( uilanguage, sizeof( uilanguage ) );

										char szOtherCredit[256];
										bool bFound = false;
										// We've found an "other" entry in our current language
										if( !Q_stricmp( name, uilanguage ) )
										{
											Q_strncpy( szOtherCredit, pSubKey->GetString( name, "" ), sizeof( szOtherCredit ) );
											bFound = true;
										}
										else
										{
											// Check if we have an english entry
											if( kvOtherRoles->GetString( "English", nullptr ) )
											{
												Q_strcat( szOtherCredit, pSubKey->GetString( "English", "" ), sizeof( szOtherCredit ) );
												bFound = true;
											}
										}

										// We've found an "other" entry, add it
										if( bFound )
										{
											Q_strcat( roles, bMultipleRoles ? VarArgs( ", %s", szOtherCredit ) : szOtherCredit, sizeof( roles ) );
											bMultipleRoles = true;
										}
									}
								}
							}
						}
					}

					pCreditEntry = new Label( this, "CreditName", bMultipleRoles ? VarArgs( "%s - %s", name, roles ) : name );
					pCreditEntry->SetFont( pScheme->GetFont( "HudFontSmallestBold", true ) );
					pCreditEntry->SetFgColor( pScheme->GetColor( "AdvTextDefault", COLOR_WHITE ) );
					pCreditEntry->SetSize( m_iControlWidth, m_iControlHeight );
					m_pListPanel->AddItem( nullptr, pCreditEntry );
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPFAdvCreditsPanel::DestroyControls()
{
	m_pListPanel->DeleteAllItems();
}

//-----------------------------------------------------------------------------
// Purpose: Shows this dialog as a modal dialog
//-----------------------------------------------------------------------------
void CPFAdvCreditsPanel::ShowModal()
{
	SetVisible( true );
	MoveToFront();
	DoModal();

	int x, y, ww, wt, wide, tall;
    vgui::surface()->GetWorkspaceBounds( x, y, ww, wt );
    GetSize(wide, tall);

    // Center it, keeping requested size
    SetPos(x + ((ww - wide) / 2), y + ((wt - tall) / 2));
}

CON_COMMAND( OpenCreditsDialog, "Shows the credits dialog" )
{
	GPFAdvCreditsPanel()->ShowModal();
}