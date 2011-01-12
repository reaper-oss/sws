/******************************************************************************
/ Adam.cpp
/
/ Copyright (c) 2010 Adam Wathan
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
#include "Adam.h"
#include "Context.h"
#include "TrackSel.h"
#include "../Xenakios/XenakiosExts.h"


//#include "Context.cpp"

// Globals for copy and paste

static bool g_AWCopyFlag;
static double g_AWCopySelLen;


///////////////////////////////////////////////////////////////////////////////
// Fills gaps aka Beat Detective
// for Adam to get started...
///////////////////////////////////////////////////////////////////////////////

void ReadFillGapsIniFile(char* cmdString,
						 char* triggerPad=NULL, char* fadeLength=NULL, char* maxGap=NULL, 
						 char* maxStretch=NULL, char* presTrans=NULL, char* transFade=NULL, 
						 int* fadeShape=NULL, int* markErrors=NULL, int* stretch=NULL, int* trans=NULL)
{
	char tmp[128] = "";
	WDL_String cmd;
	int s,t;

	GetPrivateProfileString("SWS","FillGapsTriggerPad","5",tmp,128,get_ini_file());
	cmd.AppendFormatted(128, "%s,", tmp);
	if (triggerPad)
		strncpy(triggerPad, tmp, 128);

	GetPrivateProfileString("SWS","FillGapsFadeLen","5",tmp,128,get_ini_file());
	cmd.AppendFormatted(128, "%s,", tmp);
	if (fadeLength)
		strncpy(fadeLength, tmp, 128);

	GetPrivateProfileString("SWS","FillGapsMaxGap","15",tmp,128,get_ini_file());
	cmd.AppendFormatted(128, "%s,", tmp);
	if (maxGap)
		strncpy(maxGap, tmp, 128);

	GetPrivateProfileString("SWS","FillGapsStretch","1",tmp,128,get_ini_file());
	s = atoi(tmp);
	if (stretch)
		*stretch = s; 

	GetPrivateProfileString("SWS","FillGapsMaxStretch","0.5",tmp,128,get_ini_file());
	cmd.AppendFormatted(128, "%s,", !s ? "1.0" : tmp);
	if (maxStretch)
		strncpy(maxStretch, tmp, 128);

	GetPrivateProfileString("SWS","FillGapsTrans","1",tmp,128,get_ini_file());
	t = atoi(tmp);
	if (trans)
		*trans = t;

	GetPrivateProfileString("SWS","FillGapsPresTrans","35",tmp,128,get_ini_file());
	cmd.AppendFormatted(128, "%s,", (!s || !t) ? "0" : tmp);
	if (presTrans)
		strncpy(presTrans, tmp, 128);

	GetPrivateProfileString("SWS","FillGapsTransFade","5",tmp,128,get_ini_file());
	cmd.AppendFormatted(128, "%s,", tmp);
	if (transFade)
		strncpy(transFade, tmp, 128);

	GetPrivateProfileString("SWS","FillGapsFadeShape","0",tmp,128,get_ini_file());
	cmd.AppendFormatted(128, "%s,", tmp);
	if (fadeShape)
		*fadeShape = atoi(tmp); 

	GetPrivateProfileString("SWS","FillGapsMarkErr","1",tmp,128,get_ini_file());
	cmd.AppendFormatted(128, "%s", tmp);
	if (markErrors)
		*markErrors = atoi(tmp); 

	if (cmdString)
		strncpy(cmdString, cmd.Get(), 128);
}

void SaveFillGapsIniFile(char* triggerPad, char* fadeLength, char* maxGap, 
						 char* maxStretch, char* presTrans, char* transFade, 
						 int fadeShape, int markErrors, int stretch, int trans)
{
	char tmp[128] = "";
	WritePrivateProfileString("SWS","FillGapsTriggerPad",triggerPad,get_ini_file());
	WritePrivateProfileString("SWS","FillGapsFadeLen",fadeLength,get_ini_file());
	WritePrivateProfileString("SWS","FillGapsMaxGap",maxGap,get_ini_file());
	sprintf(tmp,"%d",stretch);
	WritePrivateProfileString("SWS","FillGapsStretch",tmp,get_ini_file());
	WritePrivateProfileString("SWS","FillGapsMaxStretch",maxStretch,get_ini_file());
	sprintf(tmp,"%d",trans);
	WritePrivateProfileString("SWS","FillGapsTrans",tmp,get_ini_file());
	WritePrivateProfileString("SWS","FillGapsPresTrans",presTrans,get_ini_file());
	WritePrivateProfileString("SWS","FillGapsTransFade",transFade,get_ini_file());
	sprintf(tmp,"%d",fadeShape);
	WritePrivateProfileString("SWS","FillGapsFadeShape",tmp,get_ini_file());
	sprintf(tmp,"%d",markErrors);
	WritePrivateProfileString("SWS","FillGapsMarkErr",tmp,get_ini_file());
}

HWND g_strtchHFader = 0;
double g_strtchHFaderValue = 0.5f;
WDL_DLGRET AWFillGapsProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	const char cWndPosKey[] = "Fill gaps Window Pos"; 
	switch(Message)
	{
        case WM_INITDIALOG :
		{		
			char triggerPad[128], fadeLength[128], maxGap[128], maxStretch[128], presTrans[128], transFade[128];
			int fadeShape, markErrors, stretch, trans;
			ReadFillGapsIniFile(NULL, triggerPad, fadeLength, maxGap, maxStretch, presTrans, transFade, &fadeShape, &markErrors, &stretch, &trans);

			SetDlgItemText(hwnd,IDC_TRIG_PAD,triggerPad);
			SetDlgItemText(hwnd,IDC_XFADE_LEN1,fadeLength);
			SetDlgItemText(hwnd,IDC_MAX_GAP,maxGap);
			SetDlgItemText(hwnd,IDC_TRANS_LEN,presTrans);
			SetDlgItemText(hwnd,IDC_XFADE_LEN2,transFade);
			CheckDlgButton(hwnd, IDC_CHECK1, !!markErrors);
			CheckDlgButton(hwnd, IDC_CHECK2, !!stretch);
			CheckDlgButton(hwnd, IDC_CHECK3, !!trans);

#ifdef _WIN32
			RECT r;
			GetWindowRect(GetDlgItem(hwnd, IDC_SLIDER1), &r);
			ScreenToClient(hwnd, (LPPOINT)&r);
			ScreenToClient(hwnd, ((LPPOINT)&r)+1);
			g_strtchHFader = CreateWindowEx(WS_EX_LEFT, "REAPERhfader", "DLGFADER1",
				WS_CHILD | WS_VISIBLE | TBS_VERT,
				r.left, r.top, r.right-r.left, r.bottom-r.top, hwnd, NULL, g_hInst, NULL);
#else
			g_strtchHFader = GetDlgItem(hwnd, IDC_SLIDER1);
			ShowWindow(g_strtchHFader, SW_SHOW);
#endif
			if (g_strtchHFader) {
				SendMessage(g_strtchHFader,TBM_SETTIC,0,500);
				SendMessage(g_strtchHFader,TBM_SETPOS,1,(LPARAM)(atof(maxStretch)*1000));
			}

			int x = (int)SendDlgItemMessage(hwnd,IDC_FADE_SHAPE,CB_ADDSTRING,0,(LPARAM)"Equal Gain");
			SendDlgItemMessage(hwnd,IDC_FADE_SHAPE,CB_SETITEMDATA,x,0);
			x = (int)SendDlgItemMessage(hwnd,IDC_FADE_SHAPE,CB_ADDSTRING,0,(LPARAM)"Equal Power");
			SendDlgItemMessage(hwnd,IDC_FADE_SHAPE,CB_SETITEMDATA,x,1);
			SendDlgItemMessage(hwnd,IDC_FADE_SHAPE,CB_SETCURSEL,fadeShape,0);

			RestoreWindowPos(hwnd, cWndPosKey, false);
			SetFocus(GetDlgItem(hwnd, IDC_TRIG_PAD));
			if (g_strtchHFader)
				PostMessage(hwnd, WM_HSCROLL, 0, (LPARAM)g_strtchHFader); // indirectly refreshes stretch fader %
			PostMessage(hwnd, WM_COMMAND, IDC_CHECK2, 0); // indirectly refreshes grayed states
			PostMessage(hwnd, WM_COMMAND, IDC_CHECK3, 0); 
			return 0;
		}
		break;
        case WM_HSCROLL:
		{
			int pos=(int)SendMessage((HWND)lParam,TBM_GETPOS,0,0);
			if (g_strtchHFader && (HWND)lParam==g_strtchHFader) {
				char txt[128];
				sprintf(txt,"%d%%", (int)floor(pos/10 + 0.5));
				SetDlgItemText(hwnd, IDC_STRTCH_LIMIT, txt);
				g_strtchHFaderValue = ((double)pos)/1000;
			}
		}
		break;
		case WM_COMMAND :
		{
            switch(LOWORD(wParam))
            {
                case IDOK:
				case IDC_SAVE:
				{
					char triggerPad[128], fadeLength[128], maxGap[128], maxStretch[128], presTrans[128], transFade[128];
					int fadeShape, markErrors, stretch, trans;
					GetDlgItemText(hwnd,IDC_TRIG_PAD,triggerPad,128);
					GetDlgItemText(hwnd,IDC_XFADE_LEN1,fadeLength,128);
					GetDlgItemText(hwnd,IDC_MAX_GAP,maxGap,128);
					GetDlgItemText(hwnd,IDC_TRANS_LEN,presTrans,128);
					GetDlgItemText(hwnd,IDC_XFADE_LEN2,transFade,128);
					if (g_strtchHFader)
						sprintf(maxStretch, "%.2f", g_strtchHFaderValue);
					markErrors = IsDlgButtonChecked(hwnd, IDC_CHECK1);
					stretch = IsDlgButtonChecked(hwnd, IDC_CHECK2);
					trans = IsDlgButtonChecked(hwnd, IDC_CHECK3);
					fadeShape = (int)SendDlgItemMessage(hwnd,IDC_FADE_SHAPE,CB_GETCURSEL,0,0);

					SaveFillGapsIniFile(triggerPad, fadeLength, maxGap, maxStretch, presTrans, transFade, fadeShape, markErrors, stretch, trans);

					if(wParam == IDOK)
					{
						char prms[128];
						_snprintf(prms, 128, "%s,%s,%s,%s,%s,%s,%d,%d", 
							triggerPad, fadeLength, maxGap, !stretch ? "1.0" : maxStretch, (!stretch || !trans) ? "0" : presTrans, 
							transFade, fadeShape, markErrors);
						AWFillGapsAdv("Fill gaps between selected items", prms);
					}
					return 0;
				}
				break;
				case IDCANCEL:
				{
					ShowWindow(hwnd, SW_HIDE);
					return 0;
				}
				break;
				case IDC_CHECK2:
				{
					bool strtchEnable = (IsDlgButtonChecked(hwnd, IDC_CHECK2) == 1);
					bool transEnable = (IsDlgButtonChecked(hwnd, IDC_CHECK3) == 1);
					if (g_strtchHFader)
						EnableWindow(g_strtchHFader, strtchEnable);
					EnableWindow(GetDlgItem(hwnd, IDC_CHECK3), strtchEnable);
					EnableWindow(GetDlgItem(hwnd, IDC_TRANS_LEN), strtchEnable && transEnable);
					EnableWindow(GetDlgItem(hwnd, IDC_XFADE_LEN2), strtchEnable && transEnable);
				}
				break;
				case IDC_CHECK3:
				{
					bool transEnable = (IsDlgButtonChecked(hwnd, IDC_CHECK3) == 1);
					EnableWindow(GetDlgItem(hwnd, IDC_TRANS_LEN), transEnable);
					EnableWindow(GetDlgItem(hwnd, IDC_XFADE_LEN2), transEnable);
				}
				break;
			}
		}
		break;
		case WM_DESTROY:
			SaveWindowPos(hwnd, cWndPosKey);
			break; 
	}

	return 0;
}

void AWFillGapsAdv(COMMAND_T* t)
{
	if ((int)t->user)
	{
		char prms[128];
		ReadFillGapsIniFile(prms);
		AWFillGapsAdv(SWSAW_CMD_SHORTNAME(t), prms);
	}
	else
	{
		static HWND hwnd = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_AW_ITEM_SMOOTHING), g_hwndParent, AWFillGapsProc);
		ShowWindow(hwnd, SW_SHOW);
		SetFocus(hwnd);
	}
}

void AWFillGapsAdv(const char* title, char* retVals)
{
	// Divided by 1000 to convert to milliseconds except maxStretch
	double triggerPad = (atof(strtok(retVals, ",")))/1000;
	double fadeLength = (atof(strtok(NULL, ",")))/1000;
	double maxGap = (atof(strtok(NULL, ",")))/1000;
	double maxStretch = atof(strtok(NULL, ","));
	double presTrans = (atof(strtok(NULL, ",")))/1000;
	double transFade = (atof(strtok(NULL, ",")))/1000;
	int fadeShape = atoi(strtok(NULL, ","));
	int markErrors = atoi(strtok(NULL, ","));

	if ((triggerPad < 0) || (fadeLength < 0) || (maxGap < 0) || (maxStretch < 0) || (maxStretch > 1) || (presTrans < 0) || (transFade < 0) || (fadeShape < 0) || (fadeShape > 5))
	{
		//ShowMessageBox("Don't use such stupid values, try again.","Invalid Input",0);
		MessageBox(g_hwndParent, "All values must be non-negative, Maximum Stretch must be a value from 0 to 1 and Fade Shape must be a value from 0 to 5.", "Item Smoothing Input Error", MB_OK);
		return;
	}
	
	
	

	
	// Run loop for every track in project
	for (int trackIndex = 0; trackIndex < GetNumTracks(); trackIndex++)
	{
		
		// Gets current track in loop
		MediaTrack* track = GetTrack(0, trackIndex);
		
		// Run loop for every item on track
		
		int itemCount = GetTrackNumMediaItems(track);
		
		for (int itemIndex = 0; itemIndex < (itemCount - 1); itemIndex++)
		{
			
			MediaItem* item1 = GetTrackMediaItem(track, itemIndex);
			MediaItem* item2 = GetTrackMediaItem(track, itemIndex + 1);
			// errorFlag = 0;
			
			
			// BEGIN FIX OVERLAP CODE --------------------------------------------------------------
			
			// If first item is selected selected, run this loop
			if (GetMediaItemInfo_Value(item1, "B_UISEL"))
			{
				// Get various pieces of info for the 2 items
				double item1Start = GetMediaItemInfo_Value(item1, "D_POSITION");
				double item1Length = GetMediaItemInfo_Value(item1, "D_LENGTH");
				double item1End = item1Start + item1Length;
				
				double item2Start = GetMediaItemInfo_Value(item2, "D_POSITION");
				
				
				// If the first item overlaps the second item
				if (item1End > item2Start)
				{
					item1Length = item2Start - item1Start;
				}
				
				// If the second item is also selected, trim the trigger pad off of item 1
				if (GetMediaItemInfo_Value(item2, "B_UISEL"))
				{
					item1Length -= triggerPad;
				}
				
				SetMediaItemInfo_Value(item1, "D_LENGTH", item1Length);					
			}
			
			// END FIX OVERLAP CODE ----------------------------------------------------------------
			
			
			// BEGIN TIMESTRETCH CODE---------------------------------------------------------------
			
			// If stretching is enabled
			if (maxStretch < 1)
			{
				
				// Only stretch if both items are selected, avoids stretching unselected items or the last selected item
				if (GetMediaItemInfo_Value(item1, "B_UISEL") && GetMediaItemInfo_Value(item2, "B_UISEL"))
				{
					
					// Get various pieces of info for the 2 items
					double item1Start = GetMediaItemInfo_Value(item1, "D_POSITION");
					double item1Length = GetMediaItemInfo_Value(item1, "D_LENGTH");
					double item1End = item1Start + item1Length;
					
					double item2Start = GetMediaItemInfo_Value(item2, "D_POSITION");
					
					// Calculate gap between items
					double gap = item2Start - item1End;
					
					// If gap is bigger than the max allowable gap
					if (gap > maxGap)
					{
						
						// If preserve transient is enabled, split item at preserve point
						if (presTrans > 0)
						{
							
							// Get snap offset for item1
							double item1SnapOffset = GetMediaItemInfo_Value(item1, "D_SNAPOFFSET");
							
							// Calculate position of item1 transient
							double item1TransPos = item1Start + item1SnapOffset;
							
							// Split point is (presTrans) after the transient point
							// Subtract fade length because easier to change item end then item start
							// (don't need take loop to adjust all start offsets)
							double splitPoint = item1TransPos + presTrans - transFade;
							
							// Check for default item fades
							double defItemFades = *(double*)(GetConfigVar("deffadelen"));
							bool fadeFlag = 0;
							
							if (defItemFades > 0)
							{
								fadeFlag = 1;
								Main_OnCommand(41194,0);
							}
							
							// Split item1 at the split point
							MediaItem* item1B = SplitMediaItem(item1, splitPoint);
							
							// Revert item fades
							if (fadeFlag)
							{
								Main_OnCommand(41194,0);
							}
							
							// Get new item1 length after split
							item1Length = GetMediaItemInfo_Value(item1, "D_LENGTH") + transFade;
							
							// Create overlap of 'transFade'
							SetMediaItemInfo_Value(item1, "D_LENGTH", item1Length);
							
							// Crossfade the overlap
							SetMediaItemInfo_Value(item1, "D_FADEOUTLEN_AUTO", transFade);
							SetMediaItemInfo_Value(item1B, "D_FADEINLEN_AUTO", transFade);
							
							// Set Fade Shapes
							SetMediaItemInfo_Value(item1, "C_FADEOUTSHAPE", fadeShape);
							SetMediaItemInfo_Value(item1B, "C_FADEINSHAPE", fadeShape);
							
							// Set the stretched half to be item 1 so loop continues properly
							item1 = item1B;
							
							// Get item1 info since item1 is now item1B
							item1Start = GetMediaItemInfo_Value(item1, "D_POSITION");
							item1Length = GetMediaItemInfo_Value(item1, "D_LENGTH");
							item1End = item1Start + item1Length;
							
							// Increase itemIndex and itemCount since split added item
							itemIndex += 1;
							itemCount += 1;
						}
						
						// Calculate distance between item1Start and the beginning of the maximum allowable gap
						double item1StartToGap = (item2Start - maxGap) - item1Start;
						
						// Calculate amount to stretch
						double stretchPercent = (item1Length/(item1StartToGap));
						
						if (stretchPercent < maxStretch)
						{
							stretchPercent = maxStretch;
							
						}
						
						item1Length *= (1/stretchPercent);
						
						SetMediaItemInfo_Value(item1, "D_LENGTH", item1Length);	
						
						for (int takeIndex = 0; takeIndex < GetMediaItemNumTakes(item1); takeIndex++)
						{
							MediaItem_Take* currentTake = GetMediaItemTake(item1, takeIndex);
							SetMediaItemTakeInfo_Value(currentTake, "D_PLAYRATE", stretchPercent);
						}
					}	
				}	
			}
			
			// END TIME STRETCH CODE ---------------------------------------------------------------
			
			
			// BEGIN FILL GAPS CODE ----------------------------------------------------------------
			
			// If both items selected, run this loop
			if (GetMediaItemInfo_Value(item1, "B_UISEL") && GetMediaItemInfo_Value(item2, "B_UISEL"))
			{ 
				
				double item1Start = GetMediaItemInfo_Value(item1, "D_POSITION");
				double item1Length = GetMediaItemInfo_Value(item1, "D_LENGTH");
				double item1End = item1Start + item1Length;
				
				double item2Start = GetMediaItemInfo_Value(item2, "D_POSITION");
				double item2Length = GetMediaItemInfo_Value(item2, "D_LENGTH");
				double item2SnapOffset = GetMediaItemInfo_Value(item2, "D_SNAPOFFSET");
				
				
				
				// If there is a gap, fill it and also add an overlap to the left that is "fadeLength" long
				if (item2Start >= item1End)
				{
					double item2StartDiff = item2Start - item1End + fadeLength;
					item2Length += item2StartDiff;
					
					// Calculate gap between items
					double gap = item2Start - item1End;
					
					// Check to see if gap is bigger than maxGap, even after time stretching
					// Make sure to account for triggerPad since it is subtracted before this point and shouldn't count towards the gap
					if (gap > (maxGap + triggerPad))
					{
						// If gap is big and mark errors is enabled, add a marker
						if (markErrors == 1)
						{
							AddProjectMarker(NULL, false, item1End, NULL, "Possible Artifact", NULL);
						}
						
					}
					
					
					// Adjust start offset for all takes in item2 to account for fill
					for (int takeIndex = 0; takeIndex < GetMediaItemNumTakes(item2); takeIndex++)
					{
						MediaItem_Take* currentTake = GetMediaItemTake(item2, takeIndex);
						double startOffset = GetMediaItemTakeInfo_Value(currentTake, "D_STARTOFFS");
						startOffset -= item2StartDiff;
						SetMediaItemTakeInfo_Value(currentTake, "D_STARTOFFS", startOffset);
					}
					
					// Finally trim the item to fill the gap and adjust the snap offset
					SetMediaItemInfo_Value(item2, "D_POSITION", (item1End - fadeLength));
					SetMediaItemInfo_Value(item2, "D_LENGTH", item2Length);
					SetMediaItemInfo_Value(item2, "D_SNAPOFFSET", (item2SnapOffset + item2StartDiff));
					
					// Crossfade the overlap between the two items
					SetMediaItemInfo_Value(item1, "D_FADEOUTLEN_AUTO", fadeLength);
					SetMediaItemInfo_Value(item2, "D_FADEINLEN_AUTO", fadeLength);
					
					// Set Fade Shapes
					SetMediaItemInfo_Value(item1, "C_FADEOUTSHAPE", fadeShape);
					SetMediaItemInfo_Value(item2, "C_FADEINSHAPE", fadeShape);
				}
			}
			
			// END FILL GAPS CODE ------------------------------------------------------------------
			
		}
			
	}
	
	
	UpdateTimeline();
	Undo_OnStateChangeEx(title, UNDO_STATE_ITEMS | UNDO_STATE_MISCCFG, -1);
}


void AWFillGapsQuick(COMMAND_T* t)
{
		
	
	// Run loop for every track in project
	for (int trackIndex = 0; trackIndex < GetNumTracks(); trackIndex++)
	{
		
		// Gets current track in loop
		MediaTrack* track = GetTrack(0, trackIndex);
		
		// Run loop for every item on track
		
		int itemCount = GetTrackNumMediaItems(track);
		
		for (int itemIndex = 0; itemIndex < (itemCount - 1); itemIndex++)
		{
			
			MediaItem* item1 = GetTrackMediaItem(track, itemIndex);
			MediaItem* item2 = GetTrackMediaItem(track, itemIndex + 1);
			
		
			// BEGIN FIX OVERLAP CODE --------------------------------------------------------------
			
			// If first item is selected, run this loop
			if (GetMediaItemInfo_Value(item1, "B_UISEL"))
			{
				// Get various pieces of info for the 2 items
				double item1Start = GetMediaItemInfo_Value(item1, "D_POSITION");
				double item1Length = GetMediaItemInfo_Value(item1, "D_LENGTH");
				double item1End = item1Start + item1Length;
			
				double item2Start = GetMediaItemInfo_Value(item2, "D_POSITION");
			
				// If the first item overlaps the second item, trim the first item
				if (item1End > (item2Start))
				{
					item1Length = item2Start - item1Start;
										
					SetMediaItemInfo_Value(item1, "D_LENGTH", item1Length);
				}
				
			}
			
			// END FIX OVERLAP CODE ----------------------------------------------------------------
			
			// BEGIN FILL GAPS CODE ----------------------------------------------------------------
			
			// If both items selected, run this loop
			if (GetMediaItemInfo_Value(item1, "B_UISEL") && GetMediaItemInfo_Value(item2, "B_UISEL"))
			{ 
			
				double item1Start = GetMediaItemInfo_Value(item1, "D_POSITION");
				double item1Length = GetMediaItemInfo_Value(item1, "D_LENGTH");
				double item1End = item1Start + item1Length;
			
				double item2Start = GetMediaItemInfo_Value(item2, "D_POSITION");
				double item2Length = GetMediaItemInfo_Value(item2, "D_LENGTH");
				double item2SnapOffset = GetMediaItemInfo_Value(item2, "D_SNAPOFFSET");
		
				// If there is a gap, fill it and also add an overlap to the left that is "fadeLength" long
				if (item2Start >= item1End)
				{
					double item2StartDiff = item2Start - item1End;
					item2Length += item2StartDiff;
					
					// Adjust start offset for all takes in item2 to account for fill
					for (int takeIndex = 0; takeIndex < GetMediaItemNumTakes(item2); takeIndex++)
						{
							MediaItem_Take* currentTake = GetMediaItemTake(item2, takeIndex);
							double startOffset = GetMediaItemTakeInfo_Value(currentTake, "D_STARTOFFS");
							startOffset -= item2StartDiff;
							SetMediaItemTakeInfo_Value(currentTake, "D_STARTOFFS", startOffset);
						}
						
					// Finally trim the item to fill the gap and adjust the snap offset
					SetMediaItemInfo_Value(item2, "D_POSITION", (item1End));
					SetMediaItemInfo_Value(item2, "D_LENGTH", item2Length);
					SetMediaItemInfo_Value(item2, "D_SNAPOFFSET", (item2SnapOffset + item2StartDiff));
					
				}
			}
			
			// END FILL GAPS CODE ------------------------------------------------------------------
			
		}
		
	}

	
	
	UpdateTimeline();
	Undo_OnStateChangeEx(SWSAW_CMD_SHORTNAME(t), UNDO_STATE_ITEMS, -1);
}

void AWFillGapsQuickXFade(COMMAND_T* t)
{
	double fadeLength = fabs(*(double*)GetConfigVar("deffadelen"));
	
	// Run loop for every track in project
	for (int trackIndex = 0; trackIndex < GetNumTracks(); trackIndex++)
	{
		
		// Gets current track in loop
		MediaTrack* track = GetTrack(0, trackIndex);
		
		// Run loop for every item on track
		
		int itemCount = GetTrackNumMediaItems(track);
		
		for (int itemIndex = 0; itemIndex < (itemCount - 1); itemIndex++)
		{
			
			MediaItem* item1 = GetTrackMediaItem(track, itemIndex);
			MediaItem* item2 = GetTrackMediaItem(track, itemIndex + 1);
			
			
			// BEGIN FIX OVERLAP CODE --------------------------------------------------------------
			
			// If first item is selected, run this loop
			if (GetMediaItemInfo_Value(item1, "B_UISEL"))
			{
				// Get various pieces of info for the 2 items
				double item1Start = GetMediaItemInfo_Value(item1, "D_POSITION");
				double item1Length = GetMediaItemInfo_Value(item1, "D_LENGTH");
				double item1End = item1Start + item1Length;
				
				double item2Start = GetMediaItemInfo_Value(item2, "D_POSITION");
				
				// If the first item overlaps the second item, trim the first item
				if (item1End > (item2Start))
				{
					
					item1Length = item2Start - item1Start;
										
					SetMediaItemInfo_Value(item1, "D_LENGTH", item1Length);
				}
			}
			
			// END FIX OVERLAP CODE ----------------------------------------------------------------
			
			// BEGIN FILL GAPS CODE ----------------------------------------------------------------
			
			// If both items selected, run this loop
			if (GetMediaItemInfo_Value(item1, "B_UISEL") && GetMediaItemInfo_Value(item2, "B_UISEL"))
			{ 
				
				double item1Start = GetMediaItemInfo_Value(item1, "D_POSITION");
				double item1Length = GetMediaItemInfo_Value(item1, "D_LENGTH");
				double item1End = item1Start + item1Length;
				
				double item2Start = GetMediaItemInfo_Value(item2, "D_POSITION");
				double item2Length = GetMediaItemInfo_Value(item2, "D_LENGTH");
				double item2SnapOffset = GetMediaItemInfo_Value(item2, "D_SNAPOFFSET");
				
				// If there is a gap, fill it and also add an overlap to the left that is "fadeLength" long
				if (item2Start >= item1End)
				{
					double item2StartDiff = item2Start - item1End + fadeLength;
					item2Length += item2StartDiff;
					
					// Adjust start offset for all takes in item2 to account for fill
					for (int takeIndex = 0; takeIndex < GetMediaItemNumTakes(item2); takeIndex++)
					{
						MediaItem_Take* currentTake = GetMediaItemTake(item2, takeIndex);
						double startOffset = GetMediaItemTakeInfo_Value(currentTake, "D_STARTOFFS");
						startOffset -= item2StartDiff;
						SetMediaItemTakeInfo_Value(currentTake, "D_STARTOFFS", startOffset);
					}
					
					// Finally trim the item to fill the gap and adjust the snap offset
					SetMediaItemInfo_Value(item2, "D_POSITION", (item1End - fadeLength));
					SetMediaItemInfo_Value(item2, "D_LENGTH", item2Length);
					SetMediaItemInfo_Value(item2, "D_SNAPOFFSET", (item2SnapOffset + item2StartDiff));
					
					// Crossfade the overlap between the two items
					SetMediaItemInfo_Value(item1, "D_FADEOUTLEN_AUTO", fadeLength);
					SetMediaItemInfo_Value(item2, "D_FADEINLEN_AUTO", fadeLength);
					
				}
			}
			
			// END FILL GAPS CODE ------------------------------------------------------------------
			
		}
		
	}
	
	
	
	UpdateTimeline();
	Undo_OnStateChangeEx(SWSAW_CMD_SHORTNAME(t), UNDO_STATE_ITEMS, -1);
}


void AWFixOverlaps(COMMAND_T* t)
{
	
	// Run loop for every track in project
	for (int trackIndex = 0; trackIndex < GetNumTracks(); trackIndex++)
	{
		
		// Gets current track in loop
		MediaTrack* track = GetTrack(0, trackIndex);
		
		// Run loop for every item on track
		
		int itemCount = GetTrackNumMediaItems(track);
		
		for (int itemIndex = 0; itemIndex < (itemCount - 1); itemIndex++)
		{
			
			MediaItem* item1 = GetTrackMediaItem(track, itemIndex);
			MediaItem* item2 = GetTrackMediaItem(track, itemIndex + 1);
			
			
			// BEGIN FIX OVERLAP CODE --------------------------------------------------------------
			
			// If first item is selected, run this loop
			if (GetMediaItemInfo_Value(item1, "B_UISEL"))
			{
				// Get various pieces of info for the 2 items
				double item1Start = GetMediaItemInfo_Value(item1, "D_POSITION");
				double item1Length = GetMediaItemInfo_Value(item1, "D_LENGTH");
				double item1End = item1Start + item1Length;
				
				double item2Start = GetMediaItemInfo_Value(item2, "D_POSITION");
				
				// If the first item overlaps the second item, trim the first item
				if (item1End > (item2Start))
				{
					
					item1Length = item2Start - item1Start;
										
					SetMediaItemInfo_Value(item1, "D_LENGTH", item1Length);
				}
			
			}
			
			// END FIX OVERLAP CODE ----------------------------------------------------------------
		}
		
	}
	
	
	
	UpdateTimeline();
	Undo_OnStateChangeEx(SWSAW_CMD_SHORTNAME(t), UNDO_STATE_ITEMS, -1);
}









void AWRecordConditional(COMMAND_T* t)
{
	double t1, t2;
	GetSet_LoopTimeRange(false, false, &t1, &t2, false);
	
	if (*(int*)GetConfigVar("projrecmode") != 0) // 0 is item punch mode
	{	
		if (t1 != t2)
		{
			Main_OnCommand(40076, 0); //Set record mode to time selection auto punch
		}
		
		
		
		else 
		{
			Main_OnCommand(40252, 0); //Set record mode to normal
		}
	}
	
	Main_OnCommand(1013,0); // Transport: Record
	
	UpdateTimeline();
	Undo_OnStateChangeEx(SWSAW_CMD_SHORTNAME(t), UNDO_STATE_ITEMS, -1);
}



void AWRecordConditional2(COMMAND_T* t)
{
	double t1, t2;
	GetSet_LoopTimeRange(false, false, &t1, &t2, false);
	
	
	if (t1 != t2)
	{
		Main_OnCommand(40076, 0); //Set record mode to time selection auto punch
	}
	
	else if (CountSelectedMediaItems(0) != 0)
	{
		Main_OnCommand(40253, 0); //Set record mode to auto punch selected items
	}
	
	else 
	{
		Main_OnCommand(40252, 0); //Set record mode to normal
	}
	
	
	Main_OnCommand(1013,0); // Transport: Record
	
	UpdateTimeline();
	Undo_OnStateChangeEx(SWSAW_CMD_SHORTNAME(t), UNDO_STATE_ITEMS, -1);
}


void AWRecordAutoGroup(COMMAND_T* t)
{
	if (GetPlayState() & 4)
	{
		Main_OnCommand(1013, 0); //If recording, toggle recording and group items
		int numItems = CountSelectedMediaItems(0);
		
		if (numItems > 1)
		{
			Main_OnCommand(40032, 0); //Group selected items
		}
	}
	else
	{
		Main_OnCommand(1013, 0); //If Transport is playing or stopped, Record
	}
	
	UpdateTimeline();
	Undo_OnStateChangeEx(SWSAW_CMD_SHORTNAME(t), UNDO_STATE_ITEMS, -1);
}

void AWRecordConditionalAutoGroup2(COMMAND_T* t)
{
	double t1, t2;
	GetSet_LoopTimeRange(false, false, &t1, &t2, false);
	
	
	if (t1 != t2)
	{
		Main_OnCommand(40076, 0); //Set record mode to time selection auto punch
	}
	
	else if (CountSelectedMediaItems(0) != 0)
	{
		Main_OnCommand(40253, 0); //Set record mode to time selection auto punch
	}
	
	else 
	{
		Main_OnCommand(40252, 0); //Set record mode to time selection auto punch
	}
	
	
	if (GetPlayState() & 4)
	{
		Main_OnCommand(1013, 0); //If recording, toggle recording and group items
		int numItems = CountSelectedMediaItems(0);
		
		if (numItems > 1)
		{
			Main_OnCommand(40032, 0); //Group selected items
		}
	}
	else
	{
		Main_OnCommand(1013, 0); //If Transport is playing or stopped, Record
	}
	
	UpdateTimeline();
	Undo_OnStateChangeEx(SWSAW_CMD_SHORTNAME(t), UNDO_STATE_ITEMS, -1);
}




void AWRecordConditionalAutoGroup(COMMAND_T* t)
{
	double t1, t2;
	GetSet_LoopTimeRange(false, false, &t1, &t2, false);
	
	if (*(int*)GetConfigVar("projrecmode") != 0) // 0 is item punch mode
	{	
		if (t1 != t2)
		{
			Main_OnCommand(40076, 0); //Set record mode to time selection auto punch
		}
		
		else 
		{
			Main_OnCommand(40252, 0); //Set record mode to time selection auto punch
		}
	}
	
	if (GetPlayState() & 4)
	{
		Main_OnCommand(1013, 0); //If recording, toggle recording and group items
		int numItems = CountSelectedMediaItems(0);
		
		if (numItems > 1)
		{
			Main_OnCommand(40032, 0); //Group selected items
		}
	}
	else
	{
		Main_OnCommand(1013, 0); //If Transport is playing or stopped, Record
	}
	
	UpdateTimeline();
	Undo_OnStateChangeEx(SWSAW_CMD_SHORTNAME(t), UNDO_STATE_ITEMS, -1);
}

void AWPlayStopAutoGroup(COMMAND_T* t)
{
	if (GetPlayState() & 4)
	{
		Main_OnCommand(1016, 0); //If recording, Stop
		int numItems = CountSelectedMediaItems(0);
		
		if (numItems > 1)
		{
			Main_OnCommand(40032, 0); //Group selected items
		}
	}
	else
	{
		Main_OnCommand(40044, 0); //If Transport is playing or stopped, Play/Stop
	}
	
	UpdateTimeline();
	Undo_OnStateChangeEx(SWSAW_CMD_SHORTNAME(t), UNDO_STATE_ITEMS, -1);
}

void AWSelectToEnd(COMMAND_T* t)
{
	double projEnd = 0;
	
	// Find end position of last item in project
	
	for (int trackIndex = 0; trackIndex < GetNumTracks(); trackIndex++)
	{
		MediaTrack* track = GetTrack(0, trackIndex);
		
		int itemNum = GetTrackNumMediaItems(track) - 1;
		
		MediaItem* item = GetTrackMediaItem(track, itemNum);	
		
		double itemEnd = GetMediaItemInfo_Value(item, "D_POSITION") + GetMediaItemInfo_Value(item, "D_LENGTH");
		
		if (itemEnd > projEnd)
		{
			projEnd = itemEnd;
		}
	}
	
	double selStart = GetCursorPosition();
	
	GetSet_LoopTimeRange2(0, 1, 0, &selStart, &projEnd, 0);
	
	Main_OnCommand(40718, 0); // Select all items in time selection on selected tracks
	
	UpdateTimeline();
	Undo_OnStateChangeEx(SWSAW_CMD_SHORTNAME(t), UNDO_STATE_ITEMS | UNDO_STATE_MISCCFG, -1);
}






/* Sorry this is experimental and sucks right now
 
 void AWRecordQuickPunch(COMMAND_T* t)
 {
 if (GetPlayState() & 4)
 {
 double selStart;
 double selEnd;
 double playPos;
 
 GetSet_LoopTimeRange2(0, 0, 0, &selStart, &selEnd, 0);
 
 playPos = GetPlayPosition();
 
 if (selStart > playPos)
 {
 GetSet_LoopTimeRange2(0, 1, 0, &playPos, &selEnd, 0);
 }
 else if (selStart < playPos)
 {
 GetSet_LoopTimeRange2(0, 1, 0, &selStart, &playPos, 0);
 }
 
 }
 else if (GetPlayState() == 0)
 {
 Main_OnCommand(40076, 0); //Set to time selection punch mode
 
 double selStart = 6000;
 double selEnd = 6001;
 
 GetSet_LoopTimeRange2(0, 1, 0, &selStart, &selEnd, 0);
 
 Main_OnCommand(1013, 0);
 }
 
 UpdateTimeline();
 Undo_OnStateChangeEx(SWSAW_CMD_SHORTNAME(t), UNDO_STATE_ITEMS | UNDO_STATE_MISCCFG, -1);
 }
 */







