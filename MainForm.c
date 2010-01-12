/* FOR BEST FORMATING OF THIS FILE SET TABSTOP TO 2 */

#include <PalmOS.h>
#include "ProjectsRcp.h"
#include "ProjectsDB.h"
#include "Projects.h"

#ifndef __PROJECTS_CONFIG_H__
	#error "include config.h"
#endif /* __PROJECTS_CONFIG_H__ */

#ifdef CONFIG_HANDERA
	#include "Vga.h"
	#include "HndrFuncs.h"
#endif /* CONFIG_HANDERA */

#ifdef CONFIG_SONY
	#include <SonyCLIE.h>
#endif /* CONFIG_SONY */

#define kMainFormPriorityTableColumn 			0
#define kMainFormBeginDateTableColumn			1
#define kMainFormInfoTableColumn 					2
#define kMainFormStateTableColumn 				3
#define kMainFormMemoTableColumn					4
#define kMainFormCategoryTableColumn			5

static UInt16		gTopVisibleProjectIndex;


// declarations -----------------------------------------------------------------------------------
static void 		MainFormInit( FormType * frmP );
static void 		MainFormLoadTable( void );
static void 		MainFormDrawProject( void *tableP, Int16 row, Int16 column, RectangleType * bounds );
static void 		MainFormSelectInfoFromList( TableType *tableP, Int16 row, Int16 column );
static void 		MainFormSelectRecCategory( TableType * tblP, Int16 row, Int16 column );
static void 		MainFormSetProjectTableColumns( TableType * tableP );
static void 		MainFormHandlePreferencesSelection( void ) SECOND_SECTION;
static void 		MainFormSelectFont( void ) SECOND_SECTION;
static void			MainFormSelectCategory( void ) SECOND_SECTION;
static void 		MainFormSelectSecurity( void ) SECOND_SECTION;
static void			MainFormHandleNewProjectRequest( void ) SECOND_SECTION;
static void 		MainFormScan4DBs( void ) SECOND_SECTION;
static WinHandle MainFormInvertMoveIndicator( RectangleType * itemR, WinHandle savedBits, Boolean draw ) SECOND_SECTION;
static Boolean	MainFormMoveProjectByHand( EventType * eventP ) SECOND_SECTION;
static void 		MainFormScrollTable( ScrollTableType scrollTbl );
static Boolean 	MainFormSelectProjectByChar( Char c ) SECOND_SECTION;
static void			MainFormNextCategory( void ) SECOND_SECTION;

#ifdef CONFIG_ALLTODOS_DLG
	static void     MainFormHandleToDoButton( void ) SECOND_SECTION;
#endif /* CONFIG_ALLTODOS_DLG */

#ifdef CONFIG_HANDERA
	static void 		MainFormResize( FormType * frmP, Boolean draw ) SECOND_SECTION;
#endif /* CONFIG_HANDERA */

#ifdef CONFIG_JOGDIAL
	static void 		MainFormSelectItem( SelectItemDirection direction ) SECOND_SECTION;
#endif /* CONFIG_JOGDIAL */


// implementation ---------------------------------------------------------------------------------

#ifdef CONFIG_JOGDIAL
/*
 * FUNCTION:				MainFormSelectItem
 * DESCRIPTION:			this routine sets the gRowSelected variable and redraws the table
 * PARAMETERS:			direction - selectNext or selectPref
 * RETURNS:					nothing
 */
