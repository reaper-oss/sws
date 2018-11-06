#ifndef _REASCRIPT_VARARG_H_
#define _REASCRIPT_VARARG_H_

static void* __vararg_SNM_CreateFastString(void** arglist, int numparms)
{
  return (void*)(INT_PTR)SNM_CreateFastString((const char*)arglist[0]);
}

static void* __vararg_SNM_DeleteFastString(void** arglist, int numparms)
{
  SNM_DeleteFastString((WDL_FastString*)arglist[0]);
  return NULL;
}

static void* __vararg_SNM_GetFastString(void** arglist, int numparms)
{
  return (void*)(INT_PTR)SNM_GetFastString((WDL_FastString*)arglist[0]);
}

static void* __vararg_SNM_GetFastStringLength(void** arglist, int numparms)
{
  return (void*)(INT_PTR)SNM_GetFastStringLength((WDL_FastString*)arglist[0]);
}

static void* __vararg_SNM_SetFastString(void** arglist, int numparms)
{
  return (void*)(INT_PTR)SNM_SetFastString((WDL_FastString*)arglist[0], (const char*)arglist[1]);
}

static void* __vararg_SNM_GetMediaItemTakeByGUID(void** arglist, int numparms)
{
  return (void*)(INT_PTR)SNM_GetMediaItemTakeByGUID((ReaProject*)arglist[0], (const char*)arglist[1]);
}

static void* __vararg_SNM_GetSourceType(void** arglist, int numparms)
{
  return (void*)(INT_PTR)SNM_GetSourceType((MediaItem_Take*)arglist[0], (WDL_FastString*)arglist[1]);
}

static void* __vararg_SNM_GetSetSourceState(void** arglist, int numparms)
{
  return (void*)(INT_PTR)SNM_GetSetSourceState((MediaItem*)arglist[0], (int)(INT_PTR)arglist[1], (WDL_FastString*)arglist[2], (bool)arglist[3]);
}

static void* __vararg_SNM_GetSetSourceState2(void** arglist, int numparms)
{
  return (void*)(INT_PTR)SNM_GetSetSourceState2((MediaItem_Take*)arglist[0], (WDL_FastString*)arglist[1], (bool)arglist[2]);
}

static void* __vararg_SNM_GetSetObjectState(void** arglist, int numparms)
{
  return (void*)(INT_PTR)SNM_GetSetObjectState((void*)arglist[0], (WDL_FastString*)arglist[1], (bool)arglist[2], (bool)arglist[3]);
}

static void* __vararg_SNM_AddReceive(void** arglist, int numparms)
{
  return (void*)(INT_PTR)SNM_AddReceive((MediaTrack*)arglist[0], (MediaTrack*)arglist[1], (int)(INT_PTR)arglist[2]);
}

static void* __vararg_SNM_RemoveReceive(void** arglist, int numparms)
{
  return (void*)(INT_PTR)SNM_RemoveReceive((MediaTrack*)arglist[0], (int)(INT_PTR)arglist[1]);
}

static void* __vararg_SNM_RemoveReceivesFrom(void** arglist, int numparms)
{
  return (void*)(INT_PTR)SNM_RemoveReceivesFrom((MediaTrack*)arglist[0], (MediaTrack*)arglist[1]);
}

static void* __vararg_SNM_GetIntConfigVar(void** arglist, int numparms)
{
  return (void*)(INT_PTR)SNM_GetIntConfigVar((const char*)arglist[0], (int)(INT_PTR)arglist[1]);
}

static void* __vararg_SNM_SetIntConfigVar(void** arglist, int numparms)
{
  return (void*)(INT_PTR)SNM_SetIntConfigVar((const char*)arglist[0], (int)(INT_PTR)arglist[1]);
}

static void* __vararg_SNM_GetDoubleConfigVar(void** arglist, int numparms)
{
  double* p =(double*)arglist[numparms-1];
  double d = SNM_GetDoubleConfigVar((const char*)arglist[0], arglist[1] ? *(double*)arglist[1] : 0.0);
  if (p) *p=d;
  return p;
}

