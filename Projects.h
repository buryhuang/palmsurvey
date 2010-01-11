#ifndef PROJECTS_H
#define PROJECTS_H 

#include "config.h"
#include "sections.h"

// defines ----------------------------------------------------------------------------------------

#define memoDBType								'DATA'
#define memoDBCreator							'memo'

#define SetCustomMatch(type,prjtI)		(((UInt32)(prjtI)) | ((UInt32)(type))<<16)
#define GetTypeOfCustomMatch(match)		((0xFFFF0000 & (match))>>16)
#define GetPrjtIOfCustomMatch(match) 	(0x0000FFFF & (match))
#define kGotoToDoItem									1
#define kGotoToDoItemNote							2
#define kGotoPrjtMemo									3

#define kToDoDescriptionMaxLength		255

#define GRAY_COLOR_INDEX			0xDC
#define RED_COLOR_INDEX				0x77

#define todoDBType									'DATA'
#define todoDBCreator								'todo'

#define appPrefID                 	 0x00
#define appPrefVersionNum         	 0x03

#ifdef CONFIG_OS_BELOW_35
	#define ourMinVersion	sysMakeROMVersion(3,1,0,sysROMStageRelease,0)
#else
	#define ourMinVersion	sysMakeROMVersion(3,5,0,sysROMStageRelease,0)
#endif /* CONFIG_OS_BELOW_35 */

#define kDueDateListTodayIndex				0
#define kDueDateListTomorrowIndex			1
#define kDueDateListOneWeekLaterIndex 2
#define kDueDateListNoDateIndex				3
#define kDueDateListChooseDateIndex 	4

#define kPriorityMask		0x70
#define kStateMask			0x0F

#define kProjectNoEndDate	0xFFFF
#define kProjectNoIndex		0xFFFF

#define noRecordIndex			0xFFFF

#ifdef CONFIG_JOGDIAL
	#define noRowSelected		0xFFFF
#endif /* CONFIG_JOGDIAL */

#define kPriority1			0x10
#define kPriority2			0x20
#define kPriority3			0x30
#define kPriority4			0x40
#define kPriority5			0x50

#define kState1					0x01
#define kState2					0x02
#define kState3					0x03
#define kState4					0x04
#define kState5					0x05

#define kMainFormPriorityTableColumnMinWidth		8
#define kMainFormBeginDateTableColumnMinWidth		26	
#define kMainFormStateTableColumnMinWidth				7
#define kMainFormMemoTableColumnMinWidth				7	
#define kMainFormCategoryColumnMinWidth					40

#define ChrIsHardKey(c)		((((c) >= hardKeyMin) && ((c) <= hardKeyMax)) || ((c) == calcChr))
#define IsCharPrintable(c) ( ((c) >= '0' && (c) <= '9') || ((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z') )
#define IsCharSmallLetter(c) ((c) >= 'a' && (c) <= 'z')

#define kShowProjectBeginDateFlag	0x0001
#define kShowProjectToDoStateFlag	0x0002
#define kShowProjectPriorityFlag	0x0004
#define kShowProjectStateFlag			0x0008
#define kShowOnlyActiveProjectsFlag	0x0010

#define kShowToDoPriorityFlag			0x0020
#define kShowToDoDueDateFlag			0x0040
#define kShowCompletedToDosFlag		0x0080
#define kShowOnlyDueDateToDosFlag	0x0100
#define kCompleteToDoDueDateFlag	0x0200
#define kGeneralPageFlag					0x0400
#define kToDoPageFlag							0x0800
#define kMemoPageFlag							0x1000
#define kShowPrjtCategoriesFlag		0x2000
#define kShowToDoCategoriesFlag		0x4000

#define kMoveToDoOneUp						0x01
#define kMoveToDoOneDown					0x02

#define kDurationFieldMaxChars		6

#define kUpdateWholeForm					0x0001
#define kUpdateCurrentToDoItem		0x0002

#define kState1Char								'\xBB'
#define kState1CharFont						boldFont
#define kState2Char								'\xD7'
#define kState2CharFont						boldFont
#define kState3Char								'\x13'
#define kState3CharFont						symbolFont
#define kState4Char								'\x16'
#define kState4CharFont						symbolFont
#define kState5Char								'?'
#define kState5CharFont						boldFont

#define kLoginNameMaxLen						32

// structures -------------------------------------------------------------------------------------

typedef enum { GeneralPage = 0, ToDoPage, MemoPage } ProjectFormPages;

typedef enum { scrollTblOneUp = 0, scrollTblPageUp, scrollTblOneDown, scrollTblPageDown } ScrollTableType;

#ifdef CONFIG_JOGDIAL
	typedef enum { selectNext, selectPrev } SelectItemDirection;
#endif /* CONFIG_JOGDIAL */

