#include <PalmOS.h>
#include "ProjectsRcp.h"
#include "ProjectsDB.h"
#include "Projects.h"
#include "MemoLookup.h"

#ifndef __PROJECTS_CONFIG_H__
	#error "include config.h"
#endif /* __PROJECTS_CONFIG_H__ */

#ifdef CONFIG_SONY
	#include <SonyCLIE.h>
#endif /* CONFIG_SONY */

#ifdef CONFIG_HANDERA
	#include "Vga.h"
	#include "Silk.h"
#endif

extern UInt16 gTopVisibleToDoIndex;

// structs used only in this module ---------------------------------------------------------------
typedef struct 
{
	UInt16 lastCategory;
	UInt16 lastFormID;
	UInt16 flags;
	UInt8	 projectSortOrder;	
	UInt8	 flags_ext;
	UInt16 lastCurrentProjectIndex;		// will be needed for restoring if lastFormID == ProjectForm
	FontID mainFormFont;
	FontID memoPageFont;
	FontID todoPageFont;
	FontID noteFormFont;
	FontID allToDosFont;
} 
ProjectsPreferencesType;

typedef struct
{
	UInt16		maindb_index;
	UInt16		todo_index;
}
FeatureMemFindType;

// globaly wide used variables --------------------------------------------------------------------
GlobalPreferencesType	gGlobalPrefs;
CurrentProjectType		gCurrentProject;
MainDBType						gMainDatabase;

#ifdef CONFIG_JOGDIAL
	UInt16	gRowSelected;
#endif /* CONFIG_JOGDIAL */

// module wide declarations -----------------------------------------------------------------------
static Err			RomVersionCompatible( UInt32 requiredVersion, UInt16 launchFlags );
static Boolean 	AppHandleEvent( EventType * eventP );
static void 		AppEventLoop( void );
static void 		Search( FindParamsType *params );	// must be in the first section !!!
static void 		GoToItem( GoToParamsType *goToParams ) SECOND_SECTION;
static Err			AppStart( UInt16 *formID ) SECOND_SECTION;
static void 		AppStop( void ) SECOND_SECTION;
static void 		ProjectsHandleSyncNotify(); // must be in the first section !!!
static void 		SaveSearchCurrentPosition( UInt16 prjtI, UInt16 todoI ); // must be in the first section !!!
static void 		DrawMatch( Char * prjtName, Char * descP, RectangleType * bounds ); // must be in the first secition !!!


// globaly used functions (implementation) --------------------------------------------------------

/*
 * FUNCTION:			ToDoItemIsDue
 * DESCRIPTION:		this routine finds out where the passed item is due
 * PARAMETERS:		todoP - pointer to the record
 * RETURNS:				true if the item is due, else false
 */
Boolean ToDoItemIsDue( PrjtToDoType * todoP )
{
	Boolean		due;
	DateType 	now;

	if( DateToInt( todoP->dueDate ) == kNoDate )
		due = false;
	else
	{
		DateSecondsToDate( TimGetSeconds(), &now );
		due = ( !(todoP->priority & kToDosCompleteFlag) && (DateToDays( todoP->dueDate ) < DateToDays( now )) );
	}

	return (due);
}

/*
 * FUNCTION:			DrawFormAndRoundTitle
 * DESCRIPTION:		this routine draws the form and its title like
 * 								in the phone lookup dialog
 * PARAMETERS:		frmP - pointer to the form
 * 								title - must NOT be null
 * RETURNS:				nothing
 */
void DrawFormAndRoundTitle( FormType * frmP, Char * title )
{
	RectangleType 	eraseRect;
	RectangleType		drawRect;
	RectangleType		r;
	UInt8					*	lockedWinP = NULL;
	Char *					eolP;
	UInt16					formWidth;
	UInt16					maxWidth;
	UInt16					descWidth;
	UInt16					descLen;
	UInt16					x;
	FontID					font;
	IndexedColorType	foreColor = 0;
	IndexedColorType	backColor = 0;
	IndexedColorType	textColor = 0;
	Boolean						fitInWidth;
	Boolean						tmp_boolean;
	Char							horzEllipsis;

	ErrFatalDisplayIf( !title, "invalid param" );

#ifdef CONFIG_OS_BELOW_35
	if( gGlobalPrefs.rom35present )
#endif
		lockedWinP = WinScreenLock(winLockCopy);

	FrmDrawForm( frmP );

	FrmGetFormBounds( frmP, &r );
	formWidth = r.extent.x;
	maxWidth = formWidth - 8;

	eolP = StrChr (title, linefeedChr);
	descLen = (eolP == NULL ? StrLen (title) : eolP - title);
	
#ifdef CONFIG_HANDERA
	if( gGlobalPrefs.vgaExtension )
		font = FntSetFont( VgaBaseToVgaFont( boldFont ) );
	else
#endif
		font = FntSetFont( boldFont );

	RctSetRectangle( &eraseRect, 0, 0, formWidth, FntLineHeight()+4 );
	RctSetRectangle( &drawRect, 0, 0, formWidth, FntLineHeight()+2 );

#ifdef CONFIG_OS_BELOW_35
	if( gGlobalPrefs.rom35present )
	{
#endif

		foreColor = WinSetForeColor( UIColorGetTableEntryIndex( UIFormFrame ) );
		backColor = WinSetBackColor( UIColorGetTableEntryIndex( UIFormFill ) );
		textColor = WinSetTextColor( UIColorGetTableEntryIndex( UIFormFrame ) );

#ifdef CONFIG_OS_BELOW_35
	}
#endif
	
	WinEraseRectangle( &eraseRect, 0 );
	WinDrawRectangle( &drawRect, 3 );

	descWidth = maxWidth;
	FntCharsInWidth( title, &descWidth, &descLen, &fitInWidth );
	if( !fitInWidth )
	{
		ChrHorizEllipsis( &horzEllipsis );
		descWidth -= FntCharWidth( horzEllipsis );
		FntCharsInWidth( title, &descWidth, &descLen, &tmp_boolean );
	}

	x = (formWidth - descWidth)>>1;
	WinDrawInvertedChars( title, descLen, x, 1 );
	if( !fitInWidth )
		WinDrawInvertedChars( &horzEllipsis, 1, x + descWidth, 1 );
	
#ifdef CONFIG_OS_BELOW_35
	if( gGlobalPrefs.rom35present )
	{
#endif

		if( lockedWinP )
			WinScreenUnlock();
		WinSetForeColor (foreColor);
		WinSetBackColor (backColor);
		WinSetTextColor (textColor);

#ifdef CONFIG_OS_BELOW_35
	}
#endif

	FntSetFont (font);
}

