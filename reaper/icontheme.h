/************************************************
** Copyright (C) 2006-2012, Cockos Incorporated
*/

#ifndef _ICONTHEME_
#define _ICONTHEME_

#include "WDL/lice/lice.h"
#include "WDL/wingui/virtwnd-skin.h"


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

  LICE_IBitmap *vu_indicator; // used to be OLD_tcp_vu;
  
  LICE_IBitmap *OLD_mcp_vu, *OLD_mcp_master_vu;

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

  LICE_IBitmap *mcp_sendlist_knob, *OLD__mcp_sendlist_meter;


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
  LICE_IBitmap *mcp_master_sendlist_knob, *OLD__mcp_master_sendlist_meter;

  WDL_VirtualWnd_BGCfg DEPRECATED_prerendered_itembg[8];
 
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

  WDL_VirtualIconButton_SkinConfig mcp_folder_end;
  WDL_VirtualIconButton_SkinConfig mcp_foldercomp[3];  // [1] is not used at present

  WDL_VirtualIconButton_SkinConfig track_io_on[3];  // [0]=recv, [1]=send, [2]=both
  WDL_VirtualIconButton_SkinConfig track_io_on_dis[3];  // as above with master/parent disabled
  WDL_VirtualIconButton_SkinConfig mcp_io_on[3];  // like track_io_on
  WDL_VirtualIconButton_SkinConfig mcp_io_on_dis[3];  // like track_io_on_dis

  WDL_VirtualIconButton_SkinConfig tcp_solosafe;
  WDL_VirtualIconButton_SkinConfig mcp_solosafe;

  WDL_VirtualIconButton_SkinConfig transport_bpm;
  WDL_VirtualWnd_BGCfg transport_bpm_bg;
  WDL_VirtualWnd_BGCfg transport_group_bg;
  WDL_VirtualWnd_BGCfg transport_edit_bg;
  WDL_VirtualWnd_BGCfg transport_status_bg[2];  // 1=underrun

  WDL_VirtualWnd_BGCfg mcp_sendlist_midihw;

  int tcp_folderindent; // TRACK_TCP_FOLDERINDENTSIZE
  int tcp_supercollapsed_height; //SUPERCOLLAPSED_VIEW_SIZE
  int tcp_small_height; // collapsed, COLLAPSED_VIEW_SIZE
  int tcp_medium_height; // SMALLISH_SIZE
  int tcp_full_height; ///NORMAL_SIZE
  int tcp_master_min_height; //MIN_MASTER_SIZE 

  int envcp_min_height; // MIN_ENVCP_HEIGHT

  int mcp_min_height;

  int no_meter_reclbl;

  WDL_VirtualWnd_BGCfg tcp_recinput, mcp_recinput;

  WDL_VirtualWnd_BGCfg tcp_trackidx[2], mcp_trackidx[2];
  WDL_VirtualWnd_BGCfg tcp_mainnamebg_sel, mcp_mainnamebg_sel;

  WDL_VirtualIconButton_SkinConfig track_fx_in[2], mcp_fx_in[2];


  WDL_VirtualIconButton_SkinConfig gen_mute[2], gen_monostereo[2], gen_phase[2], gen_solo[2];
  WDL_VirtualIconButton_SkinConfig gen_envmode[5];
  WDL_VirtualIconButton_SkinConfig gen_play[2], gen_pause[2], gen_repeat[2], gen_rew, gen_fwd, gen_stop, gen_io;

  WDL_VirtualSlider_SkinConfig gen_volfader, gen_panfader, tcp_widthfader, mcp_widthfader;

  WDL_VirtualWnd_BGCfg tcp_iconbg[2][2], mcp_iconbg[2][2]; // area drawn behind track icon area [master][sel]
  WDL_VirtualWnd_BGCfg mcp_extmixbg[2][2]; // area drawn behind extended mixer [master][sel]

  WDL_VirtualWnd_BGCfg toosmall_r,toosmall_b;

  LICE_IBitmap *splash;

  WDL_VirtualWnd_BGCfg knob;
  WDL_VirtualWnd_BGCfg knob_sm;

  WDL_VirtualIconButton_SkinConfig gen_midi[2];

  WDL_VirtualWnd_BGCfg mcp_sendlist_meter_new,mcp_master_sendlist_meter_new;

  WDL_VirtualWnd_BGCfg mcp_sendlist_knob_bg, tcp_fxparm_knob_bg, mcp_fxparm_knob_bg;

  // NOTE: DO NOT REMOVE/INSERT IN THIS STRUCT. ONLY ADD.

  WDL_VirtualWnd_BGCfg tcp_vu_new, mcp_vu_new, mcp_master_vu_new;


  WDL_VirtualWnd_BGCfg mcp_arrowbuttons_lr,mcp_arrowbuttons_ud,tcp_arrowbuttons_lr,tcp_arrowbuttons_ud;

  WDL_VirtualIconButton_SkinConfig toolbar_quant[2];

  WDL_VirtualIconButton_SkinConfig item_pooled[2];

  WDL_VirtualWnd_BGCfg tcp_vol_knob, tcp_vol_knob_sm, tcp_pan_knob, tcp_pan_knob_sm, tcp_wid_knob, tcp_wid_knob_sm; 
  WDL_VirtualWnd_BGCfg mcp_vol_knob, mcp_vol_knob_sm, mcp_pan_knob, mcp_pan_knob_sm, mcp_wid_knob, mcp_wid_knob_sm; 

  WDL_VirtualWnd_BGCfg tcp_vol_label, tcp_pan_label, tcp_wid_label;
  WDL_VirtualWnd_BGCfg mcp_vol_label, mcp_pan_label, mcp_wid_label;
  WDL_VirtualWnd_BGCfg tcp_master_vol_label, tcp_master_pan_label, tcp_master_wid_label;
  WDL_VirtualWnd_BGCfg mcp_master_vol_label, mcp_master_pan_label, mcp_master_wid_label;

  char tcp_voltext_flags[2];
  char mcp_voltext_flags[2];
  char tcp_master_voltext_flags[2];
  char mcp_master_voltext_flags[2];

  LICE_IBitmap *toolbar_blank, *toolbar_overlay;

  WDL_VirtualWnd_BGCfg meter_mute, meter_automute, meter_unsolo, meter_solodim, meter_foldermute;

  WDL_VirtualWnd_BGCfg meter_bg_v,meter_bg_h, meter_ol_v,meter_ol_h; // applied only within meter bounds

  WDL_VirtualWnd_BGCfg meter_bg_mcp,meter_bg_tcp, meter_ol_mcp,meter_ol_tcp, meter_bg_mcp_master, meter_ol_mcp_master; // applied in meter bounds and outside

  LICE_IBitmap *meter_strip_v, *meter_strip_h, *meter_strip_v_rms;
  WDL_VirtualWnd_BGCfg meter_clip_v, meter_clip_h, meter_clip_v_rms, meter_clip_v_rms2;

} IconTheme;