typedef struct
{
	privateRecordViewEnum		privateRecordStatus;

	FontID									noteFont;
	FontID									memoFont;
	FontID									todoFont;
	FontID									mainFont;		// used on the main form
	FontID									allToDosFont;

	DateFormatType							dateFormat;
	Char									loginName[kLoginNameMaxLen];

#ifdef CONFIG_SONY
	UInt16									sonyLibRefNum;
#endif /* CONFIG_SONY */

#ifdef CONFIG_OS_BELOW_35
	UInt16									rom35present			:1;
#else
	UInt16									reserved1					:1;
#endif /* CONFIG_OS_BELOW_35 */

#ifdef CONFIG_COLOR
	UInt16									useColors					:1;
#else
	UInt16									reserved2					:1;
#endif /* CONFIG_COLOR */

#ifdef CONFIG_HANDERA
	UInt16									vgaExtension			:1;
	UInt16									silkPresent				:1;
#else
	UInt16									reserved3					:2;
#endif /* CONFIG_HANDERA */

	// used on the main form
	UInt16									showBeginDate					:1;
	UInt16									showToDoInfo					:1;
	UInt16									showProjPriority			:1;
	UInt16									showPrjtCategories		:1;
	UInt16									showProjState					:1;
	UInt16									showOnlyActive				:1;

	// used by the project form
	UInt16									showToDoPrio					:1;
	UInt16									showToDoDueDate				:1;
	UInt16									showToDoCategories		:1;
	UInt16									showCompleteToDos			:1;
	UInt16									showOnlyDueDateToDos	:1;
	UInt16									completeToDoDueDate		:1;

} GlobalPreferencesType;

typedef struct
{
	DmOpenRef							todoDB;
	UInt16								todoDBCategory;
	UInt8								todoDBSortOrder;
	UInt8								reserved1;
	UInt16								curToDoIndex;

	UInt16								projIndex;
	ProjectFormPages 						currentPage;
} CurrentProjectType;

typedef struct
{
	DmOpenRef								db;
	UInt16									currentCategory;
	UInt8									sortOrder;
	UInt8									reserved1;
} MainDBType;

// declarations -----------------------------------------------------------------------------------

// event handler for the main form ----------------------------------------------------------------
extern Boolean				MainFormHandleEvent( EventType * eventP ) SECOND_SECTION;

// event handler for the project form -------------------------------------------------------------
extern Boolean				ProjectFormHandleEvent( EventType * eventP ) SECOND_SECTION;;

// event handler for the name dialog --------------------------------------------------------------
extern Boolean				ProjectNameFormHandleEvent( EventType * eventP ) SECOND_SECTION;

// event handler for the note form ----------------------------------------------------------------
extern Boolean				NoteFormHandleEvent( EventType * eventP ) THIRD_SECTION;

// event handler for the details dialog -----------------------------------------------------------
extern Boolean				DetailsFormHandleEvent( EventType * eventP ) SECOND_SECTION;

#ifdef CONFIG_DEF_CATEGORY
// event handler for the default categories dialog ------------------------------------------------
// i dont know why but when we declare some of the default categories function
// in the second section we get some big problems
extern Boolean				DefaultCategoriesHandleEvent( EventType * eventP );
#endif /* CONFIG_DEF_CATEGORY */

#ifdef CONFIG_EXT_ABOUT
// event handler for the extended about dialog ----------------------------------------------------
extern Boolean 				ExtAboutHandleEvent( EventType * eventP ) SECOND_SECTION;
#endif /* CONFIG_EXT_ABOUT */

#ifdef CONFIG_ALLTODOS_DLG
// event handler for the alltodos dialog ----------------------------------------------------------
extern Boolean        AllToDosHandleEvent( EventType * eventP ) SECOND_SECTION;
#endif /* CONFIG_ALLTODOS_DLG */

// general purpose funtions -----------------------------------------------------------------------
extern Boolean				ToDoItemIsDue( PrjtToDoType * todoP );
extern void 					DrawFormAndRoundTitle( FormType * frmP, Char * title ) SECOND_SECTION;
extern ControlType * 	GetObjectPtr( UInt16 objectID );
extern Boolean 				SeekProjectRecord( UInt16* indexP, Int16 offset, Int16 direction ) SECOND_SECTION;
//extern Boolean				SortChangedProject( /*[in]*/UInt16 recIndex, /*[out]*/UInt16 *newIndex );
extern LocalID 				FindDatabaseByProjectName( Char *proname );
extern void						ShowGeneralAlert( UInt16 strID ) SECOND_SECTION;
extern void						ShowAbout( void ) SECOND_SECTION;
extern Char *					DateToAsciiWithoutYear( DateType date, Char * szBuffer );
extern PrjtPackedProjectType * GetProjectRecord( UInt16 index, Boolean setBusy );

extern GlobalPreferencesType 	gGlobalPrefs;
extern CurrentProjectType			gCurrentProject;
extern MainDBType							gMainDatabase;

#ifdef CONFIG_JOGDIAL
	extern UInt16	gRowSelected;
#endif /* CONFIG_JOGDIAL */

#endif // PROJECTS_H
