/******************************************************************************
/ BR_ContinuousActions.h
/
/ Copyright (c) 2014-2015 Dominik Martin Drzic
/ http://forum.cockos.com/member.php?u=27094
/ http://github.com/reaper-oss/sws
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

#include "BR.h"

// NF Eraser Tool
#include "../nofish/nofish.h"


/******************************************************************************
* Continuous actions                                                          *
*                                                                             *
* To make action continuous create BR_ContinuousAction object and register it *
* with ContinuousActionRegister().                                            *
*                                                                             *
* Note: works only with Main and MIDI editor section and only one continuous  *
* action may be executing at all times                                        *
* For examples see BR_Tempo.cpp or BR_Envelope.cpp                            *
******************************************************************************/
struct BR_ContinuousAction
{
public:
	BR_ContinuousAction (COMMAND_T* ct,
	                     bool           (*Init)      (COMMAND_T*,bool)            = NULL,
	                     int            (*DoUndo)    (COMMAND_T*)                 = NULL,
	                     HCURSOR        (*SetMouse)  (COMMAND_T*,int)             = NULL,
	                     WDL_FastString (*SetTooltip)(COMMAND_T*,int,bool*,RECT*) = NULL
	);

	bool           (*Init)      (COMMAND_T* ct, bool init);                                   // called on start with init = true and on shortcut release with init = false. Return false to abort init.
	int            (*DoUndo)    (COMMAND_T* ct);                                              // called when shortcut is released, return undo flag to create undo point (or 0 for no undo point). If NULL, no undo point will get created
	HCURSOR        (*SetMouse)  (COMMAND_T* ct, int window);                                  // called when setting cursor for each window, return NULL to skip
	WDL_FastString (*SetTooltip)(COMMAND_T* ct, int window, bool* setToBounds, RECT* bounds); // called when setting cursor tooltip, return empty string to remove existing tooltip
	COMMAND_T* ct;

	enum Window
	{
		MAIN_RULER,
		MAIN_ARRANGE,
		MIDI_NOTES_VIEW,
		MIDI_PIANO
	};
};

bool ContinuousActionRegister (BR_ContinuousAction* action); // register continuous action, returns true on success
void ContinuousActionStopAll ();                             // call from continuous action to stop execution before shortcut release
int  ContinuousActionTooltips ();                            // get current tooltips options (they get changed during continuous action so GetConfig() is useless)

/******************************************************************************
* Misc stuff needed for continuous actions to work properly                   *
******************************************************************************/
bool ContinuousActionHook (COMMAND_T* ct, int cmd, int flagOrRelmode, HWND hwnd); // call from the action hook and swallow action if it returns true
void ContinuousActionsExit ();                                                    // call when unloading extension to make sure everything is cleaned up

/******************************************************************************
* Put all continuous actions init functions here so their cmds end up         *
* consequential (to optimize command hook - see ContinuousActionHook())       *
******************************************************************************/
inline void ContinuousActionsInitExit (bool init)
{
	if (init)
	{
		BR_RegisterContinuousActions();

		// NF Eraser tool
		NF_RegisterContinuousActions();
	}
	else
	{
		// if we are exiting, context is already gone, trying to stop anything will likely segfault or access HWND-after-destroy
		// ContinuousActionStopAll();
	}
}
