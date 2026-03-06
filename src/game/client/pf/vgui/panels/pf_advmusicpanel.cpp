// The following include files are necessary to allow your MyPanel.cpp to compile.
#include "cbase.h"
#include "pf_advmusicpanel.h"
#include <vgui/IVGui.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/Button.h>
#include <vgui/ISurface.h>
#include <vgui_controls/AnimationController.h>
#include "pf_music_system.h"

using namespace vgui;
/*
// music panel only works on Windows for now, due to information being pulled from 
// Discord RPC which is also Windows only as requested by Nbc. -dead_thing
#ifdef _WIN32	


//CMyPanel class: Tutorial example class
class CMyPanel : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE(CMyPanel, vgui::Frame);
	//CMyPanel : This Class / vgui::Frame : BaseClass

	CMyPanel(vgui::VPANEL parent); 	// Constructor
	~CMyPanel() {};				// Destructor

protected:
	//VGUI overrides:
	virtual void OnTick();
	virtual void OnCommand(const char* pcCommand);
	virtual void	ApplySchemeSettings(vgui::IScheme* pScheme);
	vgui::Panel* m_pTestPanel;

private:
	//Other used VGUI control Elements:
	Button* m_pCloseButton;
	Button* m_pUpdateButton;
	ImagePanel* m_pTestImage;
	vgui::Label* m_pTestLabel;
	ConVar* menu_music = cvar->FindVar("pf_mainmenu_music");
	ConVar* Custom_music = cvar->FindVar("pf_mainmenu_music_custom");
	bool has_excd_anim = false;
};

// Constuctor: Initializes the Panel
CMyPanel::CMyPanel(vgui::VPANEL parent)
	: BaseClass(NULL, "MyPanel")
{
	SetParent(parent);

	SetKeyBoardInputEnabled(false);
	SetMouseInputEnabled(false);

	SetProportional(false);
	SetTitleBarVisible(true);
	SetMinimizeButtonVisible(false);
	SetMaximizeButtonVisible(false);
	SetCloseButtonVisible(false);
	SetSizeable(false);
	SetMoveable(false);
	SetVisible(true);
	SetTabPosition(-1);
	//ShouldDrawBottomRightCornerRounded();
	//MakePopup();

	//SetTitle(g_discordrpc.m_szLatchedMusicname);


	SetScheme(vgui::scheme()->LoadSchemeFromFile("resource/SourceScheme.res", "SourceScheme"));

	

	vgui::ivgui()->AddTickSignal(GetVPanel(), 100);

	DevMsg("MusicPanel has been constructed\n");

	//m_pTestPanel = new Panel(this,"MyPanel");

	m_pTestImage = new ImagePanel(this,"Image");

	m_pTestImage->SetImage("../vgui/main_menu/glyph_speaker");
	m_pTestImage->SetPos(0, 300);
	m_pTestImage->SetScaleAmount(0.5);
	m_pTestImage->Paint();


	m_pTestLabel = new Label(this ,"text_test", g_discordrpc.m_szLatchedMusicname);

	LoadControlSettings("resource/UI/pf_advmusicpanel.res");
}

//Class: CMyPanelInterface Class. Used for construction.
class CMyPanelInterface : public MyPanel
{
private:
	CMyPanel* MyPanel;
public:
	CMyPanelInterface()
	{
		MyPanel = NULL;
	}
	void Create(vgui::VPANEL parent)
	{
		MyPanel = new CMyPanel(parent);
	}
	void Destroy()
	{
		if (MyPanel)
		{
			MyPanel->SetParent((vgui::Panel*)NULL);
			delete MyPanel;
		}
	}
	void Activate(void)
	{
		if (MyPanel)
		{
			MyPanel->Activate();
		}
	}
};
static CMyPanelInterface g_MyPanel;
MyPanel* mypanel = (MyPanel*)&g_MyPanel;

ConVar cl_showmypanel("cl_showmusicpanel", "1", FCVAR_CLIENTDLL, "Sets the state of myPanel <state>");

void CMyPanel::OnTick()
{
	BaseClass::OnTick();

	//set panel to be visible if we are in the main menu
	if (menu_music->GetBool() && !engine->IsConnected())
	{
		vgui::GetAnimationController()->StartAnimationSequence("ShowMusicPlaying");
		SetVisible(true);
		vgui::GetAnimationController()->UpdateAnimations(gpGlobals->curtime);
	}
	else
	{
		SetVisible(false);
	}
	
	if (m_pTestLabel)
	{
		m_pTestLabel->SetText(g_discordrpc.m_szLatchedMusicname);
	}
}



CON_COMMAND(OpenTestPanelFenix, "Toggles testpanelfenix on or off")
{
	cl_showmypanel.SetValue(!cl_showmypanel.GetBool());
	mypanel->Activate();
};

void CMyPanel::OnCommand(const char* pcCommand)
{
	BaseClass::OnCommand(pcCommand);
}

void CMyPanel::ApplySchemeSettings(vgui::IScheme* pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	LoadControlSettings("resource/UI/mypanel.res");
}

#endif

*/