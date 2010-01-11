#include "config.h"

#define DateWheelCheckGroup							6



#define MainForm 												1
#define MainProjectTable 								2
#define MainNewButton										3
#define MainShowButton 									4
#define MainCategoriesPopTrigger				5
#define MainCategoriesList							6
#define MainScrollUpRepeating						7
#define MainScrollDownRepeating					8
#define MainPriorityList								9
#define MainStateList										10
#define MainRecCategoryList 						11

#ifdef CONFIG_ALLTODOS_DLG
  #define MainToDoButton                  12
#endif /* CONFIG_ALLTODOS_DLG */



// DO NOT FORGET TO UPDATE THIS ALSO IN THE
// PROJECTSDB.H !!!!!!!!!!!!!!!!!!!!!!!!!!!
#define ProjectsCategoriesAppInfoStr			100
#define ProjectsToDoCategoriesAppInfoStr 	101



#define ProjectNameForm 									100
#define ProjectNameFormNameField 					101
#define ProjectNameFormOKButton 					102
#define ProjectNameFormCancelButton 			103

#define PreferencesDialog									200
#define PreferencesPriorityCheckbox				201
#define PreferencesBeginDateCheckbox			202
#define PreferencesToDoStateCheckbox			203
#define PreferencesStateCheckbox					205
#define PreferencesPrjtCategoriesCheckbox	206
#define PreferencesOnlyActiveCheckbox			207
#define PreferencesSortPopTrigger					208
#define PreferencesSortList								209
#define PreferencesOKButton								210
#define PreferencesCancelButton						211

// project form's non-hidable objects
#define ProjectForm											300
#define ProjectGeneralPushButton				301
#define ProjectToDoPushButton						302
#define ProjectMemoPushButton						303
#define ProjectDoneButton								304

#define ProjectToDoPriorityList					305
#define ProjectToDoDueDateList					306
#define ProjectToDoTableCategoryList 		307

#define ProjectCategoryList							308
#define ProjectToDoCategoryList					309


// ---------- project form's hidable objects ---------------
// ----- !!!! don't make any changes in the order !!!! -----

// following objects are on the general page
#define ProjectBeginLabel								310
#define ProjectBeginSelTrigger					311
#define ProjectBeginDownRepeating				312
#define ProjectBeginUpRepeating					313
#define ProjectBeginLockCheck						314

#define ProjectDurationLabel						315
#define ProjectDurationField						316
#define ProjectDurationDownRepeating		317
#define ProjectDurationUpRepeating			318
#define ProjectDurationLockCheck				319

#define ProjectEndLabel									320
#define ProjectEndSelTrigger						321
#define ProjectEndDownRepeating					322
#define ProjectEndUpRepeating						323
#define ProjectEndLockCheck							324

#define ProjectPriorityLabel						325
#define ProjectPriority1PushButton			326
#define ProjectPriority2PushButton			327
#define ProjectPriority3PushButton			328
#define ProjectPriority4PushButton			329
#define ProjectPriority5PushButton			330
#define ProjectStateLabel								331
#define ProjectStatePopTrigger					332
#define ProjectStateList								333
#define ProjectCategoryLabel						334
#define ProjectCategoryPopTrigger 			335
#define ProjectPrivateLabel							336
#define ProjectPrivateCheckbox					337

// this two comming must be the last of the general page (see ProjectFormSwitchToGeneralPage())
#define ProjectRenameProjectButton			338
#define ProjectDeleteProjectButton			339

// following objects are on the memo page
#define ProjectMemoField								340
#define ProjectMemoScrollBar						341

// following objects are on the todo page
#define ProjectToDoTable									350
#define ProjectToDoScrollerUpRepeating		351
#define ProjectToDoScrollerDownRepeating 	352
#define ProjectToDoCategoryPopTrigger			353
#define ProjectNewToDoButton							354
#define ProjectDetailsToDoButton					355
#define ProjectMoveToDoUpButton						356
#define ProjectMoveToDoDownButton					357
// this object is only shown when the database could not be found
#define ProjectCreateDBButton							358

