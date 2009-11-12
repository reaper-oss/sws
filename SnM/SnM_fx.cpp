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
#include "SnM_Actions.h"


// Get the state of an FX -or- Set the state of one or several FX(s)
// _mode:	1=Get or set online/offline state
//			2=Get or set bypass state
//			3=Toggle all online/offline states except _fx (_value ignored)
//			4=Toggle all bypass states except _fx (_value ignored)
//			5=Toggle online/offline state for _fx (_value ignored)
//			6=Toggle bypass states for _fx (_value ignored)
// _tr
// _fx
// _value:	NULL to get, non-NULL for toggle, value to be set otherwise
// returns:	when getting, the value from the RPP chunk (1=bypass or offline) 
//			or -1 if failed
//			when setting -1 (error not yet managed)
int getSetFXState(int _mode, MediaTrack * _tr, int _fx, int * _value)
{
	// Some checks (_type is checked later)
	if (_tr && _fx >= 0)
	{
		WDL_String fxout;

		char* cData = GetSetObjectState(_tr, NULL);
		if (cData)
		{
			WDL_String curLine;
			fxout.Set("");
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
							switch (_mode)
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
							switch (_mode)
							{
								case 1: // bypass
									fxout.Append("BYPASS ");
									fxout.Append(lp.gettoken_str(1));
									fxout.AppendFormatted(3, " %d\n", *_value);
									appended = true;
									break;
								case 2: // offline/online
									fxout.Append("BYPASS ");
									fxout.AppendFormatted(2, "%d ", *_value);
									fxout.Append(lp.gettoken_str(2));
									fxout.Append("\n");
									appended = true;
									break;
								case 5: // toggle bypass
									fxout.Append("BYPASS ");
									fxout.Append(lp.gettoken_str(1));
									fxout.AppendFormatted(3, " %d\n", !lp.gettoken_int(2));
									appended = true;
									break;
								case 6: // toggle offline/online
									fxout.Append("BYPASS ");
									fxout.AppendFormatted(2, "%d ", !lp.gettoken_int(1));
									fxout.Append(lp.gettoken_str(2));
									fxout.Append("\n");
									appended = true;
									break;
								default:
									break;
							}
						}
					}
					else
					{
						// Set 
						if (_value)
						{
							switch (_mode)
							{
								case 3: // bypass "except"
									fxout.Append("BYPASS ");
									fxout.Append(lp.gettoken_str(1));
									fxout.AppendFormatted(3, " %d\n", !lp.gettoken_int(2));
									appended = true;
									break;
								case 4: // offline/online "except"
									fxout.Append("BYPASS ");
									fxout.AppendFormatted(2, "%d ", !lp.gettoken_int(1));
									fxout.Append(lp.gettoken_str(2));
									fxout.Append("\n");
									appended = true;
									break;
								default:
									break;
							}
						}
					}

					parsedFX++;
				}

				if (_value && !appended && lp.getnumtokens())
				{
					fxout.Append(curLine.Get());
					fxout.Append("\n");
				}
			}
			while (pEOL);

			FreeHeapPtr(cData);

			// Sets the new state
			if (_value && fxout.GetLength())
			{
				GetSetObjectState(_tr, fxout.Get());
			}
		}
	}
	return -1;
}

void toggleFXStateSelectedTracks(int _mode, int _fx, const char * _undoMsg)
{
	bool updated = false;

	// check that the mode is indeed a toggle one
	if (_mode >= 3 && _mode <= 6)
	{
		for (int i = 0; i <= GetNumTracks(); i++)
		{
			MediaTrack* tr = CSurf_TrackFromID(i, false);
			if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			{
				int fxId = _fx - 1;
				if (fxId < 0)
					fxId = TrackFX_GetCount(tr) + _fx; // Could support "second to last" action with -2, etc
				getSetFXState(_mode, tr, fxId, &g_i0);
				updated = true;
			}
		}
	}

	// above is "toggle" use of getSetFXState(), below is a "get + set"
	// example use of it (not used: less efficient but same result)
	else if (_mode >= 1 && _mode <= 2)
	{
		for (int i = 0; i <= GetNumTracks(); i++)
		{
			MediaTrack* tr = CSurf_TrackFromID(i, false);
			if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			{
				int fxId = _fx - 1;
				if (fxId < 0)
					fxId = TrackFX_GetCount(tr) + _fx; // Could support "second to last" action with -2, etc

				// get
				int toggleCurrent = getSetFXState(_mode, tr, fxId, NULL);
				if (toggleCurrent >= 0)
				{
					toggleCurrent = !toggleCurrent;

					// set
					getSetFXState(_mode, tr, fxId, &toggleCurrent);
					updated = true;
				}
			}
		}
	}

	// Undo point
	if (updated && _undoMsg)
		Undo_OnStateChangeEx(_undoMsg, UNDO_STATE_ALL/*UNDO_STATE_FX*/, -1);
}

void toggleFXOfflineSelectedTracks(COMMAND_T* _ct)
{
	toggleFXStateSelectedTracks(5, (int)_ct->user, SNMSWS_ZAP(_ct));
}

void toggleFXBypassSelectedTracks(COMMAND_T* _ct)
{
	toggleFXStateSelectedTracks(6, (int)_ct->user, SNMSWS_ZAP(_ct));
}

void toggleExceptFXOfflineSelectedTracks(COMMAND_T* _ct)
{
	toggleFXStateSelectedTracks(3, (int)_ct->user, SNMSWS_ZAP(_ct));
}

void toggleExceptFXBypassSelectedTracks(COMMAND_T* _ct)
{
	toggleFXStateSelectedTracks(4, (int)_ct->user, SNMSWS_ZAP(_ct));
}

void toggleAllFXsOfflineSelectedTracks(COMMAND_T* _ct)
{
	// We use the "except mode" but with an unreachable fx number
	toggleFXStateSelectedTracks(3, 0xFFFF, SNMSWS_ZAP(_ct));
}

void toggleAllFXsBypassSelectedTracks(COMMAND_T* _ct)
{
	// We use the "except mode" but with an unreachable fx number
	toggleFXStateSelectedTracks(4, 0xFFFF, SNMSWS_ZAP(_ct));
}


