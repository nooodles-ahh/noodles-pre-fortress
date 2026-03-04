//=============================================================================
//
// Purpose: Discord Presence support.
//
//=============================================================================


#include "cbase.h"
#include "pf_discord_rpc.h"
#include "c_team_objectiveresource.h"
#include "tf_gamerules.h"
#include "c_tf_team.h"
#include "c_tf_playerresource.h"
#include <inetchannelinfo.h>
#include "discord_rpc.h"
#include "discord_register.h"
#include "tf_gamerules.h"
#include <ctime>
#include "tier0/icommandline.h"
#include "ilocalize.h"
#include "filesystem.h"
#include <stdlib.h>
#include "tf_hud_escort.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar cl_richpresence_printmsg("cl_richpresence_printmsg", "0", FCVAR_CLIENTDLL, "");
ConVar pf_discord_rpc( "pf_discord_rpc", "1", FCVAR_ARCHIVE, "Enable/Disable Discord Rich Presence" );
ConVar pf_discord_class( "pf_discord_class", "1", FCVAR_ARCHIVE, "Enable/Disable Discord rpc Class icons (REQUIRES GAME RESTART)" );

#define DISCORD_APP_ID	"658004706337226791"

// update once every 10 seconds. discord has an internal rate limiter of 15 seconds as well
#define DISCORD_UPDATE_RATE 10.0f

//not needed anymore
//-Nbc66
//#define MAP_COUNT 10

const char *FinalCharLocalizedString;

const char* g_aTeamNamesLowerRPC[] =
{
	"unassigned",
	"spectator",
	"red",
	"blu"
};

//Replaced by discord_rpc.txt
//-Nbc66
/*
const char* g_aMapList[] =
{
	"tc_hydro",
	"ad_dustbowl",
	"ad_gravelpit",
	"cp_granary",
	"cp_well",
	"ctf_2fort",
	"ctf_well",
	"cp_powerhouse",
	"ad_dustbowl2",
	"ctf_badlands"
};
*/

extern const char* GetMapDisplayName(const char* mapName);

CTFDiscordRPC g_discordrpc;

//using this instead of the old method for getting a timestamp so that the timestamp starts at 0
//when we join a server
//-Nbc66
time_t timestamp = NULL;

CTFDiscordRPC::CTFDiscordRPC()
{
	pszState = "";
	srctv_port = "";
	Q_memset(m_szLatchedMapname, 0, MAX_MAP_NAME);
	Q_memset(m_szLatchedMusicname, 0, DISCORD_FIELD_SIZE);
	m_bInitializeRequested = false;
	m_bInitialized = false;
}

CTFDiscordRPC::~CTFDiscordRPC()
{
	Discord_ClearPresence();
	Discord_Shutdown();
}

void CTFDiscordRPC::Shutdown()
{
	Discord_ClearPresence();
	Discord_Shutdown();
}

void CTFDiscordRPC::PostInit()
{
	if( pf_discord_rpc.GetBool() == 0 )
	{
		Shutdown();
	}
	InitializeDiscord();
	m_bInitializeRequested = true;

	// make sure to call this after game system initialized
	ListenForGameEvent( "server_spawn" );

	//hltv ip
	//-Nbc66
	ListenForGameEvent( "hltv_status" );
}

void CTFDiscordRPC::Update( float frametime )
{
	if (pf_discord_rpc.GetBool()==0)
	{
		Shutdown();
	}

	//if (m_bErrored)
	//	return;

	// NOTE: we want to run this even if they have use_discord off, so we can clear
	// any previous state that may have already been sent
	UpdateRichPresence();
	Discord_RunCallbacks();

}

void CTFDiscordRPC::OnReady(const DiscordUser* user)
{
	ConColorMsg(Color(114, 137, 218, 255), "[Rich Presence] Ready!\n");
	ConColorMsg(Color(114, 137, 218, 255), "[Rich Presence] User %s#%s - %s\n", user->username, user->discriminator, user->userId);

	g_discordrpc.Reset();
}

void CTFDiscordRPC::OnDiscordError(int errorCode, const char* szMessage)
{
	char buff[1024];
	Q_snprintf(buff, 1024, "[Rich Presence] Init failed. code %d - error: %s\n", errorCode, szMessage);
	Warning("%s", buff);

	// ignore 4000 until I figure out what exactly is causing it
	if( errorCode != 4000 )
		g_discordrpc.m_bErrored = true;
}


