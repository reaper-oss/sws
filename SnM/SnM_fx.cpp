/******************************************************************************
/ SnM_fx.cpp
/
/ Copyright (c) 2009 Tim Payne (SWS), JF Bédague (S&M)
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


int getSetFXOnline(int _type, MediaTrack * _tr, int _fx, int * _value)
{
	// Some checks (_type is checked later)
	if (_tr && _fx >= 0)
	{
		WDL_String _fxout;

		char* cData = GetSetObjectState(_tr, NULL);
		if (cData)
		{
			WDL_String curLine;
			_fxout.Set("");
			char* pEOL = cData-1;
			int iDepth = 0;
			int parsedFX = 0;
			do
			{
				char* pLine = pEOL+1;
				pEOL = strchr(pLine, '\n');
				curLine.Set(pLine, (int)(pEOL ? pEOL-pLine : 0));

				LineParser lp(false);
				lp.parse(curLine.Get());

				// Get the depth
				if (lp.getnumtokens())
				{
					if (lp.gettoken_str(0)[0] == '<')
						iDepth++;
					else if (lp.gettoken_str(0)[0] == '>')
						iDepth--;
				}

				// Some checks for "more acsendant compatibility"
				bool appended = false;
				if (iDepth == 2 && lp.getnumtokens() == 3 && 
					strcmp(lp.gettoken_str(0), "BYPASS") == 0)
				{
					if (parsedFX == _fx)
					{
						// Get only
						if (!_value)
						{
							FreeHeapPtr(cData);
							switch (_type)
							{
								case 1:
									return lp.gettoken_int(2);
								case 2:
									return lp.gettoken_int(1);
								default:
									return -1;
							}
						}
						// Set
						{
							appended = true;
							switch (_type)
							{
								case 1:
									_fxout.Append("BYPASS ");
									_fxout.Append(lp.gettoken_str(1));
									_fxout.AppendFormatted(3, " %d\n", *_value);
									break;
								case 2:
									_fxout.Append("BYPASS ");
									_fxout.AppendFormatted(2, "%d ", *_value);
									_fxout.Append(lp.gettoken_str(2));
									_fxout.Append("\n");
									break;
								default:
									appended = false;
									break;
							}
						}
					}
					parsedFX++;
				}

				if (_value && !appended && lp.getnumtokens())
				{
					_fxout.Append(curLine.Get());
					_fxout.Append("\n");
				}
			}
			while (pEOL);

			FreeHeapPtr(cData);

			// Sets the new state
			if (_value && _fxout.GetLength())
			{
				GetSetObjectState(_tr, _fxout.Get());
			}
		}
	}
	return -1;
}

void toggleFXOfflineSelectedTracks(COMMAND_T* _ct)
{
	for (int i = 0; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			int fxId = (int)_ct->user - 1;
			if (fxId < 0)
				fxId = TrackFX_GetCount(tr) + _ct->user; // Could support "second to last" action with -2, etc
			int toggleCurrent = getSetFXOnline(1, tr, fxId, NULL);
			if (toggleCurrent >= 0)
			{
				toggleCurrent = !toggleCurrent;
				getSetFXOnline(1, tr, fxId, &toggleCurrent);

				// Undo point
				char undoStr[64];
				sprintf(undoStr, "Toggle FX %d online/offline for track %d", fxId+1, i);
				Undo_OnStateChangeEx(undoStr, UNDO_STATE_FX, -1);
			}
		}
	}
}

void toggleFXBypassSelectedTracks(COMMAND_T* _ct)
{
	for (int i = 0; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			int fxId = (int)_ct->user - 1;
			if (fxId < 0)
				fxId = TrackFX_GetCount(tr) + _ct->user;
			int toggleCurrent = getSetFXOnline(2, tr, fxId, NULL);
			if (toggleCurrent >= 0)
			{
				toggleCurrent = !toggleCurrent;
				getSetFXOnline(2, tr, fxId, &toggleCurrent);

				// Undo point
				char undoStr[64];
				sprintf(undoStr, "Toggle FX %d bypass for track %d", fxId+1, i);
				Undo_OnStateChangeEx(undoStr, UNDO_STATE_FX, -1);
			}
		}
	}
}
