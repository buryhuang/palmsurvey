/* FOR BEST FORMATING OF THIS FILE SET TABSTOP TO 2 */

#include <PalmOS.h>
#include "ProjectsDB.h"

// deklarations -----------------------------------------------------------------------------------

static Err 		PrjtDBInitMainDB( DmOpenRef db );
static Int16	PrjtDBCompareCategory( UInt8 attr1, UInt8 attr2, MemHandle appInfoH );
static Int16 	PrjtDBCompareDateType(DateType d1, DateType d2);

// implementations --------------------------------------------------------------------------------

/*
 * FUNCTION:					PrjtDBGetToDoLockedInfoPtr
 * DESCRIPTION:			  this functions locks the info block of tha passed database
 *										and returns a pointer to it
 * PARAMTERS:					todoDB - the todo database
 * RETURNS:						a locked info ptr of the passed database or NULL
 */
PrjtToDoDBInfoType * PrjtDBGetToDoLockedInfoPtr( DmOpenRef todoDB )
{
	UInt16	cardNo;
	LocalID	dbID;
	LocalID infoID;

	ErrFatalDisplayIf( !todoDB, "database is not open" );

	if( DmOpenDatabaseInfo( todoDB, &dbID, NULL, NULL, &cardNo, NULL ) )
		return NULL;
	if( DmDatabaseInfo( cardNo, dbID, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &infoID, NULL, NULL, NULL ) )
		return NULL;
	return MemLocalIDToLockedPtr( infoID, cardNo );
}

/*
 * FUNCTION:					PrjtDBCompareDateType
 * DESCRIPTION:				this routine compares two dates
 * PARAMETERS:				d1, d2 - the dates
 * RETURNS:						0 if equal
 *										< 0 if d1 < d2
 *										> 0 if d1 > d2
 */
static Int16 PrjtDBCompareDateType(DateType d1, DateType d2)
{
	Int16 ret_val;
	
	ret_val = d1.year - d2.year;
	if( ret_val )
	{
		if( DateToInt( d1 ) == kToDoNoDueDate )
			return 1;
		if( DateToInt( d2 ) == kToDoNoDueDate )
			return -1;
		return ret_val;
	}
	
	ret_val = d1.month - d2.month;
	if( ret_val )
		return ret_val;
	
	ret_val = d1.day - d2.day;
	return ret_val;
}

/*
 *
 * FUNCTION:				PrjtDBCompareCategory
 * DESCRIPTION:			this routine compares two categories 
 *									identified by records' attributes and appInfoH
 * PARAMETERS:			attr1, attr2 - attributes (see DmRecordInfo) of two records
 *									appInfoH - handle to the appInfoBlock
 * RETURNS:					0 - if equal
 *									> 0 - if s1 > s2
 *									< 0 - if s1 < s2
 */
static Int16	PrjtDBCompareCategory( UInt8 attr1, UInt8 attr2, MemHandle appInfoH )
{
	AppInfoType * appP;
	Int16			ret;
	UInt8			cat1;
	UInt8			cat2;

	cat1 = attr1 & dmRecAttrCategoryMask;
	cat2 = attr2 & dmRecAttrCategoryMask;

	ret = cat1 - cat2;
	if( !ret )
	{
		if( cat1 == dmUnfiledCategory )
			return (1);
		else if( cat2 == dmUnfiledCategory )
			return (-1);

		appP = MemHandleLock( appInfoH );
		ret = StrCompare( appP->categoryLabels[cat1], appP->categoryLabels[cat2] );
		MemPtrUnlock( appP );
	}

	return (ret);
}

/*
 * FUNCTION:				PrjtDBToDoToggleCompletionFlag
 * DESCRIPTION:			this routine toggles the completion flag of a todo record
 *									and updates the counters for the appropriate project record
 * PARAMETERS:			mainDB - reference to the open main database
 * 									prjt_index - index of the project record (in mainDB)
 *									todoDB - reference to the open todo database
 *									todo_index - index of the todo item record (in todoDB)
 *									completeDueDate - if true the todo record's date will be set to today if we mark the record 'complete'
 * RETURNS:					true if the record was marked 'completed', false if the record's complete flag was cleared.
 */
