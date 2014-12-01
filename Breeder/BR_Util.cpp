/******************************************************************************
/ BR_Util.cpp
/
/ Copyright (c) 2013-2014 Dominik Martin Drzic
/ http://forum.cockos.com/member.php?u=27094
/ https://code.google.com/p/sws-extension
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
#include "BR_Util.h"
#include "BR.h"
#include "BR_EnvelopeUtil.h"
#include "BR_MidiUtil.h"
#include "../reaper/localize.h"
#include "../SnM/SnM.h"
#include "../SnM/SnM_Chunk.h"
#include "../SnM/SnM_Dlg.h"
#include "../SnM/SnM_Item.h"
#include "../SnM/SnM_Util.h"
#include "../../WDL/lice/lice.h"
#include "../../WDL/projectcontext.h"

/******************************************************************************
* Constants                                                                   *
******************************************************************************/
const int ITEM_LABEL_MIN_HEIGHT      = 28;
const int TCP_MASTER_GAP             = 5;
const int ENV_GAP                    = 4;    // bottom gap may seem like 3 when selected, but that
const int ENV_LINE_WIDTH             = 1;    // first pixel is used to "bold" selected envelope

const int TAKE_MIN_HEIGHT_COUNT      = 10;
const int TAKE_MIN_HEIGHT_HIGH       = 12;   // min height when take count <= TAKE_MIN_HEIGHT_COUNT
const int TAKE_MIN_HEIGHT_LOW        = 6;    // min height when take count >  TAKE_MIN_HEIGHT_COUNT

const int PROJ_CONTEXT_LINE          = 4096; // same length used by ProjectContext

/******************************************************************************
* Miscellaneous                                                               *
******************************************************************************/
vector<int> GetDigits (int val)
{
	int count = (int)(log10((float)val)) + 1;

	vector<int> digits;
	digits.resize(count);

	for (int i = count-1; i >= 0; --i)
	{
		digits[i] = GetLastDigit(val);
		val /= 10;
	}
	return digits;
}

set<int> GetAllMenuIds (HMENU hMenu)
{
	set<int> menuIds;

	int count = GetMenuItemCount(hMenu);
	for (int i = 0; i < count; ++i)
	{
		if (HMENU subMenu = GetSubMenu(hMenu, i))
		{
			set<int> subMenuIds = GetAllMenuIds(subMenu);
			menuIds.insert(subMenuIds.begin(), subMenuIds.end());
		}
		else
		{
			menuIds.insert(GetMenuItemID(hMenu, i));
		}
	}

	return menuIds;
}

int GetUnusedMenuId (HMENU hMenu)
{
	set<int> menuIds = GetAllMenuIds(hMenu);
	int unusedId = 0;

	if (menuIds.empty())
	{
		unusedId = 1;
	}
	else
	{
		int last = *menuIds.rbegin();
		if (last < (std::numeric_limits<int>::max)()) // () outside is due to max() macro!
		{
			if (last < 0)
				unusedId = 1;
			else
				unusedId = last + 1;
		}
		else
		{
			for (set<int>::iterator it = menuIds.begin(); it != menuIds.end(); ++it)
			{
				if (++it != menuIds.end())
				{
					int first = *it;
					int last  = *(++it);

					bool found = false;
					if (last > 0 && last - first > 1)
					{
						while (first != last)
						{
							++first;
							if (first > 0 && first != last)
							{
								unusedId = first;
								found = true;
								break;
							}
						}
					}
					if (found)
						break;
				}
			}
		}
	}

	return unusedId;
}

int RoundToInt (double val)
{
	if (val < 0)
		return (int)(val - 0.5);
	else
		return (int)(val + 0.5);
}

int TruncToInt (double val)
{
	return (int)val;
}

int GetBit (int val, int pos)
{
	return (val & 1 << pos) != 0;
}

int SetBit (int val, int pos, bool set)
{
	if (set) return SetBit(val, pos);
	else     return ClearBit(val, pos);
}

int SetBit (int val, int pos)
{
	return val | 1 << pos;
}

int ClearBit (int val, int pos)
{
	return val & ~(1 << pos);
}

int ToggleBit (int val, int pos)
{
	return val ^= 1 << pos;
}

int GetFirstDigit (int val)
{
	val = abs(val);
	while (val >= 10)
		val /= 10;
	return val;
}

int GetLastDigit (int val)
{
	return abs(val) % 10;
}

int BinaryToDecimal (const char* binaryString)
{
	int base10 = 0;
	for (size_t i = 0; i < strlen(binaryString); ++i)
	{
		int digit = binaryString[i] - '0';
		if (digit == 0 || digit == 1)
			base10 = base10 << 1 | digit;
	}
	return base10;
}

bool IsFraction (char* str, double& convertedFraction)
{
	int size = strlen(str);
	replace(str, str + size, ',', '.' );
	char* buf = strstr(str,"/");

	if (!buf)
	{
		convertedFraction = atof(str);
		_snprintfSafe(str, size + 1, "%g", convertedFraction);
		return false;
	}
	else
	{
		int num = atoi(str);
		int den = atoi(buf+1);
		_snprintfSafe(str, size + 1, "%d/%d", num, den);
		if (den != 0)
			convertedFraction = (double)num/(double)den;
		else
			convertedFraction = 0;
		return true;
	}
}

double AltAtof (char* str)
{
	replace(str, str+strlen(str), ',', '.' );
	return atof(str);
}

double Trunc (double val)
{
	return (val < 0) ? ceil(val) : floor(val);
}

double Round (double val)
{
	return (double)(int)(val + ((val < 0) ? (-0.5) : (0.5)));
}

double RoundToN (double val, double n)
{
	double shift = pow(10.0, n);
	return RoundToInt(val * shift) / shift;
}

double TranslateRange (double value, double oldMin, double oldMax, double newMin, double newMax)
{
	double oldRange = oldMax - oldMin;
	double newRange = newMax - newMin;
	double newValue = ((value - oldMin) * newRange / oldRange) + newMin;

	return SetToBounds(newValue, newMin, newMax);
}

void ReplaceAll (string& str, string oldStr, string newStr)
{
	if (oldStr.empty())
		return;

	size_t pos = 0, oldLen = oldStr.length(), newLen = newStr.length();
	while ((pos = str.find(oldStr, pos)) != std::string::npos)
	{
		str.replace(pos, oldLen, newStr);
		pos += newLen;
	}
}

void AppendLine (WDL_FastString& str, const char* line)
{
	str.Append(line);
	str.Append("\n");
}

/******************************************************************************
* General                                                                     *
******************************************************************************/
vector<double> GetProjectMarkers (bool timeSel, double delta /*=0*/)
{
	double tStart, tEnd;
	GetSet_LoopTimeRange2(NULL, false, false, &tStart, &tEnd, false);

	int i = 0;
	bool region;
	double pos;
	vector<double> markers;
	while (EnumProjectMarkers2(NULL, i, &region, &pos, NULL, NULL, NULL))
	{
		bool pRegion;
		double pPos;
		int previous = EnumProjectMarkers2(NULL, i-1, &pRegion, &pPos, NULL, NULL, NULL);
		if (!region)
		{
			// In certain corner cases involving regions, reaper will output the same marker more than
			// once. This should prevent those duplicate values from being written into the vector.
			// Note: this might have been fixed in 4.60, but it doesn't hurt to leave it...
			if (pos != pPos || (pos == pPos && pRegion) || !previous)
			{
				if (timeSel)
				{
					if (pos > tEnd + delta)
						break;
					if (pos >= tStart - delta)
						markers.push_back(pos);
				}
				else
					markers.push_back(pos);
			}
		}
		++i;
	}
	return markers;
}

WDL_FastString GetReaperMenuIniPath ()
{
	static WDL_FastString s_iniPath;
	if (s_iniPath.GetLength() == 0)
		s_iniPath.SetFormatted(SNM_MAX_PATH, "%s/reaper-menu.ini", GetResourcePath());
	return s_iniPath;
}

WDL_FastString FormatTime (double position, int mode /*=-1*/)
{
	WDL_FastString string;
	int projTimeMode; GetConfig("projtimemode", projTimeMode);

	if (mode == 1 || (mode == -1 && projTimeMode == 1))
	{
		char formatedPosition[128];
		format_timestr_pos(position, formatedPosition, sizeof(formatedPosition), 2);
		string.AppendFormatted(sizeof(formatedPosition), "%s", formatedPosition);

		format_timestr_pos(position, formatedPosition, sizeof(formatedPosition), 0);
		string.AppendFormatted(sizeof(formatedPosition), " / %s", formatedPosition);
	}
	else
	{
		char formatedPosition[128];
		format_timestr_pos(position, formatedPosition, sizeof(formatedPosition), mode);
		string.AppendFormatted(sizeof(formatedPosition), "%s", formatedPosition);
	}

	return string;
}

int GetEffectiveTakeId (MediaItem_Take* take, MediaItem* item, int id, int* effectiveTakeCount)
{
	MediaItem*      validItem = (item) ? (item) : (GetMediaItemTake_Item(take));
	MediaItem_Take* validTake = (take) ? (take) : (GetTake(validItem, id));

	int count = CountTakes(validItem);
	int effectiveCount = 0;
	int effectiveId = -1;

	// Empty takes not displayed
	int emptyLanes; GetConfig("takelanes", emptyLanes);
	if (GetBit(emptyLanes, 2))
	{
		for (int i = 0; i < count; ++i)
		{
			if (MediaItem_Take* currentTake = GetTake(validItem, i))
			{
				if (currentTake == validTake)
					effectiveId = effectiveCount;
				++effectiveCount;
			}
		}
	}
	// Empty takes displayed
	else
	{
		if (take)
		{
			for (int i = 0; i < count; ++i)
			{
				if (GetTake(validItem, i) == validTake)
				{
					effectiveId = i;
					break;
				}
			}
		}
		else
		{
			effectiveId = id;
		}

		effectiveCount = count;
	}

	WritePtr(effectiveTakeCount, effectiveCount);
	return effectiveId;
}

int GetEffectiveCompactLevel (MediaTrack* track)
{
	int compact = 0;
	MediaTrack* parent = track;
	while (parent = (MediaTrack*)GetSetMediaTrackInfo(parent, "P_PARTRACK", NULL))
	{
		int tmp = (int)GetMediaTrackInfo_Value(parent, "I_FOLDERCOMPACT");
		if (tmp > compact)
		{
			compact = tmp;
			if (compact == 2)
				break;
		}
	}
	return compact;
}

int FindClosestProjMarkerIndex (double position)
{
	int first = 0;
	int last  = CountProjectMarkers(NULL, NULL, NULL);
	if (last < 0)
		return -1;

	// Find previous marker/region
	while (first != last)
	{
		int mid = (first + last) /2;
		double currentPos; EnumProjectMarkers3(NULL, mid, NULL, &currentPos, NULL, NULL, NULL, NULL);

		if (currentPos < position) first = mid + 1;
		else                       last  = mid;
	}
	int prevId = first - ((first == 0) ? 0 : 1);

	// Make sure we get marker index, not region
	int id = prevId + 1; bool region; double prevPosition;
	while (EnumProjectMarkers3(NULL, --id, &region, &prevPosition, NULL, NULL, NULL, NULL))
	{
		if (!region)
			break;
	}
	if (id < 0)
		return -1;

	// Check against next marker
	int nextId = prevId;
	double nextPosition;
	while (EnumProjectMarkers3(NULL, ++nextId, &region, &nextPosition, NULL, NULL, NULL, NULL))
	{
		if (!region)
		{
			if (abs(nextPosition - position) < abs(position - prevPosition)) id = nextId;
			if (id >= CountProjectMarkers(NULL, NULL, NULL))                 id = -1;
			break;
		}
	}

	return id;
}

