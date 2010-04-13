/******************************************************************************
/ padreActions.cpp
/
/ Copyright (c) 2009-2010 Tim Payne (SWS), JF BÈdague, P. Bourdon
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
#include "padreActions.h"

static COMMAND_T g_commandTable[] = 
{
	{ { DEFACCEL, "SWS/PADRE: LFO Generator: Selected Track Envelope" }, "PADRE_TRACKENVLFO", EnvelopeLfo, NULL, 0},
	{ { DEFACCEL, "SWS/PADRE: LFO Generator: Selected Active Take(s)" }, "PADRE_TAKEENVLFO", EnvelopeLfo, NULL, 1},
	{ { DEFACCEL, "SWS/PADRE: LFO Generator: Selected Active Take(s) (MIDI)" }, "PADRE_MIDILFO", EnvelopeLfo, NULL, 2},

	{ { DEFACCEL, "SWS/PADRE: Shrink Selected Items: -128 samples" }, "PADRE_SHRINK_128", ShrinkSelItems, NULL, 128},
	{ { DEFACCEL, "SWS/PADRE: Shrink Selected Items: -256 samples" }, "PADRE_SHRINK_256", ShrinkSelItems, NULL, 256},
	{ { DEFACCEL, "SWS/PADRE: Shrink Selected Items: -512 samples" }, "PADRE_SHRINK_512", ShrinkSelItems, NULL, 512},
	{ { DEFACCEL, "SWS/PADRE: Shrink Selected Items: -1024 samples" }, "PADRE_SHRINK_1024", ShrinkSelItems, NULL, 1024},
	{ { DEFACCEL, "SWS/PADRE: Shrink Selected Items: -2048 samples" }, "PADRE_SHRINK_2048", ShrinkSelItems, NULL, 2048},

	{ { DEFACCEL, "SWS/PADRE: Randomize MIDI Note Positions" }, "PADRE_RANDMIDINOTEPOS", RandomizeMidiNotePos, NULL, },

	{ {}, LAST_COMMAND, }, // Denote end of table
};

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

WDL_DLGRET EnvelopeLfoDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch(Message)
	{
        case WM_INITDIALOG :
		{
			const char* args = (const char*)lParam;
			EnvelopeProcessor::getInstance()->_parameters.envType = (EnvType)atoi(args);

			for(int i=eTIMESEGMENT_TIMESEL; i<eTIMESEGMENT_LAST; i++)
			{
				if( (EnvelopeProcessor::getInstance()->_parameters.envType == eENVTYPE_TRACK) && (i == eTIMESEGMENT_SELITEM) )
					continue;
				if( (EnvelopeProcessor::getInstance()->_parameters.envType == eENVTYPE_TAKE) && (i == eTIMESEGMENT_PROJECT) )
					continue;
				int x = SendDlgItemMessage(hwnd,IDC_PADRELFO_TIMESEGMENT,CB_ADDSTRING,0,(LPARAM)GetTimeSegmentStr((TimeSegment)i));
					SendDlgItemMessage(hwnd,IDC_PADRELFO_TIMESEGMENT,CB_SETITEMDATA,x,i);
				if(i == EnvelopeProcessor::getInstance()->_parameters.timeSegment)
					SendDlgItemMessage(hwnd,IDC_PADRELFO_TIMESEGMENT,CB_SETCURSEL,x,0);
			}

			int iLastShape = eWAVSHAPE_SAWDOWN_BEZIER;
			if(EnvelopeProcessor::getInstance()->_parameters.envType == eENVTYPE_MIDICC)
				iLastShape = eWAVSHAPE_SAWDOWN;

			for(int i=eWAVSHAPE_SINE; i<=iLastShape; i++)
			{
				int x = SendDlgItemMessage(hwnd,IDC_PADRELFO_LFOSHAPE,CB_ADDSTRING,0,(LPARAM)GetWaveShapeStr((WaveShape)i));
				SendDlgItemMessage(hwnd,IDC_PADRELFO_LFOSHAPE,CB_SETITEMDATA,x,i);
				if(i == EnvelopeProcessor::getInstance()->_parameters.waveShape)
					SendDlgItemMessage(hwnd,IDC_PADRELFO_LFOSHAPE,CB_SETCURSEL,x,0);
			}

			for(int i=eGRID_OFF; i<eGRID_LAST; i++)
			{
				int x = SendDlgItemMessage(hwnd,IDC_PADRELFO_SYNCFREQUENCY,CB_ADDSTRING,0,(LPARAM)GetGridDivisionStr((GridDivision)i));
				SendDlgItemMessage(hwnd,IDC_PADRELFO_SYNCFREQUENCY,CB_SETITEMDATA,x,i);
				if(i == EnvelopeProcessor::getInstance()->_parameters.freqBeat)
					SendDlgItemMessage(hwnd,IDC_PADRELFO_SYNCFREQUENCY,CB_SETCURSEL,x,0);
			}

			for(int i=eGRID_OFF; i<eGRID_LAST; i++)
			{
				int x = SendDlgItemMessage(hwnd,IDC_PADRELFO_SYNCDELAY,CB_ADDSTRING,0,(LPARAM)GetGridDivisionStr((GridDivision)i));
				SendDlgItemMessage(hwnd,IDC_PADRELFO_SYNCDELAY,CB_SETITEMDATA,x,i);
				if(i == EnvelopeProcessor::getInstance()->_parameters.delayBeat)
					SendDlgItemMessage(hwnd,IDC_PADRELFO_SYNCDELAY,CB_SETCURSEL,x,0);
			}

			char buffer[BUFFER_SIZE];
			sprintf(buffer, "%lf", EnvelopeProcessor::getInstance()->_parameters.freqHz);
			SetDlgItemText(hwnd, IDC_PADRELFO_FREQUENCY, buffer);
			sprintf(buffer, "%lf", EnvelopeProcessor::getInstance()->_parameters.delayMsec);
			SetDlgItemText(hwnd, IDC_PADRELFO_DELAY, buffer);
			sprintf(buffer, "%.0lf", 100.0*EnvelopeProcessor::getInstance()->_parameters.strength);
			SetDlgItemText(hwnd, IDC_PADRELFO_STRENGTH, buffer);
			sprintf(buffer, "%.0lf", 100.0*EnvelopeProcessor::getInstance()->_parameters.offset);
			SetDlgItemText(hwnd, IDC_PADRELFO_OFFSET, buffer);

			for(int i=eTAKEENV_VOLUME; i<=eTAKEENV_MUTE; i++)
			{
				int x = SendDlgItemMessage(hwnd,IDC_PADRELFO_TAKEENV,CB_ADDSTRING,0,(LPARAM)GetTakeEnvelopeStr((TakeEnvType)i));
				SendDlgItemMessage(hwnd,IDC_PADRELFO_TAKEENV,CB_SETITEMDATA,x,i);
				if(i == EnvelopeProcessor::getInstance()->_parameters.takeEnvType)
					SendDlgItemMessage(hwnd,IDC_PADRELFO_TAKEENV,CB_SETCURSEL,x,0);
			}

			for(int i=0; i<128; i++)
			{
				sprintf(buffer, "%3d", i);
				int x = SendDlgItemMessage(hwnd,IDC_PADRELFO_MIDICC,CB_ADDSTRING,0,(LPARAM)buffer);
				SendDlgItemMessage(hwnd,IDC_PADRELFO_MIDICC,CB_SETITEMDATA,x,i);
				if(i == EnvelopeProcessor::getInstance()->_parameters.midiCc)
					SendDlgItemMessage(hwnd,IDC_PADRELFO_MIDICC,CB_SETCURSEL,x,0);
			}

			switch(EnvelopeProcessor::getInstance()->_parameters.envType)
			{
				case eENVTYPE_TRACK :
					EnableWindow(GetDlgItem(hwnd,IDC_PADRELFO_TAKEENV), FALSE);
					EnableWindow(GetDlgItem(hwnd,IDC_PADRELFO_MIDICC), FALSE);
				break;

				case eENVTYPE_TAKE :
					EnableWindow(GetDlgItem(hwnd,IDC_PADRELFO_TAKEENV), TRUE);
					EnableWindow(GetDlgItem(hwnd,IDC_PADRELFO_MIDICC), FALSE);
				break;

				case eENVTYPE_MIDICC :
					EnableWindow(GetDlgItem(hwnd,IDC_PADRELFO_TAKEENV), FALSE);
					EnableWindow(GetDlgItem(hwnd,IDC_PADRELFO_MIDICC), TRUE);
				break;

				default:
				break;
			}

#ifdef _WIN32
				SendMessage(hwnd, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hwnd, IDD_PADRELFO_GENERATOR), TRUE);
#endif

			return 0;
		}

		case WM_COMMAND :
            switch(LOWORD(wParam))
            {
                case IDOK:
				{
					int combo = SendDlgItemMessage(hwnd,IDC_PADRELFO_TIMESEGMENT,CB_GETCURSEL,0,0);
					if(combo != CB_ERR)
						EnvelopeProcessor::getInstance()->_parameters.timeSegment = (TimeSegment)(SendDlgItemMessage(hwnd,IDC_PADRELFO_TIMESEGMENT,CB_GETITEMDATA,combo,0));

					combo = SendDlgItemMessage(hwnd,IDC_PADRELFO_LFOSHAPE,CB_GETCURSEL,0,0);
					if(combo != CB_ERR)
						EnvelopeProcessor::getInstance()->_parameters.waveShape = (WaveShape)(SendDlgItemMessage(hwnd,IDC_PADRELFO_LFOSHAPE,CB_GETITEMDATA,combo,0));

					combo = SendDlgItemMessage(hwnd,IDC_PADRELFO_SYNCFREQUENCY,CB_GETCURSEL,0,0);
					if(combo != CB_ERR)
						EnvelopeProcessor::getInstance()->_parameters.freqBeat = (GridDivision)(SendDlgItemMessage(hwnd,IDC_PADRELFO_SYNCFREQUENCY,CB_GETITEMDATA,combo,0));

					combo = SendDlgItemMessage(hwnd,IDC_PADRELFO_SYNCDELAY,CB_GETCURSEL,0,0);
					if(combo != CB_ERR)
						EnvelopeProcessor::getInstance()->_parameters.delayBeat = (GridDivision)(SendDlgItemMessage(hwnd,IDC_PADRELFO_SYNCDELAY,CB_GETITEMDATA,combo,0));

					char buffer[BUFFER_SIZE];
					GetDlgItemText(hwnd,IDC_PADRELFO_FREQUENCY,buffer,BUFFER_SIZE);
					EnvelopeProcessor::getInstance()->_parameters.freqHz = atof(buffer);
					GetDlgItemText(hwnd,IDC_PADRELFO_DELAY,buffer,BUFFER_SIZE);
					//EnvelopeProcessor::_parameters.delayMsec = fabs(atof(buffer));
					EnvelopeProcessor::getInstance()->_parameters.delayMsec = atof(buffer);
					GetDlgItemText(hwnd,IDC_PADRELFO_STRENGTH,buffer,BUFFER_SIZE);
					EnvelopeProcessor::getInstance()->_parameters.strength = atof(buffer)/100.0;
					GetDlgItemText(hwnd,IDC_PADRELFO_OFFSET,buffer,BUFFER_SIZE);
					EnvelopeProcessor::getInstance()->_parameters.offset = atof(buffer)/100.0;

					combo = SendDlgItemMessage(hwnd,IDC_PADRELFO_TAKEENV,CB_GETCURSEL,0,0);
					if(combo != CB_ERR)
						EnvelopeProcessor::getInstance()->_parameters.takeEnvType = (TakeEnvType)(SendDlgItemMessage(hwnd,IDC_PADRELFO_TAKEENV,CB_GETITEMDATA,combo,0));

					combo = SendDlgItemMessage(hwnd,IDC_PADRELFO_MIDICC,CB_GETCURSEL,0,0);
					if(combo != CB_ERR)
						EnvelopeProcessor::getInstance()->_parameters.midiCc = SendDlgItemMessage(hwnd,IDC_PADRELFO_MIDICC,CB_GETITEMDATA,combo,0);

					EnvelopeProcessor::ErrorCode res = EnvelopeProcessor::eERRORCODE_UNKNOWN;
					switch(EnvelopeProcessor::getInstance()->_parameters.envType)
					{
						case eENVTYPE_TRACK :
							res = EnvelopeProcessor::getInstance()->generateSelectedTrackEnvLfo();
						break;

						case eENVTYPE_TAKE :
							//res = EnvelopeProcessor::getInstance()->generateSelectedTakesLfo(EnvelopeProcessor::getInstance()->_parameters.takeEnvType, true);
res = EnvelopeProcessor::getInstance()->generateSelectedTakesLfo(true);
						break;

						case eENVTYPE_MIDICC :
							res = EnvelopeProcessor::generateSelectedMidiTakeLfo(true);
						break;

						default:
						break;
					}

					//EnvelopeProcessor::ErrorCode res = EnvelopeProcessor::generateLfo();
					switch(res)
					{
						case EnvelopeProcessor::eERRORCODE_NOENVELOPE:
							MessageBox(hwnd, "No envelope selected!", "Error", MB_OK);
						break;
						case EnvelopeProcessor::eERRORCODE_NULLTIMESELECTION:
							MessageBox(hwnd, "No time selection!", "Error", MB_OK);
						break;
						case EnvelopeProcessor::eERRORCODE_NOITEMSELECTED:
							MessageBox(hwnd, "No item selected!", "Error", MB_OK);
						break;
						case EnvelopeProcessor::eERRORCODE_NOOBJSTATE:
							MessageBox(hwnd, "Could not retrieve envelope object state!", "Error", MB_OK);
						break;
						case EnvelopeProcessor::eERRORCODE_UNKNOWN:
							MessageBox(hwnd, "Could not generate envelope! (Unknown Error)", "Error", MB_OK);
						break;
						default:
						break;
					}

					EndDialog(hwnd,0);
					return 0;
				}

				case IDCANCEL:
				{
					EndDialog(hwnd,0);
					return 0;
				}
			}
	}

	return 0;
}

void EnvelopeLfo(COMMAND_T* _ct)
{
	HWND hwndParent = GetMainHwnd();
	WDL_String args;
	args.SetFormatted(128, "%d\n", _ct->user);
	DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_PADRELFO_GENERATOR), hwndParent, EnvelopeLfoDlgProc, (LPARAM)args.Get());
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
	Undo_OnStateChangeEx("Item Processor: shrink", UNDO_STATE_ALL, -1);
	UpdateTimeline();
}

void ShrinkSelItems(COMMAND_T* _ct)
{
	int nbSamples = _ct->user;
	ShrinkSelectedTakes(nbSamples);
}

void RandomizeMidiNotePos(COMMAND_T* _ct)
{
	midiNoteRandomizer->processSelectedMidiTakes(true);
}

