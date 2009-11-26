/***************************************
*** REAPER Plug-in API
**
** Copyright (C) 2006-2008, Cockos Incorporated
**
**    This software is provided 'as-is', without any express or implied
**    warranty.  In no event will the authors be held liable for any damages
**    arising from the use of this software.
**
**    Permission is granted to anyone to use this software for any purpose,
**    including commercial applications, and to alter it and redistribute it
**    freely, subject to the following restrictions:
**
**    1. The origin of this software must not be misrepresented; you must not
**       claim that you wrote the original software. If you use this software
**       in a product, an acknowledgment in the product documentation would be
**       appreciated but is not required.
**    2. Altered source versions must be plainly marked as such, and must not be
**       misrepresented as being the original software.
**    3. This notice may not be removed or altered from any source distribution.
**
** Notes: the C++ interfaces used require MSVC on win32, or at least the MSVC-compatible C++ ABI. Sorry, mingw users :(
**
*/

#ifndef _REAPER_PLUGIN_H_
#define _REAPER_PLUGIN_H_


#define REASAMPLE_SIZE 8 // if we change this it will break everything!


#if REASAMPLE_SIZE == 4
typedef float ReaSample;
#else
typedef double ReaSample;
#endif



#ifdef _WIN32
#include <windows.h>

#define REAPER_PLUGIN_DLL_EXPORT __declspec(dllexport)
#define REAPER_PLUGIN_HINSTANCE HINSTANCE

#else
#include "../../WDL/swell/swell.h"
#include <pthread.h>

#define REAPER_PLUGIN_DLL_EXPORT __attribute__((visibility("default")))
#define REAPER_PLUGIN_HINSTANCE void *
#endif

#define REAPER_PLUGIN_ENTRYPOINT ReaperPluginEntry
#define REAPER_PLUGIN_ENTRYPOINT_NAME "ReaperPluginEntry"

#ifdef _MSC_VER
#define INT64 __int64
#define INT64_CONSTANT(x) (x##i64)
#else
#define INT64 long long
#define INT64_CONSTANT(x) (x##LL)
#endif

/* 
** Endian-tools and defines (currently only __ppc__ is recognized, for OS X -- all other platforms are assumed to be LE)
*/

#ifdef __ppc__
static int REAPER_MAKELEINT(int x)
{
  return ((((x))&0xff)<<24)|((((x))&0xff00)<<8)|((((x))&0xff0000)>>8)|((((x))&0xff000000)>>24);
}
static void REAPER_MAKELEINTMEM(void *buf)
{
  char *p=(char *)buf;
  char tmp=p[0]; p[0]=p[3]; p[3]=tmp;
  tmp=p[1]; p[1]=p[2]; p[2]=tmp;
}
static void REAPER_MAKELEINTMEM8(void *buf)
{
  char *p=(char *)buf;
  char tmp=p[0]; p[0]=p[7]; p[7]=tmp;
  tmp=p[1]; p[1]=p[6]; p[6]=tmp;
  tmp=p[2]; p[2]=p[5]; p[5]=tmp;
  tmp=p[3]; p[3]=p[4]; p[4]=tmp;
}
#define REAPER_FOURCC(d,c,b,a) (((unsigned int)(d)&0xff)|(((unsigned int)(c)&0xff)<<8)|(((unsigned int)(b)&0xff)<<16)|(((unsigned int)(a)&0xff)<<24))

#else

#define REAPER_FOURCC(a,b,c,d) (((unsigned int)(d)&0xff)|(((unsigned int)(c)&0xff)<<8)|(((unsigned int)(b)&0xff)<<16)|(((unsigned int)(a)&0xff)<<24))

#define REAPER_MAKELEINT(x) (x)
#define REAPER_MAKELEINTMEM(x)
#define REAPER_MAKELEINTMEM8(x)
#endif


#if 1
#define ADVANCE_TIME_BY_SAMPLES(t, spls, srate) ((t) += (spls)/(double)(srate))  // not completely accurate but still quite accurate.
#else
#define ADVANCE_TIME_BY_SAMPLES(t, spls, srate) ((t) = floor((t) * (srate) + spls + 0.5)/(double)(srate)) // good forever? may have other issues though if srate changes. disabled for now
#endif



//  int ReaperPluginEntry(HINSTANCE hInstance, reaper_plugin_info_t *rec);
//  return 1 if you are compatible (anything else will result in plugin being unloaded)
//  if rec == NULL, then time to unload 

#define REAPER_PLUGIN_VERSION 0x20E

