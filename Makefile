# use make or make DEBUG=1

CFLAGS = -pipe -fvisibility=hidden -fno-strict-aliasing -fno-math-errno -fPIC -DPIC -Wall -Wno-narrowing -Wno-sign-compare -Wno-unused-function
ifdef DEBUG
CFLAGS += -O0 -g 
else
CFLAGS += -O2 -s
endif

TARGET = reaper_sws64.so

ARCH := $(shell uname -m)

CFLAGS += -I. -I../WDL -DSWELL_PROVIDED_BY_APP 

ifneq ($(filter arm%,$(ARCH)),)
  CFLAGS += -fsigned-char -mfpu=vfp -march=armv6t2
endif

ifneq (x86_64, $(ARCH))
  TARGET = reaper_sws_$(ARCH).so
endif
  
CFLAGS += -DNO_TAGLIB  #taglib seems to be pre-compiled, need more code?
CXXFLAGS = $(CFLAGS)

LINKEXTRA = -lpthread

WDL_PATH = ../WDL/WDL
vpath swell-modstub-generic.cpp $(WDL_PATH)/swell
vpath lice%.cpp $(WDL_PATH)/lice
vpath png%.c $(WDL_PATH)/libpng
vpath %.c $(WDL_PATH)/zlib
vpath %.cpp $(WDL_PATH)/wingui $(WDL_PATH)/ $(WDL_PATH)/jnetlib/

WDL_OBJS           = swell-modstub-generic.o virtwnd.o virtwnd-slider.o virtwnd-iconbutton.o \
		     wndsize.o sha.o projectcontext.o \
		     jnetlib-util.o connection.o asyncdns.o httpget.o

LICE_OBJS          = lice_textnew.o lice.o lice_arc.o lice_line.o \
                     lice_png.o png.o pngerror.o pngget.o \
                     pngmem.o pngpread.o pngread.o pngrio.o \
                     pngrtran.o pngrutil.o pngset.o pngtrans.o \
                     adler32.o crc32.o infback.o inffast.o \
                     inflate.o inftrees.o uncompr.o zutil.o 

REAPER_OBJS        = reaper/reaper.o
SWS_OBJS           = sws_extension.o sws_about.o sws_util.o sws_waitdlg.o sws_wnd.o Menus.o Prompt.o ReaScript.o stdafx.o Zoom.o sws_util_generic.o
# DragDrop.o
AUTORENDER_OBJS    = Autorender/Autorender.o Autorender/RenderTrack.o
BREEDER_OBJS       = Breeder/BR_ContextualToolbars.o Breeder/BR_ContinuousActions.o Breeder/BR.o Breeder/BR_Envelope.o Breeder/BR_EnvelopeUtil.o \
                     Breeder/BR_Loudness.o Breeder/BR_MidiEditor.o Breeder/BR_MidiUtil.o Breeder/BR_Misc.o Breeder/BR_MouseUtil.o \
                     Breeder/BR_ProjState.o Breeder/BR_ReaScript.o Breeder/BR_Tempo.o Breeder/BR_TempoDlg.o Breeder/BR_Timer.o \
                     Breeder/BR_Update.o Breeder/BR_Util.o libebur128/ebur128.o
COLOR_OBJS         = Color/Autocolor.o Color/Color.o
CONSOLE_OBJS       = Console/Console.o
FINGERS_OBJS       = Fingers/CommandHandler.o Fingers/EnvelopeCommands.o Fingers/Envelope.o Fingers/FNG.o Fingers/GrooveCommands.o \
                     Fingers/GrooveDialog.o Fingers/GrooveTemplates.o Fingers/MediaItemCommands.o Fingers/MiscCommands.o Fingers/RprException.o Fingers/RprItem.o Fingers/MidiLaneCommands.o \
                     Fingers/RprMidiCCLane.o Fingers/RprMidiEvent.o Fingers/RprMidiTake.o Fingers/RprMidiTemplate.o Fingers/RprNode.o \
                     Fingers/RprStateChunk.o Fingers/RprTake.o Fingers/RprTrack.o Fingers/StringUtil.o Fingers/TimeMap.o
FREEZE_OBJS        = Freeze/ActiveTake.o Freeze/Freeze.o Freeze/ItemSelState.o Freeze/MuteState.o Freeze/TimeState.o Freeze/TrackItemState.o
# Freeze/MacroDebug.o
IX_OBJS            = IX/IX.o IX/Label.o IX/PlaylistImport.o
LIBEBUR_OBJS       = libebur128/ebur128.o
MARKERACTIONS_OBJS = MarkerActions/MarkerActions.o
MARKERLIST_OBJS    = MarkerList/MarkerListActions.o MarkerList/MarkerListClass.o MarkerList/MarkerList.o
MISC_OBJS          = Misc/Adam.o Misc/Analysis.o Misc/Context.o Misc/FolderActions.o Misc/ItemParams.o Misc/ItemSel.o Misc/Macros.o \
                     Misc/Misc.o Misc/ProjPrefs.o Misc/RecCheck.o Misc/TrackParams.o Misc/TrackSel.o Misc/EditCursor.o