#define ToDoPrefsDialog										400
#define ToDoPrefsSortLabel								401
#define ToDoPrefsSortPopTrigger						402
#define ToDoPrefsSortList									403
#define ToDoPrefsCompletedItemsCheckbox		404
#define ToDoPrefsOnlyDueItemsCheckbox			405
#define ToDoPrefsRecCompletionDateCheckbox 406
#define ToDoPrefsShowDueDateCheckbox			407
#define ToDoPrefsShowPrioritiesCheckbox		408
#define ToDoPrefsShowCategoriesCheckbox 	409
#define ToDoPrefsOKButton									410
#define ToDoPrefsCancelButton							411
#define ToDoPrefsProjectsToDosLabel 			412

#define ToDoNoteForm											500
#define ToDoNoteField											501
#define ToDoNoteScrollBar									502
#define ToDoNoteDoneButton								503
#define ToDoNoteDeleteButton							504

#define ToDoDetailsForm 									600
#define DetailsPriority1PushButton				601
#define DetailsPriority2PushButton				602
#define DetailsPriority3PushButton				603
#define DetailsPriority4PushButton				604
#define DetailsPriority5PushButton				605
#define DetailsCategoryPopTrigger					606
#define DetailsCategoryList								607
#define DetailsDueDatePopTrigger					608
#define DetailsDueDateList								609
#define DetailsOKButton										610
#define DetailsCancelButton								611
#define DetailsDeleteButton								612
#define DetailsNoteButton									613
#define DetailsExportButton								614
#define DetailsExportList									615



#ifdef CONFIG_EXT_ABOUT

	#define ExtAboutDialog									700
	#define ExtAboutScrollBar 							701
	#define ExtAboutDoneButton 							702

#else

	#define AboutDialog											700

#endif /* CONFIG_EXT_ABOUT */



#define MemoLookupDialog									800
#define MemoLookupTable										801
#define MemoLookupAddButton								802
#define MemoLookupCancelButton						803
#define MemoLookupScrollBar								804


#ifdef CONFIG_DEF_CATEGORY
	#define DefaultCategoriesForm 						900
	#define DefaultCategoriesList							901
	#define DefaultCategoriesOKButton					902
	#define DefaultCategoriesNewButton				903
	#define DefaultCategoriesRenameButton			904
	#define DefaultCategoriesDeleteButton			905

	#define EditDefaultCategoryForm						1000
	#define EditDefaultCategoryField					1001
	#define EditDefaultCategoryOKButton				1002
	#define EditDefaultCategoryCancelButton		1003
#endif /* CONFIG_DEF_CATEGORY */


#ifdef CONFIG_ALLTODOS_DLG
	#define AllToDosDialog                    1100
	#define AllToDosDoneButton								1101
	#define AllToDosShowButton								1102
	#define AllToDosTable											1103
	#define AllToDosScrollUpRepeating  				1104
	#define AllToDosScrollDownRepeating 			1105
	#define AllToDosScrollPrevProjectButton  	1106
	#define AllToDosScrollNextProjectButton 	1107
	#define AllToDosCategoryList							1108
#endif /* CONFIG_ALLTODOS_DLG */


// menus ----------------------------------------------------------------------
#define MainFormMenuV35									1000
#ifdef CONFIG_OS_BELOW_35
	#define MainFormMenu 										1001
#endif /* CONFIG_OS_BELOW_35 */

#define OptionsAbout 										1002
#define OptionsSecurity									1003
#define OptionsPhoneLookup							1004
#define OptionsFont											1005
#define OptionsPreferences							1006
#define OptionsScanForDBs								1007
#ifdef CONFIG_DEF_CATEGORY
#define OptionsDefCategories						1008
#endif /* CONFIG_DEF_CATEGORY */


// the values for the following defines
// has been taken from the UIResource.h
// so the system may handle this menu items
// event for us. :)
#define EditMenu												10000
#define EditUndo												10000
#define EditCut													10001
#define EditCopy												10002
#define EditPaste												10003
#define EditSelectAll										10004
#define EditKeyboard										10006
#define EditGraffiti										10007