Boolean	PrjtDBToDoToggleCompletionFlag( DmOpenRef mainDB, UInt16 prjt_index, DmOpenRef todoDB, UInt16 todo_index, Boolean completeDueDate )
{
	PrjtPackedProjectType * prjtP;
	PrjtToDoType * 	todoP;
	MemHandle				h;
	DateType				now;
	UInt16					num;
	UInt8						tmp;
	Boolean					completeIt;

	ErrFatalDisplayIf( !todoDB, "invalid param" );
	ErrFatalDisplayIf( !mainDB, "invalid param" );

	// grab the record
	h = DmGetRecord( todoDB, todo_index );
	ErrFatalDisplayIf( !h, "bad todo record" );
	todoP = MemHandleLock( h );

	// find out whether we check or uncheck the item as 'completed' 
	// and set the due date if necessary
	completeIt = ((todoP->priority & kToDosCompleteFlag) != kToDosCompleteFlag);
	if( completeIt )
	{
		tmp = todoP->priority | kToDosCompleteFlag;
		if( completeDueDate )
		{
			DateSecondsToDate( TimGetSeconds(), &now );
			DmWrite( todoP, 0, &now, sizeof( todoP->dueDate ) );
		}
	}
	else
		tmp = todoP->priority & ~kToDosCompleteFlag;

	// write the complete flag and release the record
	DmWrite( todoP, OffsetOf( PrjtToDoType, priority ), &tmp, sizeof( todoP->priority ) );
	MemHandleUnlock( h );
	DmReleaseRecord( todoDB, todo_index, true );

	// now update the projects information numbers
	h = DmGetRecord( mainDB, prjt_index );
	ErrFatalDisplayIf( !h, "bad project record" );
	prjtP = MemHandleLock( h );
	
	num = prjtP->numOfFinishedToDos;
	if( completeIt )
		num++;
	else
		num--;

	DmWrite( prjtP, OffsetOf( PrjtPackedProjectType, numOfFinishedToDos ), &num, sizeof( prjtP->numOfFinishedToDos ) );
	MemHandleUnlock( h );
	DmReleaseRecord( mainDB, prjt_index, true );

	return (completeIt);
}

/*
 * FUNCTION:			PrjtDBGetToDoDBInformation
 * DESCRIPTION:		this routine opens the databases app info block and reads the sortOrder and the last category
 * PARAMETERS:		db - the database
 *								lastCategory & sortOrder - to be returned
 * RETURNS:				nothing
 */
void PrjtDBGetToDoDBInformation( DmOpenRef todoDB, UInt16 * lastCategory, UInt8 * sortOrder )
{
	PrjtToDoDBInfoType * infoP;
	UInt16		cardNo;
	LocalID		dbID;
	LocalID		infoID;

	ErrFatalDisplayIf( !lastCategory || !sortOrder, "invalid param" );
	ErrFatalDisplayIf( !todoDB, "invalid param" );

	DmOpenDatabaseInfo( todoDB, &dbID, NULL, NULL, &cardNo, NULL );
	DmDatabaseInfo( cardNo, dbID, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &infoID, NULL, NULL, NULL );
	ErrFatalDisplayIf( !infoID, "no info block" );
	infoP = MemLocalIDToLockedPtr( infoID, cardNo );
	*sortOrder = infoP->sortOrder;
	*lastCategory = infoP->lastCategory;
	MemPtrUnlock( infoP );
}

/*
 * FUNCTION:			PrjtDBGetLastCategory
 * DESCRIPTION:		this routine returns the category
 *								that was lastly visited when the database
 *								was open
 * PARAMETERS:		todoDB - reference to an onpen todo database
 * RETURNS:				category index or dmAllCategories
 */
UInt16 PrjtDBGetToDoLastCategory( DmOpenRef todoDB )
{
	PrjtToDoDBInfoType * infoP;
	UInt16		cardNo;
	LocalID		dbID;
	LocalID		infoID;
	UInt16		category;

	ErrFatalDisplayIf( !todoDB, "invalid param" );

	DmOpenDatabaseInfo( todoDB, &dbID, NULL, NULL, &cardNo, NULL );
	DmDatabaseInfo( cardNo, dbID, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &infoID, NULL, NULL, NULL );
	ErrFatalDisplayIf( !infoID, "no info block" );
	infoP = MemLocalIDToLockedPtr( infoID, cardNo );
	category = infoP->lastCategory;
	MemPtrUnlock( infoP );

	return (category);
}

/*
 * FUNCTION:			PrjtDBSetToDoLastCategory
 * DESCRIPTION:		this routine sets the lastCategory member of
 *								the database info block of the passed database
 *								to the passed category
 * PARAMETERS:		todoDB - reference to an open database
 *								category - to be stored
 * RETURNS:				nothing
 */
void PrjtDBSetToDoLastCategory( DmOpenRef todoDB, UInt16 category )
{
	PrjtToDoDBInfoType * 	infoP;
	UInt8									lastCategory;

	ErrFatalDisplayIf( !todoDB, "invalid param" );

	infoP = PrjtDBGetToDoLockedInfoPtr( todoDB );
	ErrFatalDisplayIf( !infoP, "database has no info block" );
	lastCategory = category;
	DmWrite( infoP, OffsetOf( PrjtToDoDBInfoType, lastCategory ), &lastCategory, sizeof( infoP->lastCategory ) );
	MemPtrUnlock( infoP );
}

/*
 * FUNCTION:			PrjtDBGetToDoSortOrder
 * DESCRIPTION:		this routine determines the sort order of the passed todo database
 * PARAMETERS:		todoDB - reference to an open todo database
 * RETURNS:				the sort order of the passed database
 */