OBJECTSTATE_OBJS   = ObjectState/ObjectState.o ObjectState/TrackEnvelope.o ObjectState/TrackFX.o ObjectState/TrackSends.o 
PADRE_OBJS         = Padre/padreActions.o Padre/padreEnvelopeProcessor.o Padre/padreMidiItemFilters.o Padre/padreMidiItemGenerators.o \
                     Padre/padreMidiItemProcBase.o Padre/padreUtils.o
# Padre/padreRmeTotalmix.o
PROJECT_OBJS       = Projects/ProjectList.o Projects/ProjectMgr.o
SNAPSHOTS_OBJS     = Snapshots/SnapshotClass.o Snapshots/SnapshotMerge.o Snapshots/Snapshots.o
SNOOKS_OBJS        = snooks/snooks.o snooks/SN_ReaScript.o
SNM_OBJS           = SnM/SnM.o SnM/SnM_Chunk.o SnM/SnM_CSurf.o SnM/SnM_CueBuss.o SnM/SnM_Cyclactions.o SnM/SnM_Dlg.o SnM/SnM_Find.o \
                     SnM/SnM_FXChain.o SnM/SnM_FX.o SnM/SnM_Item.o SnM/SnM_LiveConfigs.o SnM/SnM_Marker.o SnM/SnM_ME.o SnM/SnM_Misc.o \
                     SnM/SnM_Notes.o SnM/SnM_Project.o SnM/SnM_RegionPlaylist.o SnM/SnM_Resources.o SnM/SnM_Routing.o SnM/SnM_Track.o \
                     SnM/SnM_Util.o SnM/SnM_VWnd.o SnM/SnM_Window.o
TRACKLIST_OBJS     = TrackList/Tracklist.o TrackList/TracklistFilter.o
UTILITY_OBJS       = Utility/Base64.o
WOL_OBJS           = Wol/wol.o Wol/wol_Util.o Wol/wol_Zoom.o
XENAKIOS_OBJS      = Xenakios/AutoRename.o Xenakios/BroadCastWavCommands.o Xenakios/CommandRegistering.o Xenakios/CreateTrax.o \
                     Xenakios/DiskSpaceCalculator.o Xenakios/Envelope_actions.o Xenakios/ExoticCommands.o Xenakios/FloatingInspector.o \
                     Xenakios/fractions.o Xenakios/ItemTakeCommands.o Xenakios/main.o Xenakios/MediaDialog.o Xenakios/MixerActions.o \
                     Xenakios/MoreItemCommands.o Xenakios/Parameters.o Xenakios/PropertyInterpolator.o Xenakios/TakeRenaming.o \
                     Xenakios/TrackTemplateActions.o Xenakios/XenQueryDlg.o Xenakios/XenUtils.o

OBJS += $(WDL_OBJS) $(LICE_OBJS) $(REAPER_OBJS) $(SWS_OBJS) $(AUTORENDER_OBJS) $(BREEDER_OBJS) $(COLOR_OBJS) \
	$(CONSOLE_OBJS) $(FINGERS_OBJS) $(FREEZE_OBJS) $(IX_OBJS) $(LIBEBUR_OBS) $(MARKERACTIONS_OBJS) \
	$(MARKERLIST_OBJS) $(MISC_OBJS) $(PADRE_OBJS) $(PROJECT_OBJS) $(SNAPSHOTS_OBJS) $(SNM_OBJS) $(SNOOKS_OBJS)\
        $(TRACKLIST_OBJS) $(UTILITY_OBJS) $(WOL_OBJS) $(XENAKIOS_OBJS) $(OBJECTSTATE_OBJS) \
        nofish/nofish.o cfillion/cfillion.o

default: $(TARGET)

.PHONY: clean

reascript_vararg.h: ReaScript.cpp reascript_vararg.php
	php reascript_vararg.php > $@

sws_extension.rc_mac_dlg: sws_extension.rc
	php $(WDL_PATH)/swell/mac_resgen.php $^

sws_extension.o: sws_extension.cpp sws_extension.rc_mac_dlg
	$(CXX) -c -o $@ $(CXXFLAGS) sws_extension.cpp

jnetlib-util.o: $(WDL_PATH)/jnetlib/util.cpp
	$(CXX) -c -o $@ $(CXXFLAGS) $< 

$(TARGET): $(OBJS)
	$(CXX) -shared -o $@ $(CXXFLAGS) $(LFLAGS) $^ $(LINKEXTRA)

clean: 
	-rm $(OBJS) $(TARGET) sws_extension.rc_mac_dlg sws_extension.rc_mac_menu

install: $(TARGET)
	-mkdir ~/.REAPER/UserPlugins
	-rm ~/.REAPER/UserPlugins/$(TARGET)
	ln -sf $(shell pwd)/$(TARGET) ~/.REAPER/UserPlugins

uninstall:
	-rm ~/.REAPER/UserPlugins/$(TARGET)