double EndOfProject (bool markers, bool regions)
{
	double projEnd = 0;
	int tracks = GetNumTracks();
	for (int i = 0; i < tracks; ++i)
	{
		MediaTrack* track = GetTrack(0, i);
		MediaItem* item = GetTrackMediaItem(track, GetTrackNumMediaItems(track) - 1);
		double itemEnd = GetMediaItemInfo_Value(item, "D_POSITION") + GetMediaItemInfo_Value(item, "D_LENGTH");
		if (itemEnd > projEnd)
			projEnd = itemEnd;
	}

	if (markers || regions)
	{
		bool region; double start, end; int i = 0;
		while (i = EnumProjectMarkers(i, &region, &start, &end, NULL, NULL))
		{
			if (regions)
				if (region && end > projEnd)
					projEnd = end;
			if (markers)
				if (!region && start > projEnd)
					projEnd = start;
		}
	}
	return projEnd;
}

double GetProjectSettingsTempo (int* num, int* den)
{
	int _den;
	TimeMap_GetTimeSigAtTime(NULL, -1, num, &_den, NULL);
	double bpm;
	GetProjectTimeSignature2(NULL, &bpm, NULL);

	WritePtr(den, _den);
	return bpm/_den*4;
}

double GetGridDivSafe ()
{
	double gridDiv;
	GetConfig("projgriddiv", gridDiv);
	if (gridDiv < MAX_GRID_DIV)
	{
		SetConfig("projgriddiv", MAX_GRID_DIV);
		return MAX_GRID_DIV;
	}
	else
		return gridDiv;
}

void InitTempoMap ()
{
	if (!CountTempoTimeSigMarkers(NULL))
	{
		PreventUIRefresh(1);
		bool master = TcpVis(GetMasterTrack(NULL));
		Main_OnCommand(41046, 0);              // Toggle show master tempo envelope
		Main_OnCommand(41046, 0);
		if (!master) Main_OnCommand(40075, 0); // Hide master if needed
		PreventUIRefresh(-1);
	}
}

void ScrollToTrackIfNotInArrange (MediaTrack* track)
{
	int offsetY;
	int height = GetTrackHeight(track, &offsetY);

	HWND hwnd = GetArrangeWnd();
	SCROLLINFO si = { sizeof(SCROLLINFO), };
	si.fMask = SIF_ALL;
	CoolSB_GetScrollInfo(hwnd, SB_VERT, &si);

	int trackEnd = offsetY + height;
	int pageEnd = si.nPos + (int)si.nPage + SCROLLBAR_W;

	if (offsetY < si.nPos || trackEnd > pageEnd)
	{
		si.nPos = offsetY;
		CoolSB_SetScrollInfo(hwnd, SB_VERT, &si, true);
		SendMessage(hwnd, WM_VSCROLL, si.nPos << 16 | SB_THUMBPOSITION, NULL);
	}
}

void StartPlayback (double position)
{
	double editCursor = GetCursorPositionEx(NULL);

	PreventUIRefresh(1);
	SetEditCurPos2(NULL, position, false, false);
	OnPlayButton();
	SetEditCurPos2(NULL, editCursor, false, false);
	PreventUIRefresh(-1);
}

void GetSetLastAdjustedSend (bool set, MediaTrack** track, int* sendId, int* type)
{
	static MediaTrack* lastTrack = NULL;
	static int         lastId    = -1;
	static int         lastType  = UNKNOWN;

	if (set)
	{
		lastTrack = *track;
		lastId    = *sendId;
		lastType  = *type;
	}
	else
	{
		WritePtr(track, lastTrack);
		WritePtr(sendId, lastId);
		WritePtr(type,   lastType);
	}
}

void GetSetFocus (bool set, HWND* hwnd, int* context)
{
	if (set)
	{
		if (context) // context first (it may refocus main window)
		{
			TrackEnvelope* envelope = GetSelectedEnvelope(NULL);
			SetCursorContext((*context == 2 && !envelope) ? 1 : *context, (*context == 2) ? envelope : NULL);
		}
		if (hwnd)
			SetFocus(*hwnd);
	}
	else
	{
		WritePtr(hwnd,    GetFocus());
		WritePtr(context, GetCursorContext2(true)); // last_valid_focus: example - select item and focus mixer. Pressing delete
	}                                               // tells us context is still 1, but GetCursorContext2(false) would return 0
}

void RefreshToolbarAlt (int cmd)
{
	if (GetToggleCommandState2(SectionFromUniqueID(32060), cmd) != -1 || GetToggleCommandState2(SectionFromUniqueID(32061), cmd) != -1)
	{
		int toggleState = GetToggleCommandState(cmd);
		BR_SetGetCommandHook2Reentrancy(true, true);

		MIDIEditor_LastFocused_OnCommand(cmd, false);
		if (GetToggleCommandState(cmd) != toggleState)
			MIDIEditor_LastFocused_OnCommand(cmd, false);

		BR_SetGetCommandHook2Reentrancy(true, false);
	}
	else
	{
		RefreshToolbar(cmd);
	}
}

bool IsPlaying ()
{
	return (GetPlayStateEx(NULL) & 1) == 1;
}

bool IsPaused ()
{
	return (GetPlayStateEx(NULL) & 2) == 2;
}

bool IsRecording ()
{
	return (GetPlayStateEx(NULL) & 4) == 4;
}

bool TcpVis (MediaTrack* track)
{
	if (track)
	{
		if ((GetMasterTrack(NULL) == track))
		{
			int master; GetConfig("showmaintrack", master);
			if (master == 0 || (int)GetMediaTrackInfo_Value(track, "I_WNDH") == 0)
				return false;
			else
				return true;
		}
		else
		{
			if (!GetMediaTrackInfo_Value(track, "B_SHOWINTCP") || !GetMediaTrackInfo_Value(track, "I_WNDH"))
				return false;
			else
				return true;
		}
	}
	else
		return false;
}

/******************************************************************************
* Items                                                                       *
******************************************************************************/
vector<MediaItem*> GetSelItems (MediaTrack* track)
{
	vector<MediaItem*> items;
	int count = CountTrackMediaItems(track);
	for (int i = 0; i < count ; ++i)
		if (*(bool*)GetSetMediaItemInfo(GetTrackMediaItem(track, i), "B_UISEL", NULL))
			items.push_back(GetTrackMediaItem(track, i));
	return items;
}

double GetSourceLengthPPQ (MediaItem_Take* take)
{
	if (take)
	{
		double itemStart    = GetMediaItemInfo_Value(GetMediaItemTake_Item(take), "D_POSITION");
		double takeOffset   = GetMediaItemTakeInfo_Value(take, "D_STARTOFFS");
		double sourceLength = GetMediaItemTake_Source(take)->GetLength();
		double startPPQ = MIDI_GetPPQPosFromProjTime(take, itemStart - takeOffset);
		double endPPQ   = MIDI_GetPPQPosFromProjTime(take, itemStart - takeOffset + sourceLength);

		return endPPQ - startPPQ;
	}
	return 0;
}

double ProjectTimeToItemTime (MediaItem* item, double projTime)
{
	if (item)
		 return projTime - GetMediaItemInfo_Value(item, "D_POSITION");
	else
		return projTime;
}

double ItemTimeToProjectTime (MediaItem* item, double itemTime)
{
	if (item)
		 return GetMediaItemInfo_Value(item, "D_POSITION") + itemTime;
	else
		return itemTime;
}

int GetTakeId (MediaItem_Take* take, MediaItem* item /*= NULL*/)
{
	item = (item) ? item : GetMediaItemTake_Item(take);

	for (int i = 0; i < CountTakes(item); ++i)
	{
		if (GetTake(item, 0) == take)
			return i;
	}
	return -1;
}

int GetTakeType (MediaItem_Take* take)
{
	int returnType = -1;
	if (take)
	{
		PCM_source* source = GetMediaItemTake_Source(take);
		source = (!strcmp(source->GetType(), "SECTION")) ? (source->GetSource()) : (source);
		if (source)
		{
			const char* type = source->GetType();

			if (!strcmp(type, "MIDIPOOL") || !strcmp(type, "MIDI"))
				returnType = 1;
			else if (!strcmp(type, "VIDEO"))
				returnType = 2;
			else if (!strcmp(type, "CLICK"))
				returnType = 3;
			else if (!strcmp(type, "LTC"))
				returnType = 4;
			else if (!strcmp(type, "RPP_PROJECT"))
				returnType = 5;
			else if (strcmp(type, ""))
				returnType = 0;
		}
	}
	return returnType;
}

bool SetIgnoreTempo (MediaItem* item, bool ignoreTempo, double bpm, int num, int den)
{
	bool midiFound = false;
	for (int i = 0; i < CountTakes(item); ++i)
	{
		if (IsMidi(GetMediaItemTake(item, i)))
		{
			midiFound = true;
			break;
		}
	}
	if (!midiFound)
		return false;

	WDL_FastString newState;
	char* chunk = GetSetObjectState(item, "");
	bool stateChanged = false;

	char* token = strtok(chunk, "\n");
	while (token != NULL)
	{
		if (!strncmp(token, "IGNTEMPO ", sizeof("IGNTEMPO ") - 1))
		{
			if (ignoreTempo)
				newState.AppendFormatted(256, "IGNTEMPO %d %.14lf %d %d\n", 1, bpm, num, den);
			else
			{
				LineParser lp(false);
				lp.parse(token);
				newState.AppendFormatted(256, "IGNTEMPO %d %.14lf %d %d\n", 0, lp.gettoken_float(2), lp.gettoken_int(3), lp.gettoken_int(4));
			}
			stateChanged = true;
		}
		else
		{
			AppendLine(newState, token);
		}
		token = strtok(NULL, "\n");
	}

	if (stateChanged)
		GetSetObjectState(item, newState.Get());
	FreeHeapPtr(chunk);

	return stateChanged;
}

bool DoesItemHaveMidiEvents (MediaItem* item)
{
	for (int i = 0; i < CountTakes(item); ++i)
	{
		if (MIDI_CountEvts(GetTake(item, i), NULL, NULL, NULL))
			return true;
	}
	return false;
}

bool TrimItem (MediaItem* item, double start, double end)
{
	double newLen  = end - start;
	double itemPos = GetMediaItemInfo_Value(item, "D_POSITION");
	double itemLen = GetMediaItemInfo_Value(item, "D_LENGTH");

	if (start != itemPos || newLen != itemLen)
	{
		double startDif = start - itemPos;
		SetMediaItemInfo_Value(item, "D_LENGTH", newLen);
		SetMediaItemInfo_Value(item, "D_POSITION", start);

		for (int i = 0; i < CountTakes(item); ++i)
		{
			MediaItem_Take* take = GetTake(item, i);
			double playrate = GetMediaItemTakeInfo_Value(take, "D_PLAYRATE");
			double offset   = GetMediaItemTakeInfo_Value(take, "D_STARTOFFS");

			SetMediaItemTakeInfo_Value(take, "D_STARTOFFS", offset + playrate*startDif);
		}
		return true;
	}
	return false;
}

