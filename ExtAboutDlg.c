#include <PalmOS.h>
#include "ProjectsDB.h"
#include "Projects.h"
#include "ProjectsRcp.h"

/*
 * THIS CODE WAS IMPLEMENTED LIKE IN THE SAMPLE APP "CUBE 3D" BY ARON ARDIRI.
 * THANKS FOR THE SAMPLE :-)
 */

#ifndef __PROJECTS_CONFIG_H__
	#error "include config.h"
#endif /* __PROJECTS_CONFIG_H__ */

#ifdef CONFIG_EXT_ABOUT

#ifdef CONFIG_SONY
	#include <SonyCLIE.h>
#endif /* CONFIG_SONY */

#ifdef CONFIG_HANDERA
	#include "Vga.h"
	#include "HndrFuncs.h"
#endif /* CONFIG_HANDERA */

#define kExtAboutBasicPageHeight			230
#define kExtAboutLeftOffset						5
#define kExtAboutOneFlagLineHeight		
#define kExtAboutFlagFont							boldFont
#define kExtAboutFlagHRFont						hrBoldFont

#define kExtAboutPageWidth						142
#define kExtAboutPageVisibleHeight		116
#define kExtAboutPageTopLeftX					3
#define kExtAboutPageTopLeftY					18

// declaratios ------------------------------------------------------------------------------------

static UInt16	ExtAboutInit( FormType * frmP, WinHandle * winH ) SECOND_SECTION;
static void		ExtAboutDraw( WinHandle winH, UInt16 position ) SECOND_SECTION;
static void 	ExtAboutScrollPage( WinHandle winH, WinDirectionType direction, Boolean total ) SECOND_SECTION;

// implementations --------------------------------------------------------------------------------

/*
 * FUNCTION:					ExtAboutScrollPage
 * DESCRIPTION:				this routine scrolls the offscreen image by a page
 * PARAMETERS:				winH - offscreen image
 * 										direction - how to scroll
 * 										total - if true the page will be scrolled immediately to the top/bottom
 * RETURNS:						nothing
 */
static void ExtAboutScrollPage( WinHandle winH, WinDirectionType direction, Boolean total )
{
	UInt16 value, min, max, page;
	FormType * frmP = FrmGetActiveForm();
	ScrollBarType * sclP = FrmGetObjectPtr( frmP, FrmGetObjectIndex( frmP, ExtAboutScrollBar ) );

	ErrFatalDisplayIf( !winH, "invalid param" );

	SclGetScrollBar( sclP, &value, &min, &max, &page );

	if( direction == winDown )
	{
		if( value == max )
			return;

		if( !total && (value + page) < max )
			value += page;
		else
			value = max;
	}
	else
	{
		if( value == min )
			return;

		if( !total && (value - page) > min )
			value -= page;
		else
			value = min;
	}

	SclSetScrollBar( sclP, value, min, max, page );
	SclDrawScrollBar( sclP );
	ExtAboutDraw( winH, value );
}

/*
 * FUNCTION:					ExtAboutDraw
 * DESCRIPTION:				this function draws the winH to the current draw window
 * PARAMETERS:				winH - win to be drawn
 * 										position - y offset
 * RETURNS:						nothing
 */
static void	ExtAboutDraw( WinHandle winH, UInt16 position )
{
	RectangleType	r;

	ErrFatalDisplayIf( !winH, "invalid param" );

	r.topLeft.x = 0;
	r.topLeft.y = position;
	r.extent.x = kExtAboutPageWidth;
	r.extent.y = kExtAboutPageVisibleHeight;

#ifdef CONFIG_SONY
	if( gGlobalPrefs.sonyLibRefNum )
	{
		r.topLeft.y <<= 1;
		r.extent.x <<= 1;
		r.extent.y <<= 1;
		HRWinCopyRectangle( gGlobalPrefs.sonyLibRefNum, winH, WinGetDrawWindow(), &r, kExtAboutPageTopLeftX<<1, kExtAboutPageTopLeftY<<1, winPaint );
	}
	else
#endif /* CONFIG_SONY */
		WinCopyRectangle( winH, WinGetDrawWindow(), &r, kExtAboutPageTopLeftX, kExtAboutPageTopLeftY, winPaint );
}

