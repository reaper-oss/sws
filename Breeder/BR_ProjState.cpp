/******************************************************************************
/ BR_ProjState.cpp
/
/ Copyright (c) 2014 Dominik Martin Drzic
/ http://forum.cockos.com/member.php?u=27094
/ http://www.standingwaterstudios.com/reaper
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
#include "stdafx.h"
#include "BR_ProjState.h"
#include "BR_EnvTools.h"
#include "BR_Util.h"

/******************************************************************************
* Project state keys                                                          *
******************************************************************************/
const char* const ENVELOPE_SEL  = "<BR_ENV_SEL_SLOT";
const char* const CURSOR_POS    = "<BR_CURSOR_POS";

/******************************************************************************
* Globals                                                                     *
******************************************************************************/
SWSProjConfig<WDL_PtrList_DeleteOnDestroy<BR_EnvSel> >    g_envSel;
SWSProjConfig<WDL_PtrList_DeleteOnDestroy<BR_CursorPos> > g_cursorPos;

/******************************************************************************
* Project state saving functionality                                          *
******************************************************************************/
static bool ProcessExtensionLine (const char *line, ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg)
{
	if (isUndo)
		return false;

	LineParser lp(false);
	if (lp.parse(line) || lp.getnumtokens() < 1)
		return false;

	// Envelope point selection
	if (!strcmp(lp.gettoken_str(0), ENVELOPE_SEL))
	{
		g_envSel.Get()->Add(new BR_EnvSel(lp.gettoken_int(1), ctx));
		return true;
	}

	// Edit cursor position
	if (!strcmp(lp.gettoken_str(0), CURSOR_POS))
	{
		g_cursorPos.Get()->Add(new BR_CursorPos(lp.gettoken_int(1), ctx));
		return true;
	}

	return false;
}

static void SaveExtensionConfig (ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg)
{
	if (isUndo)
		return;

	// Envelope point selection
	if (int count = g_envSel.Get()->GetSize())
	{
		for (int i = 0; i < count; ++i)
			g_envSel.Get()->Get(i)->SaveState(ctx);
	}

	// Edit cursor position
	if (int count = g_cursorPos.Get()->GetSize())
	{
		for (int i = 0; i < count; ++i)
			g_cursorPos.Get()->Get(i)->SaveState(ctx);
	}
}

static void BeginLoadProjectState (bool isUndo, struct project_config_extension_t *reg)
{
	if (isUndo)
		return;

	// Envelope point selection
	g_envSel.Get()->Empty(true);
	g_envSel.Cleanup();

	// Edit cursor position
	g_cursorPos.Get()->Empty(true);
	g_cursorPos.Cleanup();
}

int ProjStateInit ()
{
	static project_config_extension_t s_projectconfig = {ProcessExtensionLine, SaveExtensionConfig, BeginLoadProjectState, NULL};
	return plugin_register("projectconfig", &s_projectconfig);
}

/******************************************************************************
* Envelope selection state                                                    *
******************************************************************************/
BR_EnvSel::BR_EnvSel (int slot, TrackEnvelope* envelope)
{
	m_slot = slot;
	m_selection = GetSelPoints(envelope);
}

BR_EnvSel::BR_EnvSel (int slot, ProjectStateContext* ctx)
{
	m_slot = slot;

	char line[32];
	LineParser lp(false);
	while(!ctx->GetLine(line, sizeof(line)) && !lp.parse(line))
	{
		if (strcmp(lp.gettoken_str(0), ">") == 0)
			break;
		m_selection.push_back(lp.gettoken_int(0));
	}
}

void BR_EnvSel::SaveState (ProjectStateContext* ctx)
{
	ctx->AddLine("%s %.2d", ENVELOPE_SEL, m_slot);
	for (size_t i = 0; i < m_selection.size(); ++i)
		ctx->AddLine("%d", m_selection[i]);
	ctx->AddLine(">");
}

void BR_EnvSel::Save (TrackEnvelope* envelope)
{
	m_selection = GetSelPoints(envelope);
}

void BR_EnvSel::Restore (TrackEnvelope* envelope)
{
	BR_Envelope env(envelope);
	env.UnselectAll();

	for (size_t i = 0; i < m_selection.size(); ++i)
		env.SetSelection(m_selection[i], true);

	env.Commit();
}

int BR_EnvSel::GetSlot ()
{
	return m_slot;
}

/******************************************************************************
* Cursor position state                                                       *
******************************************************************************/
BR_CursorPos::BR_CursorPos (int slot)
{
	m_slot = slot;
	m_position = GetCursorPositionEx(NULL);
}

BR_CursorPos::BR_CursorPos (int slot, ProjectStateContext* ctx)
{
	m_slot = slot;

	char line[32];
	LineParser lp(false);
	while(!ctx->GetLine(line, sizeof(line)) && !lp.parse(line))
	{
		if (strcmp(lp.gettoken_str(0), ">") == 0)
			break;
		m_position = lp.gettoken_float(0);
	}
}

void BR_CursorPos::SaveState (ProjectStateContext* ctx)
{
	ctx->AddLine("%s %.2d", CURSOR_POS, m_slot);
	ctx->AddLine("%.14lf", m_position);
	ctx->AddLine(">");
}

void BR_CursorPos::Save ()
{
	m_position = GetCursorPositionEx(NULL);
}

void BR_CursorPos::Restore ()
{
	SetEditCurPos2(NULL, m_position, true, false);
}

int BR_CursorPos::GetSlot ()
{
	return m_slot;
}