bool GetMediaSourceProperties (MediaItem_Take* take, bool* section, double* start, double* length, double* fade, bool* reverse)
{
	bool sucess         = false;
	bool returnSection  = false;
	bool returnReverse  = false;
	double returnStart  = 0;
	double returnLength = 0;
	double returnFade   = 0;

	PCM_source* source = GetMediaItemTake_Source(take);
	if (source && !IsMidi(take))
	{
		bool foundSectionInChunk = false;
		int mode = 0;

		WDL_HeapBuf hb;
		ProjectStateContext* ctx = ProjectCreateMemCtx(&hb);
		source->SaveState(ctx);
		LineParser lp(false);
		while (ProjectContext_GetNextLine(ctx, &lp))
		{
			if (!strcmp(lp.gettoken_str(0), "LENGTH"))
			{
				foundSectionInChunk = true;
				returnSection = true;
				returnLength = lp.gettoken_float(1);
			}
			else if (!strcmp(lp.gettoken_str(0), "STARTPOS"))
				returnStart = lp.gettoken_float(1);
			else if (!strcmp(lp.gettoken_str(0), "OVERLAP"))
				returnFade = lp.gettoken_float(1);
			else if (!strcmp(lp.gettoken_str(0), "MODE"))
				mode = lp.gettoken_int(1);
			else if (lp.gettoken_str(0)[0] == '<')
				ProjectContext_EatCurrentBlock(ctx);
		}
		delete ctx;

		// Mode token doesn't have to be after other section info, so check it only after parsing everything
		if (mode == 2)
			returnReverse = true;
		else if (mode == 3)
		{
			returnReverse = true;
			returnSection = false;
		}

		// Even if section is turned off, it does display section info so get it
		if (!foundSectionInChunk)
		{
			returnLength = GetMediaItemInfo_Value(GetMediaItemTake_Item(take), "D_LENGTH");
			returnStart = GetMediaItemTakeInfo_Value(take, "D_STARTOFFS");
			returnFade = 0;
		}
		sucess = true;
	}

	WritePtr(section, returnSection);
	WritePtr(start, returnStart);
	WritePtr(length, returnLength);
	WritePtr(fade, returnFade);
	WritePtr(reverse, returnReverse);
	return sucess;
}

bool SetMediaSourceProperties (MediaItem_Take* take, bool section, double start, double length, double fade, bool reverse)
{
	PCM_source* source = GetMediaItemTake_Source(take);
	if (take && source && !IsMidi(take))
	{
		// If existing source has properties already set, it constitutes a special source named "section" that holds
		// another source within which the "real" media resides
		PCM_source* mediaSource = (!strcmp(source->GetType(), "SECTION")) ? (source->GetSource()) : (source);

		PCM_source* newSource = NULL;
		if (!section && !reverse)
		{
			newSource = mediaSource->Duplicate();
		}
		else if (newSource = PCM_Source_CreateFromType("SECTION"))
		{
			newSource->SetSource(mediaSource->Duplicate());
			WDL_FastString sourceStr;

			// Get default section values if needed
			bool getSectionDefaults = !section;
			if (!strcmp(source->GetType(), "SECTION"))
			{
				WDL_HeapBuf hb;
				if (ProjectStateContext* ctx = ProjectCreateMemCtx(&hb))
				{
					source->SaveState(ctx);
					char line[PROJ_CONTEXT_LINE];
					LineParser lp(false);
					while(!ctx->GetLine(line, sizeof(line)) && !lp.parse(line))
					{
						if (!strcmp(lp.gettoken_str(0), "LENGTH"))
						{
							if (getSectionDefaults)
								length = lp.gettoken_float(1);
						}
						else if (!strcmp(lp.gettoken_str(0), "STARTPOS"))
						{
							if (getSectionDefaults)
								start = lp.gettoken_float(1);
						}
						else if (!strcmp(lp.gettoken_str(0), "OVERLAP"))
						{
							if (getSectionDefaults)
								fade = lp.gettoken_float(1);
						}
						else if (!strcmp(lp.gettoken_str(0), "MODE"))
						{
							continue;
						}
						else if (!strcmp(lp.gettoken_str(0), "<SOURCE")) // already got it in mediaSource
						{
							ProjectContext_EatCurrentBlock(ctx);
						}
						else                                             // in case things get added in the future
						{
							AppendLine(sourceStr, line);
						}
					}
				}
			}
			else if (getSectionDefaults)
			{
				length = GetMediaItemInfo_Value(GetMediaItemTake_Item(take), "D_LENGTH");
				start = GetMediaItemTakeInfo_Value(take, "D_STARTOFFS");
				fade = 0.01;
			}

			sourceStr.AppendFormatted(256, "%s%lf\n", "LENGTH ", length);
			sourceStr.AppendFormatted(256, "%s%lf\n", "STARTPOS ", start);
			sourceStr.AppendFormatted(256, "%s%lf\n", "OVERLAP ", fade);
			if (section && reverse) sourceStr.AppendFormatted(256, "%s%d\n", "MODE ", 2);
			else if (reverse)       sourceStr.AppendFormatted(256, "%s%d\n", "MODE ", 3);
			AppendLine(sourceStr, GetSourceChunk(mediaSource).Get());

			WDL_HeapBuf hb;
			if (void* p = hb.Resize(sourceStr.GetLength(), false))
			{
				if (ProjectStateContext* ctx = ProjectCreateMemCtx(&hb))
				{
					memcpy(p, sourceStr.Get(), sourceStr.GetLength());
					newSource->LoadState("<SOURCE SECTION", ctx);
					delete ctx;
				}
				else
					newSource = NULL;
			}
			else
				newSource = NULL;
		}

		if (newSource)
		{
			GetSetMediaItemTakeInfo(take,"P_SOURCE", newSource);
			delete source;
			return true;
		}
	}
	return false;
}

bool SetTakeSourceFromFile (MediaItem_Take* take, const char* filename, bool inProjectData, bool keepSourceProperties)
{
	if (take && file_exists(filename))
	{
		if (PCM_source* oldSource = (PCM_source*)GetSetMediaItemTakeInfo(take,"P_SOURCE",NULL))
		{
			// Getting and setting properties appears faster than source->SetFileName(filename), testing
			// ReaScript export on 10000 items results in 3x performance
			bool section, reverse;
			double start, len, fade;
			bool properties = false;
			if (keepSourceProperties)
				properties = GetMediaSourceProperties(take, &section, &start, &len, &fade, &reverse);

			PCM_source* newSource = PCM_Source_CreateFromFileEx(filename, !inProjectData);
			GetSetMediaItemTakeInfo(take,"P_SOURCE", newSource);
			delete oldSource;

			if (properties)
				SetMediaSourceProperties(take, section, start, len, fade, reverse);
			return true;
		}
	}
	return false;
}

WDL_FastString GetSourceChunk (PCM_source* source)
{
	WDL_FastString sourceStr;
	if (source)
	{
		WDL_HeapBuf hb;
		if (ProjectStateContext* ctx = ProjectCreateMemCtx(&hb))
		{
			source->SaveState(ctx);

			sourceStr.AppendFormatted(PROJ_CONTEXT_LINE, "%s%s\n", "<SOURCE ", source->GetType());
			char line[PROJ_CONTEXT_LINE];
			while(!ctx->GetLine(line, sizeof(line)))
				AppendLine(sourceStr, line);
			sourceStr.Append( ">");

			delete ctx;
		}
	}
	return sourceStr;
}

/******************************************************************************
* Fades                                                                       *
******************************************************************************/
#include "../../WDL/lice/lice_bezier.h"

static void AssignDir(double* x, double* y, const double* b)
{
	/* Code bits courtesy of Cockos, http://askjf.com/index.php?q=2967s */

	x[1]=b[0];
	y[1]=b[1];
	x[2]=b[2];
	y[2]=b[3];
}

static void ApplyDir(double* x, double* y, double w0, double w1, const double* b0, const double* b1)
{
	/* Code bits courtesy of Cockos, http://askjf.com/index.php?q=2967s */

	x[1]=w0*b0[0]+w1*b1[0];
	y[1]=w0*b0[1]+w1*b1[1];
	x[2]=w0*b0[2]+w1*b1[2];
	y[2]=w0*b0[3]+w1*b1[3];
}

static bool GetMediaItemFadeBezParms(int shape, double dir, bool isfadeout, double* x, double* y)
{
	/* Code bits courtesy of Cockos, http://askjf.com/index.php?q=2967s *
	*                                                                   *
	*  x[4], y[4] .. returns true if the fade is linear                 */

	static const double b0[4] = { 0.5, 0.5, 0.5, 0.5 };
	static const double b1[4] = { 0.25, 0.5, 0.625, 1.0 };
	static const double b2[4] = { 0.375, 0.0, 0.75, 0.5 };
	static const double b3[4] = { 0.25, 1.0, 0.5, 1.0 };
	static const double b4[4] = { 0.5, 0.0, 0.75, 0.0 };
	static const double b5[4] = { 0.375, 0.0, 0.625, 1.0 };
	static const double b6[4] = { 0.875, 0.0, 0.125, 1.0 };
	static const double b7[4] = { 0.25, 0.375, 0.625, 1.0 };

	static const double b4i[4] = { 0.0, 1.0, 0.125, 1.0 };
	static const double b50[4] = { 0.25, 0.25, 0.25, 1.0 };
	static const double b51[4] = { 0.75, 0.0, 0.75, 0.75 };
	static const double b60[4] = { 0.375, 0.25, 0.0, 1.0 };
	static const double b61[4] = { 1.0, 0.0, 0.625, 0.75 };

	if (!isfadeout)
	{
		x[0]=y[0]=0.0;
		x[3]=y[3]=1.0;
	}
	else
	{
		x[0]=y[3]=0.0;
		x[3]=y[0]=1.0;
	}

	if (shape < 0 || shape >= 8)
	{
		shape=0;
		dir=0.0;
	}

	if (isfadeout)
		dir=-dir;

	bool linear=false;

	if (dir < 0.0)
	{
		double w0=-dir;
		double w1=1.0+dir;
		if (shape == 1)      ApplyDir(x, y, w0, w1, b4i, b1);
		else if (shape == 2) ApplyDir(x, y, w0, w1, b1,  b0);
		else if (shape == 5) ApplyDir(x, y, w0, w1, b50, b5);
		else if (shape == 6) ApplyDir(x, y, w0, w1, b60, b6);
		else if (shape == 7) ApplyDir(x, y, w0, w1, b4i, b7);
		else                 ApplyDir(x, y, w0, w1, b3,  b0);
	}
	else if (dir > 0.0)
	{
		double w0=1.0-dir;
		double w1=dir;
		if (shape == 1)      ApplyDir(x, y, w0, w1, b1, b4);
		else if (shape == 2) ApplyDir(x, y, w0, w1, b0, b2);
		else if (shape == 5) ApplyDir(x, y, w0, w1, b5, b51);
		else if (shape == 6) ApplyDir(x, y, w0, w1, b6, b61);
		else if (shape == 7) ApplyDir(x, y, w0, w1, b7, b4);
		else                 ApplyDir(x, y, w0, w1, b0, b4);
	}
	else // dir == 0.0
	{
		if (shape == 1)      AssignDir(x, y, b1);
		else if (shape == 5) AssignDir(x, y, b5);
		else if (shape == 6) AssignDir(x, y, b6);
		else if (shape == 7) AssignDir(x, y, b7);
		else
		{
			AssignDir(x, y, b0);
			linear=true;
		}
	}

	if (isfadeout && !linear)
	{
		double ox1=x[1];
		double ox2=x[2];
		x[1]=1.0-ox2;
		x[2]=1.0-ox1;
		swap(y[1], y[2]);
	}

	return linear;
}