/*
 * FUNCTION: 			GetProjectRecord
 * DESCRIPTION:		this routine returns the locked record at index from our main db
 * PARAMS:				index - of the record to return
 * 								setBusy - if true the records busy flag will be set
 * 									you must use DmReleaseRecord to clear this flag
 * RETURNS:				NULL on failure, pointer to a locked project record
 */
PrjtPackedProjectType * GetProjectRecord( UInt16 index, Boolean setBusy )
{
	MemHandle		recH;

	ErrFatalDisplayIf( !gMainDatabase.db, "main db not open" );

	if( setBusy )
		recH = DmGetRecord( gMainDatabase.db, index );
	else
		recH = DmQueryRecord( gMainDatabase.db, index );

	if( !recH )
		return NULL;
	else
		return (MemHandleLock( recH ));
}

/*
 * FUNCTION:			DateToAsciiWithoutYear
 * DESCRIPTION:		this routine writes the date without the year
 * PARAMETES:			date - the date
 * 								szBuffer - to recieve the string
 * RETURNS:				pointer to the beginning of the date string
 */
Char * DateToAsciiWithoutYear( DateType date, Char * szBuffer )
{
	ErrFatalDisplayIf( !szBuffer, "invalid param" );

	DateToAscii( date.month, date.day, date.year + firstYear, gGlobalPrefs.dateFormat, szBuffer );

	switch( gGlobalPrefs.dateFormat )
	{
		case dfMDYWithSlashes:
		case dfDMYWithSlashes:
		case dfDMYWithDots:
		case dfDMYWithDashes:
			*(szBuffer + (StrLen( szBuffer ) - 3)) = '\0';
			break;

		case dfYMDWithSlashes:
		case dfYMDWithDots:
		case dfYMDWithDashes:
			szBuffer += 3;
			break;

		default:
			break;
	}

	return (szBuffer);
}

/*
 * FUNCTION:			ShowGeneralAlert
 * DESCRIPTION:		this routine pops up general warning 
 * 								alert with the specified string
 * PARAMETERS:		strID - resource ID of the string to be shown
 * RETURNS:				nothing
 */
void ShowGeneralAlert( UInt16 strID )
{
	MemHandle		errH;

	errH = DmGetResource( strRsc, strID );
	ErrFatalDisplayIf( !errH, "resource not found" );
	FrmCustomAlert( GeneralWarningAlert, MemHandleLock( errH ), NULL, NULL );
	MemHandleUnlock( errH );
	DmReleaseResource( errH );
}

/*
 * FUNCTION:		ShowAbout
 * DESCRIPTION:	display our AboutDialog
 * PARAMETERS:	none
 * RETURNS:			nothing
 */
void ShowAbout( void )
{
#ifdef CONFIG_EXT_ABOUT

	FrmPopupForm( ExtAboutDialog );

#else

	FormType * aboutP;

	aboutP = FrmInitForm( AboutDialog );
	FrmDoDialog( aboutP );
	FrmDeleteForm( aboutP );

#endif /* CONFIG_EXT_ABOUT */
}

/*
 * FUNCTION:				GetObjectPtr
 * DESCRIPTION:			given a resource id this
 * 									routine returns a pointer
 * 									to the control in the currently
 * 									active form
 * PARAMETERS:			objectID - resource ID of the object
 * RETURNS:					pointer to the control identified by objectID
 * NOTE:						this routine may display a fatal error if
 * 									the object is not in the currently active form
 */
ControlType * GetObjectPtr( UInt16 objectID )
{
	FormPtr frmP;

	frmP = FrmGetActiveForm();
	return (FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, objectID)));
}

/*
 * FUNCTION:			SeekProjectRecord
 * DESCRIPTION:		looks for the next record in gMainDatabase.db
 * PARAMETERS:		index - in/out of the record
 * 								offset - 
 * 								direction - 
 * RETURNS:				true if next record found
 * 								false if not
 */
Boolean SeekProjectRecord( UInt16* indexP, Int16 offset, Int16 direction )
{
	Boolean				isActive = false;
	MemHandle 		recH;
	PrjtPackedProjectType * recP;

  ErrFatalDisplayIf( !gMainDatabase.db, "main db not open" );

	if( !gGlobalPrefs.showOnlyActive )
		return (errNone == DmSeekRecordInCategory( gMainDatabase.db, indexP, offset, direction, gMainDatabase.currentCategory ));
	else
	{
		while( true )
		{
			if( DmSeekRecordInCategory( gMainDatabase.db, indexP, offset, direction, gMainDatabase.currentCategory ) )
				break;

			if( offset == 0 )
				offset = 1;

			recH = DmQueryRecord( gMainDatabase.db, *indexP );
			if(!recH )
				continue;
			recP = MemHandleLock( recH );
			isActive = (PrjtDBProjectsState( recP->priority_status ) == kState1);
			MemHandleUnlock( recH );

			if( isActive )
				return true;
		}
	}
	return false;
}

