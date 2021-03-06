#include <PalmOS.h>
#include "DateDB.h"

/*
 * this is a modified version of the original module provided
 * by Palm Inc. with their SDK 4.0
 *
 * we concentrate here only on inserting untimed records into 
 * the datebook database
 *
 * 25.dec.01 - pete.
 */


/***********************************************************************
 *
 *	Internal Structutes
 *
 ***********************************************************************/

// The following structure doesn't really exist.  The first field
// varies depending on the data present.  However, it is convient
// (and less error prone) to use when accessing the other information.
typedef struct {
	ApptDateTimeType 	when;
	ApptDBRecordFlags	flags;	// A flag set for each  datum present
	char				firstField;
	UInt8				reserved;
} ApptPackedDBRecordType;

typedef ApptPackedDBRecordType * ApptPackedDBRecordPtr;



/***********************************************************************
 *
 *	Internal Routines
 *
 ***********************************************************************/

static Int16 		TimeCompare (TimeType t1, TimeType t2) THIRD_SECTION;

static Int16 		DateCompare (DateType d1, DateType d2) THIRD_SECTION;

static UInt16 	ApptPackedSize (ApptDBRecordPtr r) THIRD_SECTION;

static void 		ApptPack(ApptDBRecordPtr s, ApptPackedDBRecordPtr d) THIRD_SECTION;

static UInt16 	ApptFindSortPosition(DmOpenRef dbP, ApptPackedDBRecordPtr newRecord) THIRD_SECTION;

static Int16 		ApptComparePackedRecords (ApptPackedDBRecordPtr r1, ApptPackedDBRecordPtr r2, 
	 												Int16 extra, SortRecordInfoPtr info1, SortRecordInfoPtr info2, 
													MemHandle appInfoH) THIRD_SECTION;

static Err			ApptAppInfoInit(DmOpenRef dbP) THIRD_SECTION;

static void 		SetDBBackupBit(DmOpenRef dbP) THIRD_SECTION;



/***********************************************************************
 *
 * FUNCTION:     SetDBBackupBit
 *
 * DESCRIPTION:  This routine sets the backup bit on the given database.
 *					  This is to aid syncs with non Palm software.
 *					  If no DB is given, open the app's default database and set
 *					  the backup bit on it.
 *
 * PARAMETERS:   dbP -	the database to set backup bit,
 *								can be NULL to indicate app's default database
 *
 * RETURNED:     nothing
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			grant	4/1/99	Initial Revision
 *
 ***********************************************************************/
static void SetDBBackupBit(DmOpenRef dbP)
{
	DmOpenRef localDBP;
	LocalID dbID;
	UInt16 cardNo;
	UInt16 attributes;

	// Open database if necessary. If it doesn't exist, simply exit (don't create it).
	if (dbP == NULL)
		{
		localDBP = DmOpenDatabaseByTypeCreator (datebookDBType, sysFileCDatebook, dmModeReadWrite);
		if (localDBP == NULL)  return;
		}
	else
		{
		localDBP = dbP;
		}
	
	// now set the backup bit on localDBP
	DmOpenDatabaseInfo(localDBP, &dbID, NULL, NULL, &cardNo, NULL);
	DmDatabaseInfo(cardNo, dbID, NULL, &attributes, NULL, NULL,
				NULL, NULL, NULL, NULL, NULL, NULL, NULL);
	attributes |= dmHdrAttrBackup;
	DmSetDatabaseInfo(cardNo, dbID, NULL, &attributes, NULL, NULL,
				NULL, NULL, NULL, NULL, NULL, NULL, NULL);
	
	// close database if necessary
   if (dbP == NULL) 
   	{
   	DmCloseDatabase(localDBP);
      }
}



/************************************************************
 *
 *  FUNCTION: ApptAppInfoInit
 *
 *  DESCRIPTION: Create and initialize the app info chunk if missing.
 *
 *  PARAMETERS: database pointer
 *
 *  RETURNS: 0 if successful, errorcode if not
 *
 *  CREATED: 1/25/95 
 *
 *  BY: Roger Flores
 *
 *************************************************************/
static Err	ApptAppInfoInit(DmOpenRef dbP)
{
	UInt16 				cardNo;
	MemHandle 		h;
	LocalID 			dbID;
	LocalID 			appInfoID;
	ApptAppInfoPtr	appInfoP;
	
	if (DmOpenDatabaseInfo(dbP, &dbID, NULL, NULL, &cardNo, NULL))
		return dmErrInvalidParam;
		
	if (DmDatabaseInfo(cardNo, dbID, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &appInfoID, NULL, NULL, NULL))
		return dmErrInvalidParam;
	
	if (appInfoID == 0) 
		{
		h = DmNewHandle(dbP, sizeof(ApptAppInfoType));
		if (! h) return dmErrMemError;

		appInfoID = MemHandleToLocalID (h);
		DmSetDatabaseInfo(cardNo, dbID, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &appInfoID, NULL, NULL, NULL);
		}
		
	// Get pointer to app Info chunk
	appInfoP = MemLocalIDToLockedPtr(appInfoID, cardNo);
	
	// Init it
	DmSet(appInfoP, 0, sizeof(ApptAppInfoType), 0); 

	// Unlock it
	MemPtrUnlock(appInfoP);
	
	return 0;
}