static void* __vararg_SNM_SetDoubleConfigVar(void** arglist, int numparms)
{
  return (void*)(INT_PTR)SNM_SetDoubleConfigVar((const char*)arglist[0], arglist[1] ? *(double*)arglist[1] : 0.0);
}

static void* __vararg_SNM_MoveOrRemoveTrackFX(void** arglist, int numparms)
{
  return (void*)(INT_PTR)SNM_MoveOrRemoveTrackFX((MediaTrack*)arglist[0], (int)(INT_PTR)arglist[1], (int)(INT_PTR)arglist[2]);
}

static void* __vararg_SNM_GetProjectMarkerName(void** arglist, int numparms)
{
  return (void*)(INT_PTR)SNM_GetProjectMarkerName((ReaProject*)arglist[0], (int)(INT_PTR)arglist[1], (bool)arglist[2], (WDL_FastString*)arglist[3]);
}

static void* __vararg_SNM_SetProjectMarker(void** arglist, int numparms)
{
  return (void*)(INT_PTR)SNM_SetProjectMarker((ReaProject*)arglist[0], (int)(INT_PTR)arglist[1], (bool)arglist[2], arglist[3] ? *(double*)arglist[3] : 0.0, arglist[4] ? *(double*)arglist[4] : 0.0, (const char*)arglist[5], (int)(INT_PTR)arglist[6]);
}

static void* __vararg_SNM_SelectResourceBookmark(void** arglist, int numparms)
{
  return (void*)(INT_PTR)SNM_SelectResourceBookmark((const char*)arglist[0]);
}

static void* __vararg_SNM_TieResourceSlotActions(void** arglist, int numparms)
{
  SNM_TieResourceSlotActions((int)(INT_PTR)arglist[0]);
  return NULL;
}

static void* __vararg_SNM_AddTCPFXParm(void** arglist, int numparms)
{
  return (void*)(INT_PTR)SNM_AddTCPFXParm((MediaTrack*)arglist[0], (int)(INT_PTR)arglist[1], (int)(INT_PTR)arglist[2]);
}

static void* __vararg_SNM_TagMediaFile(void** arglist, int numparms)
{
  return (void*)(INT_PTR)SNM_TagMediaFile((const char*)arglist[0], (const char*)arglist[1], (const char*)arglist[2]);
}

static void* __vararg_SNM_ReadMediaFileTag(void** arglist, int numparms)
{
  return (void*)(INT_PTR)SNM_ReadMediaFileTag((const char*)arglist[0], (const char*)arglist[1], (char*)arglist[2], (int)(INT_PTR)arglist[3]);
}

static void* __vararg_FNG_AllocMidiTake(void** arglist, int numparms)
{
  return (void*)(INT_PTR)FNG_AllocMidiTake((MediaItem_Take*)arglist[0]);
}

static void* __vararg_FNG_FreeMidiTake(void** arglist, int numparms)
{
  FNG_FreeMidiTake((RprMidiTake*)arglist[0]);
  return NULL;
}

static void* __vararg_FNG_CountMidiNotes(void** arglist, int numparms)
{
  return (void*)(INT_PTR)FNG_CountMidiNotes((RprMidiTake*)arglist[0]);
}

static void* __vararg_FNG_GetMidiNote(void** arglist, int numparms)
{
  return (void*)(INT_PTR)FNG_GetMidiNote((RprMidiTake*)arglist[0], (int)(INT_PTR)arglist[1]);
}

static void* __vararg_FNG_GetMidiNoteIntProperty(void** arglist, int numparms)
{
  return (void*)(INT_PTR)FNG_GetMidiNoteIntProperty((RprMidiNote*)arglist[0], (const char*)arglist[1]);
}

static void* __vararg_FNG_SetMidiNoteIntProperty(void** arglist, int numparms)
{
  FNG_SetMidiNoteIntProperty((RprMidiNote*)arglist[0], (const char*)arglist[1], (int)(INT_PTR)arglist[2]);
  return NULL;
}

