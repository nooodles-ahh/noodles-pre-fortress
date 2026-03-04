#ifndef _INCLUDED_IMAGE_BUTTON_H
#define _INCLUDED_IMAGE_BUTTON_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>

namespace vgui
{
	class Label;
	class ImagePanel;
}

// a Button with an image panel backdrop and a label for the text

class ImageButton : public vgui::Button
{
	DECLARE_CLASS_SIMPLE( ImageButton, vgui::Button );
public:
	ImageButton(vgui::Panel *parent, const char *panelName);
	virtual ~ImageButton();
	virtual void ButtonInit();

	enum IB_ActivationType_t
	{
		ACTIVATE_ONRELEASED,
		ACTIVATE_ONPRESSED,
	};

	virtual void ApplySettings(KeyValues *inResourceData);
	virtual void SetButtonTexture(const char *szFilename);
	virtual void SetButtonOverTexture(const char *szFilename);
	virtual void OnThink();
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void SetContentAlignment(vgui::Label::Alignment Alignment);
	void SetButtonActive(bool state);
	virtual void SetArmed(bool state);

	vgui::ImagePanel *m_pBackdrop;
	char m_szButtonTexture[256];
	char m_szButtonOverTexture[256];
	char m_szBorder[128];
	char m_szBorderDisabled[128];
	bool m_bUsingOverTexture;
	bool m_bDoMouseOvers;

	int m_nBaseWidth;
	int m_nBaseTall;
	int m_nBaseX;
	int m_nBaseY;
};


#endif // _INCLUDED_IMAGE_BUTTON_H