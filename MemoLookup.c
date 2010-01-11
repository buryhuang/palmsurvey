#include <PalmOS.h>
#include "ProjectsDB.h"
#include "Projects.h"
#include "MemoLookup.h"

#ifndef __PROJECTS_CONFIG_H__
	#error "include config.h"
#endif /* __PROJECTS_CONFIG_H__ */

#ifdef CONFIG_HANDERA
	#include "Vga.h"
	#include "HndrFuncs.h"
#endif /* CONFIG_HANDERA */

// globals used only in this modules --------------------------------------------------------------
static UInt16			gMemoLookupTopVisibleIndex;														
static DmOpenRef	gMemoDB;

// exported globals -------------------------------------------------------------------------------
UInt16		gMemoLookupSelectedIndex;


// declarations -----------------------------------------------------------------------------------
static void 		MemoLookupGetSelectedRecordIndex( void ) THIRD_SECTION;
static void 		MemoLookupLoadTable( void ) THIRD_SECTION;
static void			MemoLookupDrawMemoRecordTitle( void *tableP, Int16 row, Int16 column, RectangleType * bounds ) THIRD_SECTION;
static void 		MemoLookupUpdateScrollbar( UInt16 numRows ) THIRD_SECTION;
static void 		MemoLookupHandleFindButtonEvent( EventType * eventP ) THIRD_SECTION;
#ifdef CONFIG_HANDERA
	static void			MemoLookupResize( FormType * frmP, Boolean draw ) THIRD_SECTION;
#endif /* CONFIG_HANDERA */


// exported functions -----------------------------------------------------------------------------

/*
 * FUNCTION:				MemoLookupHandleEvent
 * DESCRIPTION:			this is the event handler for the memo look up dialog
 * PARAMETERS:			eventP - the event
 * RETURNS:					true if the event was handled, otherwise false
 */
Boolean MemoLookupHandleEvent( EventType * eventP )
{
	switch( eventP->eType )
	{
		case keyDownEvent:
			if( eventP->data.keyDown.chr == vchrFind )
			{
				MemoLookupHandleFindButtonEvent( eventP );
				return (true); // IMPORTANT!!! see MemoLookupHandleFindButtonEvent
			}
			break;

		case sclRepeatEvent:
			gMemoLookupTopVisibleIndex += (eventP->data.sclRepeat.newValue - eventP->data.sclRepeat.value);
			MemoLookupLoadTable();
			TblRedrawTable( (TableType *)GetObjectPtr( MemoLookupTable ) );
			break;
			
		case ctlSelectEvent:
			// do not return true here or else we have to dismiss the dialog ourselves
			if( eventP->data.ctlSelect.controlID == MemoLookupAddButton )
				MemoLookupGetSelectedRecordIndex();
			else
				gMemoLookupSelectedIndex = noRecordIndex;
			break;

#ifdef CONFIG_HANDERA
		case displayExtentChangedEvent:
			if( gGlobalPrefs.vgaExtension )
			{
				MemoLookupResize( FrmGetActiveForm(), true );
				return (true);
			}
			break;
#endif /* CONFIG_HANDERA */

		default:
			break;
	}

	return (false);
}

/*
 * FUNCTION:				MemoLookupInitialize
 * DESCRIPTION:			this routine initializes the memo look up dialog
 * 									this routine also draws the form !!!
 * PARAMETERS:			frmP - pointer to the memo lookup dialog
 * 									memoDB - reference to the opened memo database
 * RETURNS:					nothing
 */