typedef struct reaper_plugin_info_t
{
  int caller_version; // REAPER_PLUGIN_VERSION

  HWND hwnd_main;

  // this is the API that plug-ins register most things, be it keyboard shortcuts, project importers, etc.
  // typically you register things on load of the DLL, for example:

  // static pcmsink_register_t myreg={ ... };
  // rec->Register("pcmsink",&myreg);
 
  // then on plug-in unload (or if you wish to remove it for some reason), you should do:
  // rec->Register("-pcmsink",&myreg);
  // the "-" prefix is supported for most registration types.
  int (*Register)(const char *name, void *infostruct); // returns 1 if registered successfully

  // get a generic API function, there many of these defined.
  void * (*GetFunc)(const char *name); // returns 0 if function not found

} reaper_plugin_info_t;




/****************************************************************************************
**** interface for plugin objects to save/load state. they should use ../WDL/LineParser.h too...
***************************************************************************************/

class ProjectStateContext
{
public:
  virtual ~ProjectStateContext(){};

  virtual void AddLine(const char *fmt, ...)=0;
  virtual int GetLine(char *buf, int buflen)=0; // returns -1 on eof
};



/***************************************************************************************
**** MIDI event definition and abstract list
***************************************************************************************/

typedef struct
{
  int frame_offset;
  int size; // bytes used by midi_message, can be >3, but should never be <3, even if a short 1 or 2 byte msg
  unsigned char midi_message[4]; // size is number of bytes valid -- can be more than 4!
} MIDI_event_t;

class MIDI_eventlist
{
public:
  virtual void AddItem(MIDI_event_t *evt)=0; // puts the item in the correct place
  virtual MIDI_event_t *EnumItems(int *bpos)=0; 
  virtual void DeleteItem(int bpos)=0;
  virtual int GetSize()=0; // size of block in bytes
  virtual void Empty()=0;
};



/***************************************************************************************
**** PCM source API
***************************************************************************************/



typedef struct
{
  double time_s; // start time of block

  double samplerate; // desired output samplerate and channels
  int nch;

  int length; // desired length in sample(pair)s of output

  ReaSample *samples; // samples filled in (the caller allocates this)
  int samples_out; // updated with number of sample(pair)s actually rendered

  MIDI_eventlist *midi_events;

  double approximate_playback_latency; // 0.0 if not supported
  double absolute_time_s;
  double force_bpm;
} PCM_source_transfer_t;


typedef struct
{
  double start_time; // start time of block
  double peakrate;   // end time of block

  int numpeak_points; // desired number of points for data

  int nchpeaks; // number of channels of peaks data requested

  ReaSample *peaks;  // peaks output (caller allocated)
  int peaks_out; // number of points actually output (less than desired means at end)

  int output_mode; // 0=peaks, 1=waveform, 2=midi

  double absolute_time_s;

  ReaSample *peaks_minvals; // can be NULL, otherwise receives minimum values
  int peaks_minvals_used;

  int *exp[32]; // future use
} PCM_source_peaktransfer_t;


// used to update MIDI sources with new events during recording
typedef struct
{
  double global_time;
  double global_item_time;
  double srate;
  int length; // length in samples
  int overwritemode; // 0=overdub, 1=replace, -1 = literal (do nothing just add)
  MIDI_eventlist *events;
  double item_playrate;

  double latency;

} midi_realtime_write_struct_t;


// abstract base class
class PCM_source
{
  public:
    virtual ~PCM_source() { }

    virtual PCM_source *Duplicate()=0;

    virtual bool IsAvailable()=0;
    virtual void SetAvailable(bool avail) { } // optional, if called with avail=false, close files/etc, and so on
    virtual const char *GetType()=0;
    virtual const char *GetFileName() { return NULL; }; // return NULL if no filename (not purely a file)
    virtual bool SetFileName(const char *newfn)=0; // return TRUE if supported, this will only be called when offline

    virtual PCM_source *GetSource() { return NULL; }
    virtual void SetSource(PCM_source *src) { }
    virtual int GetNumChannels()=0; // return number of channels
    virtual double GetSampleRate()=0; // returns preferred sample rate. if < 1.0 then it is assumed to be silent (or MIDI)
    virtual double GetLength()=0; // length in seconds
    virtual double GetLengthBeats() { return -1.0; } // length in beats if supported
    virtual int GetBitsPerSample() { return 0; } // returns bits/sample, if available. only used for metadata purposes, since everything returns as doubles anyway
    virtual double GetPreferredPosition() { return -1.0; } // not supported returns -1