UInt8 PrjtDBGetToDoSortOrder( DmOpenRef todoDB )
{
	PrjtToDoDBInfoType * infoP;
	UInt16		cardNo;
	LocalID		dbID;
	LocalID		infoID;
	UInt8			sortOrder;

	ErrFatalDisplayIf( !todoDB, "invalid param" );

	DmOpenDatabaseInfo( todoDB, &dbID, NULL, NULL, &cardNo, NULL );
	DmDatabaseInfo( cardNo, dbID, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &infoID, NULL, NULL, NULL );
	ErrFatalDisplayIf( !infoID, "no info block" );
	infoP = MemLocalIDToLockedPtr( infoID, cardNo );
	sortOrder = infoP->sortOrder;
	MemPtrUnlock( infoP );

	return (sortOrder);
}

/*
 * FUNCTION:			PrjtDBSetProjectSecretBit
 * DESCRIPTION:		this routine toggles the private (secret) bit
 * 								of the current project record.
 * PARAMETERS:		set - if true the secret bit will be set
 * 								otherwise it will be cleared
 * RETURNS:				nothing
 */
void PrjtDBSetProjectSecretBit( DmOpenRef mainDB, UInt16 index, Boolean set )
{
	UInt16		attr;

	ErrFatalDisplayIf( !mainDB, "invalid param" );
	ErrFatalDisplayIf( !DmQueryRecord( mainDB, index ), "bad record" );

	if( !DmRecordInfo( mainDB, index, &attr, NULL, NULL ) )
	{
		if( set )
			attr |= dmRecAttrSecret;
		else
			attr &= ~dmRecAttrSecret;
		DmSetRecordInfo( mainDB, index, &attr, NULL );
	}
}

/*
 * FUNCTION:			PrjtDBRenameProject
 * DESCRIPTION:		this routine renames the project in our main db and ONLY THERE!!!
 * 								IT DOES NOT RENAME THE APPROPRIATE TODO DATABASE.
 * PARAMETERS:		mainDB - reference to the open main database,
 * 								sortOrder - of the main database
 * 								index - [in][out] the projects index (upon return this will recieve the
 * 									projects new index if changed)
 * 								newName - new name of the project
 * RETURNS:				errNone - on success, ontherwise something else
 */
Err PrjtDBRenameProject( DmOpenRef mainDB, UInt16 sortOrder, UInt16 * index, const Char * newName )
{
	PrjtPackedProjectType * 	prjtP;
	UInt32										newSize;
	Char *										tmpP;
	Char *										s;
	MemHandle									prjtH;
	UInt16										memoLen;
	UInt16										nameLen;
	UInt16										newNameLen;

	ErrFatalDisplayIf( !mainDB || !index || !newName, "invalid param" );

	prjtH = DmGetRecord( mainDB, *index );
	if( !prjtH )
		return (dmErrIndexOutOfRange);
	
	prjtP = MemHandleLock( prjtH );
	s = &prjtP->name;
	nameLen = StrLen( s );
	s = (&prjtP->name + nameLen + 1);
	memoLen = StrLen( s );
	newNameLen = StrLen( newName );
	newSize = sizeof( PrjtPackedProjectType ) + newNameLen + memoLen;

	// if the length of the new name is shorter
	// rename, move the description and then 
	// resize the record
	if( newNameLen <= nameLen )
	{
		DmStrCopy( prjtP, OffsetOf( PrjtPackedProjectType, name ), newName );
		DmStrCopy( prjtP, OffsetOf( PrjtPackedProjectType, name ) + newNameLen + 1, (&prjtP->name) + nameLen + 1 );

		MemHandleUnlock( prjtH );
		DmResizeRecord( mainDB, *index, newSize );
		DmReleaseRecord( mainDB, *index, true );
	}
	// first resize the record to the desired size then move the memo
	// of the project back and then rename the project
	else
	{
		tmpP = MemPtrNew( memoLen + 1 );
		if( !tmpP )
		{
			DmReleaseRecord( mainDB, *index, false );
			return (dmErrMemError);
		}

		MemHandleUnlock( prjtH );
		prjtH = DmResizeRecord( mainDB, *index, newSize );
		if( !prjtH )
			return (DmGetLastErr());

		prjtP = MemHandleLock( prjtH );
		StrCopy( tmpP, &prjtP->name + nameLen + 1 );	// make a temporary copy of the memo
		DmStrCopy( prjtP, OffsetOf( PrjtPackedProjectType, name ), newName );		// write the new name
		DmStrCopy( prjtP, OffsetOf( PrjtPackedProjectType, name ) + newNameLen + 1, tmpP );	// write the copied memo
		MemPtrFree( tmpP );	// free the copy of the project memo
		
		MemHandleUnlock( prjtH );
		DmReleaseRecord( mainDB, *index, true );
	}

	if( sortOrder != kSortMainDBManually )
		PrjtDBReSortChangedProjectRecord( mainDB, sortOrder, index );

	return (errNone);
}

