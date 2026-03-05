//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Player-driven Voting System for Multiplayer Source games (currently implemented for TF2)
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_VOTEISSUES_H
#define TF_VOTEISSUES_H

#ifdef _WIN32
#pragma once
#endif

#include "vote_controller.h"

//-----------------------------------------------------------------------------
// VTABLE GLOBAL: https://raw.githubusercontent.com/sigsegv-mvm/mvm-reversed/7ce8ac98fe187a07d71df87b02b4c038548837e9/Useful/symbols/ServerLinux-server_srv.txt
// VTABLE LOCAL: https://github.com/sigsegv-mvm/mvm-reversed/blob/7ce8ac98fe187a07d71df87b02b4c038548837e9/Useful/vtable/server_srv/CBaseTFIssue.txt
//-----------------------------------------------------------------------------

class CBaseTFIssue : public CBaseIssue // Base class concept for vote issues (i.e. Kick Player).  Created per level-load and destroyed by CVoteController's dtor.
{
public:
	CBaseTFIssue( const char* pszName ) : CBaseIssue( pszName ) { }

	virtual void		ExecuteCommand( void );						// Where the magic happens.  Do your thing.
	virtual void		ListIssueDetails( CBasePlayer *pForWhom );	// Someone would like to know all your valid details
};

class CKickIssue : public CBaseTFIssue
{
public:
	virtual bool		IsEnabled( void );							// Query the issue to see if it's enabled
	virtual bool		CanCallVote( int nEntIndex, const char *pszDetails, vote_create_failed_t &nFailCode, int &nTime ); // Can this guy hold a vote on this issue?
	virtual bool		IsTeamRestrictedVote() { return true; }
	virtual const char	*GetDetailsString( void );
	virtual const char *GetDisplayString( void );					// The string that will be passed to the client for display
	virtual void		ExecuteCommand( void );						// Where the magic happens.  Do your thing.
	virtual const char *GetVotePassedString( void );				// Get the string an issue would like to display whenit passes.
	virtual void		SetIssueDetails( const char *pszDetails );

	CKickIssue() : CBaseTFIssue( "Kick" ) { }
	CUtlString m_szNetworkIDString;
};

class CRestartGameIssue : public CBaseTFIssue
{
public:
	void				Init( void );
	virtual bool		IsEnabled( void );							// Query the issue to see if it's enabled
	virtual const char *GetDisplayString( void );					// The string that will be passed to the client for display
	virtual void		ExecuteCommand( void );						// Where the magic happens.  Do your thing.
	virtual const char *GetVotePassedString( void );				// Get the string an issue would like to display whenit passes.

	CRestartGameIssue() : CBaseTFIssue( "RestartGame" ) { }
};

class CScrambleTeams : public CBaseTFIssue
{
public:
	void				Init( void );
	virtual bool		IsEnabled( void );							// Query the issue to see if it's enabled
	virtual const char *GetDisplayString( void );					// The string that will be passed to the client for display
	virtual void		ExecuteCommand( void );						// Where the magic happens.  Do your thing.
	virtual const char *GetVotePassedString( void );				// Get the string an issue would like to display whenit passes.

	CScrambleTeams() : CBaseTFIssue( "ScrambleTeams" ) { }
};

class CChangeLevelIssue : public CBaseTFIssue
{
public:
	virtual bool		IsEnabled( void );							// Query the issue to see if it's enabled
	virtual const char *GetDisplayString( void );					// The string that will be passed to the client for display
	virtual void		ExecuteCommand( void );						// Where the magic happens.  Do your thing.
	virtual const char *GetVotePassedString( void );				// Get the string an issue would like to display whenit passes.
	virtual bool		CanCallVote( int nEntIndex, const char *pszDetails, vote_create_failed_t &nFailCode, int &nTime ); // Can this guy hold a vote on this issue?

	CChangeLevelIssue() : CBaseTFIssue( "ChangeLevel" ) { }
};

class CNextLevelIssue : public CBaseTFIssue
{
public:
	virtual bool		IsEnabled( void );							// Query the issue to see if it's enabled
	virtual bool		CanCallVote( int nEntIndex, const char *pszDetails, vote_create_failed_t &nFailCode, int &nTime );
	virtual const char *GetDisplayString( void );					// The string that will be passed to the client for display
	virtual void		ExecuteCommand( void );						// Where the magic happens.  Do your thin
	virtual const char *GetVotePassedString( void );				// Get the string an issue would like to display whenit passes.
	virtual bool		IsYesNoVote( void );
	virtual bool        GetVoteOptions( CUtlVector <const char*> &vecNames );	// We use this to generate options for votingg.

	CNextLevelIssue() : CBaseTFIssue( "NextLevel" ) { }
};

class CExtendLevelIssue : public CBaseTFIssue
{
public:
	virtual bool		IsEnabled(void);							// Query the issue to see if it's enabled
	virtual const char* GetDisplayString(void);					// The string that will be passed to the client for display
	virtual void		ExecuteCommand(void);						// Where the magic happens.  Do your thing.
	virtual const char* GetVotePassedString(void);				// Get the string an issue would like to display whenit passes.

	CExtendLevelIssue() : CBaseTFIssue("ExtendLevel") { }
};

#endif // VOTE_CONTROLLER_H
