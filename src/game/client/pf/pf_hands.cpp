//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "c_tf_player.h"
#include "pf_hands.h"

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pszModelName - 
//			vecOrigin - 
//			vecForceDir - 
//			vecAngularImp - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
C_PFVMHands *C_PFVMHands::CreateViewModelHands( int iClass, C_BaseEntity *pParent )
{
	C_PFVMHands *pHands = new C_PFVMHands;
	if ( !pHands )
		return NULL;

	if ( !pHands->InitializeVMHands( iClass, pParent ) )
		return NULL;

	return pHands;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_PFVMHands::InitializeVMHands( int iClass, C_BaseEntity *pParent )
{
	// Probably a better way to decide all this, but it works
	C_BaseViewModel *pViewModel = dynamic_cast<C_BaseViewModel *>(pParent);
	const char *pchArmName = GetPlayerClassData( iClass )->m_szArmModelName;
	if( !pchArmName )
		return false;

	AddEffects( EF_BONEMERGE | EF_BONEMERGE_FASTCULL | EF_PARENT_ANIMATES );
	if ( !pViewModel || InitializeAsClientEntity( pchArmName, RENDER_GROUP_VIEW_MODEL_TRANSLUCENT ) == false )
	{
		Release();
		return false;
	}
	
	m_iClass = iClass;
	SetParent( pViewModel );
	SetLocalOrigin( vec3_origin );
	FollowEntity( pViewModel, true );
	m_nSkin = pViewModel->GetOwner()->GetTeamNumber()-2;
	//SetLightingOrigin( pViewModel->GetLightingOrigin() );

	AddSolidFlags( FSOLID_NOT_SOLID );
	SetNextClientThink( CLIENT_THINK_ALWAYS );
	return true;
}

extern ConVar cl_flipviewmodels;
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	C_PFVMHands::InternalDrawModel( int flags )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	bool bUseInvulnMaterial = ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_INVULNERABLE ) );
	if( bUseInvulnMaterial )
	{
		modelrender->ForcedMaterialOverride( *pPlayer->GetInvulnMaterialRef() );
	}

	CMatRenderContextPtr pRenderContext( materials );
	if ( cl_flipviewmodels.GetBool() )
		pRenderContext->CullMode( MATERIAL_CULLMODE_CW );

	int ret = BaseClass::InternalDrawModel( flags );

	pRenderContext->CullMode( MATERIAL_CULLMODE_CCW );

	if( bUseInvulnMaterial )
	{
		modelrender->ForcedMaterialOverride( NULL );
	}

	return ret;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : RenderGroup_t
//-----------------------------------------------------------------------------
RenderGroup_t C_PFVMHands::GetRenderGroup()
{
	return RENDER_GROUP_VIEW_MODEL_TRANSLUCENT;
}

//-----------------------------------------------------------------------------
// Purpose: How does spectating werk. I love orphaning viewmodels!
//	Until I figure it out just keep fucking checking. Would rather only
// 	destroy them when asked instead of always checking if they should be
//-----------------------------------------------------------------------------
void C_PFVMHands::ClientThink( void )
{
	if( !GetMoveParent() )
	{
		Release();
		return;
	}
}

void C_PFVMHands::SetClassModel( int iClass )
{
	m_iClass = iClass;
	const char *pchArmName = GetPlayerClassData( iClass )->m_szArmModelName;
	if( pchArmName )
	{
		SetModel( pchArmName );
	}
}