/*
 * FUNCTION:			PrjtDBRenameToDoDB
 * DESCRIPTION:		this routine sets a new name of a todo database
 * 								this means that before renaming the database this routine
 * 								appends the "PRJT" to the passed newName paramter
 * PARAMETERS:		db - reference to the open database that should be renamed
 * 								newName - new description name of the database
 * RETURNS:				errNone - on success, in othercases something else
 */
Err	PrjtDBRenameToDoDB( DmOpenRef db, const Char * newName )
{
	Char 	szFull[dmDBNameLength];
	Char * s;
	UInt16	cardNo;
	LocalID	dbID;
	UInt16	error;

	ErrFatalDisplayIf( !db || !newName, "invalid param" );

	// build the full name of the database
	s = szFull;
	while( *newName )
		*s++ = *newName++;
	StrCopy( s, appFileCreatorAsString );

	error = DmOpenDatabaseInfo( db, &dbID, NULL, NULL, &cardNo, NULL );
	if( error != errNone )
		return (error);

	error = DmSetDatabaseInfo( cardNo, dbID, szFull, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL );
	
	return (error);
}

/*
 * FUNCTION:			PrjtDBInitMainDB
 * DESCRIPTION:		this routine creates info
 * 								block of type PrjtMainDBInfoType 
 * 								and assignes it to the passed database
 * 								the info block it zeroed
 * 								it also sets the db's backup bit
 * PARAMETERS:		db - reference to an open database the info
 * 								block will be assigned to
 * RETURNS:				0 - if successfull else non-zero
 */
Err PrjtDBInitMainDB( DmOpenRef db ) 
{
	UInt16 								cardNo, attr;
	UInt16								version;
	LocalID								dbID, infoID;
	MemHandle							h;
	PrjtMainDBInfoType * 	infoP;

	if( DmOpenDatabaseInfo( db, &dbID, NULL, NULL, &cardNo, NULL ) )
		return dmErrInvalidParam;
	if( DmDatabaseInfo( cardNo, dbID, NULL, &attr, NULL, NULL, NULL, NULL, NULL, &infoID, NULL, NULL, NULL ) )
		return dmErrInvalidParam;

	if( !infoID )
	{
		h = DmNewHandle( db, sizeof( PrjtMainDBInfoType ) );
		if( !h )
			return (dmErrMemError);

		infoID = MemHandleToLocalID( h );
		attr |= dmHdrAttrBackup;
		version = kMainDBVersion;					// set the application version so 
																			// so if there will be changes in the
																			// database format in future version
																			// we can easily determine what format
																			// a database does have
		DmSetDatabaseInfo( cardNo, dbID, NULL, &attr, &version, NULL, NULL, NULL, NULL, &infoID, NULL, NULL, NULL );

		// init the block to zero
		infoP = MemLocalIDToLockedPtr( infoID, cardNo );
		DmSet( infoP, 0, sizeof( PrjtMainDBInfoType ), 0 );
		CategoryInitialize( (AppInfoType *)infoP, ProjectsCategoriesAppInfoStr );
		MemPtrUnlock( infoP );
	}

	return 0;
}

/*
 * FUNCTION:					PrjtDBReSortChangedProjectRecord
 * DESCRIPTION:				this routine resorts the main database
 * 										by finding out the correct position of the
 * 										changed project and position it there
 * PARAMETERS:				db - reference to the open main db
 * 										sortOrder - sort order of the main db
 * 										recIndex - index of the record, will 
 * 																recieve the new position
 * RETURNS:						true if the records position changed,
 * 										else false
 */
Boolean PrjtDBReSortChangedProjectRecord( DmOpenRef mainDB, UInt16 sortOrder, UInt16 * recIndex )
{
	SortRecordInfoType 	sortInfo;
	UInt16					attr;
	UInt16					index;
	MemHandle				recH;
	Err							error;

	ErrNonFatalDisplayIf( sortOrder == kSortMainDBManually, "no need to sort the project, sort order is manual" );
	ErrFatalDisplayIf( !mainDB || !recIndex, "invalid param" );

	// for later use
	index = *recIndex;

	// get the info (because of the category)
	error = DmRecordInfo( mainDB, *recIndex, &attr, NULL, NULL );
	ErrFatalDisplayIf( error != errNone, "no rec info" );

	// detach the record from the database
	error = DmDetachRecord( mainDB, *recIndex, &recH );
	ErrFatalDisplayIf( error != errNone, "rec not detached" );
	ErrFatalDisplayIf( !recH, "bad record" );

	// find out the new sort position
	sortInfo.attributes = attr;
	*recIndex = DmFindSortPosition( mainDB, MemHandleLock( recH ), &sortInfo, (DmComparF *)PrjtDBCompareProjectsRecords, sortOrder );
	MemHandleUnlock( recH );

	// attach the record to the database in the right position
	error = DmAttachRecord( mainDB, recIndex, recH, NULL );
	ErrFatalDisplayIf( error != errNone, "rec not attached" );

	// set the category (attributes) of the record again
	error = DmSetRecordInfo( mainDB, *recIndex, &attr, NULL );
	ErrFatalDisplayIf( error != errNone, "rec attrs not set" );

	return (index != *recIndex);
}

