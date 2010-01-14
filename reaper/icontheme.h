#ifndef _ICONTHEME_
#define _ICONTHEME_

#include "../../WDL/lice/lice.h"
#include "../../WDL/wingui/virtwnd-skin.h"

#define GUI_PRERENDER_ITEMS 1

typedef struct
{
  // NOTE: DO NOT REMOVE/INSERT IN THIS STRUCT. ONLY ADD.
  WDL_VirtualIconButton_SkinConfig toolbar_save,toolbar_open,toolbar_revert,toolbar_new,toolbar_projectsettings,toolbar_undo,toolbar_redo;
  WDL_VirtualIconButton_SkinConfig toolbar_lock[2], toolbar_envitemlock[2], toolbar_grid[2], 
         toolbar_snap[2], toolbar_ripple[3],  toolbar_group[2], toolbar_xfade[2];
  WDL_VirtualIconButton_SkinConfig toolbar_metro[2];


  WDL_VirtualIconButton_SkinConfig transport_rec[2], transport_play[2], transport_pause[2], transport_repeat[2];
  WDL_VirtualIconButton_SkinConfig track_envmode[5];
  WDL_VirtualIconButton_SkinConfig transport_rew, transport_fwd, transport_stop, transport_play_sync[2],transport_rec_loop[2],transport_rec_item[2];

  WDL_VirtualIconButton_SkinConfig mixer_menubutton;

  WDL_VirtualIconButton_SkinConfig track_io, track_phase[2];
  WDL_VirtualIconButton_SkinConfig track_monostereo[2];
  WDL_VirtualIconButton_SkinConfig track_fx[3];
  WDL_VirtualIconButton_SkinConfig track_fxcfg_tcp[3];
  WDL_VirtualIconButton_SkinConfig track_fxcfg_mcp[3];

  WDL_VirtualIconButton_SkinConfig track_monitor[3];
  WDL_VirtualIconButton_SkinConfig track_recarm[4]; // 0=off, 1=on, 2=auto when selected, 3=autowhenselected, on

  WDL_VirtualIconButton_SkinConfig track_mute[2], track_solo[2];
  WDL_VirtualIconButton_SkinConfig track_folder[3];
  WDL_VirtualIconButton_SkinConfig track_foldercomp[3];
  WDL_VirtualIconButton_SkinConfig track_recmode[3];

  WDL_VirtualIconButton_SkinConfig item_fx[2], item_lock[2], item_notes[2];

  // NOTE: DO NOT REMOVE/INSERT IN THIS STRUCT. ONLY ADD.
  WDL_VirtualIconButton_SkinConfig mcp_io, mcp_phase[2];
  WDL_VirtualIconButton_SkinConfig mcp_monostereo[2];
  WDL_VirtualIconButton_SkinConfig mcp_fx[3];
  WDL_VirtualIconButton_SkinConfig mcp_monitor[3];
  WDL_VirtualIconButton_SkinConfig mcp_recarm[4];
  WDL_VirtualIconButton_SkinConfig mcp_recmode[3];
  WDL_VirtualIconButton_SkinConfig mcp_mute[2], mcp_solo[2];
  WDL_VirtualIconButton_SkinConfig mcp_folder;

  LICE_IBitmap *classic_fader_bitmap_h, *classic_fader_bitmap_v;

  WDL_VirtualWnd_BGCfg transport_bg;
  WDL_VirtualWnd_BGCfg tcp_bg[2];
  WDL_VirtualWnd_BGCfg mcp_bg[2];
  WDL_VirtualWnd_BGCfg mcp_mainbg[2];
  WDL_VirtualWnd_BGCfg tcp_mainbg[2];
  WDL_VirtualWnd_BGCfg tcp_folderbg[2];
  WDL_VirtualWnd_BGCfg mcp_folderbg[2];

  WDL_VirtualWnd_BGCfg envcp_bg[2];
  WDL_VirtualWnd_BGCfg envcp_namebg;
  WDL_VirtualIconButton_SkinConfig envcp_hide, envcp_learn, envcp_parammod, envcp_totrack, envcp_arm[2];

  WDL_VirtualWnd_BGCfg toolbar_bg;

  WDL_VirtualWnd_BGCfg tcp_namebg, mcp_namebg;
  WDL_VirtualWnd_BGCfg tcp_mainnamebg, mcp_mainnamebg;
  // NOTE: DO NOT REMOVE/INSERT IN THIS STRUCT. ONLY ADD.

  WDL_VirtualWnd_BGCfg item_bg[2]; // see also prerendered_itembg below
  WDL_VirtualWnd_BGCfg item_loop;

  WDL_VirtualWnd_BGCfg folder_images[3]; // start, indent, end

  WDL_VirtualWnd_BGCfg piano_white_keys[2], piano_black_keys[2]; // [1] = pressed
 
  LICE_IBitmap *scrollbar_bmp;

  LICE_IBitmap *tcp_vu, *mcp_vu, *mcp_master_vu;

  // be sure to add any WDL_VirtualSlider_SkinConfig to the list of WDL_VirtualSlider_PreprocessSkinConfig calls
  WDL_VirtualSlider_SkinConfig tcp_volfader, tcp_panfader, mcp_volfader, mcp_panfader, 
    transport_masterspeedfader, envcp_fader;

  bool mcp_altmeterpos;
  bool tcp_showborders;
  bool mcp_showborders;
  bool transport_showborders;
  int tcp_vupeakwidth;
  int mcp_vupeakheight;
  int mcp_mastervupeakheight;

  LICE_IBitmap* midi_note_colormap;
  LICE_IBitmap* unused;  // available for next bitmap we add

  WDL_VirtualIconButton_SkinConfig transport_prev, transport_next;

  WDL_VirtualIconButton_SkinConfig global_auto[5];  // global automation state buttons
  WDL_VirtualIconButton_SkinConfig global_auto_off;
  WDL_VirtualIconButton_SkinConfig global_auto_bypass;

 // LICE_IBitmap* item_sel_label; // unused
 // LICE_IBitmap* item_mute_label;  // unused
 // LICE_IBitmap* item_group_label; // unused

  WDL_VirtualIconButton_SkinConfig item_mute[2], item_group[2];

  WDL_VirtualIconButton_SkinConfig envcp_bypass[2];

  WDL_VirtualIconButton_SkinConfig toolbar_custom[32];  // these are loaded only when needed

  WDL_VirtualIconButton_SkinConfig mcp_envmode[5];


  WDL_VirtualWnd_BGCfg tcp_fxparm_bg, mcp_fxparm_bg;

  WDL_VirtualWnd_BGCfg tcp_fxparm_norm, 
               tcp_fxparm_byp, 
               tcp_fxparm_off, 
               tcp_fxparm_empty, 
               tcp_fxparm_arrows_lr, 
               tcp_fxparm_arrows_ud;

  WDL_VirtualWnd_BGCfg mcp_fxparm_norm, 
               mcp_fxparm_byp, 
               mcp_fxparm_off, 
               mcp_fxparm_empty, 
               mcp_fxparm_arrows;

  
  WDL_VirtualWnd_BGCfg mcp_sendlist_bg, mcp_fxlist_bg, mcp_master_fxlist_bg;

  WDL_VirtualWnd_BGCfg mcp_fxlist_norm, mcp_fxlist_byp, mcp_fxlist_off, mcp_fxlist_empty;
  WDL_VirtualWnd_BGCfg mcp_master_fxlist_norm, mcp_master_fxlist_byp, mcp_master_fxlist_off, mcp_master_fxlist_empty;
  WDL_VirtualWnd_BGCfg mcp_sendlist_norm, mcp_sendlist_mute, mcp_sendlist_empty;
  WDL_VirtualWnd_BGCfg mcp_sendlist_arrows;
  WDL_VirtualWnd_BGCfg mcp_fxlist_arrows, mcp_master_fxlist_arrows;

  LICE_IBitmap *mcp_sendlist_knob, *mcp_sendlist_meter;


  LICE_IBitmap  *tcp_fxparm_knob;
  LICE_IBitmap  *mcp_fxparm_knob;

  // 3-wide bitmaps, but not iconbuttons
  LICE_IBitmap* midiinline_close;
  LICE_IBitmap* midiinline_noteview[3];
  LICE_IBitmap* midiinline_fold[3];
  LICE_IBitmap* midiinline_ccwithitems[2];
  LICE_IBitmap* midiinline_scroll;
  WDL_VirtualWnd_BGCfg midiinline_scrollbar;
  WDL_VirtualWnd_BGCfg midiinline_scrollthumb;

  WDL_VirtualIconButton_SkinConfig toolbar_dock[2];

  WDL_VirtualWnd_BGCfg mcp_master_sendlist_bg;
  WDL_VirtualWnd_BGCfg mcp_master_sendlist_norm, mcp_master_sendlist_mute, mcp_master_sendlist_empty;
  WDL_VirtualWnd_BGCfg mcp_master_sendlist_arrows;
  LICE_IBitmap *mcp_master_sendlist_knob, *mcp_master_sendlist_meter;

  WDL_VirtualWnd_BGCfg prerendered_itembg[8];  // [(ismutedtrack?4:0) + (itemsel?2:0) + (whichtrack&1)] ... only used if GUI_PRERENDER_ITEMS==1

  WDL_VirtualIconButton_SkinConfig envcp_parammod_on;
  WDL_VirtualIconButton_SkinConfig envcp_learn_on;

  WDL_VirtualIconButton_SkinConfig track_io_dis, mcp_io_dis;

  WDL_VirtualIconButton_SkinConfig toolbar_relsnap[2];
  // NOTE: DO NOT REMOVE/INSERT IN THIS STRUCT. ONLY ADD. (at least after 3.0)

  WDL_VirtualIconButton_SkinConfig item_props[2];

  WDL_VirtualWnd_BGCfg tab_up[2];
  WDL_VirtualWnd_BGCfg tab_down[2];

  WDL_VirtualIconButton_SkinConfig toolbar_filter[2];
  WDL_VirtualIconButton_SkinConfig toolbar_add, toolbar_delete;
  WDL_VirtualIconButton_SkinConfig toolbar_sync[2];

} IconTheme;



