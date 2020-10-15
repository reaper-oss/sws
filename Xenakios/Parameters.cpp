/******************************************************************************
/ Parameters.cpp
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

#include "../SnM/SnM_Dlg.h"
#include "Parameters.h"

#include <WDL/localize/localize.h>

using namespace std;

t_command_params g_command_params;
t_external_app_paths g_external_app_paths;

bool StartsWithString(char *FullString,char *PartialString)
{
	return strstr(FullString, PartialString) == FullString;
}

void DoLaunchExtTool(COMMAND_T* t)
{
	if (t->user == 1)
	{
		if (!DoLaunchExternalTool(g_external_app_paths.PathToTool1))
			MessageBox(g_hwndParent, __LOCALIZE("Could not execute external tool!\n(Set it in 'Xenakios/SWS: Command parameters')","sws_mbox"),__LOCALIZE("Xenakios - Error","sws_mbox"), MB_OK);
	}
	else if (t->user == 2)
	{
		if (!DoLaunchExternalTool(g_external_app_paths.PathToTool2))
			MessageBox(g_hwndParent, __LOCALIZE("Could not execute external tool!\n(Set it in 'Xenakios/SWS: Command parameters')","sws_mbox"),__LOCALIZE("Xenakios - Error","sws_mbox"), MB_OK);
	}
}

void ReadINIfile()
{
	char resultString[512];
	GetPrivateProfileString("XENAKIOSCOMMANDS","ITEMPOSNUDGESECS","1.0",resultString,512,g_XenIniFilename.Get());
	g_command_params.ItemPosNudgeSecs=atof(resultString);
	GetPrivateProfileString("XENAKIOSCOMMANDS","ITEMPOSNUDGEBEATS","1.0",resultString,512,g_XenIniFilename.Get());
	g_command_params.ItemPosNudgeBeats=atof(resultString);
	GetPrivateProfileString("XENAKIOSCOMMANDS","FADEINTIMEA","0.001",resultString,512,g_XenIniFilename.Get());
	g_command_params.CommandFadeInA=atof(resultString);
	GetPrivateProfileString("XENAKIOSCOMMANDS","FADEINTIMEB","0.001",resultString,512,g_XenIniFilename.Get());
	g_command_params.CommandFadeInB=atof(resultString);
	GetPrivateProfileString("XENAKIOSCOMMANDS","FADEOUTTIMEA","0.001",resultString,512,g_XenIniFilename.Get());
	g_command_params.CommandFadeOutA=atof(resultString);
	GetPrivateProfileString("XENAKIOSCOMMANDS","FADEOUTTIMEB","0.001",resultString,512,g_XenIniFilename.Get());
	g_command_params.CommandFadeOutB=atof(resultString);
	GetPrivateProfileString("XENAKIOSCOMMANDS","FADEINSHAPEA","0",resultString,512,g_XenIniFilename.Get());
	g_command_params.CommandFadeInShapeA=atoi(resultString);
	GetPrivateProfileString("XENAKIOSCOMMANDS","FADEINSHAPEB","0",resultString,512,g_XenIniFilename.Get());
	g_command_params.CommandFadeInShapeB=atoi(resultString);
	GetPrivateProfileString("XENAKIOSCOMMANDS","FADEOUTSHAPEA","0",resultString,512,g_XenIniFilename.Get());
	g_command_params.CommandFadeOutShapeA=atoi(resultString);
	GetPrivateProfileString("XENAKIOSCOMMANDS","FADEOUTSHAPEB","0",resultString,512,g_XenIniFilename.Get());
	g_command_params.CommandFadeOutShapeB=atoi(resultString);
	
	GetPrivateProfileString("XENAKIOSCOMMANDS","EDITCURRNDMEAN","1.0",resultString,512,g_XenIniFilename.Get());
	g_command_params.EditCurRndMean=atof(resultString);

	GetPrivateProfileString("XENAKIOSCOMMANDS","ITEMVOLUMENUDGE","1.0",resultString,512,g_XenIniFilename.Get());
	g_command_params.ItemVolumeNudge=atof(resultString);

	GetPrivateProfileString("XENAKIOSCOMMANDS","ITEMPITCHNUDGE","1.0",resultString,512,g_XenIniFilename.Get());
	g_command_params.ItemPitchNudgeA=atof(resultString);
	GetPrivateProfileString("XENAKIOSCOMMANDS","ITEMPITCHNUDGEB","1.0",resultString,512,g_XenIniFilename.Get());
	g_command_params.ItemPitchNudgeB=atof(resultString);

	GetPrivateProfileString("XENAKIOSCOMMANDS","RNDITEMSELPROB","50.0",resultString,512,g_XenIniFilename.Get());
	g_command_params.RndItemSelProb=atof(resultString);

	delete [] g_external_app_paths.PathToTool1; g_external_app_paths.PathToTool1 = NULL;
	delete [] g_external_app_paths.PathToTool2; g_external_app_paths.PathToTool2 = NULL;
	delete [] g_external_app_paths.PathToAudioEditor1; g_external_app_paths.PathToAudioEditor1 = NULL;
	delete [] g_external_app_paths.PathToAudioEditor2; g_external_app_paths.PathToAudioEditor2 = NULL;

	GetPrivateProfileString("XENAKIOSCOMMANDS","EXTERNALTOOL1PATH","",resultString,512,g_XenIniFilename.Get());
	if (resultString[0])
	{
		g_external_app_paths.PathToTool1=new char[strlen(resultString)+sizeof(char)];
		strcpy(g_external_app_paths.PathToTool1, resultString);
	}

	GetPrivateProfileString("XENAKIOSCOMMANDS","EXTERNALTOOL2PATH","",resultString,512,g_XenIniFilename.Get());
	if (resultString[0])
	{
		g_external_app_paths.PathToTool2=new char[strlen(resultString)+sizeof(char)];
		strcpy(g_external_app_paths.PathToTool2, resultString);
	}

	GetPrivateProfileString("XENAKIOSCOMMANDS","EXTERNALEDITOR1PATH","",resultString,512,g_XenIniFilename.Get());
	if (resultString[0])
	{
		g_external_app_paths.PathToAudioEditor1=new char[strlen(resultString)+sizeof(char)];
		strcpy(g_external_app_paths.PathToAudioEditor1, resultString);
	}

	GetPrivateProfileString("XENAKIOSCOMMANDS","EXTERNALEDITOR2PATH","",resultString,512,g_XenIniFilename.Get());
	if (resultString[0])
	{
		g_external_app_paths.PathToAudioEditor2=new char[strlen(resultString)+sizeof(char)];
		strcpy(g_external_app_paths.PathToAudioEditor2, resultString);
	}

	GetPrivateProfileString("XENAKIOSCOMMANDS","PIXELAMOUNT","12",resultString,512,g_XenIniFilename.Get());
	g_command_params.PixAmount=atoi(resultString);

	GetPrivateProfileString("XENAKIOSCOMMANDS","SECTLOOPNUDGESECS","0.1",resultString,512,g_XenIniFilename.Get());
	g_command_params.SectionLoopNudgeSecs=atof(resultString);

	//CURPOSSECSAMOUNT

	GetPrivateProfileString("XENAKIOSCOMMANDS","CURPOSSECSAMOUNT","0.005",resultString,512,g_XenIniFilename.Get());
	g_command_params.CurPosSecsAmount=atof(resultString);

	GetPrivateProfileString("XENAKIOSCOMMANDS","TRACKHEIGHTA","50",resultString,512,g_XenIniFilename.Get());
	g_command_params.TrackHeight[0]=atoi(resultString);
	GetPrivateProfileString("XENAKIOSCOMMANDS","TRACKHEIGHTB","50",resultString,512,g_XenIniFilename.Get());
	g_command_params.TrackHeight[1]=atoi(resultString);
	
	GetPrivateProfileString("XENAKIOSCOMMANDS","TRACKLABELDEFAULT","Audio",resultString,512,g_XenIniFilename.Get());
	g_command_params.DefaultTrackLabel.assign(resultString);
	GetPrivateProfileString("XENAKIOSCOMMANDS","TRACKLABELPREFIX","",resultString,512,g_XenIniFilename.Get());
	g_command_params.TrackLabelPrefix.assign(resultString);
	GetPrivateProfileString("XENAKIOSCOMMANDS","TRACKLABELSUFFIX","",resultString,512,g_XenIniFilename.Get());
	g_command_params.TrackLabelSuffix.assign(resultString);

	GetPrivateProfileString("XENAKIOSCOMMANDS","TRACKVOLNUDGEDB","1.0",resultString,512,g_XenIniFilename.Get());
	g_command_params.TrackVolumeNudge=atof(resultString);
}


void UpdateINIfile()
{
	char TextBuf[512];
	sprintf(TextBuf,"%f",g_command_params.TrackVolumeNudge);
	WritePrivateProfileString("XENAKIOSCOMMANDS","TRACKVOLNUDGEDB",TextBuf,g_XenIniFilename.Get());
	sprintf(TextBuf,"%f",g_command_params.ItemPosNudgeSecs);
	WritePrivateProfileString("XENAKIOSCOMMANDS","ITEMPOSNUDGESECS",TextBuf,g_XenIniFilename.Get());
	sprintf(TextBuf,"%f",g_command_params.ItemPosNudgeBeats);
	WritePrivateProfileString("XENAKIOSCOMMANDS","ITEMPOSNUDGEBEATS",TextBuf,g_XenIniFilename.Get());
	sprintf(TextBuf,"%f",g_command_params.CommandFadeInA);
	WritePrivateProfileString("XENAKIOSCOMMANDS","FADEINTIMEA",TextBuf,g_XenIniFilename.Get());
	sprintf(TextBuf,"%f",g_command_params.CommandFadeInB);
	WritePrivateProfileString("XENAKIOSCOMMANDS","FADEINTIMEB",TextBuf,g_XenIniFilename.Get());
	sprintf(TextBuf,"%f",g_command_params.CommandFadeOutA);
	WritePrivateProfileString("XENAKIOSCOMMANDS","FADEOUTTIMEA",TextBuf,g_XenIniFilename.Get());
	sprintf(TextBuf,"%f",g_command_params.CommandFadeOutB);
	WritePrivateProfileString("XENAKIOSCOMMANDS","FADEOUTTIMEB",TextBuf,g_XenIniFilename.Get());
	sprintf(TextBuf,"%d",g_command_params.CommandFadeInShapeA);
	WritePrivateProfileString("XENAKIOSCOMMANDS","FADEINSHAPEA",TextBuf,g_XenIniFilename.Get());
	sprintf(TextBuf,"%d",g_command_params.CommandFadeInShapeB);
	WritePrivateProfileString("XENAKIOSCOMMANDS","FADEINSHAPEB",TextBuf,g_XenIniFilename.Get());
	sprintf(TextBuf,"%d",g_command_params.CommandFadeOutShapeA);
	WritePrivateProfileString("XENAKIOSCOMMANDS","FADEOUTSHAPEA",TextBuf,g_XenIniFilename.Get());
	sprintf(TextBuf,"%d",g_command_params.CommandFadeOutShapeB);
	WritePrivateProfileString("XENAKIOSCOMMANDS","FADEOUTSHAPEB",TextBuf,g_XenIniFilename.Get());
	
	sprintf(TextBuf,"%f",g_command_params.EditCurRndMean);
	WritePrivateProfileString("XENAKIOSCOMMANDS","EDITCURRNDMEAN",TextBuf,g_XenIniFilename.Get());
	
	sprintf(TextBuf,"%f",g_command_params.ItemVolumeNudge);
	WritePrivateProfileString("XENAKIOSCOMMANDS","ITEMVOLUMENUDGE",TextBuf,g_XenIniFilename.Get());
	
	sprintf(TextBuf,"%f",g_command_params.ItemPitchNudgeA);
	WritePrivateProfileString("XENAKIOSCOMMANDS","ITEMPITCHNUDGE",TextBuf,g_XenIniFilename.Get());
	sprintf(TextBuf,"%f",g_command_params.ItemPitchNudgeB);
	WritePrivateProfileString("XENAKIOSCOMMANDS","ITEMPITCHNUDGEB",TextBuf,g_XenIniFilename.Get());
	
	sprintf(TextBuf,"%f",g_command_params.RndItemSelProb);
	WritePrivateProfileString("XENAKIOSCOMMANDS","RNDITEMSELPROB",TextBuf,g_XenIniFilename.Get());
	if (g_external_app_paths.PathToTool1)
		WritePrivateProfileString("XENAKIOSCOMMANDS","EXTERNALTOOL1PATH",g_external_app_paths.PathToTool1,g_XenIniFilename.Get());
	if (g_external_app_paths.PathToTool2)
		WritePrivateProfileString("XENAKIOSCOMMANDS","EXTERNALTOOL2PATH",g_external_app_paths.PathToTool2,g_XenIniFilename.Get());
	if (g_external_app_paths.PathToAudioEditor1)
		WritePrivateProfileString("XENAKIOSCOMMANDS","EXTERNALEDITOR1PATH",g_external_app_paths.PathToAudioEditor1,g_XenIniFilename.Get());
	if (g_external_app_paths.PathToAudioEditor2)
		WritePrivateProfileString("XENAKIOSCOMMANDS","EXTERNALEDITOR2PATH",g_external_app_paths.PathToAudioEditor2,g_XenIniFilename.Get());

	sprintf(TextBuf,"%d",g_command_params.PixAmount);
	WritePrivateProfileString("XENAKIOSCOMMANDS","PIXELAMOUNT",TextBuf,g_XenIniFilename.Get());

	sprintf(TextBuf,"%f",g_command_params.CurPosSecsAmount);
	WritePrivateProfileString("XENAKIOSCOMMANDS","CURPOSSECSAMOUNT",TextBuf,g_XenIniFilename.Get());
	
	sprintf(TextBuf,"%d",g_command_params.TrackHeight[0]);
	WritePrivateProfileString("XENAKIOSCOMMANDS","TRACKHEIGHTA",TextBuf,g_XenIniFilename.Get());
	sprintf(TextBuf,"%d",g_command_params.TrackHeight[1]);
	WritePrivateProfileString("XENAKIOSCOMMANDS","TRACKHEIGHTB",TextBuf,g_XenIniFilename.Get());

	sprintf(TextBuf,"%f",g_command_params.SectionLoopNudgeSecs);
	WritePrivateProfileString("XENAKIOSCOMMANDS","SECTLOOPNUDGESECS",TextBuf,g_XenIniFilename.Get());

	WritePrivateProfileString("XENAKIOSCOMMANDS","TRACKLABELDEFAULT",g_command_params.DefaultTrackLabel.c_str(),g_XenIniFilename.Get());
	WritePrivateProfileString("XENAKIOSCOMMANDS","TRACKLABELPREFIX",g_command_params.TrackLabelPrefix.c_str(),g_XenIniFilename.Get());
	WritePrivateProfileString("XENAKIOSCOMMANDS","TRACKLABELSUFFIX",g_command_params.TrackLabelSuffix.c_str(),g_XenIniFilename.Get());
}

WDL_DLGRET ExoticParamsDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwnd, uMsg, wParam, lParam))
		return r;
	
	switch(uMsg)
    {
		case WM_INITDIALOG:
		{
			ReadINIfile();
			char textBuf[316];
		
			sprintf(textBuf,"%.2f",g_command_params.TrackVolumeNudge);
			SetDlgItemText(hwnd, IDC_TVOL_NUDGE, textBuf);
			
			sprintf(textBuf,"%d",g_command_params.TrackHeight[0]);
			SetDlgItemText(hwnd, IDC_THGT_A, textBuf);
			sprintf(textBuf,"%d",g_command_params.TrackHeight[1]);
			SetDlgItemText(hwnd, IDC_THGT_B, textBuf);

			sprintf(textBuf,"%.4f",g_command_params.SectionLoopNudgeSecs);
			SetDlgItemText(hwnd, IDC_COMPARSECTLOOP,textBuf);

			sprintf(textBuf,"%.2f",g_command_params.EditCurRndMean);
			SetDlgItemText(hwnd, IDC_RANDMEAN, textBuf);
			sprintf(textBuf,"%.1f",g_command_params.RndItemSelProb);
			SetDlgItemText(hwnd, IDC_RANDPROB, textBuf);
			sprintf(textBuf,"%.4f",g_command_params.ItemPosNudgeSecs);
			SetDlgItemText(hwnd, IDC_NUDGE_S, textBuf);
			sprintf(textBuf,"%.4f",g_command_params.ItemPosNudgeBeats);
			SetDlgItemText(hwnd, IDC_NUDGE_B, textBuf);
			
			sprintf(textBuf,"%.3f",g_command_params.ItemPitchNudgeA);
			SetDlgItemText(hwnd, IDC_PNUDGE_A, textBuf);
			sprintf(textBuf,"%.3f",g_command_params.ItemPitchNudgeB);
			SetDlgItemText(hwnd, IDC_PNUDGE_B, textBuf);
			
			sprintf(textBuf,"%.3f",g_command_params.CommandFadeInA);
			SetDlgItemText(hwnd, IDC_FIT_A, textBuf);
			sprintf(textBuf,"%.3f",g_command_params.CommandFadeOutA);
			SetDlgItemText(hwnd, IDC_FOT_A, textBuf);
			sprintf(textBuf,"%d",g_command_params.CommandFadeOutShapeA);
			SetDlgItemText(hwnd, IDC_FOS_A, textBuf);
			sprintf(textBuf,"%d",g_command_params.CommandFadeInShapeA);
			SetDlgItemText(hwnd, IDC_FIS_A, textBuf);

			sprintf(textBuf,"%.3f",g_command_params.CommandFadeInB);
			SetDlgItemText(hwnd, IDC_FIT_B, textBuf);
			sprintf(textBuf,"%.3f",g_command_params.CommandFadeOutB);
			SetDlgItemText(hwnd, IDC_FOT_B, textBuf);
			sprintf(textBuf,"%d",g_command_params.CommandFadeOutShapeB);
			SetDlgItemText(hwnd, IDC_FOS_B, textBuf);
			sprintf(textBuf,"%d",g_command_params.CommandFadeInShapeB);
			SetDlgItemText(hwnd, IDC_FIS_B, textBuf);

			sprintf(textBuf,"%d",g_command_params.PixAmount);
			SetDlgItemText(hwnd, IDC_EDITPIXAM, textBuf);
			sprintf(textBuf,"%.3f",g_command_params.CurPosSecsAmount);
			SetDlgItemText(hwnd, IDC_EDITCURSECS, textBuf);

			sprintf(textBuf,"%.2f",g_command_params.ItemVolumeNudge);
			SetDlgItemText(hwnd, IDC_IVOL_NUDGE, textBuf);

			if (g_external_app_paths.PathToTool1)
				SetDlgItemText(hwnd,IDC_EXTTOOLPATH1,g_external_app_paths.PathToTool1);
			if (g_external_app_paths.PathToTool2)
				SetDlgItemText(hwnd,IDC_EXTTOOLPATH2,g_external_app_paths.PathToTool2);
			if (g_external_app_paths.PathToAudioEditor1)
				SetDlgItemText(hwnd,IDC_EXTEDITPATH1,g_external_app_paths.PathToAudioEditor1);
			if (g_external_app_paths.PathToAudioEditor2)
				SetDlgItemText(hwnd,IDC_EXTEDITPATH2,g_external_app_paths.PathToAudioEditor2);
			
			SetDlgItemText(hwnd,IDC_EDITDEFAULTTRACKNAME,g_command_params.DefaultTrackLabel.c_str());
			SetDlgItemText(hwnd,IDC_EDITTRPREFIX,g_command_params.TrackLabelPrefix.c_str());
			SetDlgItemText(hwnd,IDC_EDITTRSUFFIX,g_command_params.TrackLabelSuffix.c_str());
			break;
		}
		case WM_COMMAND:
			switch(LOWORD(wParam))
            {
                case IDOK:
				{
					char textbuf[101];
					
					GetDlgItemText(hwnd,IDC_TVOL_NUDGE,textbuf,100);
					g_command_params.TrackVolumeNudge=atof(textbuf);
					
					GetDlgItemText(hwnd,IDC_COMPARSECTLOOP,textbuf,100);
					g_command_params.SectionLoopNudgeSecs=atof(textbuf);
					GetDlgItemText(hwnd,IDC_EDITDEFAULTTRACKNAME,textbuf,100);
					g_command_params.DefaultTrackLabel.assign(textbuf);
					GetDlgItemText(hwnd,IDC_EDITTRPREFIX,textbuf,100);
					g_command_params.TrackLabelPrefix.assign(textbuf);
					GetDlgItemText(hwnd,IDC_EDITTRSUFFIX,textbuf,100);
					g_command_params.TrackLabelSuffix.assign(textbuf);

					GetDlgItemText(hwnd,IDC_THGT_A,textbuf,100);
					g_command_params.TrackHeight[0]=atoi(textbuf);
					GetDlgItemText(hwnd,IDC_THGT_B,textbuf,100);
					g_command_params.TrackHeight[1]=atoi(textbuf);

					GetDlgItemText(hwnd,IDC_EDITPIXAM,textbuf,100);
					g_command_params.PixAmount=atoi(textbuf);
					
					GetDlgItemText(hwnd,IDC_FIT_A,textbuf,100);
					g_command_params.CommandFadeInA=atof(textbuf);
					GetDlgItemText(hwnd,IDC_FOT_A,textbuf,100);
					g_command_params.CommandFadeOutA=atof(textbuf);
					GetDlgItemText(hwnd,IDC_FIS_A,textbuf,100);
					g_command_params.CommandFadeInShapeA=atoi(textbuf);
					GetDlgItemText(hwnd,IDC_FOS_A,textbuf,100);
					g_command_params.CommandFadeOutShapeA=atoi(textbuf);
					
					GetDlgItemText(hwnd,IDC_FIT_B,textbuf,100);
					g_command_params.CommandFadeInB=atof(textbuf);
					GetDlgItemText(hwnd,IDC_FOT_B,textbuf,100);
					g_command_params.CommandFadeOutB=atof(textbuf);
					GetDlgItemText(hwnd,IDC_FIS_B,textbuf,100);
					g_command_params.CommandFadeInShapeB=atoi(textbuf);
					GetDlgItemText(hwnd,IDC_FOS_B,textbuf,100);
					g_command_params.CommandFadeOutShapeB=atoi(textbuf);

					GetDlgItemText(hwnd,IDC_NUDGE_S,textbuf,100);
					g_command_params.ItemPosNudgeSecs=atof(textbuf);
					
					GetDlgItemText(hwnd,IDC_NUDGE_B,textbuf,100);
					g_command_params.ItemPosNudgeBeats=atof(textbuf);

					GetDlgItemText(hwnd,IDC_PNUDGE_A,textbuf,100);
					g_command_params.ItemPitchNudgeA=atof(textbuf);

					GetDlgItemText(hwnd,IDC_PNUDGE_B,textbuf,100);
					g_command_params.ItemPitchNudgeB=atof(textbuf);
					
					GetDlgItemText(hwnd,IDC_IVOL_NUDGE,textbuf,100);
					g_command_params.ItemVolumeNudge=atof(textbuf);

					GetDlgItemText(hwnd,IDC_RANDPROB,textbuf,100);
					g_command_params.RndItemSelProb=atof(textbuf);

					GetDlgItemText(hwnd,IDC_RANDMEAN,textbuf,100);
					g_command_params.EditCurRndMean =atof(textbuf);
					
					GetDlgItemText(hwnd,IDC_EDITCURSECS,textbuf,100);
					g_command_params.CurPosSecsAmount=atof(textbuf);

					UpdateINIfile();
					EndDialog(hwnd,0);
					return 0;
				}
				case IDCANCEL:
				{
					EndDialog(hwnd,0);
					return 0;
				}
				case IDC_EXTTOOL1BUT:
				{
					char* cFile = BrowseForFiles(__LOCALIZE("Browse for external tool","sws_mbox"), NULL, NULL, false, "Executables\0*.exe\0");
					if (cFile)
					{
						delete [] g_external_app_paths.PathToTool1;
						g_external_app_paths.PathToTool1 = new char[strlen(cFile)+1];
						strcpy(g_external_app_paths.PathToTool1, cFile);
						free(cFile);
					}
					SetDlgItemText(hwnd, IDC_EXTTOOLPATH1, g_external_app_paths.PathToTool1);
					return 0;
				}
				case IDC_EXTTOOL2BUT:
				{
					char* cFile = BrowseForFiles(__LOCALIZE("Browse for external tool","sws_mbox"), NULL, NULL, false, "Executables\0*.exe\0");
					if (cFile)
					{
						delete [] g_external_app_paths.PathToTool2;
						g_external_app_paths.PathToTool2 = new char[strlen(cFile)+1];
						strcpy(g_external_app_paths.PathToTool2, cFile);
						free(cFile);
					}
					SetDlgItemText(hwnd, IDC_EXTTOOLPATH2, g_external_app_paths.PathToTool2);
					return 0;
				}
				case IDC_EXTEDIT1BUT:
				{
					char* cFile = BrowseForFiles(__LOCALIZE("Browse for external editor","sws_mbox"), NULL, NULL, false, "Executables\0*.exe\0");
					if (cFile)
					{
						delete [] g_external_app_paths.PathToAudioEditor1;
						g_external_app_paths.PathToAudioEditor1 = new char[strlen(cFile)+1];
						strcpy(g_external_app_paths.PathToAudioEditor1, cFile);
						free(cFile);
					}
					SetDlgItemText(hwnd, IDC_EXTEDITPATH1, g_external_app_paths.PathToAudioEditor1);
					return 0;
				}
				case IDC_EXTEDIT2BUT:
				{
					char* cFile = BrowseForFiles(__LOCALIZE("Browse for external editor","sws_mbox"), NULL, NULL, false, "Executables\0*.exe\0");
					if (cFile)
					{
						delete [] g_external_app_paths.PathToAudioEditor2;
						g_external_app_paths.PathToAudioEditor2 = new char[strlen(cFile)+1];
						strcpy(g_external_app_paths.PathToAudioEditor2, cFile);
						free(cFile);
					}
					SetDlgItemText(hwnd, IDC_EXTEDITPATH2, g_external_app_paths.PathToAudioEditor2);
					return 0;
				}
			}
	}
	return 0;
}

void InitCommandParams()
{
	g_command_params.CommandFadeInA=0.005;
	g_command_params.CommandFadeOutA=0.050;
	g_command_params.CommandFadeInB=0.100;
	g_command_params.CommandFadeOutB=0.100;
	g_command_params.CommandFadeInShapeA=1;
	g_command_params.CommandFadeInShapeB=0;
	g_command_params.CommandFadeOutShapeA=1;
	g_command_params.CommandFadeOutShapeB=0;
	g_command_params.EditCurRndMean=1.0;
	g_command_params.ItemPitchNudgeA=1.0;
	g_command_params.ItemPosNudgeBeats=1.0;
	g_command_params.ItemPosNudgeSecs=1.0;
	g_command_params.ItemVolumeNudge=1.0;
	g_command_params.RndItemSelProb=50.0;
	g_command_params.ItemPitchNudgeB=0.25;
	ReadINIfile();
}

void DoShowCommandParameters(COMMAND_T*)
{
	DialogBox(g_hInst, MAKEINTRESOURCE(IDD_EXOCOM_PARDLG), g_hwndParent, ExoticParamsDlgProc);
}