/*
 * FUNCTION:					PrjtDBGetMainDB
 * DESCRIPTION:				this routine opens our main db
 * 										and returns a reference to it.
 * 										if the database does not exist
 * 										this function will create and
 * 										initialize it.
 * PARAMETERS:				create - if true and the db does not exist the db will be created
 * 										openMode - how to open (dmModeShowReadOnly, dmModeShowSecret ...)
 * RETURNS:						0 - on failure, else a reference to the open database
 */
DmOpenRef PrjtDBGetMainDB( Boolean create, UInt16 openMode )
{
	DmOpenRef db;
	UInt16		cardNo;
	LocalID		dbID;
	Err				error;

	// open the database
	db = DmOpenDatabaseByTypeCreator( kMainDBType, appFileCreator, openMode );
	if( !db && create )
	{
		// create the database
		error = DmCreateDatabase( kCardNo, kMainDBName, appFileCreator, kMainDBType, false );
		if( error )
			return (0);

		// open it
		db = DmOpenDatabaseByTypeCreator( kMainDBType, appFileCreator, openMode );
		if( !db )
			return (0);
		
		error = PrjtDBInitMainDB( db );
		if( error )
		{
			// on error delete the database and return
			DmOpenDatabaseInfo( db, &dbID, NULL, NULL, &cardNo, NULL );
			DmCloseDatabase( db );
			DmDeleteDatabase( cardNo, dbID );
			db = 0;
		}
	}

	return (db);
}

/*
 * FUNCTION:					PrjtDBOpenToDoDB
 * DESCRIPTION:				this routine opens a todo database given its project name
 * PARAMETERS:				name - name of the project
 * 										mode - dmModeReadWrite, dmModeReadOnly ...
 * RETURNS:						0 - on failure, else reference to an open database
 */
DmOpenRef PrjtDBOpenToDoDB( Char * name, UInt16 mode )
{
	Char 				szFullName[dmDBNameLength];
	Char *			s;
	LocalID			dbID;
	DmOpenRef		db = 0;

	ErrFatalDisplayIf( !name, "invalid param" );

	s = szFullName;
	while( *name )
		*s++ = *name++;
	StrCopy( s, appFileCreatorAsString );

	dbID = DmFindDatabase( kCardNo, szFullName );
	if( dbID )
		db = DmOpenDatabase( kCardNo, dbID, mode );

	return (db);
}

/*
 * FUNCTION:					PrjtDBCreateToDoDB
 * DESCRIPTION:				this routine creates a todo database
 * PARAMETERS:				name - name of the project
 * RETURNS:						0 - on failure, else a reference to an open database
 */
DmOpenRef PrjtDBCreateToDoDB( Char * name, Boolean initAppInfo )
{
	Char 							szFullName[dmDBNameLength];
	PrjtToDoDBInfoType * infoP;
	DmOpenRef 				db;
	LocalID 					lID;
	UInt16						cardNo;
	UInt16						attributes;
	MemHandle					infoH;
	LocalID						infoID = 0;
	UInt16						wordValue;
	UInt8							byteValue;

	StrCopy( szFullName, name );
	StrCat( szFullName, appFileCreatorAsString );

	cardNo = kCardNo;
	if( DmCreateDatabase( cardNo, szFullName, appFileCreator, kPrjtToDoDBType, false ) )
		return (0);

	lID = DmFindDatabase( cardNo, szFullName );
	if( !lID )
		return (0);

	db = DmOpenDatabase( cardNo, lID, dmModeReadWrite );
	if( db )
	{
		infoH = DmNewHandle( db, sizeof( PrjtToDoDBInfoType ) );
		if( !infoH )
		{
			DmCloseDatabase( db );
			DmDeleteDatabase( cardNo, lID );
			return 0;
		}

		// initialize the appinfotype
		infoID = MemHandleToLocalID( infoH );
		attributes = dmHdrAttrBackup;
		DmSetDatabaseInfo( cardNo, lID, NULL, &attributes, NULL, NULL, NULL, NULL, NULL, &infoID, NULL, NULL, NULL );

		if( initAppInfo )
		{
			infoP = MemHandleLock( infoH );
			DmSet( infoP, 0, sizeof( PrjtToDoDBInfoType ), 0 );

			CategoryInitialize( (AppInfoPtr)infoP, ProjectsToDoCategoriesAppInfoStr );

			wordValue = 0xFFFF;		// i don't know -> see ToDo-Example
			DmWrite( infoP, OffsetOf( PrjtToDoDBInfoType, dirtyAppInfo ), &wordValue, sizeof( infoP->dirtyAppInfo ) );

			// next (first) time the database is opened the 'all' category gets assigned as current category
			byteValue = dmAllCategories;
			DmWrite( infoP, OffsetOf( PrjtToDoDBInfoType, lastCategory ), &byteValue, sizeof( infoP->lastCategory ) );

			// init the sort order
			DmSet( infoP, OffsetOf( PrjtToDoDBInfoType, sortOrder ), sizeof( infoP->sortOrder ), kSortToDosByPriorityDueDate );
			MemHandleUnlock( infoH );
		}
	}

	return (db);
}