/*
 * FUNCTION:			FindDatabaseByProjectName
 * DESCRIPTION:		give a project name this routine
 * 								searches for a database of this
 * 								project
 * PARAMETERS:		proname - name of the project
 * RETURNS:				0 - if the database was not found
 * 								local id of the database if found
 */
LocalID FindDatabaseByProjectName( Char *proname )
{
	Char szFullName[dmDBNameLength];
	StrCopy( szFullName, proname );
	StrCat( szFullName, appFileCreatorAsString );

	return (DmFindDatabase( kCardNo, szFullName ));
}

/*
 * FUNCTION:			ProjectNameFormHandleEvent
 * DESCRIPTION:		this routine is an event handler
 * 								for the new name/rename dialog
 *								the form will only be dismissed
 *								if the user tapped cancel or the
 *								project name he entered is unique
 * PARAMETERS:		eventP - pointer to the event
 * RETURNS:				true if handled else false
 */
Boolean ProjectNameFormHandleEvent( EventType * eventP )
{
	FieldType *fieldP;

	/*
	 * we do not need to handle the edit menu items
	 * because we have assigned values that the system
	 * will handle for us :) (see UIResource.h)
	if( eventP->eType == menuEvent )
		return EditMenuEventHandler( eventP->data.menu.itemID );
	else */
#ifdef CONFIG_JOGDIAL
	if( eventP->eType == keyDownEvent &&
	#ifdef CONFIG_SONY
			eventP->data.keyDown.chr == vchrJogBack
	#else
		#ifndef CONFIG_HANDERA
			#error "please define CONFIG_HANDERA or CONFIG_SONY or remove the CONFIG_JOGDIAL definition!"
		#endif /* CONFIG_HANDERA */
			eventP->data.keyDown.chr == chrEscape
	#endif /* CONFIG_SONY */
		)
	{
		MemSet( eventP, sizeof( EventType ), 0 );
		eventP->eType = ctlSelectEvent;
		eventP->data.ctlSelect.controlID = ProjectNameFormCancelButton;
		eventP->data.ctlSelect.pControl = GetObjectPtr( ProjectNameFormCancelButton );
		EvtAddEventToQueue( eventP );
		return (true);
	}
#endif /* CONFIG_JOGDIAL */
	if( eventP->eType == ctlSelectEvent && eventP->data.ctlSelect.controlID == ProjectNameFormOKButton )
	{
		fieldP = (FieldType *)GetObjectPtr( ProjectNameFormNameField );
		if( FldGetTextLength( fieldP ) == 0 )
		{
			ShowGeneralAlert( EnterANameForProject );
			return (true);
		}
		else if( FindDatabaseByProjectName( FldGetTextPtr( fieldP ) ) )
		{
			ShowGeneralAlert( DBAlreadyExists );
			fieldP = (FieldType *)GetObjectPtr( ProjectNameFormNameField );
			FldSetSelection( fieldP, 0, FldGetTextLength( fieldP ) );
			return (true);
		}
	}
	return (false);
}

/*
 * FUNCTION:				RomVersionCompatible
 * DESCRIPTION:			this routine checks that a ROM 
 * 									version is meet your minimum requirement
 * PARAMETERS:			requiredVersion - the minimum version we require
 * 									launchFlags - from the pilot main indicating
 * 									the calling reason
 * RETURNS:					0 - if the system version is greater equal to the required
 *									sysErrRomIncompatible - if the sysVersion < required
 */
static Err RomVersionCompatible( UInt32 requiredVersion, UInt16 launchFlags )
{
	UInt32	romVersion;

	// See if we're on in minimum required version of the ROM or later.
	FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion);
	if (romVersion < requiredVersion)
	{
		if ((launchFlags & (sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp)) ==
			(sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp))
		{
			FrmAlert(RomIncompatibleAlert);
		
			// Pilot 1.0 will continuously relaunch this app unless we switch to 
			// another safe one.
			if (romVersion < sysMakeROMVersion(2,0,0,sysROMStageRelease,0))
				AppLaunchWithCommand(sysFileCDefaultApp, sysAppLaunchCmdNormalLaunch, NULL);
		}
		
		return (sysErrRomIncompatible);
	}

	return (0);
}

/*
 * FUNCTION:				AppHandleEvent
 * DESCRIPTION:			this routine is called each time
 * 									an event occurs. here we handle
 * 									only events of type frmLoadEvent
 * 									and load the appropriate form,
 * 									make it the active form and connect
 * 									an appropriate form event handler
 * PARAMETERS:			eventP - the event
 * RETURNS:					true if the event is of type frmLoadEvent
 * 											and no more handling of this event is
 * 											required
 * 									else false
 */