#define ProjectFormToDoMenu							1020
#define DeleteToDoItem									1021
#define AttachToDoNote									1022
#define DeleteToDoNote									1023
#define PurgeToDoList										1024
#define RescanToDoDatabase							1025
//#define ExportToDoItem									1025

#define ProjectFormMemoMenu							1030
#define DeleteMemo											1031
#define ExportMemo											1032
#define ImportMemo											1033

#define NoteFormMenu										1040

#define GeneralPageMenu									1050
#define DuplicateProject								1051
#define ClearEndDate										1052

#define AllToDosMenu										1060


// alerts -----------------------------------------------------------------------------------------

#define RomIncompatibleAlert 						1000
#define GeneralWarningAlert							1001
#define DeleteProjectAlert							1002
#define DeleteProjectYesButton					0
#define DeleteProjectNoButton						1
#define DeleteToDoAlert									1003
#define DeleteToDoOKButton							0
#define DeleteToDoCancelButton					1
#define SelectItemAlert									1004
#define PurgeToDoAlert									1005
#define PurgeToDoOKButton								0
#define PurgeToDoCancelButton						1
#define DeleteMemoAlert									1007
#define DeleteMemoOKButton							0
#define DeleteMemoCancelButton					1
#define DeleteNoteAlert									1008
#define DeleteNoteOKButton							0
#define DeleteNoteCancelButton					1
#ifdef CONFIG_DEF_CATEGORY
	#define SelectCategoryAlert							1009
	#define AllCategoriesUsedAlert					1010
#endif /* CONFIG_DEF_CATEGORY */
#define AddedScannedDBsAlert						1011
#define ScannedToDosAlert								1012



// strings ----------------------------------------------------------------------------------------

#ifdef CONFIG_HLP_STRS
	#define DetailsHelpString								1000
	#define ProjectNameHelpString						1001
	#ifdef CONFIG_DEF_CATEGORY
		#define DefaultCategoriesHelpString 		1002
	#endif /* CONFIG_DEF_CATEGORY */
#endif	/* CONFIG_HLP_STRS */

#define SelectDueDateTitle							1003
#define SelectBeginDateTitle						1004
#define SelectEndDateTitle							1005
#define ExpMemoNewRecFailed							1006
#define ExpEmptyItemError								1007
#define ExpDBCantOpenError							1008
#define ExpNewRecFailed									1009
#define ErrorDBNotFound									1010
#define EnterANameForProject						1011
#define DBAlreadyExists									1012
#define ErrorDBNotCreated								1013
#define DuplicateProjectTitle						1014
#define NotAllRecordsCouldBeDuplicated	1015
#define FindHeaderString								1016
#define MemoDBNotFound									1017
#define ToDoDBName4Export								1018
#define DateDBName4Export								1019

#ifdef CONFIG_EXT_ABOUT
	#define ExtAboutTitle										1020
	#define ExtAboutAuthor									1021
	#define ExtAboutEmail						 				1022
	#define ExtAboutWWW											1023
	#define ExtAboutLicence1								1024
	#define ExtAboutLicence2								1025
	#define ExtAboutBuildTitle1							1026
	#define ExtAboutFollowingFlagsString 		1027
	#define ExtAboutEppilog1								1028
	#define ExtAboutEppilog2								1029
	#define ExtAboutTranslatedBy						1030
	#define ExtAboutTranslator							1031
	#define ExtAboutTranslatorEmail					1032
#endif /* CONFIG_EXT_ABOUT */

#define NoProjectsYet                     1033
#define MemoLookupTitleString							1034


#define LoginNamePopTrigger							1035
#define LoginNameList								1036


// bitmaps ----------------------------------------------------------------------------------------

#define HomeBitmap											1
#define AppIconBitmap										2
#define PreferencesBitmap 							3
#define AppIconBitmap64x64							4
#define PopupTriangleBitmap 						5
