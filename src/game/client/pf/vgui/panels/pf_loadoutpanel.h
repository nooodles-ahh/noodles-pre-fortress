//=============================================================================
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef PF_LOADOUTPANEL_H
#define PF_LOADOUTPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/PropertyDialog.h"
#include "vgui_controls/PropertySheet.h"
#include "vgui_controls/PropertyPage.h"
#include "vgui_controls/ScrollableEditablePanel.h"
#include "basemodel_panel.h"
#include "tf_controls.h"

using namespace vgui;

class ImageButton : public vgui::Button, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( ImageButton, vgui::Button );
public:
	ImageButton( vgui::Panel *parent, const char *panelName );
	virtual void ButtonInit();

	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void SetButtonTexture( const char *szFilename );
	virtual void SetButtonOverTexture( const char *szFilename );
	virtual void OnThink();
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void SetArmed( bool state );

public: // IGameEventListener Interface
	virtual void FireGameEvent( IGameEvent *event );

	vgui::ImagePanel *m_pBackdrop;
	char m_szButtonTexture[256];
	char m_szButtonOverTexture[256];
	char m_szButtonOverTextureTeam3[256];
	bool m_bUsingOverTexture;
	int m_iBGTeam;
};

//=============================================================================
// Item model panel
//=============================================================================
class CPFItemModelPanel : public vgui::EditablePanel
{
private:
	DECLARE_CLASS_SIMPLE( CPFItemModelPanel, vgui::EditablePanel );

public:
	CPFItemModelPanel( vgui::Panel *parent, const char *name );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void PerformLayout();
	void SetModel( const char *weaponMDL, int nSkin );
	void SetDetails( const char *weaponName, const char *weaponDescription );

private:
	CBaseModelPanel *m_pModelPanel;
	CExLabel *m_pNameLabel;
	CExLabel *m_pAttribLabel;

	CPanelAnimationVarAliasType( int, m_nModelX, "model_xpos", "0", "proportional_xpos" );
	CPanelAnimationVarAliasType( int, m_nModelWidth, "model_wide", "140", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_nModelTall, "model_tall", "100", "proportional_int" );

	CPanelAnimationVarAliasType( int, m_nTextX, "text_xpos", "0", "proportional_xpos" );
	CPanelAnimationVarAliasType( int, m_nTextWide, "text_wide", "140", "proportional_int" );
	CPanelAnimationVarAliasType( bool, m_bTextCenter, "text_center", "0", "bool" );
};

//=============================================================================
//
//=============================================================================
class CPFItemSelectionPanel : public vgui::EditablePanel
{
private:
	DECLARE_CLASS_SIMPLE( CPFItemSelectionPanel, vgui::EditablePanel );

public:
	CPFItemSelectionPanel( vgui::Panel *parent );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void PerformLayout();
	virtual void OnCommand( const char *command );

	virtual bool SetTeamClassSlot( int iTeam, int iClass, int iSlot );

private:
	CUtlVector< CPFItemModelPanel *> m_pItemModelPanels;
	CUtlVector< CExButton *> m_pItemEquipButtons;
	ScrollableEditablePanel *m_pItemContainerScroller;
	EditablePanel *m_pItemContainer;
	CExLabel *m_pSlotLabel;

	int m_iTeam;
	int m_iClass;
	int m_iSlot;
	
	int m_iItemCount;
	int m_iSelectedPreset;

	CPanelAnimationVarAliasType( int, m_nButtonXPos, "button_xpos", "420", "proportional_xpos" );
	CPanelAnimationVarAliasType( int, m_nItemX, "itempanel_xpos", "130", "proportional_xpos" );
	CPanelAnimationVarAliasType( int, m_nItemYDelta, "itempanel_ydelta", "16", "proportional_ypos" );
};