static Boolean AppHandleEvent( EventType * eventP )
{
	UInt16 			formId;
	FormType * 	frmP;


	if( eventP->eType == frmLoadEvent )
	{
		// Load the form resource.
		formId = eventP->data.frmLoad.formID;
		frmP = FrmInitForm(formId);
		FrmSetActiveForm(frmP);

		// Set the event handler for the form.  The handler of the currently
		// active form is called by FrmHandleEvent each time is receives an
		// event.
		switch (formId)
		{
			case MainForm:
				FrmSetEventHandler(frmP, MainFormHandleEvent);
				break;

			case ProjectForm:
				FrmSetEventHandler(frmP, ProjectFormHandleEvent);
				break;

#ifdef CONFIG_DEF_CATEGORY
			case DefaultCategoriesForm:
				FrmSetEventHandler(frmP, DefaultCategoriesHandleEvent);
				break;
#endif /* CONFIG_DEF_CATEGORY */

			case ToDoNoteForm:
				FrmSetEventHandler(frmP, NoteFormHandleEvent);
				break;

			case ToDoDetailsForm:
				FrmSetEventHandler(frmP, DetailsFormHandleEvent);
				break;

#ifdef CONFIG_EXT_ABOUT
			case ExtAboutDialog:
				FrmSetEventHandler( frmP, ExtAboutHandleEvent );
				break;
#endif /* CONFIG_EXT_ABOUT */

#ifdef CONFIG_ALLTODOS_DLG
      case AllToDosDialog:
        FrmSetEventHandler( frmP, AllToDosHandleEvent );
        break;
#endif /* CONFIG_ALLTODOS_DLG */

			default:
				ErrFatalDisplay("Invalid Form Load Event");
				break;

		}
		return true;
	}
	
	return false;
}

/*
 * FUNCTION:		AppEventLoop
 * DESCRIPTION:	this is the heart of our app
 * 							if we get a appStopEvent we
 * 							break out of the loop and return
 * PARAMETERS:	none
 * RETURNS:			nothing
 */
static void AppEventLoop( void )
{
	UInt16		error;
	EventType event;

	do
	{
		EvtGetEvent(&event, evtWaitForever);
		
		if (! SysHandleEvent(&event))
			if (! MenuHandleEvent(0, &event, &error))
				if (! AppHandleEvent(&event))
					FrmDispatchEvent(&event);
	}
	while (event.eType != appStopEvent);
}

/*
 * FUNCTION:				AppStart
 * DESCRIPTION:			this routine gets the application preferences
 * 									and initializes global variables.
 *									it opens/creates the our main database
 * PARAMETERS:			formID - recieves the formID of the last form
 * 									that was active when our app was closed last time
 * RETURNS:					0 - on success, else non-zero
 */