static void* __vararg_FNG_AddMidiNote(void** arglist, int numparms)
{
  return (void*)(INT_PTR)FNG_AddMidiNote((RprMidiTake*)arglist[0]);
}

static void* __vararg_BR_EnvAlloc(void** arglist, int numparms)
{
  return (void*)(INT_PTR)BR_EnvAlloc((TrackEnvelope*)arglist[0], (bool)arglist[1]);
}

static void* __vararg_BR_EnvCountPoints(void** arglist, int numparms)
{
  return (void*)(INT_PTR)BR_EnvCountPoints((BR_Envelope*)arglist[0]);
}

static void* __vararg_BR_EnvDeletePoint(void** arglist, int numparms)
{
  return (void*)(INT_PTR)BR_EnvDeletePoint((BR_Envelope*)arglist[0], (int)(INT_PTR)arglist[1]);
}

static void* __vararg_BR_EnvFind(void** arglist, int numparms)
{
  return (void*)(INT_PTR)BR_EnvFind((BR_Envelope*)arglist[0], arglist[1] ? *(double*)arglist[1] : 0.0, arglist[2] ? *(double*)arglist[2] : 0.0);
}

static void* __vararg_BR_EnvFindNext(void** arglist, int numparms)
{
  return (void*)(INT_PTR)BR_EnvFindNext((BR_Envelope*)arglist[0], arglist[1] ? *(double*)arglist[1] : 0.0);
}

static void* __vararg_BR_EnvFindPrevious(void** arglist, int numparms)
{
  return (void*)(INT_PTR)BR_EnvFindPrevious((BR_Envelope*)arglist[0], arglist[1] ? *(double*)arglist[1] : 0.0);
}

static void* __vararg_BR_EnvFree(void** arglist, int numparms)
{
  return (void*)(INT_PTR)BR_EnvFree((BR_Envelope*)arglist[0], (bool)arglist[1]);
}

static void* __vararg_BR_EnvGetParentTake(void** arglist, int numparms)
{
  return (void*)(INT_PTR)BR_EnvGetParentTake((BR_Envelope*)arglist[0]);
}

static void* __vararg_BR_EnvGetParentTrack(void** arglist, int numparms)
{
  return (void*)(INT_PTR)BR_EnvGetParentTrack((BR_Envelope*)arglist[0]);
}

static void* __vararg_BR_EnvGetPoint(void** arglist, int numparms)
{
  return (void*)(INT_PTR)BR_EnvGetPoint((BR_Envelope*)arglist[0], (int)(INT_PTR)arglist[1], (double*)arglist[2], (double*)arglist[3], (int*)arglist[4], (bool*)arglist[5], (double*)arglist[6]);
}

static void* __vararg_BR_EnvGetProperties(void** arglist, int numparms)
{
  BR_EnvGetProperties((BR_Envelope*)arglist[0], (bool*)arglist[1], (bool*)arglist[2], (bool*)arglist[3], (bool*)arglist[4], (int*)arglist[5], (int*)arglist[6], (double*)arglist[7], (double*)arglist[8], (double*)arglist[9], (int*)arglist[10], (bool*)arglist[11]);
  return NULL;
}

static void* __vararg_BR_EnvSetPoint(void** arglist, int numparms)
{
  return (void*)(INT_PTR)BR_EnvSetPoint((BR_Envelope*)arglist[0], (int)(INT_PTR)arglist[1], arglist[2] ? *(double*)arglist[2] : 0.0, arglist[3] ? *(double*)arglist[3] : 0.0, (int)(INT_PTR)arglist[4], (bool)arglist[5], arglist[6] ? *(double*)arglist[6] : 0.0);
}

static void* __vararg_BR_EnvSetProperties(void** arglist, int numparms)
{
  BR_EnvSetProperties((BR_Envelope*)arglist[0], (bool)arglist[1], (bool)arglist[2], (bool)arglist[3], (bool)arglist[4], (int)(INT_PTR)arglist[5], (int)(INT_PTR)arglist[6], (bool)arglist[7]);
  return NULL;
}