void CTFDiscordRPC::OnJoinGame(const char* joinSecret)
{
	ConColorMsg(Color(114, 137, 218, 255), "[Rich Presence] Join Game: %s\n", joinSecret);

	char szCommand[128];
	Q_snprintf(szCommand, sizeof(szCommand), "connect %s\n", joinSecret);
	engine->ExecuteClientCmd(szCommand);
}

//Spectating has been removed from discord, but ill still keep this here for anyone
//who needs to get the hltv ip address for whatever reason
//-Nbc66

void CTFDiscordRPC::OnSpectateGame(const char* spectateSecret)
{
	ConColorMsg(Color(114, 137, 218, 255), "[Rich Presence] Spectate Game: %s\n", spectateSecret);

	char szCommand[128];
	Q_snprintf( szCommand, sizeof( szCommand ), "connect %s\n", spectateSecret );
	engine->ExecuteClientCmd( szCommand );
}

void CTFDiscordRPC::OnJoinRequest(const DiscordUser* joinRequest)
{
	ConColorMsg(Color(114, 137, 218, 255), "[Rich Presence] Join Request: %s#%s\n", joinRequest->username, joinRequest->discriminator);
	ConColorMsg(Color(114, 137, 218, 255), "[Rich Presence] Join Request Accepted\n");
	Discord_Respond(joinRequest->userId, DISCORD_REPLY_YES);
}

void CTFDiscordRPC::SetLogo(void)
{

	const char* pszGameType = "BETA!";
	const char* pszImageLarge = "pf2_icon";

	// string for setting the picture of the class
	// you should name the small picture affter the class itself ex: Scout.jpg, Soldier.jpg, Pyro.jpg ...
	// you get it
	// -Nbc66
	const char* pszImageSmall = "";
	const char* pszImageText = "";

	if( ( engine->IsConnected() || engine->IsDrawingLoadingImage() ) && !engine->IsLevelMainMenuBackground() )
	{

		if( pszImageLarge != m_szLatchedMapname )
		{
			// stolen from KaydemonLP
			// -Nbc66
			pszImageLarge = GetRPCMapImage( m_szLatchedMapname );
		}

		//checks the players class
		C_TFPlayer* pTFPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
		if( pf_discord_class.GetBool() && pTFPlayer )
		{
			int iClass = pTFPlayer->GetPlayerClass()->GetClassIndex();
			int iTeam = pTFPlayer->GetTeamNumber();

			pszImageText = LocalizeDiscordString( g_aPlayerClassNames[ iClass ] );
			if( iClass != TF_CLASS_UNDEFINED && iTeam > LAST_SHARED_TEAM )
			{
				pszImageSmall = VarArgs( "%s_%s", g_aTeamNamesLowerRPC[ iTeam ], g_aPlayerClassNamesLower[ iClass ] );
				pszImageText = LocalizeDiscordString( g_aPlayerClassNames[ iClass ] );
			}
			else
			{
				pszImageSmall = "spectator";
				pszImageText = "Spectating";
			}
		}
	}
	
	
	//-Nbc66
	if (TFGameRules())
    {
		if (TFGameRules()->GetGameType() == TF_GAMETYPE_UNDEFINED)
		{
			pszGameType = "";
		}
		else if (TFGameRules()->GetGameType() == TF_GAMETYPE_CTF)
		{
			pszGameType = LocalizeDiscordString("#TF_GAMETYPE_CTF");
		}
		else if (V_strnicmp(m_szLatchedMapname, "tc_", 3) == 0)
		{
			pszGameType = LocalizeDiscordString("#TF_GAMETYPE_TC");
		}
		else if (V_strnicmp(m_szLatchedMapname, "ad_", 3) == 0)
		{
			pszGameType = LocalizeDiscordString("#TF_AttackDefend");
		}
		else if(TFGameRules()->IsInKothMode())
		{
			pszGameType = LocalizeDiscordString( "#TF_GAMETYPE_KOTH" );
		}
		else if (TFGameRules()->GetGameType() == TF_GAMETYPE_CP)
		{
			pszGameType = LocalizeDiscordString("#TF_GAMETYPE_CP");
		}
		else if( TFGameRules()->IsPayloadRace() )
		{
			pszGameType = LocalizeDiscordString( "#TF_GAMETYPE_ESCORT_RACE" );
		}
		else if (TFGameRules()->GetGameType() == TF_GAMETYPE_ESCORT)
		{
			pszGameType = LocalizeDiscordString("#TF_GAMETYPE_ESCORT");
		}
	}

	if (engine->IsConnected() == false || engine->IsLevelMainMenuBackground())
	{
		pszGameType = "BETA!";
	}

	m_sDiscordRichPresence.largeImageKey = pszImageLarge;
	m_sDiscordRichPresence.largeImageText = pszGameType;
	m_sDiscordRichPresence.smallImageKey = pszImageSmall;
	m_sDiscordRichPresence.smallImageText = pszImageText;
}