static Err AppStart( UInt16 * formID )
{
	ProjectsPreferencesType prefs;
	UInt32	tmp_ui32;
	UInt16 	mode;
	UInt16	prefsSize;
	UInt16	attr;

	// check parameters
	ErrFatalDisplayIf( !formID, "invalid param" );

#ifdef CONFIG_OS_BELOW_35
	FtrGet(sysFtrCreator, sysFtrNumROMVersion, &tmp_ui32);
#endif /* CONFIG_OS_BELOW_35 */

	// open or create our main gMainDatabase.db
#ifdef CONFIG_OS_BELOW_35
	gGlobalPrefs.rom35present = (tmp_ui32 >= sysMakeROMVersion(3,5,0, sysROMStageRelease,0));

	if( !gGlobalPrefs.rom35present )
	{
		//gGlobalPrefs.rom35present = 0;

		if( (Boolean)PrefGetPreference( prefHidePrivateRecordsV33 ) )
			gGlobalPrefs.privateRecordStatus = hidePrivateRecords;
		else
			gGlobalPrefs.privateRecordStatus = showPrivateRecords;
	}
	else
#endif /* CONFIG_OS_BELOW_35 */
		gGlobalPrefs.privateRecordStatus = (privateRecordViewEnum)PrefGetPreference(prefShowPrivateRecords);

	if( gGlobalPrefs.privateRecordStatus == hidePrivateRecords )
		mode = dmModeReadWrite;
	else
		mode = dmModeReadWrite | dmModeShowSecret;

	gMainDatabase.db = PrjtDBGetMainDB( true, mode );
	if( !gMainDatabase.db )
		return (dmErrMemError);

  // prepare the configureation (preference) flags

#ifdef CONFIG_HANDERA
	gGlobalPrefs.vgaExtension = 0;
	if( _TRGVGAFeaturePresent( &tmp_ui32 ) && sysGetROMVerMajor( tmp_ui32 ) >= 1 )
	{
		gGlobalPrefs.vgaExtension = 1;
		VgaSetScreenMode( screenMode1To1, rotateModeNone );
	}

	gGlobalPrefs.silkPresent = 0;
	if( _TRGSilkFeaturePresent( &tmp_ui32 ) && sysGetROMVerMajor( tmp_ui32 ) >= 1 )
		gGlobalPrefs.silkPresent = 1;
#endif /* CONFIG_HANDERA */

#ifdef CONFIG_SONY
	gGlobalPrefs.sonyLibRefNum = 0;

	{
		SonySysFtrSysInfoP sonySysFtrSysInfoP;
		Err error = 0;
	
		if( !FtrGet( sonySysFtrCreator, sonySysFtrNumSysInfoP, (UInt32 *)&sonySysFtrSysInfoP ) )
			if( sonySysFtrSysInfoP->libr & sonySysFtrSysInfoLibrHR )
				if( (error = SysLibFind( sonySysLibNameHR, &gGlobalPrefs.sonyLibRefNum )) )
					if( error == sysErrLibNotFound )
						error = SysLibLoad( 'libr', sonySysFileCHRLib, &gGlobalPrefs.sonyLibRefNum );
	}

	if( gGlobalPrefs.sonyLibRefNum )
	{
		UInt32	width, height, depth;

		HROpen( gGlobalPrefs.sonyLibRefNum );
		width = hrWidth;
		height = hrHeight;
		depth = 8;	// (256 colors)
		HRWinScreenMode( gGlobalPrefs.sonyLibRefNum, winScreenModeSet, &width, &height, &depth, NULL );
	}
#endif /* CONFIG_SONY */

#ifdef CONFIG_COLOR
  // find out whether we can use colors
#ifdef CONFIG_OS_BELOW_35
	if( gGlobalPrefs.rom35present )
  {
#endif /* CONFIG_OS_BELOW_35 */
		WinScreenMode( winScreenModeGetSupportedDepths, NULL, NULL, &tmp_ui32, NULL );
		gGlobalPrefs.useColors = ((tmp_ui32 & 0x80) ? 1 : 0);
#ifdef CONFIG_OS_BELOW_35
  }
  else
		gGlobalPrefs.useColors = 0;
#endif /* CONFIG_OS_BELOW_35 */
#endif /* CONFIG_COLOR */


	// get prefered date format for later use
	gGlobalPrefs.dateFormat = (DateFormatType)PrefGetPreference( prefDateFormat );

	// Read the saved preferences / saved-state information.
	prefsSize = sizeof(ProjectsPreferencesType);
	if (PrefGetAppPreferences(appFileCreator, appPrefID, &prefs, &prefsSize, true) != appPrefVersionNum )
	{
		gMainDatabase.currentCategory = dmAllCategories;
		gMainDatabase.sortOrder = kSortMainDBByPriorityStateName;

		gGlobalPrefs.showBeginDate = 0;
		gGlobalPrefs.showToDoInfo = 0;
		gGlobalPrefs.showProjPriority = 1;
		gGlobalPrefs.showProjState = 1;
		gGlobalPrefs.showPrjtCategories = 0;
		gGlobalPrefs.showOnlyActive = 0;

		gGlobalPrefs.showToDoPrio = 1;
		gGlobalPrefs.showToDoDueDate = 1;
		gGlobalPrefs.showToDoCategories = 0;
		gGlobalPrefs.showCompleteToDos = 1;
		gGlobalPrefs.showOnlyDueDateToDos = 0;
		gGlobalPrefs.completeToDoDueDate = 0;

		FtrGet(sysFtrCreator, sysFtrDefaultFont, &tmp_ui32);
#ifdef CONFIG_HANDERA
		if( gGlobalPrefs.vgaExtension )
			gGlobalPrefs.allToDosFont = gGlobalPrefs.noteFont = gGlobalPrefs.memoFont = gGlobalPrefs.todoFont = gGlobalPrefs.mainFont = VgaBaseToVgaFont( (FontID)tmp_ui32 );
		else
#endif /* CONFIG_HANDERA */
		gGlobalPrefs.allToDosFont = gGlobalPrefs.noteFont = gGlobalPrefs.memoFont = gGlobalPrefs.todoFont = gGlobalPrefs.mainFont = (FontID)tmp_ui32;

		gCurrentProject.projIndex = kProjectNoIndex;
		*formID = MainForm;
	}
	else
	{
		gMainDatabase.currentCategory 			= prefs.lastCategory;
		gMainDatabase.sortOrder		= prefs.projectSortOrder;

		gGlobalPrefs.showBeginDate = (prefs.flags & kShowProjectBeginDateFlag) ? 1 : 0;
		gGlobalPrefs.showToDoInfo = (prefs.flags & kShowProjectToDoStateFlag) ? 1 : 0;
		gGlobalPrefs.showProjPriority 	= (prefs.flags & kShowProjectPriorityFlag) ? 1 : 0;
		gGlobalPrefs.showProjState			= (prefs.flags & kShowProjectStateFlag) ? 1 : 0;
		gGlobalPrefs.showPrjtCategories = (prefs.flags & kShowPrjtCategoriesFlag) ? 1 : 0;
		gGlobalPrefs.showOnlyActive = (prefs.flags & kShowOnlyActiveProjectsFlag) ? 1 : 0;

		gGlobalPrefs.showToDoPrio			= (prefs.flags & kShowToDoPriorityFlag) ? 1 : 0;
		gGlobalPrefs.showToDoDueDate			= (prefs.flags & kShowToDoDueDateFlag) ? 1 : 0;
		gGlobalPrefs.showToDoCategories 	=	(prefs.flags & kShowToDoCategoriesFlag) ? 1 : 0;
		gGlobalPrefs.showCompleteToDos 	= (prefs.flags & kShowCompletedToDosFlag) ? 1 : 0;
		gGlobalPrefs.showOnlyDueDateToDos	= (prefs.flags & kShowOnlyDueDateToDosFlag) ? 1 : 0;
		gGlobalPrefs.completeToDoDueDate	= (prefs.flags & kCompleteToDoDueDateFlag) ? 1 : 0;

		// if the last form was project form go there to the lastly visited
		// category class, else set the default category to "All"
		// so next time the user chooses a project he sees all entries
		*formID = prefs.lastFormID;

		gCurrentProject.projIndex = prefs.lastCurrentProjectIndex;

		gGlobalPrefs.mainFont = prefs.mainFormFont;
		gGlobalPrefs.memoFont = prefs.memoPageFont;
		gGlobalPrefs.todoFont = prefs.todoPageFont;
		gGlobalPrefs.noteFont = prefs.noteFormFont;
		gGlobalPrefs.allToDosFont = prefs.allToDosFont;

		gGlobalPrefs.loginName[0] = 0;

		if( prefs.flags & kGeneralPageFlag )
			gCurrentProject.currentPage = GeneralPage;
		else if( prefs.flags & kToDoPageFlag )
			gCurrentProject.currentPage = ToDoPage;
		else
			gCurrentProject.currentPage = MemoPage;
	}

	// if the lastly opened form was the project form and this
	// project is marked private and now launched we should hide
	// or mask private rocords, go to the main form.
	if( *formID == ProjectForm && gGlobalPrefs.privateRecordStatus != showPrivateRecords )
	{
		DmRecordInfo( gMainDatabase.db, gCurrentProject.projIndex, &attr, NULL, NULL );
		if( attr & dmRecAttrSecret )
		{
			*formID = MainForm;
			gCurrentProject.projIndex = kProjectNoIndex;
		}
	}

	return 0;
}