double GetNormalizedFadeValue (double position, double fadeStart, double fadeEnd, int fadeShape, double fadeCurve, bool isFadeOut)
{
	if (position < fadeStart)
		return (isFadeOut) ? 1 : 0;
	else if (position > fadeEnd)
		return (isFadeOut) ? 0 : 1;
	else
	{
		double x[4], y[4];
		GetMediaItemFadeBezParms(fadeShape, fadeCurve, isFadeOut, x, y);

		double t = (position - fadeStart) / (fadeEnd - fadeStart);
		return LICE_CBezier_GetY(x[0], x[1], x[2], x[3], y[0], y[1], y[2], y[3], t);
	}
}

double GetEffectiveFadeLength (MediaItem* item, bool isFadeOut, int* fadeShape /*= NULL*/, double* fadeCurve /*= NULL*/)
{
	if (item)
	{
		WritePtr(fadeShape, ((isFadeOut) ? (int)GetMediaItemInfo_Value(item, "C_FADEOUTSHAPE") : (int)GetMediaItemInfo_Value(item, "C_FADEINSHAPE")));
		WritePtr(fadeCurve, ((isFadeOut) ?       GetMediaItemInfo_Value(item, "D_FADEOUTDIR")  :      GetMediaItemInfo_Value(item, "D_FADEINDIR")));

		double fadeLength = GetMediaItemInfo_Value(item, ((isFadeOut) ? "D_FADEOUTLEN_AUTO" : "D_FADEINLEN_AUTO"));
		return (fadeLength > 0) ? fadeLength : GetMediaItemInfo_Value(item, ((isFadeOut) ? "D_FADEOUTLEN" : "D_FADEINLEN"));
	}
	else
	{
		WritePtr(fadeShape, 0);
		WritePtr(fadeCurve, 0.0);
		return 0;
	}
}

/******************************************************************************
* Stretch markers                                                             *
******************************************************************************/
int FindPreviousStretchMarker (MediaItem_Take* take, double position)
{
	int first = 0;
	int last = GetTakeNumStretchMarkers(take);

	while (first != last)
	{
		int mid = (first + last) / 2;
		double currentPos; GetTakeStretchMarker(take, mid, &currentPos, NULL);

		if (currentPos < position) first = mid + 1;
		else                       last  = mid;

	}
	return first - 1;
}

int FindNextStretchMarker (MediaItem_Take* take, double position)
{
	int first = 0;
	int last = GetTakeNumStretchMarkers(take);
	int count = last;

	while (first != last)
	{
		int mid = (first + last) /2;
		double currentPos; GetTakeStretchMarker(take, mid, &currentPos, NULL);

		if (position >= currentPos) first = mid + 1;
		else                        last  = mid;

	}
	return (first < count) ? first: -1;
}

int FindClosestStretchMarker (MediaItem_Take* take, double position)
{
	int prevId = FindPreviousStretchMarker(take, position);
	int nextId = prevId + 1;

	int count = GetTakeNumStretchMarkers(take);
	if (prevId == -1)
	{
		return (nextId < count) ? nextId : -1;
	}
	else
	{
		if (nextId < count)
		{

			double prevPos, nextPos;
			GetTakeStretchMarker(take, prevId, &prevPos, NULL);
			GetTakeStretchMarker(take, nextId, &nextPos, NULL);
			double len1 = position - prevPos;
			double len2 = nextPos - position;

			if (len1 >= len2)
				return nextId;
			else
				return prevId;
		}
		else
		{
			return prevId;
		}
	}
}

int FindStretchMarker (MediaItem_Take* take, double position, double surroundingRange /*= 0*/)
{
	int count = GetTakeNumStretchMarkers(take);
	if (count == 0)
		return -1;

	int prevId = FindPreviousStretchMarker(take, position);
	int nextId = prevId + 1;

	double prevPos, nextPos;
	GetTakeStretchMarker(take, prevId, &prevPos, NULL);
	GetTakeStretchMarker(take, nextId, &nextPos, NULL);

	double distanceFromPrev = (CheckBounds(prevId, 0, count-1)) ? (position - prevPos) : (abs(surroundingRange) + 1);
	double distanceFromNext = (CheckBounds(nextId, 0, count-1)) ? (nextPos - position) : (abs(surroundingRange) + 1);

	int id = -1;
	if (distanceFromPrev <= distanceFromNext)
	{
		if (distanceFromPrev <= surroundingRange)
			id = prevId;
	}
	else
	{
		if (distanceFromNext <= surroundingRange)
			id = nextId;
	}
	return (id < count) ? id : -1;
}

/******************************************************************************
* Grid                                                                        *
******************************************************************************/
double GetNextGridDiv (double position)
{
	/* This got a tiny bit complicated, but we're trying to replicate        *
	*  REAPER behavior as closely as possible...I guess the ultimate test    *
	*  would be inserting a bunch of really strange time signatures coupled  *
	*  with possibly even stranger grid divisions, things any sane person    *
	*  would never use - like 11/7 with grid division of 2/13...haha. As it  *
	*  stands now, at REAPER v4.7, this function handles it all              */

	// Grid dividing starts again from the measure in which tempo marker is so find location of first measure grid (obvious if grid division spans more measures)
	int gridDivStartTempo = FindPreviousTempoMarker(position);
	double gridDivStart;
	if (gridDivStartTempo >= 0)
	{
		if (GetTempoTimeSigMarker(NULL, gridDivStartTempo + 1, &gridDivStart, NULL, NULL, NULL, NULL, NULL, NULL))
		{
			if (gridDivStart > position)
				GetTempoTimeSigMarker(NULL, gridDivStartTempo, &gridDivStart, NULL, NULL, NULL, NULL, NULL, NULL);
			else
				gridDivStartTempo += 1;
		}
		else
			GetTempoTimeSigMarker(NULL, gridDivStartTempo, &gridDivStart, NULL, NULL, NULL, NULL, NULL, NULL);
	}
	else
	{
		gridDivStart = 0;
		gridDivStartTempo = 0; // if position is right at project start this would be -1 even if first tempo marker exists
	}

	// Get grid division translated into current time signature
	int gridDivStartMeasure, num, den;
	TimeMap2_timeToBeats(0, gridDivStart, &gridDivStartMeasure, &num, NULL, &den);
	double gridDiv = GetGridDivSafe();
	gridDiv = (den*gridDiv) / 4;

	// How much measures must pass for grid diving to start anew? (again, obvious when grid division spans more measures)
	int measureStep = (int)(gridDiv/num);
	if (measureStep == 0) measureStep = 1;

	// Find closest measure to our position, where grid diving starts again
	int positionMeasure;
	TimeMap2_timeToBeats(0, position, &positionMeasure, NULL, NULL, NULL);
	gridDivStartMeasure += (int)((positionMeasure - gridDivStartMeasure) / measureStep) * measureStep;
	gridDivStart = TimeMap2_beatsToTime(0, 0, &gridDivStartMeasure);

	// Finally find next grid position (different cases for measures and beats)
	double nextGridPosition;
	if (gridDiv > num)
	{
		int gridDivEndMeasure = gridDivStartMeasure + measureStep;
		double gridDivEnd = TimeMap2_beatsToTime(0, 0, &gridDivEndMeasure);

		// Same as before, existing tempo markers can move end measure grid (where our next grid should be) so find if that's the case
		if (measureStep > 1)
		{
			double tempoPosition;
			while (GetTempoTimeSigMarker(NULL, ++gridDivStartTempo, &tempoPosition, NULL, NULL, NULL, NULL, NULL, NULL) && tempoPosition <= gridDivEnd)
			{
				int currentMeasure;
				TimeMap2_timeToBeats(0, tempoPosition, &currentMeasure, NULL, NULL, NULL);

				gridDivEndMeasure = currentMeasure + ((currentMeasure == positionMeasure) ? (measureStep) : (0));
				gridDivEnd = TimeMap2_beatsToTime(0, 0, &gridDivEndMeasure);
			}
		}

		nextGridPosition = TimeMap2_beatsToTime(0, 0, &gridDivEndMeasure);
	}
	else
	{
		double positionBeats = TimeMap2_timeToBeats(0, position, NULL, NULL, NULL, NULL) - TimeMap2_timeToBeats(0, gridDivStart, NULL, NULL, NULL, NULL);
		double nextGridBeats = (int)((positionBeats + gridDiv) / gridDiv) * gridDiv;
		while (abs(nextGridBeats - positionBeats) < 1E-6) nextGridBeats += gridDiv; // rounding errors, yuck...

		nextGridPosition = TimeMap2_beatsToTime(0, nextGridBeats, &gridDivStartMeasure);

		// Check it didn't pass over into next measure
		int gridDivEndMeasure = gridDivStartMeasure + measureStep;
		double gridDivEnd     = TimeMap2_beatsToTime(0, 0, &gridDivEndMeasure);
		if (nextGridPosition > gridDivEnd)
			nextGridPosition = gridDivEnd;
	}

	// Not so perfect fix for this issue: http://forum.cockos.com/project.php?issueid=5263
	if (nextGridPosition < position)
	{
		double gridDiv = GetGridDivSafe();

		double tempoPosition, beat; int measure; GetTempoTimeSigMarker(NULL, gridDivStartTempo, &tempoPosition, &measure, &beat, NULL, NULL, NULL, NULL);
		double offset =  gridDiv - fmod(TimeMap_timeToQN(TimeMap2_beatsToTime(0, 0, &measure)), gridDiv);

		double gridLn = TimeMap_timeToQN(tempoPosition);
		gridLn = TimeMap_QNToTime(gridLn - offset - fmod(gridLn, gridDiv));
		while (gridLn < position + (MIN_GRID_DIST/2))
			gridLn = TimeMap_QNToTime(TimeMap_timeToQN(gridLn) + gridDiv);

		nextGridPosition = gridLn;
	}

	return nextGridPosition;
}

double GetClosestGridLine (double position)
{
	/* All other GridLine (but not GridDiv) functions   *
	*  are depending on this, but it appears SnapToGrid *
	*  is broken in certain non-real-world testing      *
	*  situations, so it should probably we rewritten   *
	*  using the stuff from GetNextGridDiv()            */

	int snap; GetConfig("projshowgrid", snap);
	SetConfig("projshowgrid", ClearBit(snap, 8));

	double grid = SnapToGrid(NULL, position);
	SetConfig("projshowgrid", snap);
	return grid;
}