#define NUM_ENVELOPE_COLORS 6+10
typedef struct
{
  // NOTE: DO NOT REMOVE/INSERT IN THIS STRUCT. ONLY ADD.
  LOGFONT timeline_font,  mediaitem_font, label_font, label_font2;
  int timeline_fontcolor;
  int mediaitem_fontcolor;
  int itembgcolor;
  int DEPRRECATED_itembgselcolor;
  int timeline_bgcolor;
  int timeline_selbgcolor;
  int itemfadescolor;
  int itemfadedgecolor;
  int envelopecolors[NUM_ENVELOPE_COLORS];
  int trackbgs[2];
  int DEPRECATED_trackbgsel[2];
  int peaks[2]; // body of peaks when not selected, even/odd
  int DEPRECATED_peaksel[2];
  int offlinetext;
  int seltrack, seltrack2;
  int cursor;
  int grid_lines[2];
  int marker;
  int item_grouphl;
  int region;
  int itembgsel[2];
  int DEPRECATED_peaksedgecolor; 

  int main_bg;
  int main_text;
  int main_3d[2];
  int main_editbk;
  // NOTE: DO NOT REMOVE/INSERT IN THIS STRUCT. ONLY ADD.

  int timesigmarker;

  int vu_dointerlace;
  int vu_bottom;
  int vu_mid;
  int vu_top;
  int vu_clip;
  int vu_intcol;

  int DEPRECATED_fader_tint[2];

  int button_bg;

  int fader_armed_bg[2];

  int grid_startofbeat;

  int fader_armed_butnotrec;

  int mixerbg_color, tracklistbg_color;

  int main_textshadow;

  int midi_rulerbg;
  int midi_rulerfg;
  int midi_grid[3];
  int midi_trackbg[2];
  int midi_endpt;
  int midi_notebg;
  int midi_notefg;
  int midi_itemctls;
  int midi_editcursor;
  int midi_pkey[3];
  int DEPRECATED_midi_pkeytext;

  int docker_shadow; // defaults to GetSysColor(COLOR_3DDKSHADOW)
  int docker_selface;  // GSC_mainwnd(COLOR_3DFACE)
  int docker_unselface; // GSC_mainwnd(COLOR_3DSHADOW)
  int docker_text; //GSC_mainwnd(COLOR_BTNTEXT)
  int docker_bg; // Getsyscolor(COLOR_3DDKSHADOW)

  int mcp_fx_normal;
  int mcp_fx_bypassed;
  int mcp_fx_offlined;

  int mcp_sends_normal;
  int mcp_sends_muted;
  int mcp_send_levels;

  int env_mute_track, env_mute_sends;

  int peakssel2[2]; // body of peaks when selected, even/odd
  int itembgcolor2;
  int DEPRECATED_itembgselcolor2;
  int DEPRECATED_peaksedgecolor2;  

  int timeline_selbgcolor2;

  int trackpanel_text[2];

  int midi_offscreennote,midi_offscreennotesel;
  int env_item_vol,env_item_pan,env_item_mute;

  int group_colors[32];

  int marquee_fill,marquee_outline;

  int midi_notemute[2];

  int timesel_drawmode;
  int marquee_drawmode;
  int itembg_drawmode;

  int ruler_lane_bgcolor[3];  // region, marker, TS
  int ruler_lane_textcolor[3]; // region, marker, TS

  int mediaitem_fontcolor_sel;

  int mediaitem_fontcolor_floating[2]; 

  int peaksedge[2]; // edge of peaks when not selected, even/odd
  int peaksedge_sel[2]; // edge of peaks when selected, even/odd

  int midi_inline_trackbg[2];

  int vu_midi;

  int fadezone_drawmode;  // quiet zone
  int fadezone_color;     // quiet zone

  int cursor_altblink;

  int playcursor_drawmode;
  int playcursor_color;

  int itemfadedgecolor_drawmode;

  int guideline_color;
  int guideline_drawmode;

  int fadearea_drawmode;  // full area (not just quiet zone)
  int fadearea_color;     // full area (not just quiet zone)

  int midi_noteon_flash;

  int toolbar_button_text;

  int toolbararmed_color;
  int toolbararmed_drawmode;

  // NOTE: DO NOT REMOVE/INSERT IN THIS STRUCT. ONLY ADD.
} ColorTheme;

#define THEME_DRAWMODE_MODEMASK LICE_BLIT_MODE_MASK
#define THEME_DRAWMODE_ALPHAMASK 0x3FF00
#define THEME_DRAWMODE_ALPHAFROMVAL(x) (((max(-256,min(x,256))+0x200)&0x3FF)<<8)
#define THEME_DRAWMODE_GETALPHA(x) (((((x)>>8)&0x3FF) - 0x200)/256.0f)
#define THEME_DRAWMODE_ISNONZEROALPHA(x) ((((x)>>8)&0x3FF) != 0x200)

#define THEME_DRAWMODE_HSVROTATEADJUST 0xff

LICE_pixel ColorTheme_Blend(LICE_pixel dest, LICE_pixel themecol, int mode);
void ColorTheme_FillRect(LICE_IBitmap *bm, int l, int t, int w, int h, int lice_color, int drawmode);



#endif
