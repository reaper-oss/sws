/******************************************************************************
/ SnM_Misc.cpp
/
/ Copyright (c) 2009 and later Jeffos
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

#include "SnM.h"
#include "SnM_Chunk.h"
#include "SnM_Item.h"
#include "SnM_Misc.h"
#include "SnM_Track.h"
#include "SnM_Util.h"
#ifdef _SNM_HOST_AW
#  include "../Misc/Adam.h"
#endif

#include <WDL/localize/localize.h>

#include <taglib/tag.h>
#include <taglib/fileref.h>

///////////////////////////////////////////////////////////////////////////////
// Reascript export, funcs made dumb-proof!
///////////////////////////////////////////////////////////////////////////////

WDL_PtrList_DOD<WDL_FastString> g_script_strs; // just to validate function parameters

WDL_FastString* SNM_CreateFastString(const char* _str) {
	return g_script_strs.Add(new WDL_FastString(_str));
}

void SNM_DeleteFastString(WDL_FastString* _str) { 
  if (_str) g_script_strs.Delete(g_script_strs.Find(_str), true);
}

const char* SNM_GetFastString(WDL_FastString* _str) {
	return _str && g_script_strs.Find(_str)>=0 ? _str->Get() : "";
}

int SNM_GetFastStringLength(WDL_FastString* _str) {
	return _str && g_script_strs.Find(_str)>=0 ? _str->GetLength() : 0;
}

WDL_FastString* SNM_SetFastString(WDL_FastString* _str, const char* _newStr)
{
	if (_str && g_script_strs.Find(_str)>=0)
	{
		_str->Set(_newStr?_newStr:"");
		return _str;
	}
	return NULL;
}

MediaItem_Take* SNM_GetMediaItemTakeByGUID(ReaProject* _project, const char* _guid)
{
	if (_guid && *_guid)
	{
		GUID g;
		stringToGuid(_guid, &g);
		return GetMediaItemTakeByGUID(_project, &g);
	}
	return NULL;
}

bool SNM_GetSourceType(MediaItem_Take* _tk, WDL_FastString* _type)
{
	if (_tk && _type && g_script_strs.Find(_type)>=0)
	{
		if (PCM_source* source = GetMediaItemTake_Source(_tk))
		{
			_type->Set(source->GetType());
			return true;
		}
	}
	return false;
}

// note: PCM_source.Load/SaveState() won't always work
//       e.g. getting an empty take src, turning wav src into midi, etc..
bool SNM_GetSetSourceState(MediaItem* _item, int _takeIdx, WDL_FastString* _state, bool _setnewvalue)
{
	bool ok = false;
	if (_item && _state && g_script_strs.Find(_state)>=0)
	{
		if (_takeIdx<0)
			_takeIdx = *(int*)GetSetMediaItemInfo(_item, "I_CURTAKE", NULL);

		int tkPos, tklen;
		WDL_FastString takeChunk;
		SNM_TakeParserPatcher p(_item, CountTakes(_item));
		if (p.GetTakeChunk(_takeIdx, &takeChunk, &tkPos, &tklen))
		{
			SNM_ChunkParserPatcher ptk(&takeChunk, false);

			// set
			if (_setnewvalue)
			{
				// standard case: a source is defined
				if (ptk.ReplaceSubChunk("SOURCE", 1, 0, _state->Get())) // no break keyword here: we're already at the end of the item..
					ok = p.ReplaceTake(tkPos, tklen, ptk.GetChunk());
				// replacing an empty take
				else
				{
					WDL_FastString newTkChunk("TAKE\n");
					newTkChunk.Append(_state);
					ok = p.ReplaceTake(tkPos, tklen, &newTkChunk);
				}
			}
			// get
			else
			{
				if (ptk.GetSubChunk("SOURCE", 1, 0, _state)<0)
					_state->Set(""); // empty take
				ok = true;
			}
		}
	}
	return ok;
}

bool SNM_GetSetSourceState2(MediaItem_Take* _tk, WDL_FastString* _state, bool _setnewvalue)
{
	if (_tk && _state && g_script_strs.Find(_state)>=0)
	{
		if (MediaItem* item = GetMediaItemTake_Item(_tk))
		{
			int tkIdx = GetTakeIndex(item, _tk);
			if (tkIdx >= 0)
				return SNM_GetSetSourceState(item, tkIdx, _state, _setnewvalue);
		}
	}
	return false;
}

bool SNM_GetSetObjectState(void* _obj, WDL_FastString* _state, bool _setnewvalue, bool _minstate)
{
	bool ok = false;
	if (_state && g_script_strs.Find(_state)>=0 && (ValidatePtr(_obj, "MediaTrack*") || ValidatePtr(_obj, "MediaItem*") || ValidatePtr(_obj, "TrackEnvelope*")))
	{
		int fxstate = SNM_PreObjectState(_setnewvalue ? _state : NULL, _minstate);
		char* p = GetSetObjectState(_obj, _setnewvalue ? _state->Get() : NULL);
		if (_setnewvalue)
		{
			ok = (p==NULL);
		}
		else if (p)
		{
			_state->Set(p);
			FreeHeapPtr((void*)p);
			ok = true;
		}
		SNM_PostObjectState(fxstate);
	}
	return ok;
}

// http://github.com/reaper-oss/sws/issues/476
// used to override the old SetProjectMarker3() which cannot set empty names "", but SetProjectMarker4() can do it now
// (keep SNM_SetProjectMarker() around for scripts that rely on it though...)
bool SNM_SetProjectMarker(ReaProject* _proj, int _num, bool _isrgn, double _pos, double _rgnend, const char* _name, int _color)
{
	return SetProjectMarker4(_proj, _num, _isrgn, _pos, _rgnend, _name, _color, _name && !*_name ? 1 : 0);
}

// just a wrapper func: reascripts cannot handle the char** param of EnumProjectMarkers()
bool SNM_GetProjectMarkerName(ReaProject* _proj, int _num, bool _isrgn, WDL_FastString* _name)
{
	if (_name && g_script_strs.Find(_name)>=0)
	{
		int x=0, num; const char* name; bool isrgn;
		while ((x = EnumProjectMarkers3(_proj, x, &isrgn, NULL, NULL, &name, &num, NULL)))
    {
			if (num==_num && isrgn==_isrgn)
      {
				_name->Set(name);
				return true;
			}
    }
	}
	return false;
}

int SNM_GetIntConfigVar(const char *varName, const int fallback) {
	if (!strcmp(varName, "vzoom2")) {
		// make older scripts compatible with REAPER v6.73+devXXXX
		// REAPER and SWS write to both vzoom3+2, but other extensions might not
		if (ConfigVar<float> vzoom3 { "vzoom3" })
			return static_cast<int>(*vzoom3);
	}

	if (ConfigVar<int> cv{varName})
		return *cv;
	else if(ConfigVar<char> cv{varName})
		return *cv;
	else
		return fallback;
}

bool SNM_SetIntConfigVar(const char *varName, const int newValue) {
	if (!strcmp(varName, "vzoom2")) // set both vzoom3 and vzoom2 (below)
		ConfigVar<float>("vzoom3").try_set(static_cast<float>(newValue));
	if (ConfigVar<int>(varName).try_set(newValue))
		return true;
	if (ConfigVar<char> cv{varName}) {
		if (newValue > std::numeric_limits<char>::max() || newValue < std::numeric_limits<char>::min())
			return false;

		*cv = newValue;
		return true;
	}

	return false;
}

double SNM_GetDoubleConfigVar(const char *varName, const double fallback) {
	if (ConfigVar<double> cv{varName})
		return *cv;
	else if (ConfigVar<float> cv{varName})
		return *cv;
	else
		return fallback;
}

bool SNM_SetDoubleConfigVar(const char *varName, const double newValue) {
	if (!strcmp(varName, "vzoom3")) // compatibility with vzoom3-unaware extensions
		*ConfigVar<int>("vzoom2") = static_cast<int>(newValue);
	if (ConfigVar<double> cv{varName})
		*cv = newValue;
	else if (ConfigVar<float> cv{varName}) {
		if (newValue > std::numeric_limits<float>::max() || newValue < std::numeric_limits<float>::min())
			return false;

		*cv = static_cast<float>(newValue);
	}
	else
		return false;

	return true;
}

bool SNM_GetLongConfigVar(const char *varName, int *highOut, int *lowOut) {
	const ConfigVar<long long> var(varName);
	if (!var)
		return false;

	if (highOut)
		*highOut = *var >> 32;
	if (lowOut)
		*lowOut = *var & 0xFFFFFFFF;

	return true;
}

bool SNM_SetLongConfigVar(const char *varName, const int newHighValue, const int newLowValue) {
	const auto newValue = (static_cast<long long>(newHighValue) << 32) | (newLowValue & 0xFFFFFFFF);
	return ConfigVar<long long>(varName).try_set(newValue);
}

bool SNM_SetStringConfigVar(const char *varName, const char *newValue) {
  int size = 0;
  char *data = static_cast<char *>(get_config_var(varName, &size));

  if(!data || !newValue || static_cast<size_t>(size) < strlen(newValue) + 1)
    return false;

  snprintf(data, size, "%s", newValue);
  return true;
}

// host some funcs from Ultraschall, https://github.com/Ultraschall
const char* ULT_GetMediaItemNote(MediaItem* _item) {
		return _item ? (const char*)GetSetMediaItemInfo(_item, "P_NOTES", NULL): "";
}

void ULT_SetMediaItemNote(MediaItem* _item, const char* _str) {
	if (_item) GetSetMediaItemInfo(_item, "P_NOTES", (char*)_str);
}

bool SNM_ReadMediaFileTag(const char *fn, const char* tag, char* tagval, int tagval_sz)
{
  if (!fn || !*fn || !tagval || tagval_sz<=0) return false;
  *tagval=0;

  TagLib::FileRef f(win32::widen(fn).c_str(), false);

  if (!f.isNull() && !f.tag()->isEmpty())
  {
    TagLib::String s;
    if (!_stricmp(tag, "artist")) s=f.tag()->artist();
    else if (!_stricmp(tag, "album")) s=f.tag()->album();
    else if (!_stricmp(tag, "genre")) s=f.tag()->genre();
    else if (!_stricmp(tag, "comment")) s=f.tag()->comment();
    else if (!_stricmp(tag, "title")) s=f.tag()->title();
    if (s.length())
    {
      const char *p=s.toCString(true);
      if (strcmp(p,"0")) lstrcpyn(tagval, p, tagval_sz); // must be a taglib bug...
    }
    else
    {
      if (!_stricmp(tag, "year") && f.tag()->year()) snprintf(tagval, tagval_sz, "%u", f.tag()->year());
      else if (!_stricmp(tag, "track") && f.tag()->track()) snprintf(tagval, tagval_sz, "%u", f.tag()->track());
    }
  }

  return !!*tagval;
}

bool SNM_TagMediaFile(const char *fn, const char* tag, const char* tagval)
{
  if (!fn || !*fn || !tagval || !tag) return false;

  TagLib::FileRef f(win32::widen(fn).c_str(), false);

  bool didsmthg=false;
  if (!f.isNull())
  {
    TagLib::String s(tagval, TagLib::String::UTF8);
    if (!_stricmp(tag, "artist")) { f.tag()->setArtist(s); didsmthg=true; }
    else if (!_stricmp(tag, "album")) { f.tag()->setAlbum(s); didsmthg=true; }
    else if (!_stricmp(tag, "genre")) { f.tag()->setGenre(s); didsmthg=true; }
    else if (!_stricmp(tag, "comment") || !_stricmp(tag, "desc")) { f.tag()->setComment(s); didsmthg=true; }
    else if (!_stricmp(tag, "title")) { f.tag()->setTitle(s); didsmthg=true; }
    else if (!_stricmp(tag, "year"))
    {
      int val=atoi(tagval);
      if (val>0 || !*tagval) { f.tag()->setYear(val); didsmthg=true; }
    }
    else if (!_stricmp(tag, "track"))
    {
      int val=atoi(tagval);
      if (val>0 || !*tagval) { f.tag()->setTrack(val); didsmthg=true; }
    }
    if (didsmthg) f.save();
  }

  return didsmthg;
}


///////////////////////////////////////////////////////////////////////////////
// Toolbars auto refresh option
///////////////////////////////////////////////////////////////////////////////

DWORD g_toolbarRefreshTime=0, g_offscreenItemsRefreshTime=0;  // really approx (updated on timer)

void EnableToolbarsAutoRefesh(COMMAND_T* _ct) {
	g_SNM_ToolbarRefresh = !g_SNM_ToolbarRefresh;
}

int IsToolbarsAutoRefeshEnabled(COMMAND_T* _ct) {
	return g_SNM_ToolbarRefresh;
}

void SNM_RefreshToolbars()
{
	constexpr int (*watchStateGetters[])(COMMAND_T *)
	{
		&HasOffscreenSelItems, // offscreen item sel. buttons
		// &WriteEnvExists, // write automation button, handled by toggleActionHook
#ifdef _SNM_HOST_AW
		&IsProjectTimebase,   // UpdateTimebaseToolbar
		&IsSelTracksTimebase, // UpdateTrackTimebaseToolbar
		&IsSelItemsTimebase,  // UpdateItemTimebaseToolbar

		// UpdateGridToolbar
		// &IsGridTriplet,  // handled by toggleActionHook
		// &IsGridDotted,   // idem
		&IsGridSwing, // toggling the checkbox in Grid Settings does not trigger toggleActionHook
		// &IsClickUnmuted, // idem
		// &IsAWSetGridPreserveType, // idem
#endif
	};

	struct ToggleStateWatch { COMMAND_T *cmd; int stateCache; };
	static std::vector<ToggleStateWatch> watchs;

	if (watchs.empty())
	{
		int i = 0;
		while (COMMAND_T **cmdPtr = SWSGetCommand(i++))
		{
			COMMAND_T *cmd = *cmdPtr;
			for (const auto getEnabled : watchStateGetters)
			{
				if (getEnabled == cmd->getEnabled)
				{
					watchs.push_back({ cmd, getEnabled(cmd) });
					break;
				}
			}
		}
		return;
	}

	for (ToggleStateWatch &watch : watchs)
	{
		const int state = watch.cmd->getEnabled(watch.cmd);
		if (state != watch.stateCache)
		{
			watch.stateCache = state;
			RefreshToolbar(watch.cmd->cmdId);
		}
	}
}

// polled via SNM_CSurfRun()
void AutoRefreshToolbarRun()
{
	if (g_SNM_ToolbarRefresh)
	{
		if (GetTickCount() > g_offscreenItemsRefreshTime) {
			g_offscreenItemsRefreshTime = GetTickCount() + SNM_OFFSCREEN_UPDATE_FREQ;
			RefreshOffscreenItems();
		}
		if (GetTickCount() > g_toolbarRefreshTime) {
			g_toolbarRefreshTime = GetTickCount() + g_SNM_ToolbarRefreshFreq; // custom freq (from S&M.ini)
			SNM_RefreshToolbars();
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
// Misc actions / helpers
///////////////////////////////////////////////////////////////////////////////

//increase/decrease the metronome volume
void ChangeMetronomeVolume(COMMAND_T* _ct) {
	KBD_OnMainActionEx(999, 0x40+(int)_ct->user, -1, 2, GetMainHwnd(), NULL);
}

void WinWaitForEvent(DWORD _event, DWORD _timeOut=500, DWORD _minReTrigger=500)
{
#ifdef _WIN32
	static DWORD sWaitTime = 0;
//	if ((GetTickCount() - sWaitTime) > _minReTrigger)
	{
		sWaitTime = GetTickCount();
		while((GetTickCount() - sWaitTime) < _timeOut) // for safety
		{
			MSG msg;
			if(PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE))
			{
				// new message to be processed
				TranslateMessage(&msg);
				DispatchMessage(&msg);
				if(msg.message == _event)
					break;
			}
		}
	}
#endif
}

// http://forum.cockos.com/showthread.php?p=612065
void SimulateMouseClick(COMMAND_T* _ct)
{
	POINT p;
	GetCursorPos(&p);
	mouse_event(MOUSEEVENTF_LEFTDOWN, p.x, p.y, 0, 0);
	mouse_event(MOUSEEVENTF_LEFTUP, p.x, p.y, 0, 0);
	WinWaitForEvent(WM_LBUTTONUP);
}

// dump actions or the wiki ALR summary
// see http://forum.cockos.com/showthread.php?t=61929 and http://wiki.cockos.com/wiki/index.php/Action_List_Reference
// _type: &1 ALR wiki (txt dump otherwise), &2=deprecated, &4=native, &8=SWS, &16=user macros/script/cycle actions/reaconsole actions
bool DumpActionList(int _type, const char* _title, const char* _lineFormat, const char* _heading, const char* _ending)
{
  char fn[SNM_MAX_PATH];
  if (!BrowseForSaveFile(_title, GetResourcePath(), "ActionList.txt", SNM_TXT_EXT_LIST, fn, sizeof(fn)))
    return false;
  
  int nbWrote=0;
  if (FILE* f = fopenUTF8(fn, "w"))
  {
    if (_heading)
      fprintf(f, "%s", _heading);
    
    char custId[SNM_MAX_ACTION_CUSTID_LEN];
    char sectionURL[SNM_MAX_SECTION_NAME_LEN]; 
    for (int sec=0; sec<SNM_NUM_MANAGED_SECTIONS; sec++)
    {
      if (sec==SNM_SEC_IDX_MAIN_ALT) continue; // skip "Main (alt recording)" section
      
      KbdSectionInfo* section = SectionFromUniqueID(SNM_GetActionSectionInfo(sec)->unique_id);
      if (!GetSectionURL((_type&1)==1, section, sectionURL, sizeof(sectionURL)))
      {
        continue;
      }
      
      int i=0;
      int cmdId;
      const char *cmdName;
      while((cmdId = kbd_enumerateActions(section, i++, &cmdName)))
      {
        *custId='\0';
        {
          const char *strid=ReverseNamedCommandLookup(cmdId);
          if (strid) snprintfStrict(custId, sizeof(custId), "_%s", strid);
          else snprintfStrict(custId, sizeof(custId), "%d", cmdId);
        }
        bool isCustom = IsMacroOrScript(cmdName);
        int isSws = IsSwsAction(cmdName);
        bool isSwsCustom = !IsLocalizableAction(custId);
        
        if (((_type&4) && !isSws && !isCustom && !isSwsCustom) || 
            ((_type&8) && isSws && !isCustom && !isSwsCustom) ||
            ((_type&16) && (isCustom||isSwsCustom)))
        {
          if (*custId)
          {
            fprintf(f, _lineFormat, sectionURL, custId, cmdName, custId);
            nbWrote++;
          }
        }
      }
    }
    
    if (_ending)
      fprintf(f, "%s", _ending); 
    
    fclose(f);
    
    WDL_FastString msg;
    msg.SetFormatted(SNM_MAX_PATH, nbWrote ? __LOCALIZE_VERFMT("Wrote %s","sws_mbox") : __LOCALIZE_VERFMT("No action wrote in %s!","sws_mbox"), fn);
    MessageBox(GetMainHwnd(), msg.Get(), _title, MB_OK);
    return true;
  }
  else
    MessageBox(GetMainHwnd(), __LOCALIZE("Dump failed: unable to write to file!","sws_mbox"), _title, MB_OK);
  return false;
}

void DumpWikiActionList(COMMAND_T* _ct)
{
	DumpActionList(((int)_ct->user)|1,
                 __LOCALIZE("S&M - Save ALR Wiki summary","sws_mbox"),
                 "|-\n| [[%s_%s|%s]] || %s\n",
                 "{| class=\"wikitable\"\n|-\n! Action name !! Cmd ID\n",
                 "|}\n");
}

void DumpActionList(COMMAND_T* _ct)
{
	DumpActionList((int)_ct->user,
                 __LOCALIZE("S&M - Dump action list","sws_mbox"),
                 "%s\t%s\t%s\n",
                 "Section\tId\tAction\n",
                 NULL);
}


