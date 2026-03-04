//========= Copyright LOLOLOLOLOLOLOLOLOL, All rights reserved. ===============//
//
// Purpose: Altering default convar values
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "pf_convar_setup.h"

CPFConvar g_ConVarList[] =
{
#ifdef CLIENT_DLL
	{
		"cl_cmdrate",	// name
		40,				// min
		100,				// max
		"66",			// default
	},
	{
		"cl_updaterate",
		40,
		100,
		"66",
	},
	{
		"cl_interp",
		"0.030303"

	},
	{
		"cl_smoothtime",
		"0.05"
	},
	{
		"cl_cmdrate",	// name
		"66",			// default
	},
	{
		"cl_updaterate",	// name
		"66",			// default
	},
#else
	{
		"sv_maxrate",
		"80000",
	},
	{
		"sv_mincmdrate",	// name
		"40",					// default
	},
	{
		"sv_maxcmdrate",
		"66",
	},
	{
		"sv_minupdaterate",
		"40",
	},
	{
		"sv_maxupdaterate",
		"66",
	},
	{
		"sv_clockcorrection_msecs",
		"30",
	},
	{
		"sv_client_max_interp_ratio",
		"5",
	},
#endif
};

CConvarSys::CConvarSys() : CAutoGameSystem("CConvarSys")
{
}

void CConvarSys::PostInit()
{
	ApplyAllConvars();
}

void CConvarSys::ApplyAllConvars()
{
	bool bFailed = false;
	for (int i = 0; i < sizeof(g_ConVarList) / sizeof(*g_ConVarList); i++)
	{
		CPFConvar cConvar = g_ConVarList[i];
		if (!cConvar.ApplyConvar())
			bFailed = true;
	}

	if (bFailed)
	{
#ifdef CLIENT_DLL
		Warning("One or more client convars have failed to changed. See above for failed changes.\n");
#else
		Warning("One or more server convars have failed to changed. See above for failed changes.\n");
#endif
	}
	else
	{
#ifdef CLIENT_DLL
		ConColorMsg(Color(64, 255, 255, 255), "Client convars changed successfully.\n");
#else
		ConColorMsg(Color(64, 255, 255, 255), "Server convars changed successfully.\n");
#endif
	}
}

static CConvarSys s_ConvarSys;

bool CPFConvar::ApplyConvar()
{
	ConVar* ChangedCon = g_pCVar->FindVar(m_sConVar);
	if (ChangedCon)
	{
		if (cChangeType == CHANGE_CONSTRAINTS)
		{
			ChangedCon->SetDefault(m_pDefaultVal);
			ChangedCon->SetMin(m_fMin);
			ChangedCon->SetMax(m_fMax);
			//ConColorMsg(Color(64, 255, 255, 255), "Changed constraints for \"%s\"\n- def. %s - min. %f - max. %f\n", m_sConVar, m_pDefaultVal, m_fMin, m_fMax);
		}
		else
		{
			ChangedCon->SetValue(m_pDefaultVal);
			ChangedCon->SetDefault(m_pDefaultVal);
			//ConColorMsg(Color(64, 255, 255, 255), "Changed default value for \"%s\" to \"%s\".\n", m_sConVar, m_pDefaultVal);
		}
		return true;
	}

	Warning("ConVar \"%s\" does not exist!\n", m_sConVar);
	return false;
}

CPFConvar::CPFConvar(const char* name, char* def)
{
	m_sConVar = name;
	m_pDefaultVal = def;
	cChangeType = CHANGE_DEFVALUE;
}

CPFConvar::CPFConvar(const char* name, int min, int max, char* def)
{
	m_sConVar = name;
	m_fMin = min;
	m_fMax = max;
	m_pDefaultVal = def;
	cChangeType = CHANGE_CONSTRAINTS;
}

CPFConvar::CPFConvar(const char* name, float min, float max, char* def)
{
	m_sConVar = name;
	m_fMin = min;
	m_fMax = max;
	m_pDefaultVal = def;
	cChangeType = CHANGE_CONSTRAINTS;
}

