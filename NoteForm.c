#include <PalmOS.h>
#include "ProjectsDB.h"
#include "Projects.h"
#include "ProjectsRcp.h"

#ifndef __PROJECTS_CONFIG_H__
	#error "include config.h"
#endif /* __PROJECTS_CONFIG_H__ */

#ifdef CONFIG_SONY
	#include <SonyCLIE.h>
#endif /* CONFIG_SONY */

#ifdef CONFIG_HANDERA
	#include "Vga.h"
	#include "HndrFuncs.h"
#endif /* CONFIG_HANDERA */

// declarations -----------------------------------------------------------------------------------
static void NoteFormDrawTitleAndForm( FormType * frmP ) THIRD_SECTION;
static void NoteFormInit( FormType * frmP ) THIRD_SECTION;
static void NoteFormUpdateScrollBar( void ) THIRD_SECTION;
static void NoteFormDeInit( void ) THIRD_SECTION;
static void	NoteFormSelectFont( void ) THIRD_SECTION;
static void	NoteFormScrollLines( Int16 numLines, Boolean updateScrollBar ) THIRD_SECTION;
static void	ScrollNotePage( WinDirectionType direction ) THIRD_SECTION;
static void NoteFormDeleteNote( void ) THIRD_SECTION;
static void NoteFormSelectWord( UInt16 startPos, UInt16 len ) THIRD_SECTION;
#ifdef CONFIG_HANDERA
	static void NoteFormResize( FormType * frmP, Boolean draw ) THIRD_SECTION;
#endif /* CONFIG_HANDERA */

// exported function ------------------------------------------------------------------------------
/*
 * FUNCTION:					NoteFormHandleEvent
 * DESCRIPTION:				this routine handles evennts occuring on the note form
 * PARAMETERS:				eventP - the event
 * RETURNS:						true if the handler handed the event completely, otherwise
 * 										false so the system will bother with the event too
 */
Boolean NoteFormHandleEvent( EventType * eventP )
{
	Boolean handled = false;
	FormType * frmP;

	switch( eventP->eType )
	{
		case frmOpenEvent:
			frmP = FrmGetActiveForm();
			NoteFormInit( frmP );
			NoteFormDrawTitleAndForm( frmP );
			NoteFormUpdateScrollBar();
			handled = true;
			break;

		/*
		 * we dont need to correspond to this message
		 * because we are editing in place
		case frmSaveEvent:
			NoteFormDeInit( false );
			handled = true;
			break;
		*/

		case frmGotoEvent:
			frmP = FrmGetActiveForm();
			NoteFormInit( frmP );
#ifdef CONFIG_OS_BELOW_35
			if( !gGlobalPrefs.rom35present )
			{
				NoteFormDrawTitleAndForm( frmP );
				NoteFormSelectWord( eventP->data.frmGoto.matchPos, eventP->data.frmGoto.matchLen );
			}
			else
			{
#endif
				NoteFormSelectWord( eventP->data.frmGoto.matchPos, eventP->data.frmGoto.matchLen );
				NoteFormDrawTitleAndForm( frmP );
#ifdef CONFIG_OS_BELOW_35
			}
#endif
			handled = true;
			break;
			
		case frmCloseEvent:
			NoteFormDeInit();
			break;

		case menuEvent:
			MenuEraseStatus(0);
			switch( eventP->data.menu.itemID )
			{
				/*
				 * we do not need to handle this
				 * menu items because we have assigned
				 * special values in our resource so
				 * the system will handle it if
				 * we do not. :)
				 * see (UIResources.h)
				case EditUndo:
				case EditCut:
				case EditCopy:
				case EditPaste:
				case EditSelectAll:
				case EditKeyboard:
				case EditGraffiti:
					handled = EditMenuEventHandler( eventP->data.menu.itemID );
					break;
				*/

				case OptionsFont:
					NoteFormSelectFont();
					handled = true;
					break;

				case OptionsPhoneLookup:
					PhoneNumberLookup( (FieldType *)GetObjectPtr( ToDoNoteField ) );
					handled = true;
					break;
			
				default:
					break;
			}
			break;

		case keyDownEvent:
			if( ChrIsHardKey( eventP->data.keyDown.chr ) )
			{
				if( !(eventP->data.keyDown.modifiers & poweredOnKeyMask) )
				{
					NoteFormDeInit();
					FrmReturnToForm(0);
					FrmUpdateForm( 0, (kUpdateWholeForm | kUpdateCurrentToDoItem) );
					handled = true;
				}
			}
			else if( eventP->data.keyDown.chr == pageUpChr )
			{
				ScrollNotePage( winUp );
				handled = true;
			}
			else if( eventP->data.keyDown.chr == pageDownChr )
			{
				ScrollNotePage( winDown );
				handled = true;
			}
#ifdef CONFIG_JOGDIAL
	#ifdef CONFIG_SONY
			else if( eventP->data.keyDown.chr == vchrJogBack )
	#else
		#ifndef CONFIG_HANDERA
			#error "please define CONFIG_HANDERA or CONFIG_SONY or remove the CONFIG_JOGDIAL definition!"
		#endif /* CONFIG_HANDERA */
			else if( eventP->data.keyDown.chr == chrEscape )
	#endif /* CONFIG_SONY */
			{
				NoteFormDeInit();
				FrmReturnToForm(0);
				FrmUpdateForm( 0, (kUpdateWholeForm | kUpdateCurrentToDoItem) );
				handled = true;
			}
#endif /* CONFIG_JOGDIAL */
			break;

		case fldChangedEvent:
			NoteFormUpdateScrollBar();
			handled = true;
			break;

		case sclRepeatEvent:
			NoteFormScrollLines( eventP->data.sclRepeat.newValue - eventP->data.sclRepeat.value, false );
			break;

		case ctlSelectEvent:
			if( eventP->data.ctlSelect.controlID == ToDoNoteDoneButton )
			{
				NoteFormDeInit();
				FrmReturnToForm(0);
				FrmUpdateForm( 0, (kUpdateWholeForm | kUpdateCurrentToDoItem) );
			}
			else
			{
				ErrFatalDisplayIf( eventP->data.ctlSelect.controlID != ToDoNoteDeleteButton, "unknown button id" );

				if( FrmAlert( DeleteNoteAlert ) == DeleteNoteOKButton )
				{
					NoteFormDeleteNote();
					NoteFormDeInit();
					FrmReturnToForm(0);
					FrmUpdateForm( 0, (kUpdateWholeForm | kUpdateCurrentToDoItem) );
				}
			}
			handled = true;
			break;

#ifdef CONFIG_HANDERA
		case displayExtentChangedEvent:
			if( gGlobalPrefs.vgaExtension )
			{
				NoteFormResize( FrmGetActiveForm(), true );
				handled = true;
			}
			break;
#endif /* CONFIG_HANDERA */

		default:
			break;
	}

	return (handled);
}

