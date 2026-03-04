
#include "cbase.h"
//#include "controls/tf_advbutton.h"
//#include "controls/tf_advslider.h"
#include "vgui_controls/SectionedListPanel.h"
#include "vgui_controls/ImagePanel.h"
//#include "tf_notificationmanager.h"
#include "engine/IEngineSound.h"
#include "vgui_avatarimage.h"
#include "filesystem.h"
#include "icommandline.h"
#include "pf_cvars.h"
#include "pf_music_system.h"
#include <ctime>
//#include "pf_discord_rpc.h"
#include "tier2/fileutils.h"
#include "tf_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

int m_nSongGuid;
bool ExecutedHoliday = false;

int lastRandomInt;

char* CPFMainMenuMusic::IsHoliday()
{
	time_t ltime = time(0);
	const time_t* ptime = &ltime;
	struct tm* today = localtime(ptime);
	if (today)
	{
		if (today->tm_mon == 3 && today->tm_mday >= 12 || today->tm_mon == 4 && today->tm_mday <= 31)
		{
			return "APRIL";
		}
		else if(today->tm_mon == 9 && today->tm_mday >= 1)
		{
			return "HALLOWEEN";
		}
		else if(today->tm_mon == 11 && today->tm_mday >= 1 || today->tm_mon == 0 && today->tm_mday <= 7)
		{
			return "CHRISTMASS";
		}
	}
	return NULL;

}

ConVar pf_mainmenu_music("pf_mainmenu_music", "1", FCVAR_ARCHIVE, "Toggle music in the main menu");

//this is used to change the main menu music to the players own custom one
//-Nbc66
ConVar pf2_mainmenu_music_custom("pf_mainmenu_music_custom", "0", FCVAR_ARCHIVE, "Use custom music from \"sound/ui/custom\"");

CPFMainMenuMusic music;
/*
char* CPFMainMenuMusic::id3_read(char* filename, SongInfo* si)
{
	CBaseFile fh;
	//ConWarning("Unable to open file.\n");

	fh.Open(filename, "r");

	if (fh.IsOk())
	{

		char raw[128];

		//go back 128 bytes from the bottom to end up at the
		//TAG section
		//-Nbc66
		fh.Seek(-128, FILESYSTEM_SEEK_TAIL);
		fh.Read(raw, sizeof(raw));
		fh.Close();

		if (raw[0] != 'T' || raw[1] != 'A' || raw[2] != 'G') {
			ConDWarning("[Music System]No ID3v1 tag found.\n");
			if (g_discordrpc.IsRPCInitialized())
			{
				V_snprintf(g_discordrpc.m_szLatchedMusicname, DISCORD_FIELD_SIZE, g_discordrpc.LocalizeDiscordString("#Discord_Music"), g_discordrpc.LocalizeDiscordString("#Discord_Music_default"));
			}
		}
		else
		{
			char* b = raw + 3;


			//Move 30 bytes to the right of the tail to find all the info
			//-Nbc66
			si->title[30] = 0; V_memcpy(si->title, b, 30); b += 30;
			si->artist[30] = 0; V_memcpy(si->artist, b, 30);
			//si->album[30] = 0; V_memcpy(si->album, b, 30); b += 30;
			//si->year[4] = 0; V_memcpy(si->year, b, 4); b += 4;
			//si->comment[30] = 0; V_memcpy(si->comment, b, 30); b += 30;
			//si->genre = *b;
			if (g_discordrpc.IsRPCInitialized())
			{
				V_snprintf(g_discordrpc.m_szLatchedMusicname, DISCORD_FIELD_SIZE, g_discordrpc.LocalizeDiscordString("#Discord_Music"), si->title);
			}
		}


	}
	else
	{
		ConWarning("[Music System] FILE NOT FOUND\n");
		return NULL;
	}
	return NULL;
}
*/
void CPFMainMenuMusic::GetRandomMusic(char* pszBuf, int iBufLength)
{
	Assert(iBufLength);

	char szPath[MAX_PATH];

	FileFindHandle_t findHandle = NULL;

	CUtlStringList MusicList;

	const char* pszFilenameMusic = g_pFullFileSystem->FindFirst("sound/ui/custom/*.mp3", &findHandle);

	char szMusicCheck[FILESYSTEM_MAX_SEARCH_PATHS];

	//We need this so we can be sure that we have at least 1 mp3 file in our folder
	//-Nbc66
	V_snprintf(szMusicCheck, sizeof(szMusicCheck), "sound/ui/custom/%s", pszFilenameMusic);

	while (pszFilenameMusic)
	{
		char szMusicName[FILESYSTEM_MAX_SEARCH_PATHS];
		Q_snprintf(szMusicName, sizeof(szMusicName), "*#ui/custom/%s", pszFilenameMusic);

		V_FixSlashes(szMusicName);

		MusicList.CopyAndAddToTail(szMusicName);

		pszFilenameMusic = g_pFullFileSystem->FindNext(findHandle);

	}

	// Check's that there's music available
	if( pf2_mainmenu_music_custom.GetBool() )
	{
		if( !g_pFullFileSystem->FileExists( szMusicCheck ) )
		{
			Assert( false );
			//*pszBuf = '\0';
			ConWarning( "[Music System] NO CUSTOM MUSIC FOUND. DEFAULTING TO NORMAL MUSIC\n" );
			pf2_mainmenu_music_custom.SetValue( 0 );
		}
	}

	if( pf2_mainmenu_music_custom.GetBool() )
	{

		// Pick a random one
		Q_snprintf( szPath, sizeof( szPath ), MusicList.Random() );
		Q_strncpy( pszBuf, szPath, iBufLength );

		MusicList.Purge();
	}
	else if( IsHoliday() != NULL )
	{
		if( !V_strcmp( IsHoliday(), "APRIL" ) && !ExecutedHoliday )
		{
			Q_strncpy( pszBuf, "*#ui/holiday/gamestartup_soldier.mp3", 37 );
			ExecutedHoliday = true;
		}

		else if( !V_strcmp( IsHoliday(), "HALLOWEEN" ) && !ExecutedHoliday )
		{
			//chose from 2 songs that we have
			//-Nbc66
			V_snprintf( pszBuf, 41, "*#ui/holiday/gamestartup_halloween%d.mp3", RandomInt( 1, 2 ) );
			ExecutedHoliday = false;
		}
	}
	else
	{
		// Discover tracks, 1 through n
		int iLastTrack = 0;
		do
		{
			Q_snprintf( szPath, sizeof( szPath ), "sound/ui/music/gamestartup%d.mp3", ++iLastTrack );
		} while( g_pFullFileSystem->FileExists( szPath ) );

		lastRandomInt = RandomInt( 1, iLastTrack - 1 );

		while( lastRandomInt == RandomInt( 1, iLastTrack - 1 ) )
		{
			lastRandomInt = RandomInt( 1, iLastTrack - 1 );
		}

		// Pick a random one
		Q_snprintf( szPath, sizeof( szPath ), "*#ui/music/gamestartup%d.mp3", lastRandomInt );
		Q_strncpy( pszBuf, szPath, iBufLength );
	}

	MusicList.Purge();

	
}