    virtual int PropertiesWindow(HWND hwndParent)=0;

    virtual void GetSamples(PCM_source_transfer_t *block)=0;
    virtual void GetPeakInfo(PCM_source_peaktransfer_t *block)=0;

    virtual void SaveState(ProjectStateContext *ctx)=0;
    virtual int LoadState(char *firstline, ProjectStateContext *ctx)=0; // -1 on error


    // these are called by the peaks building UI to build peaks for files.
    virtual void Peaks_Clear(bool deleteFile)=0;
    virtual int PeaksBuild_Begin()=0; // returns nonzero if building is opened, otherwise it may mean building isn't necessary
    virtual int PeaksBuild_Run()=0; // returns nonzero if building should continue
    virtual void PeaksBuild_Finish()=0; // called when done

    virtual int Extended(int call, void *parm1, void *parm2, void *parm3) { return 0; } // return 0 if unsupported
};

typedef struct
{
  int m_id;
  double m_time, m_endtime;
  bool m_isregion;
  char *m_name; // can be NULL if unnamed
  char resvd[128]; // future expansion -- should be 0
} REAPER_cue;

typedef struct
{
  PCM_source* m_sliceSrc;
  double m_beatSnapOffset;
  char resvd[128];  // future expansion -- should be 0
} REAPER_slice;


#define PCM_SOURCE_EXT_OPENEDITOR 0x10001 // parm1=hwnd, parm2=track idx, parm3=item description
#define PCM_SOURCE_EXT_GETEDITORSTRING 0x10002 // parm1=index (0 or 1), returns desc
// 0x10003 is deprecated
#define PCM_SOURCE_EXT_SETITEMCONTEXT 0x10004 // parm1=pointer to item handle, parm2=pointer to take handle
#define PCM_SOURCE_EXT_ADDMIDIEVENTS 0x10005 // parm1=pointer to midi_realtime_write_struct_t, nch=1 for replace, =0 for overdub, parm2=midi_quantize_mode_t* (optional)
#define PCM_SOURCE_EXT_GETASSOCIATED_RPP 0x10006 // parm1=pointer to char* that will receive a pointer to the string
#define PCM_SOURCE_EXT_GETMETADATA 0x10007 // parm1=pointer to name string, parm2=pointer to buffer, parm3=(int)buffersizemax . returns length used. Defined strings are "DESC", "ORIG", "ORIGREF", "DATE", "TIME", "UMID", "CODINGHISTORY" (i.e. BWF)
#define PCM_SOURCE_EXT_CONFIGISFILENAME 0x20000
#define PCM_SOURCE_EXT_GETBPMANDINFO 0x40000 // parm1=pointer to double for bpm. parm2=pointer to double for snap/downbeat offset (seconds).
#define PCM_SOURCE_EXT_GETNTRACKS 0x80000 // for midi data, returns number of tracks that would have been available
#define PCM_SOURCE_EXT_GETTITLE   0x80001
#define PCM_SOURCE_EXT_GETTEMPOMAP 0x80002
#define PCM_SOURCE_EXT_WANTOLDBEATSTYLE 0x80003
#define PCM_SOURCE_EXT_WANTTRIM 0x90002 // bla
#define PCM_SOURCE_EXT_TRIMITEM 0x90003 // parm1=lrflag, parm2=double *{position,length,startoffs,rate}
#define PCM_SOURCE_EXT_EXPORTTOFILE 0x90004 // parm1=output filename, only currently supported by MIDI but in theory any source could support this
#define PCM_SOURCE_EXT_ENUMCUES 0x90005 // parm1=(int) index of cue to get, parm2=REAPER_cue **. returns 0/sets parm2 to NULL when out of cues
// a PCM_source may be the parent of a number of beat-based slices, if so the parent should report length and nchannels only, handle ENUMSLICES, and be deleted after the slices are retrieved
#define PCM_SOURCE_EXT_ENUMSLICES 0x90006 // parm1=(int*) index of slice to get, parm2=REAPER_slice* (pointing to caller's existing slice struct). if parm2 passed in zero, returns the number of slices. returns 0 if no slices or out of slices. 
#define PCM_SOURCE_EXT_ENDPLAYNOTIFY 0x90007 // notify a source that it can release any pooled resources

// register with Register("pcmsrc",&struct ... and unregister with "-pcmsrc"
typedef struct {
  PCM_source *(*CreateFromType)(const char *type, int priority); // priority is 0-7, 0 is highest
  PCM_source *(*CreateFromFile)(const char *filename, int priority); // if priority is 5-7, and the file isn't found, open it in an offline state anyway, thanks

  // this is used for UI only, not so muc
  const char *(*EnumFileExtensions)(int i, char **descptr); // call increasing i until returns a string, if descptr's output is NULL, use last description
} pcmsrc_register_t;


