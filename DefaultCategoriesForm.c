#include <PalmOS.h>
#include "ProjectsDB.h"
#include "Projects.h"
#include "ProjectsRcp.h"

#ifndef __PROJECTS_CONFIG_H__
	#error "include config.h"
#endif /* __PROJECTS_CONFIG_H__ */

#ifdef CONFIG_HANDERA
	#include "VGA.h"
#endif /* CONFIG_HANDERA */

#ifdef CONFIG_DEF_CATEGORY

static Char (* gDefaultCategoriesBuffer)[dmCategoryLength] = 0;		// used only in the default category form
static Boolean	gCategoriesChanged;
 
static void 		DefaultCategoriesInit( FormType * frmP );
static void 		DefaultCategoriesDrawListItem( Int16 itemNum, RectangleType * bounds, Char ** listItems );
static void 		DefaultCategoriesRanameCurrentCategory( FormType * frmP );
static void 		DefaultCategoriesDeleteCurrentCategory( FormType * frmP );
static void 		DefaultCategoriesCreateNewCategory( FormType * frmP );
static void 		DefaultCategoriesApplyChanges( void );

/*
 * FUNCTION:			DefaultCategoriesApplyChanges
 * DESCRIPTION:		this function applies the changes
 * 								it opens the executable db with
 * 								readwrite mode resizes the app info
 * 								string rsc and writes it down.
 * RETURNS:				nothing
 */
static void DefaultCategoriesApplyChanges( void )
{
	DmOpenRef 	appDB;
	MemHandle		memH;
	MemHandle		newH;
	Char *			memP;
	UInt32			size;
	UInt16			i;

	ErrFatalDisplayIf( !gCategoriesChanged, "no need to write the unchanged categories" );

	appDB = DmOpenDatabaseByTypeCreator( 'appl', appFileCreator, dmModeReadWrite );
	if( !appDB )
		return;

	size = 0;
	for( i = 0; i < dmRecNumCategories; i++ )
	{
		size += StrLen( gDefaultCategoriesBuffer[i] );
		size++;
	}
		
	// THIS IS PROJECTS SPECIFIC; WHEN USING FOR ANOTHER APPLICATION PLEASE
	// CHANGE THE FOLLOWING LINE TO THE APPROPRIATE VALUES
	memH = DmGet1Resource( appInfoStringsRsc, ProjectsToDoCategoriesAppInfoStr );
	ErrFatalDisplayIf( !memH, "resource not found" );
	newH = DmResizeResource( memH, size );
	if( !newH )
	{
		DmCloseDatabase( appDB );
		DmReleaseResource( memH );
		return;
	}

	memP = MemHandleLock( newH );
	DmSet( memP, 0, size, 0 );
	size = 0;			// is now used as the offset
	for( i = 0; i < dmRecNumCategories; i++ )
	{
		if( *gDefaultCategoriesBuffer[i] )
		{
			DmStrCopy( memP, size, gDefaultCategoriesBuffer[i] );
			size += StrLen( gDefaultCategoriesBuffer[i] );
			size++;
		}
		else
			break;
	}

	MemHandleUnlock( newH );
	DmReleaseResource( newH );
	DmCloseDatabase( appDB );
}

/*
 * FUNCTION:			DefaultCategoriesCreateNewCategory
 * DESCRIPTION:		lets the user enter a new category
 *                or displays an alert if there are
 *                already dmRecNumCategories - 1
 * PARAMETERS:		frmP - pointer to the dialog
 * RETURNS:				nothing
 */
static void DefaultCategoriesCreateNewCategory( FormType * frmP )
{
	FormType *  editP;
	Char *			s;
	ListType * 	listP;
	FieldType * fieldP;
	Int16				numCats;
	Int16				index;
	UInt16			len;

	listP = FrmGetObjectPtr( frmP, FrmGetObjectIndex( frmP, DefaultCategoriesList ) );
	numCats = LstGetNumberOfItems( listP );
	numCats++;		// because of the hidden "unfiled" item
	if( numCats >= dmRecNumCategories )
	{
		FrmAlert( AllCategoriesUsedAlert );
		return;
	}

	editP = FrmInitForm( EditDefaultCategoryForm );
	index = FrmGetObjectIndex( editP, EditDefaultCategoryField );
	fieldP = FrmGetObjectPtr( editP, index );
	FrmSetActiveForm( editP );				// because of the graffitti shift indicator
	FrmSetFocus( editP, index );
	
	if( FrmDoDialog( editP ) != EditDefaultCategoryOKButton )
	{
		FrmSetActiveForm( frmP );
		FrmDeleteForm( editP );
		return;
	}

	len = FldGetTextLength( fieldP );
	s = FldGetTextPtr( fieldP );

	if( !len || !s )
	{
		FrmSetActiveForm( frmP );
		FrmDeleteForm( editP );
		return;
	}

	// len is now used as temporary i variable
	index = -1;
	for( len = 0; len < dmRecNumCategories; len++ )
	{
		if( *gDefaultCategoriesBuffer[len] )
		{
			if( !StrCompare( gDefaultCategoriesBuffer[len], s ) )
			{
				index = len;
				break;
			}
		}
		else
			break;
	}
	
	if( index == -1 )
	{
		gCategoriesChanged = true;
		StrCopy( gDefaultCategoriesBuffer[numCats], s );
		index = numCats;
		LstSetListChoices( listP, NULL, numCats );
	}

	FrmSetActiveForm( frmP );
	FrmDeleteForm( editP );

	if( index )
	{
		index--;
		LstMakeItemVisible( listP, index );
		LstSetSelection( listP, index );
	}
	else	// index == 0 -> "unfiled" item
		LstSetSelection( listP, -1 );

	LstDrawList( listP );
}