//=============================================================================
// Loadout panel
//=============================================================================
class CPFFullLoadoutPanel : public vgui::EditablePanel
{
private:
	DECLARE_CLASS_SIMPLE( CPFFullLoadoutPanel, vgui::EditablePanel );

public:
	CPFFullLoadoutPanel( vgui::Panel *parent );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void PerformLayout();
	virtual void OnCommand( const char *command );
	virtual void ShowLoadout( int iClass, int iTeam = TF_TEAM_RED, int iSlot = 0 );
	virtual void SetMergedWeapon( int iSlot );

private:
	CBaseModelPanel *m_pModelPanel;

	CPFItemModelPanel *m_pItemModelPanels[3];
	CExLabel *m_pInventoryCountLabels[3];
	CExButton *m_pInventoryChangeButtons[3];
	CPFItemSelectionPanel *m_pItemSelectionPanel;

	ImagePanel *m_pBracketImage;
	CExLabel *m_pNoneAvailableLabel;
	CExLabel *m_pNoneAvailable2Label;
	CExLabel *m_pNoneAvailableReasonLabel;

	int m_iClass;
	int m_iTeam;

	CPanelAnimationVarAliasType( int, m_nItemXOffset, "item_xpos_offcenter", "-110", "proportional_xpos" );
	CPanelAnimationVarAliasType( int, m_nItemY, "item_ypos", "60", "proportional_ypos" );
	CPanelAnimationVarAliasType( int, m_nItemYDelta, "item_ydelta", "80", "proportional_ypos" );

	CPanelAnimationVarAliasType( int, m_nButtonXOffset, "button_xpos_offcenter", "175", "proportional_xpos" );
	CPanelAnimationVarAliasType( int, m_nButtonY, "button_ypos", "85", "proportional_ypos" );
	CPanelAnimationVarAliasType( int, m_nButtonYDelta, "button_ydelta", "80", "proportional_int" );
};

//=============================================================================
// Class loadout selection panel
//=============================================================================
class CPFCharInfoLoadoutSubPanel : public vgui::PropertyPage
{
private:
	DECLARE_CLASS_SIMPLE( CPFCharInfoLoadoutSubPanel, vgui::PropertyPage );

public:
	CPFCharInfoLoadoutSubPanel( vgui::Panel *parent );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void PerformLayout();
	virtual void OnThink();
	virtual void RecalculateTargetClassLayout( void ); // no idea what this does in live
	virtual void RecalculateTargetClassLayoutAtPos( int x, int y ); // no idea what this does in live

	virtual void OnCommand( const char *command );
	void OpenFullLoadout( int iClass = 0 );
	CPFFullLoadoutPanel *m_pLoadoutPanel;

private:
	ImageButton *m_classImageButtons[TF_LAST_NORMAL_CLASS]; // 9

	CExLabel *m_pClassLabel;
	CExLabel *m_pItemsLabel;

	CExLabel *m_pSelectLabel;
	CExLabel *m_pLoadoutChangesLabel;

	Color m_colItemColor;
	Color m_colNoItemColor;

	int m_iMouseX, m_iMouseY;
	int m_nClassDetails;
	int m_nOldClassDetails;

	CPanelAnimationVarAliasType( int, m_iSelectLabelY, "selectlabely_default", "0", "proportional_ypos" );
	CPanelAnimationVarAliasType( int, m_iSelectLabelOnChangesY, "selectlabely_onchanges", "0", "proportional_ypos" );

	CPanelAnimationVarAliasType( int, m_iClassYPos, "class_ypos", "0", "proportional_ypos" );
	CPanelAnimationVarAliasType( int, m_iClassXDelta, "class_xdelta", "0", "proportional_xpos" );
	CPanelAnimationVarAliasType( int, m_iClassWideMin, "class_wide_min", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iClassWideMax, "class_wide_max", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iClassTallMin, "class_tall_min", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iClassTallMax, "class_tall_max", "0", "proportional_int" );

	CPanelAnimationVarAliasType( int, m_iClassDistanceMin, "class_distance_min", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iClassDistanceMax, "class_distance_max", "0", "proportional_int" );
};

#endif // PF_LOADOUTPANEL_H
