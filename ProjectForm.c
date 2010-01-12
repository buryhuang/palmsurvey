#include <PalmOS.h>
#include "ProjectsRcp.h"
#include "ProjectsDB.h"
#include "Projects.h"
#include "DateDB.h"
#include "MemoLookup.h"

#ifndef __PROJECTS_CONFIG_H__
	#error "include config.h"
#endif /* __PROJECTS_CONFIG_H__ */

#ifdef CONFIG_SONY
	#include <SonyCLIE.h>
#endif /* CONFIG_SONY */

#ifdef CONFIG_HANDERA
	#include "Vga.h"
	#include "HndrFuncs.h"
	#include "Silk.h"
#endif /* CONFIG_HANDERA */

#define kProjectFormCompletedTableColumn		0
#define kProjectFormPriorityTableColumn			1
#define kProjectFormDescriptionTableColumn	2
#define kProjectFormDueDateTableColumn			3
#define kProjectFormCategoryTableColumn			4

#define kProjectFormCompletedTableColumnMinWidth 	  14
#define kProjectFormPriorityTableColumnMinWidth		  8
#define kProjectFormDueDateTableColumnMinWidth		  25
#ifdef CONFIG_ADDITIONAL
  #define kProjectFormCategoryTableColumnMinWidth		  5
  #define kProjectFormCategoryMinColumnSymbol         '\xB7'
#endif /* CONFIG_ADDITIONAL */
#define kProjectFormCategoryTableColumnNormalWidth  25

#define noRecordSelected 										0xFFFF
#define maxExportListItems									15
#define exportToDoDBIndexID									0xFFFF
#define exportDateDBIndexID									0xFFFE

typedef enum {selectFirst, selectLast} SelectToDoItemType;

// variable used only in this module --------------------------------------------------------------
#ifdef CONFIG_COLOR
  static RGBColorType gPrevFldTxtColor; // this variable holds the color of the text in a text field used before we modify it, so we can restore it later.
#endif /* CONFIG_COLOR */
static Boolean		gGeneralPageInitialized;
static Boolean		gToDoPageInitialized;
static UInt16			gTopVisibleToDoIndex;
static UInt16			gCurrentToDoEditPosition = 0;
static UInt16			gCurrentToDoEditSelectionLength = 0;
static Boolean		gToDoItemSelected;		// if true -> gCurrentProject.curToDoIndex hold a valid value
static Boolean		gToDoScrollUpEnabled;		// we keep here the state of the two scrollers
static Boolean		gToDoScrollDownEnabled;	// so when switching between page we don't need to call ProjectFormLoadToDoTable each time

// project form related function - declarations ---------------------------------------------------
#ifdef CONFIG_JOGDIAL
static void			ProjectFormSelectToDoItem( SelectToDoItemType select ) SECOND_SECTION;
static void 		ProjectFormSelectNextField( void ) SECOND_SECTION;
static void			ProjectFormSelectPrevField( void ) SECOND_SECTION;
#endif /* CONFIG_JOGDIAL */

static void 		ProjectFormInit( FormPtr frmP ) SECOND_SECTION;
static void			ProjectFormDeInit( void ) SECOND_SECTION;

static void 		ProjectFormSwitchToGeneralPage( FormType * frmP );
static void			ProjectFormSwitchToToDoPage( FormType * frmP, Boolean drawtable );
static void 		ProjectFormSwitchToMemoPage( FormType * frmP );
static void 		ProjectFormInitializeGeneralPage( FormType * frmP );
static void 		ProjectFormInitializeMemoPage( void );
static void 		ProjectFormDeInitializeMemoPage( void );
static void 		ProjectFormInitializeToDoPage( FormType * frmP );

static void 		ProjectFormMakeToDoTableColumsUsable( FormType * frmP );
static void 		ProjectFormSelectProjectsDate( UInt16 controlID );
static void 		ProjectFormSelectPriority( UInt16 controlID );
static void 		ProjectFormSelectState( Int16 listselection );
static void 		ProjectFormUpdateMemoScrollBar( void );
static void 		ProjectFormScrollMemoLines( Int16 numLines, Boolean redraw );
static void			ProjectFormScrollMemoPage( WinDirectionType direction );
static void 		ProjectFormSelectProjectCategory( FormType * frmP );
static void 		ProjectFormSetTableColumns( TableType * tableP );
static void 		ProjectFormLoadToDoTable( Boolean fillTable );
static Err 			ProjectFormLoadDescription( void *tableP, Int16 row, Int16 column, Boolean editable, MemHandle *dataH, Int16 *dataOffset, Int16 *dataSize, FieldType * fieldP );
static Boolean 	ProjectFormSaveDescription( void *tableP, Int16 row, Int16 column );
static void 		ProjectFormCreateNewToDoRecord( void );
static void 		ProjectFormSelectToDoPriority( EventType * eventP );
static UInt16 	ProjectFormGetDescriptionHeight( UInt16 recIndex, UInt16 width );
static void 		ProjectFormUpdateToDoTableScrollers( FormType * frmP, UInt16 lastVisibleRecordIndex, Boolean lastItemClipped );
static void 		ProjectFormToggleToDoCompleted( EventType * eventP );
static void 		ProjectFormSelectDueDate( EventType * eventP );
static void			ProjectFormSelectRecCategory( TableType * tblP, UInt16 row );
static void 		ProjectFormToDoTableResizeDescription( EventType * eventP );
static void			ProjectFormDrawDueDate( void * tableP, Int16 row, Int16 column, RectangleType * bounds );
static void 		ProjectFormDrawRecCategory( void * tblP, Int16 row, Int16 column, RectangleType * bounds );
static void 		ProjectFormDeleteProject( void );
static void			ProjectFormRenameCurrentProject( void );
static void 		ProjectFormSetTitle( FormType * frmP, PrjtPackedProjectType * recP );
static void 		ProjectFormDeleteToDo( void );
static Boolean	ProjectFormToDoClearEditState( void );
static Boolean	ProjectFormClearToDoEditState( void );
static void			ProjectFormHandlePreferencesSetting( void );
static void			ProjectPurgeToDos( void );
static Boolean 	SeekToDoRecord( UInt16 *indexP, Int16 offset, Int16 direction );
static void			ProjectFormScrollToDoPage( WinDirectionType direction );
static void 		ProjectFormInitToDoTableRow( TableType *tableP, Int16 row, UInt16 recIndex, Int16 rowHeight );
static void 		ProjectFormToDoRestoreEditState( void );
static void			ProjectFormToDoDeleteNote( void );
static void 		DetermineDueDate( Int16 itemSelected, DateType *dueDateP );
static Boolean	DetailsDeleteToDo( void );
static void 		ProjectFormDuplicateProject( void ) SECOND_SECTION;
static void			ProjectFormClearEndDate( void ) SECOND_SECTION;
static void 		SelectMemoWordAndDrawForm( UInt16 position, UInt16 len );
static void 		ProjectFormDeleteMemo( void ) SECOND_SECTION; 
static void 		ProjectFormExportMemo( void ) SECOND_SECTION; 
static void 		ProjectFormRescanDatabase( void ) SECOND_SECTION; 
static void 		ProjectFormSelectFont( void ) SECOND_SECTION;
static void 		MoveSelectedToDo( UInt8 direction ) SECOND_SECTION;
static void 		ProjectFormImportMemo( void ) SECOND_SECTION;
static void 		ProjectFormCreateDatabase( void ) SECOND_SECTION;
static void			DrawEraseDBNotFoundMessage( Boolean draw ) SECOND_SECTION;
static void 		ProjectFormMoveEndDate( WinDirectionType direction ) SECOND_SECTION;
static void 		ProjectFormMoveBeginDate( WinDirectionType direction ) SECOND_SECTION;
static void 		ProjectFormAdjustDateWheel( UInt16 justChangedControlId ) SECOND_SECTION;
static void 		ProjectFormMoveDuration( WinDirectionType direction ) SECOND_SECTION;
static void 		ProjectFormEnsureCurrentItemWillBeVisible( void ) SECOND_SECTION;
static void 		ProjectFormSetCategoryTrigger( void ) SECOND_SECTION;

static void 		ProjectFormPopupToDoNote( void ) SECOND_SECTION;

#ifdef CONFIG_HANDERA
	static void			ProjectFormResize( FormType * frmP, Boolean draw ) THIRD_SECTION;
#endif /* CONFIG_HANDERA */


static Boolean	DetailsFormSelectCategory( UInt16 * category );
static void 		DetailsFormApply( UInt16 category, DateType *dueDateP ); //, Boolean categoryEdited );
static void 		DetailsFormExportItem( UInt16 category, DateType * dueDate ) THIRD_SECTION;
static void 		DetailsFormSetDateTrigger( DateType date );
static void 		DetailsFormInit( FormType *frmP, UInt16 *categoryP, DateType *dueDateP );
static void 		DetailsFormExportListDrawFunc( Int16 itemNum, RectangleType * bounds, UInt16 ** recordIndexes );

// details form fucntions -------------------------------------------------------------------------

static void DetailsFormExportListDrawFunc( Int16 itemNum, RectangleType * bounds, UInt16 ** recordIndexes )
{
	Boolean			fitInWidth;
	Char				horzEllipsis;
	UInt16			width;
	UInt16			len;
	UInt16			recIndex;
	MemHandle		recH;
	PrjtPackedProjectType * recP;
	Char *			name;

	ErrFatalDisplayIf( !gMainDatabase.db, "database not open" );


	recIndex = (*recordIndexes)[itemNum];

	if( recIndex == exportToDoDBIndexID )
	{
		recH = DmGetResource( strRsc, ToDoDBName4Export );
		ErrFatalDisplayIf( !recH, "resource not found" );
		name = MemHandleLock( recH );
	}
	else if( recIndex == exportDateDBIndexID )
	{
		recH = DmGetResource( strRsc, DateDBName4Export );
		ErrFatalDisplayIf( !recH, "resource not found" );
		name = MemHandleLock( recH );
	}
	else
	{
		recH = DmQueryRecord( gMainDatabase.db, recIndex );
		if( recH )
		{
			recP = MemHandleLock( recH );
			name = &recP->name;
		}
		else
			return;
	}

	width = bounds->extent.x;
	len = StrLen( name );
			
	FntCharsInWidth( name, &width, &len, &fitInWidth );
	if( !fitInWidth )
	{
		ChrHorizEllipsis( &horzEllipsis );
		width -= FntCharWidth( horzEllipsis );
		FntCharsInWidth( name, &width, &len, &fitInWidth );
	}

	WinDrawChars( name, len, bounds->topLeft.x, bounds->topLeft.y );
	if( !fitInWidth )
		WinDrawChars( &horzEllipsis, 1, bounds->topLeft.x + width, bounds->topLeft.y );

	MemHandleUnlock( recH );
	if( recIndex == exportToDoDBIndexID || recIndex == exportDateDBIndexID )
		DmReleaseResource( recH );
}

static Boolean DetailsDeleteToDo( void )
{
	Boolean			empty;
	Boolean			wasFinished;
	UInt16			recIndex;
	UInt16			num;
	MemHandle 	recH;
	MemHandle		projH;
	PrjtPackedProjectType * projP;
	PrjtToDoType *	recP;

	recIndex = gCurrentProject.curToDoIndex;
	recH = DmQueryRecord( gCurrentProject.todoDB, recIndex );
	ErrFatalDisplayIf( !recH, "bad record" );
	recP = MemHandleLock( recH );
	empty = (!recP->description && !(&recP->description + StrLen( &recP->description ) + 1));
	wasFinished = (recP->priority & kToDosCompleteFlag) ? true : false;
	MemHandleUnlock( recH );

	if( empty )
	{
		ProjectFormClearToDoEditState();
		return (true);
	}

	if( FrmAlert( DeleteToDoAlert ) != DeleteToDoOKButton )
		return (false);

	DmRemoveRecord( gCurrentProject.todoDB, recIndex );
	gToDoItemSelected = false;
	
	projH = DmGetRecord( gMainDatabase.db, gCurrentProject.projIndex );
	ErrFatalDisplayIf( !projH, "bad project record" );
	projP = MemHandleLock( projH );

	if( wasFinished )
	{
		num = projP->numOfFinishedToDos - 1;
		DmWrite( projP, OffsetOf( PrjtPackedProjectType, numOfFinishedToDos ), &num, sizeof( projP->numOfFinishedToDos ) );
	}
	num = projP->numOfAllToDos - 1;
	DmWrite( projP, OffsetOf( PrjtPackedProjectType, numOfAllToDos ), &num, sizeof( projP->numOfAllToDos ) );
	MemHandleUnlock( projH );
	DmReleaseRecord( gMainDatabase.db, gCurrentProject.projIndex, true );

	return (true);
}

static void	DetailsFormApply( UInt16 category, DateType *dueDateP )
{
	Boolean			dirty = false;
	Boolean			sort = false;
	UInt8				newPriority;
	UInt8				curPriority;
	UInt16		 	attr;
	UInt16			index;
	MemHandle		recH;
	FormType *	frmP;
	PrjtToDoType *	recP;

	DmRecordInfo( gCurrentProject.todoDB, gCurrentProject.curToDoIndex, &attr, NULL, NULL );

	frmP = FrmGetActiveForm();
	index = FrmGetControlGroupSelection( frmP, 5 );
	newPriority = (FrmGetObjectId( frmP, index ) - DetailsPriority1PushButton + 1);

	recH = DmQueryRecord( gCurrentProject.todoDB, gCurrentProject.curToDoIndex );
	ErrFatalDisplayIf( !recH, "bad record" );
	recP = MemHandleLock( recH );
	curPriority = recP->priority & kToDosPriorityMask;

	if( curPriority != newPriority )
	{
		newPriority |= (recP->priority & kToDosCompleteFlag);
		DmWrite( recP, OffsetOf( PrjtToDoType, priority ), &newPriority, sizeof( recP->priority ) );
		sort = dirty = true;
	}
	
	if( DateToInt( recP->dueDate ) != DateToInt( *dueDateP ) )
	{
		DmWrite( recP, OffsetOf( PrjtToDoType, dueDate ), dueDateP, sizeof( recP->dueDate ) );
		sort = dirty = true;
	}

	MemHandleUnlock( recH );

	if( (attr & dmRecAttrCategoryMask) != category )
	{
		attr &= ~dmRecAttrCategoryMask;
		attr |= category;
		dirty = true;
	}

	if( dirty )
	{
		attr |= dmRecAttrDirty;
		DmSetRecordInfo( gCurrentProject.todoDB, gCurrentProject.curToDoIndex, &attr, NULL );
	}
	
	if( sort && gCurrentProject.todoDBSortOrder != kSortToDosManually )
		PrjtDBToDoResortRecord( gCurrentProject.todoDB, &gCurrentProject.curToDoIndex, gCurrentProject.todoDBSortOrder );
}

static Boolean DetailsFormSelectCategory( UInt16 * category )
{
	Boolean categoryEdited;
	Char *labelP;

	labelP = (Char *)CtlGetLabel( GetObjectPtr( DetailsCategoryPopTrigger ) );
	categoryEdited = CategorySelect( gCurrentProject.todoDB, FrmGetActiveForm(), 
			DetailsCategoryPopTrigger, DetailsCategoryList, false, category, labelP, 1, 0 );

	// as we dont support sorting by category like in the todo app
	// theres no need to perform a sort like in the todo-example
	return (categoryEdited);
}

static void DetailsFormSetDateTrigger( DateType date )
{
	Int16 dayOfWeek;
	Char *str;
	Char *labelP;
	ListType * listP;
	ControlType *controlP;

	listP = (ListType *)GetObjectPtr( DetailsDueDateList );
	LstSetSelection( listP, kDueDateListNoDateIndex );

	controlP = GetObjectPtr( DetailsDueDatePopTrigger );
	labelP = (Char *)CtlGetLabel( controlP );
	
	if( DateToInt( date ) == kToDoNoDueDate )
	{
		str = LstGetSelectionText( listP, kDueDateListNoDateIndex );
		StrCopy( labelP, str );
		CtlSetLabel( controlP, labelP );
		LstSetSelection( listP, kDueDateListNoDateIndex );
	}
	else
	{
		// format the date into a string
		dayOfWeek = DayOfWeek( date.month, date.day, date.year + firstYear );
		DateToDOWDMFormat( date.month, date.day, date.year + firstYear, gGlobalPrefs.dateFormat, labelP );
		CtlSetLabel( controlP, labelP );
		LstSetSelection( listP, kDueDateListChooseDateIndex );
	}
}

static void DetailsFormInit( FormType *frmP, UInt16 *categoryP, DateType *dueDateP )
{
	UInt16			attr;
	UInt16			category;
	UInt16			priority;
	MemHandle 	recH;
	PrjtToDoType * 	recP;
	Char *			labelP;
	ControlType * controlP;

	ErrFatalDisplayIf( !gCurrentProject.todoDB, "database is not open" );

#ifdef CONFIG_HANDERA
	if( gGlobalPrefs.vgaExtension )
		VgaFormModify( frmP, vgaFormModify160To240 );
#endif /* CONFIG_HANDERA */

	// set the category popup trigger
	DmRecordInfo( gCurrentProject.todoDB, gCurrentProject.curToDoIndex, &attr, NULL, NULL );
	category = attr & dmRecAttrCategoryMask;

	controlP = FrmGetObjectPtr( frmP, FrmGetObjectIndex( frmP, DetailsCategoryPopTrigger ) );
	labelP = (Char *)CtlGetLabel( controlP );
	CategoryGetName( gCurrentProject.todoDB, category, labelP );
	CategorySetTriggerLabel( controlP, labelP );

	recH = DmQueryRecord( gCurrentProject.todoDB, gCurrentProject.curToDoIndex );
	ErrFatalDisplayIf( !recH, "bad record" );
	recP = MemHandleLock( recH );

	// set the priority
	priority = recP->priority & kToDosPriorityMask;
	FrmSetControlGroupSelection( frmP, 5, DetailsPriority1PushButton + priority - 1 );

	// set the duedate
	DetailsFormSetDateTrigger( recP->dueDate );

	*categoryP = category;
	*dueDateP = recP->dueDate;

	MemHandleUnlock( recH );
}	

// this function fills the dueDate with a valid date depending
// on the value of itemSelected which is references to the duedatelist
// in our resource.
// 0 -> today
// 1 -> tomorrow
// 2 -> one week later
// 3 -> choose date...
static void DetermineDueDate( Int16 itemSelected, /*[in][out]*/ DateType *dueDateP )
{
	Int16 month, day, year;
	DateType date;
	MemHandle titleH;
	Char *titleP;
	Int32	adjustment;

	ErrFatalDisplayIf( itemSelected == -1, "itemSelected == -1; invalid param" );

	if( itemSelected == kDueDateListNoDateIndex )
	{
		*((UInt16 *)dueDateP) = kToDoNoDueDate;
		return;
	}
	else if( itemSelected == kDueDateListChooseDateIndex )
	{
		if( *((UInt16 *)dueDateP) == kToDoNoDueDate )
		{
			DateSecondsToDate( TimGetSeconds(), &date );
			year = date.year + firstYear;
			month = date.month;
			day = date.day;
		}
		else
		{
			year = dueDateP->year + firstYear;
			month = dueDateP->month;
			day = dueDateP->day;
		}

		titleH = DmGetResource( strRsc, SelectDueDateTitle );
		ErrFatalDisplayIf( !titleH, "resource is not there" );
		titleP = (Char *)MemHandleLock( titleH );
		if( SelectDay( selectDayByDay, &month, &day, &year, titleP ) )
		{
			dueDateP->month = month;
			dueDateP->day = day;
			dueDateP->year = year - firstYear;
		}
		MemHandleUnlock( titleH );
		DmReleaseResource( titleH );

		return;
	}
	else if( itemSelected == kDueDateListTodayIndex )
		adjustment = 0;
	else if( itemSelected == kDueDateListTomorrowIndex )
		adjustment = 1;
	else if( itemSelected == kDueDateListOneWeekLaterIndex )
		adjustment = 7;
	else
		adjustment = 0;		// should not happed; just to make the compiler happy

	DateSecondsToDate( TimGetSeconds(), dueDateP );
	DateAdjust( dueDateP, adjustment );
}

/*
 * FUNCTION:			DetailsFormExportItem
 * DESCRIPTION:		Displays a list of projects
 * 								and copies the current todo item 
 * 								to a project database depending on
 * 								what the user choosed
 * CALLED:				when the user taps the export button
 * PARAMENTERS:		none
 *
 * HISTORY:				09/jan/02 - pete:
 * 									- the new item will be assigned the same category
 * 									  as the old one if the database contain the same
 * 									  category
 *									- if the record is going to be copied its changes
 *									  will be applied (therefore changed header)
 */
static void DetailsFormExportItem( UInt16 category, DateType * dueDate )
{
	Boolean			finished;
	UInt8				sortOrder;
	UInt16			numRecs;
	UInt16			listIndex;
	UInt16			i;
	UInt16			recIndex;
	UInt16			curCategory;

	UInt16			foundCategory;
	UInt16			attr;

	Int16				listSelection;
	MemHandle		recH;
	MemHandle		todoH;
	MemHandle		curToDoH;
	DmOpenRef		db;
	SortRecordInfoType sortInfo;
	PrjtToDoType *	todoP;
	PrjtPackedProjectType * recP;
	FormType * 	frmP;
	ListType * 	listP;
	UInt16	*		recIndexes;
	UInt32			size;
	RectangleType r;
	RectangleType listR;
	Char				categoryName[dmCategoryLength];

	ErrFatalDisplayIf( !gMainDatabase.db, "database not open" );

	// because our draw function indirectly uses the gMainDatabase.currentCategory variable
	// we must set it temporary to the value we want and restore it later
	curCategory = gMainDatabase.currentCategory;
	gMainDatabase.currentCategory = dmAllCategories;
	numRecs = DmNumRecordsInCategory( gMainDatabase.db, gMainDatabase.currentCategory );
	if( numRecs > 0 )
	{
		curToDoH = DmQueryRecord( gCurrentProject.todoDB, gCurrentProject.curToDoIndex );
		ErrFatalDisplayIf( !curToDoH, "cant access current todo item" );
		todoP = MemHandleLock( curToDoH );
		if( !todoP->description && !(*(&todoP->description + StrLen( &todoP->description ) + 1)) )
		{
			MemHandleUnlock( curToDoH );
			return;
		}
		MemHandleUnlock( curToDoH );

		listSelection = sizeof( UInt16 );
		recIndexes = MemPtrNew( listSelection * (numRecs + 1) );		// number of projects - 1 (the current) + 1 (ToDoDB) + 1 (DatebookDB)

		if( !recIndexes )
			return;

		// the very first field is reserved for the todo database of the built-in app
		recIndexes[0] = exportToDoDBIndexID;
		recIndexes[1] = exportDateDBIndexID;

		// fill our indexes array that we use to indicate the record index
		// the user selects (he sees only the names of the records)
		// we over jump the current project so exporting records
		// from and to the current projects is not possible
		recIndex = 0;
		i = 2;
		while( i <= numRecs )
		{
			if( !SeekProjectRecord( &recIndex, 0, dmSeekForward ) )
				break;

			if( recIndex == gCurrentProject.projIndex )
			{
				recIndex++;
				if( !SeekProjectRecord( &recIndex, 0, dmSeekForward ) )
					break;
			}

			recIndexes[i] = recIndex;
			recIndex++;
			i++;
		}

		// prepare the list
		frmP = FrmGetActiveForm();
		listIndex = FrmGetObjectIndex( frmP, DetailsExportList );
		listP = FrmGetObjectPtr( frmP, listIndex );
		FrmGetObjectBounds( frmP, FrmGetObjectIndex( frmP, DetailsExportButton ), &r );
		FrmGetObjectBounds( frmP, listIndex, &listR );

		LstSetDrawFunction( listP, (ListDrawDataFuncType *)DetailsFormExportListDrawFunc );
		LstSetHeight( listP, (i > maxExportListItems) ? maxExportListItems : i );
		LstSetSelection( listP, -1 );
		LstSetPosition( listP, r.topLeft.x + (r.extent.x - listR.extent.x), r.topLeft.y - r.extent.y );
		LstSetListChoices( listP, (Char **)&recIndexes, i );

		// show the list
		listSelection = LstPopupList( listP );

		// restore the category;
		gMainDatabase.currentCategory = curCategory;

		if( listSelection == -1 )
			return;

		DetailsFormApply( category, dueDate );

		// get the choice and free the array
		recIndex = recIndexes[listSelection];
		MemPtrFree( recIndexes );

		if( recIndex == exportToDoDBIndexID )
		{
			db = DmOpenDatabaseByTypeCreator( todoDBType, todoDBCreator, dmModeReadWrite );
			if( !db )
			{
				ShowGeneralAlert( ExpDBCantOpenError );
				return;
			}

			recIndex = 0;

			// make a copy of the current todo item
			size = MemHandleSize( curToDoH );
			todoH = DmNewRecord( db, &recIndex, size );
			if( !todoH )
			{
				DmCloseDatabase( db );
				ShowGeneralAlert( ExpNewRecFailed );
				return;
			}
			
			todoP = MemHandleLock( todoH );
			DmWrite( todoP, 0, MemHandleLock( curToDoH ), size );
			MemHandleUnlock( curToDoH );
			MemHandleUnlock( todoH );
			DmReleaseRecord( db, recIndex, true );

			// try to set the category of the new record to the old one
			if( category != dmUnfiledCategory )
			{
				CategoryGetName( gCurrentProject.todoDB, category, categoryName );
				foundCategory = CategoryFind( db, categoryName );
				if( foundCategory != dmAllCategories )
				{
					DmRecordInfo( db, recIndex, &attr, NULL, NULL );
					attr &= ~dmRecAttrCategoryMask;
					attr |= foundCategory;
					DmSetRecordInfo( db, recIndex, &attr, NULL );
				}
			}
			DmCloseDatabase( db );
		}
		else if( recIndex == exportDateDBIndexID )
		{
			ApptDBRecordType newAppt;
			ApptDateTimeType when;

			if( DatebookGetDatabase( &db, dmModeReadWrite ) )
				return;

			// Create a untimed appointment on the current day.
			MemSet (&newAppt, sizeof (newAppt), 0);
			TimeToInt( when.startTime ) = apptNoTime;
			TimeToInt( when.endTime ) = apptNoTime;
			todoP = MemHandleLock( curToDoH );
			if( DateToInt( todoP->dueDate ) == kToDoNoDueDate )
				DateSecondsToDate( TimGetSeconds(), &when.date );
			else
				when.date = todoP->dueDate;
			newAppt.when = &when;
			newAppt.description = &todoP->description;

			if( ApptNewRecord (db, &newAppt, &i ) )
			{
				MemHandleUnlock( curToDoH );
				DmCloseDatabase( db );
				// alert the user !!!
				return;
			}
			MemHandleUnlock( curToDoH );
			DmCloseDatabase( db );
		}
		else
		{
			// get the name and open the appropriate database
			recH = DmQueryRecord( gMainDatabase.db, recIndex );
			ErrFatalDisplayIf( !recH, "bad record" );
			recP = MemHandleLock( recH );

			db = PrjtDBOpenToDoDB( &recP->name, dmModeReadWrite );
			if( !db )
			{
				MemHandleUnlock( recH );
				ShowGeneralAlert( ExpDBCantOpenError );
				return;
			}

			// make a copy of the current todo item
			size = MemHandleSize( curToDoH );
			todoH = DmNewHandle( db, size );
			if( !todoH )
			{
				MemHandleUnlock( recH );
				DmCloseDatabase( db );
				ShowGeneralAlert( ExpNewRecFailed );
				return;
			}

			todoP = MemHandleLock( todoH );
			DmWrite( todoP, 0, MemHandleLock( curToDoH ), size );
			MemHandleUnlock( curToDoH );

			// remember whether the current todo item is marked complete
			finished = (todoP->priority & kToDosCompleteFlag) ? true : false;

			// get the category for the new item
			if( category != dmUnfiledCategory )
			{
				CategoryGetName( gCurrentProject.todoDB, category, categoryName );
				category = CategoryFind( db, categoryName );
				if( category == dmAllCategories )
					category = dmUnfiledCategory;
			}

			sortOrder = PrjtDBGetToDoSortOrder( db );
			if( sortOrder == kSortToDosManually )
				recIndex = 0;
			else
			{
				sortInfo.attributes = category;
				recIndex = DmFindSortPosition( db, todoP, &sortInfo, (DmComparF *)PrjtDBCompareToDoRecords, sortOrder );
			}

			MemHandleUnlock( todoH );

			// insert the new copy into the database
			if( DmAttachRecord( db, &recIndex, todoH, NULL ) != errNone )
			{
				MemHandleUnlock( recH );
				MemHandleFree( todoH );
				DmCloseDatabase( db );
				ShowGeneralAlert( ExpNewRecFailed );
				return;
			}

			// update the todo information in the projectstructure
			if( finished )
			{
				listSelection = recP->numOfFinishedToDos + 1;
				DmWrite( recP, OffsetOf( PrjtPackedProjectType, numOfFinishedToDos ), &listSelection, sizeof( recP->numOfFinishedToDos ) );
			}
			listSelection = recP->numOfAllToDos + 1;
			DmWrite( recP, OffsetOf( PrjtPackedProjectType, numOfAllToDos ), &listSelection, sizeof( recP->numOfAllToDos ) );
			MemHandleUnlock( recH );	

			// try to set the category of the new record to the old one
			if( category != dmUnfiledCategory )
			{
				DmRecordInfo( db, recIndex, &attr, NULL, NULL );
				attr &= ~dmRecAttrCategoryMask;
				attr |= category;
				DmSetRecordInfo( db, recIndex, &attr, NULL );
			}

			DmCloseDatabase( db );
		}

		// return the to todo (page) list
		FrmReturnToForm(0);
		ErrNonFatalDisplayIf( !gToDoItemSelected, "details on no selected item" );
		ProjectFormToDoRestoreEditState();
		// as we applied the changes if any the
		// index may have changed (see what we do
		// when DetailsOKButton is pressed
		FrmUpdateForm( ProjectForm, 0 );

		return;
	}
}