/*
** OK so the pcm source class has a lot of responsibility, and people may not wish to
** have to implement them. So, for basic audio decoders, you can use this class instead:
**
** And you can use the PCM_Source_CreateFromSimple API to create a PCM_source from your ISimpleMediaDecoder derived class
*/
class ISimpleMediaDecoder
{
public:
  virtual ~ISimpleMediaDecoder() { }

  virtual ISimpleMediaDecoder *Duplicate()=0;

  // filename can be NULL to use "last filename"
  // diskread* are suggested values to pass to WDL_FileRead if you use it, otherwise can ignore
  virtual void Open(const char *filename, int diskreadmode, int diskreadbs, int diskreadnb)=0;

  // if fullClose=0, close disk resources, but can leave decoders etc initialized (and subsequently check the file date on re-open)
  virtual void Close(bool fullClose)=0; 

  virtual const char *GetFileName()=0;
  virtual const char *GetType()=0;

  // an info string suitable for a dialog, and a title for that dialog
  virtual void GetInfoString(char *buf, int buflen, char *title, int titlelen)=0; 

  virtual bool IsOpen()=0;
  virtual int GetNumChannels()=0;

  virtual int GetBitsPerSample()=0;
  virtual double GetSampleRate()=0;


  // positions in sample frames
  virtual INT64 GetLength()=0;
  virtual INT64 GetPosition()=0;
  virtual void SetPosition(INT64 pos)=0;

  // returns sample-frames read. buf will be at least length*GetNumChannels() ReaSamples long.
  virtual int ReadSamples(ReaSample *buf, int length)=0; 


  // these extended messages may include PCM_source messages
  virtual int Extended(int call, void *parm1, void *parm2, void *parm3) { return 0; } // return 0 if unsupported
};





/***************************************************************************************
**** PCM sink API
***************************************************************************************/

typedef struct
{
  bool doquant;
  char movemode; // 0=default(l/r), -1=left only, 1=right only
  char sizemode; // 0=preserve length, 1=quantize end
  char quantstrength; // 1-100
  double quantamt; // quantize to (in qn)
  char swingamt; // 1-100
  char range_min; // 0-100
  char range_max; 
} midi_quantize_mode_t;

class PCM_sink
{
  public:
    PCM_sink() { m_st=0.0; }
    virtual ~PCM_sink() { }

    virtual void GetOutputInfoString(char *buf, int buflen)=0;
    virtual double GetStartTime() { return m_st; }
    virtual void SetStartTime(double st) { m_st=st; }
    virtual const char *GetFileName()=0; // get filename, if applicable (otherwise "")
    virtual int GetNumChannels()=0; // return number of channels
    virtual double GetLength()=0; // length in seconds, so far
    virtual INT64 GetFileSize()=0;

    virtual void WriteMIDI(MIDI_eventlist *events, int len, double samplerate)=0;
    virtual void WriteDoubles(ReaSample **samples, int len, int nch, int offset, int spacing)=0;
    virtual bool WantMIDI() { return 0; }

    virtual int GetLastSecondPeaks(int sz, ReaSample *buf) { return 0; }
    virtual void GetPeakInfo(PCM_source_peaktransfer_t *block) { } // allow getting of peaks thus far

    virtual int Extended(int call, void *parm1, void *parm2, void *parm3) { return 0; } // return 0 if unsupported

  private:
    double m_st;
};

#define PCM_SINK_EXT_CREATESOURCE 0x80000 // parm1 = PCM_source ** (will be created if supported)
#define PCM_SINK_EXT_DONE 0x80001 // parm1 = HWND of the render dialog
#define PCM_SINK_EXT_VERIFYFMT 0x80002 // parm1=int *srate, parm2= int *nch. plugin can override (and return 1!!)
#define PCM_SINK_EXT_SETQUANT 0x80003 // parm1 = (midi_quantize_mode_t*), or NULL to disable
#define PCM_SINK_EXT_SETRATE 0x80004 // parm1 = (double *) rateadj
#define PCM_SINK_EXT_GETBITDEPTH 0x80005 // parm1 = (int*) bitdepth (return 1 if supported)
#define PCM_SINK_EXT_ADDCUE 0x80006 // parm1=(PCM_cue*)cue