/*
 * FUNCTION:			DefaultCategoriesDeleteCurrentCategory
 * DESCRIPTION:		Deletes the currently selected category
 * 								from our buffer
 * PARAMETERS:		frmP - pointer to the dialog
 * RETURNS:				nothing
 */
static void DefaultCategoriesDeleteCurrentCategory( FormType * frmP )
{
	ListType * 	listP;
	Int16				i;

	listP = FrmGetObjectPtr( frmP, FrmGetObjectIndex( frmP, DefaultCategoriesList ) );
	i = LstGetSelection( listP );

	if( i == -1 )
	{
		FrmAlert( SelectCategoryAlert );
		return;
	}

	i++;		// because we overjump the first item "unfiled"

	gCategoriesChanged = true;

	*gDefaultCategoriesBuffer[i] = '\0';	// delete the string

	// copy the strings after the deleted one field forward
	for( i++; i < dmRecNumCategories; i++ )
		StrCopy( gDefaultCategoriesBuffer[i-1], gDefaultCategoriesBuffer[i] );
	*gDefaultCategoriesBuffer[dmRecNumCategories-1] = '\0';

	LstSetSelection( listP, -1 );
	LstSetListChoices( listP, NULL, LstGetNumberOfItems(listP) - 1 );
	LstDrawList( listP );
}

/*
 * FUNCTION:			DefaultCategoriesRanameCurrentCategory
 * DESCRIPTION:		if there is a category selected it will be renamed
 * PARAMTERS:			frmP - pointer to  the default category form
 * RETURNS:				nothing
 */
static void DefaultCategoriesRanameCurrentCategory( FormType * frmP )
{
	ListType * 	listP;
	FieldType * fieldP;
	FormType *  editP;
	Char *			s;
	MemHandle		memH;
	UInt16			index;
	UInt16			len;
	Int16				curItemIndex;
	Boolean			deleted = false;

	listP = FrmGetObjectPtr( frmP, FrmGetObjectIndex( frmP, DefaultCategoriesList ) );
	curItemIndex = LstGetSelection( listP );
	if( curItemIndex == -1 )
	{
		FrmAlert( SelectCategoryAlert );
		return;
	}

	curItemIndex++;

	memH = MemHandleNew( dmCategoryLength );
	if( !memH )
		return;

	editP = FrmInitForm( EditDefaultCategoryForm );
	index = FrmGetObjectIndex( editP, EditDefaultCategoryField );
	fieldP = FrmGetObjectPtr( editP, index );

	len = StrLen( gDefaultCategoriesBuffer[curItemIndex] );
	StrCopy( MemHandleLock( memH ), gDefaultCategoriesBuffer[curItemIndex] );
	MemHandleUnlock( memH );
	FldSetTextHandle( fieldP, memH );
	FldSetSelection( fieldP, 0, len );
	FrmSetActiveForm( editP );				// because of the graffitti shift indicator
	FrmSetFocus( editP, index );
	
	if( FrmDoDialog( editP ) != EditDefaultCategoryOKButton )
	{
		FrmSetActiveForm( frmP );
		FrmDeleteForm( editP );
		return;
	}

	len = FldGetTextLength( fieldP );
	s = FldGetTextPtr( fieldP );
	if( len && s )
	{
		// look for an equal string
		for( index  = 0; index < dmRecNumCategories; index++ )
		{
			if( !*gDefaultCategoriesBuffer[index] )
				break;

			if( !StrCompare( gDefaultCategoriesBuffer[index], s ) )
			{
				if( index && index != curItemIndex )	
					// not "unfiled" item and not the current item 
					// (this would mean that the user didn't change the
					// name, but tapped the ok button)
				{
					for( index++; index < dmRecNumCategories; index++ )
						StrCopy( gDefaultCategoriesBuffer[index-1], gDefaultCategoriesBuffer[index] );
					*gDefaultCategoriesBuffer[dmRecNumCategories-1] = '\0';
					deleted = true;
					gCategoriesChanged = true;
				}
				else
				{
					LstSetSelection( listP, -1 );
					FrmSetActiveForm( frmP );
					FrmDeleteForm( editP );
					LstDrawList( listP );
					return;
				}

				break;
			}
		}

		StrCopy( gDefaultCategoriesBuffer[curItemIndex], s );
		gCategoriesChanged = true;
	}

	FrmSetActiveForm( frmP );
	FrmDeleteForm( editP );
	
	if( len )
	{
		LstMakeItemVisible( listP, curItemIndex-1 );
		LstSetSelection( listP, curItemIndex-1 );
		if( deleted )
			LstSetListChoices( listP, NULL, LstGetNumberOfItems( listP ) - 1 );
		LstDrawList( listP );
	}
}