// implementatoin of functions used only in this module -------------------------------------------

#ifdef CONFIG_HANDERA
/*
 * FUNCTION:				NoteFormResize
 * DESCRIPTION:			this routine resizes the form to fit the screen 
 * 									and redraws it if draw == true
 * PARAMETERS:			frmP - pointer to the form
 * 									draw - if true redraw the form
 * RETURNS:					nothing
 */
static void NoteFormResize( FormType * frmP, Boolean draw ) 
{
	RectangleType 	r;
	FieldType *			fldP;
	Coord						x, y;
	Int16						x_diff, y_diff;
	UInt16					i, numObjs;

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

	// move the done & delete button
	HndrMoveObject( frmP, FrmGetObjectIndex( frmP, ToDoNoteDoneButton ), x_diff, y_diff, draw && gCurrentProject.currentPage == ToDoPage );
	HndrMoveObject( frmP, FrmGetObjectIndex( frmP, ToDoNoteDeleteButton ), x_diff, y_diff, draw && gCurrentProject.currentPage == ToDoPage );

	// reposition the graffiti shift indicator
	numObjs = FrmGetNumberOfObjects( frmP );
	for( i = 0; i < numObjs; i++ )
		if( FrmGetObjectType( frmP, i ) == frmGraffitiStateObj )
		{
			HndrMoveObject( frmP, i, x_diff, y_diff, draw );
			break;
		}

	// resize the field
	fldP = (FieldType *)GetObjectPtr( ToDoNoteField );
	if( draw )
		FldEraseField( fldP );
	FldGetBounds( fldP, &r );
	r.extent.x += x_diff;
	r.extent.y += y_diff;
	FldSetBounds( fldP, &r );
	if( draw )
		FldDrawField( fldP );

	// resize the scrollbar
	i = FrmGetObjectIndex( frmP, ToDoNoteScrollBar );
	FrmGetObjectBounds( frmP, i, &r );
	if( draw )
	{
		RctInsetRectangle( &r, -2 );
		WinEraseRectangle( &r, 0 );
		RctInsetRectangle( &r, 2 );
	}
	r.extent.y += y_diff;
	FrmSetObjectBounds( frmP, i, &r );
	if( draw )
	{
		NoteFormDrawTitleAndForm( frmP );
		NoteFormUpdateScrollBar();
	}
}
#endif /* CONFIG_HANDERA */

