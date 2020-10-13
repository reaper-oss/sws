/******************************************************************************
/ Parameters.h
/
/ Copyright (c) 2009 Tim Payne (SWS), original code by Xenakios
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

#pragma once

typedef struct t_command_params
{
	double EditCurRndMean;
	double ItemPosNudgeSecs;
	double ItemPosNudgeBeats;
	double ItemVolumeNudge;
	double ItemPitchNudgeA;
	double ItemPitchNudgeB;
	double CommandFadeInA;
	double CommandFadeInB;
	double CommandFadeOutA;
	double CommandFadeOutB;
	char CommandFadeInShapeA;
	char CommandFadeOutShapeA;
	char CommandFadeInShapeB;
	char CommandFadeOutShapeB;
	double RndItemSelProb;
	int PixAmount;
	double CurPosSecsAmount;
	int TrackHeight[2];
	string DefaultTrackLabel;
	string TrackLabelPrefix;
	string TrackLabelSuffix;
	double SectionLoopNudgeSecs;
	double TrackVolumeNudge;
} t_command_params;

typedef struct t_external_app_paths
{
	char *PathToTool1;
	char *PathToTool2;
	char *PathToAudioEditor1;
	char *PathToAudioEditor2;
} t_external_app_paths;

extern t_command_params g_command_params;
extern t_external_app_paths g_external_app_paths;

void DoLaunchExtTool(COMMAND_T*);
void InitCommandParams();
void DoShowCommandParameters(COMMAND_T*);

