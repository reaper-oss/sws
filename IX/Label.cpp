/******************************************************************************
/ Label.cpp
/
/ Copyright (c) 2012 Philip S. Considine (IX)
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

#include "IX.h"
#include "../resource.h"
#include "../Misc/Analysis.h"
#include "../SnM/SnM_Dlg.h"

#include <WDL/localize/localize.h>

#define IX_LABELPROC_TEXT_KEY	"Label processor"
#define IX_LABELPROC_ALLTAKES_KEY	IX_LABELPROC_TEXT_KEY" all takes"

bool bAllTakes;

WDL_DLGRET doLabelProcDlg(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwnd, uMsg, wParam, lParam))
		return r;

	WDL_FastString *pStr = (WDL_FastString*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
			pStr = (WDL_FastString*)lParam;

			WDL_FastString helpStr;
			helpStr.Append(__LOCALIZE("All arguments are optional. The following are all valid: /E /E[3] /E[3,9]:\n","sws_DLG_163"));
			helpStr.Append(__LOCALIZE("\n\t/D\t\t\tDuration.","sws_DLG_163"));
			helpStr.Append(__LOCALIZE("\n\t/E[digits, first]\t\tEnumerate in selection.","sws_DLG_163"));
			helpStr.Append(__LOCALIZE("\n\t/e[digits, first]\t\tEnumerate in selection on track.","sws_DLG_163"));
			helpStr.Append(__LOCALIZE("\n\t/I[digits, last]\t\tInverse enumerate in selection.","sws_DLG_163"));
			helpStr.Append(__LOCALIZE("\n\t/i[digits, last]\t\tInverse enumerate in selection on track.","sws_DLG_163"));
			helpStr.Append(__LOCALIZE("\n\t/K[digits]\t\tTake count.","sws_DLG_163"));
			helpStr.Append(__LOCALIZE("\n\t/k[digits]\t\tTake number.","sws_DLG_163"));
			helpStr.Append(__LOCALIZE("\n\t/L[offset, length]\t\tCurrent label.","sws_DLG_163"));
			helpStr.Append(__LOCALIZE("\n\t/O\t\t\tSource offset.","sws_DLG_163"));
			helpStr.Append(__LOCALIZE("\n\t/P[precision]\t\tPeak level.","sws_DLG_163"));
			helpStr.Append(__LOCALIZE("\n\t/R[precision]\t\tRMS peak level.","sws_DLG_163"));
			helpStr.Append(__LOCALIZE("\n\t/r[precision]\t\tRMS average level.","sws_DLG_163"));
			helpStr.Append(__LOCALIZE("\n\t/S[offset, length]\t\tSource media full path.","sws_DLG_163"));
			helpStr.Append(__LOCALIZE("\n\t/s[offset, length]\t\tSource media filename.","sws_DLG_163"));
			helpStr.Append(__LOCALIZE("\n\t/T[offset, length]\t\tTrack name.","sws_DLG_163"));
			helpStr.Append(__LOCALIZE("\n\t/t[digits]\t\t\tTrack number.","sws_DLG_163"));
			SetWindowText(GetDlgItem(hwnd, IDC_HELPTEXT), helpStr.Get());

			if(pStr)
				SetDlgItemText(hwnd, IDC_EDIT, pStr->Get());

			bAllTakes = GetPrivateProfileInt(SWS_INI, IX_LABELPROC_ALLTAKES_KEY, 0, get_ini_file()) > 0;
			SendMessage(GetDlgItem(hwnd, IDC_CHECK1), BM_SETCHECK, bAllTakes ? BST_CHECKED : BST_UNCHECKED, 0);

			return 0;
		}

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_CHECK1 :
					if(HIWORD(wParam) == BN_CLICKED)
						bAllTakes = SendMessage(GetDlgItem(hwnd, IDC_CHECK1), BM_GETCHECK, 0, 0) == BST_CHECKED;
					break;

				case IDOK:
					{
#ifdef _WIN32
						HWND hEdit = GetDlgItem(hwnd, IDC_EDIT);
						int max = GetWindowTextLength(hEdit) + 1;
#else
						int max = 2048;
#endif
						char *buf = new char[max + 1];
						GetDlgItemText(hwnd, IDC_EDIT, buf, max);

						if(pStr)
						{
							pStr->Set(buf);
							EndDialog(hwnd, 1);
						}
						else
						{
							EndDialog(hwnd, 0);
						}
						delete [] buf;
					}
					break;

				case IDCANCEL:
					EndDialog(hwnd, 0);
					break;
			}
			break;
	}
	return 0;
}

const char * GetItemVolString(MediaItem* pItem, int mode, int precision)
{
	ANALYZE_PCM a;
	memset(&a, 0, sizeof(a));

	static WDL_FastString ret;

	if(mode == 2)
	{
		char str[100];
		GetPrivateProfileString(SWS_INI, SWS_RMS_KEY, "-20,0.1", str, 100, get_ini_file());
		char* pWindow = strchr(str, ',');
		a.dWindowSize = pWindow ? atof(pWindow+1) : 0.1;
	}

	if (AnalyzeItem(pItem, &a))
	{
		ret.SetFormatted(8, "%0.*f", precision, VAL2DB(mode == 0 ? a.dPeakVal : a.dRMS));
	}

	return ret.Get();
}


// Returns a substring of str.
// Negative values are allowed for both offset and length.
// Negative values count backwards from the end of the string.
// invalid values for offset or length are ignored
const char * GetSubString(const char *str, int offset, int length = 0)
{
	static WDL_FastString ret;

	ret.Set(str);

	// Trim start
	if(offset > 0)
	{
		ret.DeleteSub(0, offset);
	}
	else if(offset < 0)
	{
		ret.DeleteSub(0, ret.GetLength() + offset);
	}

	// Trim end
	if(length < 1) length += ret.GetLength();
	if(ret.GetLength() > length)
	{
		ret.DeleteSub(length, ret.GetLength() - length);
	}

	return ret.Get();
}

// Looks for comma separated int values enclosed by square braces at start of string.
// If found, stores up to maxvals values in vals[] and advances c to the next character after the closing token.
void ExtractValues(const char *&c, int vals[], int maxvals)
{
	if(c && *c == '[')
	{
		const char *tokend = strchr(c, ']');
		if(tokend)
		{
			int v = 0;
			do
			{
				vals[v++] = atoi(++c);
				c = strchr(c, ',');
			}
			while(c && c < tokend && v < maxvals);

			c = tokend + 1;
		}
	}
}

// Re-label selected items according to format string
void LabelProcessor(COMMAND_T* ct)
{
	WDL_FastString format;
	char buf[512];
	GetPrivateProfileString(SWS_INI, IX_LABELPROC_TEXT_KEY, "/L", buf, sizeof(buf), get_ini_file());

	format.Set(buf);

	if (!DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_IX_LABELDLG), GetMainHwnd(), doLabelProcDlg, (LPARAM) &format ))
	{
		return;
	}

	WritePrivateProfileString(SWS_INI, IX_LABELPROC_TEXT_KEY, format.Get(), get_ini_file());
	WritePrivateProfileString(SWS_INI, IX_LABELPROC_ALLTAKES_KEY, bAllTakes ? "1" : "0", get_ini_file());
	RunLabelCommand(&format, SWS_CMD_SHORTNAME(ct));
}

void RunLabelCommand(WDL_FastString* cmd, const char* undoName)
{
	if (undoName)
		Undo_BeginBlock2(NULL);

	char buf[512];
	int itemCount = 0;
	const int numSel = CountSelectedMediaItems(NULL);
	for (int iTrack = 1; iTrack <= GetNumTracks(); iTrack++)
	{
		WDL_TypedBuf<MediaItem*> items;
		SWS_GetSelectedMediaItemsOnTrack(&items, CSurf_TrackFromID(iTrack, false));
		int numSelOnTrack = items.GetSize();
		for (int i = 0; i < numSelOnTrack; i++)
		{
			MediaItem* pItem = items.Get()[i];

			// Build take list
			WDL_TypedBuf<MediaItem_Take*> takes;
			if(bAllTakes)
			{
				for(int t = 0, tc = GetMediaItemNumTakes(pItem); t < tc; t++)
				{
					takes.Add(GetMediaItemTake(pItem, t));
				}
			}
			else
			{
				takes.Add(GetActiveTake(pItem));
			}

			// Do the work
			for(int t = 0, tc = takes.GetSize(); t < tc; t++)
			{
				MediaItem_Take* pTake = takes.Get()[t];

				WDL_FastString str;

				const char *c = cmd->Get();
				const char *end = strchr(c, 0);

				while(c < end)
				{
					switch(*c)
					{
					case '/' :
						{
							switch(*(++c))
							{
							default :
								str.Append("/");
								break;

							case 'D' : // Duration
								{
									double length = *(double*) GetSetMediaItemInfo(pItem, "D_LENGTH", NULL);
									format_timestr(length, buf, sizeof(buf));
									str.AppendFormatted(str.GetLength() + (int)strlen(buf), "%s", buf);
									++c;
								}
								break;

							case 'E' : // Enumerate all
								{
									int args[2] = {2,1};
									ExtractValues(++c, args, 2);
									str.AppendFormatted(str.GetLength() + 8, "%0*d", args[0], itemCount + args[1]);
								}
								break;

							case 'e' : // Enumerate on track
								{
									int args[2] = {2,1};
									ExtractValues(++c, args, 2);
									str.AppendFormatted(str.GetLength() + 8, "%0*d", args[0], i + args[1]);
								}
								break;

							case 'I' : // Inverse enumerate all
								{
									int args[2] = {2,0};
									ExtractValues(++c, args, 2);
									str.AppendFormatted(str.GetLength() + 8, "%0*d", args[0], numSel - itemCount + args[1] - 1);
								}
								break;

							case 'i' : // Inverse enumerate on track
								{
									int args[2] = {2,0};
									ExtractValues(++c, args, 2);
									str.AppendFormatted(str.GetLength() + 8, "%0*d", args[0], numSelOnTrack - i + args[1] - 1);
								}
								break;

							case 'K' : // Take count
								{
									int digits = 1;
									ExtractValues(++c, &digits, 1);
									str.AppendFormatted(str.GetLength() + digits, "%0*d", digits, tc);
								}
								break;

							case 'k' : // Take number
								{
									int digits = 1;
									ExtractValues(++c, &digits, 1);
									str.AppendFormatted(str.GetLength() + digits, "%0*d", digits, t + 1);
								}
								break;

							case 'L' : // Current label
								{
									int args[2] = {0,0};
									ExtractValues(++c, args, 2);

									const char *label = (const char*) GetSetMediaItemTakeInfo(pTake, "P_NAME", NULL);
									if(label)
										str.Append(GetSubString(label, args[0], args[1]));
								}
								break;

							case 'O' : // Source offset
								{
									double offset = *(double*) GetSetMediaItemTakeInfo(pTake, "D_STARTOFFS", NULL);
									format_timestr(offset, buf, sizeof(buf));
									str.AppendFormatted(str.GetLength() + (int)strlen(buf), "%s", buf);
									++c;
								}
								break;

							case 'P' : // Peak
								{
									int precision = 1;
									ExtractValues(++c, &precision, 1);
									str.Append(GetItemVolString(pItem, 0, precision));
								}
								break;

							case 'R' : // RMS Max
								{
									int precision = 1;
									ExtractValues(++c, &precision, 1);
									str.Append(GetItemVolString(pItem, 2, precision));
								}
								break;

							case 'r' : // RMS Avg
								{
									int precision = 1;
									ExtractValues(++c, &precision, 1);
									str.Append(GetItemVolString(pItem, 1, precision));
								}
								break;

							case 'S' : // Source full path
								{
									int args[2] = {0,0};
									ExtractValues(++c, args, 2);

									PCM_source *pSource = (PCM_source*) GetSetMediaItemTakeInfo(pTake, "P_SOURCE", NULL);
									if(pSource)
									{
										memset(buf, 0, sizeof(buf));
										GetMediaSourceFileName(pSource, buf, sizeof(buf));
										if(*buf)
											str.Append(GetSubString(buf, args[0], args[1]));
									}
								}
								break;

							case 's' : // Source filename only
								{
									int args[2] = {0,0};
									ExtractValues(++c, args, 2);

									PCM_source *pSource = (PCM_source*) GetSetMediaItemTakeInfo(pTake, "P_SOURCE", NULL);
									if(pSource)
									{
										memset(buf, 0, sizeof(buf));
										GetMediaSourceFileName(pSource, buf, sizeof(buf));
										if(*buf)
										{
											const char *f = strrchr(buf, PATH_SLASH_CHAR);
											if(f)
												str.Append(GetSubString(f + 1, args[0], args[1]));
										}
									}
								}
								break;

							case 'T' : // Track name
								{
									int args[2] = {0,0};
									ExtractValues(++c, args, 2);

									const char *label = (const char*) GetSetMediaTrackInfo(GetMediaItem_Track(pItem), "P_NAME", NULL);
									if(label)
										str.Append(GetSubString(label, args[0], args[1]));
								}
								break;

							case 't' : // Track index
								{
									int width = 2;
									ExtractValues(++c, &width, 1);
									str.AppendFormatted(str.GetLength() + 8, "%0*d", width, iTrack);
								}
								break;
							}
						}
						break;

					default :
						str.Append(c++, 1);
						break;
					}
				}

				GetSetMediaItemTakeInfo(pTake, "P_NAME", (void*) str.Get());
			}

			++itemCount;
		}
	}

	if (undoName)
		Undo_EndBlock2(NULL, undoName, UNDO_STATE_ITEMS);

	UpdateTimeline();
}

//!WANT_LOCALIZE_1ST_STRING_BEGIN:sws_actions
static COMMAND_T g_commandTable[] =
{
	{ { DEFACCEL, "SWS/IX: Label processor" },	"IX_LABEL_PROC",	LabelProcessor,	NULL, 0},

	{ {}, LAST_COMMAND, }, // Denote end of table
};
//!WANT_LOCALIZE_1ST_STRING_END

int LabelInit()
{
	SWSRegisterCommands(g_commandTable);

	return 1;
}