static void* __vararg_BR_EnvSortPoints(void** arglist, int numparms)
{
  BR_EnvSortPoints((BR_Envelope*)arglist[0]);
  return NULL;
}

static void* __vararg_BR_EnvValueAtPos(void** arglist, int numparms)
{
  double* p =(double*)arglist[numparms-1];
  double d = BR_EnvValueAtPos((BR_Envelope*)arglist[0], arglist[1] ? *(double*)arglist[1] : 0.0);
  if (p) *p=d;
  return p;
}

static void* __vararg_BR_GetArrangeView(void** arglist, int numparms)
{
  BR_GetArrangeView((ReaProject*)arglist[0], (double*)arglist[1], (double*)arglist[2]);
  return NULL;
}

static void* __vararg_BR_GetClosestGridDivision(void** arglist, int numparms)
{
  double* p =(double*)arglist[numparms-1];
  double d = BR_GetClosestGridDivision(arglist[0] ? *(double*)arglist[0] : 0.0);
  if (p) *p=d;
  return p;
}

static void* __vararg_BR_GetCurrentTheme(void** arglist, int numparms)
{
  BR_GetCurrentTheme((char*)arglist[0], (int)(INT_PTR)arglist[1], (char*)arglist[2], (int)(INT_PTR)arglist[3]);
  return NULL;
}

static void* __vararg_BR_GetMediaItemByGUID(void** arglist, int numparms)
{
  return (void*)(INT_PTR)BR_GetMediaItemByGUID((ReaProject*)arglist[0], (const char*)arglist[1]);
}

static void* __vararg_BR_GetMediaItemGUID(void** arglist, int numparms)
{
  BR_GetMediaItemGUID((MediaItem*)arglist[0], (char*)arglist[1], (int)(INT_PTR)arglist[2]);
  return NULL;
}

static void* __vararg_BR_GetMediaItemImageResource(void** arglist, int numparms)
{
  return (void*)(INT_PTR)BR_GetMediaItemImageResource((MediaItem*)arglist[0], (char*)arglist[1], (int)(INT_PTR)arglist[2], (int*)arglist[3]);
}

static void* __vararg_BR_GetMediaItemTakeGUID(void** arglist, int numparms)
{
  BR_GetMediaItemTakeGUID((MediaItem_Take*)arglist[0], (char*)arglist[1], (int)(INT_PTR)arglist[2]);
  return NULL;
}

static void* __vararg_BR_GetMediaSourceProperties(void** arglist, int numparms)
{
  return (void*)(INT_PTR)BR_GetMediaSourceProperties((MediaItem_Take*)arglist[0], (bool*)arglist[1], (double*)arglist[2], (double*)arglist[3], (double*)arglist[4], (bool*)arglist[5]);
}

static void* __vararg_BR_GetMediaTrackByGUID(void** arglist, int numparms)
{
  return (void*)(INT_PTR)BR_GetMediaTrackByGUID((ReaProject*)arglist[0], (const char*)arglist[1]);
}

static void* __vararg_BR_GetMediaTrackFreezeCount(void** arglist, int numparms)
{
  return (void*)(INT_PTR)BR_GetMediaTrackFreezeCount((MediaTrack*)arglist[0]);
}

static void* __vararg_BR_GetMediaTrackGUID(void** arglist, int numparms)
{
  BR_GetMediaTrackGUID((MediaTrack*)arglist[0], (char*)arglist[1], (int)(INT_PTR)arglist[2]);
  return NULL;
}

static void* __vararg_BR_GetMediaTrackLayouts(void** arglist, int numparms)
{
  BR_GetMediaTrackLayouts((MediaTrack*)arglist[0], (char*)arglist[1], (int)(INT_PTR)arglist[2], (char*)arglist[3], (int)(INT_PTR)arglist[4]);
  return NULL;
}

static void* __vararg_BR_GetMediaTrackSendInfo_Envelope(void** arglist, int numparms)
{
  return (void*)(INT_PTR)BR_GetMediaTrackSendInfo_Envelope((MediaTrack*)arglist[0], (int)(INT_PTR)arglist[1], (int)(INT_PTR)arglist[2], (int)(INT_PTR)arglist[3]);
}

