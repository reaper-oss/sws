/************************************************
** Copyright (C) 2006-2012, Cockos Incorporated
*/

#ifndef _REAPER_LOCALIZE_H_
#define _REAPER_LOCALIZE_H_


#define LOCALIZE_FLAG_VERIFY_FMTS 1 // verifies translated format-string (%d should match %d, etc)
#define LOCALIZE_FLAG_NOCACHE 2
#define LOCALIZE_FLAG_PAIR 4 // one \0 in string needed -- this is not doublenull terminated but just a special case
#define LOCALIZE_FLAG_DOUBLENULL 8 // doublenull terminated string
#define __LOCALIZE(str, ctx) __localizeFunc("" str "" , "" ctx "",0)
#define __LOCALIZE_2N(str,ctx) __localizeFunc("" str "" , "" ctx "",LOCALIZE_FLAG_PAIR)
#define __LOCALIZE_VERFMT(str, ctx) __localizeFunc("" str "", "" ctx "",LOCALIZE_FLAG_VERIFY_FMTS)
#define __LOCALIZE_NOCACHE(str, ctx) __localizeFunc("" str "", "" ctx "",LOCALIZE_FLAG_NOCACHE)
#define __LOCALIZE_VERFMT_NOCACHE(str, ctx) __localizeFunc("" str "", "" ctx "",LOCALIZE_FLAG_VERIFY_FMTS|LOCALIZE_FLAG_NOCACHE)

// localize a string
const char *__localizeFunc(const char *str, const char *subctx, int flags);

// localize a menu; rescat can be NULL, or a prefix
void __localizeMenu(const char *rescat, HMENU hMenu, LPCSTR lpMenuName);


// localize a dialog; rescat can be NULL, or a prefix
// if returns non-NULL, use retval as dlgproc and pass ptrs as LPARAM to dialogboxparam
// nptrs should be at least 4 (might increase someday, but this will only ever be used by utilfunc.cpp or localize-import.h)
DLGPROC __localizePrepareDialog(const char *rescat, HINSTANCE hInstance, const char *lpTemplate, DLGPROC dlgProc, LPARAM lParam, void **ptrs, int nptrs); 


#ifndef __REAMOTE__

#ifndef UTILFUNC_NODIALOGBOX_REDEF
#undef DialogBox
#undef CreateDialog
#undef DialogBoxParam
#undef CreateDialogParam

#ifdef _WIN32
  #define DialogBoxParam(a,b,c,d,e) ((INT_PTR)__localizeDialog(a,b,c,d,e,1))
  #define CreateDialogParam(a,b,c,d,e) __localizeDialog(a,b,c,d,e,0)
#else
  #define DialogBoxParam(a,b,c,d,e) ((INT_PTR)__localizeDialog(NULL,b,c,d,e,1))
  #define CreateDialogParam(a,b,c,d,e) __localizeDialog(NULL,b,c,d,e,0)
#endif

#define DialogBox(hinst,lpt,hwndp,dlgproc) DialogBoxParam(hinst,lpt,hwndp,dlgproc,0)
#define CreateDialog(hinst,lpt,hwndp,dlgproc) CreateDialogParam(hinst,lpt,hwndp,dlgproc,0)


#undef LoadMenu
#define LoadMenu(a,b) __localizeLoadMenu(a,b)
#endif


HMENU __localizeLoadMenu(HINSTANCE hInstance, const char *lpMenuName);
HWND __localizeDialog(HINSTANCE hInstance, const char * lpTemplate, HWND hwndParent, DLGPROC dlgProc, LPARAM lParam, int mode);

// wrappers so utilfunc.cpp can call these directly
#define __localizeDialogBoxParam(a,b,c,d,e) ((INT_PTR)__localizeDialog(a,b,c,d,e,1))
#define __localizeCreateDialogParam(a,b,c,d,e) __localizeDialog(a,b,c,d,e,0)

#endif


#endif
