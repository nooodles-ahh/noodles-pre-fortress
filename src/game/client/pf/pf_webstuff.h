//=============================================================================
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef PF_WEBSTUFF_H
#define PF_WEBSTUFF_H
#ifdef _WIN32
#pragma once
#endif

#include "igamesystem.h"
#include "steam/steam_api.h"

#define WEBSITE_URL "https://prefortress.com"

class CPFWebStuff : public CAutoGameSystemPerFrame, public ISteamMatchmakingServerListResponse
{
public:
	CPFWebStuff();

	// IGameSystem methods
	virtual bool Init();
	virtual void PostInit();
	virtual char const *Name() { return "CPFWebStuff"; }
	virtual void Update( float frametime );

	// Server has responded ok with updated data
	virtual void ServerResponded( HServerListRequest hRequest, int iServer );
	// Server has failed to respond
	virtual void ServerFailedToRespond( HServerListRequest hRequest, int iServer ){}
	// A list refresh you had initiated is now 100% completed
	virtual void RefreshComplete( HServerListRequest hRequest, EMatchMakingServerResponse response );

	void OnHTTPCreditsRequestCompleted( HTTPRequestCompleted_t *CallResult, bool iofailure );
	void OnHTTPNewsRequestCompleted( HTTPRequestCompleted_t *CallResult, bool iofailure );

	int GetCurrentServerCount() { return m_iCurrentServerCount; }

	KeyValues *GetCredits() { return m_pDownloadedCredits; }
	bool HasCredits() { return !!m_iHasCredits; }

	KeyValues *GetNews() { return m_pDownloadedNews; }
	bool HasNews() { return !!m_iHasNews; }
	
	char m_szVerString[32];
	bool m_bNeedsUpdating;

private:
	void CheckServerCount();
	void CheckCredits();
	void CheckNews();

private:

	// Server count stuff
	HServerListRequest m_hRequest;
	float m_flNextUpdate;
	int m_iCurrentServerCount;
	int m_iOldServerCount;

	// Credits stuff
	int m_iHasCredits;
	KeyValues *m_pDownloadedCredits;
	HTTPRequestHandle m_pCreditsKVRequest;
	CCallResult< CPFWebStuff, HTTPRequestCompleted_t > m_CreditsKVReady;

	// News stuff
	int m_iHasNews;
	KeyValues *m_pDownloadedNews;
	HTTPRequestHandle m_pNewsKVRequest;
	CCallResult< CPFWebStuff, HTTPRequestCompleted_t > m_NewsKVReady;

	// Steam interfaces
	ISteamMatchmakingServers *m_pMatchMakingServers;
	ISteamHTTP *m_pHTTP;
};

CPFWebStuff *GetWebStuff();

#endif