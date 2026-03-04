//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#ifndef PF_HANDS_H
#define PF_HANDS_H
#include "c_baseanimating.h"

// Model versions of muzzle flashes
class C_PFVMHands : public C_BaseAnimating
{
	DECLARE_CLASS( C_PFVMHands, C_BaseAnimating );
public:
	static C_PFVMHands *CreateViewModelHands( int iClass, C_BaseEntity *pParent);
	bool	InitializeVMHands( int iClass, C_BaseEntity *pParent );
	virtual int	InternalDrawModel( int flags );
	RenderGroup_t GetRenderGroup();
	void	ClientThink( void );
	virtual void SetClassModel( int iClass );
	
	int			m_iClass;
};

#endif //PF_HANDS_H