void AWFadeSelection(COMMAND_T* t)
{
	double dFadeLen; 
	double dEdgeAdj;
	double dEdgeAdj1;
	double dEdgeAdj2;
	bool leftFlag=false;
	bool rightFlag=false;
	
	double selStart;
	double selEnd;
	
	GetSet_LoopTimeRange2(0, 0, 0, &selStart, &selEnd, 0);
	
	
	
	for (int iTrack = 1; iTrack <= GetNumTracks(); iTrack++)
	{
		MediaTrack* tr = CSurf_TrackFromID(iTrack, false);
		for (int iItem1 = 0; iItem1 < GetTrackNumMediaItems(tr); iItem1++)
		{
			MediaItem* item1 = GetTrackMediaItem(tr, iItem1);
			if (*(bool*)GetSetMediaItemInfo(item1, "B_UISEL", NULL))
			{
				double dStart1 = *(double*)GetSetMediaItemInfo(item1, "D_POSITION", NULL);
				double dEnd1   = *(double*)GetSetMediaItemInfo(item1, "D_LENGTH", NULL) + dStart1;
				
				leftFlag = false;
				rightFlag = false;
				
				
				
				// Try to match up the end with the start of the other selected media items
				for (int iItem2 = 0; iItem2 < GetTrackNumMediaItems(tr); iItem2++)
				{
					
					MediaItem* item2 = GetTrackMediaItem(tr, iItem2);
					
					if (item1 != item2 && *(bool*)GetSetMediaItemInfo(item2, "B_UISEL", NULL))
					{
						
						double dStart2 = *(double*)GetSetMediaItemInfo(item2, "D_POSITION", NULL);
						double dEnd2   = *(double*)GetSetMediaItemInfo(item2, "D_LENGTH", NULL) + dStart2;
						//double dTest = fabs(dEnd1 - dStart2);
						
						// Need a tolerance for "items are adjacent". Also check to see if split is inside time selection
						if ((fabs(dEnd1 - dStart2) < SWS_ADJACENT_ITEM_THRESHOLD))
						{	// Found a matching item
							
							// If split is within selection and selection is within total item selection
							if ((selStart > dStart1) && (selEnd < dEnd2) && (selStart < dStart2) && (selEnd > dEnd1))
							{
								dFadeLen = selEnd - selStart;
								dEdgeAdj1 = selEnd - dEnd1;
								dEdgeAdj2 = dStart2 - selStart;

								MediaItem_Take* activeTake = GetActiveTake(item2);
								if (activeTake && dEdgeAdj2 > *(double*)GetSetMediaItemTakeInfo(activeTake, "D_STARTOFFS", NULL))
								{	
									dFadeLen -= dEdgeAdj2;
									dEdgeAdj2 = *(double*)GetSetMediaItemTakeInfo(activeTake, "D_STARTOFFS", NULL);
									dFadeLen += dEdgeAdj2;
								}
								
								// Move the edges around and set the crossfades
								double dLen1 = dEnd1 - dStart1 + dEdgeAdj1;
								*(double*)GetSetMediaItemInfo(item1, "D_LENGTH", NULL) = dLen1;
								GetSetMediaItemInfo(item1, "D_FADEOUTLEN_AUTO", &dFadeLen);
								
								double dLen2 = *(double*)GetSetMediaItemInfo(item2, "D_LENGTH", NULL);
								dStart2 -= dEdgeAdj2;
								dLen2   += dEdgeAdj2;
								*(double*)GetSetMediaItemInfo(item2, "D_POSITION", NULL) = dStart2;
								*(double*)GetSetMediaItemInfo(item2, "D_LENGTH", NULL) = dLen2;
								GetSetMediaItemInfo(item2, "D_FADEINLEN_AUTO", &dFadeLen);
								double dSnapOffset2 = *(double*)GetSetMediaItemInfo(item2, "D_SNAPOFFSET", NULL);
								if (dSnapOffset2)
								{	// Only adjust the snap offset if it's non-zero
									dSnapOffset2 += dEdgeAdj2;
									GetSetMediaItemInfo(item2, "D_SNAPOFFSET", &dSnapOffset2);
								}
								
								for (int iTake = 0; iTake < GetMediaItemNumTakes(item2); iTake++)
								{
									MediaItem_Take* take = GetMediaItemTake(item2, iTake);
									if (take)
									{
										double dOffset = *(double*)GetSetMediaItemTakeInfo(take, "D_STARTOFFS", NULL);
										dOffset -= dEdgeAdj2;
										GetSetMediaItemTakeInfo(take, "D_STARTOFFS", &dOffset);
									}
								}
								rightFlag=true;
								break;
								
							}
							// If selection does not surround split or does not exist
							
							
							else
							{
								dFadeLen = fabs(*(double*)GetConfigVar("deffadelen")); // Abs because neg value means "not auto"
								dEdgeAdj1 = dFadeLen / 2.0;
								
								dEdgeAdj2 = dFadeLen / 2.0;
								
								// Need to ensure that there's "room" to move the start of the second item back
								// Check all of the takes' start offset before doing any "work"
								MediaItem_Take* activeTake = GetActiveTake(item2);								
								if (activeTake && dEdgeAdj2 > *(double*)GetSetMediaItemTakeInfo(activeTake, "D_STARTOFFS", NULL))
								{
									dEdgeAdj2 = *(double*)GetSetMediaItemTakeInfo(activeTake, "D_STARTOFFS", NULL);
									dEdgeAdj1 = dFadeLen - dEdgeAdj2;
								}
								
								
								// We're all good, move the edges around and set the crossfades
								double dLen1 = dEnd1 - dStart1 + dEdgeAdj1;
								*(double*)GetSetMediaItemInfo(item1, "D_LENGTH", NULL) = dLen1;
								GetSetMediaItemInfo(item1, "D_FADEOUTLEN_AUTO", &dFadeLen);
								
								double dLen2 = *(double*)GetSetMediaItemInfo(item2, "D_LENGTH", NULL);
								dStart2 -= dEdgeAdj2;
								dLen2   += dEdgeAdj2;
								*(double*)GetSetMediaItemInfo(item2, "D_POSITION", NULL) = dStart2;
								*(double*)GetSetMediaItemInfo(item2, "D_LENGTH", NULL) = dLen2;
								GetSetMediaItemInfo(item2, "D_FADEINLEN_AUTO", &dFadeLen);
								double dSnapOffset2 = *(double*)GetSetMediaItemInfo(item2, "D_SNAPOFFSET", NULL);
								if (dSnapOffset2)
								{	// Only adjust the snap offset if it's non-zero
									dSnapOffset2 += dEdgeAdj2;
									GetSetMediaItemInfo(item2, "D_SNAPOFFSET", &dSnapOffset2);
								}
								
								for (int iTake = 0; iTake < GetMediaItemNumTakes(item2); iTake++)
								{
									MediaItem_Take* take = GetMediaItemTake(item2, iTake);
									if (take)
									{
										double dOffset = *(double*)GetSetMediaItemTakeInfo(take, "D_STARTOFFS", NULL);
										dOffset -= dEdgeAdj2;
										GetSetMediaItemTakeInfo(take, "D_STARTOFFS", &dOffset);
									}
								}
								rightFlag=true;
								break;
							}
							
							
							
						}
						
						
						// If items are adjacent in opposite direction, turn flag on. Actual fades handled when situation is found
						// in reverse, but still need to flag it here
						
						else if ((fabs(dEnd2 - dStart1) < SWS_ADJACENT_ITEM_THRESHOLD))
						{
							leftFlag=true;
						}
						
						
						// if items already overlap (item 1 on left and item 2 on right)
						
						else if ((dStart2 < dEnd1) && (dEnd1 < dEnd2) && (dStart1 < dStart2)) 
						{
							
							
							
							// If split is within selection and selection is within total item selection
							if ((selStart > dStart1) && (selEnd < dEnd2) && (selEnd > dStart2) && (selStart < dEnd1))
							{
								dFadeLen = selEnd - selStart;
								dEdgeAdj1 = selEnd - dEnd1;
								
								dEdgeAdj2 = dStart2 - selStart;
								
								
								// Need to ensure that there's "room" to move the start of the second item back
								// Check all of the takes' start offset before doing any "work"
								MediaItem_Take* activeTake = GetActiveTake(item2);								
								if (activeTake && dEdgeAdj2 > *(double*)GetSetMediaItemTakeInfo(activeTake, "D_STARTOFFS", NULL))
								{
									dFadeLen -= dEdgeAdj2;
									dEdgeAdj2 = *(double*)GetSetMediaItemTakeInfo(activeTake, "D_STARTOFFS", NULL);
									dFadeLen += dEdgeAdj2;
								}
								
								
								// We're all good, move the edges around and set the crossfades
								double dLen1 = dEnd1 - dStart1 + dEdgeAdj1;
								*(double*)GetSetMediaItemInfo(item1, "D_LENGTH", NULL) = dLen1;
								GetSetMediaItemInfo(item1, "D_FADEOUTLEN_AUTO", &dFadeLen);
								
								double dLen2 = *(double*)GetSetMediaItemInfo(item2, "D_LENGTH", NULL);
								dStart2 -= dEdgeAdj2;
								dLen2   += dEdgeAdj2;
								*(double*)GetSetMediaItemInfo(item2, "D_POSITION", NULL) = dStart2;
								*(double*)GetSetMediaItemInfo(item2, "D_LENGTH", NULL) = dLen2;
								GetSetMediaItemInfo(item2, "D_FADEINLEN_AUTO", &dFadeLen);
								double dSnapOffset2 = *(double*)GetSetMediaItemInfo(item2, "D_SNAPOFFSET", NULL);
								if (dSnapOffset2)
								{	// Only adjust the snap offset if it's non-zero
									dSnapOffset2 += dEdgeAdj2;
									GetSetMediaItemInfo(item2, "D_SNAPOFFSET", &dSnapOffset2);
								}
								
								for (int iTake = 0; iTake < GetMediaItemNumTakes(item2); iTake++)
								{
									MediaItem_Take* take = GetMediaItemTake(item2, iTake);
									if (take)
									{
										double dOffset = *(double*)GetSetMediaItemTakeInfo(take, "D_STARTOFFS", NULL);
										dOffset -= dEdgeAdj2;
										GetSetMediaItemTakeInfo(take, "D_STARTOFFS", &dOffset);
									}
								}
								
								// Set both flags, prevents bad behavior with "trim" fade when selection only overlaps left side of crossfade
								// Normally the leftFlag wouldn't get set so the trim fade code creates a fade in
								// No harm in leftFlag here because it has been verified that the selection does not extend past the left
								// edge anyways.
								leftFlag=true;
								rightFlag=true;
								break;
								
							}
							
							
							// If selection does not surround split or does not exist
							else
							{
								dFadeLen = dEnd1 - dStart2;
								dEdgeAdj = 0;
								
								// Need to ensure that there's "room" to move the start of the second item back
								// Check all of the takes' start offset before doing any "work"
								int iTake;
								for (iTake = 0; iTake < GetMediaItemNumTakes(item2); iTake++)
									if (dEdgeAdj > *(double*)GetSetMediaItemTakeInfo(GetMediaItemTake(item2, iTake), "D_STARTOFFS", NULL))
										break;
								if (iTake < GetMediaItemNumTakes(item2))
									continue;	// Keep looking
								
								// We're all good, move the edges around and set the crossfades
								double dLen1 = dEnd1 - dStart1 + dEdgeAdj;
								*(double*)GetSetMediaItemInfo(item1, "D_LENGTH", NULL) = dLen1;
								GetSetMediaItemInfo(item1, "D_FADEOUTLEN_AUTO", &dFadeLen);
								
								double dLen2 = *(double*)GetSetMediaItemInfo(item2, "D_LENGTH", NULL);
								dStart2 -= dEdgeAdj;
								dLen2   += dEdgeAdj;
								*(double*)GetSetMediaItemInfo(item2, "D_POSITION", NULL) = dStart2;
								*(double*)GetSetMediaItemInfo(item2, "D_LENGTH", NULL) = dLen2;
								GetSetMediaItemInfo(item2, "D_FADEINLEN_AUTO", &dFadeLen);
								double dSnapOffset2 = *(double*)GetSetMediaItemInfo(item2, "D_SNAPOFFSET", NULL);
								if (dSnapOffset2)
								{	// Only adjust the snap offset if it's non-zero
									dSnapOffset2 += dEdgeAdj;
									GetSetMediaItemInfo(item2, "D_SNAPOFFSET", &dSnapOffset2);
								}
								
								for (iTake = 0; iTake < GetMediaItemNumTakes(item2); iTake++)
								{
									MediaItem_Take* take = GetMediaItemTake(item2, iTake);
									if (take)
									{
										double dOffset = *(double*)GetSetMediaItemTakeInfo(take, "D_STARTOFFS", NULL);
										dOffset -= dEdgeAdj;
										GetSetMediaItemTakeInfo(take, "D_STARTOFFS", &dOffset);
									}
								}
								rightFlag=true;
								break;
							}
							
							
							
						}
						
						
						
						// If the items overlap backwards ie item 2 is before item 1, turn on leftFlag
						// Actual fade calculations and stuff will be handled when the overlap gets discovered the opposite way
						// but this prevents other weird behavior
						
						
						else if ((dStart1 < dEnd2) && (dStart2 < dStart1) && (dEnd1 > dEnd2)) 
						{
							leftFlag=true;
						}
						
						
						
						// If there's a gap between two adjacent items, item 1 before item 2
						else if ((dEnd1 < dStart2))
						{
							
							// If selection starts inside item1, crosses gap and ends inside item 2
							if ((selStart > dStart1) && (selStart < dEnd1) && (selEnd > dStart2) && (selEnd < dEnd2))
							{
								dFadeLen = selEnd - selStart;
								dEdgeAdj1 = selEnd - dEnd1;
								dEdgeAdj2 = dStart2 - selStart;
								
								
								// Need to ensure that there's "room" to move the start of the second item back
								// Check all of the takes' start offset before doing any "work"
								int iTake;
								
								
								for (iTake = 0; iTake < GetMediaItemNumTakes(item2); iTake++)
									if (dEdgeAdj2 > *(double*)GetSetMediaItemTakeInfo(GetMediaItemTake(item2, iTake), "D_STARTOFFS", NULL))
									{
										dFadeLen -= dEdgeAdj2;
										
										dEdgeAdj2 = *(double*)GetSetMediaItemTakeInfo(GetMediaItemTake(item2, iTake), "D_STARTOFFS", NULL);
										
										dFadeLen += dEdgeAdj2;
									}
								if (iTake < GetMediaItemNumTakes(item2))
									continue;	// Keep looking
								
								
								
								// We're all good, move the edges around and set the crossfades
								double dLen1 = dEnd1 - dStart1 + dEdgeAdj1;
								*(double*)GetSetMediaItemInfo(item1, "D_LENGTH", NULL) = dLen1;
								GetSetMediaItemInfo(item1, "D_FADEOUTLEN_AUTO", &dFadeLen);
								
								double dLen2 = *(double*)GetSetMediaItemInfo(item2, "D_LENGTH", NULL);
								dStart2 -= dEdgeAdj2;
								dLen2   += dEdgeAdj2;
								
								*(double*)GetSetMediaItemInfo(item2, "D_POSITION", NULL) = dStart2;
								*(double*)GetSetMediaItemInfo(item2, "D_LENGTH", NULL) = dLen2;
								GetSetMediaItemInfo(item2, "D_FADEINLEN_AUTO", &dFadeLen);
								double dSnapOffset2 = *(double*)GetSetMediaItemInfo(item2, "D_SNAPOFFSET", NULL);
								if (dSnapOffset2)
								{	// Only adjust the snap offset if it's non-zero
									dSnapOffset2 += dEdgeAdj2;
									GetSetMediaItemInfo(item2, "D_SNAPOFFSET", &dSnapOffset2);
								}
								
								for (iTake = 0; iTake < GetMediaItemNumTakes(item2); iTake++)
								{
									MediaItem_Take* take = GetMediaItemTake(item2, iTake);
									if (take)
									{
										double dOffset = *(double*)GetSetMediaItemTakeInfo(take, "D_STARTOFFS", NULL);
										dOffset -= dEdgeAdj2;
										GetSetMediaItemTakeInfo(take, "D_STARTOFFS", &dOffset);
									}
								}
								rightFlag=true;
								break;
								
							}
						}
						
						// Else if there's a gap but the items are in reverse order, set the opposite flag but let the fades
						// get taken care of when it finds them "item 1 (gap) item 2"
						
						else if ((dEnd2 < dStart1))
						{
							if ((selStart > dStart2) && (selStart < dEnd2) && (selEnd > dStart1) && (selEnd < dEnd1))
							{
								leftFlag=true;
								
								//Check for selected items in between item 1 and 2
								
								for (int iItem3 = 0; iItem3 < GetTrackNumMediaItems(tr); iItem3++)
								{
									
									MediaItem* item3 = GetTrackMediaItem(tr, iItem3);
									
									if (item1 != item2 && item2 != item3 && *(bool*)GetSetMediaItemInfo(item3, "B_UISEL", NULL))
									{
										
										double dStart3 = *(double*)GetSetMediaItemInfo(item3, "D_POSITION", NULL);
										double dEnd3   = *(double*)GetSetMediaItemInfo(item3, "D_LENGTH", NULL) + dStart3;
										
										// If end of item 3 is between end of item 2 and start of item 1, reset the flag
										
										if (dEnd3 > dEnd2 && dEnd3 <= dStart1)
										{
											leftFlag = false;
										}
										
									}
								}
							}
						}
					}
				}
				
				if (!(leftFlag))
				{
					double fadeLength;
					
					if ((selStart <= dStart1) && (selEnd > dStart1) && (selEnd < dEnd1) && (!(rightFlag)))
					{			
						fadeLength = selEnd - dStart1;
						SetMediaItemInfo_Value(item1, "D_FADEINLEN", fadeLength);
						SetMediaItemInfo_Value(item1, "D_FADEINLEN_AUTO", 0.0);
					}
					
					else if (selStart <= dStart1 && selEnd >= dEnd1)
					{
						fadeLength = fabs(*(double*)GetConfigVar("deffadelen")); // Abs because neg value means "not auto"
						SetMediaItemInfo_Value(item1, "D_FADEINLEN", fadeLength);
						
					}
					
					else if (selStart > dStart1 && selEnd < dEnd1)
					{
						double dFadeIn = selStart - dStart1;
						//double dFadeOut = dEnd1 - selEnd;
						
						SetMediaItemInfo_Value(item1, "D_FADEINLEN", dFadeIn);
						SetMediaItemInfo_Value(item1, "D_FADEINLEN_AUTO", 0.0);
						//SetMediaItemInfo_Value(item1, "D_FADEOUTLEN_AUTO", dFadeOut);
					}
					
					///* Might break shit, replaces D and G in PT
					else if ((selStart < dStart1 && selEnd < dStart1) || (selStart > dEnd1 && selEnd > dEnd1) || (selStart == selEnd))
					{
						double cursorPos = GetCursorPosition();
						double dLength1 = *(double*)GetSetMediaItemInfo(item1, "D_LENGTH", NULL);
						
						
						if (cursorPos > dStart1 && cursorPos < (dStart1 + (0.5 * dLength1)))
						{
							fadeLength = cursorPos - dStart1;
							SetMediaItemInfo_Value(item1, "D_FADEINLEN", fadeLength);
							SetMediaItemInfo_Value(item1, "D_FADEINLEN_AUTO", 0.0);
						}
						
					}
					//*/
				}
				
				if (!(rightFlag))
				{
					double fadeLength;
					
					if ((selStart > dStart1) && (selEnd >= dEnd1) && (selStart < dEnd1) && (!(leftFlag)))
					{			
						fadeLength = dEnd1 - selStart;
						SetMediaItemInfo_Value(item1, "D_FADEOUTLEN", fadeLength);
						SetMediaItemInfo_Value(item1, "D_FADEOUTLEN_AUTO", 0.0);
					}
					else if (selStart <= dStart1 && selEnd >= dEnd1)
					{
						fadeLength = fabs(*(double*)GetConfigVar("deffadelen")); // Abs because neg value means "not auto"
						SetMediaItemInfo_Value(item1, "D_FADEOUTLEN", fadeLength);
					}
					
					else if (selStart > dStart1 && selEnd < dEnd1)
					{
						//double dFadeIn = selStart - dStart1;
						double dFadeOut = dEnd1 - selEnd;
						
						//SetMediaItemInfo_Value(item1, "D_FADEINLEN_AUTO", dFadeIn);
						SetMediaItemInfo_Value(item1, "D_FADEOUTLEN", dFadeOut);
						SetMediaItemInfo_Value(item1, "D_FADEOUTLEN_AUTO", 0.0);
					}
					
					
					///* Might break shit, replaces D and G in PT
					else if ((selStart < dStart1 && selEnd < dStart1) || (selStart > dEnd1 && selEnd > dEnd1) || (selStart == selEnd))
					{
						double cursorPos = GetCursorPosition();
						double dLength1 = *(double*)GetSetMediaItemInfo(item1, "D_LENGTH", NULL);
						
						
						if (cursorPos > (dStart1 + (0.5 * dLength1)) && cursorPos < dEnd1)
						{
							fadeLength = dEnd1 - cursorPos;
							SetMediaItemInfo_Value(item1, "D_FADEOUTLEN", fadeLength);
							SetMediaItemInfo_Value(item1, "D_FADEOUTLEN_AUTO", 0.0);
						}
						
					}
					//*/
				}		
			}
		}
	}
	
	UpdateTimeline();
	Undo_OnStateChangeEx(SWSAW_CMD_SHORTNAME(t), UNDO_STATE_ITEMS, -1);
	
}





