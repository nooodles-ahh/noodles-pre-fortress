#include "cbase.h"
#include "baseentity.h"
#include "pf_logic_multiflag.h"
#include "tf_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( pf_logic_multiflag, CPFLogic_MultiFlag );

BEGIN_DATADESC( CPFLogic_MultiFlag )
DEFINE_KEYFIELD( m_iFlagCount, FIELD_INTEGER, "flagcount" ),
END_DATADESC()