/*
 * FUNCTION:				PrjtDBCreateProjectRecord
 * DESCRIPTION:			this routine creates a new projects
 * 									in db and initilizes this with the passed
 * 									template
 * PARAMETERS:			db - should be our main db, where the record will be inserted
 * 									template - how to initialize
 * 									category - which category should the new record be assigned to
 * 									sortOrder - of db
 * RETURNS:					kNoRecordIndex on failure, else the index of the new record
 */
UInt16 PrjtDBCreateProjectRecord( DmOpenRef db, PrjtUnpackedProjectType * template, UInt16 category, UInt8 sortOrder )
{
	PrjtPackedProjectType		recTmp;
	SortRecordInfoType			sortInfo;
	UInt32									size;
	PrjtPackedProjectType * recP;
	UInt16									nameLen;
	UInt16									recIndex;
	UInt16									attr;
	MemHandle								recH;
	Err											error;

	ErrFatalDisplayIf( !db || !template, "invalid param" );
	ErrFatalDisplayIf( !template->name, "project without name" );
	
	if( category == dmAllCategories )
		category = dmUnfiledCategory;

	size = sizeof( PrjtPackedProjectType );
	nameLen = StrLen( template->name );
	size += nameLen;
	if( template->note )
		size += StrLen( template->note );

	recH = DmNewHandle( db, size );
	if( !recH )
		return (kNoRecordIndex);

	recTmp.begin = template->begin;
	recTmp.end = template->end;
	recTmp.priority_status = (template->priority | template->status);
	recTmp.numOfFinishedToDos = template->numOfFinishedToDos;
	recTmp.numOfAllToDos = template->numOfAllToDos;

	recP = MemHandleLock( recH );
	DmWrite( recP, 0, &recTmp, sizeof( PrjtPackedProjectType ) - 1 );
	size = OffsetOf( PrjtPackedProjectType, name );
	DmStrCopy( recP, size, template->name );
	size += (nameLen + 1); // + 1 for the nullterminator of the name
	if( template->note )
		DmStrCopy( recP, size, template->note );
	else
		DmSet( recP, size, 1, 0 );

	if( sortOrder != kSortMainDBManually )
	{
		sortInfo.attributes = category;
		recIndex = DmFindSortPosition( db, recP, &sortInfo, (DmComparF *)PrjtDBCompareProjectsRecords, sortOrder );
	}
	else
		recIndex = 0;
		
	MemHandleUnlock( recH );
	error = DmAttachRecord( db, &recIndex, recH, NULL );
	if( error )
	{
		MemHandleFree( recH );
		return (kNoRecordIndex);
	}
	else
	{
		DmRecordInfo( db, recIndex, &attr, NULL, NULL );
		attr &= ~dmRecAttrCategoryMask;
		attr |= category;
		DmSetRecordInfo( db, recIndex, &attr, NULL );
	}

	return (recIndex);
}

/*
 * FUNCTION:					PrjtDBCompareProjectsRecords
 * DESCRIPTION:				comparison routine for sort functions
 */
