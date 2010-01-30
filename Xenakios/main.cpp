/******************************************************************************
/ main.cpp
/
/ Copyright (c) 2010 Tim Payne (SWS), original code by Xenakios
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
#include "Parameters.h"

using namespace std;

int *ShuffledNumbers;
int ShuffledNumbersGenerated=0;

bool IsRippleOneTrack(COMMAND_T*) { return *(int*)GetConfigVar("projripedit") == 1; }
bool IsRippleAll(COMMAND_T*)      { return *(int*)GetConfigVar("projripedit") == 2; }

void DoToggleRippleOneTrack(COMMAND_T*)
{
	int* ripplemode = (int*)GetConfigVar("projripedit");
	if (ripplemode)
	{
		if (*ripplemode == 1)
			Main_OnCommand(40309, 0);
		else
			Main_OnCommand(40310, 0);
	}
}

void DoToggleRippleAll(COMMAND_T*)
{
	int* ripplemode = (int*)GetConfigVar("projripedit");
	if (ripplemode)
	{
		if (*ripplemode == 2)
			Main_OnCommand(40309, 0);
		else
			Main_OnCommand(40311, 0);
	}
}

bool GenerateShuffledRandomTable(int *IntTable,int numItems,int badFirstNumber)
{
	//
	int *CheckTable=new int[1024];
	bool GoodFound=FALSE;
	int IterCount=0;
	int rndInt;
	int i;
	for (i=0;i<1024;i++) 
	{
		CheckTable[i]=0;
		IntTable[i]=0;
	}
	
	for (i=0;i<numItems;i++)
	{
		//
		GoodFound=FALSE;
		while (!GoodFound)
		{
			rndInt=rand() % numItems;
			if ((CheckTable[rndInt]==0) && (rndInt!=badFirstNumber) && (i==0)) GoodFound=TRUE;
			if ((CheckTable[rndInt]==0) && (i>0)) GoodFound=TRUE;
			
			
			IterCount++;
			if (IterCount>10000000) 
			{
				MessageBox(g_hwndParent,"Shuffle Random Table Generator Failed, over 10000000 iterations!","Error",MB_OK);
				break;
			}
		}
		if (GoodFound) 
		{
			IntTable[i]=rndInt;
			CheckTable[rndInt]=1;
		}
	}

	delete[] CheckTable;
	return FALSE;
}

void DoSelectFiles(COMMAND_T*)
{
	char* cFiles = BrowseForFiles("Select files", NULL, NULL, true, "WAV Files\0*.wav\0");
	if (cFiles)
	{
		g_filenames->Empty(true, free);
		char* pStr = cFiles;
		while(*pStr)
		{
			strcpy(g_filenames->Add((char*)malloc(strlen(pStr)+1)), pStr);
			pStr += strlen(pStr)+1;
		}
		free(cFiles);
	}
	ShuffledNumbersGenerated=0;
	GenerateShuffledRandomTable(ShuffledNumbers,g_filenames->GetSize(),-1);
}

void DoInsertRandom(COMMAND_T*)
{
	if (g_filenames->GetSize()>0)
		InsertMedia(g_filenames->Get(rand() % g_filenames->GetSize()), 0);
}

void DoInsRndFileEx(bool RndLen,bool RndOffset,bool UseTimeSel)
{
	if (g_filenames->GetSize()>0)
	{
		int filenameindex=rand() % g_filenames->GetSize();
		
		t_vect_of_Reaper_tracks TheTracks;
		XenGetProjectTracks(TheTracks,true);
		if (TheTracks.size()>0)
		{
		PCM_source *NewPCM=PCM_Source_CreateFromFile(g_filenames->Get(filenameindex));
		MediaItem *NewItem=AddMediaItemToTrack(TheTracks[0]);
		MediaItem_Take *NewTake=AddTakeToMediaItem(NewItem);
		double TimeSelStart=0.0;
		double TimeSelEnd=NewPCM->GetLength();
		GetSet_LoopTimeRange(false,false,&TimeSelStart,&TimeSelEnd,false);
		double ItemPos=TimeSelStart;
		double ItemLen=NewPCM->GetLength();
		double MediaOffset=0.0;
		if (RndOffset) 
		{
			MediaOffset=(NewPCM->GetLength()/RAND_MAX)*rand();
			ItemLen-=MediaOffset;
		}
		if (RndLen) ItemLen=((NewPCM->GetLength()-MediaOffset)/RAND_MAX)*rand();
		if (UseTimeSel) ItemLen=TimeSelEnd-TimeSelStart;
		if (!UseTimeSel) ItemPos=GetCursorPosition();
		GetSetMediaItemTakeInfo(NewTake,"P_SOURCE",NewPCM);
		GetSetMediaItemTakeInfo(NewTake,"D_STARTOFFS",&MediaOffset);
		
		GetSetMediaItemInfo(NewItem,"D_POSITION",&ItemPos);
		GetSetMediaItemInfo(NewItem,"D_LENGTH",&ItemLen);
		Main_OnCommand(40047,0); // build any missing peaks
		SetEditCurPos(ItemPos+ItemLen,false,false);
		Undo_OnStateChangeEx("Insert random file (Extended)",4,-1);
		 
		UpdateTimeline();
		}
	}	
}

void DoInsRndFileRndLen(COMMAND_T*)
{
	DoInsRndFileEx(true,false,false);	
}

void DoInsRndFileAtTimeSel(COMMAND_T*)
{
	DoInsRndFileEx(false,false,true);
}

void DoInsRndFileRndOffset(COMMAND_T*)
{
	DoInsRndFileEx(false,true,false);
}

void DoInsRndFileRndOffsetAtTimeSel(COMMAND_T*)
{
	DoInsRndFileEx(false,true,true);
}

void DoRoundRobinSelectTakes(COMMAND_T*)
{
	//
	MediaTrack* CurTrack;
	MediaItem* CurItem;
	bool ItemSelected;
	int numItems;
	int numTakes;
	int TakeToSelect = 0;
	int trackID;
	int itemID;
	for (trackID=0;trackID<GetNumTracks();trackID++)
	{
		CurTrack=CSurf_TrackFromID(trackID+1,FALSE);
		numItems=GetTrackNumMediaItems(CurTrack);
		for (itemID=0;itemID<numItems;itemID++)
		{
			CurItem = GetTrackMediaItem(CurTrack,itemID);
			ItemSelected=*(bool*)GetSetMediaItemInfo(CurItem,"B_UISEL",NULL);
			if (ItemSelected)
			{
				numTakes=GetMediaItemNumTakes(CurItem);
				if (numTakes>0)
				{	
					if (TakeToSelect>(numTakes-1)) TakeToSelect=0;
					GetSetMediaItemInfo(CurItem,"I_CURTAKE",&TakeToSelect);
					TakeToSelect++;
				}
			}
		}
	}
	Undo_OnStateChangeEx("Cycle Select Takes",4,-1);
	UpdateTimeline();
}


void DoSelectTakeInSelectedItems(int takeIndx) // -1 first -2 last take, otherwise index, if bigger than numtakes in item, the last
{
	//
	MediaTrack* CurTrack;
	MediaItem* CurItem;
	bool ItemSelected;
	int numItems;
	int numTakes;
	int trackID;
	int itemID;
	for (trackID=0;trackID<GetNumTracks();trackID++)
	{
		CurTrack=CSurf_TrackFromID(trackID+1,FALSE);
		numItems=GetTrackNumMediaItems(CurTrack);
		for (itemID=0;itemID<numItems;itemID++)
		{
			CurItem = GetTrackMediaItem(CurTrack,itemID);
			ItemSelected=*(bool*)GetSetMediaItemInfo(CurItem,"B_UISEL",NULL);
			if (ItemSelected==TRUE)
			{
				//
				numTakes=GetMediaItemNumTakes(CurItem);
				if (numTakes>0)
				{
					int TakeToSelect;
					if (takeIndx==-2) { TakeToSelect=numTakes-1; GetSetMediaItemInfo(CurItem,"I_CURTAKE",&TakeToSelect); }
					if (takeIndx==-1) { TakeToSelect=0; GetSetMediaItemInfo(CurItem,"I_CURTAKE",&TakeToSelect); }
					if (takeIndx>=0)
					{ 
						TakeToSelect=takeIndx;
						GetSetMediaItemInfo(CurItem,"I_CURTAKE",&TakeToSelect);
					}
				}
			}
		}
	}
}

void DoSelectFirstTakesInItems(COMMAND_T*)
{
	DoSelectTakeInSelectedItems(-1);
	Undo_OnStateChangeEx("Select First Takes Of Items",4,-1);
	UpdateTimeline();
}

void DoSelectLastTakesInItems(COMMAND_T*)
{
	DoSelectTakeInSelectedItems(-2);
	Undo_OnStateChangeEx("Select Last Takes Of Items",4,-1);
	UpdateTimeline();
}

void DoInsertShuffledRandomFile(COMMAND_T*)
{
	if (g_filenames->GetSize()>2)
	{
	 int FileToChoose=ShuffledNumbers[ShuffledNumbersGenerated];	
	 char* filename;
	 filename=g_filenames->Get(FileToChoose);
	 InsertMedia(filename,0);
	 ShuffledNumbersGenerated++;
	 if (ShuffledNumbersGenerated==g_filenames->GetSize())
	 {
		//
		GenerateShuffledRandomTable(ShuffledNumbers,g_filenames->GetSize(),FileToChoose);
		ShuffledNumbersGenerated=0;
	 }
	} else MessageBox(g_hwndParent,"Too few files to choose from for random shuffled insert!","Error",MB_OK);
	
}

bool ItemTimesCompFunc(MediaItem* ita,MediaItem* itb)
{
	double itapos=*(double*)GetSetMediaItemInfo(ita,"D_POSITION",0);
	double itbpos=*(double*)GetSetMediaItemInfo(itb,"D_POSITION",0);
	return itapos<itbpos;
}

double g_FirstSelectedItemPos;
double g_LastSelectedItemEnd;

void DoSetLoopPointsToSelectedItems(bool SetTheLoop)
{
	vector<MediaItem*> selitems;
	XenGetProjectItems(selitems,true,true);
	if (selitems.size()>0)
	{
		double MinItemPos,MaxItemEnd=0.0;
		sort(selitems.begin(),selitems.end(),ItemTimesCompFunc);
		MinItemPos=*(double*)GetSetMediaItemInfo(selitems[0],"D_POSITION",0);
		double MaxItemPos=*(double*)GetSetMediaItemInfo(selitems[selitems.size()-1],"D_POSITION",0);
		MaxItemEnd=MaxItemPos+*(double*)GetSetMediaItemInfo(selitems[selitems.size()-1],"D_LENGTH",0);
		g_FirstSelectedItemPos=MinItemPos;
		g_LastSelectedItemEnd=MaxItemEnd;
		if (SetTheLoop)
			GetSet_LoopTimeRange(true,true,&MinItemPos,&MaxItemEnd,false);
	}
}

bool g_PlayItemsOncePlaying=false;

void DoNudgeItemsLeftSecsAndConfBased(COMMAND_T*)
{
	DoNudgeSelectedItemsPositions(true,false,0.0);
}

void DoNudgeItemsRightSecsAndConfBased(COMMAND_T*)
{
	DoNudgeSelectedItemsPositions(true,true,0.0);
}

void DoSaveMarkersAsTextFile(COMMAND_T*)
{
	// People can just use SWS track sheet export facility instead?  TRP Oct 20 2009
	char OutFileName[512];
	if (BrowseForSaveFile("Choose text file to save markers to", NULL, NULL, "TXT files\0*.txt\0", OutFileName, 512))
	{
		char TimeText1[32];
		int x=0;
		bool isrgn;
		double pos, rgnend;
		char *name;
		int number;
		std::ofstream os(OutFileName);
		if (!os)
			return;
		os << "#\tPosition\tName" << "\n";
		while ((x=EnumProjectMarkers(x,&isrgn,&pos,&rgnend,&name,&number)))
		{
			format_timestr(pos, TimeText1,25);
			if (!isrgn)
				os << x << "\t" << TimeText1 << "\t" << name << "\n";
			else
				os << x << "\t" << TimeText1 << "\t" << name << " (Region)\n";
		}
	}
}

void DoResampleTakeOneSemitoneDown(COMMAND_T*)
{
	Undo_BeginBlock();
	Main_OnCommand(40518, 0);
	Main_OnCommand(40205, 0);
	Undo_EndBlock("Pitch item down by 1 semitone (resampled)",0);	
}

void DoResampleTakeOneSemitoneUp(COMMAND_T*)
{
	Undo_BeginBlock();
	Main_OnCommand(40517, 0);
	Main_OnCommand(40204, 0);
	Undo_EndBlock("Pitch item up by 1 semitone (resampled)",0);
}

void DoLoopAndPlaySelectedItems(COMMAND_T*)
{
	Main_OnCommand(1016, 0); // Transport Stop
	DoSetLoopPointsToSelectedItems(true);
	Main_OnCommand(40632, 0); // Move cursor to start of loop
	GetSetRepeat(1); // set repeat on
	Main_OnCommand(1007, 0); // Transport Play	
}

void PlayItemsOnceSlice()
{
	if (g_PlayItemsOncePlaying)
	{
		if (GetPlayState() == 0 || GetPlayPosition2() < g_FirstSelectedItemPos)
			g_PlayItemsOncePlaying = false;
		else if (GetPlayState() & 1 && GetPlayPosition2() >= g_LastSelectedItemEnd)
		{
			g_PlayItemsOncePlaying = false;
			Main_OnCommand(1016, 0); // Transport Stop
		}
	}
}

void DoPlayItemsOnce(COMMAND_T*)
{
	if (GetNumSelectedItems() > 0)
	{
		if (g_PlayItemsOncePlaying)
		{
			g_PlayItemsOncePlaying = false;
			Main_OnCommand(1016, 0); // Transport Stop
		}

		Main_OnCommand(40634, 0); // remove loop points
		DoSetLoopPointsToSelectedItems(false);
		SetEditCurPos(g_FirstSelectedItemPos, false, false);
		
		Main_OnCommand(1007, 0); // Transport Play
		Sleep(100);
		g_PlayItemsOncePlaying = true;
	}
}

void DoMoveCurNextTransMinusFade(COMMAND_T*)
{
	int sz=0; double *defFadeLen = (double *)get_config_var("deffadelen",&sz);
	static double prevCurPos = -666.0;
	double CurPos=GetCursorPosition();
	if (CurPos==prevCurPos)
	{
		CurPos=GetCursorPosition()+*defFadeLen;
		SetEditCurPos(CurPos,false,false);
	}
	Main_OnCommand(40375,0);
	CurPos=GetCursorPosition()-*defFadeLen;
	SetEditCurPos(CurPos,false,false);
	prevCurPos=CurPos;
}

void DoMoveCurPrevTransMinusFade(COMMAND_T*)
{
	Main_OnCommand(40376,0); // move to next transient
}

#define ALEXPIXELS 12

void DoMoveCursor10pixRight(COMMAND_T*) { for (int i = 0; i < ALEXPIXELS; i++) Main_OnCommand(40105,0); }
void DoMoveCursor10pixLeft(COMMAND_T*)  { for (int i = 0; i < ALEXPIXELS; i++) Main_OnCommand(40104,0); }
void DoMoveCursor10pixLeftCreateSel(COMMAND_T*)  { for (int i = 0; i < ALEXPIXELS; i++) Main_OnCommand(40102,0); }
void DoMoveCursor10pixRightCreateSel(COMMAND_T*) { for (int i = 0; i < ALEXPIXELS; i++) Main_OnCommand(40103,0); }

preview_register_t g_ItemPreview = { {}, 0, };
bool g_itemPreviewPlaying = false;
void ItemPreviewSlice()
{
	if (g_itemPreviewPlaying)
	{
#ifdef _WIN32
		EnterCriticalSection(&g_ItemPreview.cs);
#else
		pthread_mutex_lock(&g_ItemPreview.mutex);
#endif
		// Have we reached the end?
		if (g_ItemPreview.curpos >= g_ItemPreview.src->GetLength())
		{
#ifdef _WIN32
			LeaveCriticalSection(&g_ItemPreview.cs);
#else
			pthread_mutex_unlock(&g_ItemPreview.mutex);
#endif
			StopPreview(&g_ItemPreview);
			g_itemPreviewPlaying = false;
			delete g_ItemPreview.src;
		}
		else
#ifdef _WIN32
			LeaveCriticalSection(&g_ItemPreview.cs);
#else
			pthread_mutex_unlock(&g_ItemPreview.mutex);
#endif
	}
}

// t->user:
// 0: start
// 1: stop
// 2: toggle
void DoPreviewItem(COMMAND_T* t)
{
	if (g_itemPreviewPlaying)
	{
		// preview called while preview in progress, stopping previous preview...
		StopPreview(&g_ItemPreview);
		g_itemPreviewPlaying = false;
		delete g_ItemPreview.src;
		if (t->user == 2)
			return;
	}

	if (t->user == 1)
		return;
			
	for (int i = 0; i < GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i+1, false);
		for (int j = 0; j < GetTrackNumMediaItems(tr); j++)
		{
			MediaItem* curItem = GetTrackMediaItem(tr, j);
			if (*(bool*)GetSetMediaItemInfo(curItem, "B_UISEL", NULL))
			{
				PCM_source* src = ((PCM_source*)curItem)->Duplicate(); // Casting from MediaItem* to PCM_source works!  Who would have known?
				if (src)
				{
					double dZero = 0.0;
					GetSetMediaItemInfo((MediaItem*)src, "D_POSITION", &dZero);

					if (!g_ItemPreview.src) // src == 0 means need to initialize structure
					{
#ifdef _WIN32
						InitializeCriticalSection(&g_ItemPreview.cs);
#else
						pthread_mutex_init(&g_ItemPreview.mutex, NULL);
#endif
						g_ItemPreview.m_out_chan = 0;
						g_ItemPreview.volume = 1.0;
						g_ItemPreview.loop = false;
					}
					g_ItemPreview.src = src;
					g_ItemPreview.curpos = 0.0;
					if (PlayPreview(&g_ItemPreview))
						g_itemPreviewPlaying = true;
				}
				return;
			}
		}
	}
}

void DoStopPreviewItem(COMMAND_T*)
{
	if (g_itemPreviewPlaying)
	{
		// preview called while preview in progress, stopping previous preview...
		StopPreview(&g_ItemPreview);
		g_itemPreviewPlaying = false;
		delete g_ItemPreview.src;
	}
}

void DoDumpActionsWindow(COMMAND_T*)
{
#ifdef _WIN32
	std::ofstream os("C:/ReaperActionlist.txt");
#else
	std::ofstream os("ReaperActionlist.txt");
#endif
	
	os << "Shortcut\t\t\tDescription" << "\n";
	HWND hActionsWindow= FindWindowEx(NULL,NULL,"#32770","Actions");
	if (hActionsWindow!=0)
	{
		HWND hActionListView=FindWindowEx(hActionsWindow,NULL,"SysListView32","List1");
		if (hActionListView!=0)
		{
			int NumActions=ListView_GetItemCount(hActionListView);
			int i;
			for (i=0;i<NumActions;i++)
			{
				char buf[500];
				char buf2[50];
				ListView_GetItemText(hActionListView,i,1,buf,500);
				ListView_GetItemText(hActionListView,i,0,buf2,50);
				if (strcmp(buf2,"")==0) strcpy(buf2,"No shortcut");
				os << buf2;
				os.width(30);
				os << "\t\t\t" << buf << "\n";
			}
		}
	}
}

void DoScrollTVPageDown(COMMAND_T*)
{
	HWND hTrackView=GetTrackWnd();
	if (hTrackView) 
	{
		SendMessage(hTrackView,WM_VSCROLL,SB_PAGEDOWN,0);	
	}
}

void DoScrollTVPageUp(COMMAND_T*)
{
	HWND hTrackView=GetTrackWnd();
	if (hTrackView) 
	{
		SendMessage(hTrackView,WM_VSCROLL,SB_PAGEUP,0);	
	}
}

void DoScrollTVHome(COMMAND_T*)
{
	HWND hTrackView=GetTrackWnd();
	if (hTrackView) 
	{
		SendMessage(hTrackView,WM_VSCROLL,SB_TOP,0);	
	}
}

void DoScrollTVEnd(COMMAND_T*)
{
	HWND hTrackView=GetTrackWnd();
	if (hTrackView) 
	{
		SendMessage(hTrackView,WM_VSCROLL,SB_BOTTOM,0);	
	}
}

void DoRenameMarkersWithAscendingNumbers(COMMAND_T*)
{
	int x=0;
	
	bool isrgn;
	double pos, rgnend;
	char *name;
	int number;
	char newmarkname[100];
	int j=1;
	while ((x=EnumProjectMarkers(x,&isrgn,&pos,&rgnend,&name,&number)))
	{
		if (!isrgn)
		{
			sprintf(newmarkname,"%.3d",j);
			SetProjectMarker(number,false,pos,rgnend,newmarkname);
			j++;
		}
	}
	Undo_OnStateChangeEx("Rename markers with ascending numbers",8,-1);
}

void DoSetStopAtEndOfTimeSel(int enabled) // -1 toggle 0 unset 1 set
{
	// stopendofloop
	int sz=0;
	int *stopatend = (int *)get_config_var("stopendofloop",&sz);
    if (stopatend) 
	{ 
		if (enabled==-1)
		{
			if (*stopatend==0)
				*stopatend=1;
			else
				*stopatend=0;
		}
		else
			*stopatend=enabled;
	}
}

bool IsStopAtEndOfTimeSel(COMMAND_T*) { return *(int*)GetConfigVar("stopendofloop") ? true : false; }

void DoToggleSTopAtEndOfTimeSel(COMMAND_T*)
{
	DoSetStopAtEndOfTimeSel(-1);
}

static void menuhook(const char* menustr, HMENU hMenu, int flag)
{
	if (strcmp(menustr, "Main extensions") == 0 && flag == 0)
	{
		SWSCreateMenu(g_XenCommandTable, hMenu);
	}
	else if (strcmp(menustr, "Media item context") == 0 && flag == 0)
	{
		int i = 0;
		while (g_XenCommandTable[i++].accel.accel.cmd != 1);
		AddSubMenu(hMenu, SWSCreateMenu(&g_XenCommandTable[i]), "Extensions : Item/Take selection", -24);
		i = 0;
		while (g_XenCommandTable[i++].accel.accel.cmd != 2);
		AddSubMenu(hMenu, SWSCreateMenu(&g_XenCommandTable[i]), "Extensions : Item/Take manipulation", -24);
	}
	else if (strcmp(menustr, "Track control panel context") == 0 && flag == 0)
	{
		int i = 0;
		while (g_XenCommandTable[i++].accel.accel.cmd != 3);
		AddSubMenu(hMenu, SWSCreateMenu(&g_XenCommandTable[i]), "Extensions : Track/Mixer/Envelopes");
	}
	else if (flag == 1)
	{
		SWSSetMenuText(hMenu, SWSGetCommandID(DoLaunchExtTool, 1), g_external_app_paths.Tool1MenuText);
		SWSSetMenuText(hMenu, SWSGetCommandID(DoLaunchExtTool, 2), g_external_app_paths.Tool2MenuText);
	}
}

int XenakiosInit()
{
	if(!plugin_register("projectconfig",&xen_reftrack_pcreg))
		return 0;

	if (!plugin_register("hookcustommenu", (void*)menuhook))
		return 0;

	ShuffledNumbers=new int[1024];

	SWSRegisterCommands(g_XenCommandTable);

	AddExtensionsMainMenu();
	
	InitCommandParams();
	
	g_filenames = new(WDL_PtrList<char>);
    
	InitUndoKeyUpHandler01();
	g_hItemInspector = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_ITEM_INSPECTOR), g_hwndParent, (DLGPROC)MyItemInspectorDlgProc);
	if (g_hItemInspector!=NULL) 
	{
		ShowWindow(g_hItemInspector, SW_HIDE);
	};

	srand ((unsigned int)time(NULL));
	return 1;
}
