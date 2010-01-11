/* FOR BEST FORMATING OF THIS FILE SET TABSTOP TO 2 */

#include <PalmOS.h>
#include "ProjectsDB.h"
#include "Projects.h"
#include "ProjectsRcp.h"

#ifndef __PROJECTS_CONFIG_H__
	#error "include config.h"
#endif /* __PROJECTS_CONFIG_H__ */

#ifdef CONFIG_ALLTODOS_DLG

#ifdef CONFIG_SONY
	#include <SonyCLIE.h>
#endif /* CONFIG_SONY */

#ifdef CONFIG_HANDERA
	#include "Vga.h"
	#include "HndrFuncs.h"
#endif /* CONFIG_HANDERA */

#define PRJT_DB_CATEGORIES

// definitions ------------------------------------------------------------------------------------

typedef struct
{
	UInt16		index;					// if type is 'project name' it is the index in the main db
														// if type is 'todo item' it is the index in the todo db
	UInt16		item_type;			// projects name or todo item
	UInt16		gdb_index;				// index in our gDBs array
} __TableItemType;

typedef struct
{
	DmOpenRef		db;
	UInt16			db_category;			// currently selected category for this database
	UInt16			maindb_index;			// index of the record in main db
	UInt16			category_width;		// width of the drawn category string in pixels
  UInt16      name_width;       // width of the drawn name of the db
	UInt16			org_category;
} __DatabaseItemType;

// our column(s) in the table
#define kToDosDescColumn					0

#ifdef CONFIG_ADDITIONAL
	// our delay for the cross out
	#define kCrossOutDelayTicks					50
	#define kCrossOutDelayAnimLoops			1024
#endif

// our visual columns
#define kDBNameOffset							1
#define kToDoCompletionOffset			1
#define kToDoPriorityOffset				15
#define kToDoDescriptionOffset		23

#define kToDoCompletionWidth			14
#define kToDoPriorityWidth				8
#define kToDoDueDateWidth					25
#define kToDoCategoryWidth				25

#ifdef CONFIG_HANDERA
	#define kPopupTriangleBigWidth		20
	#define kPopupTriangleWidth				13
	#define kPopupTriangleHeight			15
#else
	#define kPopupTriangleWidth				13
#endif /* CONFIG_HANDERA */


// values for the __TableItemType 'type' member
#define kProjectNameItemType			0x00
#define kProjectToDoItemType			0x01

// values for scrolling the table
#define kScrollByOneLineUpwards		0x00
#define kScrollByOneLineDownwards	0x01
#define kScrollByOnePageUpwards		0x02
#define kScrollByOnePageDownwards	0x03

// values for the goto next/prev project
#define kGotoNextProject					0x00
#define kGotoPrevProject					0x01

// declarations -----------------------------------------------------------------------------------

static void 		AllToDosDeInit( void ) SECOND_SECTION;
static void 		AllToDosInit( FormType * frmP ) SECOND_SECTION;
static void 		AllToDosDrawTableItem( void * tblP, Int16 row, Int16 col, RectangleType * bounds ) SECOND_SECTION;
static void 		AllToDosLoadTable( UInt16 startIndex ) SECOND_SECTION;
static void 		AllToDosHandleScrolling( UInt16 scrolltype ) SECOND_SECTION;
static void 		AllToDosUpdateScrollers( UInt16 lastIndex ) SECOND_SECTION;
static void 		AllToDosGotoNextProject( UInt16 nextprev ) SECOND_SECTION;
static UInt16		AllToDosGetDBIndex( UInt16 maindb_index ) SECOND_SECTION;
static Boolean	AllToDosSeekToDoRecord( DmOpenRef db, UInt16 * index, UInt16 offset, Int16 direction, UInt16 category ) SECOND_SECTION;
static Boolean 	AllToDosSeekPrevTableItem( __TableItemType * inItem, __TableItemType * outItem ) SECOND_SECTION;
static UInt16		AllToDosOpenAndInsertDB( UInt16 maindb_index ) SECOND_SECTION;
static void			AllToDosHandlePreferences( void ) SECOND_SECTION;
static void 		AllToDosToggleToDoCompleted( UInt16	table_row ) SECOND_SECTION;
static void			AllToDosSelectFont( void ) SECOND_SECTION;
static void			AllToDosGotoToDoItem( __TableItemType * item ) SECOND_SECTION;
static void 		AllToDosHandleTapIntoDBNameRow( TableType * tblP, Int16 row, Int16 x, Int16 y ) SECOND_SECTION;
static Boolean 	AllToDosSelectDBCategory( __DatabaseItemType * dbit, RectangleType * r ) SECOND_SECTION;
#ifdef CONFIG_ADDITIONAL
	static void 		AllToDosCrossOutItem( TableType * tblP, UInt16 table_row ) SECOND_SECTION;
#endif
#ifdef CONFIG_HANDERA
	static void AllToDosResizeForm( FormType * frmP, Boolean draw ) SECOND_SECTION;
#endif /* CONFIG_HANDERA */
#ifdef CONFIG_JOGDIAL
	static void AllToDosSelectItem( SelectItemDirection direction ) SECOND_SECTION;
#endif /* CONFIG_JOGDIAL */

// globals ----------------------------------------------------------------------------------------

// although we you save this array in the tables 'ptr' member
// it makes life easier when scrolling to have an continueing memory
static __TableItemType * gTableItems = NULL;

// we hold once opened databases open. this has the advantage that we
// do not need to reopen the databases each time we need access to them
static __DatabaseItemType * gDBs = NULL;
#if ERROR_CHECK_LEVEL != ERROR_CHECK_NONE
static UInt8								gDBsNum;
#endif
static UInt8								gDBsNextFreeIndex;
static UInt8								gValidRows;					// holds the number of rows that fit into our table
																								// gets initialized in AllToDosInit, AllToDosSelectFont and
																								// AllToDosResizeForm


// implementation ---------------------------------------------------------------------------------

#ifdef CONFIG_JOGDIAL
/*
 * FUNCTION:				AllToDosSelectItem
 * DESCRIPTION:			this function updates the gRowSelected variable
 * 									and updates the view
 * PARAMETERS:			direction - selectNext or selectPrev
 * RETURNS:					nothing
 */
static void AllToDosSelectItem( SelectItemDirection direction )
{
	TableType * tblP;
	UInt16			lastRow;

	tblP = (TableType *)GetObjectPtr( AllToDosTable );

	if( direction == selectNext )
	{
		if( gRowSelected == noRecordIndex )
			gRowSelected = 0;
		else
		{
			lastRow = TblGetLastUsableRow( tblP );
			if( gRowSelected == lastRow )
			{
				if( CtlEnabled( GetObjectPtr( AllToDosScrollDownRepeating ) ) )
					AllToDosHandleScrolling( kScrollByOneLineDownwards );
				else
					return;
			}
			else
			{
				TblMarkRowInvalid( tblP, gRowSelected );
				gRowSelected++;
			}
		}
	}
	else // if( direction == selectPrev )
	{
		if( gRowSelected == noRowSelected )
			gRowSelected = TblGetLastUsableRow( tblP );
		else if( !gRowSelected )
		{
			if( CtlEnabled( GetObjectPtr( AllToDosScrollUpRepeating ) ) )
				AllToDosHandleScrolling( kScrollByOneLineUpwards );
			else
				return;
		}
		else
		{
			TblMarkRowInvalid( tblP, gRowSelected );
			gRowSelected--;
		}
	}

	TblMarkRowInvalid( tblP, gRowSelected );
	TblRedrawTable( tblP );
}
#endif /* CONFIG_JOGDIAL */

/*
 * FUNCTION:					AllToDosSelectDBCategory
 * DESCRIPTION:				this routine lets the user select a category
 * PARAMETERS:				dbit - pointer to an entry in out gDBs array
 *										r - area to act as a button (bounds of the imaginary popup trigger)
 * RETURNS:						true if a new category has been selected
 */
static Boolean AllToDosSelectDBCategory( __DatabaseItemType * dbit, RectangleType * r )
{
	ListType * 	listP;
	Char *			catP;
	UInt16			newSel, curSel;
	Boolean			ret_val;
	
	ErrFatalDisplayIf( !r, "invalid param" );
	ErrFatalDisplayIf( !dbit, "invalid param" );
	ErrFatalDisplayIf( !dbit->db, "database not open" );

	listP = (ListType *)GetObjectPtr( AllToDosCategoryList );
	ErrFatalDisplayIf( !listP, "list not in resources" );
	
	CategoryCreateList( dbit->db, listP, dbit->db_category, true, true, 1, 0, true );
	curSel = LstGetSelection( listP );
	LstSetPosition( listP, (r->topLeft.x + r->extent.x) - 72, r->topLeft.y );
	newSel = LstPopupList( listP );
	ret_val = false;
	if( newSel != -1 && newSel != curSel )
	{
		catP = LstGetSelectionText( listP, newSel );
		newSel = CategoryFind( dbit->db, catP );
		dbit->db_category = newSel;
		ret_val = true;
	}
	CategoryFreeList( dbit->db, listP, true, 0 );

	return (ret_val);
}

