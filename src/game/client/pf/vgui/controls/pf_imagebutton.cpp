
#include "cbase.h"
#include "controls/pf_imagebutton.h"
#include <vgui_controls/Label.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui/ISystem.h>
#include <KeyValues.h>
#include <vgui/MouseCode.h>
#include "vgui/IInput.h"


// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

DECLARE_BUILD_FACTORY( ImageButton );

ImageButton::ImageButton(vgui::Panel *parent, const char *panelName) :
	Button(parent, panelName, "")
{
	m_pBackdrop = new vgui::ImagePanel(this, "ImageButtonBackdrop");
	ButtonInit();
}

void ImageButton::ButtonInit()
{
	m_szButtonTexture[0] = '\0';
	m_szButtonOverTexture[0] = '\0';
	Q_snprintf(m_szBorder, sizeof(m_szBorder), "ASWBriefingButtonBorder");
	Q_snprintf(m_szBorderDisabled, sizeof(m_szBorderDisabled), "ASWBriefingButtonBorderDisabled");
	SetPaintBackgroundEnabled(false);
	m_bUsingOverTexture = false;
	SetEnabled(true);
	SetMouseInputEnabled(true);
	m_pBackdrop->SetMouseInputEnabled(false);
	m_bDoMouseOvers = true;
}

ImageButton::~ImageButton()
{
	
}

void ImageButton::ApplySettings( KeyValues *inResourceData )
{
	SetCommand(inResourceData->GetString("command", ""));

	Q_strncpy(m_szButtonOverTexture , inResourceData->GetString( "activeimage" ), sizeof( m_szButtonOverTexture ) );
	Q_strncpy( m_szButtonTexture, inResourceData->GetString( "inactiveimage", m_szButtonOverTexture ), sizeof( m_szButtonTexture ) );
	if ( m_szButtonTexture[0] != '\0' )
	{
		m_pBackdrop->SetImage(m_szButtonTexture);
	}

	BaseClass::ApplySettings( inResourceData );

	int w, h, x, y;
	GetBounds(x, y, w, h);
	m_nBaseX = x;
	m_nBaseY = y;
	m_nBaseWidth = w;
	m_nBaseTall = h; 
}


void ImageButton::SetButtonActive(bool bState)
{
	if (bState && !m_bUsingOverTexture)
	{
		if (m_pBackdrop && m_szButtonOverTexture[0] != '\0')
			m_pBackdrop->SetImage(m_szButtonOverTexture);
		m_bUsingOverTexture = true;
	}
	else if (!bState && m_bUsingOverTexture)
	{
		if (m_pBackdrop && m_szButtonTexture[0] != '\0')
			m_pBackdrop->SetImage(m_szButtonTexture);
		m_bUsingOverTexture = false;
	}
}

void ImageButton::SetButtonTexture(const char *szFilename)
{
	Q_strcpy(m_szButtonTexture, szFilename);
	if ((!IsCursorOver() || !IsEnabled()) && m_pBackdrop)
	{
		m_pBackdrop->SetImage(szFilename);
		m_pBackdrop->SetShouldScaleImage(true);
		m_bUsingOverTexture = false;
	}
}

void ImageButton::SetButtonOverTexture(const char *szFilename)
{
	Q_strcpy(m_szButtonOverTexture, szFilename);
	if (IsCursorOver() && IsEnabled() && m_pBackdrop)
	{
		m_pBackdrop->SetImage(szFilename);
		m_pBackdrop->SetShouldScaleImage(true);
		m_bUsingOverTexture = true;
	}
}

void ImageButton::SetArmed(bool state)
{
	BaseClass::SetArmed(state);
	SetButtonActive(state);

}

void ImageButton::OnThink()
{	
	BaseClass::OnThink();

	// check our label and backdrop are the right sizes
	int w = GetWide();
	int t = GetTall();

	int bw = m_pBackdrop->GetWide();
	int bt = m_pBackdrop->GetTall();
	if (bw != w || bt != t)
	{
		m_pBackdrop->SetSize(w,t);
	}
}

void ImageButton::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	m_pBackdrop->SetShouldScaleImage(true);
}

void ImageButton::SetContentAlignment(vgui::Label::Alignment Alignment)
{
	//if (m_pLabel)
	//	m_pLabel->SetContentAlignment(Alignment);
}