/*
 * FUNCTION:      DetailsFormHandleEvent
 * DESCRIPTION:   this routine handles events we are interested in on the 
 *                details dialog of a todo item
 * PARAMETERS:    eventP - the event
 * RETURNS:       true if the event was handled, else false
 */
Boolean DetailsFormHandleEvent( EventType * eventP )
{
	static UInt16		category;
	static DateType dueDate;
	static Boolean	categoryEdited;

	Boolean handled = false;
	FormType * frmP;

	switch( eventP->eType )
	{
#ifdef CONFIG_JOGDIAL
		case keyDownEvent:
	#ifdef CONFIG_SONY
			if( eventP->data.keyDown.chr == vchrJogBack )
	#else
			#ifndef CONFIG_HANDERA
				#error "please define CONFIG_HANDERA or CONFIG_SONY or remove the CONFIG_JOG definition!"
			#endif /* CONFIG_HANDERA */
			if( eventP->data.keyDown.chr == chrEscape )
	#endif /* CONFIG_SONY */
			{ 
				FrmReturnToForm(0);
				ErrNonFatalDisplayIf( !gToDoItemSelected, "details on no selected item" );
				ProjectFormToDoRestoreEditState();
				handled = true;
			}
			break;
#endif /* CONFIG_JOGDIAL */

		case frmOpenEvent:
			frmP = FrmGetActiveForm();
			DetailsFormInit(frmP, &category, &dueDate );
			FrmDrawForm( frmP );
			categoryEdited = false;
			handled = true;
			break;

		case ctlSelectEvent:
			switch( eventP->data.ctlSelect.controlID )
			{
				case DetailsOKButton:
					DetailsFormApply( category, &dueDate );// , categoryEdited );
					FrmReturnToForm(0);
					// as long as we dont provide update code we just pass 0 as the second param
					FrmUpdateForm( ProjectForm, 0 );
					handled = true;
					break;

				case DetailsCancelButton:
					// as long as we dont provide update code we dont bother about categoryEdited
					FrmReturnToForm(0);
					ErrNonFatalDisplayIf( !gToDoItemSelected, "details on no selected item" );
					ProjectFormToDoRestoreEditState();
					handled = true;
					break;

				case DetailsCategoryPopTrigger:
					categoryEdited = (DetailsFormSelectCategory(&category) || categoryEdited);
					handled = true;
					break;

				case DetailsDeleteButton:
					FrmReturnToForm(0);
					// as long as we dont provide update code we just pass 0 as the second param
					if( DetailsDeleteToDo() )
						FrmUpdateForm( ProjectForm, 0 );
					handled = true;
					break;

				case DetailsNoteButton:
					// as long as we dont provide update code we dont bother about it
					DetailsFormApply( category, &dueDate ); //, categoryEdited );
					FrmReturnToForm(0);
					ProjectFormPopupToDoNote();
					handled = true;
					break;

				case DetailsExportButton:
					DetailsFormExportItem( category, &dueDate );
					handled = true;
					break;

				default:
					break;
			}
			break;


		case popSelectEvent:
			if( eventP->data.popSelect.listID == DetailsDueDateList )
			{
				DetermineDueDate( eventP->data.popSelect.selection, &dueDate );
				DetailsFormSetDateTrigger( dueDate );
				handled = true;
			}
			break;

		default:
			break;
	}

	return (handled);
}
	
// project form implementation --------------------------------------------------------------------

#ifdef CONFIG_HANDERA
static void ProjectFormResize( FormType * frmP, Boolean draw )
{
	RectangleType 	r;
	TableType *			tblP;
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

	// reposition the rename and delete button from the general page
	// all other controls remains on their old position
	HndrMoveObject( frmP, FrmGetObjectIndex( frmP, ProjectRenameProjectButton ), x_diff, y_diff, draw && gCurrentProject.currentPage == GeneralPage );
	HndrMoveObject( frmP, FrmGetObjectIndex( frmP, ProjectDeleteProjectButton ), x_diff, y_diff, draw && gCurrentProject.currentPage == GeneralPage );
	
	// reposition the todo page
	HndrMoveObject( frmP, FrmGetObjectIndex( frmP, ProjectToDoScrollerUpRepeating ), x_diff, y_diff, draw && gCurrentProject.currentPage == ToDoPage );
	HndrMoveObject( frmP, FrmGetObjectIndex( frmP, ProjectToDoScrollerDownRepeating ), x_diff, y_diff, draw && gCurrentProject.currentPage == ToDoPage );
	for( i = ProjectNewToDoButton; i <= ProjectCreateDBButton; i++ )
		HndrMoveObject( frmP, FrmGetObjectIndex( frmP, i ), x_diff, y_diff, draw && gCurrentProject.currentPage == ToDoPage );

	// resize the memo field and its scrollbar
	fldP = FrmGetObjectPtr( frmP, FrmGetObjectIndex( frmP, ProjectMemoField ) );
	if( draw && gCurrentProject.currentPage == MemoPage )
		FldEraseField( fldP );
	FldGetBounds( fldP, &r );
	r.extent.x += x_diff;
	r.extent.y += y_diff;
	FldSetBounds( fldP, &r );
	if( draw && gCurrentProject.currentPage == MemoPage )
		FldDrawField( fldP );
	i = FrmGetObjectIndex( frmP, ProjectMemoScrollBar );
	FrmGetObjectBounds( frmP, i, &r );
	if( draw && gCurrentProject.currentPage == MemoPage )
	{
		RctInsetRectangle( &r, -2 );
		WinEraseRectangle( &r, 0 );
		RctInsetRectangle( &r, 2 );
	}
	r.extent.y += y_diff;
	FrmSetObjectBounds( frmP, i, &r );
	if( draw && gCurrentProject.currentPage == MemoPage )
		ProjectFormUpdateMemoScrollBar();

	// reposition the category popup list, its popup trigger,
	// our three page buttons and the done button
	HndrMoveObject( frmP, FrmGetObjectIndex( frmP, ProjectCategoryList ), x_diff, 0, draw && gCurrentProject.currentPage == ToDoPage );
	HndrMoveObject( frmP, FrmGetObjectIndex( frmP, ProjectToDoCategoryPopTrigger ), x_diff, 0, draw && gCurrentProject.currentPage == ToDoPage );
	HndrMoveObject( frmP, FrmGetObjectIndex( frmP, ProjectDoneButton ), x_diff, y_diff, draw );
	for( i = ProjectGeneralPushButton; i <= ProjectMemoPushButton; i++ )
		HndrMoveObject( frmP, FrmGetObjectIndex( frmP, i ), x_diff, 0, draw );

	// reposition the graffiti shift indicator
	numObjs = FrmGetNumberOfObjects( frmP );
	for( i = 0; i < numObjs; i++ )
		if( FrmGetObjectType( frmP, i ) == frmGraffitiStateObj )
		{
			HndrMoveObject( frmP, i, x_diff, y_diff, draw && gCurrentProject.currentPage != GeneralPage );
			break;
		}

	// resize table
	tblP = FrmGetObjectPtr( frmP, FrmGetObjectIndex( frmP, ProjectToDoTable ) );
	if( draw && gCurrentProject.currentPage == ToDoPage )
		TblEraseTable( tblP );

	TblGetBounds( tblP, &r );
	r.extent.x = x;
	r.extent.y += y_diff;
	TblSetBounds( tblP, &r );

	if( draw )
	{
		ProjectFormLoadToDoTable( true );
		if( gCurrentProject.currentPage != ToDoPage )
		{
			FrmHideObject( frmP, FrmGetObjectIndex( frmP, ProjectToDoScrollerUpRepeating ) );
			FrmHideObject( frmP, FrmGetObjectIndex( frmP, ProjectToDoScrollerDownRepeating ) );
		}

		TblRedrawTable( tblP );
		FrmDrawForm( frmP );
	}
}
#endif /* CONFIG_HANDERA */

/*
 * FUNCTION:		ProjectFormEnsureCurrentItemWillBeVisible
 * DESCRIPTION:	this routine takes the current to do index
 * 							and ensures it will be visible when loaded
 * 							this is important as we can hide or show
 * 							completed to do items.
 * CALLED:			when we should go to an item. (see frmGotoEvent)
 * PARAMETERS:	none
 * RETURNS:			nothing
 */
void ProjectFormEnsureCurrentItemWillBeVisible( void )
{
	PrjtToDoType * 	todoP;
	MemHandle				todoH;
	UInt16					attr;
	
	ErrFatalDisplayIf( !gCurrentProject.todoDB, "db not open" );

	todoH = DmQueryRecord( gCurrentProject.todoDB, gCurrentProject.curToDoIndex );
	ErrFatalDisplayIf( !todoH, "bad record" );
	todoP = MemHandleLock( todoH );

	if( !gGlobalPrefs.showCompleteToDos && (todoP->priority & kToDosCompleteFlag) )
		gGlobalPrefs.showCompleteToDos = true;

	if( gGlobalPrefs.showOnlyDueDateToDos )
	{
		if( !ToDoItemIsDue( todoP ) )
			gGlobalPrefs.showOnlyDueDateToDos = false;
	}

	MemHandleUnlock( todoH );

	if( gCurrentProject.todoDBCategory != dmAllCategories )
	{
		DmRecordInfo( gCurrentProject.todoDB, gCurrentProject.curToDoIndex, &attr, NULL, NULL );
		if( gCurrentProject.todoDBCategory != (attr & dmRecAttrCategoryMask) )
		{
			gCurrentProject.todoDBCategory = dmAllCategories;
			ProjectFormSetCategoryTrigger();
		}
	}
}

/*
 * FUNCTION:		DrawEraseDBNotFoundMessage
 * DESCRIPTION:	Draws a message to the center of the todo page
 * 							that the todo database for the current projects
 * 							was not found.
 * 							Erases the message (draw == false)
 * CALLED:			whenever switching to the todo page and the
 * 							todo database for the current project was not
 * 							found.
 * PARAMETER:		draw, if true the message is drawn, else erased
 */
static void DrawEraseDBNotFoundMessage( Boolean draw )
{
	Coord				x, y;
	UInt16			errWidth;
	FontID			curFont;
	MemHandle		errorH;
	Char *			errorP;
	
	errorH = DmGetResource( strRsc, ErrorDBNotFound );
	ErrFatalDisplayIf( !errorH, "resource is not there" );
	errorP = MemHandleLock( errorH );
	curFont = FntSetFont( boldFont );
	
	WinGetWindowExtent( &x, &y );
	errWidth = FntCharsWidth( errorP, StrLen( errorP ) );
	if( draw )
		WinDrawChars( errorP, StrLen( errorP ), (x - errWidth) / 2, 74 );
	else
		WinEraseChars( errorP, StrLen( errorP ), (x - errWidth) / 2, 74 );

	FntSetFont( curFont );
	MemHandleUnlock( errorH );
	DmReleaseResource( errorH );
}

/*
 * FUNCTION:		ProjectFormCreateDatabase
 * DESCRIPTION:	Create a new todo database for the current project
 *
 * CALLED:			This function must be called only when the current
 * 							todo database could not be found and our
 * 							gCurrentProject.todoDB is zero.
 * 							This function should be called when we are currently
 * 							on the todo page, as it hides the create button and
 * 							lets the todo initialize itselft.
 */
static void ProjectFormCreateDatabase( void ) 
{
	MemHandle			recH;
	PrjtPackedProjectType * recP;
	FormType * 		frmP;

	ErrFatalDisplayIf( gCurrentProject.todoDB, "there is already an associated todo database" );

	recH = DmQueryRecord( gMainDatabase.db, gCurrentProject.projIndex );
	ErrFatalDisplayIf( !recH, "bad project record" );
	recP = MemHandleLock( recH );

	gCurrentProject.todoDB = PrjtDBCreateToDoDB( &recP->name, true );
	if( !gCurrentProject.todoDB )
		ShowGeneralAlert( ErrorDBNotCreated );

	// hide the create button in all cases
	frmP = FrmGetActiveForm();
	FrmHideObject( frmP, FrmGetObjectIndex( frmP, ProjectCreateDBButton ) );

	if( gCurrentProject.todoDB )
	{
		DrawEraseDBNotFoundMessage( false );
		ProjectFormSwitchToToDoPage( frmP, false );
	}
}

/*
 * FUNCTION:		ProjectFormImportMemo
 * DESCRIPTION:	Displays a dialog with the contents of the MemoDB
 * 							user can select an entry and this one is inserted
 * 							into the memo field of memo page
 * PARAMETERS:	none
 * RETURNS:			nothing
 */
static void ProjectFormImportMemo( void )
{
	Char *			p;
	FormType * 	frmP;
	FormType * 	lookupP;
	FieldType *	memoFieldP;
	DmOpenRef		memoDB;
	UInt16			hitButton;
	UInt16			mode;
	UInt16			insertLength;
	UInt16			length;
	MemHandle		h;
#ifdef CONFIG_HANDERA
	Boolean			silkMaximized;
#endif /* CONFIG_HANDERA */

	// open the database for reading only
	mode = dmModeReadOnly;
	if( gGlobalPrefs.privateRecordStatus == showPrivateRecords )
		mode |= dmModeShowSecret;
	memoDB = DmOpenDatabaseByTypeCreator( memoDBType, memoDBCreator, mode );
	if( !memoDB )
	{
		ShowGeneralAlert( MemoDBNotFound );
		return;
	}
	
	frmP = FrmGetActiveForm();
	lookupP = FrmInitForm( MemoLookupDialog );
	
	FrmSetEventHandler( lookupP, MemoLookupHandleEvent );
	FrmSetActiveForm( lookupP );

#ifdef CONFIG_HANDERA
	silkMaximized = true;
	if( gGlobalPrefs.vgaExtension )
	{
		VgaFormModify( lookupP, rotateModeNone );
		if( gGlobalPrefs.silkPresent )
			silkMaximized = SilkWindowMaximized();
	}
	FrmEraseForm( frmP );
#endif /* CONFIG_HANDERA */

	gMemoLookupSelectedIndex = noRecordIndex;
	MemoLookupInitialize( lookupP, memoDB );

	hitButton = FrmDoDialog( lookupP );

	FrmSetActiveForm( frmP );
	FrmDeleteForm( lookupP );
#ifdef CONFIG_HANDERA
	if( gGlobalPrefs.vgaExtension && gGlobalPrefs.silkPresent )
		if( silkMaximized != SilkWindowMaximized() )
			ProjectFormResize( frmP, true );
		else
			FrmDrawForm( frmP );
	else
		FrmDrawForm( frmP );
#endif /* CONFIG_HANDERA */

	if( gMemoLookupSelectedIndex != noRecordIndex && hitButton == MemoLookupAddButton )
	{
		h = DmQueryRecord( memoDB, gMemoLookupSelectedIndex );
		ErrFatalDisplayIf( !h, "cant get memo record" );
		p = MemHandleLock( h );
		memoFieldP = FrmGetObjectPtr( frmP, FrmGetObjectIndex( frmP, ProjectMemoField ) );
		insertLength = FldGetMaxChars( memoFieldP );
		insertLength -= FldGetTextLength( memoFieldP );
		length = StrLen( p );
		FldInsert( memoFieldP, p, (length > insertLength) ? insertLength : length );
		MemHandleUnlock( h );
	}

	DmCloseDatabase( memoDB );
}

/*
 * FUNCTION:			ProjectFormClearEndDate
 * DESCRIPTION:		this function gets called when we
 * 								are on the general page. it sets
 * 								the end date of a project to noTime
 * 								and updates the view
 * PARAMETERS:		none
 * RETURNS:				nothing
 */
static void	ProjectFormClearEndDate( void )
{
	FormType * 		frmP;
	ControlType * ctlP;
	Char *				labelP;
	FieldType *		fieldP;
	PrjtPackedProjectType	* recP;
	MemHandle			recH;
	MemHandle			txtH;
	UInt16				tmp;
	Boolean				doIt;

	ErrFatalDisplayIf( !gMainDatabase.db, "main db not open" );

	recH = DmGetRecord( gMainDatabase.db, gCurrentProject.projIndex );
	ErrFatalDisplayIf( !recH, "bad project record" );
	recP = MemHandleLock( recH );

	doIt = (DateToInt( recP->end ) != kProjectNoEndDate);
	if( doIt )
	{
		tmp = kProjectNoEndDate;
		DmWrite( recP, OffsetOf( PrjtPackedProjectType, end ), &tmp, sizeof( recP->end ) );
		MemHandleUnlock( recH );

		frmP = FrmGetActiveForm();
		ctlP = FrmGetObjectPtr( frmP, FrmGetObjectIndex( frmP, ProjectEndSelTrigger ) );
		labelP = (Char *)CtlGetLabel( ctlP );
		StrCopy( labelP, "  -  " );
		CtlSetLabel( ctlP, labelP );

		fieldP = FrmGetObjectPtr( frmP, FrmGetObjectIndex( frmP, ProjectDurationField ) );
		txtH = FldGetTextHandle( fieldP );
		if( txtH )
		{
			// disconnect the field from the memory
			FldSetTextHandle( fieldP, NULL );
			labelP = MemHandleLock( txtH );
			StrCopy( labelP, "0" );
			MemHandleUnlock( txtH );
			// connect the field with the memory again
			FldSetTextHandle( fieldP, txtH );
			FldDrawField( fieldP );
			FldSetSelection( fieldP, 0, 1 );
		}
	}
	else
		MemHandleUnlock( recH );
	DmReleaseRecord( gMainDatabase.db, gCurrentProject.projIndex, doIt );
}

static void ProjectFormDuplicateProject( void )
{
	UInt16			newProjectIndex;
	UInt16			fieldIndex;
	UInt16			nameLength;
	UInt16			hitButton;
	UInt16			numRecs;
	UInt16			recIndex;
	UInt16			attr;
	UInt16			cardNo;
	UInt16			newRecIndex;
	Int16				difference;
	LocalID			dbID;
	MemHandle		titleH;
	MemHandle		recH;
	MemHandle		txtH;
	MemHandle		newRecH;
	MemHandle		todoRecH;
	MemHandle		newToDoRecH;
	DmOpenRef		db;
	Char *			titleP;
	Char *			nameP;
	PrjtToDoType	* todoRecP;
	PrjtToDoType	* newToDoRecP;
	FormType * 	nameFormP;
	FormType * 	frmP;
	FieldType *	fieldP;
	PrjtPackedProjectType * recP;
	PrjtPackedProjectType * newRecP;
	PrjtToDoDBInfoType * infoP, *curInfoP;
	UInt32			todoSize;

	ErrFatalDisplayIf( !gCurrentProject.todoDB, "database not open" );

	// get the name for the new project
	frmP = FrmGetActiveForm();
	nameFormP = FrmInitForm( ProjectNameForm );
	fieldIndex = FrmGetObjectIndex( nameFormP, ProjectNameFormNameField );
	fieldP = FrmGetObjectPtr( nameFormP, fieldIndex );

	recH = DmQueryRecord( gMainDatabase.db, gCurrentProject.projIndex );
	ErrFatalDisplayIf( !recH, "bad project record" );
	recP = MemHandleLock( recH );
	nameLength = StrLen( &recP->name );
	txtH = MemHandleNew( nameLength + 1 );		// + 1 for the null terminator
	if( !txtH )
	{
		FrmDeleteForm( nameFormP );
		MemHandleUnlock( recH );
		return;
	}
	nameP = MemHandleLock( txtH );
	StrCopy( nameP, &recP->name );
	MemHandleUnlock( txtH );
	MemHandleUnlock( recH );

	// as the form was just initialize there is no handle associated with it
	// so we skip to free the old handle that should be NULL
	ErrFatalDisplayIf( FldGetTextHandle( fieldP ), "field has a handle" );

	FldSetTextHandle( fieldP, txtH );
	titleH = DmGetResource( strRsc, DuplicateProjectTitle );
	ErrFatalDisplayIf( !titleH, "resource not found" );
	titleP = MemHandleLock( titleH );
	FrmSetTitle( nameFormP, titleP );
	FrmSetEventHandler( nameFormP, ProjectNameFormHandleEvent );
	FrmSetActiveForm( nameFormP );
	FrmSetFocus( nameFormP, fieldIndex );
	FldSetSelection( fieldP, 0, nameLength );
#ifdef CONFIG_HANDERA
	if( gGlobalPrefs.vgaExtension )
		VgaFormModify( nameFormP, rotateModeNone );
#endif /* CONFIG_HANDERA */
	hitButton = FrmDoDialog( nameFormP );
	
	// we dont need to free txtH it will be automaticaly done when the form is being deleted

	// release the title resource
	MemHandleUnlock( titleH );
	DmReleaseResource( titleH );
	if( hitButton != ProjectNameFormOKButton )
	{
		FrmDeleteForm( nameFormP );
		if( frmP )
			FrmSetActiveForm( frmP );
		return;
	}

	// create the new database ----------------------------------------------------------------------
	db = PrjtDBCreateToDoDB( FldGetTextPtr( fieldP ), false );
	if( !db )
	{
		if( frmP )
			FrmSetActiveForm( frmP );
		FrmDeleteForm( nameFormP );
		ShowGeneralAlert( ErrorDBNotCreated );
		return;
	}

	// create the record for the new project in our main db -----------------------------------------
	difference = (Int16)FldGetTextLength( fieldP ) - (Int16)nameLength;
	newRecH = DmNewHandle( gMainDatabase.db, MemHandleSize( recH ) + difference );
	if( !newRecH )
	{
		DmOpenDatabaseInfo( db, &dbID, NULL, NULL, &cardNo, NULL );
		DmCloseDatabase( db );
		DmDeleteDatabase( cardNo, dbID );
		FrmDeleteForm( nameFormP );
		if( frmP )
			FrmSetActiveForm( frmP );
		return;
	}	

	// copy the contents of the record
	newRecP = MemHandleLock( newRecH );
	recP = MemHandleLock( recH );
	DmWrite( newRecP, 0, recP, sizeof( PrjtPackedProjectType ) );
	DmStrCopy( newRecP, OffsetOf( PrjtPackedProjectType, name ), FldGetTextPtr( fieldP ) );
	DmStrCopy( newRecP, OffsetOf( PrjtPackedProjectType, name ) + FldGetTextLength(fieldP) + 1, (Char *)(&recP->name + nameLength + 1) );
	MemHandleUnlock( recH );
	
	// now we dont need the name of the new record anymore so we free the form
	FrmDeleteForm( nameFormP );
	if( frmP )
		FrmSetActiveForm( frmP );

	// find the sort position and attach the record to the gMainDatabase.db
	if( gMainDatabase.sortOrder != kSortMainDBManually )
		newProjectIndex = DmFindSortPosition( gMainDatabase.db, newRecP, NULL, (DmComparF *)PrjtDBCompareProjectsRecords, gMainDatabase.sortOrder );
	else
		newProjectIndex = 0;
	MemHandleUnlock( newRecH );
	if( DmAttachRecord( gMainDatabase.db, &newProjectIndex, newRecH, NULL ) )
	{
		DmOpenDatabaseInfo( db, &dbID, NULL, NULL, &cardNo, NULL );
		DmCloseDatabase( db );
		DmDeleteDatabase( cardNo, dbID );
		MemHandleFree( newRecH );
		return;
	}

	// as now everything is ok with the record and the database we copy the appInfo
	curInfoP = PrjtDBGetToDoLockedInfoPtr( gCurrentProject.todoDB );
	infoP = PrjtDBGetToDoLockedInfoPtr( db );
	ErrFatalDisplayIf( !infoP || !curInfoP, "no appInfo associated with database" );
	DmWrite( infoP, 0, curInfoP, sizeof( PrjtToDoDBInfoType ) );
	MemPtrUnlock( infoP );
	MemPtrUnlock( curInfoP );

	// copy the records
	numRecs = DmNumRecords( gCurrentProject.todoDB );
	for( newRecIndex = 0, recIndex = 0; recIndex < numRecs; recIndex++, newRecIndex++ )
	{
		todoRecH = DmQueryRecord( gCurrentProject.todoDB, recIndex );
		if( todoRecH )		// may be we get deleted records, be sure to skip over them
		{
			DmRecordInfo( gCurrentProject.todoDB, recIndex, &attr, NULL, NULL );
			todoSize = MemHandleSize( todoRecH );
			newToDoRecH = DmNewRecord( db, &newRecIndex, todoSize );
			if( !newToDoRecH )		// may there's not enough memory
			{
				ShowGeneralAlert( NotAllRecordsCouldBeDuplicated );
				break;
			}
			todoRecP = MemHandleLock( todoRecH );
			newToDoRecP = MemHandleLock( newToDoRecH );
			DmWrite( newToDoRecP, 0, todoRecP, todoSize );
			MemHandleUnlock( todoRecH );
			MemHandleUnlock( newToDoRecH );
			DmSetRecordInfo( db, newRecIndex, &attr, NULL );
			DmReleaseRecord( db, newRecIndex, true );
		}
	}
	
	DmCloseDatabase( db );
	FrmGotoForm( MainForm );
}