typedef struct { // register using "pcmsink"
  unsigned int (*GetFmt)(char **desc);

  const char *(*GetExtension)(const void *cfg, int cfg_l);
  HWND (*ShowConfig)(const void *cfg, int cfg_l, HWND parent);
  PCM_sink *(*CreateSink)(const char *filename, void *cfg, int cfg_l, int nch, int srate, bool buildpeaks);

} pcmsink_register_t;



/***************************************************************************************
**** Resampler API (plug-ins can use this for SRC)
**
** 
** See API functions Resampler_Create() and Resample_EnumModes()
***************************************************************************************/

class REAPER_Resample_Interface
{
public:
  virtual ~REAPER_Resample_Interface(){}
  virtual void SetRates(double rate_in, double rate_out)=0;
  virtual void Reset()=0;

  virtual double GetCurrentLatency()=0; // latency in seconds buffered -- do not call between resampleprepare and resampleout, undefined if you do...
  virtual int ResamplePrepare(int out_samples, int nch, ReaSample **inbuffer)=0; // sample ratio
  virtual int ResampleOut(ReaSample *out, int nsamples_in, int nsamples_out, int nch)=0; // returns output samples

  virtual int Extended(int call, void *parm1, void *parm2, void *parm3) { return 0; } // return 0 if unsupported
};
#define RESAMPLE_EXT_SETRSMODE 0x1000 // parm1 == (int)resamplemode, or -1 for project default


/***************************************************************************************
**** Pitch shift API (plug-ins can use this for pitch shift/time stretch)
**
** See API calls ReaperGetPitchShiftAPI(), EnumPitchShiftModes(), EnumPitchShiftSubModes()
**
***************************************************************************************/

#define REAPER_PITCHSHIFT_API_VER 0x13

class IReaperPitchShift
{
  public:
    virtual ~IReaperPitchShift() { };
    virtual void set_srate(double srate)=0;
    virtual void set_nch(int nch)=0;
    virtual void set_shift(double shift)=0;
    virtual void set_formant_shift(double shift)=0; // shift can be <0 for "only shift when in formant preserve mode", so that you can use it for effective rate changes etc in that mode
    virtual void set_tempo(double tempo)=0;

    virtual void Reset()=0;  // reset all buffers/latency
    virtual ReaSample *GetBuffer(int size)=0;
    virtual void BufferDone(int input_filled)=0;

    virtual void FlushSamples()=0; // make sure all output is available

    virtual bool IsReset()=0;

    virtual int GetSamples(int requested_output, ReaSample *buffer)=0; // returns number of samplepairs returned

    virtual void SetQualityParameter(int parm)=0; // set to: (mode<<16)+(submode), or -1 for "project default" (default)
};


/***************************************************************************************
**** Peak getting/building API
**
** These are really only needed if you implement a PCM_source or PCM_sink.
**
** See functions PeakGet_Create(), PeakBuild_Create(), GetPeakFileName(), ClearPeakCache()
**
***************************************************************************************/


class REAPER_PeakGet_Interface
{
public:
  virtual ~REAPER_PeakGet_Interface() { }

  virtual double GetMaxPeakRes()=0;
  virtual void GetPeakInfo(PCM_source_peaktransfer_t *block)=0;

  virtual int Extended(int call, void *parm1, void *parm2, void *parm3) { return 0; } // return 0 if unsupported
};

class REAPER_PeakBuild_Interface
{
public:
  virtual ~REAPER_PeakBuild_Interface() { }

  virtual void ProcessSamples(ReaSample **samples, int len, int nch, int offs, int spread)=0; // in case a sink wants to build its own peaks (make sure it was created with src=NULL)
  virtual int Run()=0; // or let it do it automatically (created with source!=NULL)

  virtual int GetLastSecondPeaks(int sz, ReaSample *buf)=0; // returns number of peaks in the last second, sz is maxsize
  virtual void GetPeakInfo(PCM_source_peaktransfer_t *block)=0; // allow getting of peaks thus far (won't hit the highest resolution mipmap, just the 10/sec one or so)

  virtual int Extended(int call, void *parm1, void *parm2, void *parm3) { return 0; } // return 0 if unsupported
};

// recommended settings for sources switching to peak caches
#define REAPER_PEAKRES_MAX_NOPKS 200.0
#define REAPER_PEAKRES_MUL_MIN 0.00001 // recommended for plug-ins, when 1000peaks/pix, toss hires source
#define REAPER_PEAKRES_MUL_MAX 1.0 // recommended for plug-ins, when 1.5pix/peak, switch to hi res source. this may be configurable someday via some sneakiness



