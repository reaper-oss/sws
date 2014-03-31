/******************************************************************************
/ BR_Loudness.h
/
/ Copyright (c) 2014 Dominik Martin Drzic
/ http://forum.cockos.com/member.php?u=27094
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
#pragma once
#include "BR_EnvTools.h"

/******************************************************************************
* Loudness object                                                             *
******************************************************************************/
class BR_LoudnessObject
{
public:
	BR_LoudnessObject (MediaTrack* track);
	BR_LoudnessObject (MediaItem_Take* take);
	~BR_LoudnessObject ();

	/* Analyze */
	bool Analyze (bool integratedOnly = false);
	void StopAnalyze ();
	bool IsRunning ();
	double GetProgress ();
	void NormalizeIntegrated (double target); // normalize using existing analyze data to target dB
	bool CreateGraph (BR_Envelope& envelope, double min, double max, bool momentary);

	/* For populating list view in analyze loudness dialog */
	double GetColumnVal (int column);
	void GetColumnStr (int column, char* str, int strSz);

	/* Various options (mainly for analyze loudness dialog) */
	double GetAudioLength ();
	bool IsTrack ();
	bool IsTargetValid ();
	bool IsSelectedInProject ();
	void SetSelectedInProject (bool selected);
	void GotoMomentaryMax (bool timeSelection);
	void GotoShortTermMax (bool timeSelection);
	void GotoTarget ();

private:
	struct AudioData
	{
		AudioAccessor* audio;
		char audioHash[128];
		int samplerate, channels, channelMode;
		double audioStart, audioEnd;
		double volume, pan;
		BR_Envelope volEnv, volEnvPreFX;
		AudioData();
	} m_audioData;

	static unsigned WINAPI AnalyzeData (void* loudnessObject);
	int CheckSetAudioData (); // call from main thread only, returns 0->target doesn't exist anymore, 1->old accessor still valid, 2->accessor got updated
	AudioData* GetAudioData ();
	void SetRunning (bool running);
	void SetProgress (double progress);
	void SetAnalyzeData (double integrated, double range, double shortTermMax, double momentaryMax, vector<double>& shortTermValues, vector<double>& momentaryValues);
	void GetAnalyzeData (double* integrated, double* range, double* shortTermMax, double* momentaryMax);
	void SetAnalyzedStatus (bool analyzed);
	bool GetAnalyzedStatus ();
	void SetIntegratedOnly (bool integratedOnly);
	bool GetIntegratedOnly ();
	void SetKillFlag (bool killFlag);
	bool GetKillFlag ();
	void SetProcess (HANDLE process);
	HANDLE GetProcess ();
	WDL_FastString GetTakeName ();
	WDL_FastString GetTrackName ();
	void operator= (const BR_LoudnessObject&);    // we don't need assignment or copy constructor so disable
	BR_LoudnessObject (const BR_LoudnessObject&); // for now! (it could destroy perfectly valid accessor)
	MediaTrack* m_track;
	MediaItem_Take* m_take;
	GUID m_takeGuid, m_trackGuid;
	double m_integrated, m_shortTermMax, m_momentaryMax, m_range;
	double m_progress;
	bool m_running, m_analyzed, m_killFlag, m_integratedOnly;
	HANDLE m_process;
	SWS_Mutex m_mutex;
	vector<double> m_shortTermValues;
	vector<double> m_momentaryValues;
};

/******************************************************************************
* Analyze loudness list view                                                  *
******************************************************************************/
class BR_AnalyzeLoudnessView : public SWS_ListView
{
public:
	BR_AnalyzeLoudnessView (HWND hwndList, HWND hwndEdit);

protected:
	virtual void GetItemText (SWS_ListItem* item, int iCol, char* str, int iStrMax);
	virtual void GetItemList (SWS_ListItemList* pList);
	virtual void OnItemSelChanged (SWS_ListItem* item, int iState);
	virtual void OnItemDblClk (SWS_ListItem* item, int iCol);
	virtual int OnItemSort (SWS_ListItem* item1, SWS_ListItem* item2);
};

/******************************************************************************
* Analyze loudness window                                                     *
******************************************************************************/
class BR_AnalyzeLoudnessWnd : public SWS_DockWnd
{
public:
	BR_AnalyzeLoudnessWnd ();
	~BR_AnalyzeLoudnessWnd () {};

	struct Properties
	{
		bool analyzeTracks;
		bool analyzeOnNormalize;
		bool mirrorSelection;
		bool doubleClickGotoTarget;
		bool timeSelectionOverMax;
		bool clearEnvelope;
		double graphMin;
		double graphMax;
		Properties ();
		void Load ();
		void Save ();
	} m_properties;

protected:
	void StopAnalyze ();
	void StopReanalyze ();
	void ClearList ();
	virtual void OnInitDlg ();
	virtual void OnCommand (WPARAM wParam, LPARAM lParam);
	virtual void OnTimer (WPARAM wParam);
	virtual void OnDestroy ();
	virtual void GetMinSize (int* w, int* h);
	virtual HMENU OnContextMenu (int x, int y, bool* wantDefaultItems);

	double m_objectsLen;
	int m_currentObjectId;
	bool m_analyzeInProgress;
	BR_AnalyzeLoudnessView* m_list;                                                  // never delete objects in reanalyzeQueue when removing them from list!!
	WDL_PtrList_DeleteOnDestroy<BR_LoudnessObject> m_analyzeQueue, m_reanalyzeQueue; // m_analyzeQueue is ok if the object didn't enter g_analyzedObjects

};

void AnalyzeLoudness (COMMAND_T*);
int IsAnalyzeLoudnessVisible (COMMAND_T*);
int AnalyzeLoudnessInit ();
void AnalyzeLoudnessExit ();
void CleanAnalyzeLoudnessProjectData ();

/******************************************************************************
* Commands                                                                    *
******************************************************************************/
void NormalizeLoudness (COMMAND_T*);