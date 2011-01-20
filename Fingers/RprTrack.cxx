#include "stdafx.h"

#include "RprItem.hxx"
#include "RprTrack.hxx"
#include "sws_util.h"

class ReaProject;

extern int (*CountTracks)(ReaProject* proj);
extern MediaTrack* (*GetTrack)(ReaProject* proj, int trackidx);
extern GUID* (*GetTrackGUID)(MediaTrack* tr);
extern int (*CountSelectedTracks)(ReaProject* proj);
extern MediaTrack* (*GetSelectedTrack)(ReaProject* proj, int seltrackidx);
extern void* (*GetSetMediaTrackInfo)(MediaTrack* tr, const char* parmname, void* setNewValue);

// Get or set track attributes.
// P_PARTRACK : MediaTrack * : parent track (read-only)
// GUID : GUID * : 16-byte GUID, can query or update (do not use on master though)
// P_NAME : char * : track name (on master returns NULL)
// 
// B_MUTE : bool * : mute flag
// B_PHASE : bool * : invert track phase
// IP_TRACKNUMBER : int : track number (returns zero if not found, -1 for master track) (read-only, returns the int directly)
// I_SOLO : int * : 0=not soloed, 1=solo, 2=soloed in place
// I_FXEN : int * : 0=fx bypassed, nonzero = fx active
// I_RECARM : int * : 0=not record armed, 1=record armed
// I_RECINPUT : int * : record input. 0..n = mono hardware input, 512+n = rearoute input, 1024 set for stereo input pair. 4096 set for MIDI input, if set, then low 5 bits represent channel (0=all, 1-16=only chan), then next 5 bits represent physical input (31=all, 30=VKB)
// I_RECMODE : int * : record mode (0=input, 1=stereo out, 2=none, 3=stereo out w/latcomp, 4=midi output, 5=mono out, 6=mono out w/ lat comp, 7=midi overdub, 8=midi replace
// I_RECMON : int * : record monitor (0=off, 1=normal, 2=not when playing (tapestyle))
// I_RECMONITEMS : int * : monitor items while recording (0=off, 1=on)
// I_AUTOMODE : int * : track automation mode (0=trim/off, 1=read, 2=touch, 3=write, 4=latch
// I_NCHAN : int * : number of track channels, must be 2-64, even
// I_SELECTED : int * : track selected? 0 or 1
// I_WNDH : int * : current TCP window height (Read-only)
// I_FOLDERDEPTH : int * : folder depth change (0=normal, 1=track is a folder parent, -1=track is the last in the innermost folder, -2=track is the last in the innermost and next-innermost folders, etc
// I_FOLDERCOMPACT : int * : folder compacting (only valid on folders), 0=normal, 1=small, 2=tiny children
// I_MIDIHWOUT : int * : track midi hardware output index (<0 for disabled, low 5 bits are which channels (0=all, 1-16), next 5 bits are output device index (0-31))
// I_PERFFLAGS : int * : track perf flags (&1=no media buffering, &2=no anticipative FX)
// I_CUSTOMCOLOR : int * : custom color, windows standard color order (i.e. RGB(r,g,b)|0x100000). if you do not |0x100000, then it will not be used (though will store the color anyway)
// I_HEIGHTOVERRIDE : int * : custom height override for TCP window. 0 for none, otherwise size in pixels
// D_VOL : double * : trim volume of track (0 (-inf)..1 (+0dB) .. 2 (+6dB) etc ..)
// D_PAN : double * : trim pan of track (-1..1)
// D_PANLAW : double * : pan law of track. <0 for project default, 1.0 for +0dB, etc
// B_SHOWINMIXER : bool * : show track panel in mixer -- do not use on master
// B_SHOWINTCP : bool * : show track panel in tcp -- do not use on master
// B_MAINSEND : bool * : track sends audio to parent
// B_FREEMODE : bool * : track free-mode enabled (requires UpdateTimeline() after changing etc)
// C_BEATATTACHMODE : char * : char * to one char of beat attached mode, -1=def, 0=time, 1=allbeats, 2=beatsposonly
// F_MCP_FXSEND_SCALE : float * : scale of fx+send area in MCP (0.0=smallest allowed, 1=max allowed)
// F_MCP_SENDRGN_SCALE : float * : scale of send area as proportion of the fx+send total area (0=min allow, 1=max)

RprTrack::RprTrack(MediaTrack *track)
{
	mTrack = track;
}

MediaTrack* RprTrack::toReaper() const
{
	return mTrack;
}
GUID *RprTrack::getGUID()
{
	return GetTrackGUID(mTrack);
}

bool RprTrack::isMuted()
{
	return *(bool *)GetSetMediaTrackInfo(mTrack, "B_MUTE", NULL);
}

bool RprTrack::isSoloed()
{
	return *(int *)GetSetMediaTrackInfo(mTrack, "I_SOLO", NULL) > 0;
}

void RprTrack::setMuted(bool muted)
{
	GetSetMediaTrackInfo(mTrack, "B_MUTE", (void *)&muted);
}

void RprTrack::setSoloed(bool soloed)
{
	int solo = soloed ? 1 : 0;
	GetSetMediaTrackInfo(mTrack, "I_SOLO", (void *)&solo);
}

bool RprTrack::operator==(RprTrack &rhs)
{
	GUID *lhsGuid = getGUID();
	GUID *rhsGuid = rhs.getGUID();

	return GuidsEqual(lhsGuid,rhsGuid);
}

int RprTrack::getTrackIndex()
{
	return (int)GetSetMediaTrackInfo(mTrack, "IP_TRACKNUMBER", NULL);
}

const char *RprTrack::getName()
{
	return (const char *)GetSetMediaTrackInfo(mTrack, "P_NAME", NULL);
}

unsigned long RprTrack::getColour()
{
	unsigned int color = *(unsigned int *)GetSetMediaTrackInfo(mTrack, "I_CUSTOMCOLOR", NULL);
	//color &= (~0x100000);
	return color;
}

RprTrackCtrPtr RprTrackCollec::getSelected()
{
	int count = CountSelectedTracks(0);
	
	RprTrackCtrPtr ctr(new RprTrackCtr);
	for(int i = 0; i < count; i++) {
		RprTrack track(GetSelectedTrack(0, i));
		ctr->add(track);		
	}
	ctr->sort();
	return ctr;
}

RprTrackCtrPtr RprTrackCollec::getAll()
{
	int count = CountTracks(0);
	
	RprTrackCtrPtr ctr(new RprTrackCtr);
	for(int i = 0; i < count; i++) {
		RprTrack track(GetTrack(0, i));
		ctr->add(track);		
	}
	ctr->sort();
	return ctr;
}