double GetClosestMeasureGridLine (double position)
{
	double gridDiv = GetGridDivSafe();
	int num, den; TimeMap_GetTimeSigAtTime(0, position, &num, &den, NULL);

	double gridLine = 0;
	if ((den*gridDiv) / 4 < num)
	{
		SetConfig("projgriddiv", 4*(double)num/(double)den); // temporarily set grid div to full measure of current time signature
		gridLine = GetClosestGridLine(position);
		SetConfig("projgriddiv", gridDiv);
	}
	else
	{
		gridLine = GetClosestGridLine(position);
	}
	return gridLine;
}

double GetClosestLeftSideGridLine (double position)
{
	double grid = GetClosestGridLine(position);
	if (position >= grid)
		return grid;

	double gridDiv = GetGridDivSafe();
	int minGridPx; GetConfig("projgridmin", minGridPx);
	double hZoom = GetHZoomLevel();

	int num, den; TimeMap_GetTimeSigAtTime(0, position, &num, &den, NULL);

	if ((den*gridDiv) / 4 < num)
	{
		double beats; int denom;
		TimeMap2_timeToBeats(NULL, grid, NULL, NULL, &beats, &denom);
		gridDiv *= (double)denom/4;

		double leftSideGrid  = TimeMap2_beatsToTime(NULL, beats - gridDiv, NULL);
		while ((grid - leftSideGrid) * hZoom < minGridPx)
		{
			gridDiv *= 2;
			leftSideGrid = TimeMap2_beatsToTime(NULL, beats - gridDiv, NULL);
		}
		return leftSideGrid;
	}
	else
	{
		gridDiv = (int)gridDiv/4;
		int measure;
		TimeMap2_timeToBeats(NULL, grid, &measure, NULL, NULL, NULL);

		measure -= (int)gridDiv;
		double leftSideGrid  = TimeMap2_beatsToTime(NULL, 0, &measure);
		while ((grid - leftSideGrid) * hZoom < minGridPx)
		{
			measure -= (int)gridDiv;
			leftSideGrid  = TimeMap2_beatsToTime(NULL, 0, &measure);
			gridDiv *= 2;
		}
		return leftSideGrid;
	}
}

double GetClosestRightSideGridLine (double position)
{
	double grid = GetClosestGridLine(position);
	if (position <= grid)
		return grid;

	double gridDiv = GetGridDivSafe();
	int minGridPx; GetConfig("projgridmin", minGridPx);
	double hZoom = GetHZoomLevel();

	int num, den; TimeMap_GetTimeSigAtTime(0, position, &num, &den, NULL);

	if ((den*gridDiv) / 4 < num)
	{
		double beats; int denom;
		TimeMap2_timeToBeats(NULL, grid, NULL, NULL, &beats, &denom);
		gridDiv *= (double)denom/4;

		double rightSideGrid = TimeMap2_beatsToTime(NULL, beats + gridDiv, NULL);
		while ((rightSideGrid - grid) * hZoom < minGridPx)
		{
			gridDiv *= 2;
			rightSideGrid = TimeMap2_beatsToTime(NULL, beats + gridDiv, NULL);
		}
		return rightSideGrid;
	}
	else
	{
		gridDiv = (int)gridDiv/4;
		int measure;
		TimeMap2_timeToBeats(NULL, grid, &measure, NULL, NULL, NULL);

		measure += (int)gridDiv;
		double rightSideGrid = TimeMap2_beatsToTime(NULL, 0, &measure);
		while ((rightSideGrid - grid) * hZoom < minGridPx)
		{
			measure += (int)gridDiv;
			rightSideGrid = TimeMap2_beatsToTime(NULL, 0, &measure);
			gridDiv *= 2;
		}
		return rightSideGrid;
	}
}

/******************************************************************************
* Locking                                                                     *
******************************************************************************/
bool IsLockingActive ()
{
	int projSelLock; GetConfig("projsellock", projSelLock);
	return !!GetBit(projSelLock, 14);
}

bool IsLocked (int lockElements)
{
	int projSelLock; GetConfig("projsellock", projSelLock);

	if (!GetBit(projSelLock, 14))
		return false;
	else
		return ((projSelLock & lockElements) == lockElements);
}

bool IsItemLocked (MediaItem* item)
{
	char itemLock = (char)GetMediaItemInfo_Value(item, "C_LOCK");
	return (itemLock & 1);
}

/******************************************************************************
* Height                                                                      *
******************************************************************************/
int GetTrackHeightFromVZoomIndex (MediaTrack* track, int vZoom)
{
	int height = 0;
	if (track)
	{
		IconTheme* theme = SNM_GetIconTheme();

		int compact = GetEffectiveCompactLevel(track);
		if (compact == 2)      height = theme->tcp_supercollapsed_height; // Compacted track is not affected by vertical
		else if (compact == 1) height = theme->tcp_small_height;          // zoom if there is no height override
		else
		{
			if (vZoom <= 4)
			{
				int fullArm; GetConfig("zoomshowarm", fullArm);
				int armed =  (int)GetMediaTrackInfo_Value(track, "I_RECARM");

				if ((GetMasterTrack(NULL) == track)) height = theme->tcp_master_min_height;
				else if (fullArm && armed)           height = theme->tcp_full_height;
				else if (vZoom == 0)                 height = theme->tcp_small_height;
				else if (vZoom == 1)                 height = (theme->tcp_small_height + theme->tcp_medium_height) / 2;
				else if (vZoom == 2)                 height = theme->tcp_medium_height;
				else if (vZoom == 3)                 height = (theme->tcp_medium_height + theme->tcp_full_height) / 2;
				else if (vZoom == 4)                 height = theme->tcp_full_height;
			}
			else
			{
				// Get max and min height
				RECT r; GetClientRect(GetArrangeWnd(), &r);
				int maxHeight = r.bottom - r.top;
				int minHeight = theme->tcp_full_height;

				// Calculate track height (method stolen from Zoom.cpp - Thanks Tim!)
				int normalizedZoom = (vZoom < 30) ? (vZoom - 4) : (30 + 3*(vZoom - 30) - 4);
				height = minHeight + ((maxHeight-minHeight)*normalizedZoom) / 56;
			}
		}
	}
	return height;
}

int GetEnvHeightFromTrackHeight (int trackHeight)
{
	int height = TruncToInt((double)trackHeight * 0.75);

	int minHeight = SNM_GetIconTheme()->envcp_min_height;
	if (height < minHeight)
		height = minHeight;

	return height;
}

int GetTrackHeight (MediaTrack* track, int* offsetY, int* topGap /*=NULL*/, int* bottomGap /*=NULL*/)
{
	bool master = (GetMasterTrack(NULL) == track) ? (true) : (false);

	// Get track's start Y coordinate
	if (offsetY)
	{
		int offset = 0;
		if (!master)
		{
			int count = CSurf_TrackToID(track, false) - 1;
			if (count >= -1)
			{
				for (int i = 0; i < count; ++i)
					offset += (int)GetMediaTrackInfo_Value(GetTrack(NULL, i), "I_WNDH"); // I_WNDH counts both track lane and any visible envelope lanes
				if (TcpVis(GetMasterTrack(NULL)))
					offset += (int)GetMediaTrackInfo_Value(GetMasterTrack(NULL), "I_WNDH") + TCP_MASTER_GAP;
			}
		}
		*offsetY = offset;
	}

	if (!TcpVis(track))
		return 0;

	int compact = GetEffectiveCompactLevel(track); // Track can't get resized at compact 2 and "I_HEIGHTOVERRIDE" could return
	if (compact == 2)                              // wrong value if track was resized prior to compacting so return here
		return SNM_GetIconTheme()->tcp_supercollapsed_height;

	// Get track height
	int height = (int)GetMediaTrackInfo_Value(track, "I_HEIGHTOVERRIDE");
	if (height == 0)
	{
		int vZoom; GetConfig("vzoom2", vZoom);
		height = GetTrackHeightFromVZoomIndex(track, vZoom);
	}
	else
	{
		// If track is compacted by 1 level and it was manually resized just prior to compacting, I_HEIGHTOVERRIDE
		// will return previous size and not 0 even though track is behaving as so. Then again, user could have
		// resized track in compact mode so returned I_HEIGHTOVERRIDE would be correct in that case.
		// So get track's HWND in TCP and get height from it
		if (compact == 1)
		{
			RECT r;
			GetClientRect(GetTcpTrackWnd(track), &r);
			height = r.bottom - r.top;
		}
		// Check I_HEIGHTOVERRIDE against minimum height (i.e. changing theme resizes tracks that are below
		// minimum, but I_HEIGHTOVERRIDE doesn't get updated)
		else
		{
			IconTheme* theme = SNM_GetIconTheme();

			if (master)
			{
				if (height < theme->tcp_master_min_height)
					height = theme->tcp_master_min_height;
			}
			else
			{
				if (height < theme->tcp_small_height)
					height = theme->tcp_small_height;
			}
		}
	}

	if (topGap || bottomGap)
		GetTrackGap(height, topGap, bottomGap);
	return height;
}

int GetItemHeight (MediaItem* item, int* offsetY)
{
	int trackOffsetY;
	int trackH = GetTrackHeight(GetMediaItem_Track(item), &trackOffsetY);
	return GetItemHeight(item, offsetY, trackH, trackOffsetY);
}

int GetTakeHeight (MediaItem_Take* take, int* offsetY)
{
	return GetTakeHeight(take, NULL, 0, offsetY, false);
}

int GetTakeHeight (MediaItem* item, int id, int* offsetY)
{
	return GetTakeHeight(NULL, item, id, offsetY, false);
}

int GetTakeEnvHeight (MediaItem_Take* take, int* offsetY)
{
	// Last take may have different height than the others, but envelope is always positioned based
	// on take height of other takes so we get average take height (relevant for last take only)
	int envelopeH = GetTakeHeight(take, NULL, 0, offsetY, true) - 2*ENV_GAP;
	envelopeH = (envelopeH > ENV_LINE_WIDTH) ? envelopeH : 0;

	if (take && offsetY) *offsetY = *offsetY + ENV_GAP;
	return envelopeH;
}

int GetTakeEnvHeight (MediaItem* item, int id, int* offsetY)
{
	// Last take may have different height than the others, but envelope is always positioned based
	// on take height of other takes so we get average take height (relevant for last take only)
	int envelopeH = GetTakeHeight(NULL, item, id, offsetY, true) - 2*ENV_GAP;
	envelopeH = (envelopeH > ENV_LINE_WIDTH) ? envelopeH : 0;

	if (item && offsetY) *offsetY = *offsetY + ENV_GAP;
	return envelopeH;
}