/*
 * FUNCTION:				AppStop
 * DESCRIPTION:			saves our globals as app preferences so they
 * 									can be restored next time our app is launched
 * 									and closes the main database
 * PARAMETERS:			none
 * RETURNS:					nothing
 */
static void AppStop( void )
{
	ProjectsPreferencesType prefs;

	prefs.lastCategory = gMainDatabase.currentCategory;
	prefs.lastFormID = FrmGetFormId( FrmGetActiveForm() );

#ifdef CONFIG_EXT_ABOUT
	if( prefs.lastFormID == ExtAboutDialog )
		prefs.lastFormID = MainForm;
	else 
#endif /* CONFIG_EXT_ABOUT */	
#ifdef CONFIG_DEF_CATEGORY
	if( prefs.lastFormID == DefaultCategoriesForm )
		prefs.lastFormID = MainForm;
	else 
#endif /* CONFIG_DEF_CATEGORY */
	if( prefs.lastFormID == ToDoNoteForm || 
			prefs.lastFormID == ToDoDetailsForm || 
			prefs.lastFormID == MemoLookupDialog )
		prefs.lastFormID = ProjectForm;

	prefs.projectSortOrder = gMainDatabase.sortOrder;
	prefs.lastCurrentProjectIndex = gCurrentProject.projIndex;
	prefs.flags = 0;
	prefs.flags_ext = 0;	// still not used

	if( gGlobalPrefs.showBeginDate )
		prefs.flags |= kShowProjectBeginDateFlag;
	if( gGlobalPrefs.showToDoInfo )
		prefs.flags |= kShowProjectToDoStateFlag;
	if( gGlobalPrefs.showProjPriority )
		prefs.flags |= kShowProjectPriorityFlag;
	if( gGlobalPrefs.showPrjtCategories )
		prefs.flags |= kShowPrjtCategoriesFlag;
	if( gGlobalPrefs.showProjState )
		prefs.flags |= kShowProjectStateFlag;
	if( gGlobalPrefs.showOnlyActive )
		prefs.flags |= kShowOnlyActiveProjectsFlag;
	if( gGlobalPrefs.showToDoPrio )
		prefs.flags |= kShowToDoPriorityFlag;
	if( gGlobalPrefs.showToDoDueDate )
		prefs.flags |= kShowToDoDueDateFlag;
	if( gGlobalPrefs.showToDoCategories )
		prefs.flags |= kShowToDoCategoriesFlag;
	if( gGlobalPrefs.showCompleteToDos )
		prefs.flags |= kShowCompletedToDosFlag;
	if( gGlobalPrefs.completeToDoDueDate )
		prefs.flags |= kCompleteToDoDueDateFlag;
	if( gGlobalPrefs.showOnlyDueDateToDos )
		prefs.flags |= kShowOnlyDueDateToDosFlag;
	switch( gCurrentProject.currentPage )
	{
		case GeneralPage:
			prefs.flags |= kGeneralPageFlag;
			break;
		case ToDoPage:
			prefs.flags |= kToDoPageFlag;
			break;
		case MemoPage:
			prefs.flags |= kMemoPageFlag;
			break;
	}
	prefs.mainFormFont = gGlobalPrefs.mainFont;
	prefs.todoPageFont = gGlobalPrefs.todoFont;
	prefs.memoPageFont = gGlobalPrefs.memoFont;
	prefs.noteFormFont = gGlobalPrefs.noteFont;
	prefs.allToDosFont = gGlobalPrefs.allToDosFont;

	// make sure all data is being saved
	FrmCloseAllForms();

	// close our projects database
	DmCloseDatabase( gMainDatabase.db );

	// Write the saved preferences / saved-state information.  This data will be backed up during a HotSync.
	PrefSetAppPreferences (appFileCreator, appPrefID, appPrefVersionNum, &prefs, sizeof (prefs), true);

#ifdef CONFIG_SONY
	if( gGlobalPrefs.sonyLibRefNum )
	{
		HRWinScreenMode( gGlobalPrefs.sonyLibRefNum, winScreenModeSetToDefaults, NULL, NULL, NULL, NULL );
		HRClose( gGlobalPrefs.sonyLibRefNum );
	}
#endif /* CONFIG_SONY */
}

/*
 * FUNCTION:			SaveSearchCurrentPosition
 * DESCRIPTION:		this routine saves the two passed parameters
 *								in feature memory
 * PARATERS:			prjtI and todoI
 * RETURNS:				nothing
 */
void SaveSearchCurrentPosition( UInt16 prjtI, UInt16 todoI )
{
	FeatureMemFindType ftype;
	void * 							ftrP;

	ftype.maindb_index = prjtI;
	ftype.todo_index = todoI;

	if( FtrGet( appFileCreator, kLastVisitedPrjtIndexFeatureNum, (UInt32 *)&ftrP ) )
		FtrPtrNew( appFileCreator, kLastVisitedPrjtIndexFeatureNum, sizeof( FeatureMemFindType ), &ftrP );

	if( ftrP )
		DmWrite( ftrP, 0, &ftype, sizeof( ftype ) );
}

/*
 * FUNCTION:		DrawMatch	
 * DESCRIPTION:	this routine gets called whenever we have found and matching
 * 							todo item and we can draw it into the search dialog
 * PARAMTERS:		prjtName - project name
 * 							descP - pointer to the description
 * 							bounds - of the drawing area
 * RETURNS:			nothing
 */