void AWTrimCrop(COMMAND_T* t)
{
	
	double selStart;
	double selEnd;
	
	GetSet_LoopTimeRange2(0, 0, 0, &selStart, &selEnd, 0);
	
	if (selStart != selEnd)
	{
		Main_OnCommand(40508, 0); // If there's a time selection, just run native Trim action because I am lazy
	}
	else // Otherwise trim to cursor
	{
		double cursorPos = GetCursorPosition();
		
		for (int iTrack = 1; iTrack <= GetNumTracks(); iTrack++)
		{
			MediaTrack* tr = CSurf_TrackFromID(iTrack, false);
			for (int iItem1 = 0; iItem1 < GetTrackNumMediaItems(tr); iItem1++)
			{
				MediaItem* item1 = GetTrackMediaItem(tr, iItem1);
				if (*(bool*)GetSetMediaItemInfo(item1, "B_UISEL", NULL))
				{
					double dStart1 = *(double*)GetSetMediaItemInfo(item1, "D_POSITION", NULL);
					double dLen1 = *(double*)GetSetMediaItemInfo(item1, "D_LENGTH", NULL);
					
					if (cursorPos > dStart1 && cursorPos < (dStart1 + (0.5 * dLen1)))
					{
						Main_OnCommand(40511, 0);
					}
					else if (cursorPos < (dStart1 + dLen1) && cursorPos > (dStart1 + (0.5 * dLen1)))
					{
						Main_OnCommand(40512, 0);
					}
					
				}
			}
		}
	}
	
	UpdateTimeline();
	Undo_OnStateChangeEx(SWSAW_CMD_SHORTNAME(t), UNDO_STATE_ITEMS, -1);
	
}