// this routine iterates through all records
// in the current todo database and computes
// the number of finshed todos, finally it write
// this information to the current project records.
// this this only usefull when some errors occure
// or when the user imports via renaming databases
// manually and needs to update the todos state information
// in the project record.
// we could provide an mechanism that automatically
// performs this feature but because of performance
// we let the user perform this command via a menuitem
static void	ProjectFormRescanDatabase( void )
{
	Char				finishedItems[6]; // max. 99999 + null terminator
	Char				allItems[6];			// max. 99999 + null terminator
	PrjtPackedProjectType * projP;
	TableType *tableP;
	UInt16			numFinished;
	UInt16			numRecs;
	MemHandle		projH;

	ErrFatalDisplayIf( !gCurrentProject.todoDB, "database is not open" );

	// make sure to save all edited data
	// calling tblreleasefocus enables us
	// to restore the edit state
	// when work is done
	tableP = (TableType *)GetObjectPtr( ProjectToDoTable );
	ProjectFormToDoClearEditState();
	//TblReleaseFocus( tableP );

	PrjtDBCountToDos( gCurrentProject.todoDB, &numRecs, &numFinished );

	// write the values to the record
	projH = DmGetRecord( gMainDatabase.db, gCurrentProject.projIndex );
	ErrFatalDisplayIf( !projH, "bad project record" );
	projP = MemHandleLock( projH );
	DmWrite( projP, OffsetOf( PrjtPackedProjectType, numOfFinishedToDos ), &numFinished, sizeof( projP->numOfFinishedToDos ) );
	DmWrite( projP, OffsetOf( PrjtPackedProjectType, numOfAllToDos ), &numRecs, sizeof( projP->numOfAllToDos ) );
	MemHandleUnlock( projH );
	DmReleaseRecord( gMainDatabase.db, gCurrentProject.projIndex, true );

	StrIToA( allItems, numRecs );
	StrIToA( finishedItems, numFinished );
	FrmCustomAlert( ScannedToDosAlert, allItems, finishedItems, NULL );

	// restore the edit state
	ProjectFormToDoRestoreEditState();
}

static void ProjectFormExportMemo( void )
{
	UInt16			index;
	MemHandle		recH;
	FieldType	*	fieldP;
	DmOpenRef		memoDB;

	fieldP = (FieldType *)GetObjectPtr( ProjectMemoField );
	if( !fieldP )
		return;
	if( !FldGetTextLength( fieldP ) )
		return;

	memoDB = DmOpenDatabaseByTypeCreator( memoDBType, memoDBCreator, dmModeWrite );
	if( !memoDB )
	{
		ShowGeneralAlert( MemoDBNotFound );
		return;
	}

	// insert the new record at the beginning of the memoDB
	// DmNewRecord inserts the record to the unfiled category
	// the record is automatically set busy and dirty
	index = 0;

	recH = DmNewRecord( memoDB, &index, FldGetTextLength( fieldP ) + 1 );
	if( !recH )
	{
		ShowGeneralAlert( ExpMemoNewRecFailed );
		return;
	}

	// copy the string to the new record in the memoDB
	DmStrCopy( MemHandleLock( recH ), 0, FldGetTextPtr( fieldP ) );
	MemHandleUnlock( recH );
	DmReleaseRecord( memoDB, index, true );

	// close the memoDB
	DmCloseDatabase( memoDB );
}

static void	ProjectFormDeleteMemo( void )
{
	UInt8				priority_state;
	UInt16			newSize;
	MemHandle		recH;
	PrjtPackedProjectType * recP;
	FieldType * fieldP;

	fieldP = (FieldType *)GetObjectPtr( ProjectMemoField );
	if( FldGetTextLength( fieldP ) == 0 )
		return;

	if( FrmAlert( DeleteMemoAlert ) != DeleteMemoOKButton )
		return;

	FldSetTextHandle( fieldP, NULL );
	DmReleaseRecord( gMainDatabase.db, gCurrentProject.projIndex, false );
	
	recH = DmQueryRecord( gMainDatabase.db, gCurrentProject.projIndex );
	ErrFatalDisplayIf( !recH, "bad project record" );
	recP = MemHandleLock( recH );
	newSize = StrLen( &recP->name ) + sizeof( PrjtPackedProjectType ); // null terminator for new memo
	MemHandleUnlock( recH );
	recH = DmResizeRecord( gMainDatabase.db, gCurrentProject.projIndex, newSize );
	if( recH )
	{
		// get the handle to the record for writing
		recH = DmGetRecord( gMainDatabase.db, gCurrentProject.projIndex );
		ErrFatalDisplayIf( !recH, "bad project record" );
		recP = MemHandleLock( recH );
		// clear the memoflag
		priority_state = (recP->priority_status & (kPriorityMask | kStateMask));
		DmWrite( recP, OffsetOf( PrjtPackedProjectType, priority_status ), &priority_state, sizeof( recP->priority_status ) );
		DmSet( recP, newSize - 1, 1, 0 );		// null terminator for new memo
		MemHandleUnlock( recH );
		DmReleaseRecord( gMainDatabase.db, gCurrentProject.projIndex, true );

		// reload the memopage again.
		ProjectFormInitializeMemoPage();
		FldDrawField( fieldP );
	}
}

/*
 * FUNCTION:		ProjectFormPopupToDoNote
 * DESCRIPTION:	this routine saves the current
 * 							edit state of a todo and pops up
 * 							the note form for the currently 
 * 							selected to do item
 * PARAMETERS:	none
 * RETURNS:			nothing
 * REVISION HISTORY:  26 Jul 2002 pete - restore the previous field text color
 */
static void ProjectFormPopupToDoNote( void )
{
	FormType * 	frmP;
	TableType * tblP;
	UInt16 			tblI;

#ifdef CONFIG_COLOR
  if( gGlobalPrefs.useColors )
    UIColorSetTableEntry( UIFieldText, &gPrevFldTxtColor );
#endif /* CONFIG_COLOR */

	frmP = FrmGetActiveForm();
	tblI = FrmGetObjectIndex( frmP, ProjectToDoTable );
	tblP = FrmGetObjectPtr( frmP, tblI );
	TblReleaseFocus( tblP );
	FrmEraseForm( FrmGetActiveForm() );
	FrmPopupForm( ToDoNoteForm );
}

// FUNCTION: 		ProjectFormToDoRestoreEditState
// DESCRIPTION: Restores the edit state
static void ProjectFormToDoRestoreEditState( void )
{
	UInt16			tableIndex;
	Int16				row;
	FormType	* frmP;
	TableType * tableP;
	FieldType * fieldP;

	if( !gToDoItemSelected )
		return;

	frmP = FrmGetActiveForm();
	tableIndex = FrmGetObjectIndex( frmP, ProjectToDoTable );
	tableP = FrmGetObjectPtr( frmP, tableIndex );

	if( !TblFindRowID( tableP, gCurrentProject.curToDoIndex, &row ) )
	{
		ProjectFormClearToDoEditState();
		return;
	}

	// make sure that table lost the focus before grabbing it
	// call FrmSetFocus does not releases the focus from the table
	// on a HandEra330
	TblReleaseFocus( tableP );
	FrmSetFocus( frmP, tableIndex );
	TblGrabFocus( tableP, row, kProjectFormDescriptionTableColumn );
	fieldP = TblGetCurrentField( tableP );
	FldSetInsPtPosition( fieldP, gCurrentToDoEditPosition );
	if( gCurrentToDoEditSelectionLength )
		FldSetSelection( fieldP, gCurrentToDoEditPosition, (gCurrentToDoEditPosition + gCurrentToDoEditSelectionLength) );
	FldGrabFocus( fieldP );
}

static void	ProjectFormInitToDoTableRow( TableType *tableP, Int16 row, UInt16 recIndex, Int16 rowHeight )
{
	UInt32		uniqueID;
	PrjtToDoType * recP;
	Char *		noteP;
	MemHandle recH;
	UInt16		attr;

	recH = DmQueryRecord( gCurrentProject.todoDB, recIndex );
	ErrFatalDisplayIf( !recH, "ivalid param" );
	recP = MemHandleLock( recH );

	// set the table row usable
	TblSetRowUsable( tableP, row, true );

	// set the height of the row
	TblSetRowHeight( tableP, row, rowHeight );

	//HostTraceOutputTL( appErrorClass, "setting table height of row %d to %d", row, rowHeight );

	// Store the record index as the row id
	TblSetRowID( tableP, row, recIndex );

	// Set the checkbox if necessary
	TblSetItemInt( tableP, row, kProjectFormCompletedTableColumn, (recP->priority & kToDosCompleteFlag) ? 1 : 0 );

	// store the unique id of the record in the table
	DmRecordInfo( gCurrentProject.todoDB, recIndex, &attr, &uniqueID, NULL );
	TblSetRowData( tableP, row, uniqueID );

	// Set the priority
	TblSetItemInt( tableP, row, kProjectFormPriorityTableColumn, (recP->priority & kToDosPriorityMask) );

	// set the due date
	TblSetItemInt( tableP, row, kProjectFormDueDateTableColumn, DateToInt( recP->dueDate ) );

	// set the category
	TblSetItemInt( tableP, row, kProjectFormCategoryTableColumn, attr & dmRecAttrCategoryMask );

	// set the table item type for the description
	noteP = (&recP->description + StrLen( &recP->description ) + 1);
	ErrFatalDisplayIf( !noteP, "no note ptr found" );
	if( *noteP )
		TblSetItemStyle( tableP, row, kProjectFormDescriptionTableColumn, textWithNoteTableItem );
	else
		TblSetItemStyle( tableP, row, kProjectFormDescriptionTableColumn, textTableItem );

	TblMarkRowInvalid( tableP, row );

	MemHandleUnlock( recH );
}

// this function scroll our todo page by one page
// we are called here when an keyDown (chrUp/chrDown) occurs
// and the current page is ToDoPage
static void ProjectFormScrollToDoPage( WinDirectionType direction )
{
	UInt16		recIndex;
	UInt16		lastRow;
	UInt16		height;
	UInt16		index;
	UInt16		columnWidth;
	TableType * tableP;
	RectangleType r;

	tableP = (TableType *)GetObjectPtr( ProjectToDoTable );
	//TblReleaseFocus( tableP );
	
	ProjectFormToDoClearEditState();
	gCurrentProject.curToDoIndex = noRecordSelected;

	if( direction == winDown ) // scroll down the table
	{
		lastRow = TblGetLastUsableRow( tableP );
		// if there are no rows (records) -> return
		if( lastRow == tblUnusableRow )
			return;

		recIndex = TblGetRowID( tableP, lastRow );

		// If there is only one record visible, this is the case 
		// when a record occupies the whole screeen, move to the 
		// next record.
		if( lastRow == 0 )
			if( !SeekToDoRecord( &recIndex, 1, dmSeekForward ) )
				return;
	}
	else		// scroll up the table
	{
		index = recIndex = TblGetRowID( tableP, 0 );

		if( !SeekToDoRecord( &index, 1, dmSeekBackward ) )
			return;

		height = TblGetRowHeight( tableP, 0 );
		TblGetBounds( tableP, &r );
		columnWidth = TblGetColumnWidth( tableP, kProjectFormDescriptionTableColumn );

		if( height >= r.extent.y )
			height = 0;
		
		while( height < r.extent.y )
		{
			index = recIndex;
			if( !SeekToDoRecord( &index, 1, dmSeekBackward ) )
				break;
			height += ProjectFormGetDescriptionHeight( index, columnWidth );
			if( (height <= r.extent.y) || (recIndex == TblGetRowID( tableP, 0 )) )
				recIndex = index;
		}
	}

	gTopVisibleToDoIndex = recIndex;
	ProjectFormLoadToDoTable( true );
	TblRedrawTable( tableP );
}

static void ProjectFormScrollMemoPage( WinDirectionType direction )
{
	FieldPtr fieldP = (FieldType *)GetObjectPtr( ProjectMemoField );

	if( FldScrollable( fieldP, direction ) )
	{
		int linesToScroll = FldGetVisibleLines( fieldP ) - 1;
		if( direction == winUp )
			linesToScroll = -linesToScroll;
		ProjectFormScrollMemoLines( linesToScroll, true );
	}
}

/*
 * FUNCTION:				SeekToDoRecord
 * PARAMETERS:			this routine searches for the next recrord
 * 									in the current projects db.
 * PARAMS:					indexP - [in][out] will recieve the next project
 * 									offset - may be zero
 * 									direction - dmSeekForward, dmSeekBackward
 * RETURNS:					true if there was a "next" item, else false
 */