void MemoLookupInitialize( FormType * frmP, DmOpenRef memoDB )
{
	TableType * 		tableP;
	Char *					ttlP;
	MemHandle				h;
	Int16						numRows;
	Int16						row;
#ifdef CONFIG_HANDERA
	UInt16					lineHeight;
	UInt16					fitRows;
	FontID					fnt;
	RectangleType		r;
#endif /* CONFIG_HANDERA */

	ErrFatalDisplayIf( !memoDB, "invalid param" );
	gMemoDB = memoDB;

	// initialize the tableObject
	tableP = FrmGetObjectPtr( frmP, FrmGetObjectIndex( frmP, MemoLookupTable ) );
	numRows = TblGetNumberOfRows( tableP );

#ifdef CONFIG_HANDERA
	if( gGlobalPrefs.vgaExtension )
		fnt = FntSetFont( VgaBaseToVgaFont( stdFont ) );
	else
		fnt = FntSetFont( stdFont );
	lineHeight = FntLineHeight();
	FntSetFont( fnt );
#endif /* CONFIG_HANDERA */

	gMemoLookupTopVisibleIndex = 0;

	for( row = 0; row < numRows; row++ )
	{
#ifdef CONFIG_HANDERA
		TblSetRowHeight( tableP, row, lineHeight );
#endif /* CONFIG_HANDERA */
		TblSetItemStyle( tableP, row, 0, customTableItem );
	}
	TblSetColumnUsable( tableP, 0, true );
	TblSetCustomDrawProcedure( tableP, 0, MemoLookupDrawMemoRecordTitle );

#ifdef CONFIG_HANDERA
	if( gGlobalPrefs.vgaExtension )
		MemoLookupResize( frmP, false );
#endif /* CONFIG_HANDERA */

#ifdef CONFIG_HANDERA
	TblGetBounds( tableP, &r );
	fitRows = r.extent.y / lineHeight;
	if( numRows > fitRows )
		numRows = fitRows;
#endif /* CONFIG_HANDERA */

	MemoLookupUpdateScrollbar( numRows );
	MemoLookupLoadTable();

	// draw the title and the form
	ttlP = (Char *)FrmGetTitle( frmP );
	h = DmGetResource( strRsc, MemoLookupTitleString );
	ErrFatalDisplayIf( !h, "memo title - resource not found" );
	DrawFormAndRoundTitle( frmP, MemHandleLock( h ) );
	MemHandleUnlock( h );
	DmReleaseResource( h );
}

// functions used only in this module -------------------------------------------------------------

#ifdef CONFIG_HANDERA
/*
 * FUNCTION:				MemoLookupResize
 * DESCRIPTION:			this routine resizes the dialog to fit the screen
 * 									and repositions the controls on the form
 * PARAMETERS:			frmP - pointer to the form
 * 									draw - if true the from will be redrawn
 * RETURNS:					nothing
 */
static void MemoLookupResize( FormType * frmP, Boolean draw )
{
	RectangleType 	r;
	TableType *			tblP;
	Coord						x, y;
	Int16						x_diff, y_diff;
	UInt16					objI;
	UInt16					lineFit;
	FontID					fnt;

	ErrFatalDisplayIf( !gGlobalPrefs.vgaExtension, "vga extension not present" );

	// get the display extent
	WinGetDisplayExtent( &x, &y );

	// get the old form bounds
	FrmGetFormBounds( frmP, &r );

	// get the difference
	y_diff = y - r.extent.y;
	x_diff = x - r.extent.x;

	// set the forms new bounds
	r.extent.x = x;
	r.extent.y = y;
	WinSetWindowBounds( FrmGetWindowHandle( frmP ), &r );

	HndrMoveObject( frmP, FrmGetObjectIndex( frmP, MemoLookupAddButton ), x_diff, y_diff, draw );
	HndrMoveObject( frmP, FrmGetObjectIndex( frmP, MemoLookupCancelButton ), x_diff, y_diff, draw );

	// resize table
	tblP = FrmGetObjectPtr( frmP, FrmGetObjectIndex( frmP, MemoLookupTable ) );
	if( draw )
		TblEraseTable( tblP );

	TblGetBounds( tblP, &r );
	r.extent.x += x_diff;
	r.extent.y += y_diff;
	TblSetBounds( tblP, &r );
	fnt = FntSetFont( VgaBaseToVgaFont( stdFont ) );
	lineFit = r.extent.y / FntLineHeight();
	FntSetFont( fnt );

	objI = FrmGetObjectIndex( frmP, MemoLookupScrollBar );
	FrmGetObjectBounds( frmP, objI, &r );

	if( draw )
	{
		RctInsetRectangle( &r, -2 );
		WinEraseRectangle( &r, 0 );
		RctInsetRectangle( &r, 2 );
	}
	r.extent.y += y_diff;
	FrmSetObjectBounds( frmP, objI, &r );
	if( draw )
	{
		gMemoLookupTopVisibleIndex = 0;
		MemoLookupUpdateScrollbar( lineFit );
		MemoLookupLoadTable();
		TblRedrawTable( tblP );
		FrmDrawForm( frmP );
	}
}
#endif /* CONFIG_HANDERA */