//Fuck me this function sucks but if you got a better solution im all ears
//Techno you better make this look like a actually good working function which can get
//Unicode characters to be converted to a char now that would be fucking epic
//but also impossible, for the time being we are stuck with this sorry
//-Nbc66
const char* CTFDiscordRPC::LocalizeDiscordString(const char *LocalizedString)
{

	const wchar_t* WcharLocalizedString = g_pVGuiLocalize->Find(LocalizedString);
	//char array is set this way to account for ASCII's
	//characters which are generaly 256 charachetrs with Windows-1252 8-bit charachter encoding
	//just dont fuck with the array size or you are going to have a bad time man
	//-Nbc66
	char CharLocalizedArray[256];
	g_pVGuiLocalize->ConvertUnicodeToANSI(WcharLocalizedString, CharLocalizedArray, sizeof(CharLocalizedArray));
	FinalCharLocalizedString = V_strdup(CharLocalizedArray);

	return FinalCharLocalizedString;

}

void CTFDiscordRPC::InitializeDiscord()
{
	DiscordEventHandlers handlers;
	Q_memset(&handlers, 0, sizeof(handlers));
	handlers.ready = &CTFDiscordRPC::OnReady;
	handlers.errored = &CTFDiscordRPC::OnDiscordError;
	handlers.joinGame = &CTFDiscordRPC::OnJoinGame;

	//Spectating was removed in 2020 just keeping it here for reference
	//-Nbc66
	handlers.spectateGame = &CTFDiscordRPC::OnSpectateGame;
	
	handlers.joinRequest = &CTFDiscordRPC::OnJoinRequest;

	char command[512];
	V_snprintf(command, sizeof(command), "%s -game \"%s\" -novid -steam\n", CommandLine()->GetParm(0), CommandLine()->ParmValue("-game"));
	Discord_Register(DISCORD_APP_ID, command);
	Discord_Initialize(DISCORD_APP_ID, &handlers, false, "");
	Reset();
	m_bInitialized = true;
}

bool CTFDiscordRPC::NeedToUpdate()
{

	if( // Music and map name need to be set in order for us to return False
		( m_szLatchedMapname[0] == '\0' && m_szLatchedMusicname[0] == '\0' ) )
		return false;

	return gpGlobals->realtime >= m_flLastUpdatedTime + DISCORD_UPDATE_RATE;
}

void CTFDiscordRPC::Reset()
{

	if (pf_discord_rpc.GetBool()==0)
	{
		Shutdown();
	}

	Q_memset(&m_sDiscordRichPresence, 0, sizeof(m_sDiscordRichPresence));
	m_sDiscordRichPresence.details = LocalizeDiscordString("#Discord_State_Menu");

	m_sDiscordRichPresence.state = m_szLatchedMusicname;

	m_sDiscordRichPresence.endTimestamp;

	SetLogo();
	Discord_UpdatePresence(&m_sDiscordRichPresence);
	if( !engine->IsLevelMainMenuBackground() )
	{
		delete[] FinalCharLocalizedString, srctv_port;
	}
}

void CTFDiscordRPC::UpdatePlayerInfo()
{
	C_TF_PlayerResource* pResource = GetTFPlayerResource();
	if (!pResource)
		return;

	int maxPlayers = gpGlobals->maxClients;
	int curPlayers = 0;

	const char* pzePlayerName = NULL;

	for (int i = 1; i < maxPlayers; i++)
	{
		if (pResource->IsConnected(i))
		{
			curPlayers++;
			if (pResource->IsLocalPlayer(i))
			{
				pzePlayerName = pResource->GetPlayerName(i);
			}
		}
	}

	//int iTimeLeft = TFGameRules()->GetTimeLeft();

	if (m_szLatchedHostname[0] != '\0')
	{
		if (cl_richpresence_printmsg.GetBool())
		{
			ConColorMsg(Color(114, 137, 218, 255), "[Discord] sending details of\n '%s'\n", m_szServerInfo);
		}
		m_sDiscordRichPresence.partySize = curPlayers;
		m_sDiscordRichPresence.partyMax = maxPlayers;
		m_sDiscordRichPresence.state = m_szLatchedHostname;
		//m_sDiscordRichPresence.state = szStateBuffer;
	}
}

