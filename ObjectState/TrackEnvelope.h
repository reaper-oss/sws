/******************************************************************************
/ TrackEnvelope.h
/
/ Copyright (c) 2009 Tim Payne (SWS)
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

#ifdef SWS_DEPRECEATED // BR: use BR_Envelope and height stuff from BR_Util.h instead

#define ENV_HEIGHT_MULTIPLIER 0.75
#define ENV_MAX_SIZE 16384

class SWS_TrackEnvelope
{
public:
	SWS_TrackEnvelope(TrackEnvelope* te);
	~SWS_TrackEnvelope();
	void Load();
	int GetHeight(int iTrackHeight);
	void SetHeight(int iHeight);
	bool GetVis();
	void SetVis(bool bVis);
private:
	TrackEnvelope* m_pTe;
	int m_iHeightOverride;
	bool m_bVis;
	bool m_bLoaded;
	char* m_cEnv;
};

class SWS_TrackEnvelopes
{
public:
	SWS_TrackEnvelopes();
	~SWS_TrackEnvelopes();
	void SetTrack(MediaTrack* tr) { m_pTr = tr; }
	int GetLanesHeight(int iTrackHeight);
private:
	MediaTrack* m_pTr;
};

#endif
