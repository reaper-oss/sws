/******************************************************************************
/ ReaScript.cpp
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
#include "SnM/SnM.h"
#include "Fingers/RprMidiTake.h"

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
	{ APIFUNC(SNM_CreateFastString), "WDL_FastString*", "const char*", "str", "[S&M] Instanciates a new \"fast string\" (i.e. WDL_FastString). Once the job done, you MUST delete this string, see SNM_DeleteFastString.", },
	{ APIFUNC(SNM_DeleteFastString), "void", "WDL_FastString*", "str", "[S&M] Deletes a \"fast string\" instance.", },
	{ APIFUNC(SNM_GetFastString), "const char*", "WDL_FastString*", "str", "[S&M] Gets the \"fast string\" content.", },
	{ APIFUNC(SNM_GetFastStringLength), "int", "WDL_FastString*", "str", "[S&M] Gets the \"fast string\" length.", },
	{ APIFUNC(SNM_SetFastString), "WDL_FastString*", "WDL_FastString*,const char*", "str,newstr", "[S&M] Sets the \"fast string\" content. Returns str for facility.", },
	{ APIFUNC(SNM_GetMediaItemTakeByGUID), "MediaItem_Take*", "ReaProject*,const char*", "project,guid", "[S&M] Gets a take by GUID as string.", },
	{ APIFUNC(SNM_GetSourceType), "bool","MediaItem_Take*,WDL_FastString*", "take,type", "[S&M] Gets the source type of a take. Returns false if failed (e.g. take with empty source, etc..)", },
	{ APIFUNC(SNM_GetSetSourceState), "bool", "MediaItem*,int,WDL_FastString*,bool", "item,takeidx,state,setnewvalue", "[S&M] Gets or sets a take source state. Returns false if failed. Use takeidx=-1 to get/alter the active take.\nNote: this function does not use a MediaItem_Take* param in order to manage empty takes (i.e. takes with MediaItem_Take*==NULL), see SNM_GetSetSourceState2.", },
	{ APIFUNC(SNM_GetSetSourceState2), "bool", "MediaItem_Take*,WDL_FastString*,bool", "take,state,setnewvalue", "[S&M] Gets or sets a take source state. Returns false if failed.\nNote: this function cannot deal with empty takes, see SNM_GetSetSourceState.", },
	{ APIFUNC(SNM_GetSetObjectState), "bool", "void*,WDL_FastString*,bool,bool", "obj,state,setnewvalue,wantminimalstate", "[S&M] Gets or sets the state of a track, an item or an envelope. Returns false if failed.\nWhen getting a track state (and when you are not interested in FX data), you can use wantminimalstate=true to radically reduce the length of the state. Do not set such minimal states back though, this is for read-only applications!\nNote: unlike the native GetSetObjectState, this function does not require any call to FreeHeapPtr.", },
	{ APIFUNC(SNM_AddReceive), "bool", "MediaTrack*,MediaTrack*,int", "src,dest,type", "[S&M] Adds a receive. Returns false if failed.\ntype 0=Post-Fader (Post-Pan), 1=Pre-FX, 2=deprecated, 3=Pre-Fader (Post-FX).\nNote: obeys default sends preferences, supports frozen tracks, etc..", },
	{ APIFUNC(SNM_RemoveReceive), "bool", "MediaTrack*,int", "tr,rcvidx", "[S&M] Removes a receive. Returns false if failed.\nNote: supports frozen tracks, etc..", },
	{ APIFUNC(SNM_GetIntConfigVar), "int", "const char*,int", "varname,errvalue", "[S&M] Returns an integer preference (look in project prefs first, then in general prefs). Returns errvalue if failed (e.g. varname not found).", },
	{ APIFUNC(SNM_SetIntConfigVar), "bool", "const char*,int", "varname,newvalue", "[S&M] Sets an integer preference (look in project prefs first, then in general prefs). Returns false if failed (e.g. varname not found).", },
	{ APIFUNC(SNM_GetDoubleConfigVar), "double", "const char*,double", "varname,errvalue", "[S&M] Returns a double preference (look in project prefs first, then in general prefs). Returns errvalue if failed (e.g. varname not found).", },
	{ APIFUNC(SNM_SetDoubleConfigVar), "bool", "const char*,double", "varname,newvalue", "[S&M] Sets a double preference (look in project prefs first, then in general prefs). Returns false if failed (e.g. varname not found).", },
	{ APIFUNC(SNM_MoveOrRemoveTrackFX), "bool", "MediaTrack*,int,int", "tr,fxId,what", "[S&M] Move or removes a track FX. Returns true if tr has been updated.\nfxId: fx index in chain or -1 for the selected fx.\nwhat: 0 to remove, -1 to move fx up in chain, 1 to move fx down in chain Sets a double preference.", },
	{ APIFUNC(SNM_GetProjectMarkerName), "bool", "ReaProject*,int,bool,WDL_FastString*", "proj,num,isrgn,name", "[S&M] Gets a marker/region name. Returns true if marker/region found.", },
	{ APIFUNC(SNM_SetProjectMarker), "bool", "ReaProject*,int,bool,double,double,const char*,int", "proj,num,isrgn,pos,rgnend,name,color", "[S&M] See SetProjectMarker3, it is the same function but this one can set empty names, i.e. \"\".", },

	{ APIFUNC(FNG_AllocMidiTake), "RprMidiTake*", "MediaItem_Take*", "take", "[FNG] Allocate a RprMidiTake from a take pointer. Returns a NULL pointer if the take is not an in-project MIDI take", },
	{ APIFUNC(FNG_FreeMidiTake), "void", "RprMidiTake*", "midiTake", "[FNG] Commit changes to MIDI take and free allocated memory", },
	{ APIFUNC(FNG_CountMidiNotes), "int", "RprMidiTake*", "midiTake", "[FNG] Count of how many MIDI notes are in the MIDI take", },
	{ APIFUNC(FNG_GetMidiNote), "RprMidiNote*", "RprMidiTake*,int", "midiTake,index", "[FNG] Get a MIDI note from a MIDI take at specified index", },
	{ APIFUNC(FNG_GetMidiNoteIntProperty), "int", "RprMidiNote*,const char*", "midiNote,property", "[FNG] Get MIDI note property", },
	{ APIFUNC(FNG_SetMidiNoteIntProperty), "void", "RprMidiNote*,const char*,int", "midiNote,property,value", "[FNG] Set MIDI note property", },
	{ APIFUNC(FNG_AddMidiNote), "RprMidiNote*", "RprMidiTake*", "midiTake", "[FNG] Add MIDI note to MIDI take", },

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
// => trick: re-register empty definitions so that exported funcs won't be part of the generated reaper_plugin_functions.h
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