static void MainFormSelectItem( SelectItemDirection direction )
{
	TableType * tblP;
	UInt16			lastUsableRow;

	tblP = (TableType *)GetObjectPtr( MainProjectTable );

	if( direction == selectNext )
	{
		if( gRowSelected == noRowSelected )
			gRowSelected = 0;
		else
		{
			lastUsableRow = TblGetLastUsableRow( tblP );
			if( gRowSelected == lastUsableRow )
			{
				if( CtlEnabled( GetObjectPtr( MainScrollDownRepeating ) ) )
					MainFormScrollTable( scrollTblOneDown );
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
	else // if( direction == selectPref )
	{
		if( gRowSelected == noRowSelected )
			gRowSelected = TblGetLastUsableRow( tblP );
		else if( !gRowSelected )
		{
			if( CtlEnabled( GetObjectPtr( MainScrollUpRepeating ) ) )
				MainFormScrollTable( scrollTblOneUp );
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
#endif /* CONFIG_HANDERA */

#ifdef CONFIG_HANDERA
/*
 * FUNCTION:					MainFormResize
 * DESCRIPTION:				this routine resizes the form to fit into the screen
 * PARAMETERS:				frmP - pointer to the MainForm
 * 										draw - if true the form will be redrawn
 * RETURNS:						nothing
 */
static void MainFormResize( FormType * frmP, Boolean draw )
{
	RectangleType 	r;
	TableType *			tblP;
	Coord						x, y;
	Int16						x_diff, y_diff;

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
	HndrMoveObject( frmP, FrmGetObjectIndex( frmP, MainNewButton ), x_diff, y_diff, draw );
#ifdef CONFIG_ALLTODOS_DLG
	HndrMoveObject( frmP, FrmGetObjectIndex( frmP, MainToDoButton ), x_diff, y_diff, draw );
#endif /* CONFIG_ALLTODOS_DLG */
	HndrMoveObject( frmP, FrmGetObjectIndex( frmP, MainShowButton ), x_diff, y_diff, draw );
	HndrMoveObject( frmP, FrmGetObjectIndex( frmP, MainScrollUpRepeating ), x_diff, y_diff, draw );
	HndrMoveObject( frmP, FrmGetObjectIndex( frmP, MainScrollDownRepeating ), x_diff, y_diff, draw );
	HndrMoveObject( frmP, FrmGetObjectIndex( frmP, MainCategoriesPopTrigger ), x_diff, 0, draw );
	HndrMoveObject( frmP, FrmGetObjectIndex( frmP, MainCategoriesList ), x_diff, 0, false );

	// resize table
	tblP = FrmGetObjectPtr( frmP, FrmGetObjectIndex( frmP, MainProjectTable ) );
	if( draw )
		TblEraseTable( tblP );

	TblGetBounds( tblP, &r );
	r.extent.x = x;
	r.extent.y += y_diff;
	TblSetBounds( tblP, &r );
	MainFormSetProjectTableColumns( tblP );

	if( draw )
	{
		MainFormLoadTable();
		TblDrawTable( tblP );

		FrmDrawForm( frmP );
	}
}
#endif /* CONFIG_HANDERA */

#ifdef CONFIG_ALLTODOS_DLG
/*
 * FUNCTION:          MainFormHandleToDoButton
 * DESCRIPTION:       this routine checks the number of projects in the current
 *                    category and pops up the all todos dialog if the number is greater 0.
 * PAMETERS:          none
 * RETURNS:           nothing
 */
static void MainFormHandleToDoButton( void )
{
  UInt16      index = 0;

  ErrFatalDisplayIf( !gMainDatabase.db, "main db not open" );

  if( !SeekProjectRecord( &index, 0, dmSeekForward ) )
    ShowGeneralAlert( NoProjectsYet );
  else
    FrmGotoForm( AllToDosDialog );
}
#endif

/*
 * FUNCTION:					MainFormSelectFont
 * DESCRIPTION:				this routine lets the user select a font
 * 										and reinitializes our table
 * PARAMETERS:				none
 * RETURNS:						nothing
 */
static void	MainFormSelectFont( void )
{
	TableType * tblP;
	FontID			font;
	UInt16			i, j;
	UInt16			lineHeight;
	Int16				rows;

	font = FontSelect( gGlobalPrefs.mainFont );
	if( font != gGlobalPrefs.mainFont )
	{
		gGlobalPrefs.mainFont = font;
		font = FntSetFont( gGlobalPrefs.mainFont );
		lineHeight = FntLineHeight();
		FntSetFont( font );

		tblP = (TableType *)GetObjectPtr( MainProjectTable );
		rows = TblGetNumberOfRows( tblP );
		for( i = 0; i < rows; i++ )
		{
			TblSetRowHeight( tblP, i, lineHeight );
			for( j = kMainFormPriorityTableColumn; j <= kMainFormCategoryTableColumn; j++ )
				TblSetItemFont( tblP, i, j, gGlobalPrefs.mainFont );
		}
#ifdef CONFIG_HANDERA
		if( gGlobalPrefs.vgaExtension )
			MainFormSetProjectTableColumns( tblP );
#endif /* CONFIG_HANDERA */
		MainFormLoadTable();
		TblRedrawTable( tblP );
	}
}

/*
 * FUNCTION:					MainFormScan4DBs
 * DESCRIPTION:				this routine lets our db module scan for dbs
 * 										and lets the user know how may dbs were added.
 * 										it also updates the table if necessary and switches
 * 										to the unfiled category if we are not there or on 
 * 										the "all" category.
 * PARAMETERS:				none
 * RETURNS:						nothing
 */
static void	MainFormScan4DBs( void )
{
	Char 					szAdded[8];
	ControlType * ctlP;
	TableType *		tableP;
	Char * 				labelP;
	UInt16				added;

	if( !gMainDatabase.db )
		return;

	added = PrjtDBScanForNewToDoDBs( gMainDatabase.db, gMainDatabase.sortOrder );

	if( added )
	{
		if( gMainDatabase.currentCategory != dmAllCategories || gMainDatabase.currentCategory != dmUnfiledCategory )
		{
			gTopVisibleProjectIndex = 0;
			gMainDatabase.currentCategory = dmUnfiledCategory;
			ctlP = GetObjectPtr( MainCategoriesPopTrigger );
			labelP = (Char *)CtlGetLabel( ctlP );
			CategoryGetName( gMainDatabase.db, gMainDatabase.currentCategory, labelP );
			CategorySetTriggerLabel( ctlP, labelP );
		}
		MainFormLoadTable();
		tableP = (TableType *)GetObjectPtr( MainProjectTable );
		TblRedrawTable( tableP );
	}

	StrIToA( szAdded, added );
	FrmCustomAlert( AddedScannedDBsAlert, szAdded, NULL, NULL );
}

/*
 * FUNCTION:					MainFormInvertMoveIndicator
 * DESCRIPTION:				this function is a helper function
 * 										for the MainFormMoveProjectByHand routine.
 *										it saves or restores previously stored
 *										bits on the screen.
 * PARAMETERS:				itemR - rectangle to be considered
 * 										savedBits - if not 0 they will be restored
 * 										draw - if true the indicator will be drawn
 * 														the the saved bits will be returned.
 *													 if false the savedBits parater will
 *													  be restored and 0 is returned
 * RETURNS:						see draw parameter description
 */
static WinHandle MainFormInvertMoveIndicator( RectangleType * itemR, WinHandle savedBits, Boolean draw )
{
	UInt16	i;
	UInt16	err;
	WinHandle	winH = 0;
	RectangleType	indicatorR;
	CustomPatternType pattern;
	CustomPatternType savedPattern;

	indicatorR.topLeft.x = itemR->topLeft.x;
	indicatorR.topLeft.y = itemR->topLeft.y + itemR->extent.y - 2;
	indicatorR.extent.x = itemR->extent.x;
	indicatorR.extent.y = 2;
	
	if( draw )
	{
		WinGetPattern( &savedPattern );

		for( i = 0; i < sizeof( CustomPatternType ) / sizeof( *pattern ); i++ )
		{
			if( i % 2 )
				pattern[i] = 0xAA;
			else
				pattern[i] = 0x55;
		}

		WinSetPattern( (const CustomPatternType *)pattern );

		winH = WinSaveBits( &indicatorR, &err );

		WinFillRectangle( &indicatorR, 0 );

		WinSetPattern( (const CustomPatternType *)savedPattern );
	}
	else
		WinRestoreBits( savedBits, indicatorR.topLeft.x, indicatorR.topLeft.y );

	return (winH);
}


/*
 * FUNCTION:		MainFormMoveProjectByHand
 * DESCRIPTION:	like in the memo app this function changes the sort order
 * 							of a project by traking the pen
 * CALLED:			when an tblEnterEvent occurs on the kMainFormInfoTableColumn
 * RETURN:			true if the index of the record was changed else false
 */
static Boolean	MainFormMoveProjectByHand( EventType * eventP )
{
	RectangleType 	r;
	TableType * 		tableP;
	WinHandle				savedBits = 0;
	Int16						row;
	UInt16					recIndex;
	UInt16					selectedRow;
	UInt16					selectedRecIndex;
	Coord						x, y;
	Boolean					penDown = true;
	Boolean					moving = false;
	Boolean					selected = true;

	selectedRow = row = eventP->data.tblEnter.row;
	tableP = eventP->data.tblEnter.pTable;
	selectedRecIndex = TblGetRowID( tableP, row );

	TblGetItemBounds( tableP, row, kMainFormInfoTableColumn, &r );
	TblSelectItem( tableP, row, kMainFormInfoTableColumn );

	while( true )
	{
		PenGetPoint( &x, &y, &penDown );

		if( !penDown )
			break;

		if( !moving )
		{
			if( !RctPtInRectangle( x, y, &r ) )
			{
				moving = true;

				TblGetItemBounds( tableP, row, kMainFormInfoTableColumn, &r );
				savedBits = MainFormInvertMoveIndicator( &r, 0, true );
				selected = false;
			}
		}
		else if( !RctPtInRectangle( x, y, &r ) )
		{
			// above the first item ?
			if( row < 0 )
			{
				if( y >= r.topLeft.y )
				{
					row++;
					MainFormInvertMoveIndicator( &r, savedBits, false );
					r.topLeft.y = r.extent.y;
					savedBits = MainFormInvertMoveIndicator( &r, 0, true );
				}
			}

			// move winUp
			else if( y < r.topLeft.y )
			{
				recIndex = TblGetRowID( tableP, row );
				if( SeekProjectRecord( &recIndex, 1, dmSeekBackward ) )
				{
					MainFormInvertMoveIndicator( &r, savedBits, false );
					if( row )
						row--;
					else
					{
						if( gTopVisibleProjectIndex > 0 )
						{
							gTopVisibleProjectIndex = recIndex;
							MainFormLoadTable();
							TblRedrawTable( tableP );
							if( TblFindRowID( tableP, selectedRecIndex, &selectedRow ) )
								TblSelectItem( tableP, selectedRow, kMainFormInfoTableColumn );
						}
					}
					TblGetItemBounds( tableP, row, kMainFormInfoTableColumn, &r );
					savedBits = MainFormInvertMoveIndicator( &r, 0, true );
				}
				else if( row == 0 )
				{
					row--;
					MainFormInvertMoveIndicator( &r, savedBits, false );
					r.topLeft.y -= r.extent.y;
					savedBits = MainFormInvertMoveIndicator( &r, 0, true );
				}
			}

			// move winDown
			else
			{
				recIndex = TblGetRowID( tableP, row );
				if( SeekProjectRecord( &recIndex, 1, dmSeekForward ) )
				{
					MainFormInvertMoveIndicator( &r, savedBits, false );
					if( row < TblGetLastUsableRow( tableP ) )
						row++;
					else
					{
						gTopVisibleProjectIndex++;
						MainFormLoadTable();
						TblRedrawTable( tableP );
						if( TblFindRowID( tableP, selectedRecIndex, &selectedRow ) )
							TblSelectItem( tableP, selectedRow, kMainFormInfoTableColumn );
					}
					TblGetItemBounds( tableP, row, kMainFormInfoTableColumn, &r );
					savedBits = MainFormInvertMoveIndicator( &r, 0, true );
				}
			}
		}
	}

	if( moving )
	{
		savedBits = MainFormInvertMoveIndicator( &r, savedBits, false );
	}

	if( TblFindRowID( tableP, selectedRecIndex, &selectedRow ) )
		TblUnhighlightSelection( tableP );


	if( moving )
	{
		if( row >= 0 )
		{
			recIndex = TblGetRowID( tableP, row );
			if( selectedRecIndex == recIndex )
				return (!selected);

			recIndex++;
		}
		else
			recIndex = TblGetRowID( tableP, 0 );

		DmMoveRecord( gMainDatabase.db, selectedRecIndex, recIndex );

		MainFormLoadTable();
#ifdef CONFIG_JOGDIAL
		gRowSelected = noRowSelected;
#endif /* CONFIG_JOGDIAL */
		TblRedrawTable( tableP );
	}

	return (!selected);
}

/*
 * FUNCTION:		MainFormNextCategory
 * DESCRIPTION:	this routine swiches to
 * 							the next category from the
 * 							current one and updates the
 * 							view. note that categories
 * 							containing no records are
 * 							left alone.
 * PARAMETERS:	none
 * RETURNS:			nothing
 * CALLED:			when the user has assigned a
 * 							hardware button to our app
 * 							and pressed this one while
 * 							being on the main form.
 */
static void	MainFormNextCategory( void )
{
	UInt16 				category;
	ControlType * ctlP;
	FormType *		frmP;
	Char	*				labelP;
	TableType * 	tableP;
	
	category = CategoryGetNext( gMainDatabase.db, gMainDatabase.currentCategory );

	if( category != gMainDatabase.currentCategory )
	{
		gMainDatabase.currentCategory = category;
		gTopVisibleProjectIndex = 0;

		frmP = FrmGetActiveForm();
		// set the triggers label
		ctlP = FrmGetObjectPtr( frmP, FrmGetObjectIndex( frmP, MainCategoriesPopTrigger ) );
		// in our resource definition this control has a label that is 16 char long
		// when making changes ensure the resource label is at least dmCategoryLength chars long
		labelP = (Char *)CtlGetLabel( ctlP );
		CategoryGetName( gMainDatabase.db, gMainDatabase.currentCategory, labelP );
		CategorySetTriggerLabel( ctlP, labelP );

#ifdef CONFIG_JOGDIAL
	gRowSelected = noRowSelected;
#endif /* CONFIG_JOGDIAL */

		// load our table and redraw it
		MainFormLoadTable();
		tableP = FrmGetObjectPtr( frmP, FrmGetObjectIndex( frmP, MainProjectTable ) );
		TblRedrawTable( tableP );
	}
}

/*
 * FUNCTION:			MainFormHandlePreferencesSelection
 * DESCRIPTION:		this routine pops up the preference
 * 								dialog of the main form. it initializes
 * 								it and saves the changes and then calls
 * 								functions to apply the changes
 * PARAMETERS:		none
 * RETURNS:				nothing
 */
static void MainFormHandlePreferencesSelection( void )
{
	Boolean			onlyActiveBefore;
	UInt8				curSortOrder;
	UInt16			priority, begindate, todostate, projectstate, onlyactive, prjtcategories;
	Char *			labelP;
	TableType * tableP = (TableType *)GetObjectPtr( MainProjectTable );
	FormType * 	prefP = FrmInitForm( PreferencesDialog );
	ControlType * popP = FrmGetObjectPtr( prefP, FrmGetObjectIndex( prefP, PreferencesSortPopTrigger ) );
	ListType *	listP = FrmGetObjectPtr( prefP, FrmGetObjectIndex( prefP, PreferencesSortList ) );

	onlyActiveBefore = gGlobalPrefs.showOnlyActive;
	curSortOrder		= gMainDatabase.sortOrder;

	onlyactive = FrmGetObjectIndex( prefP, PreferencesOnlyActiveCheckbox );
	priority = FrmGetObjectIndex( prefP, PreferencesPriorityCheckbox );
	begindate = FrmGetObjectIndex( prefP, PreferencesBeginDateCheckbox );
	todostate = FrmGetObjectIndex( prefP, PreferencesToDoStateCheckbox );
	projectstate = FrmGetObjectIndex( prefP, PreferencesStateCheckbox );
	prjtcategories = FrmGetObjectIndex( prefP, PreferencesPrjtCategoriesCheckbox );

	labelP = (Char *)CtlGetLabel( popP );
	StrCopy( labelP, LstGetSelectionText( listP, gMainDatabase.sortOrder ) );
	CtlSetLabel( popP, labelP );
	LstSetSelection( listP, gMainDatabase.sortOrder );

	FrmSetControlValue( prefP, onlyactive, gGlobalPrefs.showOnlyActive );
	FrmSetControlValue( prefP, priority, gGlobalPrefs.showProjPriority );
	FrmSetControlValue( prefP, begindate, gGlobalPrefs.showBeginDate );
	FrmSetControlValue( prefP, todostate, gGlobalPrefs.showToDoInfo );
	FrmSetControlValue( prefP, projectstate, gGlobalPrefs.showProjState );
	FrmSetControlValue( prefP, prjtcategories, gGlobalPrefs.showPrjtCategories );

#ifdef CONFIG_HANDERA
	if( gGlobalPrefs.vgaExtension )
		VgaFormModify( prefP, rotateModeNone );
#endif /* CONFIG_HANDERA */

	if( FrmDoDialog( prefP ) != PreferencesOKButton )
	{
		FrmDeleteForm( prefP );
		return;
	}

	gGlobalPrefs.showOnlyActive = FrmGetControlValue( prefP, onlyactive );
	gGlobalPrefs.showProjPriority = FrmGetControlValue( prefP, priority );
	gGlobalPrefs.showBeginDate = FrmGetControlValue( prefP, begindate );
	gGlobalPrefs.showToDoInfo = FrmGetControlValue( prefP, todostate );
	gGlobalPrefs.showProjState = FrmGetControlValue( prefP, projectstate );
	gGlobalPrefs.showPrjtCategories = FrmGetControlValue( prefP, prjtcategories );
	gMainDatabase.sortOrder = LstGetSelection( listP );

	FrmDeleteForm( prefP );

	if( (curSortOrder != gMainDatabase.sortOrder) && gMainDatabase.sortOrder != kSortMainDBManually )
	{
		DmQuickSort( gMainDatabase.db, (DmComparF *)PrjtDBCompareProjectsRecords, gMainDatabase.sortOrder );
	}
	if( (onlyActiveBefore != gGlobalPrefs.showOnlyActive) || (curSortOrder != gMainDatabase.sortOrder) )
	{
#ifdef CONFIG_JOGDIAL
		gRowSelected = noRowSelected;
#endif /* CONFIG_JOGDIAL */
		MainFormLoadTable();
	}

	MainFormSetProjectTableColumns( tableP );
	TblEraseTable( tableP );
	TblDrawTable( tableP );
}

/*
 * FUNCTION:			MainFormSetProjectTableColumns
 * DESCRIPTION:		this routine resets the columns
 * 								and each column's spacing depending
 * 								on global variable indicating which
 * 								column should be visible and which
 * 								not
 * PARAMETERS:		tableP - pointer to the table on the main form
 * RETURNS:				nothing
 */
static void MainFormSetProjectTableColumns( TableType * tableP )
{
	RectangleType r;
	UInt16				width;

	TblGetBounds( tableP, &r );

	if( gGlobalPrefs.showPrjtCategories && gMainDatabase.currentCategory == dmAllCategories )
	{
		width = r.extent.x>>2;
		if( width > kMainFormCategoryColumnMinWidth )
			width = kMainFormCategoryColumnMinWidth;
		r.extent.x -= width;
		TblSetColumnWidth( tableP, kMainFormCategoryTableColumn, width );
	}

	if( gGlobalPrefs.showProjPriority )
	{
		width = kMainFormPriorityTableColumnMinWidth;
#ifdef CONFIG_HANDERA
		if( gGlobalPrefs.vgaExtension && VgaIsVgaFont( gGlobalPrefs.mainFont ) )
			width += 4;
#endif /* CONFIG_HANDERA */
		TblSetColumnWidth( tableP, kMainFormPriorityTableColumn, width );
		TblSetColumnSpacing( tableP, kMainFormPriorityTableColumn, 0 );
		r.extent.x -= width;
	}
	if( gGlobalPrefs.showBeginDate )
	{
		width = kMainFormBeginDateTableColumnMinWidth;
		switch( gGlobalPrefs.mainFont )
		{
			case boldFont:
				width += 4;
				break;
			case largeFont:
			case largeBoldFont:
				width += 7;
				break;
			default:
				break;
		}

#ifdef CONFIG_HANDERA
		if( gGlobalPrefs.vgaExtension && VgaIsVgaFont( gGlobalPrefs.mainFont ) )
		{
			switch( VgaVgaToBaseFont( gGlobalPrefs.mainFont ) )
			{
				case stdFont:
					width += 10;
					break;
				case boldFont:
					width += 22;
					break;
				case largeFont:
				case largeBoldFont:
					width += 26;
					break;
				default:
					break;
			}
		}
#endif /* CONFIG_HANDERA */

		TblSetColumnWidth( tableP, kMainFormBeginDateTableColumn, width );
		TblSetColumnSpacing( tableP, kMainFormBeginDateTableColumn, 0 );
		r.extent.x -= width;
	}

	// info table column width will be set later 

	if( gGlobalPrefs.showProjState )
	{
		width = kMainFormStateTableColumnMinWidth;
		switch( gGlobalPrefs.mainFont )
		{
			case stdFont:
				break;
			case boldFont:
				width += 2;
				break;
			case largeBoldFont:
				width += 2;
				break;
			default:
				break;
		}
#ifdef CONFIG_HANDERA
		if( gGlobalPrefs.vgaExtension && VgaIsVgaFont( gGlobalPrefs.mainFont ) )
			width += 4;
#endif /* CONFIG_HANDERA */
		TblSetColumnWidth( tableP, kMainFormStateTableColumn, width );
		TblSetColumnSpacing( tableP, kMainFormBeginDateTableColumn, 0 );
		r.extent.x -= width;
	}

	// note table column is always visible and will be set later

	r.extent.x -= (kMainFormMemoTableColumnMinWidth + 3);

	TblSetColumnWidth( tableP, kMainFormInfoTableColumn, r.extent.x );
	TblSetColumnSpacing( tableP, kMainFormInfoTableColumn, 1 );
	TblSetColumnWidth( tableP, kMainFormMemoTableColumn, kMainFormMemoTableColumnMinWidth );
	TblSetColumnSpacing( tableP, kMainFormMemoTableColumn, 2 );

	TblSetColumnUsable( tableP, kMainFormPriorityTableColumn, gGlobalPrefs.showProjPriority );
	TblSetColumnUsable( tableP, kMainFormBeginDateTableColumn, gGlobalPrefs.showBeginDate );
	TblSetColumnUsable( tableP, kMainFormStateTableColumn, gGlobalPrefs.showProjState );
	TblSetColumnUsable( tableP, kMainFormCategoryTableColumn, (gGlobalPrefs.showPrjtCategories && gMainDatabase.currentCategory == dmAllCategories) );
}

/*
 * FUNCTION:			MainFormSelectRecCategory
 * DESCRIPTION:		this functions pops up a list and lets the user
 * 								select an item which is a category and assigns this
 * 								category to the record specified by the rowID.
 * PARAMETERS:		tblP - pointer to our table
 * 								row, column - of the record in out table
 * RETURNS:				nothing
 */
static void MainFormSelectRecCategory( TableType * tblP, Int16 row, Int16 column )
{
	RectangleType r;
	ListType * 		lstP;
	Char *				catP;
	UInt16				recI;
	UInt16				attr;
	UInt16				category;
	Int16					curSel, newSel;
	Boolean				moved;

	ErrFatalDisplayIf( !gMainDatabase.db, "db not open" );

	// get a pointer to the list and the record index
	lstP = (ListType *)GetObjectPtr( MainRecCategoryList );
	recI = TblGetRowID( tblP, row );

	ErrFatalDisplayIf( !DmQueryRecord( gMainDatabase.db, recI ), "bad prjt record" );

	// get the record's attributes
	DmRecordInfo( gMainDatabase.db, recI, &attr, NULL, NULL );
	category = attr & dmRecAttrCategoryMask;

	// unhighlight the selection in the table
	TblUnhighlightSelection( tblP );

	// create the category list
	CategoryCreateList( gMainDatabase.db, lstP, category, false, true, 1, 0, true );

	// position the list
	TblGetItemBounds( tblP, row, column, &r );
	LstSetPosition( lstP, r.topLeft.x, r.topLeft.y );

	// now make the selection
	curSel = LstGetSelection( lstP );
	newSel = LstPopupList( lstP );
	if( (curSel != newSel) && (newSel != -1 ) )
	{
		// find the category index
		catP = LstGetSelectionText( lstP, newSel );
		category = CategoryFind( gMainDatabase.db, catP );

		// set the new category of the index
		attr &= ~dmRecAttrCategoryMask;
		attr |= category;
		DmSetRecordInfo( gMainDatabase.db, recI, &attr, NULL );

		if( gMainDatabase.sortOrder == kSortMainDBByCategoryPriorityStateName ||
				gMainDatabase.sortOrder == kSortMainDBByCategoryStatePriorityName ||
				gMainDatabase.sortOrder == kSortMainDBByCategoryName )
			moved = PrjtDBReSortChangedProjectRecord( gMainDatabase.db, gMainDatabase.sortOrder, &recI );
		else
			moved = false;

		// draw the row to apply the new settings visually
		if( moved )
			MainFormLoadTable();
		else
			TblMarkRowInvalid( tblP, row );
#ifdef CONFIG_JOGDIAL
		gRowSelected = noRowSelected;
#endif /* CONFIG_JOGDIAL */
		TblRedrawTable( tblP );
	}

	// free the list previously created
	CategoryFreeList( gMainDatabase.db, lstP, false, 0 );
}

/*
 * FUNCTION:			MainFormSelectInfoFromList
 * DESCRIPTION:		depending on the column parameter
 * 								this routine pops up a list and
 * 								assignes a value to the project
 * 								specified by the RowID of the row parameter.
 *								this routine is a handler for the tblSelect
 *								event on the priority and state column of our
 *								main form table.
 * PARAMETERS:		tableP - pointer to the main form table
 * 								row - row the user has selected
 * 								column - column the user has tapped in
 * RETURNS:				nothing
 */
static void	MainFormSelectInfoFromList( TableType *tableP, Int16 row, Int16 column )
{
	RectangleType r;
	PrjtPackedProjectType * recP;
	ListType *		listP = NULL;
	Int16					index = 0;
	Int16					newIndex;
	UInt16				recIndex;
	Boolean				moved = false;
	UInt8					priority_status = 0;
	
	ErrFatalDisplayIf( column != kMainFormPriorityTableColumn &&
											column != kMainFormStateTableColumn,
											"invalid column param" );

	// get the record -----------------------------------------------------------
	recIndex = TblGetRowID( tableP, row );
	recP = GetProjectRecord( recIndex, true );
	ErrFatalDisplayIf( !recP, "bad record" );

	// get the list -------------------------------------------------------------
	if( column == kMainFormPriorityTableColumn )
	{
		listP = (ListType *)GetObjectPtr( MainPriorityList );
		index = (PrjtDBProjectsPriority( recP->priority_status ) - 1);
	}
	else //if( column == kMainFormStateTableColumn )
	{
		listP = (ListType *)GetObjectPtr( MainStateList );
		index = (PrjtDBProjectsState( recP->priority_status ) - 1);
	}

	// display the list and get the user's selection ----------------------------
	LstSetSelection( listP, index );
	TblGetItemBounds( tableP, row, column, &r );
	LstSetPosition( listP, r.topLeft.x, r.topLeft.y );
	TblUnhighlightSelection( tableP );
	newIndex = LstPopupList( listP );

	// is there a new seletion ? ------------------------------------------------
	if(	(newIndex == -1) || (newIndex == index) )
	{
		MemPtrUnlock( recP );
		DmReleaseRecord( gMainDatabase.db, recIndex, false );
		return;
	}

	// update the record --------------------------------------------------------
	if( column == kMainFormPriorityTableColumn )
	{
		priority_status = (recP->priority_status & kStateMask);
		priority_status |= ((UInt8)(newIndex+1))<<4;
		DmWrite( recP, OffsetOf( PrjtPackedProjectType, priority_status ), &priority_status, sizeof( recP->priority_status ) );
	}
	else
	{
		priority_status = (recP->priority_status & kPriorityMask); 
		priority_status |= ((UInt8)(newIndex+1));
		DmWrite( recP, OffsetOf( PrjtPackedProjectType, priority_status ), &priority_status, sizeof( recP->priority_status ) );
	}

	MemPtrUnlock( recP );
	DmReleaseRecord( gMainDatabase.db, recIndex, true );

	// reposition the record in our main database -------------------------------
	if( gMainDatabase.sortOrder == kSortMainDBByPriorityStateName || 
			gMainDatabase.sortOrder == kSortMainDBByStatePriorityName ||
			gMainDatabase.sortOrder == kSortMainDBByCategoryPriorityStateName ||
			gMainDatabase.sortOrder == kSortMainDBByCategoryStatePriorityName )
		moved = PrjtDBReSortChangedProjectRecord( gMainDatabase.db, gMainDatabase.sortOrder, &recIndex );
	else
		moved = false;

	// update the main form table -----------------------------------------------
	// if only active project should be shown and the new listselection
	// indicated that the project is no more active remove the project from
	// our list
	if( (column == kMainFormStateTableColumn) && (newIndex != 0) && gGlobalPrefs.showOnlyActive )
		MainFormLoadTable();
	else if( !moved )
	{
		TblSetItemInt( tableP, row, column, newIndex+1 );
		TblMarkRowInvalid( tableP, row );
	}
	else
		MainFormLoadTable();

#ifdef CONFIG_JOGDIAL
	gRowSelected = noRowSelected;
#endif /* CONFIG_JOGDIAL */
	TblRedrawTable( tableP );
}

/*
 * FUNCTION:			MainFormSelectCategory
 * DESCRIPTION:		lets the use choose a category for the main form
 * 								if the user choosed one it updates the view
 * PARAMETERS:		none
 * RETURNS:				nothing
 * CALLED:				each time the user taps the category popup trigger
 */
static void	MainFormSelectCategory( void )
{
	Boolean categoryEdited;
	UInt16	category;
	Char *	labelP;
	TableType *tblP;

	labelP = (Char *)CtlGetLabel( GetObjectPtr( MainCategoriesPopTrigger ) );
	category = gMainDatabase.currentCategory;
	categoryEdited = CategorySelect( gMainDatabase.db, FrmGetActiveForm(), MainCategoriesPopTrigger, MainCategoriesList, true, &category, labelP, 1, 0 );
	if( categoryEdited || (category != gMainDatabase.currentCategory) )
	{
		categoryEdited = false;
		if( gGlobalPrefs.showPrjtCategories )
			categoryEdited = (category == dmAllCategories || gMainDatabase.currentCategory == dmAllCategories);

		gMainDatabase.currentCategory = category;
		gTopVisibleProjectIndex = 0;
		MainFormLoadTable();
		tblP = (TableType *)GetObjectPtr( MainProjectTable );

		if( categoryEdited )
			MainFormSetProjectTableColumns( tblP );
#ifdef CONFIG_JOGDIAL
		gRowSelected = noRowSelected;
#endif /* CONFIG_JOGDIAL */
		TblRedrawTable( tblP );
	}
}

/*
 * FUNCTION:			MainFormDrawProject
 * DESCRIPTION:		this routine is a callback function for the table manager
 * 								and gets called each time a cell needs to be redrawn
 * 								depending on the row and column parameter it draws the
 * 								appropriate information into the cell.
 * PARAMETERS:		see palm sdk
 * RETURNS:				nothing
 */
static void	MainFormDrawProject( void *tableP, Int16 row, Int16 column, RectangleType * bounds )
{
	Char 				szBuffer[32];
	Char *			s;
	PrjtPackedProjectType * recP;
	UInt16 			pixLen;
	UInt16			len;
	Int16				width;
	FontID			prevFont;
	Boolean			fitInWidth;
#ifdef CONFIG_COLOR
	Boolean			grayout = false;
	IndexedColorType prvClr = 0;
#endif

	recP = GetProjectRecord( TblGetRowID( tableP, row ), false );
	if( recP )
	{
#ifdef CONFIG_COLOR
		grayout = (PrjtDBProjectsState( recP->priority_status ) != kState1 && !gGlobalPrefs.showOnlyActive);
		if( gGlobalPrefs.useColors && grayout )
			prvClr = WinSetTextColor( GRAY_COLOR_INDEX );
#endif
		prevFont = FntSetFont( gGlobalPrefs.mainFont );

		switch( column )
		{
			case kMainFormPriorityTableColumn:
#ifdef CONFIG_HANDERA
				len = gGlobalPrefs.mainFont;
				if( gGlobalPrefs.vgaExtension && VgaIsVgaFont( gGlobalPrefs.mainFont ) )
				{
					len = VgaVgaToBaseFont( gGlobalPrefs.mainFont );
					if( len == largeFont )
						FntSetFont( VgaBaseToVgaFont( largeBoldFont ) );
					else if( len == stdFont )
						FntSetFont( gGlobalPrefs.mainFont + 1 );
				}
				else
#endif /* CONFIG_HANDERA */
				{
					if( gGlobalPrefs.mainFont == stdFont )
						FntSetFont( boldFont );
					else if( gGlobalPrefs.mainFont == largeFont )
						FntSetFont( largeBoldFont );
				}

				szBuffer[0] = TblGetItemInt( tableP, row, column ) + '0';
				WinDrawChars( szBuffer, 1, (bounds->topLeft.x + bounds->extent.x) - (FntCharWidth( szBuffer[0] ) + 1), bounds->topLeft.y );
				break;

			case kMainFormBeginDateTableColumn:
				s = DateToAsciiWithoutYear( recP->begin, szBuffer );
				len = StrLen( s );
				pixLen = FntCharsWidth( s, len );
				WinDrawChars( s, len, bounds->topLeft.x + ((bounds->extent.x - pixLen)>>1), bounds->topLeft.y );
				break;

			case kMainFormInfoTableColumn:
				// todos state info ---------------------------------------------------
				if( gGlobalPrefs.showToDoInfo )
				{
					StrIToA( szBuffer, recP->numOfFinishedToDos );
					s = szBuffer;
					while( *s )
						s++;
					*s++ = '/';
					StrIToA( s, recP->numOfAllToDos );
					len = StrLen( szBuffer );

					pixLen = FntCharsWidth( szBuffer, len );
					WinDrawChars( szBuffer, len, bounds->topLeft.x + bounds->extent.x - pixLen - 1,  bounds->topLeft.y );
				}
				else
					pixLen = 0;

				// name ---------------------------------------------------------------
				width = bounds->extent.x - pixLen - 1;
				len = StrLen( &(recP->name) );
				WinDrawTruncChars( &recP->name, len, bounds->topLeft.x + 1, bounds->topLeft.y, width );

#ifdef CONFIG_JOGDIAL
				if( gRowSelected == row )
				{
					RctInsetRectangle( bounds, 1 );
					WinDrawGrayRectangleFrame( rectangleFrame, bounds );
					RctInsetRectangle( bounds, -1 );
				}
#endif /* CONFIG_JOGDIAL */
				break;

			case kMainFormStateTableColumn:
				// len is used as 'font2set' ;-o
				switch( PrjtDBProjectsState( recP->priority_status ) )
				{
					case kState1:
						len = kState1CharFont;
						szBuffer[0] = kState1Char;
						break;
					case kState2:
						len = kState2CharFont;
						szBuffer[0] = kState2Char;
						break;
					case kState3:
						len = kState3CharFont;
						szBuffer[0] = kState3Char;
						break;
					case kState4:
						len = kState4CharFont;
						szBuffer[0] = kState4Char;
						break;
					case kState5:
						len = kState5CharFont;
						szBuffer[0] = kState5Char;
						break;
				}
#ifdef CONFIG_HANDERA
				if( gGlobalPrefs.vgaExtension && VgaIsVgaFont( gGlobalPrefs.mainFont ) )
					len = VgaBaseToVgaFont( len );
#endif /* CONFIG_HANDERA */
				FntSetFont( len );
				WinDrawChars( szBuffer, 1, bounds->topLeft.x, bounds->topLeft.y );
				break;

			case kMainFormMemoTableColumn:
				if( *(&recP->name + StrLen( &recP->name ) + 1 ) )
				{
#ifdef CONFIG_HANDERA
					if( gGlobalPrefs.vgaExtension && VgaIsVgaFont( gGlobalPrefs.mainFont ) )
						FntSetFont( VgaBaseToVgaFont( symbolFont ) );
					else
#endif /* CONFIG_HANDERA */
						FntSetFont( symbolFont );
					szBuffer[0] = 0x0B;
					WinDrawChars( szBuffer, 1, bounds->topLeft.x, bounds->topLeft.y );
				}
				break;

			case kMainFormCategoryTableColumn:
				// temporarily we use the 'width' variable
				DmRecordInfo( gMainDatabase.db, TblGetRowID( tableP, row ), &width, NULL, NULL );
				CategoryGetName( gMainDatabase.db, width & dmRecAttrCategoryMask, szBuffer );

				width = bounds->extent.x;
				len = StrLen( szBuffer );
				FntCharsInWidth( szBuffer, &width, &len, &fitInWidth );
#if 0
				if( !fitInWidth )
				{
					ChrHorizEllipsis( &horzEllipsis );
					width -= FntCharWidth( horzEllipsis );
					FntCharsInWidth( szBuffer, &width, &len, &fitInWidth );
					szBuffer[len] = horzEllipsis;
					len++;
				}
#endif
				WinDrawChars( szBuffer, len, bounds->topLeft.x, bounds->topLeft.y );
				break;

			default:
				ErrDisplay( "invalid column" );
				break;
		}	

		MemPtrUnlock( recP );
		FntSetFont( prevFont );
#ifdef CONFIG_COLOR
		if( gGlobalPrefs.useColors && grayout )
			WinSetTextColor( prvClr );
#endif
	}
}
		
/*
 * FUNCTION:			MainFormLoadTable
 * DESCRIPTION:		loads our table on the main form with data
 * 								it also updates the scroller buttons of the form
 * PARAMETERS:		none
 * RETURNS:				nothing
 */
static void MainFormLoadTable( void )
{
	RectangleType 					tableR;
	PrjtPackedProjectType * recP;
	FormType	* 						frmP;
	TableType * 						tableP;
	Boolean									masked;
	UInt16									attr;
	UInt16									numRows;
	UInt16									i;
	UInt16									lineHeight;
	UInt16									recIndex;
	UInt16									numFitInTable;
	UInt16									upIndex, downIndex;
	Int16										lastVisibleRecordIndex;
	FontID									curFont;
	Boolean									upEnable, downEnable;

	frmP = FrmGetActiveForm();
	tableP = FrmGetObjectPtr( frmP, FrmGetObjectIndex( frmP, MainProjectTable  ) );
	TblGetBounds( tableP, &tableR );

	curFont = FntSetFont( gGlobalPrefs.mainFont );
	lineHeight = FntLineHeight();
	FntSetFont( curFont );
	
	numRows = TblGetNumberOfRows( tableP );
	numFitInTable = (tableR.extent.y+1) / lineHeight;
	if( numFitInTable > numRows )
		numFitInTable = numRows;

	TblUnhighlightSelection( tableP );
	lastVisibleRecordIndex = gTopVisibleProjectIndex;
	recIndex = gTopVisibleProjectIndex;

	if(gGlobalPrefs.loginName[0] == 0) {
		numFitInTable = 0;
	}
	
	for( i = 0; i < numFitInTable; i++ )
	{
		if( SeekProjectRecord( &recIndex, 0, dmSeekForward ) )
		{
			masked = false;
			TblSetRowUsable( tableP, i, true );
			TblSetRowID( tableP, i, recIndex );

	#ifdef CONFIG_OS_BELOW_35
			if( gGlobalPrefs.rom35present )
			{
	#endif /* CONFIG_OS_BELOW_35 */

				DmRecordInfo( gMainDatabase.db, recIndex, &attr, NULL, NULL );
				masked = ((gGlobalPrefs.privateRecordStatus == maskPrivateRecords) && (attr & dmRecAttrSecret));
				TblSetRowMasked( tableP, i, masked );

	#ifdef CONFIG_OS_BELOW_35
			}
	#endif /* CONFIG_OS_BELOW_35 */
			
			if( !masked )
			{
				recP = GetProjectRecord( recIndex, false );
				ErrFatalDisplayIf( !recP, "bad record" );
				TblSetItemInt( tableP, i, kMainFormPriorityTableColumn, PrjtDBProjectsPriority( recP->priority_status ) );
				MemPtrUnlock( recP );
			}

			TblMarkRowInvalid( tableP, i );
			lastVisibleRecordIndex = recIndex;

			recIndex++;
		}
		else
			break;
	}

	while( i < numRows )
	{
		//TblSetRowID( tableP, i, kProjectNoIndex );
		TblSetRowUsable( tableP, i, false );
		i++;
	}

	// update the scroller buttons ----------------------------------------------
	recIndex = gTopVisibleProjectIndex;
	upEnable = SeekProjectRecord( &recIndex, 1, dmSeekBackward );
	downEnable = SeekProjectRecord( &lastVisibleRecordIndex, 1, dmSeekForward );
	upIndex = FrmGetObjectIndex( frmP, MainScrollUpRepeating );
	downIndex = FrmGetObjectIndex( frmP, MainScrollDownRepeating );
	FrmUpdateScrollers( frmP, upIndex, downIndex, upEnable, downEnable );
}



/*
 * FUNCTION:			MainFormHandleNewProjectRequest
 * DESCRIPTION:		this routine handles the ctlSelect
 * 								event on the "new" button. it pops
 * 								up a dialog for the user to enter
 * 								and project name and creates the
 * 								database as well as the item in
 * 								the main db
 * 								if the all goes alright, this routine
 * 								adds an event into the eventqueue to
 * 								go to the ProjectForm
 * PARAMETERS:		none
 * RETURNS:				nothing
 */
static void MainFormHandleNewProjectRequest( void )
{
	PrjtUnpackedProjectType	project;
	FieldType 	* fieldP;
	FormType 		*	frmP; 
	FormType 		*	newP;
	TableType * 			tableP;
	UInt16				fieldIndex;
	DmOpenRef			db = 0;
	UInt16				index = kNoRecordIndex;
	const Char surveyPrjName[] = "Nursing Survey";
	ControlType * 		popP;
	ListType *			listP;
	Int16				curSel;
	Char *				labelP;

	// get the name for the new database ----------------------------------------
	frmP = FrmGetActiveForm();
	newP = FrmInitForm( ProjectNameForm );
	//fieldIndex = FrmGetObjectIndex( newP, ProjectNameFormNameField );
	//FrmSetEventHandler( newP, ProjectNameFormHandleEvent );

	//popP = FrmGetObjectPtr( newP, FrmGetObjectIndex( newP, LoginNamePopTrigger ) );
	listP = FrmGetObjectPtr( newP, FrmGetObjectIndex( newP, LoginNameList ) );
	//labelP = (Char *)CtlGetLabel( popP );
	//StrCopy( labelP, LstGetSelectionText( listP, 0 ) );
	//CtlSetLabel( popP, labelP );
	//LstSetSelection( listP, 0 );
	
	FrmSetActiveForm( newP );
	FrmSetFocus( newP, fieldIndex );

#ifdef CONFIG_HANDERA
	if( gGlobalPrefs.vgaExtension )
		VgaFormModify( newP, rotateModeNone );
#endif /* CONFIG_HANDERA */

	if( FrmDoDialog( newP ) == ProjectNameFormOKButton )
	{
		curSel = LstGetSelection( listP );
		StrNCopy(gGlobalPrefs.loginName,LstGetSelectionText( listP, curSel) ,kLoginNameMaxLen);
		gGlobalPrefs.loginName[kLoginNameMaxLen-1]=0;
		//StrNCopy(gGlobalPrefs.loginName,"Hofstadter,Leonard",kLoginNameMaxLen);

		// if we get here, we can rely on the uniqueness of the name
		//fieldP = FrmGetObjectPtr( newP, fieldIndex );
		//FldCompactText( fieldP );

		// create the database ----------------------------------------------------
		if(!FindDatabaseByProjectName(surveyPrjName)){
			db = PrjtDBCreateToDoDB( surveyPrjName, true );
			if( !db )
				ShowGeneralAlert( ErrorDBNotCreated );
			else
			{
				DmCloseDatabase( db );

				// create the record in our main database -------------------------------
				DateSecondsToDate( TimGetSeconds(), &(project.begin) );
				*((UInt16 *)(&project.end)) = kNoDate;
				project.priority = kProjectsPriority1;
				project.status = kProjectsState1;
				project.numOfFinishedToDos = 0;
				project.numOfAllToDos = 0;
				project.name = surveyPrjName;
				project.note = NULL; // "hello world ;-)";
				index = PrjtDBCreateProjectRecord( gMainDatabase.db, &project, gMainDatabase.currentCategory, gMainDatabase.sortOrder );

				if( index == kNoRecordIndex )
					ShowGeneralAlert( ErrorDBNotCreated );
			}
		}
	}

	if( frmP )
		FrmSetActiveForm( frmP );
	FrmDeleteForm( newP );

	if( db )
	{
		gCurrentProject.projIndex = index;
		gCurrentProject.currentPage = ToDoPage;
		FrmGotoForm( ProjectForm );
	}

	frmP = FrmGetActiveForm();
	MainFormInit( frmP );
	FrmDrawForm ( frmP );
}

/*
 * FUNCTION:				MainFormInit
 * DESCRIPTION:			this routine initialize
 * 									the main form (table)
 * 									and switches the menu
 * 									depending on the rom version.
 * PARAMETERS:			frmP - pointer to the main form
 * RETURNS:					nothing
 */
static void MainFormInit( FormType * frmP )
{
	TableType * 	tableP;
	ControlType *	controlP;
	Char *				labelP;
	UInt16			i, j;
	UInt16			numRows;
	UInt16			lineHeight;
	FontID			font;

	ErrFatalDisplayIf( !frmP, "invalid param" );

	font = FntSetFont( gGlobalPrefs.mainFont );
	lineHeight = FntLineHeight();
	FntSetFont( font );

 	tableP = (TableType *)GetObjectPtr( MainProjectTable );

#ifdef CONFIG_HANDERA
	if( gGlobalPrefs.vgaExtension )
	{
		VgaFormModify( frmP, vgaFormModify160To240 );
		VgaTableUseBaseFont( tableP, true );
	}
#endif /* CONFIG_HANDERA */

	// initialize our table on the main form ------------------------------------
 	numRows = TblGetNumberOfRows( tableP );
	for( i = 0; i < numRows; i++ )
	{
		TblSetRowHeight( tableP, i, lineHeight );

		TblSetItemStyle( tableP, i, kMainFormPriorityTableColumn, customTableItem );
		TblSetItemStyle( tableP, i, kMainFormBeginDateTableColumn, customTableItem );
		TblSetItemStyle( tableP, i, kMainFormInfoTableColumn, customTableItem );
		TblSetItemStyle( tableP, i, kMainFormStateTableColumn, customTableItem );
		TblSetItemStyle( tableP, i, kMainFormMemoTableColumn, customTableItem );
		TblSetItemStyle( tableP, i, kMainFormCategoryTableColumn, customTableItem );

		//we do not need to check the os version as the required os version >= 3.1 :-)
		for( j = kMainFormPriorityTableColumn; j <= kMainFormCategoryTableColumn; j++ )
			TblSetItemFont(tableP, i, j, gGlobalPrefs.mainFont);
	}
	TblSetColumnUsable( tableP, kMainFormInfoTableColumn, true );
	TblSetColumnUsable( tableP, kMainFormMemoTableColumn, true );
	MainFormSetProjectTableColumns( tableP );

	TblSetCustomDrawProcedure( tableP, kMainFormPriorityTableColumn, MainFormDrawProject );
	TblSetCustomDrawProcedure( tableP, kMainFormBeginDateTableColumn, MainFormDrawProject );
	TblSetCustomDrawProcedure( tableP, kMainFormInfoTableColumn, MainFormDrawProject );
	TblSetCustomDrawProcedure( tableP, kMainFormStateTableColumn, MainFormDrawProject );
	TblSetCustomDrawProcedure( tableP, kMainFormMemoTableColumn, MainFormDrawProject );
	TblSetCustomDrawProcedure( tableP, kMainFormCategoryTableColumn, MainFormDrawProject );

#ifdef CONFIG_OS_BELOW_35
	if( gGlobalPrefs.rom35present )
	{
#endif /* CONFIG_OS_BELOW_35 */

		for( i = kMainFormPriorityTableColumn; i <= kMainFormCategoryTableColumn; i++ )
			TblSetColumnMasked( tableP, i, true );

#ifdef CONFIG_OS_BELOW_35
	}
#endif /* CONFIG_OS_BELOW_35 */

	// the the form category trigger --------------------------------------------
	controlP = FrmGetObjectPtr( frmP, FrmGetObjectIndex( frmP, MainCategoriesPopTrigger ) );
	labelP = (Char *)CtlGetLabel( controlP );
	CategoryGetName( gMainDatabase.db, gMainDatabase.currentCategory, labelP );
	CategorySetTriggerLabel( controlP, labelP );

	if(gGlobalPrefs.loginName[0] != 0) {
		CtlSetLabel(FrmGetObjectPtr( frmP, FrmGetObjectIndex( frmP, MainNewButton ) ),gGlobalPrefs.loginName);
	}

#ifdef CONFIG_HANDERA
	if( gGlobalPrefs.vgaExtension )
		MainFormResize( frmP, false );
#endif /* CONFIG_HANDERA */

	// load the table with data -------------------------------------------------
	gTopVisibleProjectIndex = 0;
	MainFormLoadTable();

	// set the right menu -------------------------------------------------------
#ifdef CONFIG_OS_BELOW_35
	if( !gGlobalPrefs.rom35present )
		FrmSetMenu(frmP, MainFormMenu);
#endif /* CONFIG_OS_BELOW_35 */

#ifdef CONFIG_JOGDIAL
	// initialiy the gRowSelected variable
	gRowSelected = noRowSelected;
#endif /* CONFIG_JOGDIAL */


}

/*
 * FUNCTION:					MainFormSelectSecurity
 * DESCRIPTION:				this routine lets the user select the private level
 * 										and reloads the table if necessary
 * PARAMETERS:				none
 * RETURNS:						nothing
 */
static void MainFormSelectSecurity( void )
{
	UInt16	prevStatus;
	UInt16	mode;
	TableType * tableP;

	prevStatus = gGlobalPrefs.privateRecordStatus;
	gGlobalPrefs.privateRecordStatus = SecSelectViewStatus();
	if( prevStatus != gGlobalPrefs.privateRecordStatus )
	{
		// reopen the database
		DmCloseDatabase( gMainDatabase.db );
		mode = (gGlobalPrefs.privateRecordStatus == hidePrivateRecords) ? dmModeReadWrite : (dmModeReadWrite | dmModeShowSecret);
		gMainDatabase.db = PrjtDBGetMainDB( true, mode );
		ErrFatalDisplayIf( !gMainDatabase.db, "database not opened" );

		gTopVisibleProjectIndex = 0;
		MainFormLoadTable();
		tableP = (TableType *)GetObjectPtr( MainProjectTable );
		TblRedrawTable( tableP );
	}
}

/*
 * FUNCTION:			MainFormScrollTable
 * DESCRIPTION:		depending on the scrollTbl parameter
 * 								this function computes the gTopVisibleProjectIndex
 * 								and reloads the table
 * PARAMETERS:		scrollTbl - idicating how to scroll
 * RETURNS:				nothing
 */
static void MainFormScrollTable( ScrollTableType scrollTbl )
{
	RectangleType	tblR;
	TableType * tableP;
	UInt16			recIndex;
	UInt16			lastRow;
	UInt16			numRows;
	UInt16			lineHeight;
	FontID			font;
	Boolean			doScroll = false;

	// get some values for later use --------------------------------------------
	tableP  = (TableType *)GetObjectPtr( MainProjectTable );
	TblGetBounds( tableP, &tblR );
	font = FntSetFont( gGlobalPrefs.mainFont );
	lineHeight = FntLineHeight();
	FntSetFont( font );
	numRows = tblR.extent.y / lineHeight;
	if( numRows > TblGetNumberOfRows( tableP ) )
		numRows = TblGetNumberOfRows( tableP );

	// now do the scrolling -----------------------------------------------------
	switch( scrollTbl )
	{
		case scrollTblOneUp:
		case scrollTblPageUp:
			recIndex = gTopVisibleProjectIndex;
			if( SeekProjectRecord( &recIndex, 1, dmSeekBackward ) )
			{
				doScroll = true;
				gTopVisibleProjectIndex = recIndex;
				if( scrollTbl == scrollTblPageUp )
					while( numRows-- > 2 )
						if( SeekProjectRecord( &recIndex, 1, dmSeekBackward ) )
							gTopVisibleProjectIndex = recIndex;
			}
			break;

			recIndex = gTopVisibleProjectIndex;
			break;

		case scrollTblOneDown:
		case scrollTblPageDown:
			lastRow = TblGetLastUsableRow( tableP );
			recIndex = TblGetRowID( tableP, lastRow );
			if( SeekProjectRecord( &recIndex, 1, dmSeekForward ) )
			{
				doScroll = true;
				SeekProjectRecord( &gTopVisibleProjectIndex, 1, dmSeekForward );
				if( scrollTbl == scrollTblPageDown )
					while( numRows-- > 2 )
						if( SeekProjectRecord( &recIndex, 1, dmSeekForward ) )
							SeekProjectRecord( &gTopVisibleProjectIndex, 1, dmSeekForward );
			}
			break;
	}

	// reload and redraw the table ----------------------------------------------
	if( doScroll )
	{
		MainFormLoadTable();
		TblRedrawTable( tableP );
	}
}	

/*
 * FUNCTION:			MainFormSelectProjectByChar
 * DESCRIPTION:		this routine selects the next
 * 								project on the main form that
 * 								starts with a specified character
 * PARAMETERS:		c - character to search projects
 * RETURNS:				true - if a next record was found
 * 								false - if a record beginning with
 * 												c does not exist in the table
 */
static Boolean MainFormSelectProjectByChar( Char c )
{
	PrjtPackedProjectType * recP;
	TableType *	tableP;
	UInt16			recIndex;
	Int16				row;
	Boolean 		first;
	Boolean			fromZero;
	Char 				recC;

	if( IsCharSmallLetter( c ) )
		c -= ('a' - 'A');

	tableP = (TableType *)GetObjectPtr( MainProjectTable );

#ifdef CONFIG_JOGDIAL
	if( gRowSelected != noRowSelected )
	{
		TblMarkRowInvalid( tableP, gRowSelected );
		gRowSelected = noRowSelected;
		TblRedrawTable( tableP );
	}
#endif /* CONFIG_JOGDIAL */

	if( gCurrentProject.projIndex == kProjectNoIndex )
	{
		fromZero = true;
		recIndex = 0;
	}
	else
	{
		fromZero = false;
		recIndex = gCurrentProject.projIndex;
	}

	first = true;
	while( true )
	{
		if( first && gCurrentProject.projIndex != kProjectNoIndex )
		{
			first = false;
			recIndex++;
		}

		if( !SeekProjectRecord( &recIndex, 0, dmSeekForward ) )
		{
			if( gCurrentProject.projIndex == kProjectNoIndex )
			{
				TblUnhighlightSelection( tableP );
				return (false);
			}
			else if( !fromZero )
			{
				recIndex = 0;
				fromZero = true;
				continue;
			}
			else
			{
				TblUnhighlightSelection( tableP );
				return (false);
			}
		}

		recP = GetProjectRecord( recIndex, false );
		if( !recP )
		{
			TblUnhighlightSelection( tableP );
			return (false);
		}
		recC = recP->name;
		MemPtrUnlock( recP );

		if( IsCharSmallLetter( recC ) )
			recC -= ('a' - 'A');

		if( recC == c )
		{
			gCurrentProject.projIndex = recIndex;
			break;
		}

		recIndex++;
	}

	if( !TblFindRowID( tableP, gCurrentProject.projIndex, &row ) )
	{
		if( gTopVisibleProjectIndex > gCurrentProject.projIndex )
		{
			gTopVisibleProjectIndex = gCurrentProject.projIndex;
			row = 0;
		}
		else
		{
			row = TblGetLastUsableRow( tableP );
			recIndex = TblGetRowID( tableP, row );
			if( gCurrentProject.projIndex > recIndex )
			{
				while( recIndex < gCurrentProject.projIndex )
				{
					if( SeekProjectRecord( &recIndex, 1, dmSeekForward ) )
						SeekProjectRecord( &gTopVisibleProjectIndex, 1, dmSeekForward );
				}
			}
		}

		MainFormLoadTable();
		TblRedrawTable( tableP );
		ErrFatalDisplayIf( !TblFindRowID( tableP, gCurrentProject.projIndex, &row ), "project not in table" );
	}
	TblSelectItem( tableP, row, kMainFormInfoTableColumn );

	return (true);
}

// exported function ------------------------------------------------------------------------------

/*
 * FUNCTION:				MainFormHandleEvent
 * DESCRIPTION:			this is the event handler for the main form
 * PARAMETRS:				eventP - the event
 * RETURNS:					true if event handled, else false
 */
Boolean MainFormHandleEvent( EventType * eventP )
{
  Boolean 		handled = false;
  FormType	* frmP;

	switch( eventP->eType ) 
	{
		case keyDownEvent:
			if( ChrIsHardKey( eventP->data.keyDown.chr ) )
			{
				if( !(eventP->data.keyDown.modifiers & poweredOnKeyMask) )
				{
					MainFormNextCategory();
					handled = true;
				}
			}
			else if( eventP->data.keyDown.chr == pageUpChr )
			{
				if( CtlEnabled( GetObjectPtr( MainScrollUpRepeating ) ) )
				{
#ifdef CONFIG_JOGDIAL
					gRowSelected = noRowSelected;
#endif /* CONFIG_JOGDIAL */
					MainFormScrollTable( scrollTblPageUp );
				}
				handled = true;
			}
			else if( eventP->data.keyDown.chr == pageDownChr )
			{
				if( CtlEnabled( GetObjectPtr( MainScrollDownRepeating ) ) )
				{
#ifdef CONFIG_JOGDIAL
					gRowSelected = noRowSelected;
#endif /* CONFIG_JOGDIAL */
					MainFormScrollTable( scrollTblPageDown );
				}
				handled = true;
			}
#ifdef CONFIG_JOGDIAL
	#ifdef CONFIG_SONY
			else if( eventP->data.keyDown.chr == vchrJogDown )
	#else
				// applies to handera and normal version
			else if( eventP->data.keyDown.chr == vchrNextField )
	#endif /* CONFIG_SONY */
			{
				MainFormSelectItem( selectNext );
				handled = true;
			}
	#ifdef CONFIG_SONY
			else if( eventP->data.keyDown.chr == vchrJogUp )
	#else
				// applies to normal and handera version
			else if( eventP->data.keyDown.chr == vchrPrevField )
	#endif /* CONFIG_SONY */
			{
				MainFormSelectItem( selectPrev );
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
				if( gRowSelected == noRowSelected )
					MainFormSelectItem( selectNext );
				else
				{
					gCurrentProject.projIndex = TblGetRowID( (TableType *)GetObjectPtr( MainProjectTable ), gRowSelected );
					gCurrentProject.currentPage = ToDoPage;
					FrmGotoForm( ProjectForm );
				}
				handled = true;
			}
#endif /* CONFIG_JOGDIAL */
			else if( IsCharPrintable( eventP->data.keyDown.chr ) )
			{
				handled = MainFormSelectProjectByChar( eventP->data.keyDown.chr );
			}
			break;

		case menuEvent:
			MenuEraseStatus(0);
#ifdef CONFIG_JOGDIAL
			if( gRowSelected != noRowSelected )
			{
				TableType * tblP = (TableType *)GetObjectPtr( MainProjectTable );
				TblMarkRowInvalid( tblP, gRowSelected );
				gRowSelected = noRowSelected;
				TblRedrawTable( tblP );
			}
#endif /* CONFIG_JOGDIAL */
			if( eventP->data.menu.itemID == OptionsAbout )
			{
				ShowAbout();
				handled = true;
			}
			else if( eventP->data.menu.itemID == OptionsSecurity )
			{
				MainFormSelectSecurity();
				handled = true;
			}
			else if( eventP->data.menu.itemID == OptionsFont )
			{
				MainFormSelectFont();
				handled = true;
			}
			else if( eventP->data.menu.itemID == OptionsScanForDBs )
			{
				MainFormScan4DBs();
				handled = true;
			}
#ifdef CONFIG_DEF_CATEGORY
			else if( eventP->data.menu.itemID == OptionsDefCategories )
			{
				FrmPopupForm( DefaultCategoriesForm );
				handled = true;
			}
#endif /* CONFIG_DEF_CATEGORY */
			break;

#ifdef CONFIG_ADDITIONAL
		case menuCmdBarOpenEvent:
			MenuCmdBarAddButton( menuCmdBarOnRight, BarInfoBitmap, menuCmdBarResultMenuItem, OptionsAbout, NULL );
			MenuCmdBarAddButton( menuCmdBarOnRight, BarSecureBitmap, menuCmdBarResultMenuItem, OptionsSecurity, 0);

			// prevent the system to add the field button we have already added
			eventP->data.menuCmdBarOpen.preventFieldButtons = true;
			// leave the event unhandled so the system can catch it
			break;
#endif /* CONFIG_ADDITIONAL */

		case frmOpenEvent:
			frmP = FrmGetActiveForm();
			MainFormInit( frmP );
			FrmDrawForm ( frmP );
			handled = true;
			break;

		case ctlSelectEvent:
			switch( eventP->data.ctlSelect.controlID )
			{
				case MainNewButton:
					MainFormHandleNewProjectRequest();
					handled = true;
					break;

				case MainShowButton:
					MainFormHandlePreferencesSelection();
					handled = true;
					break;

#ifdef CONFIG_ALLTODOS_DLG
        case MainToDoButton:
          MainFormHandleToDoButton();
          handled = true;
          break;
#endif

				case MainCategoriesPopTrigger:
					MainFormSelectCategory();
					handled = true;
					break;
				
				default:
					break;
			}
			break;

		case ctlRepeatEvent:
#ifdef CONFIG_JOGDIAL
			gRowSelected = noRowSelected;
#endif /* CONFIG_JOGDIAL */
			MainFormScrollTable( (eventP->data.ctlRepeat.controlID == MainScrollUpRepeating) ? scrollTblOneUp : scrollTblOneDown );
			break;

		case tblEnterEvent:
			if( eventP->data.tblEnter.column == kMainFormInfoTableColumn && gMainDatabase.sortOrder == kSortMainDBManually )
				handled = MainFormMoveProjectByHand( eventP );
			break;

		case tblSelectEvent:

#ifdef CONFIG_OS_BELOW_35
			if( gGlobalPrefs.rom35present )
			{
#endif /* CONFIG_OS_BELOW_35 */

				if( TblRowMasked( eventP->data.tblSelect.pTable, eventP->data.tblSelect.row) )
				{
					if( SecVerifyPW(showPrivateRecords) )
					{
						gGlobalPrefs.privateRecordStatus = showPrivateRecords;
						eventP->data.tblSelect.column = kMainFormInfoTableColumn;
					}
					else
						break;
				}

#ifdef CONFIG_OS_BELOW_35
			}
#endif /* CONFIG_OS_BELOW_35 */

			switch( eventP->data.tblSelect.column )
			{
				case kMainFormPriorityTableColumn:
					MainFormSelectInfoFromList( eventP->data.tblSelect.pTable, eventP->data.tblSelect.row, eventP->data.tblSelect.column );
					handled = true;
					break;

				case kMainFormBeginDateTableColumn:
					gCurrentProject.projIndex = TblGetRowID( eventP->data.tblSelect.pTable, eventP->data.tblSelect.row );
					gCurrentProject.currentPage = GeneralPage;
					FrmGotoForm( ProjectForm );
					handled = true;
					break;

				case kMainFormInfoTableColumn:
					gCurrentProject.projIndex = TblGetRowID( eventP->data.tblSelect.pTable, eventP->data.tblSelect.row );
					gCurrentProject.currentPage = ToDoPage;
					FrmGotoForm( ProjectForm );
					handled = true;
					break;

				case kMainFormStateTableColumn:
					MainFormSelectInfoFromList( eventP->data.tblSelect.pTable, eventP->data.tblSelect.row, eventP->data.tblSelect.column );
					handled = true;
					break;

				case kMainFormMemoTableColumn:
					gCurrentProject.projIndex = TblGetRowID( eventP->data.tblSelect.pTable, eventP->data.tblSelect.row );
					gCurrentProject.currentPage = MemoPage;
					FrmGotoForm( ProjectForm );
					handled = true;
					break;

				case kMainFormCategoryTableColumn:
					//MainFormSelectInfoFromList( eventP->data.tblSelect.pTable, eventP->data.tblSelect.row, eventP->data.tblSelect.column );
					MainFormSelectRecCategory( eventP->data.tblSelect.pTable, eventP->data.tblSelect.row, eventP->data.tblSelect.column );
					handled = true;
					break;

#if ERROR_CHECK_LEVEL != ERROR_CHECK_NONE
				default:
					break;
#endif
			}
			break;

#ifdef CONFIG_HANDERA
		case displayExtentChangedEvent:
			if( gGlobalPrefs.vgaExtension )
			{
				frmP = FrmGetActiveForm();
#ifdef CONFIG_JOGDIAL
				gRowSelected = noRowSelected;
#endif /* CONFIG_JOGDIAL */
				MainFormResize( frmP, true );
				handled = true;
			}
			break;
#endif /* CONFIG_HANDERA */

		default:
			break;
		
	}
	
	return (handled);
}
