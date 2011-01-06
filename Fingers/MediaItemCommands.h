#ifndef _MEDIA_ITEM_COMMANDS_H_
#define _MEDIA_ITEM_COMMANDS_H_

#include <list>
#include <algorithm>
#include <string>

#include "CommandHandler.h"

namespace MediaItemCommands {
	void Init();
};

class CmdExpandItems : public CReaperCommand
{
public:
	CmdExpandItems(double dAmount) : m_dAmount(dAmount)
	{}
private:
	void DoCommand(int flag);
	double m_dAmount;
};

class SetEditCursor : public CReaperCommand
{
public:
	enum ItemCursorPos { FirstSelected, FirstSelectedRight, ClosestSelected, ClosestSelectedRight};

	SetEditCursor(ItemCursorPos pos) : m_pos(pos)
	{}
	virtual bool LeaveEditCursorAlone() {return true;}
private:
	
	virtual void DoCommand(int flag);
	ItemCursorPos m_pos;
};

class CmdRotateItems : public CReaperCommand
{
public:
	CmdRotateItems(bool bRotateLengths, bool reverse) : 
	m_bRotateLengths(bRotateLengths), m_bReverse(reverse)
	{}
private:
		virtual void DoCommand(int flag);
		bool m_bRotateLengths;
		bool m_bReverse;
};

class CmdExpandItemsToBar: public CReaperCommand
{
public:
	CmdExpandItemsToBar(int nBars) : m_nBars(nBars)
	{}
private:
	virtual void DoCommand(int flag);
	int m_nBars;
};


class CmdSelectAllItemsOnTrackBetweenLoopPoints: public CReaperCommand
{
public:
	CmdSelectAllItemsOnTrackBetweenLoopPoints(bool allInLoop) {m_allInLoop = allInLoop;}
private:
	virtual void DoCommand(int flag);
	bool m_allInLoop;
};


class CmdPlaySelectedMediaItem: public CReaperCommand
{
public:
	CmdPlaySelectedMediaItem(bool onTrack, bool fromCursor, bool tillNextTransient) : 
	  m_OnTrack(onTrack), m_FromCursor(fromCursor), m_TillNextTrans(tillNextTransient), m_OnSelectedTrack(false)
	{
		InitializeCriticalSection(&m_PlayItem.cs); 
		m_PlayItem.src = 0;
		m_PlayItem.loop = false;
		m_PlayItem.curpos = 0.0;
	}
	virtual ~CmdPlaySelectedMediaItem()
	{
		if(m_PlayItem.src)
			delete m_PlayItem.src;
	}
	virtual void Stop()
	{
		if(m_OnTrack)
			StopTrackPreview(&m_PlayItem);
		else
			StopPreview(&m_PlayItem);
	}
	virtual void PlayThroughSelectedTrack()
	{
		m_OnSelectedTrack = true;
	}
private:
	virtual void DoCommand(int flag);
	preview_register_t m_PlayItem;
	bool m_OnTrack;
	bool m_FromCursor;
	bool m_TillNextTrans;
	bool m_OnSelectedTrack;
};

static void StopFNGPreview(CmdPlaySelectedMediaItem *p)
{
	p->Stop();
}

class CmdStopAllPreviews : public CReaperCommand
{
public:
	CmdStopAllPreviews() {}
	void RegisterPreview(CmdPlaySelectedMediaItem *preview)
	{
		m_previews.push_back(preview);
	}
private:
	virtual void DoCommand(int flag)
	{
		std::for_each(m_previews.begin(), m_previews.end(), StopFNGPreview);
	}
	std::vector<CmdPlaySelectedMediaItem *> m_previews;

};

class CmdInsertMidiNote : public CReaperCommand
{
public:
	CmdInsertMidiNote() {}
private:
	virtual void DoCommand(int flag);

};

class CmdPitchUpMidi : public CReaperCommand
{
public:
	CmdPitchUpMidi(int amt) {m_pitchAmt = amt;}
private:
	virtual void DoCommand(int flag);
	int m_pitchAmt;

};

class CmdVelChangeMidi : public CReaperCommand
{
public:
	CmdVelChangeMidi(int amt) {m_velAmt = amt;}
private:
	virtual void DoCommand(int flag);
	int m_velAmt;
};

class CmdAddToColourComponents  : public CReaperCommand
{
public:
	CmdAddToColourComponents(char r, char g, char b) {m_r = r; m_g = g; m_b = b;}
private:
	virtual void DoCommand(int flag);
	char m_r;
	char m_g;
	char m_b;
};

class CmdSetItemNameMidi : public CReaperCommand
{
public:
	CmdSetItemNameMidi(bool trackMidiName) {m_trackMidiName = trackMidiName;}
private:
	virtual void DoCommand(int flag);
	bool m_trackMidiName;

};

class CmdIncreaseItemRate : public CReaperCommand
{
public:
	CmdIncreaseItemRate(double amt) {m_amt = amt;}
private:
	virtual void DoCommand(int flag);
	double m_amt;
};

class CmdRemoveAllMarkersWithPrefix : public CReaperCommand
{
public:
	CmdRemoveAllMarkersWithPrefix(std::string prefix) {m_prefix = prefix;}
private:
	virtual void DoCommand(int flag);
	std::string m_prefix;
};


class CmdDeselectIfNotStartInTimeSelection : public CReaperCommand
{
public:
	CmdDeselectIfNotStartInTimeSelection() {}
private:
	virtual void DoCommand(int flag);
};

#endif /* _MEDIA_ITEM_COMMANDS_H_ */