/*
 * FUNCTION:					ExtAboutInit
 * DESCRIPTION:				this routine initializes the form and creates the offscreen image
 * PARAMETERS:				frmP - pointer to the ExtAboutDialog
 * 										winH - to be returned
 * RETURNS:						the height of the offscreen image
 */
static UInt16 ExtAboutInit( FormType * frmP, WinHandle * winH )
{
	Char						szDateTime[32];
	const CustomPatternType	erase = {0,0,0,0,0,0,0,0};
	RectangleType		r;
	Char *					strP;
	ScrollBarType *	sclP;
	BitmapType *		bmpP;
	WinHandle				curWinH;
	FontID					curFont = stdFont;
#ifdef CONFIG_SONY
	HRFontID				hrCurFont = hrStdFont;
#endif /* CONFIG_SONY */
	UInt16					y;
	UInt16					len;
	UInt16					lineHeight;
	UInt16					i;
	UInt16					objIDs[10] = { ExtAboutAuthor, 
																ExtAboutEmail, 
																ExtAboutWWW, 
																ExtAboutTranslatedBy,
																ExtAboutTranslator,
																ExtAboutTranslatorEmail,
																ExtAboutLicence1, 
																ExtAboutLicence2, 
																ExtAboutBuildTitle1, 
																ExtAboutFollowingFlagsString };
	MemHandle				memH;
	Err							error;

	r.topLeft.x = r.topLeft.y = 0;
	r.extent.x = kExtAboutPageWidth;
	r.extent.y = kExtAboutBasicPageHeight;

	FntSetFont( kExtAboutFlagFont);
	lineHeight = FntLineHeight();

	// find out how long our page will be
#ifdef CONFIG_COLOR
	r.extent.y += lineHeight;
#endif /* CONFIG_COLOR */
#ifdef CONFIG_OS_BELOW_35
	r.extent.y += lineHeight;
#endif /* CONFIG_OS_BELOW_35 */
#ifdef CONFIG_EXT_ABOUT
	r.extent.y += lineHeight;
#endif /* CONFIG_EXT_ABOUT */
#ifdef CONFIG_HLP_STRS
	r.extent.y += lineHeight;
#endif /* CONFIG_HLP_STRS */
#ifdef CONFIG_ADDITIONAL
	r.extent.y += lineHeight;
#endif /* CONFIG_ADDITIONAL */
#ifdef CONFIG_ALLTODOS_DLG
	r.extent.y += lineHeight;
#endif /* CONFIG_ALLTODOS_DLG */
#ifdef CONFIG_DEF_CATEGORY
	r.extent.y += lineHeight;
#endif /* CONFIG_DEF_CATEGORY */
#ifdef CONFIG_SONY
	r.extent.y += lineHeight;
#endif /* CONFIG_SONY */
#ifdef CONFIG_HANDERA
	r.extent.y += lineHeight;
#endif /* CONFIG_HANDERA */
#ifdef CONFIG_JOGDIAL
	r.extent.y += lineHeight;
#endif /* CONFIG_JOGDIAL */
	
	// create the offset screen
#ifdef CONFIG_SONY
	if( gGlobalPrefs.sonyLibRefNum )
	{
		r.extent.x <<= 1;
		r.extent.y <<= 1;
		*winH = HRWinCreateOffscreenWindow( gGlobalPrefs.sonyLibRefNum, r.extent.x, r.extent.y, genericFormat, &error );
	}
	else
#endif /* CONFIG_SONY */
		*winH = WinCreateOffscreenWindow( r.extent.x, r.extent.y, genericFormat, &error );

	if( error != errNone )
		return (0);
	
	// save the current draw state
	curWinH = WinGetDrawWindow();
#ifdef CONFIG_SONY
	if( gGlobalPrefs.sonyLibRefNum )
		hrCurFont = HRFntGetFont( gGlobalPrefs.sonyLibRefNum );
	else
#endif /* CONFIG_SONY */
		curFont = FntGetFont();

	// set the new drawing area and clear it
	WinSetDrawWindow( *winH );
	WinSetPattern( &erase );
#ifdef CONFIG_SONY
	if( gGlobalPrefs.sonyLibRefNum )
		HRWinFillRectangle( gGlobalPrefs.sonyLibRefNum, &r, 0 );
	else
#endif /* CONFIG_SONY */
		WinFillRectangle( &r, 0 );

	// allocated addition memory
	y = 2;

	// create the picture (do the drawings)
#ifdef CONFIG_SONY
	if( gGlobalPrefs.sonyLibRefNum )
		memH = DmGetResource( bitmapRsc, AppIconBitmap64x64 );
	else
#endif /* CONFIG_SONY */
		memH = DmGetResource( bitmapRsc, AppIconBitmap );
	ErrFatalDisplayIf( !memH, "resource not found" );
	bmpP = MemHandleLock( memH );

#ifdef CONFIG_SONY
	if( gGlobalPrefs.sonyLibRefNum )
		HRWinDrawBitmap( gGlobalPrefs.sonyLibRefNum, bmpP, (kExtAboutPageWidth<<1) - (kExtAboutLeftOffset + 64), y );
	else
#endif /* CONFIG_SONY */
		WinDrawBitmap( bmpP, kExtAboutPageWidth - (kExtAboutLeftOffset + 32), y );
	y += 4;

	MemHandleUnlock( memH );
	DmReleaseResource( memH );

	// our application version with name ------------------------------------------------------------
#ifdef CONFIG_SONY
	if( gGlobalPrefs.sonyLibRefNum )
		HRFntSetFont( gGlobalPrefs.sonyLibRefNum, hrBoldFont );
	else
#endif /* CONFIG_SONY */
		FntSetFont( boldFont );
	memH = DmGetResource( strRsc, ExtAboutTitle );
	ErrFatalDisplayIf( !memH, "resource not found" );
	strP = MemHandleLock( memH );
	len = StrLen( strP );

#ifdef CONFIG_SONY
	if( gGlobalPrefs.sonyLibRefNum )
		HRWinDrawChars( gGlobalPrefs.sonyLibRefNum, strP, len, kExtAboutLeftOffset, y );
	else
#endif /* CONFIG_SONY */
		WinDrawChars( strP, len, kExtAboutLeftOffset, y );

	MemHandleUnlock( memH );
	DmReleaseResource( memH );
	y += FntLineHeight();
	y += FntLineHeight()>>1;

#ifdef CONFIG_SONY
	if( gGlobalPrefs.sonyLibRefNum )
		HRFntSetFont( gGlobalPrefs.sonyLibRefNum, hrStdFont );
	else
#endif /* CONFIG_SONY */
		FntSetFont( stdFont );
	lineHeight = FntLineHeight();
	for( i = 0; i < 10; i++ )
	{
		memH = DmGetResource( strRsc, objIDs[i] );
		ErrFatalDisplayIf( !memH, "resource not found" );
		strP = MemHandleLock( memH );
		len = StrLen( strP );

#ifdef CONFIG_SONY
		if( gGlobalPrefs.sonyLibRefNum )
			HRWinDrawChars( gGlobalPrefs.sonyLibRefNum, strP, len, kExtAboutLeftOffset, y );
		else
#endif /* CONFIG_SONY */
			WinDrawChars( strP, len, kExtAboutLeftOffset, y );
		MemHandleUnlock( memH );
		DmReleaseResource( memH );
		y += lineHeight;

		if( i == 1 || i == 2 || i == 5 || i == 7 || i == 9 )
		{
			y += lineHeight>>1;
			if( i == 7 )
			{
				// draw separator
				WinDrawLine( r.topLeft.x + 5, y, r.topLeft.x + r.extent.x - 10, y );
				y += (lineHeight>>1);
			}
		}
	}

// flags there were defined -----------------------------------------------------------------------

#ifdef CONFIG_SONY
	if( gGlobalPrefs.sonyLibRefNum )
		HRFntSetFont( gGlobalPrefs.sonyLibRefNum, kExtAboutFlagHRFont );
	else
#endif /* CONFIG_SONY */
		FntSetFont( kExtAboutFlagFont);
	lineHeight = FntLineHeight();

	// we dot not need to get the line height again
	// because they are equal on the bold- and stdFont

	// ERROR CHECK LEVEL -----------------------------------------------------------------------------
#if ERROR_CHECK_LEVEL == ERROR_CHECK_FULL
	strP = "ERROR_CHECK_FULL";
#elif ERROR_CHECK_LEVEL == ERROR_CHECK_PARTIAL
	strP = "ERROR_CHECK_PARTIAL";
#elif ERROR_CHECK_LEVEL == ERROR_CHECK_NONE
	strP = "ERROR_CHECK_NONE";
#else
	#error	ERROR: the compiler define 'ERROR_CHECK_LEVEL' must be defined!
#endif /* ERROR_CHECK_LEVEL */

	len = StrLen( strP );
#ifdef CONFIG_SONY
	if( gGlobalPrefs.sonyLibRefNum )
		HRWinDrawChars( gGlobalPrefs.sonyLibRefNum, strP, len, kExtAboutLeftOffset, y );
	else
#endif /* CONFIG_SONY */
		WinDrawChars( strP, len, kExtAboutLeftOffset, y );
	y += lineHeight;
	

	// COLORS ---------------------------------------------------------------------------------------
#ifdef CONFIG_COLOR
	strP = "CONFIG_COLOR";
	len = StrLen( strP );

#ifdef CONFIG_SONY
	if( gGlobalPrefs.sonyLibRefNum )
		HRWinDrawChars( gGlobalPrefs.sonyLibRefNum, strP, len, kExtAboutLeftOffset, y );
	else
#endif /* CONFIG_SONY */
		WinDrawChars( strP, len, kExtAboutLeftOffset, y );

	y += lineHeight;
#endif /* CONFIG_COLOR */

	// OS VERSION REQUIRED --------------------------------------------------------------------------
#ifdef CONFIG_OS_BELOW_35
	strP = "CONFIG_OS_BELOW_35";
	len = StrLen( strP );

#ifdef CONFIG_SONY
	if( gGlobalPrefs.sonyLibRefNum )
		HRWinDrawChars( gGlobalPrefs.sonyLibRefNum, strP, len, kExtAboutLeftOffset, y );
	else
#endif /* CONFIG_SONY */
		WinDrawChars( strP, len, kExtAboutLeftOffset, y );

	y += lineHeight;
#endif /* CONFIG_OS_BELOW_35 */

	// EXTENDED ABOUT DIALOG BOX --------------------------------------------------------------------
#ifdef CONFIG_EXT_ABOUT
	strP = "CONFIG_EXT_ABOUT";
	len = StrLen( strP );

#ifdef CONFIG_SONY
	if( gGlobalPrefs.sonyLibRefNum )
		HRWinDrawChars( gGlobalPrefs.sonyLibRefNum, strP, len, kExtAboutLeftOffset, y );
	else
#endif /* CONFIG_SONY */
		WinDrawChars( strP, len, kExtAboutLeftOffset, y );

	y += lineHeight;
#endif /* CONFIG_EXT_ABOUT */

	// HELP STRINGS ---------------------------------------------------------------------------------
#ifdef CONFIG_HLP_STRS
	strP = "CONFIG_HLP_STRS";
	len = StrLen( strP );

#ifdef CONFIG_SONY
	if( gGlobalPrefs.sonyLibRefNum )
		HRWinDrawChars( gGlobalPrefs.sonyLibRefNum, strP, len, kExtAboutLeftOffset, y );
	else
#endif /* CONFIG_SONY */
		WinDrawChars( strP, len, kExtAboutLeftOffset, y );

	y += lineHeight;
#endif /* CONFIG_HLP_STRS */

	// ADDITIONAL FEATURES --------------------------------------------------------------------------
#ifdef CONFIG_ADDITIONAL
	strP = "CONFIG_ADDITIONAL";
	len = StrLen( strP );

#ifdef CONFIG_SONY
	if( gGlobalPrefs.sonyLibRefNum )
		HRWinDrawChars( gGlobalPrefs.sonyLibRefNum, strP, len, kExtAboutLeftOffset, y );
	else
#endif /* CONFIG_SONY */
		WinDrawChars( strP, len, kExtAboutLeftOffset, y );

	y += lineHeight;
#endif /* CONFIG_ADDITIONAL */

	// ALLTODOS DIALOG ------------------------------------------------------------------------------
#ifdef CONFIG_ALLTODOS_DLG
	strP = "CONFIG_ALLTODOS_DLG";
	len = StrLen( strP );

#ifdef CONFIG_SONY
	if( gGlobalPrefs.sonyLibRefNum )
		HRWinDrawChars( gGlobalPrefs.sonyLibRefNum, strP, len, kExtAboutLeftOffset, y );
	else
#endif /* CONFIG_SONY */
		WinDrawChars( strP, len, kExtAboutLeftOffset, y );

	y += lineHeight;
#endif /* CONFIG_ALLTODOS_DLG */

	// DEFAULT CATEGORIES ----------------------------------------------------------------------------
#ifdef CONFIG_DEF_CATEGORY
	strP = "CONFIG_DEF_CATEGORY";
	len = StrLen( strP );

#ifdef CONFIG_SONY
	if( gGlobalPrefs.sonyLibRefNum )
		HRWinDrawChars( gGlobalPrefs.sonyLibRefNum, strP, len, kExtAboutLeftOffset, y );
	else
#endif /* CONFIG_SONY */
		WinDrawChars( strP, len, kExtAboutLeftOffset, y );

	y += lineHeight;
#endif /* CONFIG_DEF_CATEGORY */

	// VERSION FOR SONY? ----------------------------------------------------------------------------
#ifdef CONFIG_SONY
	strP = "CONFIG_SONY";
	len = StrLen( strP );

	if( gGlobalPrefs.sonyLibRefNum )
		HRWinDrawChars( gGlobalPrefs.sonyLibRefNum, strP, len, kExtAboutLeftOffset, y );
	else
		WinDrawChars( strP, len, kExtAboutLeftOffset, y );
	
	y += lineHeight;
#endif /* CONFIG_SONY */

	// VERSION FOR HANDERA? -------------------------------------------------------------------------
#ifdef CONFIG_HANDERA
	strP = "CONFIG_HANDERA";
	len = StrLen( strP );

#ifdef CONFIG_SONY
	if( gGlobalPrefs.sonyLibRefNum )
		HRWinDrawChars( gGlobalPrefs.sonyLibRefNum, strP, len, kExtAboutLeftOffset, y );
	else
#endif /* CONFIG_SONY */
		WinDrawChars( strP, len, kExtAboutLeftOffset, y );

	y += lineHeight;
#endif /* CONFIG_HANDERA */

	// VERSION WITH SUPPORT FOR JOGDIAL -------------------------------------------------------------
#ifdef CONFIG_JOGDIAL
	strP = "CONFIG_JOGDIAL";
	len = StrLen( strP );

#ifdef CONFIG_SONY
	if( gGlobalPrefs.sonyLibRefNum )
		HRWinDrawChars( gGlobalPrefs.sonyLibRefNum, strP, len, kExtAboutLeftOffset, y );
	else
#endif /* CONFIG_SONY */
		WinDrawChars( strP, len, kExtAboutLeftOffset, y );

	y += lineHeight;
#endif /* CONFIG_JOGDIAL */

	y += (lineHeight>>1);

	// draw separator
	WinDrawLine( r.topLeft.x + 5, y, r.topLeft.x + r.extent.x - 10, y );
	y += (lineHeight>>1);

#ifdef CONFIG_SONY
	if( gGlobalPrefs.sonyLibRefNum )
		HRFntSetFont( gGlobalPrefs.sonyLibRefNum, hrStdFont );
	else
#endif /* CONFIG_SONY */
		FntSetFont( stdFont );

	// we dot not need to get the line height again
	// because they are equal on the bold- and stdFont

	// draw eppilog
	memH = DmGetResource( strRsc, ExtAboutEppilog1 );
	ErrFatalDisplayIf( !memH, "resource not found" );
	strP = MemHandleLock( memH );
	len = StrLen( strP );
#ifdef CONFIG_SONY
	if( gGlobalPrefs.sonyLibRefNum )
		HRWinDrawChars( gGlobalPrefs.sonyLibRefNum, strP, len, kExtAboutLeftOffset, y );
	else
#endif /* CONFIG_SONY */
		WinDrawChars( strP, len, kExtAboutLeftOffset, y );
	MemHandleUnlock( memH );
	DmReleaseResource( memH );
	y += lineHeight;

	memH = DmGetResource( strRsc, ExtAboutEppilog2 );
	ErrFatalDisplayIf( !memH, "resource not found" );
	strP = MemHandleLock( memH );
	len = StrLen( strP );
#ifdef CONFIG_SONY
	if( gGlobalPrefs.sonyLibRefNum )
		HRWinDrawChars( gGlobalPrefs.sonyLibRefNum, strP, len, kExtAboutLeftOffset, y );
	else
#endif /* CONFIG_SONY */
		WinDrawChars( strP, len, kExtAboutLeftOffset, y );
	MemHandleUnlock( memH );
	DmReleaseResource( memH );
	y += lineHeight;
	y += (lineHeight>>1);

	// draw separator
	WinDrawLine( r.topLeft.x + 5, y, r.topLeft.x + r.extent.x - 10, y );
	y += (lineHeight>>1);

	// build the date & time string
	strP = szDateTime;
	StrCopy( strP, __DATE__ );
	while( *strP )
		strP++;
	StrCopy( strP, "  -  " );
	while( *strP )
		strP++;
	StrCopy( strP, __TIME__ );

	// draw build date & time
	len = StrLen( szDateTime );

#ifdef CONFIG_SONY
	if( gGlobalPrefs.sonyLibRefNum )
	{
		HRFntSetFont( gGlobalPrefs.sonyLibRefNum, hrBoldFont );
		HRWinDrawChars( gGlobalPrefs.sonyLibRefNum, szDateTime, len, kExtAboutLeftOffset, y );
	}
	else
#endif /* CONFIG_SONY */
	{
		FntSetFont( boldFont );
		WinDrawChars( szDateTime, len, kExtAboutLeftOffset, y );
	}

	// restore the previous draw state and free allocated memory
#ifdef CONFIG_SONY
	if( gGlobalPrefs.sonyLibRefNum )
		HRFntSetFont( gGlobalPrefs.sonyLibRefNum, hrCurFont );
	else
#endif /* CONFIG_SONY */
		FntSetFont( curFont );
	WinSetDrawWindow( curWinH );

	// set up our scroll bar on the about dialog
	sclP = FrmGetObjectPtr( frmP, FrmGetObjectIndex( frmP, ExtAboutScrollBar ) );
#ifdef CONFIG_SONY
	if( gGlobalPrefs.sonyLibRefNum )
		SclSetScrollBar( sclP, 0, 0, (r.extent.y - (kExtAboutPageVisibleHeight<<1))>>1, (kExtAboutPageVisibleHeight>>1) );
	else
#endif 
		SclSetScrollBar( sclP, 0, 0, r.extent.y - kExtAboutPageVisibleHeight, kExtAboutPageVisibleHeight>>1 );

	return (r.extent.y);
}

