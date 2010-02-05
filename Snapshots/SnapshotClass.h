/******************************************************************************
/ SnapshotClass.h
/
/ Copyright (c) 2010 Tim Payne (SWS)
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

#define DOUBLES_PER_LINE 8

class FXSnapshot
{
public:
    FXSnapshot(MediaTrack* tr, int fx);
	FXSnapshot(FXSnapshot& fx);
    FXSnapshot(LineParser* lp);
    ~FXSnapshot();

	void GetChunk(WDL_String* chunk);
    void RestoreParams(char* str);
    int UpdateReaper(MediaTrack* tr, bool* bMatched, int num);
	bool Exists(MediaTrack* tr);

    double* m_dParams;
    int m_iNumParams;
	int m_iCurParam;
    char m_cName[256];
};

class TrackSnapshot
{
public:
    TrackSnapshot(MediaTrack* tr, int mask);
	TrackSnapshot(TrackSnapshot& ts);
    TrackSnapshot(LineParser* lp);
    ~TrackSnapshot();

    bool UpdateReaper(int mask, bool bSelOnly, int* fxErr, WDL_PtrList<TrackSendFix>* pFix);
	bool Cleanup();
	void GetChunk(WDL_String* chunk);
	void GetDetails(WDL_String* details, int iMask);

// TODO these should be private
	GUID m_guid;
    double m_dVol;
    double m_dPan;
    bool m_bMute;
    int m_iSolo;
    int m_iFXEn;
	int m_iVis;
	int m_iSel;
    WDL_PtrList<FXSnapshot> m_fx;
	WDL_TypedBuf<char> m_sFXChain;
	TrackSends m_sends;
	WDL_String m_sName;
	int m_iTrackNum;
};

// Mask:
#define VOL_MASK        0x001
#define PAN_MASK        0x002
#define MUTE_MASK       0x004
#define SOLO_MASK       0x008
#define FXATM_MASK		0x010
#define SENDS_MASK		0x020
//#define SELONLY_MASK	0x040
#define VIS_MASK		0x080
#define SEL_MASK		0x100
#define FXCHAIN_MASK	0x200
#define ALL_MASK        0xFFF // large enough for forward compat
#define MIX_MASK		(VOL_MASK | PAN_MASK | MUTE_MASK | SOLO_MASK | FXCHAIN_MASK | SENDS_MASK)

// Map controls to mask elements
const int cSSMasks[] = { VOL_MASK, PAN_MASK, MUTE_MASK, SOLO_MASK, SENDS_MASK, VIS_MASK,       SEL_MASK,      FXCHAIN_MASK };
const int cSSCtrls[] = { IDC_VOL,  IDC_PAN,  IDC_MUTE,  IDC_SOLO,  IDC_SENDS,  IDC_VISIBILITY, IDC_SELECTION, IDC_FXCHAIN }; 
#define MASK_CTRLS 8

class Snapshot
{
public:
    Snapshot(int slot, int mask, bool bSelOnly, char* name);   // For capture
	Snapshot(const char* chunk); // For project load
    ~Snapshot();
    bool UpdateReaper(int mask, bool bSelOnly, bool bHideNewVis);
    char* Tooltip(char* str, int maxLen);
    void SetName(const char* name);
	void AddSelTracks();
	void DelSelTracks();
	void SelectTracks();
	int Find(MediaTrack* tr);
	static void RegisterGetCommand(int iSlot);
	char* GetTimeString(char* str, int iStrMax, bool bDate);
	void GetChunk(WDL_String* chunk);
	void GetDetails(WDL_String* details);

// TODO these should be private
	char* m_cName;
    int m_iSlot;
    int m_iMask;
	int m_time;
    
    WDL_PtrList<TrackSnapshot> m_tracks;
};