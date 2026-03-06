#include "cbase.h"
#include "pf_credits.h"
#include <vgui_controls/TextEntry.h>
#include <vgui_controls/Button.h>
#include "icvar.h"
#include <filesystem.h>
#include <vgui/ILocalize.h>
#include "vgui/ISurface.h"
#include "vgui_controls/Controls.h"
#include "engine/IEngineSound.h"
#include "c_playerresource.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

char fileName[MAX_PATH];
char* creditText;
wchar* wcreditText;
bool set = false;
bool w = false;

int soundgui = NULL;

void OpenCreditWindow()
{
	if (fileName[0] == '\0')
	{
		char uilanguage[64];
		uilanguage[0] = 0;
		engine->GetUILanguage(uilanguage, sizeof(uilanguage));
		ConVarRef lang_file("lang_file");

		if (V_strcmp(uilanguage, lang_file.GetString()) != 0)
			Q_snprintf(fileName, sizeof(fileName), "scripts/credits_%s.txt", lang_file.GetString());
		else
			Q_snprintf(fileName, sizeof(fileName), "scripts/credits_%s.txt", uilanguage);
		bool fileFound = g_pFullFileSystem->FileExists(fileName);
		if (!fileFound)
		{
			Q_snprintf(fileName, sizeof(fileName), "scripts/credits_english.txt");
			fileFound = g_pFullFileSystem->FileExists(fileName);
		}
		if (!fileFound)
		{
			Warning("Could not open credits file.\n");
			Q_snprintf(fileName, sizeof(fileName), "");
		}
	}
	CCreditWindow* creditWindow = new CCreditWindow(fileName);
	creditWindow->DoModal();

	

	if (!engine->IsConnected())
	{
		enginesound->StopAllSounds(true);
		enginesound->EmitAmbientSound("ui/menu_music.wav", 1.0f);
		soundgui = enginesound->GetGuidForLastSoundEmitted();
		ConDColorMsg(Color(248, 252, 3, 255), "Sound GUID for credits music = %i\n", soundgui);
	}
}
ConCommand OpenCreditsDialog("OpenCreditsDialog", OpenCreditWindow);

void CCreditWindow::Activate()
{
	Frame::Activate();
}


void CCreditWindow::OnClose()
{
	if (!engine->IsConnected()) 
	{
		enginesound->StopSoundByGuid(soundgui);
	}
	Frame::OnClose();
}



CCreditWindow::CCreditWindow(char* fileName, Panel* parent) : Frame(parent, "CreditWindow", false, true)
{
	if (!set)
	{
		if (fileName[0] != '\0')
		{
			FileHandle_t f = g_pFullFileSystem->Open(fileName, "rb");

			// read into a memory block
			int fileSize = g_pFullFileSystem->Size(f);
			int dataSize = fileSize + sizeof(wchar_t);
			if (dataSize % 2)
				++dataSize;
			wchar_t* memBlock = (wchar_t*)malloc(dataSize);
			memset(memBlock, 0x0, dataSize);
			int bytesRead = g_pFullFileSystem->Read(memBlock, fileSize, f);
			if (bytesRead < fileSize)
			{
				// NULL-terminate based on the length read in, since Read() can transform \r\n to \n and
				// return fewer bytes than we were expecting.
				char* data = reinterpret_cast<char*>(memBlock);
				data[bytesRead] = 0;
				data[bytesRead + 1] = 0;
			}

			// null-terminate the stream (redundant, since we memset & then trimmed the transformed buffer already)
			memBlock[dataSize / sizeof(wchar_t) - 1] = 0x0000;

			// check the first character, make sure this a little-endian unicode file

#if defined( _X360 )
			if (memBlock[0] != 0xFFFE)
#else
			if (memBlock[0] != 0xFEFF)
#endif
			{
				// its a ascii char file
				creditText = reinterpret_cast<char*>(memBlock);
				w = false;
			}
			else
			{
				// ensure little-endian unicode reads correctly on all platforms
				CByteswap byteSwap;
				byteSwap.SetTargetBigEndian(false);
				byteSwap.SwapBufferToTargetEndian(memBlock, memBlock, dataSize / sizeof(wchar_t));

				wcreditText = memBlock + 1;
				w = true;
			}
			g_pFullFileSystem->Close(f);
			//free(memBlock);
		}
		else
		{
			w = false;
			creditText = "MISSINGNO.";
		}
	}
	m_pCreditMessage = new vgui::TextEntry(this, "CreditMessage");
	m_pCreditMessage->SetEditable(false);
	m_pCreditMessage->SetVerticalScrollbar(true);
	if (!w) m_pCreditMessage->SetText(creditText);
	else m_pCreditMessage->SetText(wcreditText);
	m_pCreditMessage->GotoTextStart();
	this->SetTitle("#GameUI_Credits", true);

	m_pCreditMessage->SetMultiline(true);
	m_pCreditMessage->SetAutoResize(PIN_TOPLEFT, AUTORESIZE_DOWNANDRIGHT, 16, 34, -16, -40);
	m_pCreditMessage->SetSize(448, 250);
	this->SetSize(480, 360);
	m_pCreditMessage->SetPos(16, 34);

	set = true;

	m_pCloseButton = new vgui::Button(this, "CreditCloseButton", "#GameUI_Close", this, "Close");
	m_pCloseButton->SetPos(374, 328);
	m_pCloseButton->SetSize(82, 24);
	m_pCloseButton->SetAutoResize(PIN_BOTTOMRIGHT, AUTORESIZE_NO, -24, -8, 0, 0);
}