// In Progress



void AWTrimFill(COMMAND_T* t)
{
	
	double selStart;
	double selEnd;
	bool leftFlag = false;
	bool rightFlag = false;
	
	GetSet_LoopTimeRange2(0, 0, 0, &selStart, &selEnd, 0);
	
	double cursorPos = GetCursorPosition();
	
	for (int iTrack = 1; iTrack <= GetNumTracks(); iTrack++)
	{
		MediaTrack* tr = CSurf_TrackFromID(iTrack, false);
		for (int iItem1 = 0; iItem1 < GetTrackNumMediaItems(tr); iItem1++)
		{
			MediaItem* item1 = GetTrackMediaItem(tr, iItem1);
			if (*(bool*)GetSetMediaItemInfo(item1, "B_UISEL", NULL))
			{
				double dStart1 = *(double*)GetSetMediaItemInfo(item1, "D_POSITION", NULL);
				double dEnd1 = *(double*)GetSetMediaItemInfo(item1, "D_LENGTH", NULL) + dStart1;
				
				// Reset flags
				leftFlag = false;
				rightFlag = false;
				
				// If the item is selected the the time selection crosses either of it's edges
				if ((selStart < dEnd1 && selEnd > dEnd1) || (selStart < dStart1 && selEnd > dStart1))
				{
					
					// Check for other items that are also crossed by time selection that may be earlier  or later
					for (int iItem2 = 0; iItem2 < GetTrackNumMediaItems(tr); iItem2++)
					{
						
						MediaItem* item2 = GetTrackMediaItem(tr, iItem2);
						
						if (item1 != item2 && *(bool*)GetSetMediaItemInfo(item2, "B_UISEL", NULL))
						{
							
							double dStart2 = *(double*)GetSetMediaItemInfo(item2, "D_POSITION", NULL);
							double dEnd2   = *(double*)GetSetMediaItemInfo(item2, "D_LENGTH", NULL) + dStart2;
							
							// If selection crosses right edge
							if (selStart < dEnd1 && selEnd >= dEnd1)
							{
								// If selection ends after item 2 starts
								if (selEnd >= dStart2)
								{
									// Set right flag if item 2 ends after item 1 ends
									if (dEnd2 > dEnd1)
										rightFlag = true;
								}
							}
							
							
							// If selection crosses left edge
							if (selStart <= dStart1 && selEnd > dStart1)
							{
								// If selection starts before item 2 ends
								if  (selStart <= dEnd2)
								{
									// Set left flag if item 2 starts before item 1
									if (dStart2 < dStart1)
										leftFlag = true;
								}
							}
						}
						
						
					}	
					
					if (selEnd < dEnd1)
						rightFlag = true;
					
					if (selStart > dStart1)
						leftFlag = true;
					
					if (!(leftFlag))
					{
						
						double dLen1 = *(double*)GetSetMediaItemInfo(item1, "D_LENGTH", NULL);
						double edgeAdj = dStart1 - selStart;
						
						dLen1 += edgeAdj;
						
						//*(double*)GetSetMediaItemInfo(item1, "D_POSITION", NULL) = selStart;
						//*(double*)GetSetMediaItemInfo(item1, "D_LENGTH", NULL) = dLen1;
						GetSetMediaItemInfo(item1, "D_POSITION", &selStart);
						GetSetMediaItemInfo(item1, "D_LENGTH", &dLen1);
						
						for (int iTake = 0; iTake < GetMediaItemNumTakes(item1); iTake++)
						{
							MediaItem_Take* take = GetMediaItemTake(item1, iTake);
							if (take)
							{
								double dOffset = *(double*)GetSetMediaItemTakeInfo(take, "D_STARTOFFS", NULL);
								dOffset -= edgeAdj;
								GetSetMediaItemTakeInfo(take, "D_STARTOFFS", &dOffset);
							}
						}
						
						
					}
					
					if (!(rightFlag))
					{
						
						double dLen1 = *(double*)GetSetMediaItemInfo(item1, "D_LENGTH", NULL);
						double edgeAdj = selEnd - dEnd1;
						
						dLen1 += edgeAdj;
						
						//*(double*)GetSetMediaItemInfo(item1, "D_POSITION", NULL) = selStart;
						//*(double*)GetSetMediaItemInfo(item1, "D_LENGTH", NULL) = dLen1;
						GetSetMediaItemInfo(item1, "D_LENGTH", &dLen1);							
					}
					
				}
				
				
				// If there's no time selection
				else //if (selStart = selEnd)
				{
					
					
					// Check for other items that are also selected on track
					for (int iItem2 = 0; iItem2 < GetTrackNumMediaItems(tr); iItem2++)
					{
						
						MediaItem* item2 = GetTrackMediaItem(tr, iItem2);
						
						if (item1 != item2 && *(bool*)GetSetMediaItemInfo(item2, "B_UISEL", NULL))
						{
							
							double dStart2 = *(double*)GetSetMediaItemInfo(item2, "D_POSITION", NULL);
							double dEnd2   = *(double*)GetSetMediaItemInfo(item2, "D_LENGTH", NULL) + dStart2;
							
							// If cursor is before item 1
							if (cursorPos < dStart1)
							{
								// If cursor is before item 2
								if (cursorPos < dEnd2)
								{
									// Set left flag if item 2 comes first
									if (dStart1 > dStart2)
										leftFlag = true;
								}
							}
							
							
							// If cursor is after item 1
							if (cursorPos > dEnd1)
							{
								// If cursor is after item 2
								if  (cursorPos > dStart2)
								{
									// Set right flag if item 2 ends later than item 1
									if (dEnd1 < dEnd2)
										rightFlag = true;
								}
							}
						}
						
						
					}	
					
					
					if (cursorPos >= dStart1)
					{
						leftFlag = true;
					}
					if (cursorPos <= dEnd1)
					{
						rightFlag = true;
					}
					
					
					if (!(leftFlag))
					{
						
						double dLen1 = *(double*)GetSetMediaItemInfo(item1, "D_LENGTH", NULL);
						double edgeAdj = dStart1 - cursorPos;
						
						dLen1 += edgeAdj;
						
						//*(double*)GetSetMediaItemInfo(item1, "D_POSITION", NULL) = selStart;
						//*(double*)GetSetMediaItemInfo(item1, "D_LENGTH", NULL) = dLen1;
						GetSetMediaItemInfo(item1, "D_POSITION", &cursorPos);
						GetSetMediaItemInfo(item1, "D_LENGTH", &dLen1);
						
						for (int iTake = 0; iTake < GetMediaItemNumTakes(item1); iTake++)
						{
							MediaItem_Take* take = GetMediaItemTake(item1, iTake);
							if (take)
							{
								double dOffset = *(double*)GetSetMediaItemTakeInfo(take, "D_STARTOFFS", NULL);
								dOffset -= edgeAdj;
								GetSetMediaItemTakeInfo(take, "D_STARTOFFS", &dOffset);
							}
						}
						
						
					}
					
					if (!(rightFlag))
					{
						
						double dLen1 = *(double*)GetSetMediaItemInfo(item1, "D_LENGTH", NULL);
						double edgeAdj = cursorPos - dEnd1;
						
						dLen1 += edgeAdj;
						
						//*(double*)GetSetMediaItemInfo(item1, "D_POSITION", NULL) = selStart;
						//*(double*)GetSetMediaItemInfo(item1, "D_LENGTH", NULL) = dLen1;
						GetSetMediaItemInfo(item1, "D_LENGTH", &dLen1);							
					}
				}
				
			}
		}
	}
	
	
	UpdateTimeline();
	Undo_OnStateChangeEx(SWSAW_CMD_SHORTNAME(t), UNDO_STATE_ITEMS, -1);
	
}



