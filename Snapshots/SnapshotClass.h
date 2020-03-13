/******************************************************************************
/ SnapshotClass.h
/
/ Copyright (c) 2010 Tim Payne (SWS)
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

#pragma once

#define DOUBLES_PER_LINE 8

class FXSnapshot
{
public:
    FXSnapshot(MediaTrack* tr, int fx);
	FXSnapshot(FXSnapshot& fx);
    FXSnapshot(LineParser* lp);
    ~FXSnapshot();

	void GetChunk(WDL_FastString* chunk);
    void RestoreParams(const char* str);
    int UpdateReaper(MediaTrack* tr, bool* bMatched, int num);
	bool Exists(MediaTrack* tr);

    double* m_dParams;
    int m_iNumParams;
	int m_iCurParam;
    char m_cName[256];
    char m_cNotes[256];
};

class TrackSnapshot
{
public:
    TrackSnapshot(MediaTrack* tr, int mask);
	TrackSnapshot(TrackSnapshot& ts);
    TrackSnapshot(LineParser* lp);
    ~TrackSnapshot();

	bool UpdateReaper(int mask, bool bSelOnly, int* fxErr, bool wantChunk, WDL_PtrList<TrackSendFix>* pFix);
	bool Cleanup();
	void GetChunk(WDL_FastString* chunk);
	void GetDetails(WDL_FastString* details, int iMask);

	static void GetSetEnvelope(MediaTrack* tr, WDL_FastString* str, const char* env, bool bSet);
	static bool ProcessEnv(const char* chunk, char* line, int iLineMax, int* pos, const char* env, WDL_FastString* str);

// TODO these should be private
	GUID m_guid;
    double m_dVol;
    double m_dPan;
    bool m_bMute;
    int m_iSolo;
    int m_iFXEn;
	int m_iVis;
	int m_iSel;
	bool m_bPhase;
	// track playback offset, REAPER v6.0+
	int m_iPlayOffsetFlag;
	double m_dPlayOffset;
    WDL_PtrList<FXSnapshot> m_fx;
	WDL_TypedBuf<char> m_sFXChain;
	TrackSends m_sends;
	WDL_FastString m_sName;
	int m_iTrackNum;

	// New v4 pans:
	int m_iPanMode;
	double m_dPanWidth;
	double m_dPanL;
	double m_dPanR;
	double m_dPanLaw;
	
	WDL_FastString m_sVolEnv;
	WDL_FastString m_sVolEnv2;
	WDL_FastString m_sPanEnv;
	WDL_FastString m_sPanEnv2;
	WDL_FastString m_sWidthEnv;
	WDL_FastString m_sWidthEnv2;
	WDL_FastString m_sMuteEnv;
};

// Mask:
#define VOL_MASK         0x001
#define PAN_MASK         0x002
#define MUTE_MASK        0x004
#define SOLO_MASK        0x008
#define FXATM_MASK       0x010
#define SENDS_MASK       0x020
//#define SELONLY_MASK   0x040
#define VIS_MASK         0x080
#define SEL_MASK         0x100
#define FXCHAIN_MASK     0x200
#define PHASE_MASK       0x400
#define PLAY_OFFSET_MASK 0x800 // track playback offset, REAPER v6.0+
#define ALL_MASK         0xFEF // large enough for forward compat, leave out FXATM
#define MIX_MASK         (VOL_MASK | PAN_MASK | MUTE_MASK | SOLO_MASK | FXCHAIN_MASK | SENDS_MASK | PHASE_MASK | PLAY_OFFSET_MASK)

// Map controls to mask elements
const int cSSMasks[] = { VOL_MASK, PAN_MASK, MUTE_MASK, SOLO_MASK, SENDS_MASK, VIS_MASK,       SEL_MASK,      FXCHAIN_MASK, PHASE_MASK, PLAY_OFFSET_MASK };
const int cSSCtrls[] = { IDC_VOL,  IDC_PAN,  IDC_MUTE,  IDC_SOLO,  IDC_SENDS,  IDC_VISIBILITY, IDC_SELECTION, IDC_FXCHAIN,  IDC_PHASE,  IDC_PLAY_OFFSET };
#define MASK_CTRLS 10

class Snapshot
{
public:
    Snapshot(int slot, int mask, bool bSelOnly, const char* name, const char* desc);   // For capture
	Snapshot(const char* chunk); // For project load
    ~Snapshot();
    bool UpdateReaper(int mask, bool bSelOnly, bool bHideNewVis);
    char* Tooltip(char* str, int maxLen);
    void SetName(const char* name);
    void SetNotes(const char* notes);
	void AddSelTracks();
	void DelSelTracks();
	void SelectTracks();
	int Find(MediaTrack* tr);
	char* GetTimeString(char* str, int iStrMax, bool bDate);
	void GetChunk(WDL_FastString* chunk);
	void GetDetails(WDL_FastString* details);
	bool IncludesSelTracks();

// TODO these should be private
	char* m_cName;
	char* m_cNotes;
    int m_iSlot;
    int m_iMask;
	int m_time;
    
    WDL_PtrList<TrackSnapshot> m_tracks;
};