Int16 PrjtDBCompareProjectsRecords( PrjtPackedProjectType *rec1, PrjtPackedProjectType *rec2, Int16 sortOrder,
		SortRecordInfoPtr rec1sortInfo, SortRecordInfoPtr rec2sortInfo, MemHandle appInfoH )
{
	Int16 ret_val;
	UInt32 days1, days2;

	ErrFatalDisplayIf( !rec1sortInfo || !rec2sortInfo, "invalid param" );

PrjtDBCompareProjectsRecords_Start:

	switch( sortOrder )
	{
		case kSortMainDBByPriorityStateName:
			ret_val = PrjtDBProjectsPriority( rec1->priority_status ) - PrjtDBProjectsPriority( rec2->priority_status );
			if( !ret_val )
			{
				ret_val = PrjtDBProjectsState( rec1->priority_status ) - PrjtDBProjectsState( rec2->priority_status );
				if( !ret_val )
					ret_val = StrCompare( &rec1->name, &rec2->name );
			}
			break;

		case kSortMainDBByStatePriorityName:
			ret_val = PrjtDBProjectsState( rec1->priority_status ) - PrjtDBProjectsState( rec2->priority_status );
			if( !ret_val )
			{
				ret_val = PrjtDBProjectsPriority( rec1->priority_status ) - PrjtDBProjectsPriority( rec2->priority_status );
				if( !ret_val )
					ret_val = StrCompare( &rec1->name, &rec2->name );
			}
			break;

		case kSortMainDBByBeginDateName:
			days1 = DateToDays( rec1->begin );
			days2 = DateToDays( rec2->begin );
			if( days1 > days2 )
				ret_val = -1;
			else if( days1 < days2 )
				ret_val = 1;
			else
				ret_val = StrCompare( &rec1->name, &rec2->name );
			break;

		case kSortMainDBByName:
			ret_val = StrCompare( &rec1->name, &rec2->name );
			break;

		case kSortMainDBManually:
			ErrNonFatalDisplay( "manual order; no need to sort the db" );
			ret_val = 0;
			break;

		case kSortMainDBByCategoryPriorityStateName:
			ret_val = PrjtDBCompareCategory( rec1sortInfo->attributes, rec2sortInfo->attributes, appInfoH );
			if( !ret_val )
			{
				sortOrder = kSortMainDBByPriorityStateName;
				goto PrjtDBCompareProjectsRecords_Start;
			}
			break;

		case kSortMainDBByCategoryStatePriorityName:
			ret_val = PrjtDBCompareCategory( rec1sortInfo->attributes, rec2sortInfo->attributes, appInfoH );
			if( !ret_val )
			{
				sortOrder = kSortMainDBByStatePriorityName;
				goto PrjtDBCompareProjectsRecords_Start;
			}
			break;

		case kSortMainDBByCategoryName:
			ret_val = PrjtDBCompareCategory( rec1sortInfo->attributes, rec2sortInfo->attributes, appInfoH );
			if( !ret_val )
			{
				sortOrder = kSortMainDBByName;
				goto PrjtDBCompareProjectsRecords_Start;
			}
			break;
			
		default:
			ret_val = 0;
	}

	return (ret_val);
}

/*
 * FUNCTION:				PrjtDBScanForNewToDoDBs
 * DESCRIPTION:			this routine look in the RAM for projects
 * 									todo database that are still not in our
 * 									main db.
 * PARAMETERS:			mainDB - reference to the open main database
 *									sortOrder - sort order of records in the main database
 * RETURNS:					the number of new inserted records into the mainDB
 */
UInt16 PrjtDBScanForNewToDoDBs( DmOpenRef mainDB, UInt8 sortOrder )
{
	Char 										szDBName[dmDBNameLength];
	DmSearchStateType 			state;
	PrjtUnpackedProjectType project;
	PrjtPackedProjectType	*	recP;
	DateType								today;
	DmOpenRef								db;
	UInt16									recIndex;
	UInt16									newDBs;
	UInt16									numRecs;								
	UInt16									appendLen;
	UInt16									cardNo;
	UInt16									len;
	UInt16									i;
	LocalID									dbID;
	Err											error;
	MemHandle								recH;
	Boolean									alreadyThere;

	ErrFatalDisplayIf( !mainDB, "invalid param" );

	newDBs = 0;
	appendLen = StrLen( appFileCreatorAsString );

	DateSecondsToDate( TimGetSeconds(), &today );

	error = DmGetNextDatabaseByTypeCreator( true, &state, kPrjtToDoDBType, appFileCreator, false /*see palmos sdk 4.0 reference*/, &cardNo, &dbID );
	while( !error && dbID )
	{
		error = DmDatabaseInfo( cardNo, dbID, szDBName, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL );
		if( !error )
		{
			len = StrLen( szDBName );
			len -= appendLen;
			szDBName[len] = '\0';

			alreadyThere = false;
			// we must perform a stupid sequential search 
			// as the main db may be sorted by hand :(
			numRecs = DmNumRecords( mainDB );
			for( i = 0; i < numRecs; i++ )
			{
				recH = DmQueryRecord( mainDB, i );
				if( recH )
				{
					recP = MemHandleLock( recH );
					alreadyThere = (StrCompare( &recP->name , szDBName ) == 0);
					MemHandleUnlock( recH );
					if( alreadyThere )
						break;
				}
			}

			if( !alreadyThere )
			{
				db = DmOpenDatabase( cardNo, dbID, dmModeReadOnly );
				if( db )
				{
					PrjtDBCountToDos( db, &project.numOfAllToDos, &project.numOfFinishedToDos );
					DmCloseDatabase( db );

					project.begin = today;
					*((UInt16 *)(&project.end)) = kNoDate;
					project.priority = kProjectsPriority1;
					project.status = kProjectsState1;
					project.name = szDBName;
					project.note = NULL; //"hello world ;-)";
					recIndex = PrjtDBCreateProjectRecord( mainDB, &project, dmUnfiledCategory, sortOrder );
					
					if( recIndex == kNoRecordIndex )
						break;

					numRecs++;
					newDBs++;
				}
			}
		}

		error = DmGetNextDatabaseByTypeCreator( false, &state, kPrjtToDoDBType, appFileCreator, false /*see palmos sdk 4.0 reference*/, &cardNo, &dbID );
	}

	return (newDBs);
}