/*
 * FUNCTION:				AllToDosHandleTapIntoDBNameRow
 * DESCRIPTION:			this routine handles the tblEnterEvent on a row containing a database name
 * PARAMETERS:			tblP - pointer to our table
 *									row - the row the event occured
 *									x, y - coord's of the event
 * RETURNS:					nothing
 */
static void AllToDosHandleTapIntoDBNameRow( TableType * tblP, Int16 row, Int16 x, Int16 y )
{
	RectangleType r;
	UInt16				gdb_index;
	Boolean				pen_down;
	Boolean				is_in, was_in;
  Boolean       projectname;

	gdb_index = gTableItems[row].gdb_index;
  TblGetItemBounds( tblP, row, kToDosDescColumn, &r );
  if( gDBs[gdb_index].db && x >= (r.topLeft.x + gDBs[gdb_index].name_width+1) )
  {
		r.topLeft.x += r.extent.x;
		r.extent.x = (gDBs[gdb_index].category_width+2);
		r.topLeft.x -= r.extent.x;
    projectname = false;
  }
  else
  {
    r.extent.x = (gDBs[gdb_index].name_width+1);
    projectname = true;
  }

	if( gDBs[gdb_index].db )
	{
		was_in = false;
		do
		{
			EvtGetPen( &x, &y, &pen_down );
			if( pen_down )
			{
				is_in = RctPtInRectangle( x, y, &r );
				if( is_in != was_in )
				{
					WinInvertRectangle( &r, 0 );
					was_in = is_in;
				}
			}
			else if( was_in )
			{
				WinInvertRectangle( &r, 0 );
				break;
			}
			else
				return;
		}
		while( pen_down );

    if( projectname )
    {
      gCurrentProject.projIndex = gDBs[gdb_index].maindb_index;
      gCurrentProject.currentPage = ToDoPage;
      FrmGotoForm( ProjectForm );
    }
    else if( AllToDosSelectDBCategory( &gDBs[gdb_index], &r ) )
		{
			AllToDosLoadTable( row );
#ifdef CONFIG_JOGDIAL
			gRowSelected = noRowSelected;
#endif /* CONFIG_JOGDIAL */
			TblRedrawTable( tblP );
		}
	}
}

#ifdef CONFIG_HANDERA
static void AllToDosResizeForm( FormType * frmP, Boolean draw )
{
	RectangleType r;
	TableType *		tblP;
	Coord					x, y;
	Int16					x_diff, y_diff;
	UInt16				lineHeight;
	FontID				fnt;

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

	// position the controls
	HndrMoveObject( frmP, FrmGetObjectIndex( frmP, AllToDosDoneButton ), x_diff, y_diff, draw );
	HndrMoveObject( frmP, FrmGetObjectIndex( frmP, AllToDosShowButton ), x_diff, y_diff, draw );
	HndrMoveObject( frmP, FrmGetObjectIndex( frmP, AllToDosScrollUpRepeating ), x_diff, y_diff, draw );
	HndrMoveObject( frmP, FrmGetObjectIndex( frmP, AllToDosScrollDownRepeating ), x_diff, y_diff, draw );
	HndrMoveObject( frmP, FrmGetObjectIndex( frmP, AllToDosScrollPrevProjectButton ), x_diff, y_diff, draw );
	HndrMoveObject( frmP, FrmGetObjectIndex( frmP, AllToDosScrollNextProjectButton ), x_diff, y_diff, draw );

	// resize table
	tblP = FrmGetObjectPtr( frmP, FrmGetObjectIndex( frmP, AllToDosTable ) );
	if( draw )
		TblEraseTable( tblP );

	TblGetBounds( tblP, &r );
	r.extent.x = x;
	r.extent.y += y_diff;
	TblSetBounds( tblP, &r );

	fnt = FntSetFont( gGlobalPrefs.allToDosFont );
	lineHeight = FntLineHeight();
	FntSetFont( fnt );
	gValidRows = r.extent.y / lineHeight;

	if( draw )
	{
		AllToDosLoadTable( 0 );
		TblDrawTable( tblP );

		FrmDrawForm( frmP );
	}
}
#endif /* CONFIG_HANDERA */

/*
 * FUNCTION:				AllToDosSelectFont
 * DESCRIPTION:			this routine lets the user select a form
 * 									and updates the view
 * PARAMETERS:			none
 * RETURNS:					nothing
 */
static void	AllToDosSelectFont( void )
{
	RectangleType r;
	TableType *	tblP;
	FontID			font;
	UInt16			lineHeight;
	UInt16			i;
	UInt16			rows;

	font = FontSelect( gGlobalPrefs.allToDosFont );
	if( font != gGlobalPrefs.allToDosFont )
	{
		gGlobalPrefs.allToDosFont = font;
		font = FntSetFont( font );
		lineHeight = FntLineHeight();
		FntSetFont( font );

		tblP = (TableType *)GetObjectPtr( AllToDosTable );
		rows = TblGetNumberOfRows( tblP );

		for( i = 0; i < rows; i++ )
		{
			TblSetRowHeight( tblP, i, lineHeight );
			TblSetItemFont( tblP, i, kToDosDescColumn, gGlobalPrefs.allToDosFont );
		}

		TblGetBounds( tblP, &r );
		gValidRows = r.extent.y / lineHeight;
		if( rows < gValidRows )
			gValidRows = rows;

		AllToDosLoadTable( 0 );
#ifdef CONFIG_JOGDIAL
		gRowSelected = noRowSelected;
#endif /* CONFIG_JOGDIAL */
		TblRedrawTable( tblP );
	}
}

/*
 * FUNCTION:				AllToDosGotoToDoItem
 * DESCRIPTION:			this routine closes the current form and opens the ProjectForm
 * 									and directs the form to go to the specified item
 * PARAMETERS:			item - pointer to the item we should go to
 * RETURNS:					nothing
 */
void AllToDosGotoToDoItem( __TableItemType * item )
{
	EventType event;
	FormType *	frmP;
	UInt16	todo_index;

	ErrFatalDisplayIf( !item, "invalid param" );
	ErrFatalDisplayIf( item->item_type != kProjectToDoItemType, "invalid item" );

	todo_index = item->index;
	gCurrentProject.projIndex = gDBs[item->gdb_index].maindb_index;

	MemSet( &event, sizeof( EventType ), 0 );

	// close the current form
	frmP = FrmGetActiveForm();
	AllToDosDeInit();
	FrmEraseForm( frmP );
	FrmDeleteForm( frmP );

	// now load the ProjectsForm
	event.eType = frmLoadEvent;
	event.data.frmLoad.formID = ProjectForm;
	EvtAddEventToQueue( &event );

	event.eType = frmGotoEvent;
	event.data.frmGoto.recordNum = todo_index;
	event.data.frmGoto.matchCustom = kGotoToDoItem;
	event.data.frmGoto.matchPos = 0;
	event.data.frmGoto.matchLen = 0;
	event.data.frmGoto.matchFieldNum = 0;
	EvtAddEventToQueue( &event );
}

#ifdef CONFIG_ADDITIONAL
/*
 * FUNCTION:				AllToDosCrossOutItem
 * DESCRIPTION:			if a todo item was toggled complete and we do not show complete items
 * 									this function is called to show a small animation striking the item out
 * PARAMETERS:			tblP - pointer to the table
 * 									row - row index in our table to be crossed out
 * RETURNS:					nothing
 */