void AWStretchFill(COMMAND_T* t)
{
	
	double selStart;
	double selEnd;
	bool leftFlag = false;
	bool rightFlag = false;
	
	GetSet_LoopTimeRange2(0, 0, 0, &selStart, &selEnd, 0);
	
	double cursorPos = GetCursorPosition();
	
	for (int iTrack = 1; iTrack <= GetNumTracks(); iTrack++)
	{
		MediaTrack* tr = CSurf_TrackFromID(iTrack, false);
		for (int iItem1 = 0; iItem1 < GetTrackNumMediaItems(tr); iItem1++)
		{
			MediaItem* item1 = GetTrackMediaItem(tr, iItem1);
			if (*(bool*)GetSetMediaItemInfo(item1, "B_UISEL", NULL))
			{
				double dStart1 = *(double*)GetSetMediaItemInfo(item1, "D_POSITION", NULL);
				double dEnd1 = *(double*)GetSetMediaItemInfo(item1, "D_LENGTH", NULL) + dStart1;
				
				// Reset flags
				leftFlag = false;
				rightFlag = false;
				
				// If the item is selected the the time selection crosses either of it's edges
				if ((selStart < dEnd1 && selEnd > dEnd1) || (selStart < dStart1 && selEnd > dStart1))
				{
					
					// Check for other items that are also crossed by time selection that may be earlier  or later
					for (int iItem2 = 0; iItem2 < GetTrackNumMediaItems(tr); iItem2++)
					{
						
						MediaItem* item2 = GetTrackMediaItem(tr, iItem2);
						
						if (item1 != item2 && *(bool*)GetSetMediaItemInfo(item2, "B_UISEL", NULL))
						{
							
							double dStart2 = *(double*)GetSetMediaItemInfo(item2, "D_POSITION", NULL);
							double dEnd2   = *(double*)GetSetMediaItemInfo(item2, "D_LENGTH", NULL) + dStart2;
							
							// If selection crosses right edge
							if (selStart < dEnd1 && selEnd >= dEnd1)
							{
								// If selection ends after item 2 starts
								if (selEnd >= dStart2)
								{
									// Set right flag if item 2 ends after item 1 ends
									if (dEnd2 > dEnd1)
										rightFlag = true;
								}
							}
							
							
							// If selection crosses left edge
							if (selStart <= dStart1 && selEnd > dStart1)
							{
								// If selection starts before item 2 ends
								if  (selStart <= dEnd2)
								{
									// Set left flag if item 2 starts before item 1
									if (dStart2 < dStart1)
										leftFlag = true;
								}
							}
						}
						
						
					}	
					
					if (selEnd < dEnd1)
						rightFlag = true;
					
					if (selStart > dStart1)
						leftFlag = true;
					
					if (!(leftFlag))
					{
						
						double dLen1 = *(double*)GetSetMediaItemInfo(item1, "D_LENGTH", NULL);
						double edgeAdj = dStart1 - selStart;
						
						double dNewLen1 = dLen1 + edgeAdj;
						
						//*(double*)GetSetMediaItemInfo(item1, "D_POSITION", NULL) = selStart;
						//*(double*)GetSetMediaItemInfo(item1, "D_LENGTH", NULL) = dLen1;
						
						GetSetMediaItemInfo(item1, "D_POSITION", &selStart);
						GetSetMediaItemInfo(item1, "D_LENGTH", &dNewLen1);
						
						
						for (int iTake = 0; iTake < GetMediaItemNumTakes(item1); iTake++)
						{
							MediaItem_Take* take = GetMediaItemTake(item1, iTake);
							if (take)
							{
								double dRate = *(double*)GetSetMediaItemTakeInfo(take, "D_PLAYRATE", NULL);
								dRate *= (dLen1/dNewLen1);
								GetSetMediaItemTakeInfo(take, "D_PLAYRATE", &dRate);
							}
						}
						
						
					}
					
					if (!(rightFlag))
					{
						
						double dLen1 = *(double*)GetSetMediaItemInfo(item1, "D_LENGTH", NULL);
						double edgeAdj = selEnd - dEnd1;
						
						double dNewLen1 = dLen1 + edgeAdj;
						
						//*(double*)GetSetMediaItemInfo(item1, "D_POSITION", NULL) = selStart;
						//*(double*)GetSetMediaItemInfo(item1, "D_LENGTH", NULL) = dLen1;
						GetSetMediaItemInfo(item1, "D_LENGTH", &dNewLen1);			
						
						for (int iTake = 0; iTake < GetMediaItemNumTakes(item1); iTake++)
						{
							MediaItem_Take* take = GetMediaItemTake(item1, iTake);
							if (take)
							{
								double dRate = *(double*)GetSetMediaItemTakeInfo(take, "D_PLAYRATE", NULL);
								dRate *= (dLen1/dNewLen1);
								GetSetMediaItemTakeInfo(take, "D_PLAYRATE", &dRate);
							}
						}
						
						
					}
					
				}
				
				
				else //if (selStart = selEnd)
				{
					
					
					// Check for other items that are also selected on track
					for (int iItem2 = 0; iItem2 < GetTrackNumMediaItems(tr); iItem2++)
					{
						
						MediaItem* item2 = GetTrackMediaItem(tr, iItem2);
						
						if (item1 != item2 && *(bool*)GetSetMediaItemInfo(item2, "B_UISEL", NULL))
						{
							
							double dStart2 = *(double*)GetSetMediaItemInfo(item2, "D_POSITION", NULL);
							double dEnd2   = *(double*)GetSetMediaItemInfo(item2, "D_LENGTH", NULL) + dStart2;
							
							// If cursor is before item 1
							if (cursorPos < dStart1)
							{
								// If cursor is before item 2
								if (cursorPos < dEnd2)
								{
									// Set left flag if item 2 comes first
									if (dStart1 > dStart2)
										leftFlag = true;
								}
							}
							
							
							// If cursor is after item 1
							if (cursorPos > dEnd1)
							{
								// If cursor is after item 2
								if  (cursorPos > dStart2)
								{
									// Set right flag if item 2 ends later than item 1
									if (dEnd1 < dEnd2)
										rightFlag = true;
								}
							}
						}
						
						
					}	
					
					
					if (cursorPos >= dStart1)
					{
						leftFlag = true;
					}
					if (cursorPos <= dEnd1)
					{
						rightFlag = true;
					}
					
					
					if (!(leftFlag))
					{
						
						double dLen1 = *(double*)GetSetMediaItemInfo(item1, "D_LENGTH", NULL);
						double edgeAdj = dStart1 - cursorPos;
						
						double dNewLen1 = dLen1 + edgeAdj;
						
						//*(double*)GetSetMediaItemInfo(item1, "D_POSITION", NULL) = selStart;
						//*(double*)GetSetMediaItemInfo(item1, "D_LENGTH", NULL) = dLen1;
						
						GetSetMediaItemInfo(item1, "D_POSITION", &cursorPos);
						GetSetMediaItemInfo(item1, "D_LENGTH", &dNewLen1);
						
						
						for (int iTake = 0; iTake < GetMediaItemNumTakes(item1); iTake++)
						{
							MediaItem_Take* take = GetMediaItemTake(item1, iTake);
							if (take)
							{
								double dRate = *(double*)GetSetMediaItemTakeInfo(take, "D_PLAYRATE", NULL);
								dRate *= (dLen1/dNewLen1);
								GetSetMediaItemTakeInfo(take, "D_PLAYRATE", &dRate);
							}
						}
						
						
					}
					
					if (!(rightFlag))
					{
						
						double dLen1 = *(double*)GetSetMediaItemInfo(item1, "D_LENGTH", NULL);
						double edgeAdj = cursorPos - dEnd1;
						
						double dNewLen1 = dLen1 + edgeAdj;
						
						//*(double*)GetSetMediaItemInfo(item1, "D_POSITION", NULL) = selStart;
						//*(double*)GetSetMediaItemInfo(item1, "D_LENGTH", NULL) = dLen1;
						GetSetMediaItemInfo(item1, "D_LENGTH", &dNewLen1);			
						
						for (int iTake = 0; iTake < GetMediaItemNumTakes(item1); iTake++)
						{
							MediaItem_Take* take = GetMediaItemTake(item1, iTake);
							if (take)
							{
								double dRate = *(double*)GetSetMediaItemTakeInfo(take, "D_PLAYRATE", NULL);
								dRate *= (dLen1/dNewLen1);
								GetSetMediaItemTakeInfo(take, "D_PLAYRATE", &dRate);
							}
						}
						
						
					}
				}
				
			}
		}
	}
	
	
	
	UpdateTimeline();
	Undo_OnStateChangeEx(SWSAW_CMD_SHORTNAME(t), UNDO_STATE_ITEMS, -1);
	
}



