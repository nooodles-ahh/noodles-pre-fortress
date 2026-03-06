#pragma once

#include <vgui_controls/Frame.h>
#include <vgui_controls/TextEntry.h>

class CCreditWindow : public vgui::Frame
{
public:
	CCreditWindow(char* textFile, Panel* Parent = NULL);
	virtual void OnClose();
	virtual void Activate();
protected:
	vgui::TextEntry* m_pCreditMessage;
	vgui::Button* m_pCloseButton;
};