//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "ienginevgui.h"
#include "pf_menupanelbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CPFMenuPanelBase::CPFMenuPanelBase( vgui::Panel *parent, const char *name ) : BaseClass( parent, name, false, false )
{
	SetParent( parent );
	SetProportional( true );
	SetVisible( true );
	SetDeleteSelfOnClose( true );

	SetMoveable( false );
	SetSizeable( false );

	int x, y;
	int width, height;
	GetParent()->GetBounds( x, y, width, height );
	SetSize( width, height );
	SetPos( 0, 0 );

	InvalidateLayout( true, true );
}

//-----------------------------------------------------------------------------
// Purpose: Applies scheme settings
//-----------------------------------------------------------------------------
void CPFMenuPanelBase::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	SetProportional( true );
}

//-----------------------------------------------------------------------------
// Purpose: Command handler
//-----------------------------------------------------------------------------
void CPFMenuPanelBase::OnCommand( const char *command )
{
	engine->ExecuteClientCmd( command );
}

void CPFMenuPanelBase::PaintBackground()
{
	SetPaintBackgroundType( 0 );
	BaseClass::PaintBackground();
}