/*
 * FUNCTION:				MemoLookupLoadTable
 * DESCRIPTION:			this routine loads the table with record indexes
 * PARAMETERS:			none
 * RETURNS:					nothing
 */
static void MemoLookupLoadTable( void )
{
	FormType * frmP;
	TableType * tableP;
	UInt16		recIndex;
	Int16			numRows;
	Int16			row;
#ifdef CONFIG_HANDERA
	RectangleType r;
	UInt16		fitRows;
	FontID		fnt;
#endif /* CONFIG_HANDERA */


	ErrFatalDisplayIf( !gMemoDB, "memo database is not open" );

	frmP = FrmGetActiveForm();
	tableP = FrmGetObjectPtr( frmP, FrmGetObjectIndex( frmP, MemoLookupTable ) );
	numRows = TblGetNumberOfRows( tableP );

#ifdef CONFIG_HANDERA
	if( gGlobalPrefs.vgaExtension )
		fnt = FntSetFont( VgaBaseToVgaFont( stdFont ) );
	else
		fnt = FntSetFont( stdFont );

	TblGetBounds( tableP, &r );
	fitRows = r.extent.y / FntLineHeight();
	FntSetFont( fnt );
	if( numRows < fitRows )
		fitRows = numRows;
#endif /* CONFIG_HANDERA */

	// we rely on the validity of the value of gMemoLookupTopVisible

	recIndex = gMemoLookupTopVisibleIndex;
	row = 0;
	do
	{
		if( !DmQueryNextInCategory( gMemoDB, &recIndex, dmAllCategories ) )
			break;

		TblSetRowUsable( tableP, row, true );
		TblSetRowID( tableP, row, recIndex );

		recIndex++;
		row++;
	}
#ifdef CONFIG_HANDERA
	while( row < fitRows );
#else
	while( true );
#endif /* CONFIG_HANDERA */

	while( row < numRows )
	{
		TblSetRowUsable( tableP, row, false );
		row++;
	}

	TblMarkTableInvalid( tableP );
}

/*
 * FUNCTION:					MemoLookupMemoRecordTitle
 * DESCRIPTION:				this routine draws one row in our table
 * PARAMETERS:				see palm sdk
 * RETURNS:						nothing
 */