// Experimental cut, copy and paste that will let you use relative paste since JCS are mean

void AWCopy(COMMAND_T* t)
{
	Undo_BeginBlock();
	
	g_AWCopyFlag = 0;
	
	double selStart;
	double selEnd;
	
	GetSet_LoopTimeRange2(0, 0, 0, &selStart, &selEnd, 0);
	
	
	// If items have focus
	if (GetCursorContext() == 1)
	{
		
		if (selStart != selEnd)
		{
			g_AWCopySelLen = selEnd - selStart;
			g_AWCopyFlag = 1;
		}
		
		// Create array of selected media items
		WDL_TypedBuf<MediaItem*> items;
		SWS_GetSelectedMediaItems(&items);
		
		
		// If there are selected items
		
		if (items.GetSize() > 0)
		{
			g_AWCopyFlag = 0;
			
			// Create array to store item timebases
			char* cItemsTimebase = new char[items.GetSize()];
			
			// Loop through every item in array
			for (int i = 0; i < items.GetSize(); i++)
			{
				// Store timebase of current item in array
				cItemsTimebase[i] = *(char*)GetSetMediaItemInfo(items.Get()[i], "C_BEATATTACHMODE", NULL);
				
				// Set item's timebase to "Time"
				char cZero = 0;
				GetSetMediaItemInfo(items.Get()[i], "C_BEATATTACHMODE", &cZero);
			}
			
			
			SmartCopy(NULL);
			
			
			// Restore item's original timebase
			for (int i = 0; i < items.GetSize(); i++)
				GetSetMediaItemInfo(items.Get()[i], "C_BEATATTACHMODE", &(cItemsTimebase[i]));
			
			// Delete timebase array
			delete [] cItemsTimebase;
		}
		
		
	}
	
	// Otherwise just do whatever, I don't want anything to do with it
	else
		SmartCopy(NULL);
	
	Undo_EndBlock(SWSAW_CMD_SHORTNAME(t), UNDO_STATE_ALL);
	
}

