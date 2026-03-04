#include "cbase.h"
#include "pf_tooltip.h"
#include "pf_mainmenu_override.h"
#include <vgui/ILocalize.h>
#include <vgui/ISystem.h>
#include <vgui/ISurface.h>
#include "c_tf_player.h"
#include "tf_controls.h"
#include "vgui_controls/TextImage.h"

using namespace vgui;
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CMainMenuToolTip::CMainMenuToolTip( vgui::Panel *parent, const char *name ) : BaseClass( parent, name )
{
}

void CMainMenuToolTip::PerformLayout()
{
    if( ShouldLayout() ) 
    {
        int iWide = 0;
        int iTall = 0;

        // Get number of, what should be, labels controls
        int iChildCount = pPanel->GetChildCount();
        if( iChildCount > 0 ) 
        {
            int iYPos = 0;
            int iXPos = 0;

            int i = 0;
            do
            {
                vgui::Label* pChild = (vgui::Label *)pPanel->GetChild( i );
                char pText[2];
                // check we have a label and that the label is not empty
                if( pChild && (pChild->GetText( pText, 2 ), pText[0] != '\0') )
                {
                    pChild->SizeToContents();
                    pChild->GetPos( iXPos, iYPos );

                    int iChildWide = pChild->GetWide();
                    iWide = MAX( 0, (iXPos * 2) + iChildWide );

                    int iChildTall = pChild->GetTall();
                    iTall = MAX( 0, (iYPos * 2) + iChildTall );
                }
                ++i;
            } 
            while( i < iChildCount );
        }
        pPanel->SetSize( iWide, iTall );
        pPanel->SetVisible( true );

        // not done here in live but eh
        pPanel->SetKeyBoardInputEnabled( false );
        pPanel->SetMouseInputEnabled( false );

        PositionWindow( pPanel );
    }
    return;
}

void CMainMenuToolTip::HideTooltip( void )
{
    if( pPanel ) 
    {
        pPanel->SetVisible( false );
    }
    BaseClass::HideTooltip();
}

void CMainMenuToolTip::SetText( const char *text )
{
    if( pPanel ) 
    {
        _isDirty = true;
        if( text )
        {
            // Check if the text is a localized string
            if( (*text != '#') )
            {
                pPanel->SetDialogVariable( "tiptext", text );
            }
            else
            {
                wchar_t *wszText = g_pVGuiLocalize->Find( text );
                pPanel->SetDialogVariable( "tiptext", wszText );
            }
        }

        // tipsubtext is empty?
        pPanel->SetDialogVariable( "tipsubtext", "" );
    }
    return;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFTextToolTip::CTFTextToolTip( vgui::Panel *parent, const char *name ) : BaseClass( parent, name )
{
}

/* CTFTextToolTip::PerformLayout() */
void CTFTextToolTip::PerformLayout( void )
{
    if( ShouldLayout() ) 
    {
        _isDirty = false;

        int iTall = 0;
        int iChildCount = pPanel->GetChildCount();
        if( iChildCount > 0 )
        {
            int i = 0;
            do 
            {
                vgui::Label *pChild = (vgui::Label *)pPanel->GetChild( i );
                char pText[2];
                // check we have a label and that the label is not empty
                if( pChild && (pChild->GetText( pText, 2 ), pText[0] != '\0') )
                {
                    int iXPos, iYPos;
                    pChild->GetPos( iXPos, iYPos );
                    
                    // Set a new width with extra space from the label's x pos
                    int iNewWide = pPanel->GetWide() + (iXPos * -2);
                    // Resize the label to fit our newly created 
                    TextImage *pTextImage = pChild->GetTextImage();
                    pTextImage->ResizeImageToContentMaxWidth( iNewWide );
                    pChild->SizeToContents();
                    // Set the width to our actual desired width again?
                    pChild->SetWide( iNewWide );
                    pChild->InvalidateLayout( true, false );

                    // The x and y didn't seem to change but I'll trust it for now
                    pChild->GetPos( iXPos, iYPos );
                    iTall = MAX( 0, pChild->GetTall() + (iYPos * 2) );
                }
                ++i;
            } 
            while( i < iChildCount );
        }
        pPanel->SetSize( pPanel->GetWide(), iTall );
        pPanel->SetVisible( true );

        // not done here in live but eh
        pPanel->SetKeyBoardInputEnabled( false );
        pPanel->SetMouseInputEnabled( false );

        PositionWindow( pPanel );
    }
}

void CTFTextToolTip::PositionWindow( Panel *pTipPanel )
{
    int iTipW, iTipH;
    pTipPanel->GetSize( iTipW, iTipH );

    int cursorX, cursorY;
    vgui::input()->GetCursorPos( cursorX, cursorY );

    int wide, tall;
    wide = tall = 0;

    // Live does something else I can/dont want to decipher right now
    // Just get the parent panel and use its sizing to to determine when the
    // tooltip is gonna be drawn off its drawn area
    Panel *pParent = pTipPanel->GetParent();
    if( pParent )
    {
        pParent->GetSize( wide, tall );
        // Get the parent position
        int iX, iY;
        pParent->GetPos( iX, iY );
        // offset the cursor position because our parent is not the full size of the screen
        // and if we don't the tip will be drawn far further down and right
        cursorX -= iX;
        cursorY -= iY;
    }

    // Rest is the same as BaseTooltip
    if( wide - iTipW > cursorX )
    {
        cursorY += 20;
        // menu hanging right
        if( tall - iTipH > cursorY )
        {
            // menu hanging down
            pTipPanel->SetPos( cursorX, cursorY );
        }
        else
        {
            // menu hanging up
            pTipPanel->SetPos( cursorX, cursorY - iTipH - 20 );
        }
    }
    else
    {
        // menu hanging left
        if( tall - iTipH > cursorY )
        {
            // menu hanging down
            pTipPanel->SetPos( cursorX - iTipW, cursorY );
        }
        else
        {
            // menu hanging up
            pTipPanel->SetPos( cursorX - iTipW, cursorY - iTipH - 20 );
        }
    }
}

/* CTFTextToolTip::SetText(char const*) */
void CTFTextToolTip::SetText( const char *text )
{
    _isDirty = true;
    BaseClass::SetText( text );
}