void AllToDosCrossOutItem( TableType * tblP, UInt16 row )
{
	RectangleType  	bounds;
	UInt32					ticks;
	PrjtToDoType * 	todoP;
	Char *					strP;
	MemHandle				todoH;
	UInt16					x, y;
	UInt16					width;
	FontID					prvFnt;

	ErrFatalDisplayIf( !tblP, "invalid param" );

	ticks = TimGetTicks();

	TblGetItemBounds( tblP, row, kToDosDescColumn, &bounds );
	y = bounds.topLeft.y + bounds.extent.y/2;

	x = (gGlobalPrefs.showToDoPrio) ? (kToDoPriorityOffset+kToDoPriorityWidth+2) : (kToDoCompletionOffset+kToDoCompletionWidth+2);

	todoH = DmQueryRecord( gDBs[gTableItems[row].gdb_index].db, gTableItems[row].index );
	ErrFatalDisplayIf( !todoH, "bad record" );
	todoP = MemHandleLock( todoH );
	strP = &todoP->description;
	prvFnt = FntSetFont( gGlobalPrefs.allToDosFont );
	width = FntCharsWidth( strP, StrLen( strP ) );
	MemHandleUnlock( todoH );

	bounds.extent.x -= (kToDoCompletionWidth+1);
	if( gGlobalPrefs.showToDoDueDate )
		bounds.extent.x -= (kToDoDueDateWidth);
	if( gGlobalPrefs.showToDoPrio )
		bounds.extent.x -= (kToDoPriorityWidth+1);
	if( width < bounds.extent.x )
		bounds.extent.x = (width+1);
	bounds.extent.x += x;

	while( x < bounds.extent.x )
	{
		WinDrawLine( x, y, x+1, y );
		x++;

		// some delay ...
		// waiting for one tick is much to long so we loop simply n times
		// this may result however in different times on different devices
		asm( "move.l %%d0, -(%%sp)" : : );
		asm( "move.l %0, %%d0" : : "g" (kCrossOutDelayAnimLoops) );
		asm( "
AnimLoop: nop
					dbra %%d0, AnimLoop" :: );
		asm( "move.l (%%sp)+, %%d0" : : );
	}

	while( (TimGetTicks() - ticks) < kCrossOutDelayTicks )
		;
}
#endif

/*
 * FUNCTION:				AllToDosToggleToDoCompleted
 * DESCRIPTION:			this routine toggles the items checkbox
 * 									and performs a little animation if the item
 * 									should disappear from view. NOTE that the item
 * 									specified by the row parameter must be of type
 * 									kProjectToDoItemType!!!
 * PARAMETERS:			row - row index in the table of the item
 * RETURNS:					nothing
 */
void AllToDosToggleToDoCompleted( UInt16	table_row )
{
	__TableItemType * tiP;
	TableType *				tblP;
	Boolean						complete;

	tiP = &gTableItems[table_row];
	ErrFatalDisplayIf( tiP->item_type != kProjectToDoItemType, "invalid item type" );
	ErrFatalDisplayIf( !gMainDatabase.db, "main db not open" );
	ErrFatalDisplayIf( !gDBs[tiP->gdb_index].db, "prjt db not open" );

	complete = PrjtDBToDoToggleCompletionFlag(
							gMainDatabase.db, gDBs[tiP->gdb_index].maindb_index,
							gDBs[tiP->gdb_index].db, tiP->index, gGlobalPrefs.completeToDoDueDate );

	tblP = (TableType *)GetObjectPtr( AllToDosTable );

	if( complete && !gGlobalPrefs.showCompleteToDos )
	{
#ifdef CONFIG_ADDITIONAL
		AllToDosCrossOutItem( tblP, table_row );
#endif
		AllToDosLoadTable( table_row );
	}
	else
		TblMarkRowInvalid( tblP, table_row );

	TblRedrawTable( tblP );
}

/*
 * FUNCTION:				AllToDosHandlePreferences
 * DESCRIPTION:			this routine pops up the todo preferences dialog and lets
 * 									the user customize the view. note that the sort order controls
 * 									from the dialog are temprorily hidden as we operate on more than
 * 									one database
 * PARAMETERS:			none
 * RETURNS:					nothing
 */
void AllToDosHandlePreferences( void )
{
	TableType *	tblP;
	FormType * 	prefP;
	UInt8				sComp, sOnlyDue, rCompDate, sDueDate, sPrior, sCat;
	Boolean			redraw = false;
	Boolean			reload = false;

	prefP = FrmInitForm( ToDoPrefsDialog );

	sComp = FrmGetObjectIndex( prefP, ToDoPrefsCompletedItemsCheckbox );
	sOnlyDue = FrmGetObjectIndex( prefP, ToDoPrefsOnlyDueItemsCheckbox );
	rCompDate = FrmGetObjectIndex( prefP, ToDoPrefsRecCompletionDateCheckbox );
	sDueDate = FrmGetObjectIndex( prefP, ToDoPrefsShowDueDateCheckbox );
	sPrior = FrmGetObjectIndex( prefP, ToDoPrefsShowPrioritiesCheckbox );
	sCat = FrmGetObjectIndex( prefP, ToDoPrefsShowCategoriesCheckbox );

	FrmSetControlValue( prefP, sComp, gGlobalPrefs.showCompleteToDos );
	FrmSetControlValue( prefP, sOnlyDue, gGlobalPrefs.showOnlyDueDateToDos );
	FrmSetControlValue( prefP, rCompDate, gGlobalPrefs.completeToDoDueDate );
	FrmSetControlValue( prefP, sDueDate, gGlobalPrefs.showToDoDueDate );
	FrmSetControlValue( prefP, sPrior, gGlobalPrefs.showToDoPrio );
	FrmSetControlValue( prefP, sCat, gGlobalPrefs.showToDoCategories );

	FrmHideObject( prefP, FrmGetObjectIndex( prefP, ToDoPrefsSortLabel ) );
	FrmHideObject( prefP, FrmGetObjectIndex( prefP, ToDoPrefsSortPopTrigger ) );
	FrmShowObject( prefP, FrmGetObjectIndex( prefP, ToDoPrefsProjectsToDosLabel ) );

#ifdef CONFIG_HANDERA
	if( gGlobalPrefs.vgaExtension )
		VgaFormModify( prefP, rotateModeNone );
#endif /* CONFIG_HANDERA */

	if( FrmDoDialog( prefP ) != ToDoPrefsOKButton )
	{
		FrmDeleteForm( prefP );
		return;
	}

	if( FrmGetControlValue( prefP, sComp ) != gGlobalPrefs.showCompleteToDos )
	{
		reload = redraw = true;
		gGlobalPrefs.showCompleteToDos = !gGlobalPrefs.showCompleteToDos;
	}
	if( FrmGetControlValue( prefP, sOnlyDue ) != gGlobalPrefs.showOnlyDueDateToDos )
	{
		reload = redraw = true;
		gGlobalPrefs.showOnlyDueDateToDos = !gGlobalPrefs.showOnlyDueDateToDos;
	}
	if( FrmGetControlValue( prefP, rCompDate ) != gGlobalPrefs.completeToDoDueDate )
		gGlobalPrefs.completeToDoDueDate = !gGlobalPrefs.completeToDoDueDate;
	if( FrmGetControlValue( prefP, sDueDate ) != gGlobalPrefs.showToDoDueDate )
	{
		redraw = true;
		gGlobalPrefs.showToDoDueDate = !gGlobalPrefs.showToDoDueDate;
	}
	if( FrmGetControlValue( prefP, sPrior ) != gGlobalPrefs.showToDoPrio )
	{
		redraw = true;
		gGlobalPrefs.showToDoPrio = !gGlobalPrefs.showToDoPrio;
	}
	if( FrmGetControlValue( prefP, sCat ) != gGlobalPrefs.showToDoCategories )
	{
		redraw = true;
		gGlobalPrefs.showToDoCategories = !gGlobalPrefs.showToDoCategories;
	}

	// free the form as we do not need it anymore
	FrmDeleteForm( prefP );

	if( reload )
	{
		AllToDosLoadTable( 0 );
#ifdef CONFIG_JOGDIAL
		gRowSelected = noRowSelected;
#endif /* CONFIG_JOGDIAL */
	}
	if( redraw )
	{
		tblP = (TableType *)GetObjectPtr( AllToDosTable );
		if( !reload )
			TblMarkTableInvalid( tblP );
		TblRedrawTable( tblP );
	}
}

/*
 * FUNCTION:				AllToDosOpenAndInsertDB
 * DESCRIPTION:			this routine opens a database and inserts into our 'gDBs' array
 * PARAMETERS:			maindb_index - main database index of the record to open its database
 * RETURNS:					index in the 'gDBs' where the db info was inserted
 */
UInt16 AllToDosOpenAndInsertDB( UInt16 maindb_index )
{
	UInt16	gdb_index;
	PrjtPackedProjectType * prjtP;
	__DatabaseItemType * dbit;

	gdb_index = gDBsNextFreeIndex++;
	dbit = &gDBs[gdb_index];

	prjtP = GetProjectRecord( maindb_index, false );
	ErrFatalDisplayIf( !prjtP, "bad main record" );
	// open the database read/write as we can toggle to the compelete flag of an item
	dbit->db = PrjtDBOpenToDoDB( &prjtP->name, dmModeReadWrite );
	if( dbit->db )
		dbit->org_category = dbit->db_category = PrjtDBGetToDoLastCategory( dbit->db );
	else
		dbit->org_category = dbit->db_category = dmAllCategories;
	dbit->maindb_index = maindb_index;
	MemPtrUnlock( prjtP );

	return (gdb_index);
}

/*
 * FUNCTION:				AllToDosSeekPrevTableItem
 * DESCRIPTION:			this routine finds the previous item to be inserted into our table
 * PARAMETERS:			inItem - from where to start the search
 * 									outItem - NULL of a pointer to a entry to be filled with the correct values if an item is found
 * RETURNS:					true if an item is found, else false
 */
Boolean AllToDosSeekPrevTableItem( __TableItemType * inItem, __TableItemType * outItem )
{
	UInt16	index;
	UInt16	gdb_index;
	UInt16	offset = 1;

	ErrFatalDisplayIf( !inItem, "invalid param" );

	if( inItem->item_type == kProjectNameItemType )
	{
		if( inItem->gdb_index == 0 )
			return (false);

		gdb_index = inItem->gdb_index - 1;
		if( gDBs[gdb_index].db )
		{
			index = DmNumRecords( gDBs[gdb_index].db );
			offset = 0;
		}
	}
	else // if( inItem->item_type == kProjectToDoItemType )
	{
		gdb_index = inItem->gdb_index;
		index = inItem->index;
	}

	if( gDBs[gdb_index].db )
	{
		if( AllToDosSeekToDoRecord( gDBs[gdb_index].db, &index, offset, dmSeekBackward, gDBs[gdb_index].db_category ) )
		{
			if( outItem )
			{
				outItem->index = index;
				outItem->item_type = kProjectToDoItemType;
				outItem->gdb_index = gdb_index;
			}
			return (true);
		}
		else
			goto AllToDosSeekPrevTableItem_Backwards_InsertDBName;
	}
	else
	{
AllToDosSeekPrevTableItem_Backwards_InsertDBName:
		if( outItem )
		{
			outItem->index = gDBs[gdb_index].maindb_index;
			outItem->item_type = kProjectNameItemType;
			outItem->gdb_index = gdb_index;
		}
		return (true);
	}
}

/*
 * FUNCTION:				AllToDosSeekToDoRecord
 * DESCRIPTION:			looks for the next todo in the given database
 * PARAMETERS:			db - where to look
 * 									index - index of the record where the search starts will be updated upon return if the search was success full
 * 									offset - 0 or 1
 *									direction - dmSeekForward or dmSeekBackward
 *									category - in which category to be searched?
 * RETURNS:					true if an item was found, else false
 */
Boolean AllToDosSeekToDoRecord( DmOpenRef db, UInt16 * index, UInt16 offset, Int16 direction, UInt16 category )
{
	UInt16			i;
	DateType		tmpDate;
	MemHandle		recH;
	PrjtToDoType * 	recP;
	UInt32			daysNow = 0;

	ErrFatalDisplayIf( !index, "invalid param" );
	ErrFatalDisplayIf( !db, "invalid param" );

	i = *index;

	// initialize daysNow for later use
	if( gGlobalPrefs.showOnlyDueDateToDos )
	{
		DateSecondsToDate( TimGetSeconds(), &tmpDate );
		daysNow = DateToDays( tmpDate );
	}

	do
	{
		if( DmSeekRecordInCategory( db, &i, offset, direction, category ) )
			return (false);

		if( gGlobalPrefs.showCompleteToDos && !gGlobalPrefs.showOnlyDueDateToDos )
		{
			*index = i;
			return (true);
		}

		recH = DmQueryRecord( db, i );
		recP = MemHandleLock( recH );

		if( gGlobalPrefs.showCompleteToDos || !(recP->priority & kToDosCompleteFlag) )
		{
			if( !gGlobalPrefs.showOnlyDueDateToDos )
				break;

			if( DateToInt( recP->dueDate ) == kToDoNoDueDate )
				break;

			if( DateToDays( recP->dueDate ) <= daysNow )
				break;
		}

		if( !offset )
			offset = 1;

		MemHandleUnlock( recH );
	}
	while( true );

	MemHandleUnlock( recH );
	*index = i;
	return (true);
}

/*
 * FUNCTION:				AllToDosGetDBIndex
 * DESCRIPTION:			this routine returns the index in 'gDBs'
 * 									of the passed database index.
 * PARAMETERS:			maindb_index - index of the projects record in our maindb
 * RETURNS:					noRecordIndex on error, else the index in 'gDBs' of the passed maindb_index
 */
UInt16 AllToDosGetDBIndex( UInt16 maindb_index )
{
	UInt16	i;
	UInt16	ret = noRecordIndex;

	ErrFatalDisplayIf( !gDBs, "gDBs is NULL" );

	for( i = 0; i < gDBsNextFreeIndex; i++ )
		if( gDBs[i].maindb_index >= maindb_index )
		{
			if( maindb_index == gDBs[i].maindb_index )
				ret = i;
			break;
		}

	return (ret);
}

/*
 * FUNCTION:				AllToDosGotoNextProject
 * DESCRIPTION:			this routine updates the buffer so that it shows the next
 * 									or previous possible project and redraws the table
 * PARAMETERS:			nextprev - kGotoNextProject or kGotoPrevProject
 * RETURNS:					nothing
 */
void AllToDosGotoNextProject( UInt16 nextprev )
{
	__TableItemType * tiP;
	UInt16						i;

	ErrFatalDisplayIf( nextprev != kGotoNextProject && nextprev != kGotoPrevProject, "invalid param" );

	if( nextprev == kGotoNextProject )
	{
		for( i = 1; i < gValidRows; i++ )
			if( gTableItems[i].item_type == kProjectNameItemType )
				break;
		if( i != gValidRows )
			gTableItems[0] = gTableItems[i];
		else
		{
			tiP = &gTableItems[0];
			SeekProjectRecord( &tiP->index, 1, dmSeekForward );
			tiP->item_type = kProjectNameItemType;
		}
	}
	else
	{
		tiP = &gTableItems[0];
		if( tiP->item_type == kProjectToDoItemType )
			tiP->item_type = kProjectNameItemType;
		else
			tiP->gdb_index--;
		tiP->index = gDBs[tiP->gdb_index].maindb_index;
	}

	AllToDosLoadTable( 0 );
	TblRedrawTable( (TableType *)GetObjectPtr( AllToDosTable ) );
}



/*
 * FUNCTION:					AllToDosUpdateScrollers
 * DESCRIPTION:				updates the repeating scrollers on the form
 * PARAMETERS:				lastIndex - the index in our buffer (gTableItems) of the last entry
 * RETURNS:						nothing
 * REVISION HISTORY:		Aug 10 2002 - pete
 * 												added updating of the project scrollers
 */
static void AllToDosUpdateScrollers( UInt16 lastIndex )
{
	__TableItemType *				tiP;
	FormType *							frmP;
	ControlType *						ctlP;
	Char *									lblP;
	Char *									newLblP;
	UInt16									tmp;
	UInt16									offset;
	Boolean									enUp, enDn, nxtPrjt;

	ErrFatalDisplayIf( lastIndex >= gValidRows, "invalid param; index too big" );

	// up scroller ----
	tiP = &gTableItems[0];
	if( tiP->item_type == kProjectNameItemType )
	{
		// as we stored also the gdb_index for project names items
		// we can simple look if it's the first db

#if 1
		enUp = (tiP->gdb_index > 0);
#else
		tmp = tiP->index;
		enUp = SeekProjectRecord( &tmp, 1, dmSeekBackward );
#endif
	}
	else
		enUp = true; // we have a todo record as the first item so there must (!) come at least the project name :-)

	// down scroller ----
	tiP = &gTableItems[lastIndex];
	tmp = gDBs[tiP->gdb_index].maindb_index;
	nxtPrjt = enDn = SeekProjectRecord( &tmp, 1, dmSeekForward );

	if( !enDn )
	{
		if( gDBs[tiP->gdb_index].db )
		{
			if( tiP->item_type == kProjectNameItemType )
			{
				tmp = 0;
				offset = 0;
			}
			else // if( tiP->item_type = kProjectToDoItemType )
			{
				tmp = tiP->index;
				offset = 1;
			}
			enDn = AllToDosSeekToDoRecord( gDBs[tiP->gdb_index].db, &tmp, offset, dmSeekForward, gDBs[tiP->gdb_index].db_category );
		}
		else
			enDn = false;
	}

	frmP = FrmGetActiveForm();

	// this is necessary as we test if these controls are
	// enabled and in this situation the FrmUpdateScrollers
	// function simple hides the controls but does not disabled
	// them :-(
	if( !enUp && !enDn )
	{
		tmp = FrmGetObjectIndex( frmP, AllToDosScrollUpRepeating );
		ctlP = FrmGetObjectPtr( frmP, tmp );
		CtlSetEnabled( ctlP, false );
		FrmHideObject( frmP, tmp );

		tmp = FrmGetObjectIndex( frmP, AllToDosScrollDownRepeating );
		ctlP = FrmGetObjectPtr( frmP, tmp );
		CtlSetEnabled( ctlP, false );
		FrmHideObject( frmP, tmp );
	}
	else
	{
		// update the up and down scrollers
		tmp = FrmGetObjectIndex( frmP, AllToDosScrollUpRepeating );
		FrmUpdateScrollers( frmP, tmp, FrmGetObjectIndex( frmP, AllToDosScrollDownRepeating ), enUp, enDn );
	}

	if( !enUp && !nxtPrjt )
	{
		FrmHideObject( frmP, FrmGetObjectIndex( frmP, AllToDosScrollPrevProjectButton ) );
		FrmHideObject( frmP, FrmGetObjectIndex( frmP, AllToDosScrollNextProjectButton ) );
	}
	else
	{
		// update the prev/next project scrollers
		ctlP = FrmGetObjectPtr( frmP, FrmGetObjectIndex( frmP, AllToDosScrollPrevProjectButton ) );
		lblP = (Char *)CtlGetLabel( ctlP );
		newLblP = enUp ? "\x02" : "\x05";
		if( *lblP != *newLblP )
		{
			StrCopy( lblP, newLblP );
			CtlSetEnabled( ctlP, enUp );
			CtlSetLabel( ctlP, lblP );
		}

		ctlP = FrmGetObjectPtr( frmP, FrmGetObjectIndex( frmP, AllToDosScrollNextProjectButton ) );
		lblP = (Char *)CtlGetLabel( ctlP );
		newLblP = nxtPrjt ? "\x03" : "\x06";
		if( *lblP != *newLblP )
		{
			StrCopy( lblP, newLblP );
			CtlSetEnabled( ctlP, nxtPrjt );
			CtlSetLabel( ctlP, lblP );
		}
	}
}

/*
 * FUNCTION:					AllToDosHandleScrolling
 * DESCRIPTION:				this routine scrolls the table contents
 * PARAMETERS:				scrollType - one of the above defined scroll values
 * RETURNS:						nothing
 */
void AllToDosHandleScrolling( UInt16 scrolltype )
{
	__TableItemType ti_buffer;
	TableType * 	tblP;
	Int16					i, tmp;
	UInt16				index;

	tblP = (TableType *)GetObjectPtr( AllToDosTable );

	if( scrolltype == kScrollByOneLineDownwards )
	{
		// shift the gTableItems by one upwards
		for( i = 1; i < gValidRows; i++ )
			gTableItems[i-1] = gTableItems[i];
		if( gTableItems[gValidRows-1].item_type == kProjectNameItemType )
			i = gValidRows - 2;
		else
			i = gValidRows - 1;
		AllToDosLoadTable( i );
	}
	else if( scrolltype == kScrollByOneLineUpwards )
	{
		for( i = gValidRows-1; i > 0; i-- )
			gTableItems[i] = gTableItems[i-1];
		AllToDosSeekPrevTableItem( &gTableItems[0], &gTableItems[0] );

		index = TblGetLastUsableRow( tblP );
		if( index != (gValidRows- 1) )
			TblSetRowUsable( tblP, ++index, true );
		AllToDosUpdateScrollers( index );
	}
	else if( scrolltype == kScrollByOnePageUpwards )
	{
		gValidRows--;
		for( i = 0; i < gValidRows; i++ )
			if( AllToDosSeekPrevTableItem( &gTableItems[0], &ti_buffer ) )
			{
				for( tmp = gValidRows; tmp > 0; tmp-- )
					gTableItems[tmp] = gTableItems[tmp-1];
				gTableItems[0] = ti_buffer;
			}
			else
				break;
		gValidRows++;
		AllToDosUpdateScrollers( gValidRows-1 );
	}
	else if( scrolltype == kScrollByOnePageDownwards )
	{
		index = TblGetLastUsableRow( tblP );
		gTableItems[1] = gTableItems[0] = gTableItems[index];
		if( gTableItems[0].item_type == kProjectNameItemType )
			index = 0;
		else
			index = 1;

		AllToDosLoadTable( index );
	}

	TblMarkTableInvalid( tblP );
	TblRedrawTable( tblP );
}

/*
 * FUNCTION:					AllToDosLoadTable
 * DESCRIPTION:				this routine loads the table and updates the scrollers
 * 										however it does not redraw the table
 * PARAMETERS:				startIndex - where to start (row) initializing the table;
 * 																	the entry in 'gTableItems[startIndex] will be
 * 																	used to initialize the function
 * RETURNS:						nothing
 */
void AllToDosLoadTable( UInt16 startIndex )
{
	__TableItemType 				ti_buffer;
	__TableItemType *				tiP;
	TableType * 						tblP;
	DmOpenRef		todoDB;
	UInt16			todoI;
	Int16				rows, i, tmp;
	Int16				maindb_index;
	UInt16			item_type;
	UInt16			gdb_index;
	UInt16			lastI;
	UInt16			attr;
	Boolean			filledUp = true;
	Boolean			masked;

	tblP = (TableType *)GetObjectPtr( AllToDosTable );
	rows = TblGetNumberOfRows( tblP );
	ErrFatalDisplayIf( !rows, "zero rows table" );
	ErrFatalDisplayIf( startIndex >= gValidRows, "invalid param" );

	lastI = i = startIndex;
	tiP = &gTableItems[i];
	item_type = tiP->item_type;
	if( item_type == kProjectNameItemType )
	{
		maindb_index = tiP->index;
		todoI = 0;
	}
	else
	{
		todoI = tiP->index;
		maindb_index = gDBs[tiP->gdb_index].maindb_index;
	}

	todoI = (item_type == kProjectNameItemType) ? 0 : tiP->index+1;
	do
	{
		if( SeekProjectRecord( &maindb_index, 0, dmSeekForward ) )
		{
			// grab the database name
			if( item_type == kProjectNameItemType )
			{
				// on this form we skip masked projects
#ifdef CONFIG_OS_BELOW_35
				if( gGlobalPrefs.rom35present )
				{
#endif
					DmRecordInfo( gMainDatabase.db, maindb_index, &attr, NULL, NULL );
					masked = ((gGlobalPrefs.privateRecordStatus == maskPrivateRecords) && (attr & dmRecAttrSecret));
					if( masked )
					{
						maindb_index++;
						continue;
					}
#ifdef CONFIG_OS_BELOW_35
				}
#endif
				tiP = &gTableItems[i];
				tiP->index = maindb_index;
				tiP->item_type = kProjectNameItemType;

				// grab the database todo records
				gdb_index = AllToDosGetDBIndex( maindb_index );
				if( gdb_index == noRecordIndex )
					gdb_index = AllToDosOpenAndInsertDB( maindb_index );
				tiP->gdb_index = gdb_index;
				TblSetRowUsable( tblP, i, true );
				TblMarkRowInvalid( tblP, i );
				todoI = 0;
				lastI = i;
				i++;
			}
			else
			{
				gdb_index = AllToDosGetDBIndex( maindb_index );
				ErrFatalDisplayIf( gdb_index == noRecordIndex, "db not in gDBs" );

				// preparation for the next iteration
				item_type = kProjectNameItemType;
			}

			if( (todoDB = gDBs[gdb_index].db) )
			{
				attr = gDBs[gdb_index].db_category;

				// now insert the todo items of the database
				while( i < gValidRows )
				{
					if( !AllToDosSeekToDoRecord( todoDB, &todoI, 0, dmSeekForward, attr ) )
						break;
					else
					{
						tiP = &gTableItems[i];
						tiP->index = todoI;
						tiP->item_type  = kProjectToDoItemType;
						tiP->gdb_index = gdb_index;
						TblSetRowUsable( tblP, i, true );
						TblMarkRowInvalid( tblP, i );
						lastI = i;
						i++;
						todoI++;
					}
				}
			}

			maindb_index++;
		}
		else
			break;
	}
	while( i < gValidRows );

	// now ensure that our table is full
	while( i < gValidRows && AllToDosSeekPrevTableItem( &gTableItems[0], &ti_buffer ) )
	{
		for( tmp = i; tmp > 0; tmp-- )
			gTableItems[tmp] = gTableItems[tmp-1];
		gTableItems[0] = ti_buffer;
		TblSetRowUsable( tblP, i, true );
		filledUp = true;
		lastI = i;
		i++;
	}

	while( i < rows )
		TblSetRowUsable( tblP, i++, false );

	AllToDosUpdateScrollers( lastI );

	if( filledUp )
		TblMarkTableInvalid( tblP );
}

/*
 * FUNCTION:					AllToDosDrawTableItem
 * DESCRIPTION:				this is a callback routine which
 * 										draws the items in our nice table
 * PARAMETERS:				(see DrawTableItemFuncType in palm sdk)
 * RETURNS:						nothing
 */
void AllToDosDrawTableItem( void * tblP, Int16 row, Int16 col, RectangleType * bounds )
{
	Char										szBuffer[dmCategoryLength];
	__TableItemType * 			titP;
	PrjtPackedProjectType * maindb_recP;
	Char	*									strP;
	PrjtToDoType * 					todoP;
	BitmapType * 						bmpP;
	MemHandle 							bmpH;
	MemHandle								todoH;
	DmOpenRef								todoDB;
	FontID									prvFnt;
	UInt16									width, len, tmp, cat_width;
	Boolean									fitInWidth;
	Boolean									todoFinished, itemIsDue = false;
	Boolean									showCategory;
	Char										horzEllipsis;
	UInt16									x, y;
#ifdef CONFIG_COLOR
	IndexedColorType prvTextColor = 0;
#endif

	ErrFatalDisplayIf( !gMainDatabase.db, "main db not open" );
	ErrFatalDisplayIf( !gTableItems, "gTableItems is NULL" );
	ErrFatalDisplayIf( !gDBs, "gDBs is NULL" );

#ifdef CONFIG_OS_BELOW_35
	if( !gGlobalPrefs.rom35present )
		WinEraseRectangle( bounds, 0 );
#endif

	prvFnt = FntGetFont();
	titP = &gTableItems[row];
	y = bounds->topLeft.y;

	if( titP->item_type == kProjectNameItemType )
	{
		// grab the main db record
		maindb_recP = GetProjectRecord( titP->index, false );
		ErrFatalDisplayIf( !maindb_recP, "bad record" );

		if( gDBs[titP->gdb_index].db )
		{
			CategoryGetName( gDBs[titP->gdb_index].db, gDBs[titP->gdb_index].db_category, szBuffer );
#ifdef CONFIG_HANDERA
			if( gGlobalPrefs.vgaExtension && VgaIsVgaFont( gGlobalPrefs.allToDosFont ) )
				FntSetFont( VgaBaseToVgaFont( stdFont ) );
			else
#endif /* CONFIG_HANDERA */
				FntSetFont( stdFont );
			cat_width = FntCharsWidth( szBuffer, StrLen( szBuffer ) );
#ifdef CONFIG_HANDERA
			if( gGlobalPrefs.vgaExtension && VgaIsVgaFont( gGlobalPrefs.allToDosFont ) )
				cat_width += kPopupTriangleBigWidth;
			else
#endif
				cat_width += kPopupTriangleWidth;
		}
		else
			cat_width = 0;

#ifdef CONFIG_HANDERA
		if( gGlobalPrefs.vgaExtension && VgaIsVgaFont( gGlobalPrefs.allToDosFont ) )
		{
			tmp = VgaVgaToBaseFont( gGlobalPrefs.allToDosFont );
			if( tmp == largeFont )
				tmp = largeBoldFont;
			else if( tmp == stdFont )
				tmp = boldFont;
			FntSetFont( VgaBaseToVgaFont( tmp ) );
		}
		else
#endif
		{
			if( gGlobalPrefs.allToDosFont == stdFont )
				tmp = boldFont;
			else if( gGlobalPrefs.allToDosFont == largeFont )
				tmp = largeBoldFont;
			else
				tmp = stdFont;
			FntSetFont( tmp );
		}

		strP = &maindb_recP->name;
		len = StrLen( strP );
		width = bounds->extent.x - (kDBNameOffset+3+cat_width);
		FntCharsInWidth( strP, &width, &len, &fitInWidth );
		if( !fitInWidth && cat_width > bounds->extent.x>>2 )
		{
			cat_width = bounds->extent.x>>2;
			width = bounds->extent.x - (kDBNameOffset+2+cat_width);
			len = StrLen( strP );
			FntCharsInWidth( strP, &width, &len, &fitInWidth );
		}

    gDBs[titP->gdb_index].name_width = width;

		if( fitInWidth )
		{
			tmp = y + (bounds->extent.y>>1);
			WinDrawChars( strP, len, bounds->topLeft.x + kDBNameOffset, y );
			WinDrawLine( bounds->topLeft.x + (kDBNameOffset+2) + width, tmp, (bounds->topLeft.x + bounds->extent.x) - (4+cat_width) , tmp );
		}
		else
		{
			ChrHorizEllipsis( &horzEllipsis );
			width -= FntCharWidth( horzEllipsis );
			FntCharsInWidth( strP, &width, &len, &fitInWidth );
			WinDrawChars( strP, len, bounds->topLeft.x + kDBNameOffset, y );
			WinDrawChars( &horzEllipsis, 1, bounds->topLeft.x + kDBNameOffset + width, y );
		}

		// unlock the main db record
		MemPtrUnlock( maindb_recP );

		if( gDBs[titP->gdb_index].db )
		{
#ifdef CONFIG_HANDERA
			if( gGlobalPrefs.vgaExtension && VgaIsVgaFont( gGlobalPrefs.allToDosFont ) )
				FntSetFont( VgaBaseToVgaFont( stdFont ) );
			else
#endif /* CONFIG_HANDERA */
				FntSetFont( stdFont );
			x = (bounds->topLeft.x + bounds->extent.x) - (cat_width+1);

#ifdef CONFIG_HANDERA
			tmp = PopupTriangleBitmap;
			if( gGlobalPrefs.vgaExtension && VgaIsVgaFont( gGlobalPrefs.allToDosFont ) )
				tmp += 0x1000;
			bmpH = DmGetResource( bitmapRsc, tmp );
#else
			bmpH = DmGetResource( bitmapRsc, PopupTriangleBitmap );
#endif /* CONFIG_HANDERA */
			ErrFatalDisplayIf( !bmpH, "resource not found" );
			bmpP = MemHandleLock( bmpH );
#ifdef CONFIG_HANDERA
			if( gGlobalPrefs.vgaExtension && VgaIsVgaFont( gGlobalPrefs.allToDosFont ) )
			{
				y += (FntLineHeight()-kPopupTriangleHeight)>>1;
				WinDrawBitmap( bmpP, x, y );
				y -= (FntLineHeight()-kPopupTriangleHeight)>>1;
				x += kPopupTriangleBigWidth;
				cat_width -= kPopupTriangleBigWidth;
			}
			else
#endif /* CONFIG_HANDERA */
			{
				WinDrawBitmap( bmpP, x, y );
				x += kPopupTriangleWidth;
				cat_width -= kPopupTriangleWidth;
			}
			MemHandleUnlock( bmpH );
			DmReleaseResource( bmpH );

			width = cat_width;
			len = StrLen( szBuffer );
			FntCharsInWidth( szBuffer, &width, &len, &fitInWidth );
			WinDrawChars( szBuffer, len, x, y );
#ifdef CONFIG_HANDERA
			if( gGlobalPrefs.vgaExtension && VgaIsVgaFont( gGlobalPrefs.allToDosFont ) )
				cat_width += kPopupTriangleBigWidth;
			else
#endif /* CONFIG_HANDERA */
				cat_width += kPopupTriangleWidth;
			gDBs[titP->gdb_index].category_width = cat_width;
		}
	}
	else // if( titP->item_type == kProjectToDoItemType )
	{
		x = bounds->topLeft.x + kToDoCompletionOffset;

		todoDB = gDBs[titP->gdb_index].db;
		ErrFatalDisplayIf( !todoDB, "db not open" );
		todoH = DmQueryRecord( todoDB, titP->index );
		ErrFatalDisplayIf( !todoH, "bad record" );
		todoP = MemHandleLock( todoH );
		showCategory = (gGlobalPrefs.showToDoCategories && gDBs[titP->gdb_index].db_category == dmAllCategories);

		// first: draw the checkbox (always unchecked)
		// temporarily we use 'horzEllipsis' here
		// after this block the 'todoFinished' variable
		// is properly initialized
		width = (kToDoCompletionWidth+1);
#ifdef CONFIG_HANDERA
		if( gGlobalPrefs.vgaExtension && VgaIsVgaFont( gGlobalPrefs.allToDosFont ) )
		{
			FntSetFont( VgaBaseToVgaFont( symbol11Font ) );
			width += 4;
		}
		else
#endif /* CONFIG_HANDERA */
			FntSetFont( symbol11Font );
		todoFinished = todoP->priority & kToDosCompleteFlag;
		if( todoFinished )
			horzEllipsis = '\x01';
		else
			horzEllipsis = '\x00';

#ifdef CONFIG_COLOR
		if( gGlobalPrefs.useColors && todoFinished )
			prvTextColor = WinSetTextColor( GRAY_COLOR_INDEX );
#endif

		WinDrawChars( &horzEllipsis, 1, x, y );
		x += width;

		// second: draw the priority of that item
		// temporarily we use 'horzEllipsis' here
		if( gGlobalPrefs.showToDoPrio )
		{
#ifdef CONFIG_HANDERA
			if( gGlobalPrefs.vgaExtension && VgaIsVgaFont( gGlobalPrefs.allToDosFont ) )
			{
				tmp = VgaVgaToBaseFont( gGlobalPrefs.allToDosFont );
				if( tmp == stdFont )
					tmp = boldFont;
				else if( tmp == largeFont )
					tmp = largeBoldFont;
				FntSetFont( VgaBaseToVgaFont( tmp ) );
			}
			else
#endif /* CONFIG_HANDERA */
			{
				if( gGlobalPrefs.allToDosFont == stdFont )
					tmp = boldFont;
				else if( gGlobalPrefs.allToDosFont == largeFont )
					tmp = largeBoldFont;
        else
          tmp = gGlobalPrefs.allToDosFont;
        FntSetFont( tmp );
			}
			horzEllipsis = '0';
			horzEllipsis += (todoP->priority & kToDosPriorityMask);
			WinDrawChars( &horzEllipsis, 1, x, y );
			x += (kToDoPriorityWidth+3);
		}

		// third: draw the record's due date if necessary
		itemIsDue = ToDoItemIsDue( todoP );
		if( gGlobalPrefs.showToDoDueDate )
		{
			// push temporarily the x value on the stack
			// as we need the current x value for later use
			asm( "move.w %0, -(%%sp)" : : "d" (x) );

			x = (bounds->topLeft.x + bounds->extent.x) - kToDoDueDateWidth;
			if( showCategory )
				x -= (kToDoCategoryWidth+1);
			if( DateToInt( todoP->dueDate ) == kNoDate )
			{
				FntSetFont( gGlobalPrefs.allToDosFont );
				strP = " \x97 ";
				len = StrLen( strP );
				width = FntCharsWidth( strP, len);
				WinDrawChars( strP, len, x + (kToDoDueDateWidth - width)/2, y );
			}
			else
			{
				DateToAsciiWithoutYear( todoP->dueDate, szBuffer );

#ifdef CONFIG_HANDERA
				if( gGlobalPrefs.vgaExtension && VgaIsVgaFont( gGlobalPrefs.allToDosFont ) )
				{
					tmp = VgaVgaToBaseFont( gGlobalPrefs.allToDosFont );
					if( tmp == stdFont )
						tmp = boldFont;
					else if( tmp == largeFont )
						tmp = largeBoldFont;
					FntSetFont( VgaBaseToVgaFont( tmp ) );
				}
				else
#endif /* CONFIG_HANDERA */
				{
					if( gGlobalPrefs.allToDosFont == stdFont )
						tmp = boldFont;
					else if( gGlobalPrefs.allToDosFont == largeFont )
						tmp = largeBoldFont;
          else
            tmp = gGlobalPrefs.allToDosFont;
          FntSetFont( tmp );
				}
				tmp = FntCharWidth('!');

				if( itemIsDue )
					WinDrawChars( "!", 1, x + (kToDoDueDateWidth - tmp), y );

				FntSetFont( gGlobalPrefs.allToDosFont );
				len = StrLen( szBuffer );
				width = FntCharsWidth( szBuffer, len );
				WinDrawChars( szBuffer, len, x + (kToDoDueDateWidth - (width+tmp)), y );
			}

			// restore the previously pushed value
			asm( "move.w (%%sp)+, %0" : "=d" (x) : );
		}

		if( showCategory )
		{
			FntSetFont( gGlobalPrefs.allToDosFont );

			DmRecordInfo( todoDB, titP->index, &tmp, NULL, NULL );
			CategoryGetName( todoDB, tmp & dmRecAttrCategoryMask, szBuffer );
			width = kToDoCategoryWidth;
			len = StrLen( szBuffer );
			FntCharsInWidth( szBuffer, &width, &len, &fitInWidth );
			WinDrawChars( szBuffer, len, (bounds->topLeft.x + bounds->extent.x) - width, y );
		}

		// fourth: draw the record's description
		strP = &todoP->description;
		FntSetFont( gGlobalPrefs.allToDosFont );
		len = StrLen( strP );
		width = bounds->extent.x - x;
		if( gGlobalPrefs.showToDoDueDate )
			width -= (kToDoDueDateWidth+1);
		if( showCategory )
			width -= (kToDoCategoryWidth+1);

		FntCharsInWidth( strP, &width, &len, &fitInWidth );
#ifdef CONFIG_COLOR
		if( gGlobalPrefs.useColors && itemIsDue )
			prvTextColor = WinSetTextColor( RED_COLOR_INDEX );
#endif
		if( fitInWidth )
			WinDrawChars( strP, len, x, y );
		else
		{
			ChrHorizEllipsis( &horzEllipsis );
			width -= FntCharWidth( horzEllipsis );
			FntCharsInWidth( strP, &width, &len, &fitInWidth );
			WinDrawChars( strP, len, x, bounds->topLeft.y );
			WinDrawChars( &horzEllipsis, 1, x + width, bounds->topLeft.y );
		}
#ifdef CONFIG_COLOR
		if( gGlobalPrefs.useColors && (itemIsDue || todoFinished) )
			WinSetTextColor( prvTextColor );
#endif

		MemHandleUnlock( todoH );
	}

	// restore the font
	FntSetFont( prvFnt );

#ifdef CONFIG_JOGDIAL
	if( gRowSelected == row )
	{
		RctInsetRectangle( bounds, 1 );
		WinDrawGrayRectangleFrame( rectangleFrame, bounds );
		RctInsetRectangle( bounds, -1 );
	}
#endif /* CONFIG_JOGDIAL */
}

/*
 * FUNCTION:					AllToDosDeInit
 * DESCRIPTION:				this routine makes cleans up the form's data.
 * CALLED:						when returning from this form
 * PARAMETERS:				none
 * RETURNS:						nothing
 */
void AllToDosDeInit( void )
{
	UInt16	i;

	ErrFatalDisplayIf( !gTableItems, "gTableItems is NULL" );
	ErrFatalDisplayIf( !gDBs, "gDBs is NULL" );

	// free the table items array
	MemPtrFree( gTableItems );
	gTableItems = NULL;

	// free the structs for the opened databases
	for( i = 0; i < gDBsNextFreeIndex; i++ )
		if( gDBs[i].db )
		{
			if( gDBs[i].db_category != gDBs[i].org_category )
				PrjtDBSetToDoLastCategory( gDBs[i].db, gDBs[i].db_category );
			DmCloseDatabase( gDBs[i].db );
		}

	MemPtrFree( gDBs );
	gDBs = NULL;
}

/*
 * FUNCTION:					AllToDosInit
 * DESCRIPTION:				initializes the form's table
 * CALLED:						before we draw our table
 * PARAMETERS:				frmP - pointer to the form
 * RETURNS:						nothing
 */
void AllToDosInit( FormType * frmP )
{
	RectangleType	r;
	TableType * 	tblP;
	Int16					i, rows;
	UInt16				lineHeight;
	FontID				font;

	tblP = FrmGetObjectPtr( frmP, FrmGetObjectIndex( frmP, AllToDosTable ) );
	rows = TblGetNumberOfRows( tblP );

#ifdef CONFIG_HANDERA
	if( gGlobalPrefs.vgaExtension )
	{
		VgaFormModify( frmP, vgaFormModify160To240 );
		VgaTableUseBaseFont( tblP, true );
		AllToDosResizeForm( frmP, false );
	}
#endif /* CONFIG_HANDERA */

	font = FntSetFont( gGlobalPrefs.allToDosFont );
	lineHeight = FntLineHeight();
	FntSetFont( font );
	for( i = 0; i < rows; i++ )
	{
		TblSetRowHeight( tblP, i, lineHeight );
		TblSetItemStyle( tblP, i, kToDosDescColumn, customTableItem );
		TblSetItemFont( tblP, i, kToDosDescColumn, gGlobalPrefs.allToDosFont );
	}
	TblSetColumnUsable( tblP, kToDosDescColumn, true );
	TblSetCustomDrawProcedure( tblP, kToDosDescColumn, AllToDosDrawTableItem );

	TblGetBounds( tblP, &r );
	gValidRows = r.extent.y / lineHeight;
	if( rows < gValidRows )
		gValidRows = rows;

	// allocate memory for out gTableItems
	// as we need to success the information there often
	// we use MemPtrNew instead of MemHandleNew
	gTableItems = MemPtrNew( sizeof( __TableItemType ) * rows );
	ErrFatalDisplayIf( !gTableItems, "not enough memory" );

	// init the buffers first entry for the load function!
	gTableItems[0].index = 0;
	gTableItems[0].item_type = kProjectNameItemType;
	gTableItems[0].gdb_index = noRecordIndex;

	// allocate buffer for out gDBs array to hold references
	// to opened databases! first find out how much memory we do need.
	i = rows = 0;
	do
	{
		if( SeekProjectRecord( &i, 0, dmSeekForward ) )
		{
			rows++;
			i++;
		}
		else
			break;
	}
	while( 1 );
	ErrFatalDisplayIf( !rows, "no projects to be displayed" );
	gDBs = MemPtrNew( sizeof( __DatabaseItemType ) * rows );
	ErrFatalDisplayIf( !gDBs, "not enough memory" );
	// init the memory
	for( i = 0; i < rows; i++ )
	{
		gDBs[i].db = 0;
		gDBs[i].maindb_index = noRecordIndex;
	}
#if ERROR_CHECK_LEVEL != ERROR_CHECK_NONE
	gDBsNum = rows;
#endif
	gDBsNextFreeIndex = 0;

	// load the table
	AllToDosLoadTable( 0 );

#ifdef CONFIG_JOGDIAL
	gRowSelected = noRecordIndex;
#endif /* CONFIG_JOGDIAL */
}

/*
 * FUNCTION:					AllToDosHandleEvent
 * DESCRIPTION:				this routine handles all events on the dialog
 * PARAMETERS:				eventP - the event
 * RETURNS:						true if the event was handled
 * 										else false is returned.
 */
Boolean AllToDosHandleEvent( EventType * eventP )
{
  Boolean handled = false;
  FormType * frmP;

  switch( eventP->eType )
	{
		case menuEvent:
			if( eventP->data.menu.itemID == OptionsAbout )
			{
				ShowAbout();
				handled = true;
			}
			else if( eventP->data.menu.itemID == OptionsFont )
			{
				AllToDosSelectFont();
				handled = true;
			}
			break;

		case keyDownEvent:
			if( ChrIsHardKey( eventP->data.keyDown.chr ) )
			{
				if( !(eventP->data.keyDown.modifiers & poweredOnKeyMask) )
				{
					FrmGotoForm( MainForm );
					handled = true;
				}
			}
			else if( eventP->data.keyDown.chr == pageDownChr )
			{
				if( CtlEnabled( GetObjectPtr( AllToDosScrollDownRepeating ) ) )
				{
#ifdef CONFIG_JOGDIAL
					gRowSelected = noRowSelected;
#endif /* CONFIG_JOGDIAL */
					AllToDosHandleScrolling( kScrollByOnePageDownwards );
				}
				handled = true;
			}
			else if( eventP->data.keyDown.chr == pageUpChr )
			{
				if( CtlEnabled( GetObjectPtr( AllToDosScrollUpRepeating ) ) )
				{
#ifdef CONFIG_JOGDIAL
					gRowSelected = noRowSelected;
#endif /* CONFIG_JOGDIAL */
					AllToDosHandleScrolling( kScrollByOnePageUpwards );
				}
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
				FrmGotoForm( MainForm );
				handled = true;
			}
	#ifdef CONFIG_SONY
			else if( eventP->data.keyDown.chr == vchrJogDown )
	#else
				// applies to handera and normal version
			else if( eventP->data.keyDown.chr == vchrNextField )
	#endif /* CONFIG_SONY */
			{
				AllToDosSelectItem( selectNext );
				handled = true;
			}
	#ifdef CONFIG_SONY
			else if( eventP->data.keyDown.chr == vchrJogUp )
	#else
				// applies to handera and normal version
			else if( eventP->data.keyDown.chr == vchrPrevField )
	#endif /* CONFIG_SONY */
			{
				AllToDosSelectItem( selectPrev );
				handled = true;
			}
	#ifdef CONFIG_SONY
			else if( eventP->data.keyDown.chr == vchrJogPush )
	#else
			#ifndef CONFIG_HANDERA
				#error "please define CONFIG_HANDERA or CONFIG_SONY or remove the CONFIG_JOGDIAL definition!"
			#endif /* CONFIG_HANDERA */
			else if( eventP->data.keyDown.chr == chrCarriageReturn )
	#endif /* CONFIG_SONY */
			{
				if( gRowSelected == noRowSelected )
					AllToDosSelectItem( selectNext );
				else if( gTableItems[gRowSelected].item_type == kProjectToDoItemType )
					AllToDosGotoToDoItem( &gTableItems[gRowSelected] );
				else
				{
					gCurrentProject.projIndex = gDBs[gTableItems[gRowSelected].gdb_index].maindb_index;
					gCurrentProject.currentPage = ToDoPage;
					FrmGotoForm( ProjectForm );
				}
				handled = true;
			}
#endif /* CONFIG_JOGDIAL */
			break;

		case frmOpenEvent:
			frmP = FrmGetActiveForm();
			AllToDosInit( frmP );
			FrmDrawForm( frmP );
			handled = true;
			break;

		case frmCloseEvent:
			AllToDosDeInit();
			break;

		case ctlSelectEvent:
			if( eventP->data.ctlSelect.controlID == AllToDosDoneButton )
			{
				FrmGotoForm( MainForm );
				handled = true;
			}
			else if( eventP->data.ctlSelect.controlID == AllToDosShowButton )
			{
				AllToDosHandlePreferences();
				handled = true;
			}
			else if( eventP->data.ctlSelect.controlID == AllToDosScrollPrevProjectButton )
			{
#ifdef CONFIG_JOGDIAL
				gRowSelected = noRowSelected;
#endif /* CONFIG_JOGDIAL */
				AllToDosGotoNextProject( kGotoPrevProject );
				handled = true;
			}
			else if( eventP->data.ctlSelect.controlID == AllToDosScrollNextProjectButton )
			{
#ifdef CONFIG_JOGDIAL
				gRowSelected = noRowSelected;
#endif /* CONFIG_JOGDIAL */
				AllToDosGotoNextProject( kGotoNextProject );
				handled = true;
			}
			break;

		case ctlRepeatEvent:
			if( eventP->data.ctlRepeat.controlID == AllToDosScrollUpRepeating )
			{
#ifdef CONFIG_JOGDIAL
				gRowSelected = noRowSelected;
#endif /* CONFIG_JOGDIAL */
				AllToDosHandleScrolling( kScrollByOneLineUpwards );
			}
			else // if( eventP->data.ctlRepeat.controlID == AllToDosScrollDownRepeating )
			{
#ifdef CONFIG_JOGDIAL
				gRowSelected = noRowSelected;
#endif /* CONFIG_JOGDIAL */
				AllToDosHandleScrolling( kScrollByOneLineDownwards );
			}
			break;

		case tblEnterEvent:
			if( gTableItems[eventP->data.tblEnter.row].item_type == kProjectToDoItemType )
			{
				if( eventP->screenX <= (kToDoCompletionOffset+kToDoCompletionWidth) )
					AllToDosToggleToDoCompleted( eventP->data.tblEnter.row );
				else
				{
					AllToDosGotoToDoItem( &gTableItems[eventP->data.tblEnter.row] );
				}
			}
			else
			{
				AllToDosHandleTapIntoDBNameRow( eventP->data.tblEnter.pTable, eventP->data.tblEnter.row, eventP->screenX, eventP->screenY ); 
			}
			handled = true;
			break;

#ifdef CONFIG_HANDERA
		case displayExtentChangedEvent:
			if( gGlobalPrefs.vgaExtension )
			{
				frmP = FrmGetActiveForm();
				AllToDosResizeForm( frmP, true );
				handled = true;
			}
			break;
#endif /* CONFIG_HANDERA */

		default:
			break;
	}

  return (handled);
}

#endif /* CONFIG_ALLTODOS_DLG */