void CPFMainMenuMusic::Init()
{
	m_iPlayGameStartupSound = 0;
	m_psMusicStatus = MUSIC_FIND;
	m_pzMusicLink[0] = '\0';
	m_nSongGuid = 0;

}




//was trying to set the actaul music name of the song thats playing in the main menu inside of discord rpc
//but it crashes at tier0 something regarding keyvalu memory alocation
//for now this function is useless i will still keep it inside the code if anybody wants to fix it
//
//UPDATE: It's fixed THANK YOU KAY
//-Nbc66

void CPFMainMenuMusic::SetMenuMusicString(char* m_szMusicName)
{
	if( pf_mainmenu_music.GetBool() )
	{
		//USE CUSTOM MUSIC CHECK
		//-Nbc66
		/*
		if (pf_mainmenu_music_custom.GetBool())
		{

				char formatedstring[DISCORD_FIELD_SIZE];

				//supprisingly you dont need to construct a string with localize discord to use %s in it
				//NEET!
				//-Nbc66
				V_snprintf(formatedstring, DISCORD_FIELD_SIZE, g_discordrpc.LocalizeDiscordString("#Discord_Music"), g_discordrpc.LocalizeDiscordString("#Discord_Music_default"));


				V_strncpy(g_discordrpc.m_szLatchedMusicname, formatedstring, DISCORD_FIELD_SIZE);

		}
		else
		{
			KeyValues* pDiscordRPC = new KeyValues("Discord");
			pDiscordRPC->LoadFromFile(filesystem, "scripts/discord_rpc.txt");
			if (pDiscordRPC)
			{
				//char* test;
				KeyValues* pMaps = pDiscordRPC->FindKey("Music");
				if (pMaps)
				{
					char formatedstring[DISCORD_FIELD_SIZE];

					V_snprintf(formatedstring, DISCORD_FIELD_SIZE, g_discordrpc.LocalizeDiscordString("#Discord_Music"), pMaps->GetString(V_GetFileName(m_szMusicName), g_discordrpc.LocalizeDiscordString("#Discord_Music_default")));


					V_strncpy(g_discordrpc.m_szLatchedMusicname, formatedstring, DISCORD_FIELD_SIZE);
					

				}

				pDiscordRPC->deleteThis();
			}
		}
		*/
	}
}


void CPFMainMenuMusic::OnTick()
{

	if ((pf_mainmenu_music.GetBool() && !engine->IsConnected()) || (pf_mainmenu_music.GetBool() && engine->IsLevelMainMenuBackground()))
	{
		if ((m_psMusicStatus == MUSIC_FIND || m_psMusicStatus == MUSIC_STOP_FIND) && !enginesound->IsSoundStillPlaying(m_nSongGuid))
		{
			GetRandomMusic(m_pzMusicLink, sizeof(m_pzMusicLink));
			m_psMusicStatus = MUSIC_PLAY;
			SetMenuMusicString(m_pzMusicLink);
		}
		else if ((m_psMusicStatus == MUSIC_PLAY || m_psMusicStatus == MUSIC_STOP_PLAY) && m_pzMusicLink[0] != '\0')
		{
			enginesound->StopSoundByGuid(m_nSongGuid);
			// dont need to do a volume check since we allready set the music volume with the "*" char
			// -Nbc66
			//ConVar* snd_musicvolume = cvar->FindVar("snd_musicvolume");
			//float fVolume = (snd_musicvolume ? snd_musicvolume->GetFloat() : 1.0f);
			enginesound->EmitAmbientSound(m_pzMusicLink, VOL_NORM );
			m_nSongGuid = enginesound->GetGuidForLastSoundEmitted();
			SetMenuMusicString(m_pzMusicLink);
			m_psMusicStatus = MUSIC_FIND;
		}
	}
	else if (m_psMusicStatus == MUSIC_FIND)
	{
		enginesound->StopSoundByGuid(m_nSongGuid);
		m_psMusicStatus = (m_nSongGuid == 0 ? MUSIC_STOP_FIND : MUSIC_STOP_PLAY);
	}

}

void RandomMusicCommand()
{
	enginesound->StopSoundByGuid(m_nSongGuid);
}

ConCommand randommusic("randommusic", RandomMusicCommand , "play random music");