/*
 * FUNCTION:				NoteFormSelectWord
 * DESCRIPTION:			this routine selects the string specified by the parameters
 * PARAMETERS:			startPos - position within the string displayed on this form
 * 									len - length of the selection
 * RETURNS:					nothing
 */
static void NoteFormSelectWord( UInt16 startPos, UInt16 len )
{
	FieldType * fieldP;

	fieldP = (FieldType *)GetObjectPtr( ToDoNoteField );

	FldSetScrollPosition( fieldP, startPos );
	FldSetSelection( fieldP, startPos, startPos + len );
	NoteFormUpdateScrollBar();
}

/*
 * FUNCTION:				NoteFormInit
 * DESCRIPTON:			this routine initializes the form which means that 
 * 									it is mainly responsible for connecting the field with
 * 									the records text.
 * PARAMETERS:			frmP - pointer to the note form
 * RETURNS:					nothing
 */
static void NoteFormInit( FormType * frmP )
{
	PrjtToDoType *	recP;
	Char 		 *			noteP;
	FieldType * 		fieldP;
	UInt16					descLen;
	UInt16					offset;
	UInt16					fldIndex;
	MemHandle 			recH;

#ifdef CONFIG_HANDERA
	if( gGlobalPrefs.vgaExtension )
	{
		VgaFormModify( frmP, vgaFormModify160To240 );
		NoteFormResize( frmP, false );
	}
#endif /* CONFIG_HANDERA */

	recH = DmQueryRecord( gCurrentProject.todoDB, gCurrentProject.curToDoIndex );
	ErrFatalDisplayIf( !recH, "bad record" );
	recP = MemHandleLock( recH );

	// compute the offset within the record of the note string
	noteP = (&recP->description + StrLen( &recP->description ) + 1);
	offset = (noteP - (Char *)recP);
	descLen = StrLen( noteP );
	MemHandleUnlock( recH );

	// connect the noteFormP's field width the current record
	fldIndex = FrmGetObjectIndex( frmP, ToDoNoteField );
	fieldP = FrmGetObjectPtr( frmP, fldIndex );
	FldSetText( fieldP, recH, offset, descLen+1 );
	FldSetFont( fieldP, gGlobalPrefs.noteFont );
	FrmSetFocus( frmP, fldIndex );
}

/*
 * FUNCTION:				NoteFormDrawTitleAndForm
 * DESCRIPTION:			this routine draws the form and its title
 * PARAMETERS:			frmP - pointer to the note form
 * RETURNS:					nothing
 */
static void NoteFormDrawTitleAndForm( FormType * frmP )
{
	PrjtToDoType *recP;
	MemHandle			recH;

	recH = DmQueryRecord( gCurrentProject.todoDB, gCurrentProject.curToDoIndex );
	ErrFatalDisplayIf( !recH, "bad record" );
	recP = MemHandleLock( recH );
	DrawFormAndRoundTitle( frmP, &recP->description );
	MemHandleUnlock( recH );
}

/*
 * FUNCTION:			NoteFormUpdateScrollBar
 * DESCRIPTION:		this routine updates the scrollbar on the note form
 * PARAMETERS:		none
 * RETURNS:				nothing
 */
static void NoteFormUpdateScrollBar( void )
{
	Int16				maxVal;
	Int16				curPos;
	Int16				txtHeight;
	Int16				fieldHeight;

	FieldType * fieldP;
	ScrollBarType * barP;

	fieldP = (FieldType *)GetObjectPtr( ToDoNoteField );
	barP = (ScrollBarType *)GetObjectPtr( ToDoNoteScrollBar );

	FldGetScrollValues( fieldP, &curPos, &txtHeight, &fieldHeight );

	if( txtHeight > fieldHeight )
		maxVal = txtHeight - fieldHeight;
	else
		maxVal = 0;
	
	SclSetScrollBar( barP, curPos, 0, maxVal, fieldHeight - 1 );
}