/*
 * FUNCTION:				PrjtDBCountToDos
 * DESCRIPTION:			this routine iterates over a todo database and
 * 									counts the number of todos
 * PARAMETERS:			db - reference to an open todo database
 * 									[out] allToDos, [out] finishedToDos - will recieve the values
 * RETURNS:					nothing
 */
void PrjtDBCountToDos( DmOpenRef todoDB, UInt16 * allToDos, UInt16 * finishedToDos )
{
	PrjtToDoType * 	recP;
	MemHandle				recH;
	UInt16					all;
	UInt16					finished;

	ErrFatalDisplayIf( !todoDB || !allToDos || !finishedToDos, "invalid param" );

	all = 0;
	finished = 0;
	while( true )
	{
		recH = DmQueryRecord( todoDB, all );
		if( !recH )
			break;
		
		recP = MemHandleLock( recH );
		if( recP->priority & kToDosCompleteFlag )
			finished++;
		all++;

		MemHandleUnlock( recH );
	}

	*allToDos = all;
	*finishedToDos = finished;
}

/*
 * FUNCTION:					PrjtDBToDoResortRecord
 * DESCRIPTION:				this routine detachtes the record specified by db and recIndex
 *										and puts it on its right position with the database
 * PARAMETERS:				db - the database
 *										recIndex - holds the record index, upon return holds the new record index
 *										sortOrder - sort order of the database (db)
 * RETURNS:						true if the index has changed, else false
 */
Boolean PrjtDBToDoResortRecord( DmOpenRef db, UInt16 * recIndex, UInt8 sortOrder )
{
	SortRecordInfoType sortInfo;
	MemHandle recH;
	UInt16		attr;
	UInt16		index, tmp;

	ErrFatalDisplayIf( !recIndex, "invalid param" );
	ErrFatalDisplayIf( sortOrder == kSortToDosManually, "bad sort order" );

	tmp = index = *recIndex;

	DmRecordInfo( db, index, &attr, NULL, NULL );
	if( !DmDetachRecord( db, index, &recH ) )
	{
		sortInfo.attributes = attr;
		index = DmFindSortPosition( db, MemHandleLock( recH ), &sortInfo, (DmComparF *)PrjtDBCompareToDoRecords, sortOrder );
		MemHandleUnlock( recH );
		DmAttachRecord( db, &index, recH, NULL );
		DmSetRecordInfo( db, index, &attr, NULL );
		*recIndex = index;
	}

	return (tmp != index);
}


/*
 * FUNCTION:				PrjtDBCompareToDoRecords
 * DESCRIPTION:			this routine compares two todo records
 * PARAMETERS:			see palm sdk
 * RETURNS:					0 if equal
 *									< 0 if r1 < r2
 *									> 0 if r2 > r2
 */
Int16 PrjtDBCompareToDoRecords( PrjtToDoType *rec1, PrjtToDoType *rec2, 
		Int16 sortOrder, SortRecordInfoType *sort1, SortRecordInfoType *sort2, MemHandle appInfoH )
{
	Int16		ret_val;

	ErrFatalDisplayIf( !sort1 || !sort2, "invalid param" );

PrjtDBCompareToDoRecords_Start:

	switch( sortOrder )
	{
		case kSortToDosByPriorityDueDate:
			ret_val = (rec1->priority & kToDosPriorityMask) - (rec2->priority & kToDosPriorityMask);
			if( !ret_val )
				ret_val = PrjtDBCompareDateType( rec1->dueDate, rec2->dueDate );
			break;

		case kSortToDosByDueDatePriority:
			ret_val = PrjtDBCompareDateType( rec1->dueDate, rec2->dueDate );
			if( !ret_val )
				ret_val = (Int16)((Int32)DateToDays( rec1->dueDate ) - (Int32)DateToDays( rec2->dueDate ));
			break;

		case kSortToDosManually:
			ErrDisplay( "bad sort order" );
			ret_val = 0;
			break;

		case kSortToDosByCategoryPriority:
			ret_val = PrjtDBCompareCategory( sort1->attributes, sort2->attributes, appInfoH );
			if( !ret_val )
			{
				sortOrder = kSortToDosByPriorityDueDate;
				goto PrjtDBCompareToDoRecords_Start;
			}
			break;

		case kSortToDosByCategoryDueDate:
			ret_val = PrjtDBCompareCategory( sort1->attributes, sort2->attributes, appInfoH );
			if( !ret_val )
			{
				sortOrder = kSortToDosByDueDatePriority;
				goto PrjtDBCompareToDoRecords_Start;
			}
			break;

		default:
			ErrDisplay( "bad sort order" );
			ret_val = 0;
			break;
	}

	return (ret_val);
}