/*
** accelerator_register_t allows you to register ("accelerator") a record which lets you get a place in the 
** keyboard processing queue.
*/

typedef struct accelerator_register_t
{
  // translateAccel returns:
  // 0 if not our window, 
  // 1 to eat the keystroke, 
  // -1 to pass it on to the window, 
  // -666 to force it to the main window's accel table (with the exception of ESC)
  int (*translateAccel)(MSG *msg, accelerator_register_t *ctx); 
  bool isLocal; // must be TRUE, now (false is no longer supported, heh)
  void *user;
} accelerator_register_t;


/*
** gaccel_register_t allows you to register ("gaccel") an action into the main keyboard section action list, and at the same time
** a default binding for it (accel.cmd is the command ID, desc is the description, and accel's other parameters are the
** key to bind. 
*/

typedef struct 
{
  ACCEL accel; // key flags/etc represent default values (user may customize)
  const char *desc; // description (for user customizability)
} gaccel_register_t; // use "gaccel"

/*
** action_help_t lets you register help text ("action_help") for an action, mapped by action name
** (a "help" plugin could register help text for Reaper built-in actions)
*/

typedef struct
{
  const char* action_desc;
  const char* action_help;
} action_help_t;

/*
** editor_register_t lets you integrate editors for "open in external editor" for files directly.
*/

typedef struct // register with "editor"
{
  int (*editFile)(const char *filename, HWND parent, int trackidx); // return TRUE if handled for this file
  const char *(*wouldHandle)(const char *filename); // return your editor's description string

} editor_register_t;


/*
** Project import registration.
** 
** Implemented as a your-format->RPP converter, allowing you to generate directly to a ProjectStateContext
*/
typedef struct // register with "projectimport"
{
  bool (*WantProjectFile)(const char *fn); // is this our file?
  const char *(*EnumFileExtensions)(int i, char **descptr); // call increasing i until returns NULL. if descptr's output is NULL, use last description
  int (*LoadProject)(const char *fn, ProjectStateContext *genstate); // return 0=ok, Generate RPP compatible project info in genstate
} project_import_register_t;


typedef struct project_config_extension_t // register with "projectconfig"
{
  // plug-ins may or may not want to save their undo states (look at isUndo)
  // undo states will be saved if UNDO_STATE_MISCCFG is set (for adding your own undo points)
  bool (*ProcessExtensionLine)(const char *line, ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg); // returns BOOL if line (and optionally subsequent lines) processed (return false if not plug-ins line)
  void (*SaveExtensionConfig)(ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg);

  // optional: called on project load/undo before any (possible) ProcessExtensionLine. NULL is OK too
  // also called on "new project" (wont be followed by ProcessExtensionLine calls in that case)
  void (*BeginLoadProjectState)(bool isUndo, struct project_config_extension_t *reg); 

  void *userData;
} project_config_extension_t;



typedef struct audio_hook_register_t
{
  void (*OnAudioBuffer)(bool isPost, int len, double srate, struct audio_hook_register_t *reg); // called twice per frame, isPost being false then true
  void *userdata1;
  void *userdata2;

  // plug-in should zero these and they will be set by host
  // only call from OnAudioBuffer, nowhere else!!!
  int input_nch, output_nch; 
  ReaSample *(*GetBuffer)(bool isOutput, int idx); 

} audio_hook_register_t;

/*
** Allows you to get callback from the audio thread before and after REAPER's processing.
** register with Audio_RegHardwareHook()

  Note that you should be careful with this! :)

*/


/* 
** Customizable keyboard section definition etc
**
** Plug-ins may register keyboard action sections in by registering a "accel_section" to a KbdSectionInfo*.
*/

struct KbdAccel;
struct CommandAction;

typedef struct  
{
  DWORD cmd;  // action command ID
  const char *text; // description of action
} KbdCmd;

typedef struct
{
  int key;  // key identifier
  int cmd;  // action command ID
  int flags; // key flags
} KbdKeyBindingInfo;



typedef struct
{
  int uniqueID; // 0=main, < 0x10000000 for cockos use only plzkthx
  const char *name; // section name

  KbdCmd *action_list;   // list of assignable actions
  int action_list_cnt;

  KbdKeyBindingInfo *def_keys; // list of default key bindings
  int def_keys_cnt;

  // hwnd is 0 if MIDI etc. return false if ignoring
  bool (*onAction)(int cmd, int val, int valhw, int relmode, HWND hwnd);

  // this is allocated by the host not by the plug-in using it
  // the user can edit the list of actions/macros
#ifdef _WDL_PTRLIST_H_
  WDL_PtrList<struct KbdAccel> *accels;  
  WDL_PtrList<struct CommandAction> *recent_cmds;
#else
  void *accels;
  void *recent_cmds;
#endif

  void *extended_data[32]; // for internal use
} KbdSectionInfo;