/***********************************************************************
 *
 * FUNCTION:     DateGetDatabase
 *
 * DESCRIPTION:  Get the application's database.  Open the database if it
 * exists, create it if neccessary.
 *
 * PARAMETERS:   *dbPP - pointer to a database ref (DmOpenRef) to be set
 *					  mode - how to open the database (dmModeReadWrite)
 *
 * RETURNED:     Err - zero if no error, else the error
 *
 * REVISION HISTORY:
 *			Name		Date		Description
 *			----		----		-----------
 *			roger		12/3/97	Initial Revision
 *
 ***********************************************************************/
Err DatebookGetDatabase (DmOpenRef *dbPP, UInt16 mode)
{
	Err error = 0;
	DmOpenRef dbP;
	UInt16 cardNo;
	LocalID dbID;
	
	
	*dbPP = 0;
	dbP = DmOpenDatabaseByTypeCreator(datebookDBType, sysFileCDatebook, mode);
	if (! dbP)
		{
		error = DmCreateDatabase (0, datebookDBName, sysFileCDatebook,
								datebookDBType, false);
		if (error) return error;
		
		dbP = DmOpenDatabaseByTypeCreator(datebookDBType, sysFileCDatebook, mode);
		if (! dbP) return ~0;

		// Set the backup bit.  This is to aid syncs with non Palm software.
		SetDBBackupBit(dbP);
		
		error = ApptAppInfoInit (dbP);
      if (error) 
      	{
			DmOpenDatabaseInfo(dbP, &dbID, NULL, NULL, &cardNo, NULL);
      	DmCloseDatabase(dbP);
      	DmDeleteDatabase(cardNo, dbID);
         return error;
         }
		}
	
	*dbPP = dbP;
	return 0;
}



/***********************************************************************
 *
 * FUNCTION:    DateCompare
 *
 * DESCRIPTION: This routine compares two dates.
 *
 * PARAMETERS:  d1 - a date 
 *              d2 - a date 
 *
 * RETURNED:    if d1 > d2  returns a positive int
 *              if d1 < d2  returns a negative int
 *              if d1 = d2  returns zero
 *
 * NOTE: This routine treats the DateType structure like an unsigned int,
 *       it depends on the fact the the members of the structure are ordered
 *       year, month, day form high bit to low low bit.
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	6/12/95		Initial Revision
 *
 ***********************************************************************/
static Int16 DateCompare (DateType d1, DateType d2)
{
	UInt16 int1, int2;
	
	int1 = DateToInt(d1);
	int2 = DateToInt(d2);
	
	if (int1 > int2)
		return (1);
	else if (int1 < int2)
		return (-1);
	return 0;
}



/***********************************************************************
 *
 * FUNCTION:    TimeCompare
 *
 * DESCRIPTION: This routine compares two times.  "No time" is represented
 *              by minus one, and is considered less than all times.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    if t1 > t2  returns a positive int
 *              if t1 < t2  returns a negative int
 *              if t1 = t2  returns zero
 *
 * REVISION HISTORY:
 *			Name	Date		Description
 *			----	----		-----------
 *			art	6/12/95		Initial Revision
 *
 ***********************************************************************/
static Int16 TimeCompare (TimeType t1, TimeType t2)
{
	Int16 int1, int2;
	
	int1 = TimeToInt(t1);
	int2 = TimeToInt(t2);
	
	if (int1 > int2)
		return (1);
	else if (int1 < int2)
		return (-1);
	return 0;

}



/************************************************************
 *
 *  FUNCTION: ApptPackedSize
 *
 *  DESCRIPTION: Return the packed size of an ApptDBRecordType 
 *
 *  PARAMETERS: database record
 *
 *  RETURNS: the size in bytes
 *
 *  CREATED: 1/25/95 
 *
 *  BY: Roger Flores
 *
 *************************************************************/
static UInt16 ApptPackedSize (ApptDBRecordPtr r)
{
	UInt16 size;

	
	size = sizeof (ApptDateTimeType) + sizeof (ApptDBRecordFlags);
	
	if (r->alarm != NULL)
		size += sizeof (AlarmInfoType);
	
	if (r->repeat != NULL)
		size += sizeof (RepeatInfoType);
	
	if (r->exceptions != NULL)
		size += sizeof (UInt16) + 
			(r->exceptions->numExceptions * sizeof (DateType));
	
	if (r->description != NULL)
		size += StrLen(r->description) + 1;
	
	if (r->note != NULL)
		size += StrLen(r->note) + 1;
	

	return size;
}



/************************************************************
 *
 *  FUNCTION: ApptPack
 *
 *  DESCRIPTION: Pack an ApptDBRecordType
 *
 *  PARAMETERS: database record
 *
 *  RETURNS: the ApptPackedDBRecord is packed
 *
 *  CREATED: 1/25/95 
 *
 *  BY: Roger Flores
 *
 *************************************************************/