void AWCut(COMMAND_T* t)
{
	Undo_BeginBlock();
	
	g_AWCopyFlag = 0;
	
	
	// If items have focus
	if (GetCursorContext() == 1)
	{
		Main_OnCommand(40433, 0); // Set item timebase to time, necessary for relative paste
		
		SmartCut(NULL);
		
	}
	
	// Otherwise just do whatever, I don't want anything to do with it
	else
		SmartCut(NULL);
	
	Undo_EndBlock(SWSAW_CMD_SHORTNAME(t), UNDO_STATE_ALL);
	
}

void AWPaste(COMMAND_T* t)
{
	Undo_BeginBlock();
	
	int* pTrimMode = (int*)GetConfigVar("autoxfade");
	
	if (g_AWCopyFlag)
	{
		// Code to paste empty time selection goes here
		
		
		double t1 = GetCursorPosition();
		double t2 = t1 + g_AWCopySelLen;
		
		GetSet_LoopTimeRange(true, false, &t1, &t2, false);
		
		if (*pTrimMode & 2)
		{
			Main_OnCommand(40718, 0); // item select all item on selected track in time selection
			Main_OnCommand(40312, 0); // item remove selected area of item	
		}
	}
	
	else if (GetCursorContext() == 1)
	{
		int* pCursorMode = (int*)GetConfigVar("itemclickmovecurs");
		int savedMode = *pCursorMode;
		*pCursorMode &= ~8;	// Enable "move edit cursor on paste" so that the time selection can be set properly
		
		
		
		
		if (*pTrimMode & 2)
		{
			double cursorPos = GetCursorPosition();
			
			Main_OnCommand(40625, 0); // Set time selection start point
			Main_OnCommand(40058, 0); // item paste items tracks	
			Main_OnCommand(40626, 0); // time selection set end point
			
			// Select only tracks with selected items	
			
			SelTracksWItems();
			
			Main_OnCommand(40718, 0); // item select all item on selected track in time selection
			Main_OnCommand(40312, 0); // item remove selected area of item	
			
			// Go to beginning of selection, this is smoother than using actual action and easier
			SetEditCurPos(cursorPos, 0, 0);
			
			
			DoSelectFirstOfSelectedTracks(NULL);
			
			*pCursorMode = savedMode;
			
			Main_OnCommand(40058, 0); // Paste items	
			Main_OnCommand(40380, 0); // Set item timebase to project default (wish could copy time base from source item but too hard)
			
			// This would be PT style but probably better to just follow preference in preferences
			//Main_OnCommand(40630, 0); // go to start of time selection
		}
		else
			Main_OnCommand(40058, 0); // Std paste
		
	}
	
	else
		Main_OnCommand(40058, 0); // Std paste
	
	UpdateTimeline();
	
	Undo_EndBlock(SWSAW_CMD_SHORTNAME(t), UNDO_STATE_ALL);
	
}


void AWDelete(COMMAND_T*)
{
	double selStart;
	double selEnd;
	
	GetSet_LoopTimeRange2(0, 0, 0, &selStart, &selEnd, 0);
	
	if (GetCursorContext() == 2 && (selStart != selEnd))
		Main_OnCommand(40089, 0); // Remove env points in time selection
	else
		SmartRemove(NULL);
}








void AWConsolidateSelection(COMMAND_T* t)
{
	Undo_BeginBlock();
	
	// Set time sel to sel items if no time sel
	double t1, t2;
	GetSet_LoopTimeRange(false, false, &t1, &t2, false);
	if (t1 == t2)
		Main_OnCommand(40290, 0);
	
	
	int trackOn = 1;
	
	// Save trim mode state
	int* pTrimMode = (int*)GetConfigVar("autoxfade");
	int savedMode = *pTrimMode;
	
	// Turn trim mode off
	*pTrimMode &= ~2;
	
	SelTracksWItems();
	
	SaveSelected();
	
	
	for (int iTrack = 1; iTrack <= GetNumTracks(); iTrack++)
	{
		MediaTrack* tr = CSurf_TrackFromID(iTrack, false);
		
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			ClearSelected();
			
			GetSetMediaTrackInfo(tr, "I_SELECTED", &trackOn); // Select current track
			
			Main_OnCommand(40142, 0); // Insert empty item
			Main_OnCommand(40718, 0); // Select all items on track in time selection
			Main_OnCommand(40919, 0); // Set per item mix behavior to always mix
			Main_OnCommand(40362, 0); // Glue items	
			
			RestoreSelected();
			
		}
	}
	
	*pTrimMode = savedMode;
	
	RestoreSelected();
	
	Undo_EndBlock(SWSAW_CMD_SHORTNAME(t), UNDO_STATE_ALL);
	
}


void AWSelectStretched(COMMAND_T* t)
{
	
	WDL_TypedBuf<MediaItem*> items;	
	SWS_GetSelectedMediaItems(&items);
	
	for (int i = 0; i < items.GetSize(); i++)
	{
		MediaItem_Take* take = GetMediaItemTake(items.Get()[i], -1);
		
		if (take && *(double*)GetSetMediaItemTakeInfo(take, "D_PLAYRATE", NULL) == 1.0)
			GetSetMediaItemInfo(items.Get()[i], "B_UISEL", &g_i0);
	}
	
	UpdateArrange();
	
	Undo_OnStateChangeEx(SWSAW_CMD_SHORTNAME(t), UNDO_STATE_ITEMS, -1);
	
}