void DrawMatch( Char * prjtName, Char * descP, RectangleType * bounds )
{
	Char 		szBuf[dmDBNameLength];
	Char		hrzEllipsis = 0;
	Char *	s;
	UInt16	descWidth;
	UInt16	prjtWidth;
	UInt16	descLen;
	UInt16	prjtLen;
	UInt16	w, i;
	Boolean	fiw = true;
	Boolean tmp;

	ErrFatalDisplayIf( !descP || !bounds, "invalid param" );

	s = StrChr( descP, chrLineFeed );
	if( s )
		descLen = s - descP;
	else
		descLen = StrLen( descP );
	descWidth = FntCharsWidth( descP, descLen );

	prjtLen = StrLen( prjtName );
	prjtWidth = FntCharsWidth( prjtName, prjtLen );
	w = (FntCharWidth('[')+FntCharWidth(']'));
	prjtWidth += w;

	if( (descWidth+prjtWidth) >= (bounds->extent.x-2) )
	{
		prjtWidth = bounds->extent.x/3 - w;

		FntCharsInWidth( prjtName, &prjtWidth, &prjtLen, &fiw );
		if( !fiw )
		{
			ChrHorizEllipsis( &hrzEllipsis );
			prjtWidth -= FntCharWidth( hrzEllipsis );
			FntCharsInWidth( prjtName, &prjtWidth, &prjtLen, &tmp );
			prjtWidth += w;
		}

		prjtWidth += w;
	}

	// todo description
	descWidth = bounds->extent.x - (prjtWidth+1);
	WinDrawTruncChars( descP, descLen, bounds->topLeft.x+1, bounds->topLeft.y, descWidth );

	// project name
	szBuf[0] = '[';
	for( i = 0; i <= prjtLen; i++ )
		szBuf[i+1] = prjtName[i];
	if( !fiw )
		szBuf[i++] = hrzEllipsis;
	szBuf[i++] = ']';

	prjtWidth++;
	WinDrawChars( szBuf, i, (bounds->topLeft.x + bounds->extent.x) - prjtWidth, bounds->topLeft.y );
}

/*
 * FUNCTION:		Search
 * DESCRIPTION:	this routine searches in our database
 * 							for todo items
 * PARAMETERS:	param - pointer to a FindParamsType structure
 * RETURNS:			nothing
 * NOTE:				this routine makes use of feature memory to store
 * 							a UInt16 which holds the last index in main db visited
 */
void Search( FindParamsType *params )
{
	RectangleType 						r;
	Char *										strP;
	PrjtPackedProjectType * 	prjtP;
	PrjtToDoType * 						todoP;
	FeatureMemFindType *			ftrPtr = 0;
	DmOpenRef									mainDB;
	DmOpenRef									todoDB;
	MemHandle									todoH, prjtH;
	UInt16										prjtI, todoI, position;
	UInt16										foundType;
	Boolean										done, match;
	Boolean										searchformemo;

	// open our main database containg the names of the todo databases
	mainDB = PrjtDBGetMainDB( false, params->dbAccesMode );
	if( !mainDB )
	{
		params->more = false;
		return;
	}

	// draw our nice application name header
	todoH = DmGetResource( strRsc, FindHeaderString );
	ErrFatalDisplayIf( !todoH, "find header string resource not found" );
	strP = MemHandleLock( todoH );
	done = FindDrawHeader( params, strP );
	MemHandleUnlock( todoH );
	DmReleaseResource( todoH );
	
	// if full exit this routine
	if( done )
		goto Search_exit;

	searchformemo = true;
	if( params->continuation )
	{
		if( FtrGet( appFileCreator, kLastVisitedPrjtIndexFeatureNum, (UInt32 *)&ftrPtr ) )
			todoI = prjtI = 0;
		else
		{
			prjtI = ftrPtr->maindb_index;
			todoI = ftrPtr->todo_index;
			if( todoI != noRecordIndex )
			{
				todoI = 0;
				searchformemo = false;
			}
		}
	}
	else
		todoI = prjtI = 0;

	// now iterate through the databases and its todo items
	do
	{
		// grab the project record
		prjtH = DmQueryNextInCategory( mainDB, &prjtI, dmAllCategories );
		if( !prjtH )
		{
			params->more = false;
			if( ftrPtr )
				FtrPtrFree( appFileCreator, kLastVisitedPrjtIndexFeatureNum );
			break;
		}
		prjtP = MemHandleLock( prjtH );

		// find a possible memo field containing the search string
		if( searchformemo )
		{
			strP = &prjtP->name + StrLen( &prjtP->name ) + 1;
			if( *strP )
			{
				match = FindStrInStr( strP, params->strToFind, &position );
				if( match )
				{
					done = FindSaveMatch( params, 0, position, 0, SetCustomMatch( kGotoPrjtMemo, prjtI ), 0, 0 );
					if( done )
					{
						MemHandleUnlock( prjtH );

						params->more = true;
						SaveSearchCurrentPosition( prjtI, noRecordIndex );
						goto Search_exit;
					}

					FindGetLineBounds( params, &r );
					DrawMatch( &prjtP->name, strP, &r );
					params->lineNumber++;
				}
			}
		}
		else
			searchformemo = true;

		// now search for todo items
		todoDB = PrjtDBOpenToDoDB( &prjtP->name, dmModeReadOnly );
		if( todoDB )
		{
			do
			{
				// check for events
				if( (todoI & 0x000F) == 0 && 		// every 16th record
						EvtSysEventAvail( true ) )
				{
					MemHandleUnlock( prjtH );
					DmCloseDatabase( todoDB );

					// store the values for a possible next search
					params->more = true;
					SaveSearchCurrentPosition( prjtI, todoI );
					goto Search_exit;
				}

				todoH = DmQueryRecord( todoDB, todoI );
				if( todoH )
				{
					// if there is a match in the description don't look at the note
					// but if we don't find any match in the description of an item
					// we'll try in the note of the item
					todoP = MemHandleLock( todoH );
					strP = &todoP->description;
					match = FindStrInStr( strP, params->strToFind, &position );
					if( match )
						foundType = kGotoToDoItem;
					else
					{
						strP = (&todoP->description + StrLen( &todoP->description ) + 1);
						if( *strP ) // is there a note at all? if not we don't need to call the function
							match = FindStrInStr( strP, params->strToFind, &position );
						if( match )
							foundType = kGotoToDoItemNote;
					}

					if( match )
					{
						done = FindSaveMatch( params, todoI, position, 0, SetCustomMatch( foundType, prjtI ), 0, 0 );
						if( done )
						{
							MemHandleUnlock( todoH );
							MemHandleUnlock( prjtH );
							DmCloseDatabase( todoDB );

							params->more = true;
							SaveSearchCurrentPosition( prjtI, todoI );
							goto Search_exit;
						}

						FindGetLineBounds( params, &r );
						DrawMatch( &prjtP->name, strP, &r );
						params->lineNumber++;
					}

					MemHandleUnlock( todoH );
					todoI++;
				}
			}
			while( todoH );

			// close the database
			DmCloseDatabase( todoDB );
			todoI = 0;
		}

		MemHandleUnlock( prjtH );
		prjtI++;
	}
	while( 1 );

Search_exit:
	DmCloseDatabase( mainDB );
}

