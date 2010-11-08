/******************************************************************************
/ SnM_Dlg.cpp
/
/ Copyright (c) 2009-2010 Tim Payne (SWS), Jeffos
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


int g_waitDlgProcCount = 0;


///////////////////////////////////////////////////////////////////////////////
// Custom WDL UIs
///////////////////////////////////////////////////////////////////////////////

void SNM_VirtualComboBox::OnPaint(LICE_IBitmap *drawbm, int origin_x, int origin_y, RECT *cliprect)
{
	if (GetFont()) GetFont()->SetBkMode(TRANSPARENT);

	RECT r;
	GetPosition(&r);
	r.left+=origin_x;
	r.right+=origin_x;
	r.top+=origin_y;
	r.bottom+=origin_y;

	int col = LICE_RGBA(255,255,255,50); 
	LICE_FillRect(drawbm,r.left,r.top,r.right-r.left,r.bottom-r.top,col,0.02f,LICE_BLIT_MODE_COPY);

	{
	  RECT tr=r;
	  tr.left=tr.right-(tr.bottom-tr.top);
	  LICE_FillRect(drawbm,tr.left,tr.top,tr.right-tr.left,tr.bottom-tr.top,col,0.02f,LICE_BLIT_MODE_COPY);
	}   

	if (GetFont() && GetItem(GetCurSel()) && GetItem(GetCurSel())[0])
	{
	  RECT tr=r;
	  tr.left+=4;
	  tr.right-=16;
	  GetFont()->DrawText(drawbm,GetItem(GetCurSel()),-1,&tr,DT_SINGLELINE|DT_VCENTER|DT_LEFT|DT_NOPREFIX);
	}

	int pencol = GetFont() ? GetFont()->GetTextColor() : WDL_STYLE_GetSysColor(COLOR_3DSHADOW);
	pencol = LICE_RGBA_FROMNATIVE(pencol,255);
	int pencol2 = GetFont() ? GetFont()->GetTextColor() : WDL_STYLE_GetSysColor(COLOR_3DHILIGHT);
	pencol2 = LICE_RGBA_FROMNATIVE(pencol2,255);

	// draw the down arrow button
	{
	  int bs=(r.bottom-r.top);
	  int l=r.right-bs;
	  int a=(bs/4)&~1;

	  LICE_Line(drawbm,l-1,r.top,l-1,r.bottom-1,pencol2,0.10f,LICE_BLIT_MODE_COPY,false);

	  int tcol = GetFont() ? GetFont()->GetTextColor() : WDL_STYLE_GetSysColor(COLOR_BTNTEXT);
	  tcol=LICE_RGBA_FROMNATIVE(tcol,255);

	  LICE_Line(drawbm,l+bs/2-a,r.top+bs/2-a/2,
					   l+bs/2,r.top+bs/2+a/2,tcol,0.50f,LICE_BLIT_MODE_COPY,true);
	  LICE_Line(drawbm,l+bs/2,r.top+bs/2+a/2,
					   l+bs/2+a,r.top+bs/2-a/2,tcol,0.50f,LICE_BLIT_MODE_COPY,true);
	}  

	// draw the border
	LICE_Line(drawbm,r.left,r.bottom-1,r.left,r.top,pencol,0.10f,0,false);
	LICE_Line(drawbm,r.left,r.top,r.right-1,r.top,pencol,0.10f,0,false);
	LICE_Line(drawbm,r.right-1,r.top,r.right-1,r.bottom-1,pencol2,0.10f,0,false);
	LICE_Line(drawbm,r.left,r.bottom-1,r.right-1,r.bottom-1,pencol2,0.10f,0,false);
}


///////////////////////////////////////////////////////////////////////////////
// Cue buss dialog box
///////////////////////////////////////////////////////////////////////////////

void fillHWoutDropDown(HWND _hwnd, int _idc)
{
	int x=0, x0=0;
	char buffer[BUFFER_SIZE] = "<None>";
	x0 = (int)SendDlgItemMessage(_hwnd,_idc,CB_ADDSTRING,0,(LPARAM)buffer);
	SendDlgItemMessage(_hwnd,_idc,CB_SETITEMDATA,x0,0);
	
	// get mono outputs
	WDL_PtrList<WDL_String> monos;
	int monoIdx=0;
	while (GetOutputChannelName(monoIdx))
	{
		monos.Add(new WDL_String(GetOutputChannelName(monoIdx)));
		monoIdx++;
	}

	// add stereo outputs
	WDL_PtrList<WDL_String> stereos;
	if (monoIdx)
	{
		for(int i=0; i < (monoIdx-1); i++)
		{
			WDL_String* hw = new WDL_String();
			hw->SetFormatted(256, "%s / %s", monos.Get(i)->Get(), monos.Get(i+1)->Get());
			stereos.Add(hw);
		}
	}

	// fill dropdown
	for(int i=0; i < stereos.GetSize(); i++)
	{
		x = (int)SendDlgItemMessage(_hwnd,_idc,CB_ADDSTRING,0,(LPARAM)stereos.Get(i)->Get());
		SendDlgItemMessage(_hwnd,_idc,CB_SETITEMDATA,x,i+1); // +1 for <none>
	}
	for(int i=0; i < monos.GetSize(); i++)
	{
		x = (int)SendDlgItemMessage(_hwnd,_idc,CB_ADDSTRING,0,(LPARAM)monos.Get(i)->Get());
		SendDlgItemMessage(_hwnd,_idc,CB_SETITEMDATA,x,i+1); // +1 for <none>
	}

//	SendDlgItemMessage(_hwnd,_idc,CB_SETCURSEL,x0,0);
	monos.Empty(true);
	stereos.Empty(true);
}

WDL_DLGRET CueBusDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	const char cWndPosKey[] = "CueBus Window Pos"; //JFB hum.. but kept not to mess REPAER.ini
	switch(Message)
	{
        case WM_INITDIALOG :
		{
			char busName[BUFFER_SIZE] = "";
			char trTemplatePath[BUFFER_SIZE] = "";
			int reaType, userType, soloDefeat, hwOuts[8];
			bool trTemplate, showRouting, sendToMaster;
			readCueBusIniFile(busName, &reaType, &trTemplate, trTemplatePath, &showRouting, &soloDefeat, &sendToMaster, hwOuts);
			userType = GetComboSendIdxType(reaType);
			SetDlgItemText(hwnd,IDC_SNM_CUEBUS_NAME,busName);

			int x=0;
			for(int i=1; i<4; i++)
			{
				x = (int)SendDlgItemMessage(hwnd,IDC_SNM_CUEBUS_TYPE,CB_ADDSTRING,0,(LPARAM)GetSendTypeStr(i));
				SendDlgItemMessage(hwnd,IDC_SNM_CUEBUS_TYPE,CB_SETITEMDATA,x,i);
				if (i==userType) SendDlgItemMessage(hwnd,IDC_SNM_CUEBUS_TYPE,CB_SETCURSEL,x,0);
			}

			SetDlgItemText(hwnd,IDC_SNM_CUEBUS_TEMPLATE,trTemplatePath);
			CheckDlgButton(hwnd, IDC_CHECK1, sendToMaster);
			CheckDlgButton(hwnd, IDC_CHECK2, showRouting);
			CheckDlgButton(hwnd, IDC_CHECK3, trTemplate);
			CheckDlgButton(hwnd, IDC_CHECK4, (soloDefeat == 1));

			for(int i=0; i < SNM_MAX_HW_OUTS; i++) 
			{
				fillHWoutDropDown(hwnd,IDC_SNM_CUEBUS_HWOUT1+i);
				SendDlgItemMessage(hwnd,IDC_SNM_CUEBUS_HWOUT1+i,CB_SETCURSEL,hwOuts[i],0);
			}

			RestoreWindowPos(hwnd, cWndPosKey, false);
			SetFocus(GetDlgItem(hwnd, IDC_SNM_CUEBUS_NAME));
			PostMessage(hwnd, WM_COMMAND, IDC_CHECK3, 0); // enable//disable state
			return 0;
		}
		break;

		case WM_COMMAND :
		{
            switch(LOWORD(wParam))
            {
                case IDOK:
				case IDC_SAVE:
				{
					char cueBusName[BUFFER_SIZE];
					GetDlgItemText(hwnd,IDC_SNM_CUEBUS_NAME,cueBusName,BUFFER_SIZE);

					int userType = 2, reaType;
					int combo = (int)SendDlgItemMessage(hwnd,IDC_SNM_CUEBUS_TYPE,CB_GETCURSEL,0,0);
					if(combo != CB_ERR)
						userType = combo+1;
					switch(userType)
					{
						case 1: reaType=0; break;
						case 2: reaType=3; break;
						case 3: reaType=1; break;
						default: break;
					}

					int sendToMaster = IsDlgButtonChecked(hwnd, IDC_CHECK1);
					int showRouting = IsDlgButtonChecked(hwnd, IDC_CHECK2);
					int trTemplate = IsDlgButtonChecked(hwnd, IDC_CHECK3);
					int soloDefeat = IsDlgButtonChecked(hwnd, IDC_CHECK4);

					char trTemplatePath[BUFFER_SIZE];
					GetDlgItemText(hwnd,IDC_SNM_CUEBUS_TEMPLATE,trTemplatePath,BUFFER_SIZE);

					int hwOuts[SNM_MAX_HW_OUTS];
					for (int i=0; i<SNM_MAX_HW_OUTS; i++)
					{
						hwOuts[i] = (int)SendDlgItemMessage(hwnd,IDC_SNM_CUEBUS_HWOUT1+i,CB_GETCURSEL,0,0);
						if(hwOuts[i] == CB_ERR)	hwOuts[i] = 0;
					}

					// *** Create cue buss ***
					if (LOWORD(wParam) == IDC_SAVE ||
						cueTrack(cueBusName, reaType, "Create cue buss",
							(showRouting == 1), soloDefeat, 
							trTemplate ? trTemplatePath : NULL, 
							(sendToMaster == 1), hwOuts))
					{
						saveCueBusIniFile(cueBusName, reaType, (trTemplate == 1), trTemplatePath, (showRouting == 1), soloDefeat, (sendToMaster == 1), hwOuts);
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

				case IDC_FILES:
				{
					char currentPath[BUFFER_SIZE];
					GetDlgItemText(hwnd,IDC_SNM_CUEBUS_TEMPLATE,currentPath,BUFFER_SIZE);
					if (!strlen(currentPath))
						_snprintf(currentPath, BUFFER_SIZE, "%s%c%TrackTemplates", GetResourcePath(), PATH_SLASH_CHAR);
					char* filename = BrowseForFiles("Load track template", currentPath, NULL, false, "REAPER Track Template (*.RTrackTemplate)\0*.RTrackTemplate\0");
					if (filename) {
						SetDlgItemText(hwnd,IDC_SNM_CUEBUS_TEMPLATE,filename);
						free(filename);
					}
				}
				break;

				case IDC_CHECK3:
				{
					bool templateEnable = (IsDlgButtonChecked(hwnd, IDC_CHECK3) == 1);
					EnableWindow(GetDlgItem(hwnd, IDC_SNM_CUEBUS_TEMPLATE), templateEnable);
					EnableWindow(GetDlgItem(hwnd, IDC_FILES), templateEnable);
					EnableWindow(GetDlgItem(hwnd, IDC_SNM_CUEBUS_NAME), !templateEnable);
					for(int k=0; k < SNM_MAX_HW_OUTS ; k++)
						EnableWindow(GetDlgItem(hwnd, IDC_SNM_CUEBUS_HWOUT1+k), !templateEnable);
					EnableWindow(GetDlgItem(hwnd, IDC_CHECK1), !templateEnable);
					EnableWindow(GetDlgItem(hwnd, IDC_CHECK4), !templateEnable);
					SetFocus(GetDlgItem(hwnd, templateEnable ? IDC_SNM_CUEBUS_TEMPLATE : IDC_SNM_CUEBUS_NAME));
				}
				break;

				default:
					break;
			}
		}
		break;

		case WM_DESTROY:
			SaveWindowPos(hwnd, cWndPosKey);
			break; 

		default:
			break;
	}

	return 0;
}


///////////////////////////////////////////////////////////////////////////////
// WaitDlgProc
///////////////////////////////////////////////////////////////////////////////

WDL_DLGRET WaitDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
		case WM_INITDIALOG:
			SetTimer(hwnd, 1, 1, NULL);
			break;
/*
		case WM_COMMAND:
			if ((LOWORD(wParam)==IDOK || LOWORD(wParam)==IDCANCEL))
			{
				EndDialog(hwnd,0);
				g_waitDlgProcCount = 0;
			}
			break;
*/
		case WM_TIMER:
			{
				SendDlgItemMessage(hwnd, IDC_EDIT, PBM_SETRANGE, 0, MAKELPARAM(0, LET_BREATHE_MS));
				if (g_waitDlgProcCount < LET_BREATHE_MS)
				{
					SendDlgItemMessage(hwnd, IDC_EDIT, PBM_SETPOS, (WPARAM) g_waitDlgProcCount, 0);
					g_waitDlgProcCount++;
				}
				else
				{
					EndDialog(hwnd,0);
					g_waitDlgProcCount = 0;
				}
			}
			break;
	}
	return 0;
}