static void* __vararg_BR_GetMediaTrackSendInfo_Track(void** arglist, int numparms)
{
  return (void*)(INT_PTR)BR_GetMediaTrackSendInfo_Track((MediaTrack*)arglist[0], (int)(INT_PTR)arglist[1], (int)(INT_PTR)arglist[2], (int)(INT_PTR)arglist[3]);
}

static void* __vararg_BR_GetMidiSourceLenPPQ(void** arglist, int numparms)
{
  double* p =(double*)arglist[numparms-1];
  double d = BR_GetMidiSourceLenPPQ((MediaItem_Take*)arglist[0]);
  if (p) *p=d;
  return p;
}

static void* __vararg_BR_GetMidiTakePoolGUID(void** arglist, int numparms)
{
  return (void*)(INT_PTR)BR_GetMidiTakePoolGUID((MediaItem_Take*)arglist[0], (char*)arglist[1], (int)(INT_PTR)arglist[2]);
}

static void* __vararg_BR_GetMidiTakeTempoInfo(void** arglist, int numparms)
{
  return (void*)(INT_PTR)BR_GetMidiTakeTempoInfo((MediaItem_Take*)arglist[0], (bool*)arglist[1], (double*)arglist[2], (int*)arglist[3], (int*)arglist[4]);
}

static void* __vararg_BR_GetMouseCursorContext(void** arglist, int numparms)
{
  BR_GetMouseCursorContext((char*)arglist[0], (int)(INT_PTR)arglist[1], (char*)arglist[2], (int)(INT_PTR)arglist[3], (char*)arglist[4], (int)(INT_PTR)arglist[5]);
  return NULL;
}

static void* __vararg_BR_GetMouseCursorContext_Envelope(void** arglist, int numparms)
{
  return (void*)(INT_PTR)BR_GetMouseCursorContext_Envelope((bool*)arglist[0]);
}

static void* __vararg_BR_GetMouseCursorContext_Item(void** arglist, int numparms)
{
  return (void*)(INT_PTR)BR_GetMouseCursorContext_Item();
}

static void* __vararg_BR_GetMouseCursorContext_MIDI(void** arglist, int numparms)
{
  return (void*)(INT_PTR)BR_GetMouseCursorContext_MIDI((bool*)arglist[0], (int*)arglist[1], (int*)arglist[2], (int*)arglist[3], (int*)arglist[4]);
}

static void* __vararg_BR_GetMouseCursorContext_Position(void** arglist, int numparms)
{
  double* p =(double*)arglist[numparms-1];
  double d = BR_GetMouseCursorContext_Position();
  if (p) *p=d;
  return p;
}

static void* __vararg_BR_GetMouseCursorContext_StretchMarker(void** arglist, int numparms)
{
  return (void*)(INT_PTR)BR_GetMouseCursorContext_StretchMarker();
}

static void* __vararg_BR_GetMouseCursorContext_Take(void** arglist, int numparms)
{
  return (void*)(INT_PTR)BR_GetMouseCursorContext_Take();
}

static void* __vararg_BR_GetMouseCursorContext_Track(void** arglist, int numparms)
{
  return (void*)(INT_PTR)BR_GetMouseCursorContext_Track();
}

static void* __vararg_BR_GetNextGridDivision(void** arglist, int numparms)
{
  double* p =(double*)arglist[numparms-1];
  double d = BR_GetNextGridDivision(arglist[0] ? *(double*)arglist[0] : 0.0);
  if (p) *p=d;
  return p;
}

static void* __vararg_BR_GetPrevGridDivision(void** arglist, int numparms)
{
  double* p =(double*)arglist[numparms-1];
  double d = BR_GetPrevGridDivision(arglist[0] ? *(double*)arglist[0] : 0.0);
  if (p) *p=d;
  return p;
}