/*
 * FUNCTION:				NoteFormDeInit
 * DESCRIPTION:			this routine cleans up the form which mean 
 * 									that it disconnects the field from the edited record
 * 									if the field was edited it sets the dirty attribute of the record
 * PARAMETERS:			none
 * RETURNS:					nothing
 */
static void NoteFormDeInit( void )
{
	UInt16			recAttr;
	FieldType * fieldP = (FieldType *)GetObjectPtr( ToDoNoteField );

	// dirty the record if necessary
	if( FldDirty( fieldP ) )
	{
		FldCompactText( fieldP );
		DmRecordInfo( gCurrentProject.todoDB, gCurrentProject.curToDoIndex, &recAttr, NULL, NULL );
		recAttr |= dmRecAttrDirty;
		DmSetRecordInfo( gCurrentProject.todoDB, gCurrentProject.curToDoIndex, &recAttr, NULL );
	}
	FldSetTextHandle( fieldP, NULL );
}

/*
 * FUNCTION:			NoteFormSelectFont
 * DESCRIPTION:		this routine lets the user select a font
 * 								and applies changes to the note form field
 * PARAMETERS:		none
 * RETURNS:				nothing
 */
static void NoteFormSelectFont( void )
{
	FieldType * fieldP;
	FontID 			newFont;

	newFont = FontSelect( gGlobalPrefs.noteFont );
	if( newFont != gGlobalPrefs.noteFont )
	{
		gGlobalPrefs.noteFont = newFont;
		fieldP = (FieldType *)GetObjectPtr( ToDoNoteField );
		FldSetFont( fieldP, gGlobalPrefs.noteFont );
		NoteFormUpdateScrollBar();
	}
}

/*
 * FUNCTION:				NoteformScrollLines
 * DESCRIPTION:			scrolls the text on the note form by a number of lines
 * PARAMETERS:			numLines - number of lines to scroll
 * 									updatedScrollBar - if draw the scrollbar will be updated
 * RETURNS:					nothing
 */
static void NoteFormScrollLines( Int16 numLines, Boolean updateScrollBar )
{
	FieldType * fieldP = (FieldType *)GetObjectPtr( ToDoNoteField );

	if( numLines < 0 )
		FldScrollField( fieldP, -numLines, winUp );
	else
		FldScrollField( fieldP, numLines, winDown );

	if( updateScrollBar || (FldGetNumberOfBlankLines( fieldP ) && numLines < 0) )
		NoteFormUpdateScrollBar();
}

/*
 * FUNCTION:				ScrollNotePage
 * DESCRIPTION:			this routine scrolls the test by the height of the field
 * PARAMETERS:			direction - winUp or winDown
 * RETURNS:					nothing
 */
static void ScrollNotePage( WinDirectionType direction )
{
	FieldPtr fieldP;
	
	fieldP = (FieldType *)GetObjectPtr( ToDoNoteField );
	if( FldScrollable( fieldP, direction ) )
	{
		int linesToScroll = FldGetVisibleLines( fieldP ) - 1;
		if( direction == winUp )
			linesToScroll = -linesToScroll;
		NoteFormScrollLines( linesToScroll, true );
	}
}

/*
 * FUNCTION:			NoteFormDeleteNote
 * DESCRIPTION:		this routine deletes the text on the note form
 * PARAMETERS:		none
 * RETURNS:				nothing
 */
static void NoteFormDeleteNote( void )
{
	UInt16			newSize;
	MemHandle		recH;
	PrjtToDoType	*	recP;
	FieldType * fieldP;


	// disconnect the field from the record
	fieldP = (FieldType *)GetObjectPtr( ToDoNoteField );
	FldSetTextHandle( fieldP, NULL );

	// while using DmGetRecord we ensure the record is set dirty
	recH = DmGetRecord( gCurrentProject.todoDB, gCurrentProject.curToDoIndex );
	ErrFatalDisplayIf( !recH, "bad record" );
	recP = MemHandleLock( recH );
	newSize = sizeof( PrjtToDoType ) + StrLen( &recP->description ) + 1;
	MemHandleUnlock( recH );
	DmReleaseRecord( gCurrentProject.todoDB, gCurrentProject.curToDoIndex, true );

	recH = DmResizeRecord( gCurrentProject.todoDB, gCurrentProject.curToDoIndex, newSize );
	if( recH )
	{
		DmSet(MemHandleLock( recH ), newSize - 1, 1, 0 );		// write a null terminator for the note
		MemHandleUnlock( recH );
	}
}
