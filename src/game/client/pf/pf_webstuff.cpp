//=============================================================================
//
// Purpose: Class to handle steam/prefortress website stuff
// Based on tf2c's CTFNotificationManager 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "pf_mainmenu_override.h"
#include "pf_webstuff.h"
#include "filesystem.h"

const char *COM_GetModDirectory();

#define CREDITS_URL WEBSITE_URL "/credits.txt"
#define NEWS_URL WEBSITE_URL "/news.txt"

static CPFWebStuff g_PFWebStuff;
CPFWebStuff *GetWebStuff()
{
	return &g_PFWebStuff;
}

CPFWebStuff::CPFWebStuff() : CAutoGameSystemPerFrame( "CPFWebStuff" )
{
	m_hRequest = NULL;
}

bool CPFWebStuff::Init()
{
	m_flNextUpdate = 1.0f;
	m_iHasCredits = -1;
	m_pDownloadedCredits = nullptr;
	m_iCurrentServerCount = -1;
	m_iOldServerCount = -1;
	m_pMatchMakingServers = nullptr;
	m_pHTTP = nullptr;
	m_szVerString[0] = '\0';
	m_bNeedsUpdating = false;
	return true;
}

void CPFWebStuff::PostInit()
{
	if( steamapicontext )
	{
		m_pMatchMakingServers = steamapicontext->SteamMatchmakingServers();
		m_pHTTP = steamapicontext->SteamHTTP();
	}

	// Read the version file
	if( g_pFullFileSystem->FileExists( "version.txt" ) )
	{
		FileHandle_t fh = filesystem->Open( "version.txt", "r", "MOD" );
		int file_len = filesystem->Size( fh );
		char *versionText = new char[file_len + 1];

		filesystem->Read( (void *)versionText, file_len, fh );
		versionText[file_len] = 0; // null terminator

		filesystem->Close( fh );

		Q_snprintf( m_szVerString, sizeof( m_szVerString ), versionText + 8 );

		delete[] versionText;
	}

	// we only need the credits once
	CheckCredits();
	// we only need the news once
	CheckNews();
}

void CPFWebStuff::Update( float frametime )
{
	if( gpGlobals->curtime > m_flNextUpdate )
	{
		if( m_pMatchMakingServers )
			CheckServerCount();

		m_flNextUpdate = gpGlobals->curtime + 30.0f;
	}
}

// when a server responds you can do a thing here
void CPFWebStuff::ServerResponded( HServerListRequest hRequest, int iServer )
{
	gameserveritem_t* serveritem = steamapicontext->SteamMatchmakingServers()->GetServerDetails( hRequest, iServer );
	if( serveritem )
	{
		++m_iCurrentServerCount;
	}
}

// when all the servers have responded you can do another thing here
void CPFWebStuff::RefreshComplete( HServerListRequest hRequest, EMatchMakingServerResponse response )
{
	if( m_iCurrentServerCount != m_iOldServerCount )
	{
		if( guiroot )
		{
			guiroot->UpdateServerCount();
		}
	}

	m_iOldServerCount = m_iCurrentServerCount;

	m_pMatchMakingServers->ReleaseRequest( m_hRequest );
	m_hRequest = NULL;
}

void CPFWebStuff::CheckServerCount()
{
	if( !steamapicontext->SteamUtils() )
		return;

	if( m_hRequest )
	{
		m_pMatchMakingServers->ReleaseRequest( m_hRequest );
		return;
	}

	CUtlVector<MatchMakingKeyValuePair_t> vecServerFilters;

	MatchMakingKeyValuePair_t filter;
	Q_strncpy( filter.m_szKey, "gamedir", sizeof( filter.m_szKey ) );
	Q_strncpy( filter.m_szValue, COM_GetModDirectory(), sizeof(filter.m_szKey));
	vecServerFilters.AddToTail( filter );
	MatchMakingKeyValuePair_t *pFilters = vecServerFilters.Base();

	m_iCurrentServerCount = 0;
	m_hRequest = m_pMatchMakingServers->RequestInternetServerList( steamapicontext->SteamUtils()->GetAppID(), &pFilters, vecServerFilters.Count(), this );
}