/*
 * FUNCTION:			DefaultCategoriesDrawListItem 
 * DESCRIPTION:		draws a category into the list (callback function)
 * PARAMETERS:		see palm sdk LstSetDrawFunction
 * RERTURNS:			nothing
 */
static void DefaultCategoriesDrawListItem( Int16 itemNum, RectangleType * bounds, Char ** listItems )
{
	ErrFatalDisplayIf( !gDefaultCategoriesBuffer, "no buffer" );

	WinDrawChars( gDefaultCategoriesBuffer[itemNum+1], StrLen( gDefaultCategoriesBuffer[itemNum+1] ), bounds->topLeft.x + 1, bounds->topLeft.y );
}

/*
 * FUNCTION:			DefaultCategoriesInit
 * DESCRIPTION:		initializes our default categories form
 * 								and the global data associated with the form
 * PARAMETERS:		frmP - pointer to the default categories form
 * CALLED:				when the event handled for the form recieved
 * 								frmOpenEvent
 * RETURNS:				nothing
 */
static void DefaultCategoriesInit( FormType * frmP )
{
	ListType * 	listP;
	Char *			memP;
	MemHandle		memH;
	UInt16			i;
	UInt16			numCats;
	
	ErrFatalDisplayIf( gDefaultCategoriesBuffer, "buffer was not freed" );

#ifdef CONFIG_HANDERA
	if( gGlobalPrefs.vgaExtension )
		VgaFormModify( frmP, vgaFormModify160To240 );
#endif /* CONFIG_HANDERA */

	gDefaultCategoriesBuffer = MemPtrNew( dmRecNumCategories * dmCategoryLength );
	if( !gDefaultCategoriesBuffer )
		return;

	memH = DmGetResource( appInfoStringsRsc, ProjectsToDoCategoriesAppInfoStr );
	ErrFatalDisplayIf( !memH, "resource not found" );
	memP = MemHandleLock( memH );
	
	numCats = 0;
	for( i = 0; i < dmRecNumCategories; i++ )
	{
		StrCopy( gDefaultCategoriesBuffer[i], memP );

		if( *memP )
			numCats++;

		memP += (StrLen( memP ) + 1);
	}

	MemHandleUnlock( memH );
	DmReleaseResource( memH );

	gCategoriesChanged = false;

	listP = FrmGetObjectPtr( frmP, FrmGetObjectIndex( frmP, DefaultCategoriesList ) );
	LstSetListChoices( listP, NULL, numCats - 1 ); // the very first item "unfiled" will not be editable
	LstSetDrawFunction( listP, DefaultCategoriesDrawListItem );
}


// exported functions -----------------------------------------------------------------------------

/*
 * FUNCTION:			DefaultCategoriesHandleEvent
 * DESCRIPTION:		event handler for the DefaultCategoriesForm
 * PARAMETERS:		eventP - pointer to a event structure
 * RETURNS:				true - if the event was completely handled
 * 								false - else
 */
Boolean DefaultCategoriesHandleEvent( EventType * eventP )
{
	Boolean	handled = false;
	FormType *frmP;

	switch( eventP->eType )
	{
		case frmOpenEvent:
			frmP = FrmGetActiveForm();
			DefaultCategoriesInit( frmP );
			FrmDrawForm( frmP );
			handled = true;
			break;

		case frmCloseEvent:
			if( gDefaultCategoriesBuffer )
			{
				MemPtrFree( gDefaultCategoriesBuffer );
				gDefaultCategoriesBuffer = 0;
			}
			break;

		case ctlSelectEvent:
			if( eventP->data.ctlSelect.controlID == DefaultCategoriesOKButton )
			{
				if( gDefaultCategoriesBuffer )
				{
					if( gCategoriesChanged )
						DefaultCategoriesApplyChanges();

					MemPtrFree( gDefaultCategoriesBuffer );
					gDefaultCategoriesBuffer = 0;
				}
				FrmReturnToForm(0);
				handled = true;
			}
			else if( eventP->data.ctlSelect.controlID == DefaultCategoriesRenameButton )
			{
				if( gDefaultCategoriesBuffer )
				{
					frmP = FrmGetActiveForm();
					DefaultCategoriesRanameCurrentCategory( frmP );
				}
				handled = true;
			}
			else if( eventP->data.ctlSelect.controlID == DefaultCategoriesDeleteButton )
			{
				if( gDefaultCategoriesBuffer )
				{
					frmP = FrmGetActiveForm();
					DefaultCategoriesDeleteCurrentCategory( frmP );
				}
				handled = true;
			}
			else if( eventP->data.ctlSelect.controlID == DefaultCategoriesNewButton )
			{
				if( gDefaultCategoriesBuffer )
				{
					frmP = FrmGetActiveForm();
					DefaultCategoriesCreateNewCategory( frmP );
				}
				handled = true;
			}
			break;
			
		default:
			break;
	}

	return (handled);
}

#endif /* CONFIG_DEF_CATEGORY */
