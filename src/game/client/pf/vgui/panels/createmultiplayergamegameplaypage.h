//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef CREATEMULTIPLAYERGAMEGAMEPLAYPAGE_H
#define CREATEMULTIPLAYERGAMEGAMEPLAYPAGE_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/PropertyPage.h>
#include <vgui_controls/EditablePanel.h>

#define OPTIONS_DIR "cfg"
#define DEFAULT_OPTIONS_FILE OPTIONS_DIR "/settings_default.scr"
#define OPTIONS_FILE OPTIONS_DIR "/settings.scr"
#define DEFAULT_OPTIONS_MOD_FILE OPTIONS_DIR "/settings_mod_default.scr"
#define OPTIONS_MOD_FILE OPTIONS_DIR "/settings_mod.scr"

class CPanelListPanel;
class CDescription;
class mpcontrol_t;

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: server options page of the create game server dialog
//-----------------------------------------------------------------------------
class CCreateMultiplayerGameGameplayPage : public vgui::PropertyPage
{
public:
	CCreateMultiplayerGameGameplayPage(vgui::Panel *parent, const char *name);
	~CCreateMultiplayerGameGameplayPage();

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

	// returns currently entered information about the server
	int GetMaxPlayers();
	const char *GetPassword();
	const char *GetHostName();

	virtual const char *GetResName() { return "Resource/CreateMultiplayerGameGameplayPage.res"; }
	virtual char *GetDefaultFileName() { return DEFAULT_OPTIONS_FILE; }
	virtual char *GetUserFileName() { return OPTIONS_FILE; }

protected:
	virtual void OnApplyChanges();

	const char *GetValue(const char *cvarName, const char *defaultValue);
	void LoadGameOptionsList();
	void GatherCurrentValues();

private:
	CDescription *m_pDescription;
	mpcontrol_t *m_pList;
	vgui::PanelListPanel *m_pOptionsList;

	bool m_bLoaded;
};

class CCreateMultiplayerGameModPage : public CCreateMultiplayerGameGameplayPage
{
public:
	CCreateMultiplayerGameModPage( vgui::Panel *parent, const char *name );

	virtual const char *GetResName() { return "Resource/CreateMultiplayerGameModPage.res"; }
	virtual char *GetDefaultFileName() { return DEFAULT_OPTIONS_MOD_FILE; }
	virtual char *GetUserFileName() { return OPTIONS_MOD_FILE; }

};


#endif // CREATEMULTIPLAYERGAMEGAMEPLAYPAGE_H
