/******************************************************************************
/ AutoRename.cpp
/
/ Copyright (c) 2009 Tim Payne (SWS), original code by Xenakios
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

#include "../Breeder/BR_Util.h"
#include "../SnM/SnM_Dlg.h"

#include <WDL/localize/localize.h>

#define LAST_AUTORENAME_STR_KEY "Last autorename string"

using namespace std;

vector<MediaItem_Take*> g_VecTakesToRename;
vector<MediaItem_Take*> g_VecTakesToRenameTimeOrdered;

int stringFindAndErase(string& str, const char* findStr)
{
	string::size_type pos;
	pos = str.find(findStr);
	if (pos == string::npos)
		return -1;
	str.erase(pos, strlen(findStr));
	return (int)pos;
}

string GetNewTakeNameFromFormatString(const char *formatString, MediaItem_Take *take, int takeNumber)
{
	string newName = formatString;
	int pos;
	// Use "while" for everything to ensure multiples of the same tag are taken care of
	while((pos = stringFindAndErase(newName, "[takename]")) >= 0)
	{
		const char* name = (const char*)GetSetMediaItemTakeInfo(take, "P_NAME", NULL);
		if (name)
			newName.insert(pos, name);
	}
	while((pos = stringFindAndErase(newName, "[takenamenoext]")) >= 0)
	{
		const char* name = (const char*)GetSetMediaItemTakeInfo(take, "P_NAME", NULL);
		if (name)
		{
			char str[256];
			lstrcpyn(str, name, 256);
			char* pExt = strrchr(str, '.');
			if (pExt)
				*pExt = 0;
			newName.insert(pos, str);
		}
	}
	while((pos = stringFindAndErase(newName, "[trackname]")) >= 0)
	{
		const char* name = (const char*)GetSetMediaTrackInfo((MediaTrack*)GetSetMediaItemTakeInfo(take, "P_TRACK", NULL), "P_NAME", NULL);
		if (name)
			newName.insert(pos, name);
	}
	while((pos = stringFindAndErase(newName, "[foldername]")) >= 0)
	{
		MediaTrack* tr = (MediaTrack*)GetSetMediaTrackInfo((MediaTrack*)GetSetMediaItemTakeInfo(take, "P_TRACK", NULL), "P_PARTRACK", NULL);
		if (tr)
		{
			const char* name = (const char*)GetSetMediaTrackInfo(tr, "P_NAME", NULL);
			if (name)
				newName.insert(pos, name);
		}
	}
	while((pos = stringFindAndErase(newName, "[tracknum]")) >= 0)
	{
		char str[16];
		sprintf(str, "%.2d", CSurf_TrackToID((MediaTrack*)GetSetMediaItemTakeInfo(take, "P_TRACK", NULL), false));
		newName.insert(pos, str);
	}
	while((pos = stringFindAndErase(newName, "[GUID]")) >= 0)
	{
		char cGUID[64];
		guidToString((GUID*)GetSetMediaItemTakeInfo(take, "GUID", NULL), cGUID);
		newName.insert(pos, cGUID);
	}
	while((pos = stringFindAndErase(newName, "[inctrackorder]")) >= 0 || (pos = stringFindAndErase(newName, "[inctimeorder]")) >= 0)
	{
		char str[16];
		sprintf(str, "%.2d", takeNumber);
		newName.insert(pos, str);
	}
	return newName;
}

void UpdateSampleNameList(HWND hDlg, const char* formatString)
{
	vector<MediaItem_Take*>* pTakes = &g_VecTakesToRename;
	bool bTimeOrder = false;
	if (strstr(formatString, "[inctimeorder]"))
	{
		bTimeOrder = true;
		pTakes = &g_VecTakesToRenameTimeOrdered;
	}

	ListView_DeleteAllItems(GetDlgItem(hDlg,IDC_AUTONAMEOUTPUT));
	int trackItemCounter = 0;
	MediaTrack* trPrev = NULL;

	for (int i = 0; i < (int)pTakes->size();i++)
	{
		MediaTrack* tr = (MediaTrack*)GetSetMediaItemTakeInfo((*pTakes)[i], "P_TRACK", NULL);
		if (!bTimeOrder && tr != trPrev)
		{
			trackItemCounter = 0;
			trPrev = tr;
		}
		LVITEM item;
		item.mask = LVIF_TEXT;
		item.iItem = i;
		item.iSubItem = 0;
		item.pszText = (char*)GetSetMediaItemTakeInfo((*pTakes)[i], "P_NAME", NULL);
		ListView_InsertItem(GetDlgItem(hDlg, IDC_AUTONAMEOUTPUT), &item);
		string SampleName = GetNewTakeNameFromFormatString(formatString,(*pTakes)[i], trackItemCounter + 1);
		ListView_SetItemText(GetDlgItem(hDlg, IDC_AUTONAMEOUTPUT), i, 1, (char*)SampleName.c_str());
		trackItemCounter++;
	}
}

typedef struct
{
	string Description;
	string FormattingString;
} t_autorenamepreset;

vector<t_autorenamepreset> g_AutoNamePresets;

void InitPresets()
{
	char file[512];
	FILE* f;
	snprintf(file, 512, "%s%cSWS-RenamePresets.txt", GetResourcePath(), PATH_SLASH_CHAR);
	if (!FileExists(file))
	{	// Generate defaults
		f = fopen(file, "w");
		if (f)
		{
			char buf[512];
			strcpy(buf, "Increasing suffix, track-by-track==[takename]-[inctrackorder]\n");
			fwrite(buf, strlen(buf), 1, f);
			strcpy(buf, "Increasing suffix, time order across all tracks==[takename]-[inctimeorder]\n");
			fwrite(buf, strlen(buf), 1, f);
			strcpy(buf, "Folder.Track.Increasing suffix==[foldername].[trackname].[inctrackorder]\n");
			fwrite(buf, strlen(buf), 1, f);
			strcpy(buf, "Strip file extension==[takenamenoext]\n");
			fwrite(buf, strlen(buf), 1, f);
			fclose(f);
		}
	}
	f = fopen(file, "r");
	if (f)
	{
		g_AutoNamePresets.clear();

		char buf[512];
		while (fgets(buf, 512, f))
		{
			char* pFormat = strstr(buf, "==");
			if (pFormat)
			{
				*pFormat=0;
				pFormat += 2;
				int iFL = (int)strlen(pFormat);
				if (pFormat[iFL-1] == '\n')
					pFormat[iFL-1] = 0;
				if (buf[0] && pFormat[0])
				{
					t_autorenamepreset preset;
					preset.Description = buf;
					preset.FormattingString = pFormat;
					g_AutoNamePresets.push_back(preset);
				}
			}
		}
		fclose(f);
	}
}

WDL_DLGRET AutoRenameDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	if (ThemeListViewInProc(hwnd, Message, lParam, GetDlgItem(hwnd,IDC_AUTONAMEOUTPUT), true))
		return 1;
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwnd, Message, wParam, lParam))
		return r;

	switch(Message)
    {
        case WM_INITDIALOG:
		{
			WDL_UTF8_HookComboBox(GetDlgItem(hwnd,IDC_AUTONAMEPRESETS));
			if (SWS_THEMING) SNM_ThemeListView(GetDlgItem(hwnd,IDC_AUTONAMEOUTPUT));

			for (int i = 0; i < (int)g_AutoNamePresets.size(); i++)
			{
				// fill preset combobox
				SendMessage(GetDlgItem(hwnd,IDC_AUTONAMEPRESETS), CB_ADDSTRING, 0, (LPARAM)g_AutoNamePresets[i].Description.c_str());
				SendMessage(GetDlgItem(hwnd,IDC_AUTONAMEPRESETS), CB_SETITEMDATA, i, (LPARAM)&g_AutoNamePresets[i].FormattingString);
			}

			char buf[500];
			GetPrivateProfileString(SWS_INI, LAST_AUTORENAME_STR_KEY, "[trackname]-Take[inctrackorder]", buf, 500, get_ini_file());
			SetDlgItemText(hwnd,IDC_EDIT1,buf);

			LVCOLUMN col;
			col.mask=LVCF_TEXT|LVCF_WIDTH;
			col.cx=242;
			col.pszText=(char*)__LOCALIZE("Current take name","sws_DLG_136");
			ListView_InsertColumn(GetDlgItem(hwnd,IDC_AUTONAMEOUTPUT), 0 , &col);

			col.mask=LVCF_TEXT|LVCF_WIDTH;
			col.cx=242;
			col.pszText=(char*)__LOCALIZE("New take name","sws_DLG_136");
			ListView_InsertColumn(GetDlgItem(hwnd,IDC_AUTONAMEOUTPUT), 1 , &col);

			UpdateSampleNameList(hwnd, buf);
			break;
		}
		case WM_COMMAND:
		{
			if (HIWORD(wParam) == EN_CHANGE && LOWORD(wParam) == IDC_EDIT1)
			{
				char buf[500];
				GetDlgItemText(hwnd, IDC_EDIT1, buf, 500);
				UpdateSampleNameList(hwnd, buf);
				break;
			}
			if (HIWORD(wParam) == CBN_SELCHANGE && LOWORD(wParam) == IDC_AUTONAMEPRESETS)
			{
				int comboIndex = (int)SendMessage(GetDlgItem(hwnd, IDC_AUTONAMEPRESETS), CB_GETCURSEL, 0, 0);
				if (comboIndex >= 0 && comboIndex < (int)g_AutoNamePresets.size())
				{
					string* formattingString = (string*)SendMessage(GetDlgItem(hwnd,IDC_AUTONAMEPRESETS),CB_GETITEMDATA, comboIndex, 0);
					if (formattingString)
						SetDlgItemText(hwnd, IDC_EDIT1, formattingString->c_str());
				}
				break;
			}
			switch(LOWORD(wParam))
			{
				case IDCANCEL:
					EndDialog(hwnd, 0);
					break;
				case IDOK:
				{
					char buf[500];
					GetDlgItemText(hwnd, IDC_EDIT1, buf, 500);
					WritePrivateProfileString(SWS_INI, LAST_AUTORENAME_STR_KEY, buf, get_ini_file());

					// Do the work!
					string NewTakeName;
					MediaTrack* PrevTrack = NULL;
					int TrackItemCounter = 0;
					bool bTimeOrder = false;
					if (strstr(buf, "[inctimeorder]"))
					{
						g_VecTakesToRename = g_VecTakesToRenameTimeOrdered;
						bTimeOrder = true;
					}

					for (int i = 0; i < (int)g_VecTakesToRename.size(); i++)
					{
						NewTakeName.assign("");
						MediaItem* CurItem = (MediaItem*)GetSetMediaItemTakeInfo(g_VecTakesToRename[i],"P_ITEM",NULL);
						MediaTrack* CurTrack = (MediaTrack*)GetSetMediaItemInfo(CurItem,"P_TRACK",NULL);
						if (!bTimeOrder && CurTrack != PrevTrack)
						{
							TrackItemCounter = 0;
							PrevTrack = CurTrack;
						}
						string SampleName = GetNewTakeNameFromFormatString(buf, g_VecTakesToRename[i], TrackItemCounter+1);
						GetSetMediaItemTakeInfo(g_VecTakesToRename[i], "P_NAME", (void*)SampleName.c_str());
						TrackItemCounter++;
					}

					Undo_OnStateChangeEx(__LOCALIZE("Autorename takes","sws_undo"),4,-1);
					UpdateTimeline();

					EndDialog(hwnd,0);
					break;
				}
			}
		}
	}
	return 0;
}

bool MyTakeSortByTimeFunc (MediaItem_Take *a,MediaItem_Take *b)
{
	MediaItem *CompaItem;
	CompaItem=(MediaItem*)GetSetMediaItemTakeInfo(a,"P_ITEM",NULL);
	double ItemPosA=*(double*)GetSetMediaItemInfo(CompaItem,"D_POSITION",NULL);
	CompaItem=(MediaItem*)GetSetMediaItemTakeInfo(b,"P_ITEM",NULL);
	double ItemPosB=*(double*)GetSetMediaItemInfo(CompaItem,"D_POSITION",NULL);
	if (ItemPosA<ItemPosB) return true;
	return false;
}

void DoAutoRename(COMMAND_T*)
{
	g_VecTakesToRename.clear();
	const int cnt=CountSelectedMediaItems(NULL);
	for (int i = 0; i < cnt; i++)
	{
		MediaItem* item = GetSelectedMediaItem(NULL, i);
		MediaItem_Take* take = GetMediaItemTake(item, -1);
		if (take)
			g_VecTakesToRename.push_back(take);
	}
	if (g_VecTakesToRename.size() > 0)
	{
		g_VecTakesToRenameTimeOrdered.assign(g_VecTakesToRename.begin(),g_VecTakesToRename.end());
		sort(g_VecTakesToRenameTimeOrdered.begin(),g_VecTakesToRenameTimeOrdered.end(),MyTakeSortByTimeFunc);
		InitPresets();

		DialogBox(g_hInst, MAKEINTRESOURCE(IDD_AUTORENAMETAKES), g_hwndParent, (DLGPROC)AutoRenameDlgProc);
	}
	else
		MessageBox(g_hwndParent, __LOCALIZE("No selected item!","sws_mbox"), __LOCALIZE("Xenakios - Error","sws_mbox"), MB_OK);
}