static void* __vararg_BR_GetSetTrackSendInfo(void** arglist, int numparms)
{
  double* p =(double*)arglist[numparms-1];
  double d = BR_GetSetTrackSendInfo((MediaTrack*)arglist[0], (int)(INT_PTR)arglist[1], (int)(INT_PTR)arglist[2], (const char*)arglist[3], (bool)arglist[4], arglist[5] ? *(double*)arglist[5] : 0.0);
  if (p) *p=d;
  return p;
}

static void* __vararg_BR_GetTakeFXCount(void** arglist, int numparms)
{
  return (void*)(INT_PTR)BR_GetTakeFXCount((MediaItem_Take*)arglist[0]);
}

static void* __vararg_BR_IsTakeMidi(void** arglist, int numparms)
{
  return (void*)(INT_PTR)BR_IsTakeMidi((MediaItem_Take*)arglist[0], (bool*)arglist[1]);
}

static void* __vararg_BR_IsMidiOpenInInlineEditor(void** arglist, int numparms)
{
  return (void*)(INT_PTR)BR_IsMidiOpenInInlineEditor((MediaItem_Take*)arglist[0]);
}

static void* __vararg_BR_ItemAtMouseCursor(void** arglist, int numparms)
{
  return (void*)(INT_PTR)BR_ItemAtMouseCursor((double*)arglist[0]);
}

static void* __vararg_BR_MIDI_CCLaneRemove(void** arglist, int numparms)
{
  return (void*)(INT_PTR)BR_MIDI_CCLaneRemove((HWND)arglist[0], (int)(INT_PTR)arglist[1]);
}

static void* __vararg_BR_MIDI_CCLaneReplace(void** arglist, int numparms)
{
  return (void*)(INT_PTR)BR_MIDI_CCLaneReplace((HWND)arglist[0], (int)(INT_PTR)arglist[1], (int)(INT_PTR)arglist[2]);
}

static void* __vararg_BR_PositionAtMouseCursor(void** arglist, int numparms)
{
  double* p =(double*)arglist[numparms-1];
  double d = BR_PositionAtMouseCursor((bool)arglist[0]);
  if (p) *p=d;
  return p;
}

static void* __vararg_BR_SetArrangeView(void** arglist, int numparms)
{
  BR_SetArrangeView((ReaProject*)arglist[0], arglist[1] ? *(double*)arglist[1] : 0.0, arglist[2] ? *(double*)arglist[2] : 0.0);
  return NULL;
}

static void* __vararg_BR_SetItemEdges(void** arglist, int numparms)
{
  return (void*)(INT_PTR)BR_SetItemEdges((MediaItem*)arglist[0], arglist[1] ? *(double*)arglist[1] : 0.0, arglist[2] ? *(double*)arglist[2] : 0.0);
}

static void* __vararg_BR_SetMediaItemImageResource(void** arglist, int numparms)
{
  BR_SetMediaItemImageResource((MediaItem*)arglist[0], (const char*)arglist[1], (int)(INT_PTR)arglist[2]);
  return NULL;
}

static void* __vararg_BR_SetMediaSourceProperties(void** arglist, int numparms)
{
  return (void*)(INT_PTR)BR_SetMediaSourceProperties((MediaItem_Take*)arglist[0], (bool)arglist[1], arglist[2] ? *(double*)arglist[2] : 0.0, arglist[3] ? *(double*)arglist[3] : 0.0, arglist[4] ? *(double*)arglist[4] : 0.0, (bool)arglist[5]);
}

static void* __vararg_BR_SetMediaTrackLayouts(void** arglist, int numparms)
{
  return (void*)(INT_PTR)BR_SetMediaTrackLayouts((MediaTrack*)arglist[0], (const char*)arglist[1], (const char*)arglist[2]);
}

static void* __vararg_BR_SetMidiTakeTempoInfo(void** arglist, int numparms)
{
  return (void*)(INT_PTR)BR_SetMidiTakeTempoInfo((MediaItem_Take*)arglist[0], (bool)arglist[1], arglist[2] ? *(double*)arglist[2] : 0.0, (int)(INT_PTR)arglist[3], (int)(INT_PTR)arglist[4]);
}

