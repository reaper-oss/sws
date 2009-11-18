/******************************************************************************
/ SnapshotClass.h
/
/ Copyright (c) 2009 Tim Payne (SWS)
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
    FXSnapshot(LineParser* lp);
    ~FXSnapshot();

	char* ItemString(char* str, int maxLen, bool* bDone);
    void RestoreParams(char* str);
    int UpdateReaper(MediaTrack* tr, bool* bMatched, int num);
	bool Exists(MediaTrack* tr);

    double* m_dParams;
    int m_iNumParams;
	int m_iCurParam;
    char m_cName[256];
};

class SendSnapshot
{
public:
    SendSnapshot(MediaTrack* tr, int send);
    SendSnapshot(LineParser* lp);
    ~SendSnapshot();

    char* ItemString(char* str, int maxLen);
    int UpdateReaper(MediaTrack* tr, bool* bMatched, int num);
	bool Exists(MediaTrack* tr);

    GUID m_dest;
    double m_dVol;
    double m_dPan;
    bool m_bMute;
    int m_iMode;
    int m_iSrcChan;
    int m_iDstChan;
    int m_iMidiFlags;
};


class TrackSnapshot
{
public:
    TrackSnapshot(MediaTrack* tr, int mask);
    TrackSnapshot(LineParser* lp);
    ~TrackSnapshot();

    char* ItemString(char* str, int maxLen);
    bool UpdateReaper(int mask, int* sendErr, int* fxErr, bool bSelOnly);
	bool Cleanup();
	MediaTrack* GetTrack();

	GUID m_guid;
    double m_dVol;
    double m_dPan;
    bool m_bMute;
    int m_iSolo;
    int m_iFXEn;
	int m_iVis;
	int m_iSel;
    WDL_PtrList<SendSnapshot> m_sends;
    WDL_PtrList<FXSnapshot> m_fx;
	WDL_String m_sFXChain;
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

class Snapshot
{
public:
    Snapshot(int slot, int mask, bool bSelOnly, char* name);   // For capture
    Snapshot(int slot, int mask, char* name, int time);        // For project load
    ~Snapshot();
    bool UpdateReaper(int mask, bool bSelOnly, bool bHideNewVis);
    char* Tooltip(char* str, int maxLen);
    void SetName(const char* name);
	void AddSelTracks();
	void DelSelTracks();
	void SelectTracks();
	int Find(MediaTrack* tr);
	static void RegisterGetCommand(int iSlot);

    char* m_cName;
    int m_iSlot;
    int m_iMask;
    int m_time;
    
    WDL_PtrList<TrackSnapshot> m_tracks;
};