void themeCompositeToolbarImage(LICE_IBitmap *img, bool postOnly=false);

// index: whichtrack&1 | sel?2 | inactivetake?4 | (trackmute?8 else itemmute?16 else lock?24)
extern WDL_VirtualWnd_BGCfg* g_prerendered_itembg[32];  

extern WDL_VirtualWnd_BGCfgCache *g_transport_bg_cache;
extern WDL_VirtualWnd_BGCfgCache *g_tcp_bg_cache; // normal [sel]
extern WDL_VirtualWnd_BGCfgCache *g_tcp_main_bg_cache; // master [sel]
extern WDL_VirtualWnd_BGCfgCache *g_tcp_folder_bg_cache; // folder [sel]
extern WDL_VirtualWnd_BGCfgCache *g_mcp_bg_cache; // normal [sel]
extern WDL_VirtualWnd_BGCfgCache *g_mcp_main_bg_cache; // master [sel]
extern WDL_VirtualWnd_BGCfgCache *g_mcp_folder_bg_cache; // folder [sel]

extern WDL_VirtualWnd_BGCfgCache *g_envcp_bg_cache;



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

  int track_divline[2];
  int envlane_divline[2];

  int mcp_sends_midihw;

  int tcp_locked_drawmode;
  int tcp_locked_color;

  int selitem_tag; // use only if 0x80000000
  int activetake_tag;  // use only if 0x80000000

  int midifont_col_dark;
  int midifont_col_light;

  int midioct;
  int midioct_inline;

  int midi_selbgcol;
  int midi_selbgmode;

  int arrange_bg;

  int mcp_fxparm_normal;
  int mcp_fxparm_bypassed;
  int mcp_fxparm_offlined;

  int env_item_pitch;

  int main_resize_color;

  LOGFONT transport_status_font;
  int transport_status_font_color;
  int transport_status_bg_color;
  
  // inherits from main_*
  int transport_editbk; 
  int main_bg2; // actual main window / transport bg
  int main_text2;  // actual main window / transport text

  int toolbar_button_text_on;
  int toolbar_frame;

  int vu_ind_very_bot;
  int vu_ind_bot;
  int vu_ind_mid;
  int vu_ind_top;

  int io_text; // overrides for i/o window
  int io_3d[2];
  
  int marqueezoom_fill,marqueezoom_outline,marqueezoom_drawmode;

//JFB found by reverse eng. --->
  int unknown_1[7];
  int genlist_bg;
  int genlist_fg;
  int genlist_gridlines;
  int genlist_sel[2];
  int genlist_selinactive[2];
//JFB <---

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