static void* __vararg_BR_SetTakeSourceFromFile(void** arglist, int numparms)
{
  return (void*)(INT_PTR)BR_SetTakeSourceFromFile((MediaItem_Take*)arglist[0], (const char*)arglist[1], (bool)arglist[2]);
}

static void* __vararg_BR_SetTakeSourceFromFile2(void** arglist, int numparms)
{
  return (void*)(INT_PTR)BR_SetTakeSourceFromFile2((MediaItem_Take*)arglist[0], (const char*)arglist[1], (bool)arglist[2], (bool)arglist[3]);
}

static void* __vararg_BR_TakeAtMouseCursor(void** arglist, int numparms)
{
  return (void*)(INT_PTR)BR_TakeAtMouseCursor((double*)arglist[0]);
}

static void* __vararg_BR_TrackAtMouseCursor(void** arglist, int numparms)
{
  return (void*)(INT_PTR)BR_TrackAtMouseCursor((int*)arglist[0], (double*)arglist[1]);
}

static void* __vararg_BR_TrackFX_GetFXModuleName(void** arglist, int numparms)
{
  return (void*)(INT_PTR)BR_TrackFX_GetFXModuleName((MediaTrack*)arglist[0], (int)(INT_PTR)arglist[1], (char*)arglist[2], (int)(INT_PTR)arglist[3]);
}

static void* __vararg_BR_Win32_GetPrivateProfileString(void** arglist, int numparms)
{
  return (void*)(INT_PTR)BR_Win32_GetPrivateProfileString((const char*)arglist[0], (const char*)arglist[1], (const char*)arglist[2], (const char*)arglist[3], (char*)arglist[4], (int)(INT_PTR)arglist[5]);
}

static void* __vararg_BR_Win32_ShellExecute(void** arglist, int numparms)
{
  return (void*)(INT_PTR)BR_Win32_ShellExecute((const char*)arglist[0], (const char*)arglist[1], (const char*)arglist[2], (const char*)arglist[3], (int)(INT_PTR)arglist[4]);
}

static void* __vararg_BR_Win32_WritePrivateProfileString(void** arglist, int numparms)
{
  return (void*)(INT_PTR)BR_Win32_WritePrivateProfileString((const char*)arglist[0], (const char*)arglist[1], (const char*)arglist[2], (const char*)arglist[3]);
}

static void* __vararg_ULT_GetMediaItemNote(void** arglist, int numparms)
{
  return (void*)(INT_PTR)ULT_GetMediaItemNote((MediaItem*)arglist[0]);
}

static void* __vararg_ULT_SetMediaItemNote(void** arglist, int numparms)
{
  ULT_SetMediaItemNote((MediaItem*)arglist[0], (const char*)arglist[1]);
  return NULL;
}

static void* __vararg_NF_GetMediaItemMaxPeak(void** arglist, int numparms)
{
  double* p =(double*)arglist[numparms-1];
  double d = NF_GetMediaItemMaxPeak((MediaItem*)arglist[0]);
  if (p) *p=d;
  return p;
}

static void* __vararg_NF_GetMediaItemMaxPeakAndMaxPeakPos(void** arglist, int numparms)
{
  double* p =(double*)arglist[numparms-1];
  double d = NF_GetMediaItemMaxPeakAndMaxPeakPos((MediaItem*)arglist[0], (double*)arglist[1]);
  if (p) *p=d;
  return p;
}

static void* __vararg_NF_GetMediaItemPeakRMS_Windowed(void** arglist, int numparms)
{
  double* p =(double*)arglist[numparms-1];
  double d = NF_GetMediaItemPeakRMS_Windowed((MediaItem*)arglist[0]);
  if (p) *p=d;
  return p;
}

static void* __vararg_NF_GetMediaItemPeakRMS_NonWindowed(void** arglist, int numparms)
{
  double* p =(double*)arglist[numparms-1];
  double d = NF_GetMediaItemPeakRMS_NonWindowed((MediaItem*)arglist[0]);
  if (p) *p=d;
  return p;
}