int GetTrackEnvHeight (TrackEnvelope* envelope, int* offsetY, bool drawableRangeOnly, MediaTrack* parent /*=NULL*/)
{
	MediaTrack* track = (parent) ? (parent) : (GetEnvParent(envelope));
	if (!track || !envelope)
	{
		WritePtr(offsetY, 0);
		return 0;
	}

	// Prepare return variables and get track's height and offset
	int envOffset = 0;
	int envHeight = 0;
	int trackOffset = 0;
	int trackHeight = GetTrackHeight(track, (offsetY) ? (&trackOffset) : (NULL));

	// Get first envelope's lane hwnd and cycle through the rest
	HWND hwnd = GetWindow(GetTcpTrackWnd(track), GW_HWNDNEXT);
	MediaTrack* nextTrack = CSurf_TrackFromID(1 + CSurf_TrackToID(track, false), false);
	list<TrackEnvelope*> checkedEnvs;

	bool found = false;
	int envelopeId = GetEnvId(envelope, track);
	int count = CountTrackEnvelopes(track);
	for (int i = 0; i < count; ++i)
	{
		LONG_PTR hwndData = GetWindowLongPtr(hwnd, GWLP_USERDATA);
		if ((MediaTrack*)hwndData == nextTrack)
			break;

		if (TrackEnvelope* currentEnvelope = HwndToEnvelope(hwnd))
		{
			if (currentEnvelope == envelope)
			{
				RECT r; GetClientRect(hwnd, &r);
				envHeight = r.bottom - r.top - ((drawableRangeOnly) ? 2*ENV_GAP : 0);
				envOffset = trackOffset + trackHeight + envOffset + ((drawableRangeOnly) ? ENV_GAP : 0);
				found = true;
			}
			else
			{
				// In certain cases (depending if user scrolled up or down), envelope
				// hwnds (got by GW_HWNDNEXT) won't be in order they are drawn, so make
				// sure requested envelope is drawn under envelope currently in loop
				if (envelopeId > GetEnvId(currentEnvelope, track))
				{
					RECT r; GetClientRect(hwnd, &r);
					envOffset += r.bottom-r.top;
				}
				checkedEnvs.push_back(currentEnvelope);
			}
		}

		hwnd = GetWindow(hwnd, GW_HWNDNEXT);
	}

	// Envelope is either hidden or drawn in track lane
	if (!found)
	{
		if (!EnvVis(envelope, NULL))
		{
			envHeight = 0;
			envOffset = 0;
		}
		else
		{
			int overlapLimit; GetConfig("env_ol_minh", overlapLimit);
			bool overlapEnv = (overlapLimit >= 0) ? (true) : (false);

			int trackGapTop, trackGapBottom;
			GetTrackGap(trackHeight, &trackGapTop, &trackGapBottom);

			int envLaneFull = trackHeight - trackGapTop - trackGapBottom;
			int envLaneCount = 0;
			int envLanePos = 0;
			bool foundEnv = false;

			int count = CountTrackEnvelopes(track);
			for (int i = 0; i < count; ++i)
			{
				TrackEnvelope* currentEnv = GetTrackEnvelope(track, i);
				std::list<TrackEnvelope*>::iterator it = find(checkedEnvs.begin(), checkedEnvs.end(), currentEnv);

				// Envelope was already encountered, no need to check visibility (chunks are expensive!) so just skip it
				if (it != checkedEnvs.end())
					checkedEnvs.erase(it); // and this is why we use list
				else
				{
					if (currentEnv != envelope)
					{
						if (EnvVis(currentEnv, NULL))
						{
							++envLaneCount;
							if (!foundEnv)
								++envLanePos;
						}
					}
					else
					{
						++envLaneCount;
						foundEnv = true;
					}
				}

				// Check current height
				if (envLaneCount != 0)
				{
					int envLaneH = envLaneFull / envLaneCount; // div reminder isn't important
					if (overlapEnv && envLaneH < overlapLimit)
					{
						if (drawableRangeOnly)
						{
							envHeight = envLaneFull - 2*ENV_GAP ;
							envOffset = trackOffset + trackGapTop + ENV_GAP;
						}
						else
						{
							envHeight = (envLaneFull - 2*ENV_GAP <= ENV_LINE_WIDTH) ? 0 : envLaneH;
							envOffset = (envLaneFull - 2*ENV_GAP <= ENV_LINE_WIDTH) ? 0 : trackOffset + trackGapTop + envLanePos*envLaneH;
						}
						break;
					}

					// No need to calculate return values every iteration
					if (i == count-1)
					{
						if (drawableRangeOnly)
						{
							envHeight = envLaneH - 2*ENV_GAP;
							envOffset = trackOffset + trackGapTop + envLanePos*envLaneH + ENV_GAP;
						}
						else
						{
							envHeight = (envLaneFull - 2*ENV_GAP <= ENV_LINE_WIDTH) ? 0 : envLaneH;
							envOffset = (envLaneFull - 2*ENV_GAP <= ENV_LINE_WIDTH) ? 0 : trackOffset + trackGapTop + envLanePos*envLaneH;
						}
					}
				}
			}
		}
	}

	if (drawableRangeOnly && envHeight <= ENV_LINE_WIDTH)
		envHeight = 0;
	WritePtr (offsetY, envOffset);
	return envHeight;
}

/******************************************************************************
* Arrange                                                                     *
******************************************************************************/
void MoveArrange (double amountTime)
{
	RECT r;
	double startTime, endTime;
	GetWindowRect(GetArrangeWnd(), &r);

	GetSet_ArrangeView2(NULL, false, r.left, r.right-SCROLLBAR_W, &startTime, &endTime);
	startTime += amountTime;
	endTime += amountTime;
	GetSet_ArrangeView2(NULL, true, r.left, r.right-SCROLLBAR_W, &startTime, &endTime);
}

void CenterArrange (double position)
{
	RECT r;
	double startTime, endTime;
	GetWindowRect(GetArrangeWnd(), &r);

	GetSet_ArrangeView2(NULL, false, r.left, r.right-SCROLLBAR_W, &startTime, &endTime);
	double halfSpan = (endTime - startTime) / 2;
	startTime = position - halfSpan;
	endTime = position + halfSpan;
	GetSet_ArrangeView2(NULL, true, r.left, r.right-SCROLLBAR_W, &startTime, &endTime);
}

void SetArrangeStart (double start)
{
	SCROLLINFO si = { sizeof(SCROLLINFO), };
	si.fMask = SIF_ALL;
	CoolSB_GetScrollInfo(GetArrangeWnd(), SB_HORZ, &si);

	si.nPos = RoundToInt(start * GetHZoomLevel()); // OCD alert: GetSet_ArrangeView2() can sometimes be off for one pixel (probably round vs trunc issue)
	CoolSB_SetScrollInfo(GetArrangeWnd(), SB_HORZ, &si, true);
	SendMessage(GetArrangeWnd(), WM_HSCROLL, SB_THUMBPOSITION, NULL);
}

void MoveArrangeToTarget (double target, double reference)
{
	if (!IsOffScreen(target))
	{
		if (IsOffScreen(reference))
			MoveArrange(target - reference);
		else
			CenterArrange(target);
	}
}

bool IsOffScreen (double position)
{
	RECT r;
	double startTime, endTime;
	GetWindowRect(GetArrangeWnd(), &r);
	GetSet_ArrangeView2(NULL, false, r.left, r.right-SCROLLBAR_W, &startTime, &endTime);

	if (position >= startTime && position <= endTime)
		return true;
	else
		return false;
}

/******************************************************************************
* Window                                                                      *
******************************************************************************/
static void PrepareLocalizedString (const char* name, char* buf, int buf_sz)
{
	/* If all the letters in name would be part of user set locale in win32  *
	*  this would not be needed - however, that doesn't have to be the case  *
	*  so we convert name to follow user locale - in turn it can be used by  *
	*  ANSI version of GetWindowText()                                       */

	#ifdef _WIN32
		wchar_t widetext[2048];
		MultiByteToWideChar(CP_UTF8, 0, name, -1, widetext,  sizeof(widetext)/sizeof(wchar_t));
		WideCharToMultiByte(CP_ACP, 0, widetext, -1, buf, buf_sz, NULL, NULL);
		MultiByteToWideChar(CP_ACP, 0, buf, -1, widetext,  sizeof(widetext)/sizeof(wchar_t));
		WideCharToMultiByte(CP_UTF8, 0, widetext, -1, buf, buf_sz, NULL, NULL);
	#endif
}

static void AllocPreparedString (const char* name, char** destination)
{
	#ifdef _WIN32
		if (IsLocalized())
		{
			char buf[2048];
			PrepareLocalizedString(name, buf, sizeof(buf));

			int size = strlen(buf) + 1;
			if (WritePtr(destination, new (nothrow) char[size]))
				strncpy(*destination, buf, size);
		}
		else
	#endif
		{
			*destination = const_cast<char*>(name);
		}
}

static HWND SearchChildren (const char* name, HWND hwnd, HWND startHwnd = NULL)
{
	/* FindWindowEx alternative that works with name prepared by *
	*  PrepareLocalizedString() (relevant for win32 only)        */

	#ifdef _WIN32
		if (IsLocalized())
		{
			if (!hwnd) hwnd = GetDesktopWindow();
			hwnd = (startHwnd) ? GetWindow(startHwnd, GW_HWNDNEXT) : GetWindow(hwnd, GW_CHILD);

			HWND returnHwnd = NULL;
			do
			{
				char wndName[2048];
				GetWindowText(hwnd, wndName, sizeof(wndName));
				if (!strcmp(wndName, name))
					returnHwnd = hwnd;
			}
			while ((hwnd = GetWindow(hwnd, GW_HWNDNEXT)) && !returnHwnd);

			return returnHwnd;
		}
		else
	#endif
		{
			return FindWindowEx(hwnd, startHwnd, NULL , name);
		}
}

static HWND SearchFloatingDockers (const char* name, const char* dockerName)
{
	HWND docker = FindWindowEx(NULL, NULL, NULL, dockerName);
	while(docker)
	{
		if (GetParent(docker) == g_hwndParent)
		{
			HWND insideDocker = FindWindowEx(docker, NULL, NULL, "REAPER_dock");
			while(insideDocker)
			{
				if (HWND w = SearchChildren(name, insideDocker))
					return w;
				insideDocker = FindWindowEx(docker, insideDocker, NULL, "REAPER_dock");
			}
		}
		docker = FindWindowEx(NULL, docker, NULL, dockerName);
	}
	return NULL;
}

static HWND FindInFloatingDockers (const char* name)
{
	#ifdef _WIN32
		HWND hwnd = SearchFloatingDockers(name, NULL);
	#else
		HWND hwnd = SearchFloatingDockers(name, __localizeFunc("Docker", "docker", 0));
		if (!hwnd)
		{
			WDL_FastString dockerName;
			dockerName.AppendFormatted(256, "%s%s", name, __localizeFunc(" (docked)", "docker", 0));
			hwnd = SearchFloatingDockers(name, dockerName.Get());
		}
		if (!hwnd)
			hwnd = SearchFloatingDockers(name, __localizeFunc("Toolbar Docker", "docker", 0));
	#endif

	return hwnd;
}

static HWND FindInReaperDockers (const char* name)
{
	HWND docker = FindWindowEx(g_hwndParent, NULL, NULL, "REAPER_dock");
	while (docker)
	{
		if (HWND hwnd = SearchChildren(name, docker))
			return hwnd;
		docker = FindWindowEx(g_hwndParent, docker, NULL, "REAPER_dock");
	}
	return NULL;
}

static HWND FindFloating (const char* name)
{
	HWND hwnd = SearchChildren(name, NULL);
	while (hwnd)
	{
		if (GetParent(hwnd) == g_hwndParent)
			return hwnd;
		hwnd = SearchChildren(name, NULL, hwnd);
	}
	return NULL;
}

static HWND FindInReaper (const char* name)
{
	return SearchChildren(name, g_hwndParent);
}