// Metronome actions

void AWMetrPlayOn(COMMAND_T* = NULL)		{ *(int*)GetConfigVar("projmetroen") |= 2;}
void AWMetrPlayOff(COMMAND_T* = NULL)		{ *(int*)GetConfigVar("projmetroen") &= ~2;}
void AWMetrRecOn(COMMAND_T* = NULL)			{ *(int*)GetConfigVar("projmetroen") |= 4;}
void AWMetrRecOff(COMMAND_T* = NULL)		{ *(int*)GetConfigVar("projmetroen") &= ~4;}
void AWCountPlayOn(COMMAND_T* = NULL)		{ *(int*)GetConfigVar("projmetroen") |= 8;}
void AWCountPlayOff(COMMAND_T* = NULL)      { *(int*)GetConfigVar("projmetroen") &= ~8;}
void AWCountRecOn(COMMAND_T* = NULL)		{ *(int*)GetConfigVar("projmetroen") |= 16;}
void AWCountRecOff(COMMAND_T* = NULL)		{ *(int*)GetConfigVar("projmetroen") &= ~16;}
void AWMetrPlayToggle(COMMAND_T* = NULL)    { *(int*)GetConfigVar("projmetroen") ^= 2;}
void AWMetrRecToggle(COMMAND_T* = NULL)     { *(int*)GetConfigVar("projmetroen") ^= 4;}
void AWCountPlayToggle(COMMAND_T* = NULL)   { *(int*)GetConfigVar("projmetroen") ^= 8;}
void AWCountRecToggle(COMMAND_T* = NULL)    { *(int*)GetConfigVar("projmetroen") ^= 16;}
bool IsMetrPlayOn(COMMAND_T* = NULL)		{ return (*(int*)GetConfigVar("projmetroen") & 2)  != 0; }
bool IsMetrRecOn(COMMAND_T* = NULL)			{ return (*(int*)GetConfigVar("projmetroen") & 4)  != 0; }
bool IsCountPlayOn(COMMAND_T* = NULL)		{ return (*(int*)GetConfigVar("projmetroen") & 8)  != 0; }
bool IsCountRecOn(COMMAND_T* = NULL)		{ return (*(int*)GetConfigVar("projmetroen") & 16) != 0; }

// Editing Preferences
void AWRelEdgeOn(COMMAND_T* = NULL)			{ *(int*)GetConfigVar("relativeedges") |= 1;}
void AWRelEdgeOff(COMMAND_T* = NULL)		{ *(int*)GetConfigVar("relativeedges") &= ~1;}
void AWRelEdgeToggle(COMMAND_T* = NULL)		{ *(int*)GetConfigVar("relativeedges") ^= 1;}
bool IsRelEdgeOn(COMMAND_T* = NULL)			{ return (*(int*)GetConfigVar("relativeedges") & 1)  != 0; }

void AWClrTimeSelClkOn(COMMAND_T* = NULL)			{ *(int*)GetConfigVar("itemclickmovecurs") |= 64;}
void AWClrTimeSelClkOff(COMMAND_T* = NULL)			{ *(int*)GetConfigVar("itemclickmovecurs") &= ~64;}
void AWClrTimeSelClkToggle(COMMAND_T* = NULL)		{ *(int*)GetConfigVar("itemclickmovecurs") ^= 64;}
bool IsClrTimeSelClkOn(COMMAND_T* = NULL)			{ return (*(int*)GetConfigVar("itemclickmovecurs") & 64)  != 0; }

void AWClrLoopClkOn(COMMAND_T* = NULL)			{ *(int*)GetConfigVar("itemclickmovecurs") |= 32;}
void AWClrLoopClkOff(COMMAND_T* = NULL)			{ *(int*)GetConfigVar("itemclickmovecurs") &= ~32;}
void AWClrLoopClkToggle(COMMAND_T* = NULL)		{ *(int*)GetConfigVar("itemclickmovecurs") ^= 32;}
bool IsClrLoopClkOn(COMMAND_T* = NULL)			{ return (*(int*)GetConfigVar("itemclickmovecurs") & 32)  != 0; }


static COMMAND_T g_commandTable[] = 
{
	// Add commands here (copy paste an example from ItemParams.cpp or similar	
	
	// Fill Gaps Actions
	{ { DEFACCEL, "SWS/AW: Fill gaps between selected items (advanced)" },												"SWS_AWFILLGAPSADV",				AWFillGapsAdv, NULL, 0 },
	{ { DEFACCEL, "SWS/AW: Fill gaps between selected items (advanced, use last settings)" },							"SWS_AWFILLGAPSADVLASTSETTINGS",	AWFillGapsAdv, NULL, 1 },
	{ { DEFACCEL, "SWS/AW: Fill gaps between selected items (quick, no crossfade)" },									"SWS_AWFILLGAPSQUICK",				AWFillGapsQuick, },
	{ { DEFACCEL, "SWS/AW: Fill gaps between selected items (quick, crossfade using default fade length)" },			"SWS_AWFILLGAPSQUICKXFADE",			AWFillGapsQuickXFade, },
	{ { DEFACCEL, "SWS/AW: Remove overlaps in selected items preserving item starts" },									"SWS_AWFIXOVERLAPS",				AWFixOverlaps, },
	
	// Transport Actions
	{ { DEFACCEL, "SWS/AW: Record Conditional (normal or time sel only)" },																"SWS_AWRECORDCOND",					AWRecordConditional, },
	{ { DEFACCEL, "SWS/AW: Record Conditional (normal, time sel or item sel)" },														"SWS_AWRECORDCOND2",				AWRecordConditional2, },
	{ { DEFACCEL, "SWS/AW: Record (automatically group simultaneously recorded items)" },												"SWS_AWRECORDGROUP",				AWRecordAutoGroup, },
	{ { DEFACCEL, "SWS/AW: Record Conditional (normal or time sel only, automatically group simultaneously recorded items)" },			"SWS_AWRECORDCONDGROUP",			AWRecordConditionalAutoGroup, },
	{ { DEFACCEL, "SWS/AW: Record Conditional (normal/time sel/item sel, automatically group simultaneously recorded items)" },			"SWS_AWRECORDCONDGROUP2",			AWRecordConditionalAutoGroup2, },
	{ { DEFACCEL, "SWS/AW: Play/Stop (automatically group simultaneously recorded items)" },											"SWS_AWPLAYSTOPGRP",				AWPlayStopAutoGroup, },
	
	// Misc Item Actions
	{ { DEFACCEL, "SWS/AW: Select from cursor to end of project (items and time selection)" },			"SWS_AWSELTOEND",					AWSelectToEnd, },
	{ { DEFACCEL, "SWS/AW: Fade in/out/crossfade selected area of selected items" },					"SWS_AWFADESEL",					AWFadeSelection, },
	{ { DEFACCEL, "SWS/AW: Trim selected items to selection or cursor (crop)" },						"SWS_AWTRIMCROP",					AWTrimCrop, },
	{ { DEFACCEL, "SWS/AW: Trim selected items to fill selection" },									"SWS_AWTRIMFILL",					AWTrimFill, },
	{ { DEFACCEL, "SWS/AW: Stretch selected items to fill selection" },									"SWS_AWSTRETCHFILL",				AWStretchFill, },
	{ { DEFACCEL, "SWS/AW: Consolidate Selection" },													"SWS_AWCONSOLSEL",					AWConsolidateSelection, },

	
	// Toggle Actions 
	
	// Metronome Actions
	{ { DEFACCEL, "SWS/AW: Enable metronome during playback" },			"SWS_AWMPLAYON",					AWMetrPlayOn, },
	{ { DEFACCEL, "SWS/AW: Disable metronome during playback" },		"SWS_AWMPLAYOFF",					AWMetrPlayOff, },
	{ { DEFACCEL, "SWS/AW: Toggle metronome during playback" },			"SWS_AWMPLAYTOG",					AWMetrPlayToggle, NULL, 0, IsMetrPlayOn },
	
	{ { DEFACCEL, "SWS/AW: Enable metronome during recording" },		"SWS_AWMRECON",						AWMetrRecOn, },
	{ { DEFACCEL, "SWS/AW: Disable metronome during recording" },		"SWS_AWMRECOFF",					AWMetrRecOff, },
	{ { DEFACCEL, "SWS/AW: Toggle metronome during recording" },		"SWS_AWMRECTOG",					AWMetrRecToggle, NULL, 0, IsMetrRecOn },
	
	
	{ { DEFACCEL, "SWS/AW: Enable count-in before playback" },			"SWS_AWCOUNTPLAYON",				AWCountPlayOn, },
	{ { DEFACCEL, "SWS/AW: Disable count-in before playback" },			"SWS_AWCOUNTPLAYOFF",				AWCountPlayOff, },
	{ { DEFACCEL, "SWS/AW: Toggle count-in before playback" },			"SWS_AWCOUNTPLAYTOG",				AWCountPlayToggle, NULL, 0, IsCountPlayOn },
	
	
	{ { DEFACCEL, "SWS/AW: Enable count-in before recording" },			"SWS_AWCOUNTRECON",					AWCountRecOn, },
	{ { DEFACCEL, "SWS/AW: Disable count-in before recording" },		"SWS_AWCOUNTRECOFF",				AWCountRecOff, },
	{ { DEFACCEL, "SWS/AW: Toggle count-in before recording" },			"SWS_AWCOUNTRECTOG",				AWCountRecToggle, NULL, 0, IsCountRecOn },
	
	// Editing Preferences
	{ { DEFACCEL, "SWS/AW: Enable relative editing when resizing item edges" },			"SWS_AWRELEDGEON",				AWRelEdgeOn, },
	{ { DEFACCEL, "SWS/AW: Disable relative editing when resizing item edges" },		"SWS_AWRELEDGEOFF",				AWRelEdgeOff, },
	{ { DEFACCEL, "SWS/AW: Toggle relative editing when resizing item edges" },			"SWS_AWRELEDGETOG",				AWRelEdgeToggle, NULL, 0, IsRelEdgeOn },
	
	
	{ { DEFACCEL, "SWS/AW: Enable clear time selection on click in arrange" },			"SWS_AWCLRTIMESELCLKON",		AWClrTimeSelClkOn, },
	{ { DEFACCEL, "SWS/AW: Disable clear time selection on click in arrange" },			"SWS_AWCLRTIMESELCLKOFF",		AWClrTimeSelClkOff, },
	{ { DEFACCEL, "SWS/AW: Toggle clear time selection on click in arrange" },			"SWS_AWCLRTIMESELCLKTOG",		AWClrTimeSelClkToggle, NULL, 0, IsClrTimeSelClkOn },
	
	{ { DEFACCEL, "SWS/AW: Enable clear loop points on click in ruler" },			"SWS_AWCLRLOOPCLKON",				AWClrLoopClkOn, },
	{ { DEFACCEL, "SWS/AW: Disable clear loop points on click in ruler" },			"SWS_AWCLRLOOPCLKOFF",				AWClrLoopClkOff, },
	{ { DEFACCEL, "SWS/AW: Toggle clear loop points on click in ruler" },			"SWS_AWCLRLOOPCLKTOG",				AWClrLoopClkToggle, NULL, 0, IsClrLoopClkOn },
	

	
	
	
	// Stuff that sort of sucks that I might make decent enough to release
	//{ { DEFACCEL, "SWS/AW: Copy" },			"SWS_AWCOPY",					AWCopy, },
	//{ { DEFACCEL, "SWS/AW: Cut" },			"SWS_AWCUT",					AWCut, },
	//{ { DEFACCEL, "SWS/AW: Paste" },		"SWS_AWPASTE",					AWPaste, },
	//{ { DEFACCEL, "SWS/AW: Delete" },		"SWS_AWDELETE",					AWDelete, NULL, },
	
	//{ { DEFACCEL, "SWS/AW: Select Stretched Items" },													"SWS_AWSELSTRETCH",					AWSelectStretched, },


	//{ { DEFACCEL, "SWS/AW: Quick Punch Record" },			"SWS_AWQUICKPUNCH",					AWRecordQuickPunch, },

	
	
	{ {}, LAST_COMMAND, }, // Denote end of table
};

int AdamInit()
{
	SWSRegisterCommands(g_commandTable);

	return 1;
}