static void* __vararg_NF_GetMediaItemAverageRMS(void** arglist, int numparms)
{
  double* p =(double*)arglist[numparms-1];
  double d = NF_GetMediaItemAverageRMS((MediaItem*)arglist[0]);
  if (p) *p=d;
  return p;
}

static void* __vararg_NF_AnalyzeMediaItemPeakAndRMS(void** arglist, int numparms)
{
  return (void*)(INT_PTR)NF_AnalyzeMediaItemPeakAndRMS((MediaItem*)arglist[0], arglist[1] ? *(double*)arglist[1] : 0.0, (void*)arglist[2], (void*)arglist[3], (void*)arglist[4], (void*)arglist[5]);
}

static void* __vararg_NF_AnalyzeTakeLoudness_IntegratedOnly(void** arglist, int numparms)
{
  return (void*)(INT_PTR)NF_AnalyzeTakeLoudness_IntegratedOnly((MediaItem_Take*)arglist[0], (double*)arglist[1]);
}

static void* __vararg_NF_AnalyzeTakeLoudness(void** arglist, int numparms)
{
  return (void*)(INT_PTR)NF_AnalyzeTakeLoudness((MediaItem_Take*)arglist[0], (bool)arglist[1], (double*)arglist[2], (double*)arglist[3], (double*)arglist[4], (double*)arglist[5], (double*)arglist[6], (double*)arglist[7]);
}

static void* __vararg_NF_AnalyzeTakeLoudness2(void** arglist, int numparms)
{
  return (void*)(INT_PTR)NF_AnalyzeTakeLoudness2((MediaItem_Take*)arglist[0], (bool)arglist[1], (double*)arglist[2], (double*)arglist[3], (double*)arglist[4], (double*)arglist[5], (double*)arglist[6], (double*)arglist[7], (double*)arglist[8], (double*)arglist[9]);
}

static void* __vararg_NF_GetSWSTrackNotes(void** arglist, int numparms)
{
  return (void*)(INT_PTR)NF_GetSWSTrackNotes((MediaTrack*)arglist[0]);
}

static void* __vararg_NF_SetSWSTrackNotes(void** arglist, int numparms)
{
  NF_SetSWSTrackNotes((MediaTrack*)arglist[0], (const char*)arglist[1]);
  return NULL;
}

static void* __vararg_NF_GetSWSMarkerRegionSub(void** arglist, int numparms)
{
  return (void*)(INT_PTR)NF_GetSWSMarkerRegionSub((int)(INT_PTR)arglist[0]);
}

static void* __vararg_NF_SetSWSMarkerRegionSub(void** arglist, int numparms)
{
  return (void*)(INT_PTR)NF_SetSWSMarkerRegionSub((const char*)arglist[0], (int)(INT_PTR)arglist[1]);
}

static void* __vararg_NF_UpdateSWSMarkerRegionSubWindow(void** arglist, int numparms)
{
  NF_UpdateSWSMarkerRegionSubWindow();
  return NULL;
}

static void* __vararg_SN_FocusMIDIEditor(void** arglist, int numparms)
{
  SN_FocusMIDIEditor();
  return NULL;
}

static void* __vararg_CF_SetClipboard(void** arglist, int numparms)
{
  CF_SetClipboard((const char*)arglist[0]);
  return NULL;
}

static void* __vararg_CF_GetClipboard(void** arglist, int numparms)
{
  CF_GetClipboard((char*)arglist[0], (int)(INT_PTR)arglist[1]);
  return NULL;
}

static void* __vararg_CF_GetClipboardBig(void** arglist, int numparms)
{
  return (void*)(INT_PTR)CF_GetClipboardBig((WDL_FastString*)arglist[0]);
}

static void* __vararg_CF_ShellExecute(void** arglist, int numparms)
{
  return (void*)(INT_PTR)CF_ShellExecute((const char*)arglist[0]);
}

static void* __vararg_CF_LocateInExplorer(void** arglist, int numparms)
{
  return (void*)(INT_PTR)CF_LocateInExplorer((const char*)arglist[0]);
}


#endif
