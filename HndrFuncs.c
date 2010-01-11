#include "config.h"

#ifdef CONFIG_HANDERA

#include <PalmOS.h>
#include "Vga.h"
#include "HndrFuncs.h"

/*
 * FUNCTION:				HndrMoveObject
 * DESCRIPTION:			this routine moves the specified object by x_diff and y_diff
 * PARAMETERS:			frmP - pointer to the form containing the object
 * 									objIndex - index of the object
 * 									x_diff - to move left/right
 * 									y_diff - to move up/down
 * 									draw - if through the object will be redrawn
 * RETURNS:					nothing
 */
void HndrMoveObject( 
		FormType * frmP, UInt16 objIndex, 
		Int16 x_diff, Int16 y_diff, 
		Boolean draw )
{
    RectangleType r;
    
    FrmGetObjectBounds(frmP, objIndex, &r);
    if (draw)
    {
        RctInsetRectangle(&r, -2);   //need to erase the frame as well
        WinEraseRectangle(&r, 0);
        RctInsetRectangle(&r, 2);
    }    
    r.topLeft.y += y_diff;
		r.topLeft.x += x_diff;
    FrmSetObjectBounds(frmP, objIndex, &r);
}

/*
 * FUNCTION:				HndrCenterForm
 * DESCRIPTION:			this function places the passed form into the center of the screen
 * PARAMTERS:				frmP - pointer to the form to be centered
 * 									draw - if true the form will be redrawn
 * RETURNS:					nothing
 */
void HndrCenterForm( FormType * frmP, Boolean draw )
{
	RectangleType r;
	Coord					extentx, extenty;
	Coord					winx, winy;
	WinHandle			winH;
	WinHandle			actWinH;

	actWinH = WinGetDrawWindow();
	winH = FrmGetWindowHandle( frmP );
	WinSetDrawWindow( winH );
	WinGetWindowExtent( &winx, &winy );
	WinSetDrawWindow( actWinH );
	WinGetDisplayExtent( &extentx, &extenty );

	r.extent.x = winx;
	r.extent.y = winy;
	r.topLeft.x = (extentx - winx)>>1;
	r.topLeft.y = (extenty - winy)>>1;

	WinSetBounds( winH, &r );
}

#endif /* CONFIG_HANDERA */