/*
 * FUNCTION:					ExtAboutHandleEvent
 * DESCRIPTION:				this routine handles the events on the
 * 										extended about dialog mainly it is responsible
 * 										for scrolling our page
 * PARAMETERS:				eventP - the event
 * RETURNS:						true if handled, else false
 */
Boolean ExtAboutHandleEvent( EventType * eventP )
{
	static WinHandle 	offscrH = NULL;

	Boolean handled = false;
	FormType * frmP;

	switch( eventP->eType )
	{
		case frmOpenEvent:
			frmP = FrmGetActiveForm();
#ifdef CONFIG_HANDERA
			if( gGlobalPrefs.vgaExtension )
				HndrCenterForm( frmP, false );
#endif /* CONFIG_HANDERA */
			ExtAboutInit( frmP, &offscrH );
			FrmDrawForm( frmP );
			if( offscrH )
				ExtAboutDraw( offscrH, 0 );
			handled = true;
			break;

		case frmCloseEvent:
			if( offscrH )
			{
				WinDeleteWindow( offscrH, false );
				offscrH = NULL;
			}
			break;

		case ctlSelectEvent:
			if( eventP->data.ctlSelect.controlID == ExtAboutDoneButton )
			{
				if( offscrH )
				{
					WinDeleteWindow( offscrH, false );
					offscrH = NULL;
				}
				FrmReturnToForm(0);
				handled = true;
			}
			break;

		case keyDownEvent:
			if( EvtKeydownIsVirtual( eventP ) && offscrH )
			{
				if( eventP->data.keyDown.chr == vchrPageDown )
				{
					ExtAboutScrollPage( offscrH, winDown, (eventP->data.keyDown.modifiers & autoRepeatKeyMask) == autoRepeatKeyMask );
					handled = true;
				}
				else if( eventP->data.keyDown.chr == vchrPageUp )
				{
					ExtAboutScrollPage( offscrH, winUp, (eventP->data.keyDown.modifiers & autoRepeatKeyMask) == autoRepeatKeyMask );
					handled = true;
				}
			}
			break;

		case sclRepeatEvent:
			if( offscrH )
				ExtAboutDraw( offscrH, eventP->data.sclRepeat.newValue );
			break;

		default:
			break;
	}

	return (handled);
}

#endif /* CONFIG_EXT_ABOUT */
