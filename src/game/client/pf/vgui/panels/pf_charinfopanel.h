//=============================================================================
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef PF_CHARINFOPANEL_H
#define PF_CHARINFOPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/PropertyDialog.h"
#include "vgui_controls/PropertySheet.h"
#include "vgui_controls/PropertyPage.h"
#include "vgui_controls/ScrollableEditablePanel.h"
#include "basemodel_panel.h"
#include "tf_controls.h"
#ifdef PF2BETA
#include "pf_loadoutpanel.h"
#endif
#include "tf_statsummary.h"
using namespace vgui;

//=============================================================================
// Character info panel
//=============================================================================
class CPFCharInfoPanel : public vgui::PropertyDialog,
	public CGameEventListener
{
private:
	DECLARE_CLASS_SIMPLE( CPFCharInfoPanel, vgui::PropertyDialog );

public:
	CPFCharInfoPanel( vgui::Panel *parent );
	virtual ~CPFCharInfoPanel();

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void	FireGameEvent( IGameEvent *event );
	void CloseModal();

	void ShowModal( int iClass = 0 );
	virtual void OnCommand( const char *command );

private:
#ifdef PF2BETA
	CPFCharInfoLoadoutSubPanel *m_pCharInfoPanel;
#endif
	CTFStatsSummaryPanel *m_pStatsPanel;
	bool m_bShowLoadout;
	bool m_bCloseExits;
};


CPFCharInfoPanel *GetCharacterInfoPanel();
void CreatePFInventoryPanel();
void DestroyPFInventoryPanel();

#endif // PF_CHARINFOPANEL_H
