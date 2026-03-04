#include "cbase.h"
#include "ioverrideinterface.h"
#include "pf_mainmenu_override.h"

// Derived class of override interface
class COverrideInterface : public IOverrideInterface
{
private:
	OverrideUI_RootPanel *MainMenu;
 
public:
	int UI_Interface( void )
	{
		MainMenu = NULL;
		return 0;
	}
 
	void Create( vgui::VPANEL parent )
	{
		// Create immediately
		MainMenu = new OverrideUI_RootPanel(parent);
	}

	vgui::VPANEL GetPanel( void )
	{
		if ( !MainMenu )
			return NULL;
		return MainMenu->GetVPanel();
	}
 
	void Destroy( void )
	{
		if ( MainMenu )
		{
			MainMenu->SetParent( (vgui::Panel *)NULL );
			delete MainMenu;
		}
	}
 
};
 
static COverrideInterface g_SMenu;
IOverrideInterface *OverrideUI = ( IOverrideInterface * )&g_SMenu;