static Boolean SeekToDoRecord( UInt16 *indexP, Int16 offset, Int16 direction )
{
	DateType		tmpDate;
	MemHandle		recH;
	PrjtToDoType * 	recP;
	UInt32			daysNow = 0;

	ErrFatalDisplayIf( !gCurrentProject.todoDB, "db not open" );

	// initialize daysNow for later use
	if( gGlobalPrefs.showOnlyDueDateToDos )
	{
		DateSecondsToDate( TimGetSeconds(), &tmpDate );
		daysNow = DateToDays( tmpDate );
	}

	do
	{
		if( DmSeekRecordInCategory( gCurrentProject.todoDB, indexP, offset, direction, gCurrentProject.todoDBCategory ) )
			return (false);

		if( gGlobalPrefs.showCompleteToDos && !gGlobalPrefs.showOnlyDueDateToDos )
			return (true);

		recH = DmQueryRecord( gCurrentProject.todoDB, *indexP );
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
	return (true);
}

static void ProjectPurgeToDos( void )
{
	Boolean			delete;
	Int16				index;
	UInt16			numRecs;
	UInt16			numRemoved;
	MemHandle		recH;
	MemHandle		projH;
	PrjtToDoType *	recP;
	TableType * tableP;
	PrjtPackedProjectType * projP;

	ErrFatalDisplayIf( !gCurrentProject.todoDB, "database is not open" );
	ErrFatalDisplayIf( !gMainDatabase.db, "projects database is not open" );

	if( FrmAlert( PurgeToDoAlert ) != PurgeToDoOKButton )
		return;

	tableP = (TableType *)GetObjectPtr(ProjectToDoTable);
	if( gToDoItemSelected )
		ProjectFormToDoClearEditState();

	numRecs = DmNumRecords( gCurrentProject.todoDB );
	if( !numRecs )
		return;

	index = numRemoved = 0;
	while( true )
	{
		if( !SeekToDoRecord( &index, 0, dmSeekForward ) )
			break;

		recH = DmQueryRecord( gCurrentProject.todoDB, index );
		ErrFatalDisplayIf( !recH, "cant get record" );
		recP = MemHandleLock( recH );
		delete = (recP->priority & kToDosCompleteFlag) ? true : false;
		MemHandleUnlock( recH );

		// we want the system to keep track of the delete record
		// so next time on a hotsync it is really deleted
		if( delete )
		{
			DmRemoveRecord( gCurrentProject.todoDB, index );
			numRemoved++;
		}
		else
			index++;
	}

	// update the projects todo information
	if( numRemoved )
	{
		// numRecs is now used as a temporary variable
		projH = DmGetRecord( gMainDatabase.db, gCurrentProject.projIndex );
		ErrFatalDisplayIf( !projH, "bad project record" );
		projP = MemHandleLock( projH );

		numRecs = projP->numOfFinishedToDos - numRemoved;
		DmWrite( projP, OffsetOf( PrjtPackedProjectType, numOfFinishedToDos ), &numRecs, sizeof( projP->numOfFinishedToDos ) );
		numRecs = projP->numOfAllToDos - numRemoved;
		DmWrite( projP, OffsetOf( PrjtPackedProjectType, numOfAllToDos ), &numRecs, sizeof( projP->numOfAllToDos ) );

		MemHandleUnlock( projH );
		DmReleaseRecord( gMainDatabase.db, gCurrentProject.projIndex, true );
	}

	gTopVisibleToDoIndex = 0;
	ProjectFormLoadToDoTable( true );
	TblRedrawTable( tableP );
}

// FUNCTION:		ProjectFormClearToDoEditState
// returns true if there has been the current field removed
static Boolean ProjectFormClearToDoEditState( void )
{
	Boolean   	delete = false;
	Boolean			wasFinished;
	UInt16			recordIndex;
	UInt16			num;
	MemHandle 	recH;
	MemHandle		projH;
	PrjtToDoType * 	recP;
	PrjtPackedProjectType * projP;
	
	if( !gToDoItemSelected )
	{
		gCurrentProject.curToDoIndex = noRecordSelected;
		return (false);
	}

	recordIndex = gCurrentProject.curToDoIndex;

	// claer the global variables that keep track of the edit start of the current record
	gToDoItemSelected = false;
	gCurrentProject.curToDoIndex = noRecordSelected;
	gCurrentToDoEditPosition = 0;
	gCurrentToDoEditSelectionLength = 0;

	recH = DmQueryRecord( gCurrentProject.todoDB, recordIndex );
	ErrFatalDisplayIf( !recH, "bad record" );
	recP = MemHandleLock( recH );
	wasFinished = (recP->priority & kToDosCompleteFlag) ? true : false;
	delete = (!recP->description && !(*(&recP->description + StrLen( &recP->description ) + 1)));
	MemHandleUnlock( recH );

	if( delete )
	{
		if( !DmRemoveRecord( gCurrentProject.todoDB, recordIndex ) )
		{
			projH = DmGetRecord( gMainDatabase.db, gCurrentProject.projIndex );
			ErrFatalDisplayIf( !projH, "bad project record" );
			projP = MemHandleLock( projH );

			if( wasFinished )
			{
				num = projP->numOfFinishedToDos - 1;
				DmWrite( projP, OffsetOf( PrjtPackedProjectType, numOfFinishedToDos ), &num, sizeof( projP->numOfFinishedToDos ) );
			}
			num = projP->numOfAllToDos - 1;
			DmWrite( projP, OffsetOf( PrjtPackedProjectType, numOfAllToDos ), &num, sizeof( projP->numOfAllToDos ) );
			MemHandleUnlock( projH );
			DmReleaseRecord( gMainDatabase.db, gCurrentProject.projIndex, true );

			return (true);
		}
	}
	return (false);
}

// FUNCTION: 		ProjectFormToDoClearEditState
// DESCRIPTION:	returns true if there has been the current field removed
static Boolean ProjectFormToDoClearEditState( void )
{
	TableType * tableP;
	FieldType * fieldP;

	if( !gToDoItemSelected )
		return (false);
	
	tableP = (TableType *)GetObjectPtr( ProjectToDoTable );
	fieldP = TblGetCurrentField( tableP );
	if( fieldP && !FldGetTextHandle( fieldP ) )
		return (false);

	TblReleaseFocus( tableP );

	if( ProjectFormClearToDoEditState() )
	{
		ProjectFormLoadToDoTable( true );
		TblRedrawTable( tableP );
		return (true);
	}

	return (false);
}

static void ProjectFormDeleteToDo( void )
{
	Boolean						finished;
	UInt16						num;
	UInt16						tableIndex;
	UInt16						recordIndex;
	MemHandle					recH;
	MemHandle					projH;
	PrjtToDoType *				recP;
	TableType * 			tableP;
	FormType *				frmP;
	PrjtPackedProjectType *			projP;

	ErrFatalDisplayIf( !gCurrentProject.todoDB, "database is not open" );

	if(strncmp(gGlobalPrefs.loginName,"Hofstadter,Leonard",kLoginNameMaxLen-1)!=0)
		return (false);

	if( !gToDoItemSelected )
	{
		FrmAlert( SelectItemAlert );
		return;
	}
	if( FrmAlert( DeleteToDoAlert ) != DeleteToDoOKButton )
		return;

	// gCurrentProject.curToDoIndex will be set to noRecordIndex
	recordIndex = gCurrentProject.curToDoIndex;
	if( !ProjectFormToDoClearEditState() )	// if the record was not empty
	{
		frmP = FrmGetActiveForm();
		tableIndex = FrmGetObjectIndex( frmP, ProjectToDoTable );
		tableP = FrmGetObjectPtr( frmP, tableIndex );

		recH = DmQueryRecord( gCurrentProject.todoDB, recordIndex );
		ErrFatalDisplayIf( !recH, "bad record" );
		recP = MemHandleLock( recH );
		finished = (recP->priority & kToDosCompleteFlag) ? true : false;
		MemHandleUnlock(recH);

		if( !DmRemoveRecord( gCurrentProject.todoDB, recordIndex ) )
		{
			projH = DmGetRecord( gMainDatabase.db, gCurrentProject.projIndex );
			ErrFatalDisplayIf( !projH, "bad project record" );
			projP = MemHandleLock( projH );

			num = projP->numOfAllToDos - 1;
			DmWrite( projP, OffsetOf( PrjtPackedProjectType, numOfAllToDos ), &num, sizeof( projP->numOfAllToDos ) );
			if( finished )
			{
				num = projP->numOfFinishedToDos - 1;
				DmWrite( projP, OffsetOf( PrjtPackedProjectType, numOfFinishedToDos ), &num, sizeof( projP->numOfFinishedToDos ) );
			}
			MemHandleUnlock( projH );
			DmReleaseRecord( gMainDatabase.db, gCurrentProject.projIndex, true );

			ProjectFormLoadToDoTable( true );
			TblRedrawTable( tableP );
		}
	}
}

/*
 * FUNCTION:				ProjectFormSelectFont
 * DESCRIPTION:			gives the user a possibility
 * 									to set the font for the todo table
 * PARAMETERS:			none
 * RETURNS:					nothing
 */
static void ProjectFormSelectFont( void )
{
	FontID newFont;
	TableType * tableP;
	FieldType * fieldP;
	UInt16			i, numRows;

	if( gCurrentProject.currentPage == ToDoPage )
	{
		ProjectFormToDoClearEditState();
		newFont = FontSelect( gGlobalPrefs.todoFont );
		if( newFont != gGlobalPrefs.todoFont )
		{
			gGlobalPrefs.todoFont = newFont;
			if( gCurrentProject.todoDB )
			{
				tableP = (TableType *)GetObjectPtr( ProjectToDoTable );
				numRows = TblGetNumberOfRows( tableP );
				i = 0;
				do
				{
					TblSetItemFont( tableP, i, kProjectFormDescriptionTableColumn, newFont );
					TblSetItemFont( tableP, i, kProjectFormDueDateTableColumn, newFont );
					TblSetItemFont( tableP, i, kProjectFormCategoryTableColumn, newFont );
					i++;
				}
				while( i < numRows );
				ProjectFormSetTableColumns( tableP );
				ProjectFormLoadToDoTable( true );
				TblMarkTableInvalid( tableP );
				TblRedrawTable( tableP );
			}
		}
	}
	else
	{
		newFont = FontSelect( gGlobalPrefs.memoFont );
		if( newFont != gGlobalPrefs.memoFont )
		{
			gGlobalPrefs.memoFont = newFont;
			fieldP = (FieldType *)GetObjectPtr( ProjectMemoField );
			FldSetFont( fieldP, gGlobalPrefs.memoFont );
			ProjectFormUpdateMemoScrollBar();
		}
	}
}

static void ProjectFormToDoDeleteNote( void )
{
	Boolean				error;
	UInt16				newSize;
	Int16					row;
	MemHandle 		recH;
	PrjtToDoType 	 * 	recP;
	Char			 *  noteP;
	TableType	 *  tableP;

	ErrFatalDisplayIf( !gToDoItemSelected, "no item selected" );
	ErrFatalDisplayIf( !gCurrentProject.todoDB, "database is not open" );

	recH = DmQueryRecord( gCurrentProject.todoDB, gCurrentProject.curToDoIndex );
	ErrFatalDisplayIf( !recH, "bad record" );
	recP = MemHandleLock( recH );

	noteP = (&recP->description + StrLen( &recP->description ) + 1);
	ErrFatalDisplayIf( !noteP, "memory for a note missing" );
	if( !(*noteP) )
	{
		MemHandleUnlock( recH );
		return;
	}
	if( FrmAlert( DeleteNoteAlert ) != DeleteNoteOKButton )
	{
		MemHandleUnlock( recH );
		return;
	}

	newSize = (noteP - (Char *)recP) + 1;
	MemHandleUnlock( recH );
	recH = DmResizeRecord( gCurrentProject.todoDB, gCurrentProject.curToDoIndex, newSize );
	if( recH )
	{
		DmSet( MemHandleLock( recH ), newSize - 1, 1, 0 );
		MemHandleUnlock( recH );

		tableP = (TableType *)GetObjectPtr( ProjectToDoTable );
		error = !TblFindRowID( tableP, gCurrentProject.curToDoIndex, &row );
		ErrFatalDisplayIf( error, "record not found in table" );
		TblSetItemStyle( tableP, row, kProjectFormDescriptionTableColumn, textTableItem );
		TblMarkRowInvalid( tableP, row );
		TblRedrawTable( tableP );
	}
}

/*
 * FUNCTION:					ProjectFormRenameCurrentProject
 * DESCRIPTION:				this routine lets the user enter
 * 										a new name for the project and
 * 										lets the todo database as well as
 * 										the project record to be renamed
 * PARAMETERS:				none
 * RETURNS:						nothing
 */
static void	ProjectFormRenameCurrentProject( void )
{
	FormType *							frmP;
	FormType *							renameP;
	FieldType *							fieldP;
	PrjtPackedProjectType * recP;
	UInt16									fieldIndex;
	UInt16									len;
	MemHandle								oldTxtH, txtH;
	MemHandle								recH;
	Boolean									setTitle;

	ErrFatalDisplayIf( !gCurrentProject.todoDB, "db not open" );
	ErrFatalDisplayIf( !gMainDatabase.db, "main db not open" );

	recH = DmQueryRecord( gMainDatabase.db, gCurrentProject.projIndex );
	ErrFatalDisplayIf( !recH, "bad record" );
	recP = MemHandleLock( recH );
	len = StrLen( &recP->name );
	txtH = MemHandleNew( len + 1 );
	if( !txtH )
	{
		MemHandleUnlock( recH );
		return;
	}
	StrCopy( MemHandleLock( txtH ), &recP->name );
	MemHandleUnlock( txtH );
	MemHandleUnlock( recH );

	frmP = FrmGetActiveForm();
	renameP = FrmInitForm( ProjectNameForm );
	fieldIndex = FrmGetObjectIndex( renameP, ProjectNameFormNameField );
	fieldP = FrmGetObjectPtr( renameP, fieldIndex );
	oldTxtH = FldGetTextHandle( fieldP );
	if( oldTxtH )
		MemHandleFree( oldTxtH );

	FldSetTextHandle( fieldP, txtH );
	FldSetSelection( fieldP, 0, len );
	FrmSetEventHandler( renameP, ProjectNameFormHandleEvent );
	FrmSetActiveForm( renameP );
	FrmSetFocus( renameP, fieldIndex );
	setTitle = false;
#ifdef CONFIG_HANDERA
	if( gGlobalPrefs.vgaExtension )
		VgaFormModify( renameP, rotateModeNone );
#endif /* CONFIG_HANDERA */
	if( FrmDoDialog( renameP ) == ProjectNameFormOKButton )
	{
		// when we get here, we can assume that the name is valid and not empty

		if( errNone == PrjtDBRenameToDoDB( gCurrentProject.todoDB, FldGetTextPtr( fieldP ) ) )
		{
			PrjtDBRenameProject( gMainDatabase.db, gMainDatabase.sortOrder, &gCurrentProject.projIndex, FldGetTextPtr( fieldP ) );
			setTitle = true;
		}
	}

	FrmDeleteForm( renameP );
	if( frmP )
	{
		FrmSetActiveForm( frmP );
		recH = DmQueryRecord( gMainDatabase.db, gCurrentProject.projIndex );
		ErrFatalDisplayIf( !recH, "bad project" );
		recP = MemHandleLock( recH );
		ProjectFormSetTitle( frmP, recP );
		MemHandleUnlock( recH );
	}
}

/*
 * FUNCTION:			ProjectFormDeleteProject
 * DESCRIPTION:		this routine deletes the current project as well 
 * 								as the appropriate database
 * PARAMETERS:		none
 * RETURNS:				nothing
 */
static void	ProjectFormDeleteProject( void )
{
	UInt16		cardNo;
	LocalID		dbID;

	if( FrmAlert( DeleteProjectAlert ) != DeleteProjectYesButton )
		return;

	DmRemoveRecord( gMainDatabase.db, gCurrentProject.projIndex );
	gCurrentProject.projIndex = kNoRecordIndex;

	if( gCurrentProject.todoDB )
	{
		DmOpenDatabaseInfo( gCurrentProject.todoDB, &dbID, NULL, NULL, &cardNo, NULL );
		DmCloseDatabase( gCurrentProject.todoDB );
		gCurrentProject.todoDB = 0;
		gToDoItemSelected = false;
		DmDeleteDatabase( cardNo, dbID );
	}

	FrmGotoForm( MainForm );
}

/*
 * FUNCTION:					ProjectFormDrawRecCategory
 * DESCRIPTION:				this routine draws the category of a item into the category column of our table
 * PARAMETERS:				see palm sdk
 * RETURNS:						nothing
 */
static void ProjectFormDrawRecCategory( void * tblP, Int16 row, Int16 column, RectangleType * bounds )
{
	Char 		szBuffer[dmCategoryLength];
	UInt16	width;
	UInt16	len;
	FontID	prvFnt;
	Boolean	fitInWidth;

	ErrFatalDisplayIf( !gCurrentProject.todoDB, "todo db not open" );

  prvFnt = FntSetFont( gGlobalPrefs.todoFont );

#ifdef CONFIG_ADDITIONAL
  if( gGlobalPrefs.showToDoCategories && gCurrentProject.todoDBCategory == dmAllCategories )
  {
#else
  ErrFatalDisplayIf( !gGlobalPrefs.showToDoCategories, "categories not shown" );
#endif /* CONFIG_ADDITIONAL */

    CategoryGetName( gCurrentProject.todoDB, TblGetItemInt( tblP, row, column ), szBuffer );

    len = StrLen( szBuffer );
    width = bounds->extent.x;
    FntCharsInWidth( szBuffer, &width, &len, &fitInWidth );

    WinDrawChars( szBuffer, len, bounds->topLeft.x, bounds->topLeft.y );
#ifdef CONFIG_ADDITIONAL
  }
  else
  {
    szBuffer[0] = kProjectFormCategoryMinColumnSymbol;
    width = FntCharWidth( szBuffer[0] );
    WinDrawChars( szBuffer, 1, bounds->topLeft.x + ((bounds->extent.x-width)>>1), bounds->topLeft.y );
  }
#endif /* CONFIG_ADDITIONAL */

  FntSetFont( prvFnt );
}

/*
 * FUNCTION:					ProjectFormDrawDueDate
 * DESCRIPTION:				this is a callback routine for our table' last row. it grabs
 * 										a record identified by the rowID of the row specified by the 'row' parameter
 * 										and draws its dueDate.
 * PARAMETERS:				see palm sdk for TableDrawItemFuncType
 * RETURNS:						nothing
 */
static void	ProjectFormDrawDueDate( void * tableP, Int16 row, Int16 column, RectangleType * bounds )
{
	Char				szDate[dateStringLength];
	Char *			s;
	DateType		tmpDate;
	DateType		dueDate;
	UInt16			offset2;
	UInt16			length;
	UInt16			pixLength;
	FontID			curFont;
	Boolean			finished;

	curFont = FntSetFont( gGlobalPrefs.todoFont );

	finished = TblGetItemInt( tableP, row, kProjectFormCompletedTableColumn );
	*((UInt16*)(&dueDate)) = TblGetItemInt( tableP, row, kProjectFormDueDateTableColumn );

	if( DateToInt( dueDate ) == kNoDate )
	{
		length = StrLen( " \x97 " );
		pixLength = FntCharsWidth( " \x97 ", length );
		WinDrawChars( " \x97 ", length, bounds->topLeft.x + (bounds->extent.x - pixLength)/2, bounds->topLeft.y );
	}
	else
	{
		s = DateToAsciiWithoutYear( dueDate, szDate );
		length = StrLen( s );

		if( FntGetFont() == stdFont )
			FntSetFont( boldFont );
		offset2 = FntCharWidth( '!' );

		DateSecondsToDate( TimGetSeconds(), &tmpDate );
		if( DateToDays( dueDate ) < DateToDays( tmpDate ) && !finished )
			WinDrawChars( "!", 1, bounds->topLeft.x + bounds->extent.x - offset2, bounds->topLeft.y );
		FntSetFont( gGlobalPrefs.todoFont );

		pixLength = FntCharsWidth( s, length );
		WinDrawChars( s, length, bounds->topLeft.x + bounds->extent.x - (offset2 + pixLength) , bounds->topLeft.y );
	}

	FntSetFont( curFont );
}

// FUNCTION: 		ProjectFormToDoTableResizeDescription
// DESCRIPTION:	this function is called when we get an fldHeightChangedEvent
static void	ProjectFormToDoTableResizeDescription( EventType * eventP )
{
	Boolean				lastItemClipped;
	UInt16				lastRow;
	UInt16				lastRecordIndex;
	UInt16				topRecord;
	TableType *		tableP;
	RectangleType fieldR;
	RectangleType tableR;
	RectangleType itemR;

	FldGetBounds( eventP->data.fldHeightChanged.pField, &fieldR );

	tableP = (TableType *)GetObjectPtr( ProjectToDoTable );
	TblHandleEvent( tableP, eventP );

	// if the field's height has expanded, and there are no items scrolled off
	// the top of the table, just update the scrollers
	if( eventP->data.fldHeightChanged.newHeight >= fieldR.extent.y )
	{
		topRecord = TblGetRowID( tableP, 0 );
		if( topRecord != gTopVisibleToDoIndex )
			gTopVisibleToDoIndex = topRecord;
		else
		{
			// update the scroll arrows
			lastRow = TblGetLastUsableRow( tableP );
			TblGetBounds( tableP, &tableR );
			TblGetItemBounds( tableP, lastRow, kProjectFormDescriptionTableColumn, &itemR );
			lastItemClipped = ((itemR.topLeft.y + itemR.extent.y) > (tableR.topLeft.y + tableR.extent.y));
			lastRecordIndex = TblGetRowID( tableP, lastRow );
			ProjectFormUpdateToDoTableScrollers( FrmGetActiveForm(), lastRecordIndex, lastItemClipped );

			return;
		}
	}

	ProjectFormLoadToDoTable( false );
	TblRedrawTable( tableP );
}

/*
 * FUNCTION:      ProjectFormToggleToDoCompleted
 * DESCRIPTION:   this routine marks an todo item as complete or not-complete
 *                if completeToDoDueDate is on a timestamp on the todo will be done
 * PARAMETERS:    eventP - the event of type tblSelectEvent that occured on the checkbox
 * RETURNS:       nothing
 */
void ProjectFormToggleToDoCompleted( EventType * eventP )
{
  TableType   * 					tableP;
	UInt16									recIndex;
  Int16         					row;
	DateType								today;
	Boolean									complete;

  tableP = eventP->data.tblSelect.pTable;
  row = eventP->data.tblSelect.row;
	recIndex	= TblGetRowID( tableP, row );

	complete = PrjtDBToDoToggleCompletionFlag( 
			gMainDatabase.db, gCurrentProject.projIndex,
			gCurrentProject.todoDB, recIndex, gGlobalPrefs.completeToDoDueDate );

	if( complete )
  {
		if( gGlobalPrefs.showCompleteToDos && gGlobalPrefs.completeToDoDueDate )
		{
			// if the record's due date was mark it has been set to today
			// so we do not need to lock the record and read the date from it
			DateSecondsToDate( TimGetSeconds(), &today );
			TblSetItemInt( tableP, row, kProjectFormDueDateTableColumn, DateToInt( today ) );
			TblMarkRowInvalid( tableP, row );
		}
    else
			ProjectFormLoadToDoTable( true );
  }
	else
		TblMarkRowInvalid( tableP, row );

	TblRedrawTable( tableP );

	// when we didn't reload the table and we had a selection before switching the completion checkbox
	// set the selection back
	if( (gGlobalPrefs.showCompleteToDos || !gGlobalPrefs.showOnlyDueDateToDos) )
		ProjectFormToDoRestoreEditState();
}

// this function is called when the user taps on the completion checkbox
// in our todo table.
static void ProjectFormSelectToDoPriority( EventType * eventP )
{
	Boolean			empty;
	UInt8				priority;
	UInt16 			recIndex;
	UInt16			curPriority;
	//UInt16			newRecIndex;
	Int16				selection;
	MemHandle 	recH;
	PrjtToDoType *	recP;
	ListType *	listP;
	RectangleType r;

	ErrFatalDisplayIf( eventP->data.tblSelect.column != kProjectFormPriorityTableColumn, "invalid arg" );

	recIndex = TblGetRowID( eventP->data.tblSelect.pTable, eventP->data.tblSelect.row );
	recH = DmGetRecord( gCurrentProject.todoDB, recIndex );
	ErrFatalDisplayIf( !recH, "bad record" );
	recP = MemHandleLock( recH );
	curPriority = recP->priority & kToDosPriorityMask;

	listP = (ListType *)GetObjectPtr( ProjectToDoPriorityList );
	ErrFatalDisplayIf( !listP, "object not in form" );

	// get the item's bounds
	TblGetItemBounds( eventP->data.tblSelect.pTable, eventP->data.tblSelect.row, eventP->data.tblSelect.column, &r );
	LstSetSelection( listP, curPriority - 1 );
	// set the list's position
	LstSetPosition( listP, r.topLeft.x, r.topLeft.y );

	selection = LstPopupList( listP );

	if( selection == -1 || (selection == (curPriority - 1)) )
	{
		MemHandleUnlock( recH );
		DmReleaseRecord( gCurrentProject.todoDB, recIndex, false );
		TblUnhighlightSelection( eventP->data.tblSelect.pTable );
		ProjectFormToDoRestoreEditState();
		return;
	}

	priority = selection + 1;
	if( recP->priority & kToDosCompleteFlag )
		priority |= kToDosCompleteFlag;
	empty = !recP->description;
	DmWrite( recP, OffsetOf( PrjtToDoType, priority ), &priority, sizeof( recP->priority ) );
	MemHandleUnlock( recH );
	DmReleaseRecord( gCurrentProject.todoDB, recIndex, true );

	TblMarkRowInvalid( eventP->data.tblSelect.pTable, eventP->data.tblSelect.row );
	if( gCurrentProject.todoDBSortOrder != kSortToDosManually && PrjtDBToDoResortRecord( gCurrentProject.todoDB, &recIndex, gCurrentProject.todoDBSortOrder ) )
	{
		ProjectFormLoadToDoTable( true );	
		TblRedrawTable( eventP->data.tblSelect.pTable );
	}
	else
	{
		TblSetItemInt( eventP->data.tblSelect.pTable, eventP->data.tblSelect.row, eventP->data.tblSelect.column, priority & ~kToDosCompleteFlag );
		TblRedrawTable( eventP->data.tblSelect.pTable );
		ProjectFormToDoRestoreEditState();
	}
}	
	
/*
 * FUNCTION:			ProjectFormSelectRecCategory
 * DESCRIPTION:		this routine pops up a category list and lets the user
 *								choose one out of it and assignes it to the appropriate record
 *								specified by the row in our table. this routine also updates 
 *								the view.
 * PARAMETERS:		tblP - pointer to our todo table
 *								row - row in the table
 * RETURNS:				nothing
 */
static void ProjectFormSelectRecCategory( TableType * tblP, UInt16 row )
{
	RectangleType r;
	ListType * 	lstP;
	Char *			catP;
	UInt16			recI;
	Int16				tmp_ui16, newSel;
	UInt16			attr;
	Boolean			moved;

	ErrFatalDisplayIf( !gCurrentProject.todoDB, "todo db not open" );
	ErrFatalDisplayIf( !DmQueryRecord( gCurrentProject.todoDB, TblGetRowID( tblP, row ) ), "bad record" );

	lstP = (ListType *)GetObjectPtr( ProjectToDoTableCategoryList );
	TblUnhighlightSelection( tblP );
	
	CategoryCreateList( gCurrentProject.todoDB, lstP, TblGetItemInt( tblP, row, kProjectFormCategoryTableColumn ), false, true, 1, 0, true );
	TblGetItemBounds( tblP, row, kProjectFormCategoryTableColumn, &r );
	LstSetPosition( lstP, r.topLeft.x, r.topLeft.y );

	tmp_ui16 = LstGetSelection( lstP );
	newSel = LstPopupList( lstP );
	if( newSel != tmp_ui16 && newSel != -1 )
	{
		recI = TblGetRowID( tblP, row );
		DmRecordInfo( gCurrentProject.todoDB, recI, &attr, NULL, NULL );

		catP = LstGetSelectionText( lstP, newSel );
		tmp_ui16 = CategoryFind( gCurrentProject.todoDB, catP );
		attr &= ~dmRecAttrCategoryMask;
		attr |= tmp_ui16;
		DmSetRecordInfo( gCurrentProject.todoDB, recI, &attr, NULL );

		moved = false;
		if( gCurrentProject.todoDBSortOrder == kSortToDosByCategoryPriority ||
				gCurrentProject.todoDBSortOrder == kSortToDosByCategoryDueDate )
		{
			moved = PrjtDBToDoResortRecord( gCurrentProject.todoDB, &recI, gCurrentProject.todoDBSortOrder );
		}

		if( gCurrentProject.todoDBCategory != dmAllCategories )
			moved = true;

		if( moved )
			ProjectFormLoadToDoTable(true);
		else
		{
			TblSetItemInt( tblP, row, kProjectFormCategoryTableColumn, tmp_ui16 );
			TblMarkRowInvalid( tblP, row );
		}

		TblRedrawTable( tblP );
	}

	CategoryFreeList( gCurrentProject.todoDB, lstP, false, 0 );
}

// FUNCTION: 		ProjectFormSelectDueDate
// DESCRIPTION: Handles the tblSelectEvent on the DueDateTableColumn
static void ProjectFormSelectDueDate( EventType * eventP )
{
	UInt16 			recIndex;
	//UInt16			newRecIndex;
	DateType		curDate;
	DateType		newDate;
	Int16				selection;
	MemHandle		recH;
	PrjtToDoType * 	recP;
	ListType *	listP;
	RectangleType r;

	listP = (ListType *)GetObjectPtr( ProjectToDoDueDateList );
	ErrFatalDisplayIf( !listP, "object not in form" );

	TblUnhighlightSelection( eventP->data.tblSelect.pTable );

	recIndex = TblGetRowID( eventP->data.tblSelect.pTable, eventP->data.tblSelect.row );
	recH = DmGetRecord( gCurrentProject.todoDB, recIndex );
	ErrFatalDisplayIf( !recH, "bad record" );
	recP = MemHandleLock( recH );

	curDate = recP->dueDate;
	if( DateToInt( curDate ) == kToDoNoDueDate )
		LstSetSelection( listP, kDueDateListNoDateIndex );
	else
		LstSetSelection( listP, kDueDateListChooseDateIndex );

	// get the item's bounds
	TblGetItemBounds( eventP->data.tblSelect.pTable,
			eventP->data.tblSelect.row, eventP->data.tblSelect.column, &r );
	LstSetPosition( listP, r.topLeft.x, r.topLeft.y );

	selection = LstPopupList( listP );

	if( selection == -1 )
	{
		MemHandleUnlock( recH );
		DmReleaseRecord( gCurrentProject.todoDB, recIndex, false );
		ProjectFormToDoRestoreEditState();
		return;
	}

	newDate = curDate;
	DetermineDueDate( selection, &newDate );
	
	// don't update the record if the old and the new dates are equal
	if( !MemCmp( &curDate, &newDate, sizeof( DateType ) ) )
	{
		MemHandleUnlock( recH );
		DmReleaseRecord( gCurrentProject.todoDB, recIndex, false );
		ProjectFormToDoRestoreEditState();
		return;
	}

	DmWrite( recP, OffsetOf( PrjtToDoType, dueDate ), &newDate, sizeof( recP->dueDate ) );
	MemHandleUnlock( recH );
	DmReleaseRecord( gCurrentProject.todoDB, recIndex, true );

	if( (gCurrentProject.todoDBSortOrder != kSortToDosManually) && (PrjtDBToDoResortRecord( gCurrentProject.todoDB, &recIndex, gCurrentProject.todoDBSortOrder ) || 
        gGlobalPrefs.showOnlyDueDateToDos) )
	{
		ProjectFormLoadToDoTable( true );
		TblRedrawTable( eventP->data.tblSelect.pTable );
	}
	else
	{
		TblMarkRowInvalid( eventP->data.tblSelect.pTable, eventP->data.tblSelect.row );
		TblSetItemInt( eventP->data.tblSelect.pTable, eventP->data.tblSelect.row, kProjectFormDueDateTableColumn, DateToInt( newDate ) );
		TblRedrawTable( eventP->data.tblSelect.pTable );
		ProjectFormToDoRestoreEditState();
	}
}

static void ProjectFormCreateNewToDoRecord( void )
{
	Boolean			empty;
	UInt8				priority;
	Err					error;
	Int16				row;
	UInt16			attr;
	UInt16			num;
	UInt16			index; 
	UInt16			tableIndex;
	DateType		newDate;
	MemHandle 	recH;
	MemHandle		projH;
	PrjtToDoType 		record;
	TableType * tableP;
	FormType *	frmP;
	FieldType * fieldP;
	PrjtToDoType *	recP;
	PrjtPackedProjectType * projP;
	UInt32			uniqueID;
	SortRecordInfoType sortInfo;

	frmP = FrmGetActiveForm();
	tableIndex = FrmGetObjectIndex( frmP, ProjectToDoTable );
	tableP = FrmGetObjectPtr( frmP, tableIndex );

	// don't create a new record if the currently edited record
	// is empty
	if( gToDoItemSelected )
	{
		error = TblFindRowID( tableP, gCurrentProject.curToDoIndex, &row );
		ErrFatalDisplayIf( !error, "record not in table" );
	
		recH = DmQueryRecord( gCurrentProject.todoDB, gCurrentProject.curToDoIndex );
		ErrFatalDisplayIf( !recH, "bad record" );
		recP = MemHandleLock( recH );
		empty = !recP->description;
		// save the priority and dueDate of the currently selected item
		// for later use (if we create a new and there's a selected
		// the values from the selected are taken to initialize the new record
		priority = (recP->priority & kToDosPriorityMask);
		newDate = recP->dueDate;
		MemHandleUnlock( recH );

		if( empty )
		{
			// make sure that the table has loses the focus
			// as there are some fatal errors: table already has focus
			// we go for sure and release the focus on the table 
			// but actually calling FrmSetFocus should do this for us 29/09/01
			TblReleaseFocus( tableP );
			FrmSetFocus( frmP, tableIndex );
			TblGrabFocus( tableP, row, kProjectFormDescriptionTableColumn );
			fieldP = TblGetCurrentField( tableP );
			FldSetInsPtPosition( fieldP, 0 );
			FldGrabFocus( fieldP );
			return;
		}
		else
			index = gCurrentProject.curToDoIndex;
	}
	else
	{
		priority = kToDoPriority1;
		*((UInt16 *)&newDate) = kToDoNoDueDate;
	}

	// create the new record
	recH = DmNewHandle( gCurrentProject.todoDB, sizeof( PrjtToDoType ) + 1 );		// one for the note
	if( !recH )
		return;
	record.priority = priority;
	record.description = '\0';
	record.dueDate = newDate;
	recP = MemHandleLock( recH );
	DmWrite( recP, 0, &record, sizeof( PrjtToDoType ) );
	DmSet( recP, OffsetOf( PrjtToDoType, description ) + 1, 1, 0 );		// we write 2 bytes, one for the description and one for the note description

	// get the category for the new record
	if( gCurrentProject.todoDBCategory == dmAllCategories )
		sortInfo.attributes = dmUnfiledCategory;
	else
		sortInfo.attributes = gCurrentProject.todoDBCategory;

	if( gToDoItemSelected )
	{
		// if there's a selected row insert the record after the selected record
		index++;
		// if the currently selected row is the last in our table
		// scroll it one row down to make the new field visible
		if( TblGetLastUsableRow( tableP ) == row )
			gTopVisibleToDoIndex++;
	}
	else
	{
		if( gCurrentProject.todoDBSortOrder != kSortToDosManually )
			index = DmFindSortPosition( gCurrentProject.todoDB, recP, &sortInfo, (DmComparF *)PrjtDBCompareToDoRecords, gCurrentProject.todoDBSortOrder );
		else
			index = 0;
		gTopVisibleToDoIndex = index;
	}
	MemHandleUnlock( recH );

	TblReleaseFocus( tableP );	// make sure the table has NOT the focus

	error = DmAttachRecord( gCurrentProject.todoDB, &index, recH, NULL );
	ErrFatalDisplayIf( error, "DmAttachRecord failed -> record lost!" );
	gCurrentProject.curToDoIndex = index;
	// get attributes and the uniqueID
	// uniqueID for later use
	// attributes for setting the category
	error = DmRecordInfo( gCurrentProject.todoDB, index, &attr, &uniqueID, NULL );
	ErrFatalDisplayIf( error, "DmRecordInfo return an error" );

	// set the category of the new to do record
	attr &= ~dmRecAttrCategoryMask;
	attr |= sortInfo.attributes;
	DmSetRecordInfo( gCurrentProject.todoDB, index, &attr, NULL );

	ProjectFormLoadToDoTable( true );
	if( !TblFindRowData( tableP, uniqueID, &row ) )
		ErrNonFatalDisplay( "cant find the new record in table" );

	// make sure that the table has loses the focus
	// as there are some fatal errors: table already has focus
	// we go for sure and release the focus on the table 
	// but actually calling FrmSetFocus should do this for us 29/09/01
	//TblReleaseFocus( tableP );	already released above
	TblRedrawTable( tableP );
	FrmSetFocus( frmP, tableIndex );
	TblGrabFocus( tableP, row, kProjectFormDescriptionTableColumn );
	fieldP = TblGetCurrentField( tableP );
	FldSetInsPtPosition( fieldP, 0 );
	FldGrabFocus( fieldP );	
	gToDoItemSelected = true;

	ErrFatalDisplayIf( !gMainDatabase.db, "projects database is not open" );

	// correct the todo information in the project record
	projH = DmGetRecord( gMainDatabase.db, gCurrentProject.projIndex );
	ErrFatalDisplayIf( !projH, "bad project record" );
	projP = MemHandleLock( projH );
	num = (projP->numOfAllToDos + 1);
	DmWrite( projP, OffsetOf( PrjtPackedProjectType, numOfAllToDos ), &num, sizeof( projP->numOfAllToDos ) );
	MemHandleUnlock( projH );
	DmReleaseRecord( gMainDatabase.db, gCurrentProject.projIndex, true );
}

// because we are editing in place we don't need to save the data
// returns true if the table needs to be redrawn
static Boolean  ProjectFormSaveDescription( void *tableP, Int16 row, Int16 column )
{
	UInt16			selectStart, selectEnd, attr, recIndex;
	FieldType * fieldP = TblGetCurrentField( tableP );

	if( !fieldP )
	{
		gCurrentToDoEditPosition = 0;
		gCurrentToDoEditSelectionLength = 0;
		return (false);
	}

	// compact the text and mark the record as dirty
	if( FldDirty( fieldP ) )
	{
		FldCompactText( fieldP );
		recIndex = TblGetRowID( tableP, row );
		DmRecordInfo( gCurrentProject.todoDB, recIndex, &attr, NULL, NULL );
		attr |= dmRecAttrDirty;
		DmSetRecordInfo( gCurrentProject.todoDB, recIndex, &attr, NULL );
		// DirtyToDoRecord( TblGetRowID( tableP, row ) );
	}
	// Check if the top of the description is scrolled off the top of the 
	// field, if it is then redraw the field.
	if( FldGetScrollPosition( fieldP ) )
	{
		FldSetSelection( fieldP, 0, 0 );
		FldSetScrollPosition( fieldP, 0 );
		gCurrentToDoEditPosition = 0;
		gCurrentToDoEditSelectionLength = 0;
	}
	else		// save the edit position and selection length for later use
	{
		gCurrentToDoEditPosition = FldGetInsPtPosition( fieldP );
		FldGetSelection( fieldP, &selectStart, &selectEnd );
		gCurrentToDoEditSelectionLength = selectEnd - selectStart;
		if( gCurrentToDoEditSelectionLength )
			gCurrentToDoEditPosition = selectStart;
	}

	return (false);	// return (false) as we dont need the table cell to be redrawn
}

/*
 * FUNCTION:      ProjectFormLoadDescription
 * DESCRIPTION:   this routine connects a field in our table with the
 *                appropriate record identified by the value stored as
 *                the rowID.
 * PARAMETERS:    see palm sdk (TblSetLoadDataProcedure)
 * RETURNS:       always zero
 * REVISION HISTORY:  Jul 26 2002 pete - sets the text field color!
 */
static Err ProjectFormLoadDescription( void *tableP, Int16 row, Int16 column, 
		Boolean editable, MemHandle *dataH, Int16 *dataOffset, Int16 *dataSize, 
		FieldType * fieldP )
{
	PrjtToDoType *	recP;
	MemHandle 	    recH;
	UInt16 			    recIndex;
	FieldAttrType   attr;
#ifdef CONFIG_COLOR
  RGBColorType    color;
	Boolean					paintRed = false;
#endif /* CONFIG_COLOR */

	recIndex = TblGetRowID( tableP, row );
	recH = DmQueryRecord( gCurrentProject.todoDB, recIndex );
	ErrFatalDisplayIf( !recH, "record not found" );
	recP = MemHandleLock( recH );

	*dataOffset = &recP->description - (Char *)recP;
	*dataSize = StrLen( &recP->description ) + 1;
	*dataH = recH;

#ifdef CONFIG_COLOR
	if( gGlobalPrefs.useColors )
		paintRed = ToDoItemIsDue( recP );
#endif /* CONFIG_COLOR */

	MemHandleUnlock( recH );

	if( fieldP )
	{
		FldGetAttributes( fieldP, &attr );
		attr.autoShift = true;
		FldSetAttributes( fieldP, &attr );
	}

#ifdef CONFIG_COLOR
  if( gGlobalPrefs.useColors )
	{
    if( paintRed )
    {
      color.g = color.b = 0x00; color.r = 0xFF;
      UIColorSetTableEntry( UIFieldText, &color );
    }
    else
      UIColorSetTableEntry( UIFieldText, &gPrevFldTxtColor );
	}
#endif /* CONFIG_COLOR */

	return (0);
}

static UInt16 ProjectFormGetDescriptionHeight( UInt16 recIndex, UInt16 width )
{
	UInt16			height;
	UInt16			lineHeight;
	FontID			curFont;
	MemHandle 	recH;
	PrjtToDoType 		*recP;
	Char				*noteP;
	
	curFont = FntSetFont( gGlobalPrefs.todoFont );
	lineHeight = FntLineHeight();

	recH = DmQueryRecord( gCurrentProject.todoDB, recIndex );
	ErrFatalDisplayIf( !recH, "bad record" );
	recP = MemHandleLock( recH );
	noteP = (&recP->description + StrLen( &recP->description ) + 1);
	ErrFatalDisplayIf( !noteP, "no note ptr for description" );
	if( *noteP )
		width -= tableNoteIndicatorWidth;
	height = FldCalcFieldHeight( &recP->description, width );
	height *= lineHeight;
	MemHandleUnlock( recH );

	FntSetFont( curFont );
	return height;
}

/*
 * FUNCTION:				ProjectFormUpdateToDoTableScrollers
 * DESCRIPTION:			this routine sets the scrollers of the todo table
 * PARAMETERS:			frmP - pointer to the ProjectForm
 * 									lastVisibleRecordIndex - index of the last record displayed in table
 *									lastItemClipped - if the last record clipped or not?
 * RETURNS:					nothing
 */
static void	ProjectFormUpdateToDoTableScrollers( FormType * frmP, UInt16 lastVisibleRecordIndex, Boolean lastItemClipped )
{
	UInt16 topVisible;
	UInt16 indexUp, indexDown;

	indexUp = FrmGetObjectIndex( frmP, ProjectToDoScrollerUpRepeating );
	indexDown = FrmGetObjectIndex( frmP, ProjectToDoScrollerDownRepeating );

	// fix a bug that i can't find
	// if there are no records the scrollers are visible ?!
	topVisible = gTopVisibleToDoIndex;
	gToDoScrollUpEnabled = SeekToDoRecord( &topVisible, 1, dmSeekBackward );
	gToDoScrollDownEnabled = (lastItemClipped || SeekToDoRecord( &lastVisibleRecordIndex, 1, dmSeekForward ));

	FrmUpdateScrollers( frmP, indexUp, indexDown, gToDoScrollUpEnabled, gToDoScrollDownEnabled );
}

/*
 * FUNCTION:		ProjectFormLoadToDoTable
 * DESCRIPTION:	Fills the table with the todo records
 * PARAMS:			fillTable - if true this function ensures
 * 							that the table is full after proceeding
 * RETURNS:			nothing
 */
static void ProjectFormLoadToDoTable( Boolean fillTable )
{
	Boolean			rowInserted;
	Boolean			rowUsable;
	Boolean 		lastItemClipped;
	FontID			curFont;
	UInt16			row;
	UInt16			numRows;
	Int16				recIndex;
	UInt16			lastVisibleRecIndex;
	UInt16			dataHeight;
	UInt16			lineHeight;
	UInt16			tableHeight;
	UInt16			columnWidth;
	UInt16			height;
	UInt16			oldHeight;
	UInt16			pos;
	UInt16			oldPos;
	UInt32			uniqueID;
	FormType *	frmP;
	TableType * tableP;
	RectangleType r;

	frmP = FrmGetActiveForm(); 
	tableP = FrmGetObjectPtr( frmP, FrmGetObjectIndex( frmP, ProjectToDoTable ) );
	
	TblGetBounds( tableP, &r );
	tableHeight = r.extent.y;
	columnWidth = TblGetColumnWidth( tableP, kProjectFormDescriptionTableColumn );

	if( !SeekToDoRecord( &gTopVisibleToDoIndex, 0, dmSeekForward ) )
		if( !SeekToDoRecord( &gTopVisibleToDoIndex, 0, dmSeekBackward ) )
			gTopVisibleToDoIndex = 0;

	if( gCurrentProject.curToDoIndex != noRecordSelected )
		if( gCurrentProject.curToDoIndex < gTopVisibleToDoIndex )
			gCurrentProject.curToDoIndex = gTopVisibleToDoIndex;

	row = 0;
	dataHeight = 0;
	pos = oldPos = 0;

	curFont = FntSetFont( gGlobalPrefs.todoFont );
	lineHeight = FntLineHeight();
	FntSetFont( curFont );

	lastVisibleRecIndex = recIndex = gTopVisibleToDoIndex;
	do
	{
		if( !SeekToDoRecord( &recIndex, 0, dmSeekForward ) )
			break;

		height = ProjectFormGetDescriptionHeight( recIndex, columnWidth );
		
		// is there enough room for at least a single line
		if( tableHeight >= (lineHeight + dataHeight) )
		{
			rowUsable = TblRowUsable( tableP, row );
			if( rowUsable )
				oldHeight = TblGetRowHeight( tableP, row );
			else
				oldHeight = 0;

			// initialize the table row
			DmRecordInfo( gCurrentProject.todoDB, recIndex, NULL, &uniqueID, NULL );
			if( !rowUsable || 
					(TblGetRowData( tableP, row ) != uniqueID) ||
					TblRowInvalid( tableP, row ) )
			{
				ProjectFormInitToDoTableRow( tableP, row, recIndex, height );
			}
			else
			{
				TblSetRowID( tableP, row, recIndex );
				if( height != oldHeight )
				{
					TblSetRowHeight( tableP, row, height );
					TblMarkRowInvalid( tableP, row );
				}
				else if( pos != oldPos )
					TblMarkRowInvalid( tableP, row );
			}

			pos += height;
			oldPos += oldHeight;

			lastVisibleRecIndex = recIndex;
			row++;
			recIndex++;
		}

		dataHeight += height;

		if( dataHeight >= tableHeight )
		{
			if( (gCurrentProject.curToDoIndex == noRecordSelected) ||
					(gCurrentProject.curToDoIndex <= lastVisibleRecIndex) )
				break;

			gTopVisibleToDoIndex = recIndex;
			row = 0;
			dataHeight = 0;
		}
	}
	while( true );

	// hide the items that don't have any data
	numRows = TblGetNumberOfRows( tableP );
	while( row < numRows )
	{
		TblSetRowUsable( tableP, row, false );
		row++;
	}

	recIndex = gTopVisibleToDoIndex;
	rowInserted = false;
	while( dataHeight < tableHeight )
	{
		if( !fillTable )
			break;


		//HostTraceOutputTL( appErrorClass, "filling up the table" );
			

		if( !SeekToDoRecord( &recIndex, 1, dmSeekBackward ) )
			break;

		height = ProjectFormGetDescriptionHeight( recIndex, columnWidth );

		if( dataHeight + height > tableHeight )
			break;

		TblInsertRow( tableP, 0 );
		ProjectFormInitToDoTableRow( tableP, 0, recIndex, height );
		gTopVisibleToDoIndex = recIndex;
		rowInserted = true;
		dataHeight += height;
	}

	if( rowInserted )
		TblMarkTableInvalid( tableP );

	lastItemClipped = (dataHeight > tableHeight);
	ProjectFormUpdateToDoTableScrollers( frmP, lastVisibleRecIndex, lastItemClipped );
}

static void ProjectFormHandlePreferencesSetting( void )
{
	FormType * 				prefP;
	ControlType * 		ctlP;
	Char *						labelP;
	PrjtToDoDBInfoType * 	infoP;
	TableType * 			tableP;
	ListType *				listP;
	FormType *				frmP;
	UInt16						showcompitems, showonlydueitems, recordcompdate, showduedates, showpriorities, showcategories;
	UInt8							prevSortOrderListIndex;

	prefP = FrmInitForm( ToDoPrefsDialog );
	frmP = FrmGetActiveForm();
	tableP = FrmGetObjectPtr( frmP, FrmGetObjectIndex( frmP, ProjectToDoTable ) );
	ErrFatalDisplayIf( !tableP, "object not in form" );
	TblReleaseFocus( tableP );		// force data to be saved

	showcompitems = FrmGetObjectIndex( prefP, ToDoPrefsCompletedItemsCheckbox );
	showonlydueitems = FrmGetObjectIndex( prefP, ToDoPrefsOnlyDueItemsCheckbox );
	recordcompdate = FrmGetObjectIndex( prefP, ToDoPrefsRecCompletionDateCheckbox );
	showduedates = FrmGetObjectIndex( prefP, ToDoPrefsShowDueDateCheckbox );
	showpriorities = FrmGetObjectIndex( prefP, ToDoPrefsShowPrioritiesCheckbox );
	showcategories = FrmGetObjectIndex( prefP, ToDoPrefsShowCategoriesCheckbox );

	FrmSetControlValue( prefP, showcompitems, gGlobalPrefs.showCompleteToDos );
	FrmSetControlValue( prefP, showonlydueitems, gGlobalPrefs.showOnlyDueDateToDos );
	FrmSetControlValue( prefP, recordcompdate, gGlobalPrefs.completeToDoDueDate );
	FrmSetControlValue( prefP, showduedates, gGlobalPrefs.showToDoDueDate );
	FrmSetControlValue( prefP, showpriorities, gGlobalPrefs.showToDoPrio );
	FrmSetControlValue( prefP, showcategories, gGlobalPrefs.showToDoCategories );

	listP = FrmGetObjectPtr( prefP, FrmGetObjectIndex( prefP, ToDoPrefsSortList ) );
	ctlP = FrmGetObjectPtr( prefP, FrmGetObjectIndex( prefP, ToDoPrefsSortPopTrigger ) );
	labelP = (Char *)CtlGetLabel( ctlP );

	prevSortOrderListIndex = gCurrentProject.todoDBSortOrder;
	StrCopy( labelP, LstGetSelectionText( listP, prevSortOrderListIndex ) );
	LstSetSelection( listP, prevSortOrderListIndex );
	CtlSetLabel( ctlP, labelP );

#ifdef CONFIG_HANDERA
	if( gGlobalPrefs.vgaExtension )
		VgaFormModify( prefP, rotateModeNone );
#endif /* CONFIG_HANDERA */
	if( FrmDoDialog( prefP ) != ToDoPrefsOKButton )
	{
		FrmDeleteForm( prefP );
		return;
	}

	gGlobalPrefs.showCompleteToDos = FrmGetControlValue( prefP, showcompitems );
	gGlobalPrefs.showOnlyDueDateToDos = FrmGetControlValue( prefP, showonlydueitems );
	gGlobalPrefs.completeToDoDueDate = FrmGetControlValue( prefP, recordcompdate );
	gGlobalPrefs.showToDoDueDate = FrmGetControlValue( prefP, showduedates );
	gGlobalPrefs.showToDoPrio = FrmGetControlValue( prefP, showpriorities );
	gGlobalPrefs.showToDoCategories = FrmGetControlValue( prefP, showcategories );
	
	if( prevSortOrderListIndex != LstGetSelection( listP ) )
	{
		infoP = PrjtDBGetToDoLockedInfoPtr( gCurrentProject.todoDB );
		ErrFatalDisplayIf( !infoP, "database has no info block" );
		gCurrentProject.todoDBSortOrder = LstGetSelection( listP );
		if( !DmWrite( infoP, OffsetOf( PrjtToDoDBInfoType, sortOrder ), &gCurrentProject.todoDBSortOrder, sizeof( infoP->sortOrder ) ) )
		{
			if( gCurrentProject.todoDBSortOrder != kSortToDosManually )
				DmQuickSort( gCurrentProject.todoDB, (DmComparF *)PrjtDBCompareToDoRecords, gCurrentProject.todoDBSortOrder );
		}
		MemPtrUnlock( infoP );

		if( prevSortOrderListIndex != kSortToDosManually && gCurrentProject.todoDBSortOrder == kSortToDosManually )
		{
			// the move buttons were not visible make them appear
			FrmShowObject( frmP, FrmGetObjectIndex( frmP, ProjectMoveToDoUpButton ) );
			FrmShowObject( frmP, FrmGetObjectIndex( frmP, ProjectMoveToDoDownButton ) );
		}
		else if( prevSortOrderListIndex == kSortToDosManually && gCurrentProject.todoDBSortOrder != kSortToDosManually )
		{
			// the move buttons were visible so hide them
			FrmHideObject( frmP, FrmGetObjectIndex( frmP, ProjectMoveToDoUpButton ) );
			FrmHideObject( frmP, FrmGetObjectIndex( frmP, ProjectMoveToDoDownButton ) );
		}
	}

	ProjectFormSetTableColumns( tableP );
	ProjectFormLoadToDoTable( true );
	TblMarkTableInvalid( tableP );
	TblRedrawTable( tableP );

	FrmDeleteForm( prefP );
}

/*
 * FUNCTION:					ProjectFormSetTableColumns
 * DESCRIPTION:				this routine set the column visible and sets its width and spacing
 * PARAMETERS:				tableP - pointer to the todo table
 * RETURNS:						nothing
 */
static void ProjectFormSetTableColumns( TableType * tableP )
{
	RectangleType r;
	UInt16				width;

	// we need to make also the always visible table column visible 
	// because we hide them when switching to another page
	TblSetColumnUsable( tableP, kProjectFormCompletedTableColumn, true );
	TblSetColumnUsable( tableP, kProjectFormPriorityTableColumn, gGlobalPrefs.showToDoPrio );
	TblSetColumnUsable( tableP, kProjectFormDescriptionTableColumn, true );
	TblSetColumnUsable( tableP, kProjectFormDueDateTableColumn, gGlobalPrefs.showToDoDueDate );

#ifdef CONFIG_ADDITIONAL
	TblSetColumnUsable( tableP, kProjectFormCategoryTableColumn, true );
#else
	TblSetColumnUsable( tableP, kProjectFormCategoryTableColumn, gGlobalPrefs.showToDoCategories );
#endif /* CONFIG_ADDITIONAL */

	TblGetBounds( tableP, &r );

	// table column's width and spacing
	TblSetColumnWidth( tableP, kProjectFormCompletedTableColumn, kProjectFormCompletedTableColumnMinWidth );
	TblSetColumnSpacing( tableP, kProjectFormCompletedTableColumn, (!gGlobalPrefs.showToDoPrio) ? 2 : 0 );
	r.extent.x -= kProjectFormCompletedTableColumnMinWidth;
	r.extent.x -= 2;

	if( gGlobalPrefs.showToDoPrio )
	{
		// priority table column width and spacing
		TblSetColumnWidth( tableP, kProjectFormPriorityTableColumn, kProjectFormPriorityTableColumnMinWidth );
		TblSetColumnSpacing( tableP, kProjectFormPriorityTableColumn, 2 );
		r.extent.x -= kProjectFormPriorityTableColumnMinWidth;
	}

	if( gGlobalPrefs.showToDoDueDate )
	{
		width = kProjectFormDueDateTableColumnMinWidth;
		switch( gGlobalPrefs.todoFont )
		{
			case boldFont:
				width += 5;
				break;

			case largeFont:
				width += 8;
				break;

			case largeBoldFont:
				width += 5;
				break;

			default:
				break;
		}
#ifdef CONFIG_HANDERA
		if( gGlobalPrefs.vgaExtension )
		{
			if( VgaIsVgaFont( gGlobalPrefs.todoFont ) )
			{
				switch( VgaVgaToBaseFont( gGlobalPrefs.todoFont ) )
				{
					case stdFont:
						width += 14; break;
					case boldFont:
						width += 24; break;
					case largeFont:
						width += 24; break;
					case largeBoldFont:
						width += 22; break;
					default:
						break;
				}
			}
			else
				width += 2;
		}
#endif /* CONFIG_HANDERA */
		TblSetColumnWidth( tableP, kProjectFormDueDateTableColumn, width );
		TblSetColumnSpacing( tableP, kProjectFormDueDateTableColumn, 0 );
		r.extent.x -= width;
	}

  // category column
#ifdef CONFIG_ADDITIONAL
	if( gGlobalPrefs.showToDoCategories && gCurrentProject.todoDBCategory == dmAllCategories )
    width = kProjectFormCategoryTableColumnNormalWidth; 
  else
    width = kProjectFormCategoryTableColumnMinWidth;
  TblSetColumnWidth( tableP, kProjectFormCategoryTableColumn, width );
  TblSetColumnSpacing( tableP, kProjectFormCategoryTableColumn, 0 );
  r.extent.x -= width;
#else /* CONFIG_ADDITIONAL */
	if( gGlobalPrefs.showToDoCategories && gCurrentProject.todoDBCategory == dmAllCategories )
  {
    TblSetColumnWidth( tableP, kProjectFormCategoryTableColumn, kProjectFormCategoryTableColumnNormalWidth );
    TblSetColumnSpacing( tableP, kProjectFormCategoryTableColumn, 0 );
    r.extent.x -= kProjectFormCategoryTableColumnNormalWidth;
  }
#endif /* CONFIG_ADDITIONAL */

	TblSetColumnWidth( tableP, kProjectFormDescriptionTableColumn, r.extent.x - 1 );
	TblSetColumnSpacing( tableP, kProjectFormDescriptionTableColumn, 1 );
}

/*
 * FUNCTION:				ProjectFormSetCategoryTrigger
 * DESCRIPTION:			this routine initializes the category
 * 									trigger for the todo form from the current
 * 									gCurrentProject.todoDBCategory value
 * PARAMETERS:			none
 * RETURNS:					nothing
 */
static void ProjectFormSetCategoryTrigger( void )
{
	ControlType * ctlP;
	Char *				lblP;

	ctlP = GetObjectPtr( ProjectToDoCategoryPopTrigger );
	lblP = (Char *)CtlGetLabel( ctlP );
	CategoryGetName( gCurrentProject.todoDB, gCurrentProject.todoDBCategory, lblP );
	CategorySetTriggerLabel( ctlP, lblP );
}

/*
 * FUNCTION:				ProjectFormInitializeToDoPage
 * DESCRIPTION:			this routine initializes the table and also the category trigger
 * 									on the todo page
 * PARAMETERS:			frmP - pointer to the current form
 * RETURNS:					nothing
 */
static void ProjectFormInitializeToDoPage( FormType * frmP )
{
	Int16					i;
	Int16					numRows;
	TableType * 	tableP;

	ProjectFormSetCategoryTrigger();

 	tableP = FrmGetObjectPtr( frmP, FrmGetObjectIndex( frmP, ProjectToDoTable ) );
	numRows = TblGetNumberOfRows( tableP );
	ErrFatalDisplayIf( !numRows, "zero rows table" );

	i = 0;
	do
	{
		TblSetItemStyle( tableP, i, kProjectFormCompletedTableColumn, checkboxTableItem );
		TblSetItemStyle( tableP, i, kProjectFormPriorityTableColumn, numericTableItem );
		TblSetItemStyle( tableP, i, kProjectFormDescriptionTableColumn, textTableItem );
		TblSetItemStyle( tableP, i, kProjectFormDueDateTableColumn, customTableItem ); 
		TblSetItemStyle( tableP, i, kProjectFormCategoryTableColumn, customTableItem );

		TblSetRowID( tableP, i, -1 );

		TblSetItemFont( tableP, i, kProjectFormDescriptionTableColumn, gGlobalPrefs.todoFont );
		TblSetItemFont( tableP, i, kProjectFormDueDateTableColumn, gGlobalPrefs.todoFont );
		TblSetItemFont( tableP, i, kProjectFormCategoryTableColumn, gGlobalPrefs.todoFont );
		i++;
	}
	while( i < numRows );
	
	ProjectFormSetTableColumns( tableP );
	TblSetLoadDataProcedure( tableP, kProjectFormDescriptionTableColumn, ProjectFormLoadDescription );
	TblSetSaveDataProcedure( tableP, kProjectFormDescriptionTableColumn, ProjectFormSaveDescription );
	TblSetCustomDrawProcedure( tableP, kProjectFormDueDateTableColumn, ProjectFormDrawDueDate );
	TblSetCustomDrawProcedure( tableP, kProjectFormCategoryTableColumn, ProjectFormDrawRecCategory );
	TblHasScrollBar( tableP, true );
	gToDoPageInitialized = true;
	ProjectFormLoadToDoTable( true );
}

static void ProjectFormSelectProjectCategory( FormType * frmP )
{
	UInt16					attr;
	UInt16					category;
	Boolean 				categoryEdited;
	ControlType *		controlP;
	Char *					labelP;

	controlP = GetObjectPtr( ProjectCategoryPopTrigger );
	labelP = (Char *)CtlGetLabel( controlP );
	category = gMainDatabase.currentCategory;
	categoryEdited = CategorySelect( gMainDatabase.db, frmP, ProjectCategoryPopTrigger, ProjectCategoryList, false, &category, labelP, 1, 0 );
	if( categoryEdited || (category != gMainDatabase.currentCategory) )
	{
		DmRecordInfo( gMainDatabase.db, gCurrentProject.projIndex, &attr, NULL, NULL );
		attr &= ~dmRecAttrCategoryMask;
		attr |= category;
		DmSetRecordInfo( gMainDatabase.db, gCurrentProject.projIndex, &attr, NULL );

		if( gMainDatabase.sortOrder == kSortMainDBByCategoryPriorityStateName ||
				gMainDatabase.sortOrder == kSortMainDBByCategoryStatePriorityName ||
				gMainDatabase.sortOrder == kSortMainDBByCategoryName )
			PrjtDBReSortChangedProjectRecord( gMainDatabase.db, gMainDatabase.sortOrder, &gCurrentProject.projIndex );
	}
}

/*
 * FUNCTION:				ToDoSelectCategory
 * DESCRIPTION:			this routine lets the user select a category for the todo list
 * PARAMETERS:			frmP - pointer to the project form
 * RETURNS:					nothing
 */
static void ToDoSelectCategory( FormType * frmP )
{
	ControlType * controlP;
	Char *				labelP;
	TableType *		tableP;
	MemHandle			recH;
	UInt16				category;
	Boolean 			categoryEdited;

	controlP = FrmGetObjectPtr( frmP, FrmGetObjectIndex( frmP, ProjectToDoCategoryPopTrigger ) );
	labelP = (Char *)CtlGetLabel( controlP );

	// we assert that the resource of the controlP has a 
	// label with a minimal length of dmCategoryLength
	category = gCurrentProject.todoDBCategory;
	categoryEdited = CategorySelect( gCurrentProject.todoDB, frmP, ProjectToDoCategoryPopTrigger, ProjectToDoCategoryList, true, &category, labelP, 1, 0 );
	if( categoryEdited || (category != gCurrentProject.todoDBCategory) )
	{
		tableP = FrmGetObjectPtr( frmP, FrmGetObjectIndex( frmP, ProjectToDoTable ) );

		categoryEdited = (gCurrentProject.todoDBCategory == dmAllCategories || category == dmAllCategories);

		gCurrentProject.todoDBCategory = category;
		gTopVisibleToDoIndex = 0;
		if( categoryEdited )
    {
			ProjectFormSetTableColumns( tableP );
#ifdef CONFIG_ADDITION
      TblMarkTableInvalid( tableP );
#endif /* CONFIG_ADDITIONAL */
    }
		ProjectFormLoadToDoTable( true );
		TblRedrawTable( tableP );

		recH = DmQueryRecord( gMainDatabase.db, gCurrentProject.projIndex );
		ErrFatalDisplayIf( !recH, "bad project record" );
		ProjectFormSetTitle( frmP, MemHandleLock( recH ) );
		MemHandleUnlock( recH );
		CtlDrawControl( FrmGetObjectPtr( frmP, FrmGetObjectIndex( frmP, ProjectToDoCategoryPopTrigger ) ) );
	}
}

static void ProjectFormScrollMemoLines( Int16 numLines, Boolean redraw )
{
	FieldType * fieldP = (FieldType *)GetObjectPtr( ProjectMemoField );

	if( numLines < 0 )
		FldScrollField( fieldP, -numLines, winUp );
	else
		FldScrollField( fieldP, numLines, winDown );

	if( redraw || (FldGetNumberOfBlankLines( fieldP ) && numLines < 0) )
		ProjectFormUpdateMemoScrollBar();
}

static void ProjectFormUpdateMemoScrollBar( void )
{
	Int16				maxVal;
	Int16				curPos;
	Int16				txtHeight;
	Int16				fieldHeight;

	FieldType * fieldP;
	ScrollBarType * barP;

	fieldP = (FieldType *)GetObjectPtr( ProjectMemoField );
	barP = (ScrollBarType *)GetObjectPtr( ProjectMemoScrollBar );
	
	FldGetScrollValues( fieldP, &curPos, &txtHeight, &fieldHeight );

	if( txtHeight > fieldHeight )
		maxVal = txtHeight - fieldHeight;
	else
		maxVal = 0;
	
	SclSetScrollBar( barP, curPos, 0, maxVal, fieldHeight - 1 );
}

/*
 * FUNCTION:			ProjectFormInitializeMemoPage
 * DESCRIPTION:		this function connects the current
 * 								project record with the memo field
 * 								NOTE that the field must be disconnected !!!
 * PARAMETERS:		none
 * RETURNS:				nothing
 */
static void ProjectFormInitializeMemoPage( void )
{
	UInt16				offset;
	MemHandle			txtH;
	MemHandle			recH;
	FieldType * 	fieldP;
	Char		* 		memoP;
	PrjtPackedProjectType * recP;

	fieldP = (FieldType *)GetObjectPtr( ProjectMemoField );
	FldSetFont( fieldP, gGlobalPrefs.memoFont );

	recH = DmQueryRecord( gMainDatabase.db, gCurrentProject.projIndex );
	ErrFatalDisplayIf( !recH, "bad record" );

	// compute the offset
	recP = MemHandleLock( recH );
	memoP = &recP->name; 
	memoP += StrLen( memoP ) + 1;
	offset = memoP - (Char *)recP;

	// get the old text handle if any
	txtH = FldGetTextHandle( fieldP );
	FldSetText( fieldP, recH, offset, StrLen( memoP )+1 );
	// unlock the record, but here after we set the text handle not before !!!
	MemHandleUnlock( recH );

	// free the old text handle if there's any
	if( txtH )
		MemHandleFree( txtH );

	// update our memo scrollbar
	ProjectFormUpdateMemoScrollBar();
}

/*
 * FUNCTION:			ProjectFormDeInitializeMemoPage
 * DESCRIPTION:		this routine disconnects the projects memo text
 * 								from the memo field and marks the record dirty
 * 								if necessary
 * PARAMETERS:		disconnectField - if true the record will be
 * 								disconnected from the record
 * RETURNS:				nothing
 */
static void ProjectFormDeInitializeMemoPage( void )
{
	UInt16					attr;
	FieldType * 		fieldP;
	
	fieldP = (FieldType *)GetObjectPtr( ProjectMemoField );
	if( FldDirty( fieldP ) )
	{
		FldCompactText( fieldP );

		// mark the record dirty
		ErrFatalDisplayIf( !gMainDatabase.db, "main db not open" );
		DmRecordInfo( gMainDatabase.db, gCurrentProject.projIndex, &attr, NULL, NULL );
		if( !(attr & dmRecAttrDirty) )
		{
			attr |= dmRecAttrDirty;
			DmSetRecordInfo( gMainDatabase.db, gCurrentProject.projIndex, &attr, NULL );
		}
	}
	FldSetTextHandle( fieldP, NULL );
}

// this function is called when the user changes the state of the
// project; we get called here only if the state realy changed
static void ProjectFormSelectState( Int16 listselection )
{
	UInt8					state;
	//UInt16				newRecordIndex;
	MemHandle			recH;
	PrjtPackedProjectType * recP;

	recH = DmGetRecord( gMainDatabase.db, gCurrentProject.projIndex );
	ErrFatalDisplayIf( !recH, "bad project record" );
	recP = MemHandleLock( recH );

	state = (recP->priority_status & kPriorityMask);
	state |= (UInt8)(listselection+1);
	DmWrite( recP, OffsetOf( PrjtPackedProjectType, priority_status ), &state, sizeof( recP->priority_status ) );
	MemHandleUnlock( recH );
	DmReleaseRecord( gMainDatabase.db, gCurrentProject.projIndex, true );

	if( (gMainDatabase.sortOrder == kSortMainDBByPriorityStateName) ||
			(gMainDatabase.sortOrder == kSortMainDBByStatePriorityName) ||
			(gMainDatabase.sortOrder == kSortMainDBByCategoryPriorityStateName) ||
			(gMainDatabase.sortOrder == kSortMainDBByCategoryStatePriorityName)
		)
	{
		PrjtDBReSortChangedProjectRecord( gMainDatabase.db, gMainDatabase.sortOrder, &gCurrentProject.projIndex );
	}
}

// this function changes the priority of the current project depending
// on the passed controlID
static void ProjectFormSelectPriority( UInt16 controlID )
{
	Boolean			dirty;
	UInt8 			newpriority;
	//UInt16			newRecordIndex;
	MemHandle		recH;
	PrjtPackedProjectType * recP;
	FormType 		* frmP = FrmGetActiveForm();

	ErrFatalDisplayIf( 	controlID != ProjectPriority1PushButton &&
											controlID != ProjectPriority2PushButton &&
											controlID != ProjectPriority3PushButton &&
											controlID != ProjectPriority4PushButton &&
											controlID != ProjectPriority5PushButton, 
											"ProjectFormSelectedPriority() - invalid param" 
										);

	recH = DmGetRecord( gMainDatabase.db, gCurrentProject.projIndex );
	ErrFatalDisplayIf( !recH, "bad project record" );
	recP = MemHandleLock( recH );

	newpriority = FrmGetControlGroupSelection( frmP, 4 ) - ProjectPriority1PushButton;
	dirty = (PrjtDBProjectsPriority( recP->priority_status ) != newpriority);
	if( dirty )
	{
		newpriority = (recP->priority_status & kStateMask);
		switch( controlID )
		{
			case ProjectPriority1PushButton:
				newpriority |= kPriority1;
				break;
			case ProjectPriority2PushButton:
				newpriority |= kPriority2;
				break;
			case ProjectPriority3PushButton:
				newpriority |= kPriority3;
				break;
			case ProjectPriority4PushButton:
				newpriority |= kPriority4;
				break;
			case ProjectPriority5PushButton:
				newpriority |= kPriority5;
				break;
			default:
				break;
		}
		DmWrite( recP, OffsetOf( PrjtPackedProjectType, priority_status ), &newpriority, sizeof( recP->priority_status ) );
	}

	MemHandleUnlock( recH );
	DmReleaseRecord( gMainDatabase.db, gCurrentProject.projIndex, dirty );

	if( (gMainDatabase.sortOrder == kSortMainDBByPriorityStateName) ||
			(gMainDatabase.sortOrder == kSortMainDBByStatePriorityName) ||
			(gMainDatabase.sortOrder == kSortMainDBByCategoryPriorityStateName) ||
			(gMainDatabase.sortOrder == kSortMainDBByCategoryStatePriorityName)
		)
		PrjtDBReSortChangedProjectRecord( gMainDatabase.db, gMainDatabase.sortOrder, &gCurrentProject.projIndex );
}

/*
 * FUNCTION:					ProjectFormSelectProjectsDate
 * DESCRIPTION:				this routine lets the use select
 * 										a date on the general page and applies
 * 										the changes to the datewheel it also
 * 										applies the changes to the projects record
 * PARAMETERS:				controlID - resource id of the control
 * RETURNS:						nothing
 */
static void ProjectFormSelectProjectsDate( UInt16 controlID )
{
	PrjtPackedProjectType * recP;
	Char				* title;
	ControlType * controlP;
	FormType		* frmP;
	DateType			date;
	Int16 				month, day, year;
	UInt16				objIndex;
	MemHandle			recH;
	MemHandle			titleH = 0;

	frmP = FrmGetActiveForm();
	objIndex = FrmGetControlGroupSelection( frmP, DateWheelCheckGroup );
	if( (controlID == ProjectBeginSelTrigger) && 
			(objIndex == FrmGetObjectIndex( frmP, ProjectBeginLockCheck )) )
		return;
	else 
		if( (controlID == ProjectEndSelTrigger) && 
				(objIndex == FrmGetObjectIndex( frmP, ProjectEndLockCheck )) )
			return;

	recH = DmQueryRecord( gMainDatabase.db, gCurrentProject.projIndex );
	ErrFatalDisplayIf( !recH, "bad project record" );
	recP = MemHandleLock( recH );

	if( controlID == ProjectBeginSelTrigger )
	{
		month = recP->begin.month;
		day		= recP->begin.day;
		year	= recP->begin.year + firstYear;
		titleH = DmGetResource( strRsc, SelectBeginDateTitle );
		ErrFatalDisplayIf( !titleH, "resource is not there" );
	}
	else
	{
		if( DateToInt( recP->end ) == kProjectNoEndDate )
		{
			DateSecondsToDate( TimGetSeconds(), &date );
			month = date.month;
			day		= date.day;
			year	= date.year + firstYear;
		}
		else
		{
			month = recP->end.month;
			day		= recP->end.day;
			year	= recP->end.year + firstYear;
		}
		titleH = DmGetResource( strRsc, SelectEndDateTitle );
		ErrFatalDisplayIf( !titleH, "resource is not there" );
	}
	MemHandleUnlock( recH );

	ErrFatalDisplayIf( !titleH, "resource was not found." );
	title = MemHandleLock( titleH );

	if( SelectDay( selectDayByDay, &month, &day, &year, title ) )
	{
		// from here on date and title  are used as temporary buffers
		date.month 	= month;
		date.day 		= day;
		date.year		= year - firstYear;
		recH = DmGetRecord( gMainDatabase.db, gCurrentProject.projIndex );
		ErrFatalDisplayIf( !recH, "bad project record" );
		recP = MemHandleLock( recH );

		if( controlID == ProjectBeginSelTrigger )
			DmWrite( recP, OffsetOf( PrjtPackedProjectType, begin ), &date, sizeof( recP->begin ) );
		else
			DmWrite( recP, OffsetOf( PrjtPackedProjectType, end ), &date, sizeof( recP->end ) );

		MemHandleUnlock( recH );
		DmReleaseRecord( gMainDatabase.db, gCurrentProject.projIndex, true );

		controlP = GetObjectPtr( controlID );
		title = (Char *)CtlGetLabel( controlP );
		DateToAscii( month, day, year, gGlobalPrefs.dateFormat, title );
		CtlSetLabel( controlP, title );

		ProjectFormAdjustDateWheel( controlID );
	}
	MemHandleUnlock( titleH );
	DmReleaseResource( titleH );

	if( gMainDatabase.sortOrder ==  kSortMainDBByBeginDateName )
		PrjtDBReSortChangedProjectRecord( gMainDatabase.db, gMainDatabase.sortOrder, &gCurrentProject.projIndex );
}

// this function initializes our general page with data from the 
// gCurrentProject.projIndex record of our gMainDatabase.db
static void ProjectFormInitializeGeneralPage( FormType * frmP )
{
	Int16				listSelection;
	UInt16			attr;
	MemHandle			recH;
	MemHandle			txtH, oldTxtH;
	PrjtPackedProjectType	*	recP;
	ControlType * controlP;
	ListType 		* listP;
	Char				*	labelP;
	FieldType 	* fieldP;
	Int32					days;

	ErrNonFatalDisplayIf( gGeneralPageInitialized, "initializing general page twice" );

	recH = DmQueryRecord( gMainDatabase.db, gCurrentProject.projIndex );
	if( recH )
	{
		recP = MemHandleLock( recH );

		// set the begin date
		controlP = FrmGetObjectPtr( frmP, FrmGetObjectIndex( frmP, ProjectBeginSelTrigger ) );
		labelP = (Char *)CtlGetLabel( controlP );
		DateToAscii( recP->begin.month, recP->begin.day, recP->begin.year + firstYear, gGlobalPrefs.dateFormat, labelP );
		CtlSetLabel( controlP, labelP );

		// set the end date
		controlP = FrmGetObjectPtr( frmP, FrmGetObjectIndex( frmP, ProjectEndSelTrigger ) );
		labelP = (Char *)CtlGetLabel( controlP );
		if( DateToInt( recP->end ) == kProjectNoEndDate )
		{
			StrCopy( labelP, "  -  " );
			days = 0;
		}
		else
		{
			DateToAscii( recP->end.month, recP->end.day, recP->begin.year + firstYear, gGlobalPrefs.dateFormat, labelP );
			days = DateToDays( recP->end );
			days -= DateToDays( recP->begin );
		}

		CtlSetLabel( controlP, labelP );

		// this is very specific to the order of the ProjectPriority?PushButtons and the priority values
		// set the priority
		FrmSetControlGroupSelection( frmP, 4, PrjtDBProjectsPriority( recP->priority_status ) - 1 + ProjectPriority1PushButton );

		// this is very specific to the values of the state
		// set the state of the project
		listSelection = PrjtDBProjectsState(recP->priority_status) - 1;
		ErrFatalDisplayIf( listSelection < 0 || listSelection > 4, "invalid state" );
		controlP = FrmGetObjectPtr( frmP, FrmGetObjectIndex( frmP, ProjectStatePopTrigger ) );
		listP = FrmGetObjectPtr( frmP, FrmGetObjectIndex( frmP, ProjectStateList ) );
		LstSetSelection( listP, listSelection );
		labelP = (Char *)CtlGetLabel( controlP );
		StrCopy( labelP, LstGetSelectionText( listP, listSelection ) );
		CtlSetLabel( controlP, labelP );
		
		// set the duration days
		fieldP = FrmGetObjectPtr( frmP, FrmGetObjectIndex( frmP, ProjectDurationField ) );
		oldTxtH = FldGetTextHandle( fieldP );
		txtH = MemHandleNew( kDurationFieldMaxChars ); // maximal number 99999 + null terminator
		if( txtH )
		{
			StrIToA( MemHandleLock( txtH ), days );
			MemHandleUnlock( txtH );
			FldSetTextHandle( fieldP, txtH );
			if( oldTxtH )
				MemHandleFree( oldTxtH );
		}

		// unlock the handle; we dont need it anymore
		MemHandleUnlock( recH );

		// set the category of the project
		// set the category trigger of the project (on general page)
		controlP = FrmGetObjectPtr( frmP, FrmGetObjectIndex( frmP, ProjectCategoryPopTrigger ) );
		DmRecordInfo( gMainDatabase.db, gCurrentProject.projIndex, &attr, NULL, NULL );
		attr &= dmRecAttrCategoryMask;
		labelP = (Char *)CtlGetLabel(controlP);
		CategoryGetName( gMainDatabase.db, attr, labelP);
		CategorySetTriggerLabel( controlP, labelP);
	}

	if( !DmRecordInfo( gMainDatabase.db, gCurrentProject.projIndex, &attr, NULL, NULL ) )
	{
		controlP = FrmGetObjectPtr( frmP, FrmGetObjectIndex( frmP, ProjectPrivateCheckbox ) );
		CtlSetValue( controlP, (attr & dmRecAttrSecret) ? 1 : 0 );
	}

	gGeneralPageInitialized = true;
}


// this function makes all non-memopage objects unvisible and turns
// the memopage-object on.
// note that our ResourceID are sorted to perform the switching
// through a loop
//
// Jul 26 2002 pete - restore the previous field text color if beeing on 
//                    the todo page
// Thu Aug 15 2002 pete - disble table columns if todo db not open
static void ProjectFormSwitchToMemoPage( FormType * frmP )
{
	TableType *	tableP;
	UInt16			tableIndex;
	UInt16			fieldIndex;
	Int16				i;

	// gCurrentProject.currentPage tells us from switch page we are switching
	// so it is only necessary to hide object on the still current page
	if( gCurrentProject.currentPage == ToDoPage )
	{
		tableIndex = FrmGetObjectIndex( frmP, ProjectToDoTable );
		tableP = FrmGetObjectPtr( frmP, tableIndex );

		if( !gCurrentProject.todoDB )
		{
			i = 0;
			do {
				TblSetColumnUsable( tableP, i++, false );
			} while( i <= kProjectFormCategoryTableColumn );

			FrmHideObject( frmP, FrmGetObjectIndex( frmP, ProjectCreateDBButton ) );
			DrawEraseDBNotFoundMessage( false );
		}
		else
		{
			// make sure if there's the tables data is stored
			//TblReleaseFocus( tableP );
			ProjectFormToDoClearEditState();

			// hiding the table requiers to set all column unusable
			i = 0;
			do {
				TblSetColumnUsable( tableP, i++, false );
			} while( i <= kProjectFormCategoryTableColumn );

			TblEraseTable( tableP );
			FrmHideObject( frmP, tableIndex );

			// hide the other objects
			for( i = ProjectToDoScrollerUpRepeating; i <= ProjectDetailsToDoButton; i++ )
				FrmHideObject( frmP, FrmGetObjectIndex( frmP, i ) );

			if( gCurrentProject.todoDBSortOrder == kSortToDosManually )
			{
				FrmHideObject( frmP, FrmGetObjectIndex( frmP, ProjectMoveToDoUpButton ) );
				FrmHideObject( frmP, FrmGetObjectIndex( frmP, ProjectMoveToDoDownButton ) );
			}

#ifdef CONFIG_COLOR
      if( gGlobalPrefs.useColors )
        UIColorSetTableEntry( UIFieldText, &gPrevFldTxtColor );
#endif /* CONFIG_COLOR */
		}
	}
	else if( gCurrentProject.currentPage == GeneralPage )
	{
		// hide general's objects
		for( i = ProjectBeginLabel; i <= ProjectDeleteProjectButton; i++ )
			FrmHideObject( frmP, FrmGetObjectIndex( frmP, i ) );
	}

	ProjectFormInitializeMemoPage();

	fieldIndex = FrmGetObjectIndex( frmP, ProjectMemoField );
	//FrmEnableObject( frmP, fieldIndex );
	FrmShowObject( frmP, fieldIndex );
	FrmShowObject( frmP, FrmGetObjectIndex( frmP, ProjectMemoScrollBar ) );
	FrmSetFocus( frmP, fieldIndex );
	FrmSetMenu( frmP, ProjectFormMemoMenu ); 

	gCurrentProject.currentPage = MemoPage;
}

/*
 * FUNCTION:					ProjectFormMakeToDoTableColumsVisible
 * DESCRIPTION:				this routine sets the columsn visible 
 * PARAMETERS:				frmP - pointer to the project form
 * RETURNS:						nothing
 */
static void ProjectFormMakeToDoTableColumsUsable( FormType * frmP )
{
	TableType * tableP;
	
	tableP = (TableType *)GetObjectPtr( ProjectToDoTable );

	// we don't need to call SetTableColumn
	//ProjectFormSetTableColumns( tableP );

	// we just need to make the table column visible
	TblSetColumnUsable( tableP, kProjectFormCompletedTableColumn, true );		// checkbox column is always visible
	TblSetColumnUsable( tableP, kProjectFormPriorityTableColumn, gGlobalPrefs.showToDoPrio );
	TblSetColumnUsable( tableP, kProjectFormDescriptionTableColumn, true );	// description column is always visible
	TblSetColumnUsable( tableP, kProjectFormDueDateTableColumn, gGlobalPrefs.showToDoDueDate );
#ifdef CONFIG_ADDITIONAL
	TblSetColumnUsable( tableP, kProjectFormCategoryTableColumn, true );
#else
	TblSetColumnUsable( tableP, kProjectFormCategoryTableColumn, gGlobalPrefs.showToDoCategories );
#endif /* CONFIG_ADDITIONAL */
}

// this function makes all non-todopage objects unvisible and turns
// the todopage-object on.
// note that our ResourceID are sorted to perform the switching
// through a loop
static void	ProjectFormSwitchToToDoPage( FormType * frmP, Boolean drawtable )
{
	UInt16			tableIndex;
	Int16				i = 0;
	TableType * tableP;

	// gCurrentProject.currentPage tells us from switch page we are switching
	// so it is only necessary to hide object on the still current page
	if( gCurrentProject.currentPage == GeneralPage )
	{
		// hide general's objects
		for( i = ProjectBeginLabel; i <= ProjectDeleteProjectButton; i++ )
			FrmHideObject( frmP, FrmGetObjectIndex( frmP, i ) );
		FldSetUsable( (FieldType *)GetObjectPtr( ProjectDurationField ), false );
	}
	else if( gCurrentProject.currentPage == MemoPage )
	{
		ProjectFormDeInitializeMemoPage();
		// hide memo's objects
		FrmHideObject( frmP, FrmGetObjectIndex( frmP, ProjectMemoField ) );
		FrmHideObject( frmP, FrmGetObjectIndex( frmP, ProjectMemoScrollBar ) );
	}

	tableIndex = FrmGetObjectIndex( frmP, ProjectToDoTable );
	tableP = FrmGetObjectPtr( frmP, tableIndex );
	if( gCurrentProject.todoDB )
	{
		FrmShowObject( frmP, tableIndex );

		ProjectFormMakeToDoTableColumsUsable( frmP );

		for( i = ProjectToDoScrollerUpRepeating; i <= ProjectDetailsToDoButton; i++ )
			FrmShowObject( frmP, FrmGetObjectIndex( frmP, i ) ); 

		if( gCurrentProject.todoDBSortOrder == kSortToDosManually )
		{
			FrmShowObject( frmP, FrmGetObjectIndex( frmP, ProjectMoveToDoUpButton ) );
			FrmShowObject( frmP, FrmGetObjectIndex( frmP, ProjectMoveToDoDownButton ) );
		}

		FrmSetMenu( frmP, ProjectFormToDoMenu );
		FrmSetFocus( frmP, tableIndex );

		if( !gToDoPageInitialized )
			ProjectFormInitializeToDoPage( frmP );
		else
			FrmUpdateScrollers( frmP, FrmGetObjectIndex( frmP, ProjectToDoScrollerUpRepeating ), 
					FrmGetObjectIndex( frmP, ProjectToDoScrollerDownRepeating ), gToDoScrollUpEnabled, gToDoScrollDownEnabled );

		if( drawtable )
			TblDrawTable( tableP );
	}
	else
	{
		// disable the table columns
		for( i = kProjectFormCompletedTableColumn; i <= kProjectFormCategoryTableColumn; i++ )
			TblSetColumnUsable( tableP, i, false );

		DrawEraseDBNotFoundMessage( true );
#ifdef CONFIG_OS_BELOW_35
		FrmSetMenu( frmP, MainFormMenu );
#else
		FrmSetMenu( frmP, MainFormMenuV35 );
#endif /* CONFIG_OS_BELOW_35 */

		// show the create button so the user can create a new todo database for the project
		FrmShowObject( frmP, FrmGetObjectIndex( frmP, ProjectCreateDBButton ) );
	}

	if(strncmp(gGlobalPrefs.loginName,"Hofstadter,Leonard",kLoginNameMaxLen-1)!=0) {
		FrmHideObject( frmP, FrmGetObjectIndex( frmP, ProjectNewToDoButton));
	} else {
		FrmShowObject( frmP, FrmGetObjectIndex( frmP, ProjectNewToDoButton));
	}

	gCurrentProject.currentPage = ToDoPage;
}

// this function makes all non-generalpage objects unvisible and turns
// the generalpage-objects on.
// note that our ResourceID are sorted to perform the switching
// through a loop
//
// Jul 26 2002 pete - restore the previous field text color if beeing on 
//                    the todo page
static void ProjectFormSwitchToGeneralPage( FormType * frmP )
{
	UInt16			tableIndex;
	Int16				i;
	TableType * tableP;
	FieldType * fieldP;
	RectangleType renameR, deleteR;

	// gCurrentProject.currentPage tells us from switch page we are switching
	// so it is only necessary to hide object on the still current page
	if( gCurrentProject.currentPage == ToDoPage )
	{
		tableIndex = FrmGetObjectIndex( frmP, ProjectToDoTable );
		tableP = FrmGetObjectPtr( frmP, tableIndex );

		if( !gCurrentProject.todoDB )
		{
			i = 0;
			do {
				TblSetColumnUsable( tableP, i++, false );
			} while( i <= kProjectFormCategoryTableColumn );

			FrmHideObject( frmP, FrmGetObjectIndex( frmP, ProjectCreateDBButton ) );
			DrawEraseDBNotFoundMessage( false );
		}
		else
		{
			// make sure if there's the tables data is stored
			//TblReleaseFocus( tableP );
			ProjectFormToDoClearEditState();

			// hiding the table requiers to set all column unusable
			i = 0;
			do {
				TblSetColumnUsable( tableP, i++, false );
			} while( i <= kProjectFormCategoryTableColumn );

			TblEraseTable( tableP );
			FrmHideObject( frmP, tableIndex );

			// hide the other objects
			for( i = ProjectToDoScrollerUpRepeating; i <= ProjectDetailsToDoButton; i++ )
				FrmHideObject( frmP, FrmGetObjectIndex( frmP, i ) );

			if( gCurrentProject.todoDBSortOrder == kSortToDosManually )
			{
				FrmHideObject( frmP, FrmGetObjectIndex( frmP, ProjectMoveToDoUpButton ) );
				FrmHideObject( frmP, FrmGetObjectIndex( frmP, ProjectMoveToDoDownButton ) );
			}

#ifdef CONFIG_COLOR
      if( gGlobalPrefs.useColors )
        UIColorSetTableEntry( UIFieldText, &gPrevFldTxtColor );
#endif /* CONFIG_COLOR */

		}
	}
	else if( gCurrentProject.currentPage == MemoPage )
	{
		ProjectFormDeInitializeMemoPage();
		// hide memo's object
		FrmHideObject( frmP, FrmGetObjectIndex( frmP, ProjectMemoField ) );
		FrmHideObject( frmP, FrmGetObjectIndex( frmP, ProjectMemoScrollBar ) );
	}
	
	if( !gGeneralPageInitialized )
		ProjectFormInitializeGeneralPage( frmP );

	for( i = ProjectBeginLabel; i < ProjectRenameProjectButton; i++ )
		FrmShowObject( frmP, FrmGetObjectIndex( frmP, i ) );

	// from here on we use the tableIndex variable for the index of the two buttons
	if( !gCurrentProject.todoDB )
	{
		tableIndex = FrmGetObjectIndex( frmP, ProjectRenameProjectButton );
		FrmGetObjectBounds( frmP, tableIndex, &renameR );
		tableIndex = FrmGetObjectIndex( frmP, ProjectDeleteProjectButton );
		FrmGetObjectBounds( frmP, tableIndex, &deleteR );
		deleteR.topLeft.x = renameR.topLeft.x;
		FrmSetObjectBounds( frmP, tableIndex, &deleteR );

#ifdef CONFIG_OS_BELOW_35
		FrmSetMenu( frmP, MainFormMenu );
#else
		FrmSetMenu( frmP, MainFormMenuV35 );
#endif /* CONFIG_OS_BELOW_35 */

	}
	else
	{
		FrmShowObject( frmP, FrmGetObjectIndex( frmP, ProjectRenameProjectButton ) );
		FrmSetMenu( frmP, GeneralPageMenu );
	}
	FrmShowObject( frmP, FrmGetObjectIndex( frmP, ProjectDeleteProjectButton ) );

	gCurrentProject.currentPage = GeneralPage;

	// tableIndex is now used for the field index
	tableIndex = FrmGetObjectIndex( frmP, ProjectDurationField );
	FrmSetFocus( frmP, tableIndex );
	fieldP = FrmGetObjectPtr( frmP, tableIndex );
	FldSetSelection( fieldP, 0, FldGetTextLength( fieldP ) );
	FldSetUsable( fieldP, FrmGetControlGroupSelection( frmP, DateWheelCheckGroup ) != FrmGetObjectIndex( frmP, ProjectDurationLockCheck ) );
}

/*
 * FUNCTION:				ProjectFormSetTitle
 * DESCRIPTION:			this routine sets the title of the project form
 * PARAMETERS:			frmP - pointer to the project form
 * 									recP - poitner to the locked project record
 * RETURNS:					nothing
 */
static void ProjectFormSetTitle( FormType * frmP, PrjtPackedProjectType * recP )
{
	Char szTitle[32];
	RectangleType r;
	UInt16	width;
	UInt16	len;
	FontID	curFont;
	Boolean	fitInWidth;
	Char		horzEllipsis;

	// prepare the title for the form
	StrCopy( szTitle, "Pr: " );
	// as the names of the project are no longer than 27 chars we can use StrCat
	StrCat( szTitle, &recP->name );
	len = StrLen( szTitle );
	width = 160 - 8;

	if( gCurrentProject.currentPage == ToDoPage )
	{
		FrmGetObjectBounds( frmP, FrmGetObjectIndex( frmP, ProjectToDoCategoryPopTrigger ), &r );
		width -= (r.extent.x + 2);
	}

	curFont = FntSetFont( boldFont );
	FntCharsInWidth( szTitle, &width, &len, &fitInWidth );
	if( !fitInWidth )
	{
		ChrHorizEllipsis( &horzEllipsis );
		szTitle[len-1] = horzEllipsis;
		szTitle[len] = '\0';
	}
	
	// set the title
	FrmCopyTitle( frmP, szTitle );
}

/*
 * FUNCTION:			ProjectFormDeInit
 * DESCRIPTION:		this routine cleans up and saves the the 'last category'
 * PARAMETERS:		none
 * RETURNS:				nothing
 */
static void ProjectFormDeInit( void )
{
	// restore the previously stored field text color
#ifdef CONFIG_COLOR
	if( gGlobalPrefs.useColors )
		UIColorSetTableEntry( UIFieldText, &gPrevFldTxtColor );
#endif /* CONFIG_COLOR */

	// make sure our project is unlocked
	if( gCurrentProject.currentPage == MemoPage )
		ProjectFormDeInitializeMemoPage();
	else if( gCurrentProject.currentPage == ToDoPage && gCurrentProject.todoDB )
		TblReleaseFocus( (TableType *)GetObjectPtr( ProjectToDoTable ) );
	if( gCurrentProject.todoDB )
	{
		PrjtDBSetToDoLastCategory( gCurrentProject.todoDB, gCurrentProject.todoDBCategory );

		// now close the database
		DmCloseDatabase( gCurrentProject.todoDB );
		gCurrentProject.todoDB = 0;
	}
}

/*
 * FUNCTION:			ProjectFormInit
 * DESCRIPTION:		this routine initializes the whole form
 * 								it opens the database and draws the form
 * PARAMS:				frmP - pointer to the ProjectForm
 * RETURNS:				nothing
 * REVISION HISTORY:  Jul 26 2002 pete - save field text color
 */
static void ProjectFormInit( FormPtr frmP )
{
	PrjtPackedProjectType * recP;
	MemHandle								recH;
	UInt16									gotoPage;

	ErrFatalDisplayIf( !gMainDatabase.db, "main db not open" );
	ErrFatalDisplayIf( gCurrentProject.todoDB, "db already open" );

#ifdef CONFIG_HANDERA
	if( gGlobalPrefs.vgaExtension )
	{
		VgaFormModify( frmP, vgaFormModify160To240 );
		VgaTableUseBaseFont( (TableType *)GetObjectPtr( ProjectToDoTable ), true );
		ProjectFormResize( frmP, false );
	}
#endif /* CONFIG_HANDERA */
		
	recH = DmQueryRecord( gMainDatabase.db, gCurrentProject.projIndex );
	if( !recH )
	{
		FrmGotoForm( MainForm );
		return;
	}

	recP = MemHandleLock( recH );
	gCurrentProject.todoDB = PrjtDBOpenToDoDB( &recP->name, dmModeReadWrite );

	if( gCurrentProject.todoDB )
		PrjtDBGetToDoDBInformation( gCurrentProject.todoDB, &gCurrentProject.todoDBCategory, &gCurrentProject.todoDBSortOrder );

	gGeneralPageInitialized = gToDoPageInitialized = false;
	gTopVisibleToDoIndex = 0;
	gToDoItemSelected = false;
	gCurrentToDoEditPosition = 0;
	gCurrentToDoEditSelectionLength = 0;

#ifdef CONFIG_COLOR
  // save the current text field color for restoring it later
  if( gGlobalPrefs.useColors )
    UIColorGetTableEntryRGB( UIFieldText, &gPrevFldTxtColor );
#endif /* CONFIG_COLOR */

	// this is a workaround of a bug that appears
	// on rom >= 4.0 :(
	// if launching the project form directly with
	// general or memo page then the controls seem
	// to be disabled
	ProjectFormMakeToDoTableColumsUsable( frmP );
	gotoPage = gCurrentProject.currentPage;
	gCurrentProject.currentPage = ToDoPage;

	switch( gotoPage )
	{
		case GeneralPage:
			ProjectFormSwitchToGeneralPage( frmP );
			FrmSetControlGroupSelection( frmP, 1, ProjectGeneralPushButton );
			break;
		case ToDoPage:
			// to prevent a bug set the field initialy non-editable
			FldSetUsable( (FieldType *)GetObjectPtr( ProjectDurationField ), false );
			ProjectFormSwitchToToDoPage( frmP, false );
			FrmSetControlGroupSelection( frmP, 1, ProjectToDoPushButton );
			break;
		case MemoPage:
			// to prevent a bug set the field initialy non-editable
			FldSetUsable( (FieldType *)GetObjectPtr( ProjectDurationField ), false );
			ProjectFormSwitchToMemoPage( frmP );
			FrmSetControlGroupSelection( frmP, 1, ProjectMemoPushButton );
			break;
	}

	ProjectFormSetTitle( frmP, recP );

	// unlock the project record as we dont need it anymore
	MemHandleUnlock( recH );
}

/*
 * FUNCTION:					SelectMemoWordAndDrawForm
 * DESCRIPTION:				this routine selects a word on the memo page
 * PARAMETERS:				position - where to start in the string displayed on the memo page
 * 										len - lenght of the selection to make
 * RETURNS:						nothing
 */
static void SelectMemoWordAndDrawForm( UInt16 position, UInt16 len )
{
	FieldType * fieldP;

	fieldP = (FieldType *)GetObjectPtr( ProjectMemoField );

	FldSetScrollPosition( fieldP, position );
	FldSetSelection( fieldP, position, position + len );
	ProjectFormUpdateMemoScrollBar();
}

static void MoveSelectedToDo( UInt8 direction )
{
	UInt16		row;
	UInt16		recIndex;
	UInt16		rowID;
	TableType * tableP;

	ErrFatalDisplayIf( !gToDoItemSelected, "no item selected" );
	ErrFatalDisplayIf( !gCurrentProject.todoDB, "database is not open" );

	tableP = (TableType *)GetObjectPtr( ProjectToDoTable );
	recIndex = gCurrentProject.curToDoIndex;

	if( direction == kMoveToDoOneUp )
	{
		if( !SeekToDoRecord( &recIndex, 1, dmSeekBackward ) )
		{
			ProjectFormToDoRestoreEditState();
			return;
		}

		if( errNone == DmMoveRecord( gCurrentProject.todoDB, gCurrentProject.curToDoIndex, recIndex ) )
		{
			rowID = TblGetRowID( tableP, 0 );

			if( recIndex < rowID )
				gTopVisibleToDoIndex = recIndex;
		}
	}
	else if( direction == kMoveToDoOneDown )
	{
		if( !SeekToDoRecord( &recIndex, 1, dmSeekForward ) )
		{
			ProjectFormToDoRestoreEditState();
			return;
		}

		if( errNone == DmMoveRecord( gCurrentProject.todoDB, gCurrentProject.curToDoIndex, recIndex + 1) )
		{
			row = TblGetLastUsableRow( tableP );
			rowID = TblGetRowID( tableP, row );
			if( recIndex > rowID )
				gTopVisibleToDoIndex = recIndex;
		}
	}

	gCurrentProject.curToDoIndex = recIndex;
	ProjectFormLoadToDoTable( true );
	TblRedrawTable( tableP );
	ProjectFormToDoRestoreEditState();
}

/*
 * FUNCTION:		ProjectformAdjustDateWheel
 * DESCRIPTION:	Changes the other controls of our date wheel
 * PARAMETERS:	controlID of the control that has just been 
 * 							changed and wants to adjust the other controls
 * 							there are only 3: 
 * 							- ProjectBeginDateSelTrigger
 * 							- ProjectEndDateSelTrigger
 * 							- ProjectDurationField
 *
 * RETURNS:			nothing
 */
static void ProjectFormAdjustDateWheel( UInt16 justChangedControlId )
{
	UInt16					lockedObjIndex;
	UInt16					len;
	DateType 				date;
	MemHandle				recH;
	MemHandle				txtH;
	PrjtPackedProjectType * 	recP;
	Char *					durationText;
	Char *					labelP;
	FormType * 			frmP;
	FieldType *			fieldP;
	ControlType *		selTriggerP;
	Int32						durationDays;

	frmP = FrmGetActiveForm();
	lockedObjIndex = FrmGetControlGroupSelection( frmP, DateWheelCheckGroup );

	if( justChangedControlId == ProjectBeginSelTrigger )
	{
		if( lockedObjIndex == FrmGetObjectIndex( frmP, ProjectDurationLockCheck ) )
		{
			// duration is locked so adjust the end date
			fieldP = FrmGetObjectPtr( frmP, FrmGetObjectIndex( frmP, ProjectDurationField ) );
			durationText = FldGetTextPtr( fieldP );

			// if there is no duration specified or duration is zero we dont change the end date
			if( !durationText || !FldGetTextLength( fieldP ) )
				return;

			durationDays = StrAToI( durationText );

			recH = DmGetRecord( gMainDatabase.db, gCurrentProject.projIndex );
			if( !recH )
				return;
			recP = MemHandleLock( recH );
			date = recP->begin;

			DateAdjust( &date, durationDays );

			DmWrite( recP, OffsetOf( PrjtPackedProjectType, end ), &date, sizeof( recP->end ) );
			selTriggerP = FrmGetObjectPtr( frmP, FrmGetObjectIndex( frmP, ProjectEndSelTrigger ) );
			labelP = (Char *)CtlGetLabel( selTriggerP );
			DateToAscii( date.month, date.day, date.year + firstYear, gGlobalPrefs.dateFormat, labelP );
			CtlSetLabel( selTriggerP, labelP );

			MemHandleUnlock( recH );
			DmReleaseRecord( gMainDatabase.db, gCurrentProject.projIndex, true );
		}
		else
		{
			// the end date is locked so we adjust the duration
			recH = DmQueryRecord( gMainDatabase.db, gCurrentProject.projIndex );
			if( !recH )
				return;
			recP = MemHandleLock( recH );
			if( DateToInt( recP->end ) == kProjectNoEndDate )
				return;

			durationDays = DateToDays( recP->end );
			durationDays -= DateToDays( recP->begin );

			// as we dont need the record again we unlock it
			MemHandleUnlock( recH );

			frmP = FrmGetActiveForm();
			fieldP = FrmGetObjectPtr( frmP, FrmGetObjectIndex( frmP, ProjectDurationField ) );
			txtH = FldGetTextHandle( fieldP );
			if( !txtH )
				return;

			FldSetTextHandle( fieldP, NULL );
			durationText = MemHandleLock( txtH );
			StrIToA( durationText, durationDays );
			len = StrLen( durationText );
			MemHandleUnlock( txtH );

			FldSetTextHandle( fieldP, txtH );
			FldDrawField( fieldP );
			FldSetSelection( fieldP, 0, len );
		}
	}
	else if( justChangedControlId == ProjectDurationField )
	{
		fieldP = FrmGetObjectPtr( frmP, FrmGetObjectIndex( frmP, ProjectDurationField ) );
		durationText = FldGetTextPtr( fieldP );

		// if there is no duration specified or duration is zero we dont change the end date
		if( !durationText || !FldGetTextLength( fieldP ) )
			return;

		durationDays = StrAToI( durationText );

		recH = DmGetRecord( gMainDatabase.db, gCurrentProject.projIndex );
		if( !recH )
			return;
		recP = MemHandleLock( recH );
		if( lockedObjIndex == FrmGetObjectIndex( frmP, ProjectBeginLockCheck ) )
			date = recP->begin;
		else
			date = recP->end;

		if( lockedObjIndex == FrmGetObjectIndex( frmP, ProjectBeginLockCheck ) )
		{
			DateAdjust( &date, durationDays );
			DmWrite( recP, OffsetOf( PrjtPackedProjectType, end ), &date, sizeof( recP->end ) );
			selTriggerP = FrmGetObjectPtr( frmP, FrmGetObjectIndex( frmP, ProjectEndSelTrigger ) );
		}
		else
		{
			DateAdjust( &date, -durationDays );
			DmWrite( recP, OffsetOf( PrjtPackedProjectType, begin ), &date, sizeof( recP->begin ) );
			selTriggerP = FrmGetObjectPtr( frmP, FrmGetObjectIndex( frmP, ProjectBeginSelTrigger ) );
		}

		labelP = (Char *)CtlGetLabel( selTriggerP );
		DateToAscii( date.month, date.day, date.year + firstYear, gGlobalPrefs.dateFormat, labelP );
		CtlSetLabel( selTriggerP, labelP );

		MemHandleUnlock( recH );
		DmReleaseRecord( gMainDatabase.db, gCurrentProject.projIndex, true );
	}	
	else
	{
		// end date was changed
		if( lockedObjIndex == FrmGetObjectIndex( frmP, ProjectBeginLockCheck ) )
		{
			// the begin date is locked so adjust the duration
			recH = DmQueryRecord( gMainDatabase.db, gCurrentProject.projIndex );
			if( !recH )
				return;
			recP = MemHandleLock( recH );
			if( DateToInt( recP->end ) == kProjectNoEndDate )
				return;

			durationDays = DateToDays( recP->end );
			durationDays -= DateToDays( recP->begin );

			// as we dont need the record again we unlock it
			MemHandleUnlock( recH );

			frmP = FrmGetActiveForm();
			fieldP = FrmGetObjectPtr( frmP, FrmGetObjectIndex( frmP, ProjectDurationField ) );
			txtH = FldGetTextHandle( fieldP );
			if( !txtH )
				return;

			FldSetTextHandle( fieldP, NULL );
			durationText = MemHandleLock( txtH );
			StrIToA( durationText, durationDays );
			len = StrLen( durationText );
			MemHandleUnlock( txtH );

			FldSetTextHandle( fieldP, txtH );
			FldDrawField( fieldP );
			FldSetSelection( fieldP, 0, len );
		}
		else
		{
			// duration is locked so adjust the end date
			fieldP = FrmGetObjectPtr( frmP, FrmGetObjectIndex( frmP, ProjectDurationField ) );
			durationText = FldGetTextPtr( fieldP );

			// if there is no duration specified or duration is zero we dont change the end date
			if( !durationText || !FldGetTextLength( fieldP ) )
				return;

			durationDays = StrAToI( durationText );
			durationDays = -durationDays;

			recH = DmGetRecord( gMainDatabase.db, gCurrentProject.projIndex );
			if( !recH )
				return;

			recP = MemHandleLock( recH );
			date = recP->end;

			DateAdjust( &date, durationDays );

			DmWrite( recP, OffsetOf( PrjtPackedProjectType, begin), &date, sizeof( recP->begin ) );
			selTriggerP = FrmGetObjectPtr( frmP, FrmGetObjectIndex( frmP, ProjectBeginSelTrigger ) );
			labelP = (Char *)CtlGetLabel( selTriggerP );
			DateToAscii( date.month, date.day, date.year + firstYear, gGlobalPrefs.dateFormat, labelP );
			CtlSetLabel( selTriggerP, labelP );

			MemHandleUnlock( recH );
			DmReleaseRecord( gMainDatabase.db, gCurrentProject.projIndex, true );
		}
	}
}

static void ProjectFormMoveBeginDate( WinDirectionType direction )
{
	UInt16				objIndex;
	MemHandle			recH;
	DateType 			date;
	PrjtPackedProjectType *	recP;
	ControlType * ctlP;
	Char *				labelP;
	FormType	*		frmP;

	frmP = FrmGetActiveForm();
	objIndex = FrmGetControlGroupSelection( frmP, DateWheelCheckGroup );
	if( objIndex == FrmGetObjectIndex( frmP, ProjectBeginLockCheck ) )
		return;

	recH = DmGetRecord( gMainDatabase.db, gCurrentProject.projIndex );
	ErrFatalDisplayIf( !recH, "bad projects record" );
	recP = MemHandleLock( recH );

	date = recP->begin;
	DateAdjust( &date, (direction == winUp) ? 1 : -1 );
	DmWrite( recP, OffsetOf( PrjtPackedProjectType, begin ), &date, sizeof( recP->begin ) );
	MemHandleUnlock( recH );
	DmReleaseRecord( gMainDatabase.db, gCurrentProject.projIndex, true );

	ctlP = GetObjectPtr( ProjectBeginSelTrigger );
	labelP = (Char *)CtlGetLabel( ctlP );

	DateToAscii( date.month, date.day, date.year + firstYear, gGlobalPrefs.dateFormat, labelP );
	CtlSetLabel( ctlP, labelP );

	ProjectFormAdjustDateWheel( ProjectBeginSelTrigger );
}

static void ProjectFormMoveEndDate( WinDirectionType direction )
{
	UInt16				objIndex;
	MemHandle			recH;
	DateType 			date;
	PrjtPackedProjectType *	recP;
	ControlType * ctlP;
	Char *				labelP;
	FormType *		frmP;

	frmP = FrmGetActiveForm();
	objIndex = FrmGetControlGroupSelection( frmP, DateWheelCheckGroup );
	if( objIndex == FrmGetObjectIndex( frmP, ProjectEndLockCheck ) )
		return;

	recH = DmGetRecord( gMainDatabase.db, gCurrentProject.projIndex );
	ErrFatalDisplayIf( !recH, "bad projects record" );
	recP = MemHandleLock( recH );

	date = recP->end;
	if( DateToInt( date ) == kProjectNoEndDate )
		DateSecondsToDate( TimGetSeconds(), &date );

	DateAdjust( &date, (direction == winUp) ? 1 : -1 );
	DmWrite( recP, OffsetOf( PrjtPackedProjectType, end ), &date, sizeof( recP->end ) );
	MemHandleUnlock( recH );
	DmReleaseRecord( gMainDatabase.db, gCurrentProject.projIndex, true );

	ctlP = GetObjectPtr( ProjectEndSelTrigger );
	labelP = (Char *)CtlGetLabel( ctlP );

	DateToAscii( date.month, date.day, date.year + firstYear, gGlobalPrefs.dateFormat, labelP );
	CtlSetLabel( ctlP, labelP );

	ProjectFormAdjustDateWheel( ProjectEndSelTrigger );
}

static void ProjectFormMoveDuration( WinDirectionType direction )
{
	UInt16			objIndex;
	UInt16			len;
	MemHandle		txtH;
	FieldType * fieldP;
	Char *			txtP;
	FormType * 	frmP;
	Int32				durationDays;

	frmP = FrmGetActiveForm();
	objIndex = FrmGetControlGroupSelection( frmP, DateWheelCheckGroup );
	if( objIndex == FrmGetObjectIndex( frmP, ProjectDurationLockCheck ) )
		return;

	fieldP = (FieldType *)GetObjectPtr( ProjectDurationField );
	txtH = FldGetTextHandle( fieldP );

	if( !txtH )
	{
		txtH = MemHandleNew( kDurationFieldMaxChars );
		if( !txtH )
			return;
		StrCopy( MemHandleLock( txtH ), "0" );
		MemHandleUnlock( txtH );
	}

	txtP = MemHandleLock( txtH );

	durationDays = StrAToI( txtP );

	if( direction == winUp )
		durationDays++;
	else
		durationDays--;

	StrIToA( txtP, durationDays );
	len = StrLen( txtP );

	MemHandleUnlock( txtH );
	FldSetTextHandle( fieldP, txtH );
	FldDrawField( fieldP );
	FldSetSelection( fieldP, 0, len );

	ProjectFormAdjustDateWheel( ProjectDurationField );
}

#ifdef CONFIG_JOGDIAL
/*
 * FUNCTION:			ProjectFormSelectToDoItem
 * DESCRIPTION:		this routine sets the insertion point to the first/last item
 * 								in our todo table
 * PARAMETERS:		select - selectFirst or selectLast
 * RETURNS:				nothing
 */
static void ProjectFormSelectToDoItem( SelectToDoItemType select )
{
	TableType * tblP;
	Int16				row;

	ErrFatalDisplayIf( gCurrentProject.currentPage != ToDoPage, "invalid page" );
	ErrFatalDisplayIf( gToDoItemSelected, "an item already selected" );

	tblP = (TableType *)GetObjectPtr( ProjectToDoTable );
	row = TblGetLastUsableRow( tblP );
	if( row != tblUnusableRow )
	{
		if( select == selectFirst )
			row = 0;
	}
	else
		return;

	gToDoItemSelected = true;
	gCurrentProject.curToDoIndex = TblGetRowID( tblP, row );
	gCurrentToDoEditPosition = 0;
	gCurrentToDoEditSelectionLength = 0;
	ProjectFormToDoRestoreEditState();
}

/*
 * FUNCTION:			ProjectFormSelectNextField
 * DESCRIPTION:		this routine inserts the focus to the next field (if any)
 * 								the table will be scrolled if nesseccary
 * PARAMETERS:		none
 * RETURNS:				nothing
 */
static void	ProjectFormSelectNextField( void )
{
	TableType * tblP;
	UInt16			recIndex;
	Int16				row;
	Int16				lastRow;
	Boolean			ok;

	ErrFatalDisplayIf( !gToDoItemSelected, "no item selected" );

	tblP = (TableType * )GetObjectPtr( ProjectToDoTable );
	recIndex = gCurrentProject.curToDoIndex;
	if( SeekToDoRecord( &recIndex, 1, dmSeekForward ) )
	{
		TblReleaseFocus( tblP );

		gCurrentProject.curToDoIndex = recIndex;
		ok = TblFindRowID( tblP, recIndex, &row );
		lastRow = TblGetLastUsableRow( tblP );
		if( !ok || row == lastRow )
		{
			gTopVisibleToDoIndex++;
			ProjectFormLoadToDoTable( true );
			TblRedrawTable( tblP );
		}

		gCurrentToDoEditPosition = 0;
		gCurrentToDoEditSelectionLength = 0;
		ProjectFormToDoRestoreEditState();
	}
}

/*
 * FUNCTION:			ProjectFormSelectPrevField
 * DESCRIPTION:		this routine inserts the focus to the previous field (if any)
 * 								the form will be scrolled if nesseccary
 * PARAMETERS:		none
 * RETURNS:				nothing
 */
static void	ProjectFormSelectPrevField( void )
{
	TableType * tblP;
	UInt16			recIndex;
	Int16				row;
	Boolean			ok;
	
	ErrFatalDisplayIf( !gToDoItemSelected, "no item selected" );

	tblP = (TableType * )GetObjectPtr( ProjectToDoTable );
	recIndex = gCurrentProject.curToDoIndex;
	if( SeekToDoRecord( &recIndex, 1, dmSeekBackward ) )
	{
		TblReleaseFocus( tblP );

		gCurrentProject.curToDoIndex = recIndex;
		ok = TblFindRowID( tblP, recIndex, &row );

		if( !ok )
		{
			gTopVisibleToDoIndex = recIndex;
			ProjectFormLoadToDoTable( true );
			TblRedrawTable( tblP );
		}

		gCurrentToDoEditPosition = 0;
		gCurrentToDoEditSelectionLength = 0;
		ProjectFormToDoRestoreEditState();
	}
}
#endif /* CONFIG_JOGDIAL */

/*
 * FUNCTION:				ProjectFormHandleEvent
 * DESCRIPTION:			this routine manages all events
 * 									on the ProjectForm
 * PARAMS:					eventP - the event
 * RETURNS:					true if the event was handled, else false
 */
Boolean ProjectFormHandleEvent( EventType * eventP )
{
	Boolean handled = false;
	Int16 	row;
	FormType * frmP;
	TableType * tableP;

	switch( eventP->eType )
	{
		case menuEvent:
			MenuEraseStatus(0);
			switch( eventP->data.menu.itemID )
			{
				case OptionsAbout:
					ShowAbout();
					handled = true;
					break;

				case OptionsPhoneLookup:
					if( gCurrentProject.currentPage == ToDoPage )
					{
						if( !gToDoItemSelected )
							FrmAlert( SelectItemAlert );
						else
							PhoneNumberLookup( TblGetCurrentField( (TableType *)GetObjectPtr( ProjectToDoTable ) ) );
					}
					else
						PhoneNumberLookup( (FieldType *)GetObjectPtr( ProjectMemoField ) );

					handled = true;
					break;

				case OptionsFont:
					ProjectFormSelectFont();
					handled = true;
					break;

				case OptionsPreferences:
					ProjectFormToDoClearEditState();
					ProjectFormHandlePreferencesSetting();
					handled = true;
					break;

				case EditPaste:
					if( gCurrentProject.currentPage == ToDoPage && !gToDoItemSelected )
						ProjectFormCreateNewToDoRecord();
					break;

				case DeleteToDoItem:
					ProjectFormDeleteToDo();
					handled = true;
					break;

				case AttachToDoNote:
					// dont clear the edit state here
					if( !gToDoItemSelected )
						FrmAlert( SelectItemAlert );
					else
						ProjectFormPopupToDoNote();
					handled = true;
					break;

				case DeleteToDoNote:
					if( !gToDoItemSelected )
						FrmAlert( SelectItemAlert );
					else
						ProjectFormToDoDeleteNote();
					handled = true;
					break;

				case PurgeToDoList:
					ProjectPurgeToDos();
					handled = true;
					break;

				case RescanToDoDatabase:
					ProjectFormRescanDatabase();
					handled = true;
					break;

				// we get this command from the memo page
				case DeleteMemo:
					ProjectFormDeleteMemo();
					handled = true;
					break;

				// we get this command from the memo page
				case ImportMemo:
					ProjectFormImportMemo();
					handled = true;
					break;

				// we get this command from the memo page
				case ExportMemo:
					ProjectFormExportMemo();
					handled = true;
					break;

				// we get this command from the general page
				case DuplicateProject:
					ProjectFormDuplicateProject();
					handled = true;
					break;

				// we get this command from the general page
				case ClearEndDate:
					ProjectFormClearEndDate();
					handled = true;
					break;

				default:
					break;
			}
			break;

#ifdef CONFIG_ADDITIONAL
		// we dont need to check for the rom version because this event
		// is not generated on older versions
		case menuCmdBarOpenEvent:
			if( gCurrentProject.currentPage == ToDoPage )
			{
				if( gToDoItemSelected )
				{
					FieldType * fieldP = TblGetCurrentField( (TableType *)GetObjectPtr( ProjectToDoTable ) );
					UInt16 startpos, endpos;

					FldGetSelection( fieldP, &startpos, &endpos );
					if( startpos == endpos )		// if there's no selection
					{
						FldHandleEvent( fieldP, eventP );	// make the field add buttons first
						MenuCmdBarAddButton( menuCmdBarOnRight, BarDeleteBitmap, menuCmdBarResultMenuItem, DeleteToDoItem, NULL );
						// prevent the system to add the field button we have already added
						eventP->data.menuCmdBarOpen.preventFieldButtons = true;
					}
					// don't set handled to true, the system needs to get this event, too.
				}
				else
				{
					MenuCmdBarAddButton( menuCmdBarOnRight, BarInfoBitmap, menuCmdBarResultMenuItem, OptionsAbout, NULL );
					MenuCmdBarAddButton( menuCmdBarOnRight, PreferencesBitmap, menuCmdBarResultMenuItem, OptionsPreferences, NULL );
					// prevent the system to add the field button we have already added
					eventP->data.menuCmdBarOpen.preventFieldButtons = true;
				}
			}
			else if( gCurrentProject.currentPage == GeneralPage )
			{
				MenuCmdBarAddButton( menuCmdBarOnRight, BarInfoBitmap, menuCmdBarResultMenuItem, OptionsAbout, NULL );
				eventP->data.menuCmdBarOpen.preventFieldButtons = true;
			}
			else	// memo page
			{
				MenuCmdBarAddButton( menuCmdBarOnLeft, BarInfoBitmap, menuCmdBarResultMenuItem, OptionsAbout, NULL );
			}
			break;
#endif /* CONFIG_ADDITIONAL */

		case keyDownEvent:
			if( ChrIsHardKey( eventP->data.keyDown.chr ) )
			{
				if( !(eventP->data.keyDown.modifiers & poweredOnKeyMask) )
				{
					if( gCurrentProject.currentPage == ToDoPage )
						ProjectFormToDoClearEditState();
					FrmGotoForm( MainForm );
					handled = true;
				}
			}
#ifdef CONFIG_JOGDIAL
	#ifdef CONFIG_SONY
			else if( eventP->data.keyDown.chr == vchrJogDown )
	#else
				// applies to handera and normal version
			else if( eventP->data.keyDown.chr == vchrNextField )
	#endif /* CONFIG_SONY */
			{
				if( gCurrentProject.currentPage == ToDoPage && gCurrentProject.todoDB )
				{
					if( gToDoItemSelected )
						ProjectFormSelectNextField();
					else
						ProjectFormSelectToDoItem( selectFirst );
				}
				else if( gCurrentProject.currentPage == MemoPage )
					ProjectFormScrollMemoPage( winDown );
				handled = true;
			}
	#ifdef CONFIG_SONY
			else if( eventP->data.keyDown.chr == vchrJogUp )
	#else
				// applies to normal and handera version
			else if( eventP->data.keyDown.chr == vchrPrevField )
	#endif /* CONFIG_SONY */
			{
				if( gCurrentProject.currentPage == ToDoPage && gCurrentProject.todoDB )
				{
					if( gToDoItemSelected )
						ProjectFormSelectPrevField();
					else
						ProjectFormSelectToDoItem( selectLast );
				}
				else if( gCurrentProject.currentPage == MemoPage )
					ProjectFormScrollMemoPage( winUp );
				handled = true;
			}
	#ifdef CONFIG_SONY
			else if( eventP->data.keyDown.chr == vchrJogPush )
	#else
			#ifndef CONFIG_HANDERA
				#error "please define CONFIG_HANDERA or CONFIG_SONY or remove the CONFIG_JOG definition!"
			#endif /* CONFIG_HANDERA */
			else if( eventP->data.keyDown.chr == chrCarriageReturn )
	#endif /* CONFIG_SONY */
			{
				if( gCurrentProject.currentPage == ToDoPage && gToDoItemSelected )
					ProjectFormPopupToDoNote();
				handled = true;
			}
	#ifdef CONFIG_SONY
			else if( eventP->data.keyDown.chr == vchrJogBack )
	#else
		#ifndef CONFIG_HANDERA
			#error "please define CONFIG_HANDERA or CONFIG_SONY or remove the CONFIG_JOGDIAL definition!"
		#endif /* CONFIG_HANDERA */
			else if( eventP->data.keyDown.chr == chrEscape )
	#endif /* CONFIG_SONY */
			{
				if( gCurrentProject.currentPage == ToDoPage )
					ProjectFormToDoClearEditState();
				FrmGotoForm( MainForm );
				handled = true;
			}
#endif /* CONFIG_JOGDIAL */
			else if( eventP->data.keyDown.chr == vchrPageUp )
			{
				if( gCurrentProject.currentPage == ToDoPage && gToDoScrollUpEnabled )
				{
					ProjectFormScrollToDoPage( winUp );
					handled = true;
				}
				else if( gCurrentProject.currentPage == MemoPage )
				{
					ProjectFormScrollMemoPage( winUp );
					handled = true;
				}
			}
			else if( eventP->data.keyDown.chr == vchrPageDown )
			{
				if( gCurrentProject.currentPage == ToDoPage && gToDoScrollDownEnabled )
				{
					ProjectFormScrollToDoPage( winDown );
					handled = true;
				}
				if( gCurrentProject.currentPage == MemoPage )
				{
					ProjectFormScrollMemoPage( winDown );
					handled = true;
				}
			}
			else if( !gToDoItemSelected &&
								IsCharPrintable( eventP->data.keyDown.chr ) &&
								(gCurrentProject.currentPage == ToDoPage) )
			{
				ProjectFormCreateNewToDoRecord();
				if( IsCharSmallLetter( eventP->data.keyDown.chr ) )
					eventP->data.keyDown.chr -= ('a' - 'A');
			}
			else if( gCurrentProject.currentPage == GeneralPage && 
								eventP->data.keyDown.chr >= '0' &&
								eventP->data.keyDown.chr <= '9' )
			{
				frmP = FrmGetActiveForm();
				if( FrmGetControlGroupSelection( frmP, DateWheelCheckGroup ) != FrmGetObjectIndex( frmP, ProjectDurationLockCheck ) )
				{
					if( FrmHandleEvent( FrmGetActiveForm(), eventP ) )
						ProjectFormAdjustDateWheel( ProjectDurationField );
					handled = true;	// dont let the system get this message twice
				}
			}
			break;

		case frmGotoEvent:
			if( eventP->data.frmGoto.matchCustom == kGotoToDoItem || 
					eventP->data.frmGoto.matchCustom == kGotoToDoItemNote )
			{
				gCurrentProject.currentPage = ToDoPage;

				frmP = FrmGetActiveForm();
				ProjectFormInit( frmP );
				gCurrentProject.curToDoIndex = eventP->data.frmGoto.recordNum;
				ProjectFormEnsureCurrentItemWillBeVisible();
				ProjectFormLoadToDoTable( true );
				gToDoItemSelected = true;
				if( eventP->data.frmGoto.matchCustom == kGotoToDoItem )
				{
					FrmDrawForm( frmP );
					gCurrentToDoEditPosition = eventP->data.frmGoto.matchPos;
					gCurrentToDoEditSelectionLength = eventP->data.frmGoto.matchLen;
					ProjectFormToDoRestoreEditState();
				}
				else
				{
					gCurrentToDoEditPosition = (eventP->data.frmGoto.matchPos);
					gCurrentToDoEditSelectionLength = (eventP->data.frmGoto.matchLen);

					MemSet( eventP, sizeof( EventType ), 0 );
					eventP->eType = frmLoadEvent;
					eventP->data.frmLoad.formID = ToDoNoteForm;
					EvtAddEventToQueue( eventP );

					eventP->eType = frmGotoEvent;
					eventP->data.frmGoto.formID = ToDoNoteForm;
					eventP->data.frmGoto.matchPos = gCurrentToDoEditPosition;
					eventP->data.frmGoto.matchLen = gCurrentToDoEditSelectionLength;
					EvtAddEventToQueue( eventP );

					gCurrentToDoEditPosition = kToDoDescriptionMaxLength;
					gCurrentToDoEditSelectionLength = 0;
				}
			}
			else
			{
				ErrFatalDisplayIf( eventP->data.frmGoto.matchCustom != kGotoPrjtMemo, "invalid goto type" );

				gCurrentProject.currentPage = MemoPage;

				frmP = FrmGetActiveForm();
				ProjectFormInit( frmP );
				SelectMemoWordAndDrawForm( eventP->data.frmGoto.matchPos, eventP->data.frmGoto.matchLen );
				FrmDrawForm( frmP );
			}
			handled = true;
			break;

		case frmOpenEvent:
			frmP = FrmGetActiveForm();
			ProjectFormInit( frmP );
			FrmDrawForm( frmP );
			handled = true;
			break;

		case frmCloseEvent:
			ProjectFormDeInit();
			break;

		case frmUpdateEvent:
			if( eventP->data.frmUpdate.updateCode & kUpdateWholeForm )
				FrmDrawForm( FrmGetActiveForm() );

			tableP = (TableType *)GetObjectPtr( ProjectToDoTable );
			if( eventP->data.frmUpdate.updateCode & kUpdateCurrentToDoItem )
			{
				TblFindRowID( tableP, gCurrentProject.curToDoIndex, &row );
				TblMarkRowInvalid( tableP, row );
			}
			else
				TblMarkTableInvalid( tableP );
			ProjectFormLoadToDoTable( true );
			TblRedrawTable( tableP );
			if( gToDoItemSelected )
				ProjectFormToDoRestoreEditState();
			handled = true;
			break;

		case penDownEvent:
			if( !FrmHandleEvent( FrmGetActiveForm(), eventP ) )
				ProjectFormToDoClearEditState();
			handled = true;	// don't let the system get this message twice
			break;

		case popSelectEvent:
			if( (eventP->data.popSelect.selection != eventP->data.popSelect.priorSelection) &&
					eventP->data.popSelect.selection != -1 )
				ProjectFormSelectState( eventP->data.popSelect.selection );
			// let the system get this event -> don't set handled to true
			break;

		case fldChangedEvent:
			if( eventP->data.fldChanged.fieldID == ProjectMemoField )
			{
				ProjectFormUpdateMemoScrollBar();
				handled = true;
			}
			break;

		case fldHeightChangedEvent:
			ProjectFormToDoTableResizeDescription( eventP );
			handled = true;	// don't let the system get this message again. (tblhandleevent is called in the prev function call)
			break;

		case sclRepeatEvent:
			ProjectFormScrollMemoLines( eventP->data.sclRepeat.newValue - eventP->data.sclRepeat.value, false );
			// let the system get this event, too.
			break;

		case tblEnterEvent:
			if( gToDoItemSelected )
			{
				if( TblFindRowID( eventP->data.tblEnter.pTable, gCurrentProject.curToDoIndex, &row ) )
				{
					if( eventP->data.tblEnter.row != row )
						handled = ProjectFormToDoClearEditState();
				}
			}
			else if( eventP->data.tblEnter.column == kProjectFormDescriptionTableColumn )
				gCurrentProject.curToDoIndex = TblGetRowID( eventP->data.tblEnter.pTable, eventP->data.tblEnter.row );
			break;

		case tblExitEvent:
			if( gToDoItemSelected )
			{
				ProjectFormToDoRestoreEditState();
				handled = true;
			}
			break;

		case tblSelectEvent:
			switch( eventP->data.tblSelect.column )
			{
				case kProjectFormCompletedTableColumn:
					ProjectFormToggleToDoCompleted( eventP );
					handled = true;
					break;

				case kProjectFormPriorityTableColumn:
					ProjectFormSelectToDoPriority( eventP );
					handled = true;
					break;

				case kProjectFormDescriptionTableColumn:
					frmP = FrmGetActiveForm();
					tableP = FrmGetObjectPtr( frmP, FrmGetObjectIndex( frmP, ProjectToDoTable ) );
					gCurrentProject.curToDoIndex = TblGetRowID( tableP, eventP->data.tblSelect.row );
					if( TblEditing( tableP ) )
						gToDoItemSelected = true;
					else
						ProjectFormPopupToDoNote();
					break;

				case kProjectFormDueDateTableColumn:
					ProjectFormSelectDueDate( eventP );
					handled = true;
					break;

				case kProjectFormCategoryTableColumn:
					ProjectFormSelectRecCategory( eventP->data.tblEnter.pTable, eventP->data.tblEnter.row );
					handled = true;
					break;

				default:
					break;
			}
			break;

		case ctlEnterEvent:
			switch( eventP->data.ctlEnter.controlID )
			{
				case ProjectToDoScrollerUpRepeating:
				case ProjectToDoScrollerDownRepeating:
					ProjectFormToDoClearEditState();
					break;

				default:
					break;
			}
			break;

		case ctlExitEvent:
			switch( eventP->data.ctlEnter.controlID )
			{
				case ProjectGeneralPushButton:
				case ProjectToDoPushButton:
				case ProjectMemoPushButton:
				case ProjectDoneButton:
				case ProjectNewToDoButton:
				case ProjectDetailsToDoButton:
				case ProjectToDoCategoryPopTrigger:
				case ProjectMoveToDoUpButton:
				case ProjectMoveToDoDownButton:
					ProjectFormToDoRestoreEditState();
				break;
			}
			break;

		case ctlRepeatEvent:
			if( (eventP->data.ctlRepeat.controlID == ProjectToDoScrollerUpRepeating) || (eventP->data.ctlRepeat.controlID == ProjectToDoScrollerDownRepeating) )
				ProjectFormScrollToDoPage( (eventP->data.ctlRepeat.controlID == ProjectToDoScrollerUpRepeating) ? winUp : winDown );
			else if( eventP->data.ctlRepeat.controlID == ProjectBeginDownRepeating ||
							eventP->data.ctlRepeat.controlID == ProjectBeginUpRepeating )
				ProjectFormMoveBeginDate( (eventP->data.ctlRepeat.controlID == ProjectBeginDownRepeating) ? winDown : winUp );
			else if( eventP->data.ctlRepeat.controlID == ProjectEndDownRepeating ||
							eventP->data.ctlRepeat.controlID == ProjectEndUpRepeating )
				ProjectFormMoveEndDate( (eventP->data.ctlRepeat.controlID == ProjectEndDownRepeating) ? winDown : winUp );
			else if( eventP->data.ctlRepeat.controlID == ProjectDurationDownRepeating ||
							eventP->data.ctlRepeat.controlID == ProjectDurationUpRepeating )
				ProjectFormMoveDuration( (eventP->data.ctlRepeat.controlID == ProjectDurationDownRepeating) ? winDown : winUp );
			break;

		case ctlSelectEvent:
			switch( eventP->data.ctlSelect.controlID )
			{
				case ProjectGeneralPushButton:
					if( gCurrentProject.currentPage == ToDoPage )
						ProjectFormToDoClearEditState();
					if( gCurrentProject.currentPage != GeneralPage )
						ProjectFormSwitchToGeneralPage( FrmGetActiveForm() );
					handled = true;
					break;

				case ProjectToDoPushButton:
					if( gCurrentProject.currentPage == ToDoPage )
						ProjectFormToDoClearEditState();
					if( gCurrentProject.currentPage != ToDoPage )
						ProjectFormSwitchToToDoPage( FrmGetActiveForm(), true );
					handled = true;
					break;

				case ProjectMemoPushButton:
					if( gCurrentProject.currentPage == ToDoPage )
						ProjectFormToDoClearEditState();
					if( gCurrentProject.currentPage != MemoPage )
						ProjectFormSwitchToMemoPage( FrmGetActiveForm() );
					handled = true;
					break;

				case ProjectBeginSelTrigger:
				case ProjectEndSelTrigger:
					ProjectFormSelectProjectsDate( eventP->data.ctlSelect.controlID );
					handled = true;
					break;

				case ProjectPriority1PushButton:
				case ProjectPriority2PushButton:
				case ProjectPriority3PushButton:
				case ProjectPriority4PushButton:
				case ProjectPriority5PushButton:
					ProjectFormSelectPriority( eventP->data.ctlSelect.controlID );
					handled = true;
					break;

				case ProjectCategoryPopTrigger:
					ProjectFormSelectProjectCategory( FrmGetActiveForm() );
					handled = true;
					break;

				case ProjectToDoCategoryPopTrigger:
					// as this control appears only on the todo page 
					// we dont have to check whether we are realy 
					// on the todo page
					ProjectFormToDoClearEditState();
					ToDoSelectCategory( FrmGetActiveForm() );
					handled = true;
					break;

				case ProjectPrivateCheckbox:
					PrjtDBSetProjectSecretBit( gMainDatabase.db, gCurrentProject.projIndex, eventP->data.ctlSelect.on );
					handled = true;
					break;

				case ProjectRenameProjectButton:
					ProjectFormRenameCurrentProject();
					handled = true;
					break;

				case ProjectDeleteProjectButton:
					ProjectFormDeleteProject();
					handled = true;
					break;

				case ProjectNewToDoButton:
					// dont clear the edit state here
					ProjectFormCreateNewToDoRecord();
					handled = true;
					break;

				case ProjectDetailsToDoButton:
					// dont clear the edit state here
					if( !gToDoItemSelected )
						FrmAlert( SelectItemAlert );
					else
						FrmPopupForm( ToDoDetailsForm );
					handled = true;
					break;

				case ProjectMoveToDoUpButton:
				case ProjectMoveToDoDownButton:
					if( !gToDoItemSelected )
						FrmAlert( SelectItemAlert );
					else
						MoveSelectedToDo( (eventP->data.ctlSelect.controlID == ProjectMoveToDoUpButton) ? kMoveToDoOneUp : kMoveToDoOneDown );
					handled = true;
					break;

				case ProjectDoneButton:
					if( gCurrentProject.currentPage == ToDoPage )
						ProjectFormToDoClearEditState();
					FrmGotoForm( MainForm );
					handled = true;
					break;

				case ProjectCreateDBButton:
					ErrFatalDisplayIf( gCurrentProject.todoDB, "there is already a database" );
					ProjectFormCreateDatabase();
					handled = true;
					break;

				case ProjectDurationLockCheck:
				case ProjectBeginLockCheck:
				case ProjectEndLockCheck:
					frmP = FrmGetActiveForm();
					FldSetUsable( (FieldType *)GetObjectPtr( ProjectDurationField ), (FrmGetControlGroupSelection( frmP, DateWheelCheckGroup ) != FrmGetObjectIndex( frmP, ProjectDurationLockCheck )) );
					handled = true;
					break;

				default:
					break;
			}
			break;

#ifdef CONFIG_HANDERA
		case displayExtentChangedEvent:
			if( gGlobalPrefs.vgaExtension )
			{
				ProjectFormResize( FrmGetActiveForm(), true );
				handled = true;
			}
			break;
#endif /* CONFIG_HANDERA */

		default:
			break;
	}

	return (handled);
}
