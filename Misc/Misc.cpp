/******************************************************************************
/ Misc.cpp
/
/ Copyright (c) 2011 Tim Payne (SWS)
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

// "Misc" is a container for actions that don't belong in any of the other
// major subgroups of the SWS extension.  For the most part, these are just
// actions that change to state of Reaper in some fashion.
//
// All the actions are contained in separate .cpp files in the Misc directory.
// This file, misc.cpp, is just serves as a place to initialize each individual
// misc module.

#include "stdafx.h"
#include "Adam.h"
#include "Analysis.h"
#include "Context.h"
#include "EditCursor.h"
#include "FolderActions.h"
#include "ItemParams.h"
#include "ItemSel.h"
#include "Macros.h"
#include "ProjPrefs.h"
#include "RecCheck.h"
#include "TrackParams.h"
#include "TrackSel.h"
#include "../Zoom.h"
#include "Misc.h"

void MiscSlice()
{
	EditCursorSlice();
}

int MiscInit()
{
	// Call sub-init routines
	if (!AdamInit())
		return 0;
	if (!AnalysisInit())
		return 0;
	if (!ContextInit())
		return 0;
	if (!EditCursorInit())
		return 0;
	if (!FolderActionsInit())
		return 0;
	if (!ItemParamsInit())
		return 0;
	if (!ItemSelInit())
		return 0;
	if (!MacrosInit())
		return 0;
	if (!ProjPrefsInit())
		return 0;
	if (!RecordCheckInit())
		return 0;
	if (!TrackParamsInit())
		return 0;
	if (!TrackSelInit())
		return 0;
	
	return 1;
}

void MiscExit()
{
	EditCursorExit();
	ZoomExit();
}