typedef struct
{
/*
** Note: you must initialize/deinitialize the cs/mutex (depending on OS) manually, and use it if accessing most parameters while the preview is active.
*/

#ifdef _WIN32
  CRITICAL_SECTION cs;
#else
  pthread_mutex_t mutex;
#endif
  PCM_source *src;
  int m_out_chan; // &1024 means mono, low 10 bits are index of first channel
  double curpos;
  bool loop;
  double volume;

  double peakvol[2];
} preview_register_t;

/*
** preview_register_t is not used with the normal register system, instead it's used with PlayPreview(), StopPreview(), PlayTrackPreview(), StopTrackPreview()
*/


/*
** ColorTheme API access, these are used with GetColorTheme() and GetIconThemePointer();

// todo: document more of these:
*/

#define COLORTHEMEIDX_TIMELINEFG 0
#define COLORTHEMEIDX_ITEMTEXT 1
#define COLORTHEMEIDX_ITEMBG 2
#define COLORTHEMEIDX_TIMELINEBG 4
#define COLORTHEMEIDX_TIMELINESELBG 5
#define COLORTHEMEIDX_ITEMCONTROLS 6
#define COLORTHEMEIDX_TRACKBG1 24
#define COLORTHEMEIDX_TRACKBG2 25
#define COLORTHEMEIDX_PEAKS1 28
#define COLORTHEMEIDX_PEAKS2 29
#define COLORTHEMEIDX_EDITCURSOR 35
#define COLORTHEMEIDX_GRID1 36
#define COLORTHEMEIDX_GRID2 37
#define COLORTHEMEIDX_MARKER 38
#define COLORTHEMEIDX_REGION 40
#define COLORTHEMEIDX_GRID3 61
#define COLORTHEMEIDX_LOOPSELBG 100

#define COLORTHEMEIDX_ITEM_LOGFONT -2 // these return LOGFONT * as (int)
#define COLORTHEMEIDX_TL_LOGFONT -1


#define COLORTHEMEIDX_MIDI_TIMELINEBG 66
#define COLORTHEMEIDX_MIDI_TIMELINEFG 67
#define COLORTHEMEIDX_MIDI_GRID1 68
#define COLORTHEMEIDX_MIDI_GRID2 69
#define COLORTHEMEIDX_MIDI_GRID3 70
#define COLORTHEMEIDX_MIDI_TRACKBG1 71
#define COLORTHEMEIDX_MIDI_TRACKBG2 72
#define COLORTHEMEIDX_MIDI_ENDPT 73
#define COLORTHEMEIDX_MIDI_NOTEBG 74
#define COLORTHEMEIDX_MIDI_NOTEFG 75
#define COLORTHEMEIDX_MIDI_ITEMCONTROLS 76
#define COLORTHEMEIDX_MIDI_EDITCURSOR 77
#define COLORTHEMEIDX_MIDI_PKEY1 78
#define COLORTHEMEIDX_MIDI_PKEY2 79
#define COLORTHEMEIDX_MIDI_PKEY3 80
#define COLORTHEMEIDX_MIDI_PKEYTEXT 81
#define COLORTHEMEIDX_MIDI_OFFSCREENNOTE 103
#define COLORTHEMEIDX_MIDI_OFFSCREENNOTESEL 104



/*
** Screenset API
** 
*/

/*
  Note that "id" is a unique identifying string (usually a GUID etc) that is valid across 
  program runs (stored in project etc). lParam is instance-specific parameter (i.e. "this" pointer etc).
*/
enum
{
  SCREENSET_ACTION_GETHWND = 0,
  SCREENSET_ACTION_IS_DOCKED = 1,
  SCREENSET_ACTION_SHOW = 2, //param2 = dock status
  SCREENSET_ACTION_CLOSE = 3,
  SCREENSET_ACTION_SWITCH_DOCK = 4, //dock if undocked and vice-versa
  SCREENSET_ACTION_NOMOVE = 5, //return 1 if no move desired
  SCREENSET_ACTION_GETHASH = 6, //return hash string
};
typedef LRESULT (*screensetCallbackFunc)(int action, char *id, void *param, int param2);

// This are managed using screenset_register() etc


/*
** MIDI hardware device access.
**
*/