void CTFDiscordRPC::FireGameEvent(IGameEvent* event)
{
	const char* type = event->GetName();



	if (Q_strcmp(type, "server_spawn") == 0)
	{
		//setup the discord timestamp to be 0 when we join a server
		//-Nbc66
		timestamp = time(0);

		Q_strncpy(m_szLatchedHostname, event->GetString("hostname"), 255);

	}

	//Check the master keyvalue for the hltv servers ip
	//-Nbc66
	if( !Q_strcmp( type, "hltv_status" ) )
	{
		srctv_port = V_strdup(event->GetString("master"));
	}
	else
	{
		srctv_port = NULL;
	}

}

void CTFDiscordRPC::UpdateRichPresence()
{

	if (!NeedToUpdate())
		return;

	m_flLastUpdatedTime = gpGlobals->realtime;

	// By default, our state is set to the music that's playing ( if there's any playing at all )
	m_sDiscordRichPresence.state = m_szLatchedMusicname;

	if( ( engine->IsConnected() || engine->IsDrawingLoadingImage() ) && !engine->IsLevelMainMenuBackground() )
	{
		//The elapsed timer function using <ctime>
		//this is for setting up the time when the player joins a server
		//-Nbc66
		//time_t iSysTime;
		//time(&iSysTime);
		//struct tm* tStartTime = NULL;
		//tStartTime = localtime(&iSysTime);
		//tStartTime->tm_sec = 0;

		V_snprintf(szStateBuffer, sizeof(szStateBuffer), "%s : %s", LocalizeDiscordString("#Discord_Map"), GetRPCMapName(m_szLatchedMapname));
		g_discordrpc.UpdatePlayerInfo();
		g_discordrpc.UpdateNetworkInfo();
		//starts the elapsed timer for discord rpc
		//-Nbc66
		m_sDiscordRichPresence.startTimestamp = timestamp;
		//sets the map name
		m_sDiscordRichPresence.details = szStateBuffer;
	}

	//checks if the loading bar is being drawn
	//and sets the discord status to "Currently is loading..."
	//-Nbc66
	//if (engine->IsDrawingLoadingImage() == true && !engine->IsLevelMainMenuBackground())
	//{
	//	m_sDiscordRichPresence.state = "";
	//	m_sDiscordRichPresence.details = "Currently loading...";
	//}

	SetLogo();

	Discord_UpdatePresence(&m_sDiscordRichPresence);
}


void CTFDiscordRPC::UpdateNetworkInfo()
{
	INetChannelInfo* ni = engine->GetNetChannelInfo();
	if( ni )
	{
		//ConWarning( "tv_port = %s\n", srctv_port );

		// adding -party here because secrets cannot match the party id
		m_sDiscordRichPresence.partyId = VarArgs( "%s-party", ni->GetAddress() );
		m_sDiscordRichPresence.joinSecret = ni->GetAddress();
	}

	//doesn't work until i can figure out how to get the source tv ip
	//-Nbc66: somewhere around 2019 lol
	// 
	//works now lol
	//-Nbc66: 15.02.2022
	if( (srctv_port != NULL) && (srctv_port[0] == '\0') )
	{
		m_sDiscordRichPresence.spectateSecret = NULL;
	}
	else
	{
		m_sDiscordRichPresence.spectateSecret = srctv_port;
	}
	
}

void CTFDiscordRPC::LevelInitPreEntity()
{
	// we cant update our presence here, because if its the first map a client loaded,
	// discord api may not yet be loaded, so latch
	Q_strcpy(m_szLatchedMapname, MapName());
	//V_snprintf(szStateBuffer, sizeof(szStateBuffer), "MAP: %s", m_szLatchedMapname);
	// important, clear last update time as well
	m_flLastUpdatedTime = MAX(0, gpGlobals->realtime - DISCORD_UPDATE_RATE);

	Reset();
}

void CTFDiscordRPC::LevelShutdownPostEntity()
{
	m_flLastUpdatedTime = MAX( 0, gpGlobals->realtime - DISCORD_UPDATE_RATE );
	Reset();
}