/*
 * FUNCTION:			GoToItem
 * DESCRIPTION:		this function determines the
 * 								item we should go to and posts
 * 								events to direct projects to display
 * 								the item
 * PARAMETERS:		goToParams - as passed to PilotMain
 * RETURNS:				nothing
 * NOTE: 					01.10.01 - we support only going to 
 * 								the mainform and marking the found project name
 */
static void GoToItem( GoToParamsType *goToParams )
{
	EventType event;

	gMainDatabase.currentCategory = dmAllCategories;
	gCurrentProject.projIndex = GetPrjtIOfCustomMatch( goToParams->matchCustom );

	MemSet (&event, sizeof(EventType), 0);
	event.eType = frmLoadEvent;
	event.data.frmLoad.formID = ProjectForm;
	EvtAddEventToQueue (&event);

	event.eType = frmGotoEvent;
	event.data.frmGoto.formID = ProjectForm;
	event.data.frmGoto.matchCustom = GetTypeOfCustomMatch( goToParams->matchCustom );
	event.data.frmGoto.matchPos = goToParams->matchPos;
	event.data.frmGoto.recordNum = goToParams->recordNum;
	event.data.frmGoto.matchLen = goToParams->searchStrLen;
	EvtAddEventToQueue (&event);

#if ERROR_CHECK_LEVEL != ERROR_CHECK_NONE
	{
		UInt16		type;
		type = GetTypeOfCustomMatch( goToParams->matchCustom );
		ErrFatalDisplayIf( type == kGotoToDoItem && type == kGotoToDoItemNote && type != kGotoPrjtMemo, "invalid goto type" );
	}
#endif
}

/*
 * FUNCTION:				ProjectsHandleSyncNotify
 * DESCRIPTION:			this routine looks up for new databases
 * PARAMETERS:			none
 * RETURNS:					nothing
 */
static void	ProjectsHandleSyncNotify( void )
{
	ProjectsPreferencesType	prefs;
	UInt16									prefsSize;
	DmOpenRef								mainDB;

	mainDB = PrjtDBGetMainDB( false, dmModeReadWrite );
	if( !mainDB )
		return;

	prefsSize = sizeof( ProjectsPreferencesType );
	if( PrefGetAppPreferences( appFileCreator, appPrefID, &prefs, &prefsSize, true ) == noPreferenceFound )
		prefs.projectSortOrder = kSortMainDBByPriorityStateName;

	PrjtDBScanForNewToDoDBs( mainDB, prefs.projectSortOrder );

	DmCloseDatabase( mainDB );
}

/*
 * FUNCTION:				PilotMain
 * 									this is our app's entry point
 * 									for a detailed description see palm sdk
 */
UInt32 PilotMain( UInt16 cmd, void * cmdPBP, UInt16 launchFlags )
{
	Err error;
	UInt16 formID;

	error = RomVersionCompatible (ourMinVersion, launchFlags);
	if( error ) 
		return (error);

	//HostTraceInit();

	switch( cmd )
	{
		case sysAppLaunchCmdNormalLaunch:
			//HostTraceInit();
			error = AppStart( &formID );
			if (error) 
			{
				//HostTraceClose();
				return error;
			}
			FrmGotoForm( formID );
			AppEventLoop();
			AppStop();
			//HostTraceClose();
			break;

		case sysAppLaunchCmdFind:
			Search( (FindParamsType *)cmdPBP );
			break;

		case sysAppLaunchCmdGoTo:
			FtrPtrFree( appFileCreator, kLastVisitedPrjtIndexFeatureNum );
			if( launchFlags & sysAppLaunchFlagNewGlobals )
			{
				// we were not running before

				error = AppStart( &formID );			// pass formID although we are not interested in it.
				if( error )
					return error;

				GoToItem( (GoToParamsType *)cmdPBP );

				AppEventLoop();
				AppStop();
			}
			else
			{
				FrmCloseAllForms();
				GoToItem( (GoToParamsType *)cmdPBP );
			}
			break;

		case sysAppLaunchCmdSyncNotify:
			ProjectsHandleSyncNotify();
			break;

#if 0
			// we don't need to respond to this
			// command as we edit all data in place :-)
		case sysAppLaunchCmdSaveData:
			break;
#endif

		default:
			break;
	}

	//HostTraceClose();

	return 0;
}
