/******************************************************************************
/ Reascript.cpp
/
/ Copyright (c) 2012 Jeffos
/ http://www.standingwaterstudios.com/reaper
/
/ Permission is hereby granted, free of charge, to any person obtaining a copy
/ of this software and associated documentation files (the "Software"), to deal
/ in the Software without restriction, including without limitation the rights to
/ use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
/ of the Software, and to permit persons to whom the Software is furnished to
/ do so, subject to the following conditions:
/
/ The above copyright notice and this permission notice shall be included in all
/ copies or substantial portions of the Software.
/
/ THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
/ EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
/ OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
/ NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
/ HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
/ WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
/ FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
/ OTHER DEALINGS IN THE SOFTWARE.
/
******************************************************************************/

#include "stdafx.h"
#include "SnM/SnM_Misc.h"


// Important: 
// keep APIFUNC() and the 6 first fields of the struct "APIdef" as they are defined:
// the script reascript_python.pl needs those to parse the exported functions and generate
// python wrappers (sws_python.py) at compile-time. 
// "Yes, that is a Perl script reading C++ to generate Python." - schwa
// See http://code.google.com/p/sws-extension/issues/detail?id=432


#define APIFUNC(x) (void*)x,#x,"API_" #x "","APIdef_" #x ""

typedef struct APIdef
{
	void* func;
	const char* func_name;
	const char* regkey_func;
	const char* regkey_def;
	const char* ret_val;
	const char* parm_types;

	// if additionnal data are needed, add them below (see top remark)
	const char* parm_names;
	const char* help;
	char* dyn_def; // used for dynamic allocations/cleanups

} APIdef;


///////////////////////////////////////////////////////////////////////////////
// Add the functions you want to export here (+ related #include on top)
// Make sure function names have a prefix like "SWS_", "FNG_", etc.. 
///////////////////////////////////////////////////////////////////////////////

APIdef g_apidefs[] =
{
	// S&M stuff
	{ APIFUNC(SNM_GetMediaItemTakeByGUID), "MediaItem_Take*", "ReaProject*,const char*", "project,guid", "[SWS/S&M extension] Gets a take by GUID as string.", },
	{ APIFUNC(SNM_GetSetSourceState), "bool", "MediaItem*,int,char*,bool", "item,takeIdx,state,setnewvalue", "[SWS/S&M extension] Gets or sets a take's source state. Use takeIdx=-1 to get/alter the active take.\nNote: this func does not use a MediaItem_Take* param in order to manage empty takes (i.e. takes with MediaItem_Take*==NULL), also see SNM_GetSetTakeSourceState.", },
	{ APIFUNC(SNM_AddReceive), "bool", "MediaTrack*,MediaTrack*,int", "src,dest,type", "[SWS/S&M extension] Adds a receive, type 0=Post-Fader (Post-Pan), 1=Pre-FX, 2=deprecated, 3=Pre-Fader (Post-FX).\nNote: obeys default sends preferences, supports frozen tracks, etc..", },
	{ APIFUNC(SNM_GetIntConfigVar), "int", "const char*,int", "varName,errVal", "[SWS/S&M extension] Returns an integer preference (look in project prefs first, then in general prefs). Returns errVal if it fails (e.g. varName not found).", },
	{ APIFUNC(SNM_SetIntConfigVar), "bool", "const char*,int", "varName,newVal", "[SWS/S&M extension] Sets an integer preference (look in project prefs first, then in general prefs). Returns false if it fails (e.g. varName not found).", },
	{ APIFUNC(SNM_GetDoubleConfigVar), "double", "const char*,double", "varName,errVal", "[SWS/S&M extension] Returns a double preference (look in project prefs first, then in general prefs). Returns errVal if it fails (e.g. varName not found).", },
	{ APIFUNC(SNM_SetDoubleConfigVar), "bool", "const char*,double", "varName,newVal", "[SWS/S&M extension] Sets a double preference (look in project prefs first, then in general prefs). Returns false if it fails (e.g. varName not found).", },

	// SWS stuff
	// etc..

	{ NULL, } // denote end of table
};

///////////////////////////////////////////////////////////////////////////////

bool RegisterExportedFuncs(reaper_plugin_info_t* _rec)
{
	bool ok = (_rec!=NULL);
	int i=-1;
	while (ok && g_apidefs[++i].func)
		ok &= (_rec->Register(g_apidefs[i].regkey_func, g_apidefs[i].func) != 0);
	return ok;
}

bool RegisterExportedAPI(reaper_plugin_info_t* _rec)
{
	bool ok = (_rec!=NULL);
	int i=-1;
	char tmp[8*1024];
	while (ok && g_apidefs[++i].func)
	{
		memset(tmp, 0, sizeof(tmp));
		_snprintf(tmp, sizeof(tmp), "%s\r%s\r%s\r%s", g_apidefs[i].ret_val, g_apidefs[i].parm_types, g_apidefs[i].parm_names, g_apidefs[i].help);
		char* p = g_apidefs[i].dyn_def = _strdup(tmp);
		while (*p) { if (*p=='\r') *p='\0'; p++; }
		ok &= (_rec->Register(g_apidefs[i].regkey_def, g_apidefs[i].dyn_def) != 0);
	}
	return ok;
}

// REAPER bug? unfortunately _rec->Register("-APIdef_myfunc",..) does not work
// => trick: we re-register definitions as empty strings so that exported funcs
//           won't be part of the generated reaper_plugin_functions.h
bool UnregisterExportedAPI(reaper_plugin_info_t* _rec)
{
	bool ok = (_rec!=NULL);
	int i=-1;
	while (ok && g_apidefs[++i].func)
	{
/*JFB commented: does not work, see remark above..
		char tmp[2048];
		_snprintf(tmp, sizeof(tmp), "-%s", g_apidefs[i].regkey_def);
		ok &= (_rec->Register(tmp, (void*)g_apidefs[i].dyn_def) != 0);
*/
// => trick:
		char* oldDef = g_apidefs[i].dyn_def;
		g_apidefs[i].dyn_def = _strdup("");
		ok &= (_rec->Register(g_apidefs[i].regkey_def, g_apidefs[i].dyn_def) != 0);
		free(oldDef);
	}
	return ok;
}

