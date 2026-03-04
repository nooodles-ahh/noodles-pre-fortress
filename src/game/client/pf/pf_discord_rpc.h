//=============================================================================
//
// Purpose: Discord and Steam Rich Presence support.
//
//=============================================================================
#ifndef PF_DISCORD_RPC_H
#define PF_DISCORD_RPC_H
#ifdef _WIN32
#pragma once
#endif

#include "GameEventListener.h"
#include "hl2orange.spa.h"
#include "discord_rpc.h"

#define DISCORD_FIELD_SIZE 128

class CTFDiscordRPC : public CAutoGameSystemPerFrame, public CGameEventListener
{
public:
	CTFDiscordRPC();
	~CTFDiscordRPC();
	void Shutdown();

	//virtual bool Init();
	virtual void PostInit();
	virtual void Update( float frametime );

	void LevelInitPreEntity();
	virtual void LevelShutdownPostEntity();
	void Reset();
	bool RPCInitialized() { return m_bInitialized; }
	bool m_bInitialized;

	static void OnReady(const DiscordUser* user);
	static void OnDiscordError(int errorCode, const char* szMessage);
	static void OnJoinGame(const char* joinSecret);
	static void OnSpectateGame(const char* spectateSecret);
	static void OnJoinRequest(const DiscordUser* joinRequest);

	//void SetMenuMusicString(char* m_szMusicName);

	// IGameEventListener interface:
	virtual void FireGameEvent(IGameEvent* event);

	const char* LocalizeDiscordString(const char* LocalizedString);
	char m_szLatchedMusicname[DISCORD_FIELD_SIZE];

private:
	void InitializeDiscord();
	bool NeedToUpdate();

	void UpdateRichPresence();
	void UpdatePlayerInfo();
	void UpdateNetworkInfo();
	void SetLogo();
	const char* pszState;
	const char*	srctv_port;

	

	bool m_bErrored;
	bool m_bInitializeRequested;
	float m_flLastUpdatedTime;
	DiscordRichPresence m_sDiscordRichPresence;

	// scratch buffers to send in api struct. they need to persist
	// for a short duration after api call it seemed, it must be async
	// using a stack allocated would occassionally corrupt
	char m_szServerInfo[DISCORD_FIELD_SIZE];
	char m_szDetails[DISCORD_FIELD_SIZE];
	char m_szLatchedHostname[255];
	char m_szLatchedMapname[MAX_MAP_NAME];
	
	char szStateBuffer[256];
	//HINSTANCE m_hDiscordDLL;
};

extern CTFDiscordRPC g_discordrpc;
#endif // PF_DISCORD_RPC_H