static void ApptPack(ApptDBRecordPtr s, ApptPackedDBRecordPtr d)
{
	ApptDBRecordFlags	flags;
	UInt16 	size;
	UInt32	offset = 0;
	
	
	*(UInt8 *)&flags = 0;			// clear the flags
	
	
	// copy the ApptDateTimeType
	//c = (char *) d;
	offset = 0;
	DmWrite(d, offset, s->when, sizeof(ApptDateTimeType));
	offset += sizeof (ApptDateTimeType) + sizeof (ApptDBRecordFlags);
	

	if (s->alarm != NULL)
		{
		DmWrite(d, offset, s->alarm, sizeof(AlarmInfoType));
		offset += sizeof (AlarmInfoType);
		flags.alarm = 1;
		}
	

	if (s->repeat != NULL)
		{
		DmWrite(d, offset, s->repeat, sizeof(RepeatInfoType));
		offset += sizeof (RepeatInfoType);
		flags.repeat = 1;
		}
	

	if (s->exceptions != NULL)
		{
		size = sizeof (UInt16) + 
			(s->exceptions->numExceptions * sizeof (DateType));
		DmWrite(d, offset, s->exceptions, size);
		offset += size;
		flags.exceptions = 1;
		}
	
	
	if (s->description != NULL)
		{
		size = StrLen(s->description) + 1;
		DmWrite(d, offset, s->description, size);
		offset += size;
		flags.description = 1;
		}
	

	
	if (s->note != NULL)
		{
		size = StrLen(s->note) + 1;
		DmWrite(d, offset, s->note, size);
		offset += size;
		flags.note = 1;
		}
	
	DmWrite(d, sizeof(ApptDateTimeType), &flags, sizeof(flags));
}



/************************************************************
 *
 *  FUNCTION: ApptFindSortPosition
 *
 *  DESCRIPTION: Return where a record is or should be
 *		Useful to find or find where to insert a record.
 *
 *  PARAMETERS: database record
 *
 *  RETURNS: position where a record should be
 *
 *  CREATED: 1/25/95 
 *
 *  BY: Roger Flores
 *
 *************************************************************/
static UInt16 ApptFindSortPosition(DmOpenRef dbP, ApptPackedDBRecordPtr newRecord)
{
	return (DmFindSortPosition (dbP, newRecord, NULL, (DmComparF *)ApptComparePackedRecords, 0));
}



/************************************************************
 *
 *  FUNCTION:    ApptComparePackedRecords
 *
 *  DESCRIPTION: Compare two packed records.
 *
 *  PARAMETERS:  r1    - database record 1
 *				     r2    - database record 2
 *               extra - extra data, not used in the function
 *
 *  RETURNS:    -1 if record one is less
 *		           1 if record two is less
 *
 *  CREATED: 1/14/95 
 *
 *  BY: Roger Flores
 *
 *	COMMENTS:	Compare the two records key by key until
 *	there is a difference.  Return -1 if r1 is less or 1 if r2
 *	is less.  A zero is never returned because if two records
 *	seem identical then their unique IDs are compared!
 *
 *************************************************************/ 
static Int16 ApptComparePackedRecords (ApptPackedDBRecordPtr r1, 
	ApptPackedDBRecordPtr r2, Int16 extra, SortRecordInfoPtr info1, 
	SortRecordInfoPtr info2, MemHandle appInfoH)
{
	Int16 result;

	if ((r1->flags.repeat) || (r2->flags.repeat))
		{
		if ((r1->flags.repeat) && (r2->flags.repeat))
			result = 0;
		else if (r1->flags.repeat)
			result = -1;
		else
			result = 1;
		}

	else
		{
		result = DateCompare (r1->when.date, r2->when.date);
		if (result == 0)
			{
			result = TimeCompare (r1->when.startTime, r2->when.startTime);
			}
		}
	return result;
}



/************************************************************
 *
 *  FUNCTION: ApptNewRecord
 *
 *  DESCRIPTION: Create a new packed record in sorted position
 *
 *  PARAMETERS: database pointer
 *				database record
 *
 *  RETURNS: ##0 if successful, error code if not
 *
 *  CREATED: 1/25/95 
 *
 *  BY: Roger Flores
 *
 *************************************************************/
Err ApptNewRecord(DmOpenRef dbP, ApptDBRecordPtr r, UInt16 *index)
{
	MemHandle recordH;
	ApptPackedDBRecordPtr recordP;
	UInt16 					newIndex;
	Err err;

	
	// Make a new chunk with the correct size.
	recordH = DmNewHandle (dbP, (UInt32) ApptPackedSize(r));
	if (recordH == NULL)
		return dmErrMemError;

	recordP = MemHandleLock (recordH);
	
	// Copy the data from the unpacked record to the packed one.
	ApptPack (r, recordP);

	newIndex = ApptFindSortPosition(dbP, recordP);

	MemPtrUnlock (recordP);

	// 4) attach in place
	err = DmAttachRecord(dbP, &newIndex, recordH, 0);
	if (err) 
		MemHandleFree(recordH);
	else
		*index = newIndex;
	
	return err;
}