static void	MemoLookupDrawMemoRecordTitle( void *tableP, Int16 row, Int16 column, RectangleType * bounds )
{
	Boolean		fitInWidth;
	Char			horzEllipsis;
	Int16			x;
	UInt16		width;
	UInt16		recIndex;
	UInt16		length;
	UInt16		recNumber;
	FontID		curFont;
	MemHandle	recH;
	Char *		eolP;
	Char *		recP;
	Char			szNumber[6];	// 99999. is the biggest string we support for the memonumber
	
	ErrFatalDisplayIf( !gMemoDB, "Memo database is not open" );

	recIndex = TblGetRowID( tableP, row );
	recH = DmQueryRecord( gMemoDB, recIndex );
	if( !recH )
		return;
	
	recNumber = DmPositionInCategory( gMemoDB, recIndex, dmAllCategories );
	if( !recNumber && DmGetLastErr() != errNone )
		return;
	recNumber++;

	recP = MemHandleLock( recH );

	StrIToA( szNumber, recNumber );
	eolP = StrChr( szNumber, '\0' );
	ErrFatalDisplayIf( !eolP, "no string terminator" );
	length = StrLen( szNumber );
	*eolP = '.';
	length += 1;

#ifdef CONFIG_HANDERA
	if(gGlobalPrefs.vgaExtension )
		curFont = FntSetFont( VgaBaseToVgaFont( stdFont ) );
	else
#endif /* CONFIG_HANDERA */
		curFont = FntSetFont( stdFont );
	x = bounds->topLeft.x + 1;
	if( length < 3 )
		x += FntCharWidth( '1' );
	WinDrawChars( szNumber, length, x, bounds->topLeft.y );
	x += FntCharsWidth( szNumber, length ) + 4;

	width = bounds->extent.x - x;

	eolP = StrChr( recP, linefeedChr );
	length = (eolP == NULL ? StrLen( recP ) : eolP - recP);

	FntCharsInWidth( recP, &width, &length, &fitInWidth );
	if( !fitInWidth )
	{
		ChrHorizEllipsis( &horzEllipsis );
		width -= FntCharWidth( horzEllipsis );
		FntCharsInWidth( recP, &width, &length, &fitInWidth );
	}

	WinDrawChars( recP, length, x + 1, bounds->topLeft.y );
	if( !fitInWidth )
		WinDrawChars( &horzEllipsis, 1, x + 1 + width, bounds->topLeft.y );

	FntSetFont( curFont );
	MemHandleUnlock( recH );
}

/*
 * FUNCTION:				MemoLookupGetSelectedRecordIndex
 * DESCRIPTION:			this routine sets the gMemoLookupSelectedIndex
 * 									variable
 * PARAMETERS:			none
 * RETURNS:					nothing
 * CALLED:					if the used tapped the "add" button
 */
static void MemoLookupGetSelectedRecordIndex( void )
{
	Int16 	row;
	Int16		column;
	TableType *tableP;

	tableP = (TableType *)GetObjectPtr( MemoLookupTable );

	if( TblGetSelection( tableP, &row, &column ) )
		gMemoLookupSelectedIndex = TblGetRowID( tableP, row );
	else
		gMemoLookupSelectedIndex = noRecordIndex;
}

/*
 * FUNCTION:				MemoLookupUpdateScrollbar
 * DESCRIPTION:			this routine sets the scrollbar's values
 * PARAMETERS:			numRows - the number of rows that are visible
 * RETURNS:					nothing
 */
static void MemoLookupUpdateScrollbar( UInt16 numRows )
{
	ScrollBarType * sclP;
	UInt16					numRecs;
	Int16						sclMax;

	ErrFatalDisplayIf( !gMemoDB, "memo db not open" );

	sclP = (ScrollBarType *)GetObjectPtr( MemoLookupScrollBar );
	numRecs = DmNumRecordsInCategory( gMemoDB, dmAllCategories );
	if( numRecs > numRows )
		sclMax = numRecs - numRows;
	else
		sclMax = 0;
	SclSetScrollBar( sclP, gMemoLookupTopVisibleIndex, 0, sclMax, numRows - 1 );
}

/*
 * FUNCTION:				MemoLookupHandleFindButtonEvent
 * DESCRIPTION:			this routine handles the situation when the find button was tapped
 * 									it dismissed this form and posts the passed event once again, thus it 
 * 									is essential not to let the system get the passed event
 * PARAMETERS:			eventP - the event
 * RETURNS:					nothing
 */
static void MemoLookupHandleFindButtonEvent( EventType * eventP )
{
	EventType evt;

	MemSet( &evt, sizeof( evt ), 0 );
	evt.eType = ctlSelectEvent;
	evt.data.ctlSelect.controlID = MemoLookupCancelButton;
	evt.data.ctlSelect.pControl = GetObjectPtr( MemoLookupCancelButton );
	EvtAddEventToQueue( &evt );
	EvtAddEventToQueue( eventP );
}