static HWND FindReaperWndByPreparedString (const char* name)
{
	if (name)
	{
		HWND hwnd = FindInReaperDockers(name);
		if (!hwnd) hwnd = FindFloating(name);
		if (!hwnd) hwnd = FindInFloatingDockers(name);
		if (!hwnd) hwnd = FindInReaper(name);
		return hwnd;
	}
	return NULL;
}

HWND FindReaperWndByTitle (const char* name)
{
	#ifdef _WIN32
		if (IsLocalized())
		{
			char preparedName[2048];
			PrepareLocalizedString(name, preparedName, sizeof(preparedName));
			return FindReaperWndByPreparedString(preparedName);
		}
		else
	#endif
		{
			return FindReaperWndByPreparedString(name);
		}
}

HWND GetArrangeWnd ()
{
	static HWND s_hwnd = NULL; /* More efficient and Justin says it's safe: http://askjf.com/index.php?q=2653s */

	if (!s_hwnd)
	{
		#ifdef _WIN32
			if (IsLocalized())
			{
				char preparedName[2048];
				PrepareLocalizedString(__localizeFunc("trackview", "DLG_102", 0), preparedName, sizeof(preparedName));

				HWND wnd = SearchChildren(preparedName, g_hwndParent);
				while (wnd)
				{
					char buf[256];
					GetClassName(wnd, buf, sizeof(buf));
					if (!strcmp(buf, "REAPERTrackListWindow"))
					{
						s_hwnd = wnd;
						break;
					}
					wnd = SearchChildren(preparedName, g_hwndParent, wnd);
				}
			}
			else
			{
				s_hwnd = FindWindowEx(g_hwndParent, 0, "REAPERTrackListWindow", __localizeFunc("trackview", "DLG_102", 0));
			}
		#else
			return GetWindow(g_hwndParent, GW_CHILD);
		#endif
	}
	return s_hwnd;
}

HWND GetRulerWndAlt ()
{
	static HWND s_hwnd = NULL; /* More efficient and Justin says it's safe: http://askjf.com/index.php?q=2653s */
	if (!s_hwnd)
	{
		#ifdef _WIN32
			if (IsLocalized())
			{
				char preparedName[2048];
				PrepareLocalizedString(__localizeFunc("timeline", "DLG_102", 0), preparedName, sizeof(preparedName));

				HWND wnd = SearchChildren(preparedName, g_hwndParent);
				while (wnd)
				{
					char buf[256];
					GetClassName(wnd, buf, sizeof(buf));
					if (!strcmp(buf, "REAPERTimeDisplay"))
					{
						s_hwnd = wnd;
						break;
					}
					wnd = SearchChildren(preparedName, g_hwndParent, wnd);
				}
			}
			else
			{
				s_hwnd = FindWindowEx(g_hwndParent, 0, "REAPERTimeDisplay", __localizeFunc("timeline", "DLG_102", 0));
			}
		#else
			return GetWindow(GetArrangeWnd(), GW_HWNDNEXT);
		#endif
	}
	return s_hwnd;
}

HWND GetTransportWnd ()
{
	static char* s_name = NULL;
	if (!s_name)
		AllocPreparedString(__localizeFunc("Transport", "DLG_188", 0), &s_name);

	if (s_name)
	{
		HWND hwnd = FindInReaper(s_name);
		if (!hwnd) hwnd = FindInReaperDockers(s_name);
		if (!hwnd) hwnd = FindInFloatingDockers(s_name);
		if (!hwnd) hwnd = FindFloating(s_name); // transport can't float by itself but search in case this changes in the future
		return hwnd;
	}
	return NULL;
}

HWND GetMixerWnd ()
{
	static char* s_name = NULL;
	if (!s_name)
		AllocPreparedString(__localizeFunc("Mixer", "DLG_151", 0), &s_name);
	return FindReaperWndByPreparedString(s_name);
}

HWND GetMixerMasterWnd ()
{
	static char* s_name = NULL;
	if (!s_name)
		AllocPreparedString(__localizeFunc("Mixer Master", "mixer", 0), &s_name);
	return FindReaperWndByPreparedString(s_name);
}

HWND GetMediaExplorerWnd ()
{
	static char* s_name = NULL;
	if (!s_name)
		AllocPreparedString(__localizeFunc("Media Explorer", "explorer", 0), &s_name);
	return FindReaperWndByPreparedString(s_name);
}

HWND GetMcpWnd ()
{
	if (HWND mixer = GetMixerWnd())
	{
		HWND hwnd = FindWindowEx(mixer, NULL, NULL, NULL);
		while (hwnd)
		{
			if ((MediaTrack*)GetWindowLongPtr(hwnd, GWLP_USERDATA) != GetMasterTrack(NULL)) // skip master track
				return hwnd;
			hwnd = FindWindowEx(mixer, hwnd, NULL, NULL);
		}
	}
	return NULL;
}

HWND GetTcpWnd ()
{
	static HWND s_hwnd = NULL;
	if (!s_hwnd)
	{
		MediaTrack* track = GetTrack(NULL, 0);
		MediaTrack* master = GetMasterTrack(NULL);
		bool IsMasterVisible = TcpVis(master) ? true : false;

		// Can't find TCP if there are no tracks in project, kinda silly to expect no tracks in a DAW but oh well... :)
		MediaTrack* firstTrack = (IsMasterVisible) ? (master) : (track ? track : master);
		if (!track && !IsMasterVisible)
		{
			PreventUIRefresh(1);
			Main_OnCommandEx(40075, 0, NULL);
		}

		// TCP hierarchy: TCP owner -> TCP hwnd -> tracks
		bool found = false;
		HWND tcpOwner = FindWindowEx(g_hwndParent, NULL, NULL, "");
		while (tcpOwner && !found)
		{
			HWND tcpHwnd = FindWindowEx(tcpOwner, NULL, NULL, "");
			while (tcpHwnd && !found)
			{
				HWND trackHwnd = FindWindowEx(tcpHwnd, NULL, NULL, "");
				while (trackHwnd && !found)
				{
					if ((MediaTrack*)GetWindowLongPtr(trackHwnd, GWLP_USERDATA) == firstTrack)
					{
						s_hwnd = tcpHwnd;
						found = true;
					}
					trackHwnd = FindWindowEx(tcpHwnd, trackHwnd, NULL, "");
				}
				tcpHwnd = FindWindowEx(tcpOwner, tcpHwnd, NULL, "");
			}
			tcpOwner = FindWindowEx(g_hwndParent, tcpOwner, NULL, "");
		}

		// Restore TCP master to previous state
		if (!track && !IsMasterVisible)
		{
			PreventUIRefresh(-1);
			Main_OnCommandEx(40075, 0, NULL);
		}
	}
	return s_hwnd;
}

HWND GetTcpTrackWnd (MediaTrack* track)
{
	HWND hwnd = GetWindow(GetTcpWnd(), GW_CHILD);
	do
	{
		if ((MediaTrack*)GetWindowLongPtr(hwnd, GWLP_USERDATA) == track)
			return hwnd;
	}
	while (hwnd = GetWindow(hwnd, GW_HWNDNEXT));

	return NULL;
}

HWND GetNotesView (void* midiEditor)
{
	if (MIDIEditor_GetMode(midiEditor) != -1)
	{
		#ifdef _WIN32
			static char* s_name  = (IsLocalized()) ? (NULL) : (const_cast<char*>(__localizeFunc("midiview", "midi_DLG_102", 0)));

			if (IsLocalized())
			{
				if (!s_name)
					AllocPreparedString(__localizeFunc("midiview", "midi_DLG_102", 0),&s_name);
				return SearchChildren(s_name,  (HWND)midiEditor);
			}
			else
				return FindWindowEx((HWND)midiEditor, NULL, NULL , s_name);
		#else
			return GetWindow(GetWindow((HWND)midiEditor, GW_CHILD), GW_HWNDNEXT);
		#endif
	}
	else
		return NULL;
}

HWND GetPianoView (void* midiEditor)
{
	if (MIDIEditor_GetMode(midiEditor) != -1)
	{
		#ifdef _WIN32
			static char* s_name  = (IsLocalized()) ? (NULL) : (const_cast<char*>(__localizeFunc("midipianoview", "midi_DLG_102", 0)));

			if (IsLocalized())
			{
				if (!s_name)
					AllocPreparedString(__localizeFunc("midipianoview", "midi_DLG_102", 0),&s_name);
				return SearchChildren(s_name,  (HWND)midiEditor);
			}
			else
				return FindWindowEx((HWND)midiEditor, NULL, NULL , s_name);
		#else
			return GetWindow((HWND)midiEditor, GW_CHILD);
		#endif
	}
	else
		return NULL;
}

