/******************************************************************************
/ padreActions.cpp
/
/ Copyright (c) 2009-2010 Tim Payne (SWS), Jeffos (S&M), P. Bourdon
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
#include "padreActions.h"
#include "padreEnvelopeProcessor.h"
#include "padreMidiItemFilters.h"
#include "padreRmeTotalmix.h"

#include <WDL/localize/localize.h>

//!WANT_LOCALIZE_1ST_STRING_BEGIN:sws_actions
static COMMAND_T g_commandTable[] = 
{
	{ { DEFACCEL, "SWS/PADRE: Envelope LFO generator" }, "PADRE_ENVLFO", EnvelopeLfo, "LFO Generator...", 0},
	{ { DEFACCEL, "SWS/PADRE: Envelope processor" }, "PADRE_ENVPROC", DoEnvelopeProcessor, "Envelope Processor...", 0},

	{ { DEFACCEL, "SWS/PADRE: Shrink selected items: -128 samples" }, "PADRE_SHRINK_128", ShrinkSelItems, NULL, 128},
	{ { DEFACCEL, "SWS/PADRE: Shrink selected items: -256 samples" }, "PADRE_SHRINK_256", ShrinkSelItems, NULL, 256},
	{ { DEFACCEL, "SWS/PADRE: Shrink selected items: -512 samples" }, "PADRE_SHRINK_512", ShrinkSelItems, NULL, 512},
	{ { DEFACCEL, "SWS/PADRE: Shrink selected items: -1024 samples" }, "PADRE_SHRINK_1024", ShrinkSelItems, NULL, 1024},
	{ { DEFACCEL, "SWS/PADRE: Shrink selected items: -2048 samples" }, "PADRE_SHRINK_2048", ShrinkSelItems, NULL, 2048},

	{ {}, LAST_COMMAND, }, // Denote end of table
};
//!WANT_LOCALIZE_1ST_STRING_END



	//{ { DEFACCEL, "SWS/PADRE: Randomize MIDI Note Positions" }, "PADRE_RANDMIDINOTEPOS", RandomizeMidiNotePos, NULL, },

	////! \todo Add as separate .dll ? (hardware-specific actions)
	//{ { DEFACCEL, "SWS/PADRE: RME Totalmix Load User Preset 1" }, "PADRE_TOTALMIX_LOADUSER1", RmeTotalmixProc, NULL, eTOTALMIX_LOADUSER1},
	//{ { DEFACCEL, "SWS/PADRE: RME Totalmix Load User Preset 2" }, "PADRE_TOTALMIX_LOADUSER2", RmeTotalmixProc, NULL, eTOTALMIX_LOADUSER2},
	//{ { DEFACCEL, "SWS/PADRE: RME Totalmix Load User Preset 3" }, "PADRE_TOTALMIX_LOADUSER3", RmeTotalmixProc, NULL, eTOTALMIX_LOADUSER3},
	//{ { DEFACCEL, "SWS/PADRE: RME Totalmix Load User Preset 4" }, "PADRE_TOTALMIX_LOADUSER4", RmeTotalmixProc, NULL, eTOTALMIX_LOADUSER4},
	//{ { DEFACCEL, "SWS/PADRE: RME Totalmix Load User Preset 5" }, "PADRE_TOTALMIX_LOADUSER5", RmeTotalmixProc, NULL, eTOTALMIX_LOADUSER5},
	//{ { DEFACCEL, "SWS/PADRE: RME Totalmix Load User Preset 6" }, "PADRE_TOTALMIX_LOADUSER6", RmeTotalmixProc, NULL, eTOTALMIX_LOADUSER6},
	//{ { DEFACCEL, "SWS/PADRE: RME Totalmix Load User Preset 7" }, "PADRE_TOTALMIX_LOADUSER7", RmeTotalmixProc, NULL, eTOTALMIX_LOADUSER7},
	//{ { DEFACCEL, "SWS/PADRE: RME Totalmix Load User Preset 8" }, "PADRE_TOTALMIX_LOADUSER8", RmeTotalmixProc, NULL, eTOTALMIX_LOADUSER8},

	//{ { DEFACCEL, "SWS/PADRE: RME Totalmix Toggle Master Mute" }, "PADRE_TOTALMIX_MASTERMUTE", RmeTotalmixProc, NULL, eTOTALMIX_MASTERMUTE},

	////! \bug These won't work (VK_CONTROL problem)
	//{ { DEFACCEL, "SWS/PADRE: RME Totalmix Load Factory Preset 1" }, "PADRE_TOTALMIX_LOADFACT1", RmeTotalmixProc, NULL, eTOTALMIX_LOADFACT1},
	//{ { DEFACCEL, "SWS/PADRE: RME Totalmix Load Factory Preset 2" }, "PADRE_TOTALMIX_LOADFACT2", RmeTotalmixProc, NULL, eTOTALMIX_LOADFACT2},
	//{ { DEFACCEL, "SWS/PADRE: RME Totalmix Load Factory Preset 3" }, "PADRE_TOTALMIX_LOADFACT3", RmeTotalmixProc, NULL, eTOTALMIX_LOADFACT3},
	//{ { DEFACCEL, "SWS/PADRE: RME Totalmix Load Factory Preset 4" }, "PADRE_TOTALMIX_LOADFACT4", RmeTotalmixProc, NULL, eTOTALMIX_LOADFACT4},
	//{ { DEFACCEL, "SWS/PADRE: RME Totalmix Load Factory Preset 5" }, "PADRE_TOTALMIX_LOADFACT5", RmeTotalmixProc, NULL, eTOTALMIX_LOADFACT5},
	//{ { DEFACCEL, "SWS/PADRE: RME Totalmix Load Factory Preset 6" }, "PADRE_TOTALMIX_LOADFACT6", RmeTotalmixProc, NULL, eTOTALMIX_LOADFACT6},
	//{ { DEFACCEL, "SWS/PADRE: RME Totalmix Load Factory Preset 7" }, "PADRE_TOTALMIX_LOADFACT7", RmeTotalmixProc, NULL, eTOTALMIX_LOADFACT7},
	//{ { DEFACCEL, "SWS/PADRE: RME Totalmix Load Factory Preset 8" }, "PADRE_TOTALMIX_LOADFACT8", RmeTotalmixProc, NULL, eTOTALMIX_LOADFACT8},




static MidiItemProcessor* midiNoteRandomizer = NULL;

int PadreInit()
{
	midiNoteRandomizer = new MidiItemProcessor("MIDI Note Position Randomize");
	midiNoteRandomizer->addFilter(new MidiFilterRandomNotePos());
	//midiNoteRandomizer->addFilter(new MidiFilterShortenEndEvents());

	return SWSRegisterCommands(g_commandTable);
}

void PadreExit()
{
	delete midiNoteRandomizer;
}

WDL_DLGRET EnvelopeLfoDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	const char cWndPosKey[] = "LFO Window Pos";

	if (INT_PTR r = SNM_HookThemeColorsMessage(hwnd, uMsg, wParam, lParam))
		return r;

	switch(uMsg)
	{
		case WM_INITDIALOG :
		{
			WDL_UTF8_HookComboBox(GetDlgItem(hwnd, IDC_PADRELFO_TARGET));
			WDL_UTF8_HookComboBox(GetDlgItem(hwnd, IDC_PADRELFO_TIMESEGMENT));
			WDL_UTF8_HookComboBox(GetDlgItem(hwnd, IDC_PADRELFO_LFOSHAPE));
			WDL_UTF8_HookComboBox(GetDlgItem(hwnd, IDC_PADRELFO_SYNCFREQUENCY));
			WDL_UTF8_HookComboBox(GetDlgItem(hwnd, IDC_PADRELFO_SYNCDELAY));
			WDL_UTF8_HookComboBox(GetDlgItem(hwnd, IDC_PADRELFO_TAKEENV));
			WDL_UTF8_HookComboBox(GetDlgItem(hwnd, IDC_PADRELFO_MIDICC));

			for(int i=eENVTYPE_TRACK; i<=eENVTYPE_MIDICC; i++)
			{
				int x = (int)SendDlgItemMessage(hwnd,IDC_PADRELFO_TARGET,CB_ADDSTRING,0,(LPARAM)GetEnvTypeStr((EnvType)i));
				SendDlgItemMessage(hwnd,IDC_PADRELFO_TARGET,CB_SETITEMDATA,x,i);
				if(i == EnvelopeProcessor::getInstance()->_parameters.envType)
					SendDlgItemMessage(hwnd,IDC_PADRELFO_TARGET,CB_SETCURSEL,x,0);
			}
			SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_PADRELFO_TARGET, CBN_SELCHANGE), 0);

			if(EnvelopeProcessor::getInstance()->_parameters.activeTakeOnly)
				CheckDlgButton(hwnd, IDC_PADRELFO_ACTIVETAKES, TRUE);
			else
				CheckDlgButton(hwnd, IDC_PADRELFO_ACTIVETAKES, FALSE);

			int iLastShape = eWAVSHAPE_SAWDOWN_BEZIER;
			if(EnvelopeProcessor::getInstance()->_parameters.envType == eENVTYPE_MIDICC)
				iLastShape = eWAVSHAPE_SAWDOWN;

			for(int i=eWAVSHAPE_SINE; i<=iLastShape; i++)
			{
				int x = (int)SendDlgItemMessage(hwnd,IDC_PADRELFO_LFOSHAPE,CB_ADDSTRING,0,(LPARAM)GetWaveShapeStr((WaveShape)i));
				SendDlgItemMessage(hwnd,IDC_PADRELFO_LFOSHAPE,CB_SETITEMDATA,x,i);
				if(i == EnvelopeProcessor::getInstance()->_parameters.waveParams.shape)
					SendDlgItemMessage(hwnd,IDC_PADRELFO_LFOSHAPE,CB_SETCURSEL,x,0);
			}

			for(int i=eGRID_OFF; i<eGRID_LAST; i++)
			{
				int x = (int)SendDlgItemMessage(hwnd,IDC_PADRELFO_SYNCFREQUENCY,CB_ADDSTRING,0,(LPARAM)GetGridDivisionStr((GridDivision)i));
				SendDlgItemMessage(hwnd,IDC_PADRELFO_SYNCFREQUENCY,CB_SETITEMDATA,x,i);
				if(i == EnvelopeProcessor::getInstance()->_parameters.waveParams.freqBeat)
					SendDlgItemMessage(hwnd,IDC_PADRELFO_SYNCFREQUENCY,CB_SETCURSEL,x,0);
			}
			SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_PADRELFO_SYNCFREQUENCY, CBN_SELCHANGE), 0);

			for(int i=eGRID_OFF; i<eGRID_LAST; i++)
			{
				if( (i>eGRID_OFF) && (i<=eGRID_1_1) )
					continue;
				int x = (int)SendDlgItemMessage(hwnd,IDC_PADRELFO_SYNCDELAY,CB_ADDSTRING,0,(LPARAM)GetGridDivisionStr((GridDivision)i));
				SendDlgItemMessage(hwnd,IDC_PADRELFO_SYNCDELAY,CB_SETITEMDATA,x,i);
				if(i == EnvelopeProcessor::getInstance()->_parameters.waveParams.delayBeat)
					SendDlgItemMessage(hwnd,IDC_PADRELFO_SYNCDELAY,CB_SETCURSEL,x,0);
			}
			SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_PADRELFO_SYNCDELAY, CBN_SELCHANGE), 0);

			char buffer[BUFFER_SIZE];

			sprintf(buffer, "%.3lf", EnvelopeProcessor::getInstance()->_parameters.waveParams.freqHz);
			SetDlgItemText(hwnd, IDC_PADRELFO_FREQUENCY, buffer);
			sprintf(buffer, "%.3lf", EnvelopeProcessor::getInstance()->_parameters.waveParams.delayMsec);
			SetDlgItemText(hwnd, IDC_PADRELFO_DELAY, buffer);
			sprintf(buffer, "%.0lf", 100.0*EnvelopeProcessor::getInstance()->_parameters.waveParams.strength);
			SetDlgItemText(hwnd, IDC_PADRELFO_STRENGTH, buffer);
			sprintf(buffer, "%.0lf", 100.0*EnvelopeProcessor::getInstance()->_parameters.waveParams.offset);
			SetDlgItemText(hwnd, IDC_PADRELFO_OFFSET, buffer);

			for(int i=eTAKEENV_VOLUME; i<=eTAKEENV_PITCH; i++)
			{
				int x = (int)SendDlgItemMessage(hwnd,IDC_PADRELFO_TAKEENV,CB_ADDSTRING,0,(LPARAM)GetTakeEnvelopeStr((TakeEnvType)i));
				SendDlgItemMessage(hwnd,IDC_PADRELFO_TAKEENV,CB_SETITEMDATA,x,i);
				if(i == EnvelopeProcessor::getInstance()->_parameters.takeEnvType)
					SendDlgItemMessage(hwnd,IDC_PADRELFO_TAKEENV,CB_SETCURSEL,x,0);
			}

			for(int i=0; i<128; i++)
			{
				sprintf(buffer, "%3d", i);
				int x = (int)SendDlgItemMessage(hwnd,IDC_PADRELFO_MIDICC,CB_ADDSTRING,0,(LPARAM)buffer);
				SendDlgItemMessage(hwnd,IDC_PADRELFO_MIDICC,CB_SETITEMDATA,x,i);
				if(i == EnvelopeProcessor::getInstance()->_parameters.midiCc)
					SendDlgItemMessage(hwnd,IDC_PADRELFO_MIDICC,CB_SETCURSEL,x,0);
			}

			RestoreWindowPos(hwnd, cWndPosKey, false);
			SetFocus(GetDlgItem(hwnd, IDC_PADRELFO_TARGET));

			return 0;
		}

		case WM_COMMAND :
            switch(LOWORD(wParam))
            {
                case IDOK:
				{
					int combo = (int)SendDlgItemMessage(hwnd,IDC_PADRELFO_TARGET,CB_GETCURSEL,0,0);
					if(combo != CB_ERR)
						EnvelopeProcessor::getInstance()->_parameters.envType = (EnvType)(SendDlgItemMessage(hwnd,IDC_PADRELFO_TARGET,CB_GETITEMDATA,combo,0));

					EnvelopeProcessor::getInstance()->_parameters.activeTakeOnly = (IsDlgButtonChecked(hwnd, IDC_PADRELFO_ACTIVETAKES) != 0);

					combo = (int)SendDlgItemMessage(hwnd,IDC_PADRELFO_TIMESEGMENT,CB_GETCURSEL,0,0);
					if(combo != CB_ERR)
						EnvelopeProcessor::getInstance()->_parameters.timeSegment = (TimeSegment)(SendDlgItemMessage(hwnd,IDC_PADRELFO_TIMESEGMENT,CB_GETITEMDATA,combo,0));

					combo = (int)SendDlgItemMessage(hwnd,IDC_PADRELFO_LFOSHAPE,CB_GETCURSEL,0,0);
					if(combo != CB_ERR)
						EnvelopeProcessor::getInstance()->_parameters.waveParams.shape = (WaveShape)(SendDlgItemMessage(hwnd,IDC_PADRELFO_LFOSHAPE,CB_GETITEMDATA,combo,0));

					combo = (int)SendDlgItemMessage(hwnd,IDC_PADRELFO_SYNCFREQUENCY,CB_GETCURSEL,0,0);
					if(combo != CB_ERR)
						EnvelopeProcessor::getInstance()->_parameters.waveParams.freqBeat = (GridDivision)(SendDlgItemMessage(hwnd,IDC_PADRELFO_SYNCFREQUENCY,CB_GETITEMDATA,combo,0));

					combo = (int)SendDlgItemMessage(hwnd,IDC_PADRELFO_SYNCDELAY,CB_GETCURSEL,0,0);
					if(combo != CB_ERR)
						EnvelopeProcessor::getInstance()->_parameters.waveParams.delayBeat = (GridDivision)(SendDlgItemMessage(hwnd,IDC_PADRELFO_SYNCDELAY,CB_GETITEMDATA,combo,0));

					char buffer[BUFFER_SIZE];
					GetDlgItemText(hwnd,IDC_PADRELFO_FREQUENCY,buffer,BUFFER_SIZE);
					EnvelopeProcessor::getInstance()->_parameters.waveParams.freqHz = atof(buffer);
					GetDlgItemText(hwnd,IDC_PADRELFO_DELAY,buffer,BUFFER_SIZE);
					EnvelopeProcessor::getInstance()->_parameters.waveParams.delayMsec = atof(buffer);
					GetDlgItemText(hwnd,IDC_PADRELFO_STRENGTH,buffer,BUFFER_SIZE);
					EnvelopeProcessor::getInstance()->_parameters.waveParams.strength = atof(buffer)/100.0;
					GetDlgItemText(hwnd,IDC_PADRELFO_OFFSET,buffer,BUFFER_SIZE);
					EnvelopeProcessor::getInstance()->_parameters.waveParams.offset = atof(buffer)/100.0;

					combo = (int)SendDlgItemMessage(hwnd,IDC_PADRELFO_TAKEENV,CB_GETCURSEL,0,0);
					if(combo != CB_ERR)
						EnvelopeProcessor::getInstance()->_parameters.takeEnvType = (TakeEnvType)(SendDlgItemMessage(hwnd,IDC_PADRELFO_TAKEENV,CB_GETITEMDATA,combo,0));

					combo = (int)SendDlgItemMessage(hwnd,IDC_PADRELFO_MIDICC,CB_GETCURSEL,0,0);
					if(combo != CB_ERR)
						EnvelopeProcessor::getInstance()->_parameters.midiCc = (int)SendDlgItemMessage(hwnd,IDC_PADRELFO_MIDICC,CB_GETITEMDATA,combo,0);

					EnvelopeProcessor::ErrorCode res = EnvelopeProcessor::eERRORCODE_UNKNOWN;
					switch(EnvelopeProcessor::getInstance()->_parameters.envType)
					{
						case eENVTYPE_TRACK :
							res = EnvelopeProcessor::getInstance()->generateSelectedTrackEnvLfo();
						break;

						case eENVTYPE_TAKE :
							res = EnvelopeProcessor::getInstance()->generateSelectedTakesLfo();
						break;

						case eENVTYPE_MIDICC :
							res = EnvelopeProcessor::getInstance()->generateSelectedMidiTakeLfo();
						break;

						default:
						break;
					}

					EnvelopeProcessor::errorHandlerDlg(hwnd, res);

					//EndDialog(hwnd,0);
					return 0;
				}
				break;

				case IDCANCEL:
				{
					ShowWindow(hwnd, SW_HIDE);
					return 0;
				}
				break;

				case IDC_PADRELFO_TARGET:
				{
					if(HIWORD(wParam) == CBN_SELCHANGE)
					{
						int combo = (int)SendDlgItemMessage(hwnd,IDC_PADRELFO_TARGET,CB_GETCURSEL,0,0);
						if(combo != CB_ERR)
						{
							EnvType envType = (EnvType)(SendDlgItemMessage(hwnd,IDC_PADRELFO_TARGET,CB_GETITEMDATA,combo,0));
							switch(envType)
							{
								case eENVTYPE_TRACK:
									EnableWindow(GetDlgItem(hwnd,IDC_PADRELFO_ACTIVETAKES), FALSE);
									EnableWindow(GetDlgItem(hwnd,IDC_PADRELFO_TAKEENV), FALSE);
									EnableWindow(GetDlgItem(hwnd,IDC_PADRELFO_MIDICC), FALSE);
								break;
								case eENVTYPE_TAKE:
									EnableWindow(GetDlgItem(hwnd,IDC_PADRELFO_ACTIVETAKES), TRUE);
									EnableWindow(GetDlgItem(hwnd,IDC_PADRELFO_TAKEENV), TRUE);
									EnableWindow(GetDlgItem(hwnd,IDC_PADRELFO_MIDICC), FALSE);
								break;
								case eENVTYPE_MIDICC:
									EnableWindow(GetDlgItem(hwnd,IDC_PADRELFO_ACTIVETAKES), TRUE);
									EnableWindow(GetDlgItem(hwnd,IDC_PADRELFO_TAKEENV), FALSE);
									EnableWindow(GetDlgItem(hwnd,IDC_PADRELFO_MIDICC), TRUE);
								break;
								default:
								break;
							}

							SendDlgItemMessage(hwnd,IDC_PADRELFO_TIMESEGMENT,CB_RESETCONTENT,0,0);
							for(int i=eTIMESEGMENT_TIMESEL; i<eTIMESEGMENT_LAST; i++)
							{
								if( (envType == eENVTYPE_TRACK) && (i == eTIMESEGMENT_SELITEM) )
									continue;
								if( (envType == eENVTYPE_TAKE) && (i == eTIMESEGMENT_PROJECT) )
									continue;
								int x = (int)SendDlgItemMessage(hwnd,IDC_PADRELFO_TIMESEGMENT,CB_ADDSTRING,0,(LPARAM)GetTimeSegmentStr((TimeSegment)i));
									SendDlgItemMessage(hwnd,IDC_PADRELFO_TIMESEGMENT,CB_SETITEMDATA,x,i);
								if(i == EnvelopeProcessor::getInstance()->_parameters.timeSegment)
									SendDlgItemMessage(hwnd,IDC_PADRELFO_TIMESEGMENT,CB_SETCURSEL,x,0);
							}
						}
					}
					return 0;
				}
				break;

				case IDC_PADRELFO_SYNCFREQUENCY:
				{
					if(HIWORD(wParam) == CBN_SELCHANGE)
					{
						int combo = (int)SendDlgItemMessage(hwnd,IDC_PADRELFO_SYNCFREQUENCY,CB_GETCURSEL,0,0);
						if(combo != CB_ERR)
						{
							GridDivision freqBeat  = (GridDivision)(SendDlgItemMessage(hwnd,IDC_PADRELFO_SYNCFREQUENCY,CB_GETITEMDATA,combo,0));
							if(freqBeat == eGRID_OFF)
								EnableWindow(GetDlgItem(hwnd,IDC_PADRELFO_FREQUENCY), TRUE);
							else
								EnableWindow(GetDlgItem(hwnd,IDC_PADRELFO_FREQUENCY), FALSE);
						}
					}
					return 0;
				}
				break;

				case IDC_PADRELFO_SYNCDELAY:
				{
					if(HIWORD(wParam) == CBN_SELCHANGE)
					{
						int combo = (int)SendDlgItemMessage(hwnd,IDC_PADRELFO_SYNCDELAY,CB_GETCURSEL,0,0);
						if(combo != CB_ERR)
						{
							GridDivision delayBeat  = (GridDivision)(SendDlgItemMessage(hwnd,IDC_PADRELFO_SYNCDELAY,CB_GETITEMDATA,combo,0));
							if(delayBeat == eGRID_OFF)
								EnableWindow(GetDlgItem(hwnd,IDC_PADRELFO_DELAY), TRUE);
							else
								EnableWindow(GetDlgItem(hwnd,IDC_PADRELFO_DELAY), FALSE);
						}
					}
					return 0;
				}
				break;
			}
		case WM_DESTROY:
			SaveWindowPos(hwnd, cWndPosKey);
			break;
	}

	return 0;
}

void EnvelopeLfo(COMMAND_T* _ct)
{
	static HWND hwnd = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_PADRELFO_GENERATOR), g_hwndParent, EnvelopeLfoDlgProc);
	ShowWindow(hwnd, SW_SHOW);
}

void ShrinkSelectedTakes(int nbSamples, bool bActiveOnly)
{
	list<MediaItem*> items;
	MidiItemProcessor::getSelectedMediaItems(items);
	for(list<MediaItem*>::iterator item = items.begin(); item != items.end(); item++)
	{
		double itemLength = *(double*)GetSetMediaItemInfo(*item, "D_LENGTH", NULL);
		int itemLengthSamples = (int)(MIDIITEMPROC_DEFAULT_SAMPLERATE * itemLength);
		itemLengthSamples -= nbSamples;
		itemLength = (double)(itemLengthSamples)/MIDIITEMPROC_DEFAULT_SAMPLERATE;
		GetSetMediaItemInfo(*item, "D_LENGTH", &itemLength);

		UpdateItemInProject(*item);
	}
	Undo_OnStateChangeEx(__LOCALIZE("Item Processor: shrink","sws_undo"), UNDO_STATE_ITEMS, -1);
	UpdateTimeline();
}

void ShrinkSelItems(COMMAND_T* _ct)
{
	int nbSamples = (int)_ct->user;
	ShrinkSelectedTakes(nbSamples);
}

void RandomizeMidiNotePos(COMMAND_T* _ct)
{
	midiNoteRandomizer->processSelectedMidiTakes(true);
}

//void EnvelopeFader(COMMAND_T* _ct)
//{
//	EnvelopeProcessor::getInstance()->generateSelectedTrackFade();
//}

void DoEnvelopeProcessor(COMMAND_T* _ct)
{
	static HWND hwnd = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_PADRE_ENVPROCESSOR), g_hwndParent, EnvelopeProcessorDlgProc);
	ShowWindow(hwnd, SW_SHOW);
}

WDL_DLGRET EnvelopeProcessorDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	const char cWndPosKey[] = "EnvProc Window Pos";

	if (INT_PTR r = SNM_HookThemeColorsMessage(hwnd, uMsg, wParam, lParam))
		return r;

	switch(uMsg)
	{
		case WM_INITDIALOG :
		{
			WDL_UTF8_HookComboBox(GetDlgItem(hwnd, IDC_PADREENVPROC_TYPE));
			WDL_UTF8_HookComboBox(GetDlgItem(hwnd, IDC_PADREENVPROC_TARGET));
			WDL_UTF8_HookComboBox(GetDlgItem(hwnd, IDC_PADREENVPROC_TIMESEGMENT));
			WDL_UTF8_HookComboBox(GetDlgItem(hwnd, IDC_PADREENVPROC_TAKEENV));

			for(int i=eENVTYPE_TRACK; i<eENVTYPE_MIDICC; i++)
			{
				int x = (int)SendDlgItemMessage(hwnd,IDC_PADREENVPROC_TARGET,CB_ADDSTRING,0,(LPARAM)GetEnvTypeStr((EnvType)i));
				SendDlgItemMessage(hwnd,IDC_PADREENVPROC_TARGET,CB_SETITEMDATA,x,i);
				if(i == EnvelopeProcessor::getInstance()->_envModParams.envType)
					SendDlgItemMessage(hwnd,IDC_PADREENVPROC_TARGET,CB_SETCURSEL,x,0);
			}
			SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_PADREENVPROC_TARGET, CBN_SELCHANGE), 0);

			if(EnvelopeProcessor::getInstance()->_envModParams.activeTakeOnly)
				CheckDlgButton(hwnd, IDC_PADREENVPROC_ACTIVETAKES, TRUE);
			else
				CheckDlgButton(hwnd, IDC_PADREENVPROC_ACTIVETAKES, FALSE);

			for(int i=eENVMOD_FADEIN; i<eENVMOD_LAST; i++)
			{
				int x = (int)SendDlgItemMessage(hwnd,IDC_PADREENVPROC_TYPE,CB_ADDSTRING,0,(LPARAM)GetEnvModTypeStr((EnvModType)i));
				SendDlgItemMessage(hwnd,IDC_PADREENVPROC_TYPE,CB_SETITEMDATA,x,i);
				if(i == EnvelopeProcessor::getInstance()->_envModParams.type)
					SendDlgItemMessage(hwnd,IDC_PADREENVPROC_TYPE,CB_SETCURSEL,x,0);
			}

			char buffer[BUFFER_SIZE];
			sprintf(buffer, "%.0lf", 100.0*EnvelopeProcessor::getInstance()->_envModParams.strength);
			SetDlgItemText(hwnd, IDC_PADREENVPROC_STRENGTH, buffer);
			sprintf(buffer, "%.0lf", 100.0*EnvelopeProcessor::getInstance()->_envModParams.offset);
			SetDlgItemText(hwnd, IDC_PADREENVPROC_OFFSET, buffer);

			for(int i=eTAKEENV_VOLUME; i<=eTAKEENV_PITCH; i++)
			{
				int x = (int)SendDlgItemMessage(hwnd,IDC_PADREENVPROC_TAKEENV,CB_ADDSTRING,0,(LPARAM)GetTakeEnvelopeStr((TakeEnvType)i));
				SendDlgItemMessage(hwnd,IDC_PADREENVPROC_TAKEENV,CB_SETITEMDATA,x,i);
				if(i == EnvelopeProcessor::getInstance()->_envModParams.takeEnvType)
					SendDlgItemMessage(hwnd,IDC_PADREENVPROC_TAKEENV,CB_SETCURSEL,x,0);
			}

			RestoreWindowPos(hwnd, cWndPosKey, false);
			SetFocus(GetDlgItem(hwnd, IDC_PADREENVPROC_TYPE));

			return 0;
		}
		break;

		case WM_COMMAND :
		{
            switch(LOWORD(wParam))
            {
                case IDOK:
				{
					int combo = (int)SendDlgItemMessage(hwnd,IDC_PADREENVPROC_TARGET,CB_GETCURSEL,0,0);
					if(combo != CB_ERR)
						EnvelopeProcessor::getInstance()->_envModParams.envType = (EnvType)(SendDlgItemMessage(hwnd,IDC_PADREENVPROC_TARGET,CB_GETITEMDATA,combo,0));

					EnvelopeProcessor::getInstance()->_envModParams.activeTakeOnly = (IsDlgButtonChecked(hwnd, IDC_PADREENVPROC_ACTIVETAKES) != 0);

					combo = (int)SendDlgItemMessage(hwnd,IDC_PADREENVPROC_TIMESEGMENT,CB_GETCURSEL,0,0);
					if(combo != CB_ERR)
						EnvelopeProcessor::getInstance()->_envModParams.timeSegment = (TimeSegment)(SendDlgItemMessage(hwnd,IDC_PADREENVPROC_TIMESEGMENT,CB_GETITEMDATA,combo,0));

					combo = (int)SendDlgItemMessage(hwnd,IDC_PADREENVPROC_TYPE,CB_GETCURSEL,0,0);
					if(combo != CB_ERR)
						EnvelopeProcessor::getInstance()->_envModParams.type = (EnvModType)(SendDlgItemMessage(hwnd,IDC_PADREENVPROC_TYPE,CB_GETITEMDATA,combo,0));

					char buffer[BUFFER_SIZE];
					GetDlgItemText(hwnd,IDC_PADREENVPROC_STRENGTH,buffer,BUFFER_SIZE);
					EnvelopeProcessor::getInstance()->_envModParams.strength = atof(buffer)/100.0;
					GetDlgItemText(hwnd,IDC_PADREENVPROC_OFFSET,buffer,BUFFER_SIZE);
					EnvelopeProcessor::getInstance()->_envModParams.offset = atof(buffer)/100.0;

					combo =(int) SendDlgItemMessage(hwnd,IDC_PADREENVPROC_TAKEENV,CB_GETCURSEL,0,0);
					if(combo != CB_ERR)
						EnvelopeProcessor::getInstance()->_envModParams.takeEnvType = (TakeEnvType)(SendDlgItemMessage(hwnd,IDC_PADREENVPROC_TAKEENV,CB_GETITEMDATA,combo,0));

					EnvelopeProcessor::ErrorCode res = EnvelopeProcessor::eERRORCODE_UNKNOWN;
					switch(EnvelopeProcessor::getInstance()->_envModParams.envType)
					{
						case eENVTYPE_TRACK :
							res = EnvelopeProcessor::getInstance()->processSelectedTrackEnv();
						break;

						case eENVTYPE_TAKE :
							res = EnvelopeProcessor::getInstance()->processSelectedTakes();
						break;

						default:
						break;
					}

					EnvelopeProcessor::errorHandlerDlg(hwnd, res);

					return 0;
				}
				break;

				case IDCANCEL:
				{
					ShowWindow(hwnd, SW_HIDE);
					return 0;
				}
				break;

				case IDC_PADREENVPROC_TARGET:
				{
					if(HIWORD(wParam) == CBN_SELCHANGE)
					{
						int combo = (int)SendDlgItemMessage(hwnd,IDC_PADREENVPROC_TARGET,CB_GETCURSEL,0,0);
						if(combo != CB_ERR)
						{
							EnvType envType = (EnvType)(SendDlgItemMessage(hwnd,IDC_PADREENVPROC_TARGET,CB_GETITEMDATA,combo,0));
							switch(envType)
							{
								case eENVTYPE_TRACK:
									EnableWindow(GetDlgItem(hwnd,IDC_PADREENVPROC_ACTIVETAKES), FALSE);
									EnableWindow(GetDlgItem(hwnd,IDC_PADREENVPROC_TAKEENV), FALSE);
								break;
								case eENVTYPE_TAKE:
									EnableWindow(GetDlgItem(hwnd,IDC_PADREENVPROC_ACTIVETAKES), TRUE);
									EnableWindow(GetDlgItem(hwnd,IDC_PADREENVPROC_TAKEENV), TRUE);
								break;
								default:
								break;
							}

							SendDlgItemMessage(hwnd,IDC_PADREENVPROC_TIMESEGMENT,CB_RESETCONTENT,0,0);
							for(int i=eTIMESEGMENT_TIMESEL; i<eTIMESEGMENT_LAST; i++)
							{
								if( (envType == eENVTYPE_TRACK) && (i == eTIMESEGMENT_SELITEM) )
									continue;
								if( (envType == eENVTYPE_TAKE) && (i == eTIMESEGMENT_PROJECT) )
									continue;
								int x = (int)SendDlgItemMessage(hwnd,IDC_PADREENVPROC_TIMESEGMENT,CB_ADDSTRING,0,(LPARAM)GetTimeSegmentStr((TimeSegment)i));
								SendDlgItemMessage(hwnd,IDC_PADREENVPROC_TIMESEGMENT,CB_SETITEMDATA,x,i);
								if(i == EnvelopeProcessor::getInstance()->_envModParams.timeSegment)
									SendDlgItemMessage(hwnd,IDC_PADREENVPROC_TIMESEGMENT,CB_SETCURSEL,x,0);
							}
						}
					}
					return 0;
				}
				break;
			}
		}
		case WM_DESTROY:
			SaveWindowPos(hwnd, cWndPosKey);
			break;
	}

	return 0;
}

#ifdef _WIN32
void RmeProc(COMMAND_T* _ct)
{
	RmeTotalmix((RmeTotalmixCmd)_ct->user);
}
#endif