class midi_Output
{
public:
  virtual ~midi_Output() {}

  virtual void BeginBlock() { }  // outputs can implement these if they wish to have timed block sends
  virtual void EndBlock(int length, double srate, double curtempo) { }
  virtual void SendMsg(MIDI_event_t *msg, int frame_offset)=0; // frame_offset can be <0 for "instant" if supported
  virtual void Send(unsigned char status, unsigned char d1, unsigned char d2, int frame_offset)=0; // frame_offset can be <0 for "instant" if supported

};


class midi_Input
{
public:
  virtual ~midi_Input() {}

  virtual void start()=0;
  virtual void stop()=0;

  virtual void SwapBufs(unsigned int timestamp)=0;
  virtual void RunPreNoteTracking(int isAccum) { }

  virtual MIDI_eventlist *GetReadBuf()=0;
};



/*
** Control Surface API
*/


class MediaTrack;
class MediaItem;
class MediaItem_Take;
class ReaProject;
class TrackEnvelope;


class IReaperControlSurface
{
  public:
    IReaperControlSurface() { }
    virtual ~IReaperControlSurface() { }
    
    virtual const char *GetTypeString()=0; // simple unique string with only A-Z, 0-9, no spaces or other chars
    virtual const char *GetDescString()=0; // human readable description (can include instance specific info)
    virtual const char *GetConfigString()=0; // string of configuration data

    virtual void CloseNoReset() { } // close without sending "reset" messages, prevent "reset" being sent on destructor


    virtual void Run() { } // called 30x/sec or so.


    // these will be called by the host when states change etc
    virtual void SetTrackListChange() { }
    virtual void SetSurfaceVolume(MediaTrack *trackid, double volume) { }
    virtual void SetSurfacePan(MediaTrack *trackid, double pan) { }
    virtual void SetSurfaceMute(MediaTrack *trackid, bool mute) { }
    virtual void SetSurfaceSelected(MediaTrack *trackid, bool selected) { }
    virtual void SetSurfaceSolo(MediaTrack *trackid, bool solo) { }
    virtual void SetSurfaceRecArm(MediaTrack *trackid, bool recarm) { }
    virtual void SetPlayState(bool play, bool pause, bool rec) { }
    virtual void SetRepeatState(bool rep) { }
    virtual void SetTrackTitle(MediaTrack *trackid, const char *title) { }
    virtual bool GetTouchState(MediaTrack *trackid, int isPan) { return false; }
    virtual void SetAutoMode(int mode) { } // automation mode for current track

    virtual void ResetCachedVolPanStates() { } // good to flush your control states here

    virtual void OnTrackSelection(MediaTrack *trackid) { } // track was selected
    
    virtual bool IsKeyDown(int key) { return false; } // VK_CONTROL, VK_MENU, VK_SHIFT, etc, whatever makes sense for your surface

    virtual int Extended(int call, void *parm1, void *parm2, void *parm3) { return 0; } // return 0 if unsupported
};

typedef struct
{
  const char *type_string; // simple unique string with only A-Z, 0-9, no spaces or other chars
  const char *desc_string; // human readable description

  IReaperControlSurface *(*create)(const char *type_string, const char *configString, int *errStats); // errstats gets |1 if input error, |2 if output error
  HWND (*ShowConfig)(const char *type_string, HWND parent, const char *initConfigString); 
} reaper_csurf_reg_t; // register using "csurf"/"-csurf"

// note you can also add a control surface behind the scenes with "csurf_inst" (IReaperControlSurface*)instance



#ifndef UNDO_STATE_ALL
#define UNDO_STATE_ALL 0xFFFFFFFF
#define UNDO_STATE_TRACKCFG 1 // has track/master vol/pan/routing, ALL envelopes (matser included)
#define UNDO_STATE_FX 2  // track/master fx
#define UNDO_STATE_ITEMS 4  // track items
#define UNDO_STATE_MISCCFG 8 // loop selection, markers, regions, extensions!
#endif

// Menu hook
#define MAINMENU_FILE 0
#define MAINMENU_EDIT 1
#define MAINMENU_VIEW 2
#define MAINMENU_TRACK 3
#define MAINMENU_OPTIONS 4
#define MAINMENU_INSERT 5 
#define MAINMENU_EXT 6
#define CTXMENU_TCP 7
#define CTXMENU_ITEM 8
#define CTXMENU_ENV 9  
#define CTXMENU_ENVPT 10
#define CTXMENU_RULER 11

#endif//_REAPER_PLUGIN_H_