void CPFWebStuff::CheckCredits()
{
	m_iHasCredits = 0;

	if( !m_pHTTP )
		return;

	m_pCreditsKVRequest = m_pHTTP->CreateHTTPRequest( k_EHTTPMethodGET, CREDITS_URL );

	SteamAPICall_t call;
	m_pHTTP->SendHTTPRequest( m_pCreditsKVRequest, &call );
	m_CreditsKVReady.Set( call, this, &CPFWebStuff::OnHTTPCreditsRequestCompleted );
}

void CPFWebStuff::CheckNews()
{
	m_iHasNews = 0;

	if( !m_pHTTP )
		return;

	m_pNewsKVRequest = m_pHTTP->CreateHTTPRequest( k_EHTTPMethodGET, NEWS_URL );

	SteamAPICall_t call;
	m_pHTTP->SendHTTPRequest( m_pNewsKVRequest, &call );
	m_NewsKVReady.Set( call, this, &CPFWebStuff::OnHTTPNewsRequestCompleted );
}

void CPFWebStuff::OnHTTPCreditsRequestCompleted( HTTPRequestCompleted_t *CallResult, bool iofailure )
{
	DevMsg( "HTTP Request %i completed: %i\n", CallResult->m_hRequest, CallResult->m_eStatusCode );

	if( CallResult->m_eStatusCode == 200 )
	{
		uint32 iBodysize;
		m_pHTTP->GetHTTPResponseBodySize( CallResult->m_hRequest, &iBodysize );

		uint8 iBodybuffer[8192];
		m_pHTTP->GetHTTPResponseBodyData( CallResult->m_hRequest, iBodybuffer, iBodysize );

		char result[8192];
		V_strncpy( result, (char *)iBodybuffer, MIN( (int)(iBodysize + 1), sizeof( result ) ) );

		KeyValues *kv = new KeyValues( "" );
		if( kv->LoadFromBuffer( "Credits", result ) )
		{
			m_pDownloadedCredits = kv;
			m_iHasCredits = 1;
		}
	}

	m_pHTTP->ReleaseHTTPRequest( CallResult->m_hRequest );
}

void CPFWebStuff::OnHTTPNewsRequestCompleted( HTTPRequestCompleted_t *CallResult, bool iofailure )
{
	DevMsg( "HTTP Request %i completed: %i\n", CallResult->m_hRequest, CallResult->m_eStatusCode );

	if( CallResult->m_eStatusCode == 200 )
	{
		uint32 iBodysize;
		m_pHTTP->GetHTTPResponseBodySize( CallResult->m_hRequest, &iBodysize );

		uint8 iBodybuffer[8192];
		m_pHTTP->GetHTTPResponseBodyData( CallResult->m_hRequest, iBodybuffer, iBodysize );

		char result[4096];
		V_strncpy( result, (char *)iBodybuffer, MIN( (int)(iBodysize + 1), sizeof( result ) ) );

		KeyValues *kv = new KeyValues( "" );
		if( kv->LoadFromBuffer( "News", result ) )
		{
			m_pDownloadedNews = kv;
			m_iHasNews = 1;

			// Do our version check here
			int arrGameVer[3];
			arrGameVer[0] = kv->GetInt( "gamestate", 0 );
			arrGameVer[1] = kv->GetInt( "version", 0 );
			arrGameVer[2] = kv->GetInt( "patch", 0 );

			// need to split up the version string so we can run a comparison
			CUtlVector<char *, CUtlMemory<char *> > splitString;
			int nState, nVer, nPatch;
			Q_SplitString( GetWebStuff()->m_szVerString, ".", splitString );

			nState = atoi( splitString[0] );
			nVer = atoi( splitString[1] );

			// If we don't have a third number just set it to 0;
			if( splitString.Size() < 3 )
				nPatch = 0;
			else
				nPatch = atoi( splitString[2] );

			// if any of the versions are lower than our local version we obviously have an update
			if( arrGameVer[0] > nState || arrGameVer[1] > nVer || arrGameVer[2] > nPatch )
				m_bNeedsUpdating = true;

			if( guiroot )
			{
				guiroot->UpdateNews();
			}
		}
	}

	m_pHTTP->ReleaseHTTPRequest( CallResult->m_hRequest );
}