/******************************************************************************
/ BR_ContinuousActions.h
/
/ Copyright (c) 2014 Dominik Martin Drzic
/ http://forum.cockos.com/member.php?u=27094
/ https://code.google.com/p/sws-extension
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

/******************************************************************************
* Continuous actions                                                          *
*                                                                             *
* To make action continuous create BR_ContinuousAction object and register it *
* with ContinuousActionRegister().                                            *
* Note: works only with Main section actions for now                          *
*                                                                             *
* For examples see BR_Tempo.cpp or BR_Envelope.cpp                            *
******************************************************************************/
struct BR_ContinuousAction
{
	BR_ContinuousAction(int cmd, bool (*Init)(bool) = NULL, bool (*DoUndo)() = NULL, HCURSOR (*SetMouseCursor)(int) = NULL, WDL_FastString (*SetTooltip)(int) = NULL)
	: cmd(cmd), Init(Init), DoUndo(DoUndo), SetMouseCursor(SetMouseCursor), SetTooltip(SetTooltip) {}

	bool           (*Init)(bool init);            // called on start with init = true and on shortcut release with init = false. Return false to abort init.
	bool           (*DoUndo)();                   // called when shortcut is released, return true to create undo point. If NULL, no undo point will get created
	HCURSOR        (*SetMouseCursor)(int window); // called when setting cursor for each window, return NULL to skip
	WDL_FastString (*SetTooltip)(int window);     // called when setting cursor tooltip, return empty string to remove existing tooltip
	const int cmd;                                // cmd of the action to be made continuous

	enum Window
	{
		RULER,
		ARRANGE
	};
};

bool ContinuousActionRegister (BR_ContinuousAction* action); // register continuous action, returns true on success
void ContinuousActionStopAll ();                             // call from continuous action to stop execution before shortcut release
int  ContinuousActionTooltips ();                            // get current tooltips options (they get changed during continuous action so GetConfig() is useless)

/******************************************************************************
* Call from the action hook and swallow action if it returns true             *
******************************************************************************/
bool ContinuousActionHook (int cmd, int flag);