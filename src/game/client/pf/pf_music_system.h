#pragma once
#include <vgui/tf_controls.h>
#include "cbase.h"
#include "vgui_basepanel.h"


class CPFMainMenuMusic
{
public:
	char*		IsHoliday();
	void		Init();
	void		OnTick();
	//void		RandomMusicCommand();
	//int					m_nSongGuid;

private:
	void GetRandomMusic(char* pszBuf, int iBufLength);
	void SetMenuMusicString(char* m_szMusicName);
	
	CUtlStringList		MusicList;

	enum MusicStatus
	{
		MUSIC_STOP,
		MUSIC_FIND,
		MUSIC_PLAY,
		MUSIC_STOP_FIND,
		MUSIC_STOP_PLAY,
	};
	
	struct SongInfo
	{
		char title[31];
		char artist[31];

	};

	//char* id3_read(char* filename, SongInfo* si);
	int					random;

	char				m_pzMusicLink[64];
	
	MusicStatus			m_psMusicStatus;
	int					m_iPlayGameStartupSound;

};
