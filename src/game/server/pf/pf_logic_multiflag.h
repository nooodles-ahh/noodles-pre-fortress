#ifndef PF_LOGIC_MULTIFLAG_H
#define PF_LOGIC_MULTIFLAG_H
#ifdef _WIN32
#pragma once
#endif

#include "basemultiplayerplayer.h"

class CPFLogic_MultiFlag : public CPointEntity
{
public:
	DECLARE_CLASS( CPFLogic_MultiFlag, CPointEntity );
	DECLARE_DATADESC();

private:
	int m_iFlagCount;
};


#endif
