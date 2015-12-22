/******************************************************************************
/ ReaScript.cpp
/
/ Copyright (c) 2012 and later Jeffos
/
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
#include "SnM/SnM_FX.h"
#include "SnM/SnM_Misc.h"
#include "SnM/SnM_Resources.h"
#include "SnM/SnM_Routing.h"
#include "SnM/SnM_Track.h"
#include "Fingers/RprMidiTake.h"
#include "Breeder/BR_ReaScript.h"


// if _TEST_REASCRIPT_EXPORT is #define'd, you'll need to rename "APITESTFUNC" into "APIFUNC" in g_apidefs too
// (because our function wrapper generation scripts aren't smart enough to skip #ifdef'ed out lines ATM)
// you will find also the realed reascript_test.eel and reascript_test.lua in the repo (python: todo).
//#define _TEST_REASCRIPT_EXPORT
#ifdef _TEST_REASCRIPT_EXPORT
  #include "reascript_test.c" // test all possible parameter/return types
#endif

#ifdef _WIN32
  #pragma warning(push, 0)
  #pragma warning(disable: 4800) // disable "forcing value to bool..." warnings
#endif
#include "reascript_vararg.h"
#ifdef _WIN32
  #pragma warning(pop)
#endif


// Important, keep APIFUNC() and APIdef as they are defined: the scripts reascript_vararg.php
// and reascript_python.pl both parse the function definition table (g_apidefs) at compilation
// time to generate variable argument function wrappers for EEL and Lua (reascript_vararg.h),
// and python function wrappers (sws_python32.py, sws_python64.py), respectively.

#ifdef _REASCRIPT_VARARG_H_
  #define APIFUNC(x)  (void*)x,#x,(void*)__vararg_ ## x,"APIvararg_" #x "","API_" #x "","APIdef_" #x ""
  #define CAPIFUNC(x) (void*)x,#x,NULL,NULL,"API_" #x "",NULL // export to C/C++ only
#else
  #pragma message("WARNING: DISABLING FUNCTION EXPORT TO LUA/EEL REASCRIPTS! Probable cause: 'reascript_vararg.php' couldn't generate 'reascript_vararg.h' (PHP installed?)")
  #define APIFUNC(x)  (void*)x,#x,NULL,NULL,"API_" #x "",NULL // export to C/C++ only
  #define CAPIFUNC(x) (void*)x,#x,NULL,NULL,"API_" #x "",NULL // export to C/C++ only
#endif

typedef struct APIdef
{
	void* func;
	const char* func_name;
	void* func_vararg;
	const char* regkey_vararg;
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
//
// Add functions you want to export in the table below (+ related #include on
// top). To distinguish SWS and native functions, make sure function names have
// a prefix like "SWS_", "FNG_", etc.
// Your functions must be made dumb-proof, they must not trust any parameter.
// However, your do not need to ValidatePr() REAPER pointer parameters (such as 
// MediaTrack*, MediaItem*, etc): REAPER does this for you when a script runs
// an exported function.
// REAPER pointer parameters are validated against the prior ReaProject* param,
// (or the current project if absent/NULL). For ex., in the following function:
// void bla(ReaProject* p1, MediaTrack* t1, ReaProject* p2, MediaItem* i2),
// t1 will be validated against p1, but i2 will be validated against p2.
// Your must interpret NULL ReaProject* params as "the current project".
//
// When defining/documenting API function parameters:
//  - if a (char*,int) pair is encountered, name them buf, buf_sz
//  - if a (const char*,int) pair is encountered, buf, buf_sz as well
//  - if a lone basicType *, use varNameOut or varNameIn or
//    varNameInOptional (if last parameter(s))
// At the moment (REAPER v5pre6) the supported parameter types are:
//  - int, int*, bool, bool*, double, double*, char*, const char*
//  - AnyStructOrClass* (handled as an opaque pointer)
// At the moment (REAPER v5pre6) the supported return types are:
//  - int, bool, double, const char*
//  - AnyStructOrClass* (handled as an opaque pointer)
//
///////////////////////////////////////////////////////////////////////////////

APIdef g_apidefs[] =
{
#ifdef _TEST_REASCRIPT_EXPORT
	{ APITESTFUNC(SNM_test1), "int", "int*", "aOut", "", },
	{ APITESTFUNC(SNM_test2), "double", "double*", "aOut", "", },
	{ APITESTFUNC(SNM_test3), "bool", "int*,double*,bool*", "aOut,bOut,cOut", "", },
	{ APITESTFUNC(SNM_test4), "void", "double", "a", "", },
	{ APITESTFUNC(SNM_test5), "void", "const char*", "a", "", },
	{ APITESTFUNC(SNM_test6), "const char*", "char*", "a", "", }, // not an "Out" parm
	{ APITESTFUNC(SNM_test7), "double", "int,int*,double*,bool*,char*,const char*", "i,aOut,bInOptional,cOutOptional,sOutOptional,csInOptional", "", },
	{ APITESTFUNC(SNM_test8), "const char*", "char*,int,const char*,int,int,char*,int,int*", "buf1,buf1_sz,buf2,buf2_sz,i,buf3,buf3_sz,iOutOptional", "", },
#endif
	{ APIFUNC(SNM_CreateFastString), "WDL_FastString*", "const char*", "str", "[S&M] Instantiates a new \"fast string\". You must delete this string, see SNM_DeleteFastString.", },
	{ APIFUNC(SNM_DeleteFastString), "void", "WDL_FastString*", "str", "[S&M] Deletes a \"fast string\" instance.", },
	{ APIFUNC(SNM_GetFastString), "const char*", "WDL_FastString*", "str", "[S&M] Gets the \"fast string\" content.", },
	{ APIFUNC(SNM_GetFastStringLength), "int", "WDL_FastString*", "str", "[S&M] Gets the \"fast string\" length.", },
	{ APIFUNC(SNM_SetFastString), "WDL_FastString*", "WDL_FastString*,const char*", "str,newstr", "[S&M] Sets the \"fast string\" content. Returns str for facility.", },
	{ APIFUNC(SNM_GetMediaItemTakeByGUID), "MediaItem_Take*", "ReaProject*,const char*", "project,guid", "[S&M] Gets a take by GUID as string. The GUID must be enclosed in braces {}. To get take GUID as string, see BR_GetMediaItemTakeGUID", },
	{ APIFUNC(SNM_GetSourceType), "bool","MediaItem_Take*,WDL_FastString*", "take,type", "[S&M] Gets the source type of a take. Returns false if failed (e.g. take with empty source, etc..)", },
	{ APIFUNC(SNM_GetSetSourceState), "bool", "MediaItem*,int,WDL_FastString*,bool", "item,takeidx,state,setnewvalue", "[S&M] Gets or sets a take source state. Returns false if failed. Use takeidx=-1 to get/alter the active take.\nNote: this function does not use a MediaItem_Take* param in order to manage empty takes (i.e. takes with MediaItem_Take*==NULL), see SNM_GetSetSourceState2.", },
	{ APIFUNC(SNM_GetSetSourceState2), "bool", "MediaItem_Take*,WDL_FastString*,bool", "take,state,setnewvalue", "[S&M] Gets or sets a take source state. Returns false if failed.\nNote: this function cannot deal with empty takes, see SNM_GetSetSourceState.", },
	{ APIFUNC(SNM_GetSetObjectState), "bool", "void*,WDL_FastString*,bool,bool", "obj,state,setnewvalue,wantminimalstate", "[S&M] Gets or sets the state of a track, an item or an envelope. The state chunk size is unlimited. Returns false if failed.\nWhen getting a track state (and when you are not interested in FX data), you can use wantminimalstate=true to radically reduce the length of the state. Do not set such minimal states back though, this is for read-only applications!\nNote: unlike the native GetSetObjectState, calling to FreeHeapPtr() is not required.", },
	{ APIFUNC(SNM_AddReceive), "bool", "MediaTrack*,MediaTrack*,int", "src,dest,type", "[S&M] Adds a receive. Returns false if nothing updated.\ntype -1=Default type (user preferences), 0=Post-Fader (Post-Pan), 1=Pre-FX, 2=deprecated, 3=Pre-Fader (Post-FX).\nNote: obeys default sends preferences, supports frozen tracks, etc..", },
	{ APIFUNC(SNM_RemoveReceive), "bool", "MediaTrack*,int", "tr,rcvidx", "[S&M] Removes a receive. Returns false if nothing updated.", },
	{ APIFUNC(SNM_RemoveReceivesFrom), "bool", "MediaTrack*,MediaTrack*", "tr,srctr", "[S&M] Removes all receives from srctr. Returns false if nothing updated.", },
	{ APIFUNC(SNM_GetIntConfigVar), "int", "const char*,int", "varname,errvalue", "[S&M] Returns an integer preference (look in project prefs first, then in general prefs). Returns errvalue if failed (e.g. varname not found).", },
	{ APIFUNC(SNM_SetIntConfigVar), "bool", "const char*,int", "varname,newvalue", "[S&M] Sets an integer preference (look in project prefs first, then in general prefs). Returns false if failed (e.g. varname not found).", },
	{ APIFUNC(SNM_GetDoubleConfigVar), "double", "const char*,double", "varname,errvalue", "[S&M] Returns a double preference (look in project prefs first, then in general prefs). Returns errvalue if failed (e.g. varname not found).", },
	{ APIFUNC(SNM_SetDoubleConfigVar), "bool", "const char*,double", "varname,newvalue", "[S&M] Sets a double preference (look in project prefs first, then in general prefs). Returns false if failed (e.g. varname not found).", },
	{ APIFUNC(SNM_MoveOrRemoveTrackFX), "bool", "MediaTrack*,int,int", "tr,fxId,what", "[S&M] Move or removes a track FX. Returns true if tr has been updated.\nfxId: fx index in chain or -1 for the selected fx. what: 0 to remove, -1 to move fx up in chain, 1 to move fx down in chain.", },
	{ APIFUNC(SNM_GetProjectMarkerName), "bool", "ReaProject*,int,bool,WDL_FastString*", "proj,num,isrgn,name", "[S&M] Gets a marker/region name. Returns true if marker/region found.", },
	{ APIFUNC(SNM_SetProjectMarker), "bool", "ReaProject*,int,bool,double,double,const char*,int", "proj,num,isrgn,pos,rgnend,name,color", "[S&M] See SetProjectMarker3(), it is the same function but this one can set empty names -- DEPRECATED: you can use the new native function SetProjectMarker4() instead.", },
	{ APIFUNC(SNM_SelectResourceBookmark), "int", "const char*", "name", "[S&M] Select a bookmark of the Resources window. Returns the related bookmark id (or -1 if failed).", },
	{ APIFUNC(SNM_TieResourceSlotActions), "void", "int", "bookmarkId", "[S&M] Attach Resources slot actions to a given bookmark.", },
	{ APIFUNC(SNM_AddTCPFXParm), "bool", "MediaTrack*,int,int", "tr,fxId,prmId", "[S&M] Add an FX parameter knob in the TCP. Returns false if nothing updated (invalid parameters, knob already present, etc..)", },
	{ APIFUNC(SNM_TagMediaFile), "bool", "const char*,const char*,const char*", "fn,tag,tagval", "[S&M] Tags a media file thanks to <a href=\"https://taglib.github.io\">TagLib</a>. Supported tags: \"artist\", \"album\", \"genre\", \"comment\", \"title\", or \"year\". Use an empty tagval to clear a tag. When a file is opened in REAPER, turn it offline before using this function. Returns false if nothing updated. See SNM_ReadMediaFileTag.", },
	{ APIFUNC(SNM_ReadMediaFileTag), "bool", "const char*,const char*,char*,int", "fn,tag,tagval,tagval_sz", "[S&M] Reads a media file tag. Supported tags: \"artist\", \"album\", \"genre\", \"comment\", \"title\", or \"year\". Returns false if tag was not found. See SNM_TagMediaFile.", },

	{ APIFUNC(FNG_AllocMidiTake), "RprMidiTake*", "MediaItem_Take*", "take", "[FNG] Allocate a RprMidiTake from a take pointer. Returns a NULL pointer if the take is not an in-project MIDI take", },
	{ APIFUNC(FNG_FreeMidiTake), "void", "RprMidiTake*", "midiTake", "[FNG] Commit changes to MIDI take and free allocated memory", },
	{ APIFUNC(FNG_CountMidiNotes), "int", "RprMidiTake*", "midiTake", "[FNG] Count of how many MIDI notes are in the MIDI take", },
	{ APIFUNC(FNG_GetMidiNote), "RprMidiNote*", "RprMidiTake*,int", "midiTake,index", "[FNG] Get a MIDI note from a MIDI take at specified index", },
	{ APIFUNC(FNG_GetMidiNoteIntProperty), "int", "RprMidiNote*,const char*", "midiNote,property", "[FNG] Get MIDI note property", },
	{ APIFUNC(FNG_SetMidiNoteIntProperty), "void", "RprMidiNote*,const char*,int", "midiNote,property,value", "[FNG] Set MIDI note property", },
	{ APIFUNC(FNG_AddMidiNote), "RprMidiNote*", "RprMidiTake*", "midiTake", "[FNG] Add MIDI note to MIDI take", },

	{ APIFUNC(BR_EnvAlloc), "BR_Envelope*", "TrackEnvelope*,bool", "envelope,takeEnvelopesUseProjectTime", "[BR] Allocate envelope object from track or take envelope pointer. Always call <a href=\"#BR_EnvFree\">BR_EnvFree</a> when done to release the object and commit changes if needed.\n takeEnvelopesUseProjectTime: take envelope points' positions are counted from take position, not project start time. If you want to work with project time instead, pass this as true.\n\nFor further manipulation see BR_EnvCountPoints, BR_EnvDeletePoint, BR_EnvFind, BR_EnvFindNext, BR_EnvFindPrevious, BR_EnvGetParentTake, BR_EnvGetParentTrack, BR_EnvGetPoint, BR_EnvGetProperties, BR_EnvSetPoint, BR_EnvSetProperties, BR_EnvValueAtPos.", },
	{ APIFUNC(BR_EnvCountPoints), "int", "BR_Envelope*", "envelope", "[BR] Count envelope points in the envelope object allocated with <a href=\"#BR_EnvAlloc\">BR_EnvAlloc</a>.", },
	{ APIFUNC(BR_EnvDeletePoint), "bool", "BR_Envelope*,int", "envelope,id", "[BR] Delete envelope point by index (zero-based) in the envelope object allocated with <a href=\"#BR_EnvAlloc\">BR_EnvAlloc</a>. Returns true on success.", },
	{ APIFUNC(BR_EnvFind), "int", "BR_Envelope*,double,double", "envelope,position,delta", "[BR] Find envelope point at time position in the envelope object allocated with <a href=\"#BR_EnvAlloc\">BR_EnvAlloc</a>. Pass delta > 0 to search surrounding range - in that case the closest point to position within delta will be searched for. Returns envelope point id (zero-based) on success or -1 on failure.", },
	{ APIFUNC(BR_EnvFindNext), "int", "BR_Envelope*,double", "envelope,position", "[BR] Find next envelope point after time position in the envelope object allocated with <a href=\"#BR_EnvAlloc\">BR_EnvAlloc</a>. Returns envelope point id (zero-based) on success or -1 on failure.", },
	{ APIFUNC(BR_EnvFindPrevious), "int", "BR_Envelope*,double", "envelope,position", "[BR] Find previous envelope point before time position in the envelope object allocated with <a href=\"#BR_EnvAlloc\">BR_EnvAlloc</a>. Returns envelope point id (zero-based) on success or -1 on failure.", },
	{ APIFUNC(BR_EnvFree), "bool", "BR_Envelope*,bool", "envelope,commit", "[BR] Free envelope object allocated with <a href=\"#BR_EnvAlloc\">BR_EnvAlloc</a> and commit changes if needed. Returns true if changes were committed successfully. Note that when envelope object wasn't modified nothing will get committed even if commit = true - in that case function returns false.", },
	{ APIFUNC(BR_EnvGetParentTake), "MediaItem_Take*", "BR_Envelope*", "envelope", "[BR] If envelope object allocated with <a href=\"#BR_EnvAlloc\">BR_EnvAlloc</a> is take envelope, returns parent media item take, otherwise NULL.", },
	{ APIFUNC(BR_EnvGetParentTrack), "MediaItem*", "BR_Envelope*", "envelope", "[BR] Get parent track of envelope object allocated with <a href=\"#BR_EnvAlloc\">BR_EnvAlloc</a>. If take envelope, returns NULL.", },
	{ APIFUNC(BR_EnvGetPoint), "bool", "BR_Envelope*,int,double*,double*,int*,bool*,double*", "envelope,id,positionOut,valueOut,shapeOut,selectedOut,bezierOut", "[BR] Get envelope point by id (zero-based) from the envelope object allocated with <a href=\"#BR_EnvAlloc\">BR_EnvAlloc</a>. Returns true on success.", },
	{ APIFUNC(BR_EnvGetProperties), "void", "BR_Envelope*,bool*,bool*,bool*,bool*,int*,int*,double*,double*,double*,int*,bool*", "envelope,activeOut,visibleOut,armedOut,inLaneOut,laneHeightOut,defaultShapeOut,minValueOut,maxValueOut,centerValueOut,typeOut,faderScalingOut", "[BR] Get envelope properties for the envelope object allocated with <a href=\"#BR_EnvAlloc\">BR_EnvAlloc</a>.\n\nactive: true if envelope is active\nvisible: true if envelope is visible\narmed: true if envelope is armed\ninLane: true if envelope has it's own envelope lane\nlaneHeight: envelope lane override height. 0 for none, otherwise size in pixels\ndefaultShape: default point shape: 0->Linear, 1->Square, 2->Slow start/end, 3->Fast start, 4->Fast end, 5->Bezier\nminValue: minimum envelope value\nmaxValue: maximum envelope value\ntype: envelope type: 0->Volume, 1->Volume (Pre-FX), 2->Pan, 3->Pan (Pre-FX), 4->Width, 5->Width (Pre-FX), 6->Mute, 7->Pitch, 8->Playrate, 9->Tempo map, 10->Parameter\nfaderScaling: true if envelope uses fader scaling", },
	{ APIFUNC(BR_EnvSetPoint), "bool", "BR_Envelope*,int,double,double,int,bool,double", "envelope,id,position,value,shape,selected,bezier", "[BR] Set envelope point by id (zero-based) in the envelope object allocated with <a href=\"#BR_EnvAlloc\">BR_EnvAlloc</a>. To create point instead, pass id = -1. Note that if new point is inserted or existing point's time position is changed, points won't automatically get sorted. To do that, see BR_EnvSortPoints.\nReturns true on success.", },
	{ APIFUNC(BR_EnvSetProperties), "void", "BR_Envelope*,bool,bool,bool,bool,int,int,bool", "envelope,active,visible,armed,inLane,laneHeight,defaultShape,faderScaling", "[BR] Set envelope properties for the envelope object allocated with <a href=\"#BR_EnvAlloc\">BR_EnvAlloc</a>. For parameter description see BR_EnvGetProperties.", },
	{ APIFUNC(BR_EnvSortPoints), "void", "BR_Envelope*", "envelope", "[BR] Sort envelope points by position. The only reason to call this is if sorted points are explicitly needed after editing them with <a href=\"#BR_EnvSetPoint\">BR_EnvSetPoint</a>. Note that you do not have to call this before doing <a href=\"#BR_EnvFree\">BR_EnvFree</a> since it does handle unsorted points too.", },
	{ APIFUNC(BR_EnvValueAtPos), "double", "BR_Envelope*,double", "envelope,position", "[BR] Get envelope value at time position for the envelope object allocated with <a href=\"#BR_EnvAlloc\">BR_EnvAlloc</a>.", },
	{ APIFUNC(BR_GetArrangeView), "void", "ReaProject*,double*,double*", "proj,startTimeOut,endTimeOut", "[BR] Get start and end time position of arrange view. To set arrange view instead, see BR_SetArrangeView", },
	{ APIFUNC(BR_GetClosestGridDivision), "double", "double", "position", "[BR] Get closest grid division to position. Note that this functions is different from <a href=\"#SnapToGrid\">SnapToGrid</a> in two regards. SnapToGrid() needs snap enabled to work and this one works always. Secondly, grid divisions are different from grid lines because some grid lines may be hidden due to zoom level - this function ignores grid line visibility and always searches for the closest grid division at given position. For more grid division functions, see <a href=\"#BR_GetNextGridDivision\">BR_GetNextGridDivision</a> and <a href=\"#BR_GetPrevGridDivision\">BR_GetPrevGridDivision</a>.", },
	{ APIFUNC(BR_GetCurrentTheme), "void", "char*,int,char*,int", "themePathOut,themePathOut_sz,themeNameOut,themeNameOut_sz", "[BR] Get current theme information. themePathOut is set to full theme path and themeNameOut is set to theme name excluding any path info and extension", },
	{ APIFUNC(BR_GetMediaItemByGUID), "MediaItem*", "ReaProject*,const char*", "proj,guidStringIn", "[BR] Get media item from GUID string. Note that the GUID must be enclosed in braces {}. To get item's GUID as a string, see BR_GetMediaItemGUID.", },
	{ APIFUNC(BR_GetMediaItemGUID), "void", "MediaItem*,char*,int", "item,guidStringOut,guidStringOut_sz", "[BR] Get media item GUID as a string (guidStringOut_sz should be at least 64). To get media item back from GUID string, see BR_GetMediaItemByGUID.", },
	{ APIFUNC(BR_GetMediaItemImageResource), "bool", "MediaItem*,char*,int,int*", "item,imageOut,imageOut_sz,imageFlagsOut", "[BR] Get currently loaded image resource and it's flags for a given item. Returns false if there is no image resource set. To set image resource, see BR_SetMediaItemImageResource.", },
	{ APIFUNC(BR_GetMediaItemTakeGUID), "void", "MediaItem_Take*,char*,int", "take,guidStringOut,guidStringOut_sz", "[BR] Get media item take GUID as a string (guidStringOut_sz should be at least 64). To get take from GUID string, see SNM_GetMediaItemTakeByGUID.", },
	{ APIFUNC(BR_GetMediaSourceProperties), "bool", "MediaItem_Take*,bool*,double*,double*,double*,bool*", "take,sectionOut,startOut,lengthOut,fadeOut,reverseOut", "[BR] Get take media source properties as they appear in <i>Item properties</i>. Returns false if take can't have them (MIDI items etc.).\nTo set source properties, see BR_SetMediaSourceProperties." },
	{ APIFUNC(BR_GetMediaTrackByGUID), "MediaTrack*", "ReaProject*,const char*", "proj,guidStringIn", "[BR] Get media track from GUID string. Note that the GUID must be enclosed in braces {}. To get track's GUID as a string, see BR_GetMediaTrackGUID.", },
	{ APIFUNC(BR_GetMediaTrackFreezeCount), "int", "MediaTrack*", "track", "[BR] Get media track freeze count (if track isn't frozen at all, returns 0).", },
	{ APIFUNC(BR_GetMediaTrackGUID), "void", "MediaTrack*,char*,int", "track,guidStringOut,guidStringOut_sz", "[BR] Get media track GUID as a string (guidStringOut_sz should be at least 64). To get media track back from GUID string, see BR_GetMediaTrackByGUID.", },
	{ APIFUNC(BR_GetMediaTrackLayouts), "void", "MediaTrack*,char*,int,char*,int", "track,mcpLayoutNameOut,mcpLayoutNameOut_sz,tcpLayoutNameOut,tcpLayoutNameOut_sz", "[BR] Get media track layouts for MCP and TCP. Empty string (\"\") means that layout is set to the default layout. To set media track layouts, see BR_SetMediaTrackLayouts. Requires REAPER v5.02+.", },
	{ APIFUNC(BR_GetMediaTrackSendInfo_Envelope), "TrackEnvelope*", "MediaTrack*,int,int,int", "track,category,sendidx,envelopeType", "[BR] Get track envelope for send/receive/hardware output.\n\ncategory is <0 for receives, 0=sends, >0 for hardware outputs\nsendidx is zero-based (see GetTrackNumSends to count track sends/receives/hardware outputs)\nenvelopeType determines which envelope is returned (0=volume, 1=pan, 2=mute)\n\nNote: To get or set other send attributes, see <a href=\"#BR_GetSetTrackSendInfo\">BR_GetSetTrackSendInfo</a> and <a href=\"#BR_GetMediaTrackSendInfo_Track\">BR_GetMediaTrackSendInfo_Track</a>.", },
	{ APIFUNC(BR_GetMediaTrackSendInfo_Track), "MediaTrack*", "MediaTrack*,int,int,int", "track,category,sendidx,trackType", "[BR] Get source or destination media track for send/receive.\n\ncategory is <0 for receives, 0=sends\nsendidx is zero-based (see GetTrackNumSends to count track sends/receives)\ntrackType determines which track is returned (0=source track, 1=destination track)\n\nNote: To get or set other send attributes, see <a href=\"#BR_GetSetTrackSendInfo\">BR_GetSetTrackSendInfo</a> and <a href=\"#BR_GetMediaTrackSendInfo_Envelope\">BR_GetMediaTrackSendInfo_Envelope</a>.", },
	{ APIFUNC(BR_GetMidiSourceLenPPQ), "double", "MediaItem_Take*", "take", "[BR] Get MIDI take source length in PPQ. In case the take isn't MIDI, return value will be -1.", },
	{ APIFUNC(BR_GetMidiTakePoolGUID), "bool", "MediaItem_Take*,char*,int", "take,guidStringOut,guidStringOut_sz", "[BR] Get MIDI take pool GUID as a string (guidStringOut_sz should be at least 64). Returns true if take is pooled.", },
	{ APIFUNC(BR_GetMidiTakeTempoInfo), "bool", "MediaItem_Take*,bool*,double*,int*,int*", "take,ignoreProjTempoOut,bpmOut,numOut,denOut", "[BR] Get \"ignore project tempo\" information for MIDI take. Returns true if take can ignore project tempo (no matter if it's actually ignored), otherwise false.", },
	{ APIFUNC(BR_GetMouseCursorContext), "void", "char*,int,char*,int,char*,int", "windowOut,windowOut_sz,segmentOut,segmentOut_sz,detailsOut,detailsOut_sz", BR_MOUSE_REASCRIPT_DESC, },
	{ APIFUNC(BR_GetMouseCursorContext_Envelope), "TrackEnvelope*", "bool*", "takeEnvelopeOut", "[BR] Returns envelope that was captured with the last call to <a href=\"#BR_GetMouseCursorContext\">BR_GetMouseCursorContext</a>. In case the envelope belongs to take, takeEnvelope will be true.", },
	{ APIFUNC(BR_GetMouseCursorContext_Item), "MediaItem*", "", "", "[BR] Returns item under mouse cursor that was captured with the last call to <a href=\"#BR_GetMouseCursorContext\">BR_GetMouseCursorContext</a>. Note that the function will return item even if mouse cursor is over some other track lane element like stretch marker or envelope. This enables for easier identification of items when you want to ignore elements within the item."},
	{ APIFUNC(BR_GetMouseCursorContext_MIDI), "HWND", "bool*,int*,int*,int*,int*", "inlineEditorOut,noteRowOut,ccLaneOut,ccLaneValOut,ccLaneIdOut", "[BR] Returns midi editor under mouse cursor that was captured with the last call to <a href=\"#BR_GetMouseCursorContext\">BR_GetMouseCursorContext</a>.\n\ninlineEditor: if mouse was captured in inline MIDI editor, this will be true (consequentially, returned MIDI editor will be NULL)\nnoteRow: note row or piano key under mouse cursor (0-127)\nccLane: CC lane under mouse cursor (CC0-127=CC, 0x100|(0-31)=14-bit CC, 0x200=velocity, 0x201=pitch, 0x202=program, 0x203=channel pressure, 0x204=bank/program select, 0x205=text, 0x206=sysex, 0x207=off velocity)\nccLaneVal: value in CC lane under mouse cursor (0-127 or 0-16383)\nccLaneId: lane position, counting from the top (0 based)\n\nNote: due to API limitations, if mouse is over inline MIDI editor with some note rows hidden, noteRow will be -1"},
	{ APIFUNC(BR_GetMouseCursorContext_Position), "double", "", "", "[BR] Returns project time position in arrange/ruler/midi editor that was captured with the last call to <a href=\"#BR_GetMouseCursorContext\">BR_GetMouseCursorContext</a>."},
	{ APIFUNC(BR_GetMouseCursorContext_StretchMarker), "int", "", "", "[BR] Returns id of a stretch marker under mouse cursor that was captured with the last call to <a href=\"#BR_GetMouseCursorContext\">BR_GetMouseCursorContext</a>."},
	{ APIFUNC(BR_GetMouseCursorContext_Take), "MediaItem_Take*", "", "", "[BR] Returns take under mouse cursor that was captured with the last call to <a href=\"#BR_GetMouseCursorContext\">BR_GetMouseCursorContext</a>."},
	{ APIFUNC(BR_GetMouseCursorContext_Track), "MediaTrack*", "", "", "[BR] Returns track under mouse cursor that was captured with the last call to <a href=\"#BR_GetMouseCursorContext\">BR_GetMouseCursorContext</a>."},
	{ APIFUNC(BR_GetNextGridDivision), "double", "double", "position", "[BR] Get next grid division after the time position. For more grid divisions function, see <a href=\"#BR_GetClosestGridDivision\">BR_GetClosestGridDivision</a> and <a href=\"#BR_GetPrevGridDivision\">BR_GetPrevGridDivision</a>.", },
	{ APIFUNC(BR_GetPrevGridDivision), "double", "double", "position", "[BR] Get previous grid division before the time position. For more grid division functions, see <a href=\"#BR_GetClosestGridDivision\">BR_GetClosestGridDivision</a> and <a href=\"#BR_GetNextGridDivision\">BR_GetNextGridDivision</a>.", },
	{ APIFUNC(BR_GetSetTrackSendInfo), "double", "MediaTrack*,int,int,const char*,bool,double", "track,category,sendidx,parmname,setNewValue,newValue", "[BR] Get or set send attributes.\n\ncategory is <0 for receives, 0=sends, >0 for hardware outputs\nsendidx is zero-based (see GetTrackNumSends to count track sends/receives/hardware outputs)\nTo set attribute, pass setNewValue as true\n\nList of possible parameters:\nB_MUTE : send mute state (1.0 if muted, otherwise 0.0)\nB_PHASE : send phase state (1.0 if phase is inverted, otherwise 0.0)\nB_MONO : send mono state (1.0 if send is set to mono, otherwise 0.0)\nD_VOL : send volume (1.0=+0dB etc...)\nD_PAN : send pan (-1.0=100%L, 0=center, 1.0=100%R)\nD_PANLAW : send pan law (1.0=+0.0db, 0.5=-6dB, -1.0=project default etc...)\nI_SENDMODE : send mode (0=post-fader, 1=pre-fx, 2=post-fx(deprecated), 3=post-fx)\nI_SRCCHAN : audio source starting channel index or -1 if audio send is disabled (&1024=mono...note that in that case, when reading index, you should do (index XOR 1024) to get starting channel index)\nI_DSTCHAN : audio destination starting channel index (&1024=mono (and in case of hardware output &512=rearoute)...note that in that case, when reading index, you should do (index XOR (1024 OR 512)) to get starting channel index)\nI_MIDI_SRCCHAN : source MIDI channel, -1 if MIDI send is disabled (0=all, 1-16)\nI_MIDI_DSTCHAN : destination MIDI channel, -1 if MIDI send is disabled (0=original, 1-16)\nI_MIDI_SRCBUS : source MIDI bus, -1 if MIDI send is disabled (0=all, otherwise bus index)\nI_MIDI_DSTBUS : receive MIDI bus, -1 if MIDI send is disabled (0=all, otherwise bus index)\nI_MIDI_LINK_VOLPAN : link volume/pan controls to MIDI\n\nNote: To get or set other send attributes, see <a href=\"#BR_GetMediaTrackSendInfo_Envelope\">BR_GetMediaTrackSendInfo_Envelope</a> and <a href=\"#BR_GetMediaTrackSendInfo_Track\">BR_GetMediaTrackSendInfo_Track</a>.", },
	{ APIFUNC(BR_GetTakeFXCount), "int", "MediaItem_Take*", "take", "[BR] Returns FX count for supplied take", },
	{ APIFUNC(BR_IsTakeMidi), "bool", "MediaItem_Take*,bool*", "take,inProjectMidiOut", "[BR] Check if take is MIDI take, in case MIDI take is in-project MIDI source data, inProjectMidiOut will be true, otherwise false.", },
	{ APIFUNC(BR_ItemAtMouseCursor), "MediaItem*", "double*", "positionOut", "[BR] Get media item under mouse cursor. Position is mouse cursor position in arrange.", },
	{ APIFUNC(BR_MIDI_CCLaneRemove), "bool", "HWND,int", "midiEditor,laneId", "[BR] Remove CC lane in midi editor. Returns true on success", },
	{ APIFUNC(BR_MIDI_CCLaneReplace), "bool", "HWND,int,int", "midiEditor,laneId,newCC", "[BR] Replace CC lane in midi editor. Returns true on success.\nValid CC lanes: CC0-127=CC, 0x100|(0-31)=14-bit CC, 0x200=velocity, 0x201=pitch, 0x202=program, 0x203=channel pressure, 0x204=bank/program select, 0x205=text, 0x206=sysex, 0x207", },
	{ APIFUNC(BR_PositionAtMouseCursor), "double", "bool", "checkRuler", "[BR] Get position at mouse cursor. To check ruler along with arrange, pass checkRuler=true. Returns -1 if cursor is not over arrange/ruler.", },
	{ APIFUNC(BR_SetArrangeView), "void", "ReaProject*,double,double", "proj,startTime,endTime", "[BR] Set start and end time position of arrange view. To get arrange view instead, see BR_GetArrangeView", },
	{ APIFUNC(BR_SetItemEdges), "bool", "MediaItem*,double,double", "item,startTime,endTime", "[BR] Set item start and end edges' position - returns true in case of any changes", },
	{ APIFUNC(BR_SetMediaItemImageResource), "void", "MediaItem*,const char*,int", "item,imageIn,imageFlags", "[BR] Set image resource and it's flags for a given item. To clear current image resource, pass imageIn as "". To get image resource, see BR_GetMediaItemImageResource.", },
	{ APIFUNC(BR_SetMediaSourceProperties), "bool", "MediaItem_Take*,bool,double,double,double,bool", "take,section,start,length,fade,reverse", "[BR] Set take media source properties. Returns false if take can't have them (MIDI items etc.). Section parameters have to be valid only when passing section=true.\nTo get source properties, see BR_GetMediaSourceProperties." },
	{ APIFUNC(BR_SetMediaTrackLayouts), "bool", "MediaTrack*,const char*,const char*", "track,mcpLayoutNameIn,tcpLayoutNameIn", "[BR] Set media track layouts for MCP and TCP. To set default layout, pass empty string (\"\") as layout name. In case layouts were successfully set, returns true (if layouts are already set to supplied layout names, it will return false since no changes were made).\nTo get media track layouts, see BR_GetMediaTrackLayouts. Requires REAPER v5.02+.", },
	{ APIFUNC(BR_SetMidiTakeTempoInfo), "bool", "MediaItem_Take*,bool,double,int,int", "take,ignoreProjTempo,bpm,num,den", "[BR] Set \"ignore project tempo\" information for MIDI take. Returns true in case the take was successfully updated.", },
	{ APIFUNC(BR_SetTakeSourceFromFile), "bool", "MediaItem_Take*,const char*,bool", "take,filenameIn,inProjectData", "[BR] Set new take source from file. To import MIDI file as in-project source data pass inProjectData=true. Returns false if failed.\nAny take source properties from the previous source will be lost - to preserve them, see BR_SetTakeSourceFromFile2.\nNote: To set source from existing take, see <a href=\"#SNM_GetSetSourceState2\">SNM_GetSetSourceState2</a>.", },
	{ APIFUNC(BR_SetTakeSourceFromFile2), "bool", "MediaItem_Take*,const char*,bool,bool", "take,filenameIn,inProjectData,keepSourceProperties", "[BR] Differs from <a href=\"#BR_SetTakeSourceFromFile\">BR_SetTakeSourceFromFile</a> only that it can also preserve existing take media source properties.", },
	{ APIFUNC(BR_TakeAtMouseCursor), "MediaItem_Take*", "double*", "positionOut", "[BR] Get take under mouse cursor. Position is mouse cursor position in arrange.", },
	{ APIFUNC(BR_TrackAtMouseCursor), "MediaTrack*", "int*,double*", "contextOut,positionOut", "[BR] Get track under mouse cursor.\nContext signifies where the track was found: 0 = TCP, 1 = MCP, 2 = Arrange.\nPosition will hold mouse cursor position in arrange if applicable.", },
	{ APIFUNC(BR_TrackFX_GetFXModuleName), "bool", "MediaTrack*,int,char*,int", "track,fx, nameOut, nameOutSz", "[BR] Get the exact name (like effect.dll, effect.vst3, etc...) of an FX.", },
	{ APIFUNC(BR_Win32_GetPrivateProfileString), "int", "const char*,const char*,const char*,const char*,char*,int", "sectionName,keyName,defaultString,filePath,stringOut,stringOut_sz", "[BR] Equivalent to win32 API GetPrivateProfileString(). For example, you can use this to get values from REAPER.ini", },
	{ APIFUNC(BR_Win32_ShellExecute), "int", "const char*,const char*,const char*,const char*,int", "operation,file,parameters,directory,showFlags", "[BR] Equivalent to win32 API ShellExecute() with HWND set to main window", },
	{ APIFUNC(BR_Win32_WritePrivateProfileString), "bool", "const char*,const char*,const char*,const char*", "sectionName,keyName,value,filePath", "[BR] Equivalent to win32 API WritePrivateProfileString(). For example, you can use this to write to REAPER.ini", },

	{ APIFUNC(ULT_GetMediaItemNote), "const char*", "MediaItem*", "item", "[ULT] Get item notes.", },
	{ APIFUNC(ULT_SetMediaItemNote), "void", "MediaItem*,const char*", "item,note", "[ULT] Set item notes.", },

	{ NULL, } // denote end of table
};

///////////////////////////////////////////////////////////////////////////////

// register exported functions
bool RegisterExportedFuncs(reaper_plugin_info_t* _rec)
{
	bool ok = (_rec!=NULL);
	int i=-1;
	while (ok && g_apidefs[++i].func)
	{
		ok &= (_rec->Register(g_apidefs[i].regkey_func, g_apidefs[i].func) != 0);
		if (g_apidefs[i].regkey_vararg && g_apidefs[i].func_vararg)
		{
			ok &= (_rec->Register(g_apidefs[i].regkey_vararg, g_apidefs[i].func_vararg) != 0);
		}
	}
	return ok;
}

// unregister exported functions
void UnregisterExportedFuncs()
{
	char tmp[512];
	int i=-1;
	while (g_apidefs[++i].func)
	{
		_snprintf(tmp, sizeof(tmp), "-%s", g_apidefs[i].regkey_func);
		plugin_register(tmp, g_apidefs[i].func);
	}
}

// register exported function definitions + help text for the reaper api header and html documentation
bool RegisterExportedAPI(reaper_plugin_info_t* _rec)
{
	bool ok = (_rec!=NULL);
	int i=-1;
	char tmp[8*1024];
	while (ok && g_apidefs[++i].func)
	{
		if (g_apidefs[i].regkey_def)
		{
			memset(tmp, 0, sizeof(tmp));
			_snprintf(tmp, sizeof(tmp), "%s\r%s\r%s\r%s", g_apidefs[i].ret_val, g_apidefs[i].parm_types, g_apidefs[i].parm_names, g_apidefs[i].help);
			char* p = g_apidefs[i].dyn_def = _strdup(tmp);
			while (*p) { if (*p=='\r') *p='\0'; p++; }
			ok &= (_rec->Register(g_apidefs[i].regkey_def, g_apidefs[i].dyn_def) != 0);
		}
	}
	return ok;
}