//========= Copyright LOLOLOLOLOLOLOLOLOL, All rights reserved. ===============//
//
// Purpose: Altering default convar values
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
class CConvarSys : public CAutoGameSystem
{	
public:
	CConvarSys();
	void PostInit() OVERRIDE;
private:
	void ApplyAllConvars();
};

enum ChangeType
{
	CHANGE_CONSTRAINTS = 0,
	CHANGE_DEFVALUE = 1
};

class CPFConvar
{
public:
	// cvar and a new value
	CPFConvar(const char*, char*); 
	// cvar with new constaints
	CPFConvar(const char*, int, int, char*);
	CPFConvar(const char*, float, float, char*);
	bool ApplyConvar();

private:
	const char *m_sConVar;
	float m_fMin;
	float m_fMax;
	char *m_pDefaultVal;
	char cChangeType;
};