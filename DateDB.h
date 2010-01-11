/*
 * DateDB.h
 * "borrowed" and modified from the palm sdk 
 */

#ifndef __TDAPPTMGR_H__
#define __TDAPPTMGR_H__

#include "sections.h"

#define datebookDBType						'DATA'
#define datebookDBName						"DatebookDB"

typedef struct {
   UInt16	renamedCategories;	// bitfield of categories with a different name
	char 	categoryLabels[dmRecNumCategories][dmCategoryLength];
	UInt8	categoryUniqIDs[dmRecNumCategories];
	UInt8	lastUniqID;	// Uniq IDs generated by the device are between
							// 0 - 127.  Those from the PC are 128 - 255.
	UInt8	reserved1;	// from the compiler word aligning things
	UInt16	reserved2;
	UInt8	startOfWeek;
	UInt8	reserved;
} ApptAppInfoType;


typedef ApptAppInfoType * ApptAppInfoPtr;


/************************************************************
 *
 * Appointment Database constants.
 *
 *************************************************************/
#define apptMaxPerDay 		100		// max appointments displayable on a day.
#define apptNoTime			-1			// start time of an untimed appt.
#define apptNoEndDate		0xffff	// end date of appts that repeat forever
#define apptNoAlarm			-1
#define apptMaxEndTime		0x1737	// 11:55 pm, hours in high byte, minutes in low byte
#define apptDawnOfTime		0
#define apptEndOfTime		0xffffffff

#define apptMaxDisplayableAlarms 10


/************************************************************
 *
 * Appointment Database structures.
 *
 *************************************************************/

// The format of a packed appointment record is as follows:
//
//	ApptDateTimeType 		when;
//	ApptDBRecordFlags		flags;			// A flag set for each datum present
//	AlarmInfoType			alarm;			// optional
//	RepeatInfoType			repeat;			// optional
//	ExceptionsListType	exceptions;		// optional
//	char []					note;				// null terminated, optional
//	char []					description;	// null terminated
//
// All optional data may or may not appear.



// Alarm advance - the period of time before the appointment that the 
// alarm should sound.
//
typedef enum alarmTypes {aauMinutes, aauHours, aauDays} AlarmUnitType;

typedef struct {
	Int8					advance;			// Alarm advance (-1 = no alarm)
	AlarmUnitType		advanceUnit;	// minutes, hours, days
} AlarmInfoType;


// The following enum is used to specify the frequency of
// repeating appointments.
//
enum repeatTypes {
	repeatNone, 
	repeatDaily, 
	repeatWeekly, 
	repeatMonthlyByDay, 
	repeatMonthlyByDate,
	repeatYearly
};
typedef enum repeatTypes RepeatType;


// This structure contains information about repeat appointments.  The 
// repeatOn member is only used by weelky and monthly-by-day repeating
// appointments.  For weekly the byte is a bit field that contains the 
// days of the week the appointments occurs on (bit: 0-sun, 1-mon, 
// 2-tue, etc.).  For monthly-by-day the byte contains the day the 
// appointments occurs, (ex: the 3rd friday), the byte is of type 
// DayOfMonthType.
//
typedef struct {
	RepeatType		repeatType;			// daily, weekly, monthlyByDay, etc.
	UInt8				reserved1;
	DateType			repeatEndDate;		// minus one if forever
	UInt8	repeatFrequency;	// i.e. every 2 days if repeatType daily
	UInt8	repeatOn;			// monthlyByDay and repeatWeekly only	
	UInt8  repeatStartOfWeek;// repeatWeekly only
	UInt8				reserved2;
} RepeatInfoType;

typedef RepeatInfoType * RepeatInfoPtr;


// This checks a weekly appointment's repeat info and returns true if the
// appointment occurs on only one day per week.
// The form (x & (x - 1)) masks out the lowest order bit in x.  (K&R, p. 51)
// If only one bit is set, which occurs iff the appointment is only
// once per week, then (x & (x - 1)) == 0.
#define OnlyRepeatsOncePerWeek(R)  (((R).repeatOn & ((R).repeatOn - 1)) == 0)


// RepeatOnDOW - true if repeat info R repeats on day of week D
#define RepeatOnDOW(R, D)   ((1 << (D)) & ((R)->repeatOn))


// This structure contains information about the date and time of an 
// appointment.
//
typedef struct {
	TimeType			startTime;			// Time the appointment starts
	TimeType			endTime;				// Time the appointment ends
	DateType			date;					// date of appointment
} ApptDateTimeType;



// This is the structure for a repeating appointment's exceptions list.  The
// exceptions list is a variable length list of dates that the repeating appointment
// should not appear on.  The list is in chronological order.
//
typedef struct {
	UInt16    	numExceptions;
	DateType	exception;
} ExceptionsListType;

typedef ExceptionsListType * ExceptionsListPtr;



// This structure describes what information is present in an
// AppointmentPackedDBRecordType
typedef struct {
	unsigned when			:1;			// set if when info changed (ApptChangeRecord)
	unsigned alarm			:1;			// set if record contains alarm info
	unsigned repeat		:1;			// set if record contains repeat info
	unsigned note			:1;			// set if record contains a note
	unsigned exceptions	:1;			// set if record contains exceptions list
	unsigned description	:1;			
} ApptDBRecordFlags;


// ApptDBRecordType
//
// This is the record used by the application.  All pointers are either NULL 
// or point to data within the PackedDB record.  All strings are null 
// character terminated.

typedef struct {
	ApptDateTimeType *	when;
	AlarmInfoType *		alarm;
	RepeatInfoType *		repeat;
	ExceptionsListType *	exceptions;
	Char *					description;
	Char *					note;
} ApptDBRecordType;

typedef ApptDBRecordType * ApptDBRecordPtr;


// ApptGetAppointments returns an array of the following structures.
typedef struct {
	TimeType		startTime;
	TimeType		endTime;
	UInt16			recordNum;
	} ApptInfoType;
typedef ApptInfoType * ApptInfoPtr;



/************************************************************
 *
 * Appointment database routines.
 *
 *************************************************************/

#ifdef __cplusplus
extern "C" {
#endif


Err 		ApptNewRecord (DmOpenRef dbP, ApptDBRecordPtr r, UInt16 *index) THIRD_SECTION;

Err 		DatebookGetDatabase (DmOpenRef *dbPP, UInt16 mode) THIRD_SECTION;


#ifdef __cplusplus
}
#endif

#endif