MediaTrack* HwndToTrack (HWND hwnd, int* hwndContext)
{
	MediaTrack* track = NULL;
	HWND hwndParent = GetParent(hwnd);
	int context = 0;

	if (!track)
	{
		HWND tcp = GetTcpWnd();
		if (hwndParent == tcp)                                               // hwnd is a track
			track = (MediaTrack*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		else if (GetParent(hwndParent) == tcp)                               // hwnd is vu meter inside track
			track = (MediaTrack*)GetWindowLongPtr(hwndParent, GWLP_USERDATA);

		if (track)
			context = 1;
		else if (hwnd == tcp)
			context = 1;
	}

	if (!track)
	{
		HWND mcp = GetMcpWnd();
		HWND mixer = GetParent(mcp);
		HWND mixerMaster = GetMixerMasterWnd();
		HWND hwndPParent = GetParent(hwndParent);

		if (hwndParent == mcp || hwndParent == mixer || hwndParent == mixerMaster)         // hwnd is a track
			track = (MediaTrack*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		else if (hwndPParent == mcp || hwndPParent == mixer || hwndPParent == mixerMaster) // hwnd is vu meter inside track
			track = (MediaTrack*)GetWindowLongPtr(hwndParent, GWLP_USERDATA);

		if (track)
			context = 2;
		else if (hwnd == mcp || hwnd == mixerMaster)
			context = 2;
	}

	if (!ValidatePtr(track, "MediaTrack*"))
		track = NULL;

	WritePtr(hwndContext, context);
	return track;
}

TrackEnvelope* HwndToEnvelope (HWND hwnd)
{
	if (GetParent(hwnd) == GetTcpWnd())
	{
		TrackEnvelope* envelope = (TrackEnvelope*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		if (ValidatePtr(envelope, "TrackEnvelope*"))
			return envelope;
	}
	return NULL;
}

void CenterDialog (HWND hwnd, HWND target, HWND zOrder)
{
	RECT r; GetWindowRect(hwnd, &r);
	RECT r1; GetWindowRect(target, &r1);

	int hwndW = r.right - r.left;
	int hwndH = r.bottom - r.top;
	int targW = r1.right - r1.left;
	int targH = r1.bottom - r1.top;

	r.left = r1.left + (targW-hwndW)/2;
	r.top = r1.top + (targH-hwndH)/2;

	EnsureNotCompletelyOffscreen(&r);
	SetWindowPos(hwnd, zOrder, r.left, r.top, 0, 0, SWP_NOSIZE);
}

void GetMonitorRectFromPoint (const POINT& p, RECT* r)
{
	if (!r)
		return;

	#ifdef _WIN32
		if (HMONITOR monitor = MonitorFromPoint(p, MONITOR_DEFAULTTONEAREST))
		{
			MONITORINFO monitorInfo = {sizeof(MONITORINFO)};
			GetMonitorInfo(monitor, &monitorInfo);
			*r = monitorInfo.rcWork;
		}
		else
		{
			r->top  = 0;
			r->left = 0;
			r->right = 800;
			r->left  = 600;
		}
	#else
		RECT pRect = {p.x, p.y, p.x, p.y};
		SWELL_GetViewPort(r, &pRect, false);
	#endif
}

void BoundToRect (RECT& boundingRect, RECT* r)
{
	if (!r)
		return;

	if (r->top < boundingRect.top || r->bottom > boundingRect.bottom || r->left < boundingRect.left || r->right > boundingRect.right)
	{
		int w = r->right  - r->left;
		int h = r->bottom - r->top;

		if (r->top < boundingRect.top)
		{
			r->top    = boundingRect.top;
			r->bottom = boundingRect.top + h;
		}
		else if (r->bottom > boundingRect.bottom)
		{
			r->bottom = boundingRect.bottom;
			r->top    = boundingRect.bottom - h;
		}

		if (r->left < boundingRect.left)
		{
			r->left  = boundingRect.left;
			r->right = boundingRect.left + w;
		}
		else if (r->right > boundingRect.right)
		{
			r->right = boundingRect.right;
			r->left  = boundingRect.right - w;
		}
	}

	EnsureNotCompletelyOffscreen(&r); // just in case
}

/******************************************************************************
* Theming                                                                     *
******************************************************************************/
void DrawTooltip (LICE_IBitmap* bm, const char* text)
{
	if (bm)
	{
		static LICE_CachedFont* s_font = NULL;
		if (!s_font)
		{
			if (HFONT ttFont = (HFONT)SendMessage(GetTooltipWindow(),WM_GETFONT,0,0))
			{
				if (s_font = new (nothrow) LICE_CachedFont())
				{
					#ifdef _WIN32
						s_font->SetFromHFont(ttFont, LICE_FONT_FLAG_OWNS_HFONT|LICE_FONT_FLAG_FORCE_NATIVE);
					#else
						s_font->SetFromHFont(ttFont, LICE_FONT_FLAG_OWNS_HFONT);
					#endif
					s_font->SetBkMode(TRANSPARENT);
					s_font->SetTextColor(LICE_RGBA(0, 0, 0, 255));
				}
			}
		}

		if (s_font)
		{
			RECT r = {0, 0, 0, 0};
			s_font->DrawText(NULL, text, -1, &r, DT_CALCRECT);
			bm->resize(r.right + 8, r.bottom + 2);

			#ifdef _WIN32
				LICE_FillRect(bm, 1, 1, bm->getWidth(), bm->getHeight(), LICE_RGBA(255, 255, 225, 255), 1.0, LICE_BLIT_MODE_COPY);
			#else
				LICE_FillRect(bm, 1, 1, bm->getWidth(), bm->getHeight(), LICE_RGBA(255, 240, 200, 255), 1.0, LICE_BLIT_MODE_COPY);
			#endif
			LICE_DrawRect(bm, 0, 0, bm->getWidth()-1, bm->getHeight()-1, LICE_RGBA(0, 0, 0, 255), 1.0, LICE_BLIT_MODE_COPY);

			r.left   += 3;
			r.right  += 3;
			r.bottom += 1;
			r.top    += 1;
			s_font->DrawText(bm, text, -1, &r, 0);
		}
	}
}

void ThemeListViewOnInit (HWND list)
{
	if (SWS_THEMING)
	{
		SWS_ListView listView(list, NULL, 0, NULL, NULL, false, NULL, true);
		SNM_ThemeListView(&listView);
	}
}

void SetWndIcon (HWND hwnd)
{
	#ifdef _WIN32
		static HICON s_icon = NULL;
		if (!s_icon)
		{
			wchar_t path[SNM_MAX_PATH];
			if (GetModuleFileNameW(NULL, path, sizeof(path)/sizeof(wchar_t)))
				s_icon = ExtractIconW(g_hInst, path, 0); // WM_GETICON isn't working so use this instead
		}

		if (s_icon)
			SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)s_icon);
	#endif
}

bool ThemeListViewInProc (HWND hwnd, int uMsg, LPARAM lParam, HWND list, bool grid)
{
	if (SWS_THEMING)
	{
		int colors[LISTVIEW_COLORHOOK_STATESIZE];
		int columns = (grid) ? (Header_GetItemCount(ListView_GetHeader(list))) : (0);
		if (ListView_HookThemeColorsMessage(hwnd, uMsg, lParam, colors, GetWindowLong(list,GWL_ID), 0, columns))
			return true;
	}
	return false;
}

/******************************************************************************
* Height helpers (mostly for optimization, exposed here for BR_MouseUtil)     *
******************************************************************************/
void GetTrackGap (int trackHeight, int* top, int* bottom)
{
	/* Calculates the part above the item where buttons and labels *
	*  reside and retrieves user setting for the bottom gap        */

	if (top)
	{
		int label; GetConfig("labelitems2", label);
		//   draw mode        ||   name                 pitch,              gain (opposite)
		if (!GetBit(label, 3) || (!GetBit(label, 0) && !GetBit(label, 2) && GetBit(label, 4)))
			*top = 0;
		else
		{
			int labelH = abs(SNM_GetColorTheme()->mediaitem_font.lfHeight) * 11/8;
			int labelMinH; GetConfig("itemlabel_minheight", labelMinH);

			if (trackHeight - labelH < labelMinH)
			{
				*top = 0;
			}
			else
			{
				// draw label inside item if it's making item too short
				if (GetBit(label, 5) && trackHeight - labelH < ITEM_LABEL_MIN_HEIGHT)
					*top = 0;
				else
					*top = labelH;
			}
		}
	}
	if (bottom)
		GetConfig("trackitemgap", *bottom);
}

bool IsLastTakeTooTall (int itemHeight, int averageTakeHeight, int effectiveTakeCount, int* lastTakeHeight)
{
	/* When placing takes into item, REAPER just divides item height by number of takes *
	*  losing division reminder - last take gets all of those reminders. However, if    *
	*  it ends up to tall (>= 1.5 of average take height) it gets cropped               */

	int lastTakeH = itemHeight - averageTakeHeight * (effectiveTakeCount - 1);
	if (lastTakeH < (int)(averageTakeHeight * 1.5))
	{
		WritePtr(lastTakeHeight, lastTakeH);
		return false;
	}
	else
	{
		WritePtr(lastTakeHeight, averageTakeHeight);
		return true;
	}
}

int GetItemHeight (MediaItem* item, int* offsetY, int trackHeight, int trackOffsetY)
{
	/* Reaper takes into account FIPM and overlapping media item lanes *
	/* so no gotchas here besides track height checking since I_LASTH  *
	/* might return wrong value when track height is 0                 */

	if (trackHeight == 0 || !item)
	{
		WritePtr(offsetY, !item ? 0 : trackOffsetY);
		return 0;
	}

	WritePtr(offsetY, trackOffsetY + *(int*)GetSetMediaItemInfo(item, "I_LASTY", NULL));
	return *(int*)GetSetMediaItemInfo(item, "I_LASTH", NULL);
}

int GetMinTakeHeight (MediaTrack* track, int takeCount, int trackHeight, int itemHeight)
{
	/* Take lanes have minimum height, after which they are hidden and *
	*  and only active take is displayed. This height is determined on *
	*  per track basis                                                 */

	if (!track)
		return 0;

	int trackTop, trackBottom;
	GetTrackGap(trackHeight, &trackTop, &trackBottom);

	int overlappingItems; GetConfig("projtakelane", overlappingItems);
	if (GetBit(overlappingItems, 1))
	{
		if (itemHeight == (trackHeight - trackTop - trackBottom))
			overlappingItems = 0;
	}
	else
		overlappingItems = 0;

	int takeH;
	if (*(bool*)GetSetMediaTrackInfo(track, "B_FREEMODE", NULL) || overlappingItems)
	{
		int maxTakeH = (trackHeight - trackTop - trackBottom) / takeCount; // max take height when item occupies the whole track lane

		// If max take height is over limits, reaper lets takes get
		// resized to 1 px in FIPM or when in overlapping lane
		if (takeCount > TAKE_MIN_HEIGHT_COUNT)
		{
			if (maxTakeH < TAKE_MIN_HEIGHT_LOW)
				takeH = TAKE_MIN_HEIGHT_LOW;
			else
				takeH = 1;
		}
		else
		{
			if (maxTakeH < TAKE_MIN_HEIGHT_HIGH)
				takeH = TAKE_MIN_HEIGHT_HIGH;
			else
				takeH = 1;
		}
	}
	else
	{
		if (takeCount > TAKE_MIN_HEIGHT_COUNT)
			takeH = TAKE_MIN_HEIGHT_LOW;
		else
			takeH = TAKE_MIN_HEIGHT_HIGH;
	}

	return takeH;
}

int GetTakeHeight (MediaItem_Take* take, MediaItem* item, int id, int* offsetY, bool averagedLast)
{
	MediaItem* validItem = (item) ? (item) : (GetMediaItemTake_Item(take));
	int trackOffset;
	int trackHeight = GetTrackHeight(GetMediaItem_Track(validItem), &trackOffset);
	return GetTakeHeight(take, item, id, offsetY, averagedLast, trackHeight, trackOffset);
}

int GetTakeHeight (MediaItem_Take* take, MediaItem* item, int id, int* offsetY, bool averagedLast, int trackHeight, int trackOffset)
{
	/* Calculates take height using all the input variables. Exists as *
	*  a separate function since sometimes trackHeight and trackOffset *
	*  will be known prior to calling this, so let the caller optimize *
	*  to prevent calculating the same thing twice                     */

	if ((!take && !item) || (take && item))
	{
		WritePtr(offsetY, 0);
		return 0;
	}
	MediaItem_Take* validTake = (take) ? (take) : (GetTake(item, id));
	MediaItem*      validItem = (item) ? (item) : (GetMediaItemTake_Item(take));
	MediaTrack*     track     = GetMediaItem_Track(validItem);

	// This gets us the offset to start of the item
	int takeOffset = trackOffset;
	int itemH = GetItemHeight(validItem, &takeOffset, trackHeight, takeOffset);

	int takeLanes; GetConfig("projtakelane", takeLanes);
	int takeH = 0;

	// Take lanes displayed
	if (GetBit(takeLanes, 0))
	{
		int effTakeCount;
		int effTakeId = GetEffectiveTakeId(take, item, id, &effTakeCount); // using original pointers on purpose here
		takeH = itemH / effTakeCount; // average take height

		if (takeH < GetMinTakeHeight(track, effTakeCount, trackHeight, itemH))
		{
			if (GetActiveTake(validItem) == validTake)
				takeH = itemH;
			else
				takeH = 0;
		}
		else
		{
			if (effTakeId >= 0)
				takeOffset = takeOffset + takeH * effTakeId;

			// Last take gets leftover pixels from integer division
			if (!averagedLast && (effTakeId == effTakeCount - 1))
				IsLastTakeTooTall(itemH, takeH, effTakeCount, &takeH);
		}
	}
	// Take lanes not displayed
	else
	{
		if (GetActiveTake(validItem) == validTake)
			takeH = itemH;
		else
			takeH = 0;
	}

	WritePtr(offsetY, takeOffset);
	return takeH;
}