#ifndef __PROJECTS_MEMO_LOOKUP_H__
#define __PROJECTS_MEMO_LOOKUP_H__

#include <PalmOS.h>
#include "ProjectsRcp.h"

// upon return commited by the "add" button this
// variable will hold the db index of the selected
// record
extern UInt16			gMemoLookupSelectedIndex;


extern void MemoLookupInitialize( FormType * frmP, DmOpenRef memoDB ) THIRD_SECTION; // must be called before FrmDoDialog !!!!
extern Boolean MemoLookupHandleEvent( EventType * eventP ) THIRD_SECTION;

#endif /* __PROJECTS_MEMO_LOOKUP_H__ */
