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
#include "Reascript.h"
#include "SnM/SnM_Misc.h"


// Important: keep APIFUNC() and the four first fields of the struct "APIdef" as they are defined:
// the script reascript_python.pl needs those to parse the exported REAPER functions and generate 
// reaper_python.py at compile-time. 
// "Yes, that is a Perl script reading C++ to generate Python." - schwa, dec. 2011
// See http://code.google.com/p/sws-extension/issues/detail?id=432


#define APIFUNC(x) (void*)x,#x

typedef struct APIdef
{
	void* func;
	const char* func_name;
	const char* ret_val;
	const char* parm_types;

	// if additionnal data are needed, add them below (see top remark)
	const char* parm_names;
	const char* help;

	char* dyn_defs[3]; // used for dynamic allocations and later cleanups, see FreeReascriptExport()

} APIdef;


///////////////////////////////////////////////////////////////////////////////
// Add the functions you want to export here (+ related #include on top)
// Make sure your function name has a prefix like "SWS_", "FNG_", etc.. 
///////////////////////////////////////////////////////////////////////////////
APIdef g_apidefs[] =
{
	// S&M stuff
	{ APIFUNC(SNM_AddReceive), "bool", "MediaTrack*,MediaTrack*,int", "src,dest,type", "[S&M extension] Adds a receive, type 0=Post-Fader (Post-Pan), 1=Pre-FX, 2=deprecated, 3=Pre-Fader (Post-FX)", },
	{ APIFUNC(SNM_GetIntConfigVar), "int", "const char*,int", "varName,errVal", "[S&M extension] Returns an integer preference (look in project prefs first, then in general prefs). Returns errVal if it fails (e.g. varName not found).", },
	{ APIFUNC(SNM_SetIntConfigVar), "bool", "const char*,int", "varName,newVal", "[S&M extension] Sets an integer preference (look in project prefs first, then in general prefs). Returns false if it fails (e.g. varName not found).", },
	{ APIFUNC(SNM_GetDoubleConfigVar), "double", "const char*,double", "varName,errVal", "[S&M extension] Returns a double preference (look in project prefs first, then in general prefs). Returns errVal if it fails (e.g. varName not found).", },
	{ APIFUNC(SNM_SetDoubleConfigVar), "bool", "const char*,double", "varName,newVal", "[S&M extension] Sets a double preference (look in project prefs first, then in general prefs). Returns false if it fails (e.g. varName not found).", },

	// SWS stuff
	// etc..

	{ NULL, } // denote end of table
};
///////////////////////////////////////////////////////////////////////////////


bool ExportReascript(reaper_plugin_info_t* _rec)
{
	bool ok = (_rec!=NULL);
	int i=-1;
	while (ok && g_apidefs[++i].func)
	{
		char tmp[2048] = "";
		_snprintf(tmp, sizeof(tmp), "API_%s", g_apidefs[i].func_name);
		g_apidefs[i].dyn_defs[0] = _strdup(tmp); // _strdup(): reaper_plugin_info_t.Register() requires permanent (heap allocated) strings

		_snprintf(tmp, sizeof(tmp), "APIdef_%s", g_apidefs[i].func_name);
		g_apidefs[i].dyn_defs[1] = _strdup(tmp);

		memset(tmp, 0, sizeof(tmp));
		_snprintf(tmp, sizeof(tmp), "%s\n%s\n%s\n%s", g_apidefs[i].ret_val, g_apidefs[i].parm_types, g_apidefs[i].parm_names, g_apidefs[i].help);
		char* p = g_apidefs[i].dyn_defs[2] = _strdup(tmp);
		while (*p) { if (*p=='\n') *p='\0'; p++; }

		ok &= (_rec->Register(g_apidefs[i].dyn_defs[0], g_apidefs[i].func) != 0);
		ok &= (_rec->Register(g_apidefs[i].dyn_defs[1], (void*)g_apidefs[i].dyn_defs[2]) != 0);
	}
	return ok;
}

void FreeReascriptExport()
{
	int i=-1;
	while (g_apidefs[++i].func)
		for (int j=0; j<3; j++)
			free(g_apidefs[i].dyn_defs[j]);
}
