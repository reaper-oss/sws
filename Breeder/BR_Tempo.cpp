/******************************************************************************
/ BR_Tempo.cpp
/
/ Copyright (c) 2013 Dominik Martin Drzic
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
#include "BR_Tempo.h"
#include "BR_TempoTools.h"
#include "BR_TempoDlg.h"
#include "BR_Util.h"
#include "../reaper/localize.h"

//Globals
extern bool g_convertMarkersToTempoDialog;
extern bool g_selectAdjustTempoDialog;
extern bool g_deselectTempoDialog;
extern bool g_tempoShapeDialog;
extern double g_tempoShapeSplitRatio;
extern int g_tempoShapeSplitMiddle;

void MoveTempo (COMMAND_T* ct)
{
	// Get tempo map and selected points' ID / closest tempo marker
	BR_TempoChunk tempoMap;
	vector<int> points;
	double cursor = GetCursorPositionEx(NULL);

	if (ct->user == 3)
	{
		int count = tempoMap.CountPoints()-1;
		for (int i = 0; i <= count; ++i)
		{	
			double cTime; tempoMap.GetPoint(i, &cTime, NULL, NULL);
			if (cTime > cursor)
			{
				double pTime; tempoMap.GetPoint(i-1, &pTime, NULL, NULL);
				
				if (i == 1) // since first tempo marker cannot be moved, always move the second
					points.push_back(i);
				else if (cursor - pTime < cTime - cursor)
					points.push_back(i-1);
				else
					points.push_back(i);
				break;
			}

			// Edit cursor is at the tempo marker -  no need to move anything
			else if (cTime == cursor)
				return;
			
			// Edit cursor is positioned after the last tempo point
			else if (i == count)
				points.push_back(i);
		}		
	}
	else
		points = *tempoMap.GetSelected();

	if (points.size() == 0)
		return;

	// Get amount of movement to be performed on the selected tempo points
	double tDiff;
	if (ct->user == 3)
	{
		double cTime; tempoMap.GetPoint(points[0], &cTime, NULL, NULL);
		tDiff = cursor - cTime;
	}
	else if (ct->user == 2 || ct->user == -2)
		tDiff = 1 / GetHZoomLevel() * (double)ct->user / 2;
	else
		tDiff = (double)ct->user/10000;
	
	// Loop through selected points
	Undo_BeginBlock2(NULL);
	int skipped	= 0;
	for (size_t i = 0; i < points.size(); ++i)
	{
		int id = points[i];
		if (id == 0) // Skip first tempo point
			continue;

		// Get tempo points
		double t1, t2, t3;
		double b1, b2, b3, Nb1, Nb2;
		bool s1, s2;

		bool P1 = tempoMap.GetPoint(id-1, &t1, &b1, &s1);
		bool P2 = tempoMap.GetPoint(id,   &t2, &b2, &s2);
		bool P3 = tempoMap.GetPoint(id+1, &t3, &b3, NULL);

		///// CALCULATE BPM VALUES /////
		////////////////////////////////	
		double Nt2 = t2+tDiff;
		
		// Current point
		if (!s2)
			Nb2 = b2*(t3-t2) / (t3-Nt2);
		else
			Nb2 = (b2+b3)*(t3-t2) / (t3-Nt2) - b3;

		// Previous point
		if (!s1)
			Nb1 = b1*(t2-t1) / (Nt2-t1);
		else
			Nb1 = (b1+b2)*(t2-t1) / (Nt2-t1) - Nb2;

		// Fix for the last tempo point so it can pass IF statement
		if (!P3) 
		{
			Nb2 = b2;
			t3 = Nt2 + 1;	
		}	

		///// CHECK POINTS BEFORE PREVIOUS POINT /////
		/////////////////////////////////////////////		
		
		int direction = -1, P0 = id - 2;
		bool doP0 = true, possibleP0 = true; 
		
		// Go through points backwards, check their shape and if linear check new BPM
		while (P0 >= 0) 
		{
			double t, b; bool s;
			tempoMap.GetPoint(P0, &t, &b, &s);
			direction *= -1;
			
			// If current points in the loop is square or does not exist (equal to being square), break
			if (!s) 
			{
				direction *= -1;
				if (P0 == id-2)	// If first point behind previous is square
					doP0 = false;		// there is no need to adjust anything
				break;	
			}		

			// Otherwise check if new BPM is possible...
			else if ((b - direction*(Nb1 - b1)) > MAX_BPM || (b - direction * (Nb1 - b1)) < MIN_BPM)
			{
				possibleP0 = false;
				break;
			}			
			P0--;
		}
		P0++; //correct ID due to the nature of a WHILE loop

		
		///// SET NEW BPM VALUES /////
		/////////////////////////////
				
		// IF statement acts as a safety net for illogical calculations.
		if (Nb1 >= MIN_BPM && Nb1 <= MAX_BPM && Nb2 >= MIN_BPM && Nb2 <= MAX_BPM && (Nt2-t1) >= MIN_TEMPO_DIST && (t3 - Nt2) >= MIN_TEMPO_DIST && possibleP0)
		{	
			// Points before previous (if needed)
			if (doP0)
			{	
				for (int j = P0; j <= id - 2 ; ++j)
				{
					double t, b; bool s;
					tempoMap.GetPoint(j, &t, &b, &s);
					tempoMap.SetPoint(j, t, (b - direction*(Nb1-b1)), s);
					direction *= -1;
				}
			}

			// Previous point
			tempoMap.SetPoint(id-1, t1, Nb1, s1);
						
			// Selected point
			tempoMap.SetPoint(id, Nt2, Nb2, s2);			
		}		
		else
			++skipped;
	}
	if (PreventUIRefresh) // prevent jumpy cursor when moving closest tempo marker
		PreventUIRefresh (1);	
	
	tempoMap.Commit();
	if (ct->user == 3)
		SetEditCurPos2(NULL, cursor, false, false); // Keep cursor position when moving to closest tempo marker (needed if timebase is beats)

	if (PreventUIRefresh)
		PreventUIRefresh (-1);

	Undo_EndBlock2 (NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL);

	// Warn user if some points weren't processed
	static bool g_pointsNotProcessed = true;
	if ((points.size() > 1 || ct->user == 3) && skipped != 0 && (g_pointsNotProcessed))
	{
		char buffer[512];
		_snprintf(buffer, sizeof(buffer), __LOCALIZE_VERFMT("%d of the selected points didn't get processed because some points would end up with illegal BPM or position. Would you like to be warned if it happens again?", "sws_mbox"), skipped);
		int userAnswer = ShowMessageBox(buffer, __LOCALIZE("SWS - Warning", "sws_mbox"), 4);
		if (userAnswer == 7)
			g_pointsNotProcessed = false;	
	}
};

void DeleteTempo (COMMAND_T* ct)
{
	// Get tempo map
	BR_TempoChunk tempoMap;
	if (tempoMap.GetSelected()->size() == 0)
		return;
	int offset = 0;

	// Loop through selected points and perform BPM calculations
	Undo_BeginBlock2(NULL);
	int skipped = 0;
	for (size_t i = 0; i < tempoMap.GetSelected()->size(); ++i)
	{
		int id = (*tempoMap.GetSelected())[i] + offset;		
		if (id == 0) // Skip first tempo points
			continue;

		// Get tempo points
		double t1, t2, t3, t4;
		double b1, b2, b3, b4;
		double m1, m2;
		bool s0, s1, s2, s3;
	
		bool P0 = tempoMap.GetPoint(id-2, NULL, NULL, &s0);
		bool P1 = tempoMap.GetPoint(id-1, &t1, &b1, &s1);
		bool P2 = tempoMap.GetPoint(id,   &t2, &b2, &s2);
		bool P3 = tempoMap.GetPoint(id+1, &t3, &b3, &s3);
		bool P4 = tempoMap.GetPoint(id+2, &t4, &b4, NULL);

		double Nt1 = t1;
		double Nt3 = t3;

		// If some points do not exist fake them as square
		if (!P0)
			s0 = false;
				
		// Get P2-P3 length
		if (!s2)
			m2 = b2*(t3-t2) / 240;
		else
			m2 = (b2+b3)*(t3-t2) / 480;


		///// CALCULATE BPM VALUES /////
		////////////////////////////////
		
		// If previous point's shape is square...
		if (P3 && !s1)
		{
			m1 = b1*(t2-t1) / 240;
						
			// If a shape of the point behind previous point is square...
			if (s0 == 0)
				b1 = 240*(m1+m2) / (t3-t1);
			
			// If a shape of the point behind previous point is linear...
			else
			{
				Nt3 = t1 + 240*(m1+m2) / b1;								
				// If P4 exists...
				if (P4)
				{
					// If P3 is linear...
					if(s3)
					{
						double m3 = (b3+b4)*(t4-t3) / 480;
						b3 = 480*m3 / (t4-Nt3) - b4;
					}
					// If P3 is square...
					else
					{
						double m3 = b3*(t4-t3) / 240;
						b3 = 240*m3 / (t4-Nt3);
					}				
				}
			}
		}
		
		// If previous point's shape is linear...
		else if (P3 && s1)
		{
			m1 = (b1+b2)*(t2-t1) / 480;
		
			// If a shape of the point behind previous point is square...
			if (s0 == 0)
				b1 = 480*(m1+m2) / (t3-t1) - b3;
			
			// If a shape of the point behind previous point is linear...
			else
			{
				// If P4 exists...
				if (P4)
				{
					// If P3 is linear...
					if(s3)
					{
						double m3 = (b3+b4)*(t4-t3) / 480;
						double f1 = 480*(m1+m2); 
						double f2 = 480*m3;
						double bp = b1-b4;
						if (bp == 0) // fix for a situation when b1 and b4 are the same
							bp -= 0.0000001;
						double a = bp*(t1+t4) + f1+f2;
						double b = bp*(t1*t4) + f1*t4 + f2*t1;
						Nt3 = (a - sqrt(pow(a,2) - 4*bp*b)) / (2*bp);
						b3 = f2 / (t4 - Nt3) - b4;	
					}
					// If P3 is square...
					else
					{
						double m3 = b3*(t4-t3) / 240;
						double f1 = 480*(m1+m2); 
						double f2 = 240*m3;
						double a = b1*(t1+t4) + f1+f2;
						double b = b1*(t1*t4) + f1*t4 + f2*t1;
						Nt3 = (a - sqrt(pow(a,2) - 4*b1*b)) / (2*b1);
						b3 = f2 / (t4 - Nt3);	
					}				
				}
				
				// If P4 does not exist...
				else
					b3 = 480*(m1+m2) / (t3-t1) - b1;
			}					
		}
			

		///// SET NEW BPM /////	
		///////////////////////

		// Readjust variables for unexisting points so they can pass next if statement (t0 to Nt1 is not important because Nt1 never moves)
		if (!P3)
		{
			Nt3 = Nt1+1;
			b3 = 120;
		}
		if (!P4)
			t4 = Nt3+1;

		// IF statement acting as a safety net for illogical calculations
		if (b1>=MIN_BPM && b1<=MAX_BPM && b2>=MIN_BPM && b2<=MAX_BPM && b3>=MIN_BPM && b3<=MAX_BPM && (Nt3-Nt1)>=MIN_TEMPO_DIST && (t4-Nt3)>=MIN_TEMPO_DIST)
		{
			// Set points
			if (P3)
			{
				// Previous point
				tempoMap.SetPoint(id-1, Nt1, b1, s1);					
				
				// Next point
				tempoMap.SetPoint(id+1, Nt3, b3, s3);
			}
			
			// Delete point
			tempoMap.DeletePoint(id);			
			--offset;
		}
		else
			++skipped;
	}
	tempoMap.Commit();
	Undo_EndBlock2 (NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL);

	// Warn user if some points weren't processed
	static bool g_pointsNotProcessed = true;
	if (skipped != 0 && (g_pointsNotProcessed))
	{
		char buffer[512];
		_snprintf(buffer, sizeof(buffer), __LOCALIZE_VERFMT("%d of the selected points didn't get processed because some points would end up with illegal BPM or position. Would you like to be warned if it happens again?", "sws_mbox"), skipped);
		int userAnswer = ShowMessageBox(buffer, __LOCALIZE("SWS - Warning", "sws_mbox"), 4);
		if (userAnswer == 7)
			g_pointsNotProcessed = false;	
	}
};

void EditTempo (COMMAND_T* ct)
{
	// Get tempo map
	BR_TempoChunk tempoMap;
	if (tempoMap.GetSelected()->size() == 0)
		return;

	// Get values and type of operation to be performed
	bool percentage = false;
	double diff;
	if (GetFirstDigit((int)ct->user) == 1)
		diff = (double)ct->user / 1000;
	else
	{
		diff = (double)ct->user / 200000;
		percentage = true;
	}	

	// Loop through selected points and perform BPM calculations
	Undo_BeginBlock2(NULL);
	int skipped	= 0;
	for (size_t i = 0; i < tempoMap.GetSelected()->size(); ++i)
	{
		// Get tempo points
		int id = (*tempoMap.GetSelected())[i];
		double t0, t1, t2, t3, t4;
		double b0, b1, b2, b3, b4;
		bool s0, s1, s2, s3;
		
		bool P0 = tempoMap.GetPoint(id-2, &t0, &b0, &s0);
		bool P1 = tempoMap.GetPoint(id-1, &t1, &b1, &s1);
		bool P2 = tempoMap.GetPoint(id,   &t2, &b2, &s2);
		bool P3 = tempoMap.GetPoint(id+1, &t3, &b3, &s3);
		bool P4 = tempoMap.GetPoint(id+2, &t4, &b4, NULL);

		// Calculate percentage of the selected point
		double bDiff;
		if (percentage)
			bDiff = b2*diff;
		else
			bDiff = diff;

		double Nt1 = t1;
		double Nt3 = t3;
	
		// If points behind selected point don't exist, fake them as square.
		if (!P0)
			s0 = false;
		if (!P1)
			s1 = false;

		///// CALCULATE BPM VALUES /////	
		////////////////////////////////

		///// PREVIOUS POINT /////

		// If previous point's shape is linear...
		if (s1)
		{
			// If a shape of the point behind previous point is linear...
			if (s0)
			{
				double m0 = (b0+b1)*(t1-t0) / 480;
				double m1 = (b1+b2)*(t2-t1) / 480;
				double f1 = 480*m0; 
				double f2 = 480*m1;
				double bp = b0 - (b2+bDiff);
				if (bp == 0) // fix for a situation when a new b2 and b0 are the same
					bp -= 0.0000001;
				double a = bp*(t0+t2) + f1+f2;
				double b = bp*(t0*t2) + f1*t2 + f2*t0;
				Nt1 = (a - sqrt(pow(a,2) - 4*bp*b)) / (2*bp);
				b1 = f1 / (Nt1-t0) - b0;						
			}
			// If a shape of the point behind previous point is square...
			else
			{
				b1 -= bDiff;
				Nt1 = t1;
			}				
		}
		// If previous point's shape is square... 
		else
			P1 = false;	

		///// NEXT POINT /////

		// If current point's shape is linear...
		if (s2 && P3)
		{
			double m2 = (b2+b3)*(t3-t2) / 480;
			double f1 = 480*m2;

			if (P4)
			{
				// If next point's shape is linear...
				if (s3)						
				{
					double m3 = (b3+b4)*(t4-t3) / 480;
					double f2 = 480*m3;
					double bp = (b2 + bDiff) - b4;
					if (bp == 0)
						bp -= 0.0000001;
					double a = bp*(t2+t4) + f1+f2;
					double b = bp*(t2*t4) + f1*t4 + f2*t2;
					Nt3 = (a - sqrt(pow(a,2) - 4*bp*b)) / (2*bp);
					b3 = f1 / (Nt3-t2) - (b2+bDiff);
				}			
		
				// If next point's shape is square...
				else
				{
					double m3 = b3*(t4-t3) / 240;
					double f2 = 240*m3;
					double bp = (b2+bDiff);
					double a = bp*(t2+t4) + f1+f2;
					double b = bp*(t2*t4) + f1*t4 + f2*t2;
					Nt3 = (a - sqrt(pow(a,2) - 4*bp*b)) / (2*bp);
					b3 = f1 / (Nt3-t2) - (b2+bDiff);
				}
			}
			else
				Nt3 = t2 + 480*m2 / (b2+b3+diff);	
		}
			
		// If current point's shape is square...
		else if (!s2 && P3)
		{
			double m2 = b2*(t3-t2) / 240;
			
			if (P4)
			{
				// If next point's shape is linear...
				if (s3)
				{	
					double m3 = (b3+b4)*(t4-t3) / 480;
					Nt3 = (240*m2 + (b2+bDiff)*t2) / (b2+bDiff);
					b3 = 480*m3 / (t4-Nt3) - b4;							
				}
	
				// If next point's shape is square...
				else
				{
					double m3 = b3*(t4-t3) / 240;
					Nt3 = (240*m2 + (b2+bDiff)*t2) / (b2+bDiff);
					b3 = 240*m3 / (t4-Nt3);			
				}
			}
			else
				Nt3 = t2 + 240*m2 / (b2+diff);	
		}
		
		///// SET NEW BPM /////	
		///////////////////////

		// Readjust variables for unexisting points so they can pass next if statement
		if (!P1)
		{
			Nt1 = t2-1;
			b1 = 120;
		}
		if (!P3)
		{
			Nt3 = t2+1;
			b3 = 120;
		}

		if (!P4)
			t4 = Nt3+1;
		if (!P0)
			t0 = Nt1-1;
	
		// IF statement acting as a safety net for illogical calculations
		if (b1>=MIN_BPM && b1<=MAX_BPM && (b2+bDiff)>=MIN_BPM && (b2+bDiff)<=MAX_BPM && b3>=MIN_BPM && b3<=MAX_BPM && (Nt1-t0)>=MIN_TEMPO_DIST && (t2-Nt1)>=MIN_TEMPO_DIST && (Nt3-t2)>=MIN_TEMPO_DIST &&(t4-Nt3)>=MIN_TEMPO_DIST)
		{
			// Previous point
			if (P1)
				tempoMap.SetPoint(id-1, Nt1, b1, s1);

			// Current point
			tempoMap.SetPoint(id, t2, b2+bDiff, s2);
	
			// Next point
			if (P3)
				tempoMap.SetPoint(id+1, Nt3, b3, s3);
		}
		else
			++skipped;
	}
	tempoMap.Commit();
	Undo_EndBlock2 (NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL);

	// Warn user if some points weren't processed
	static bool g_pointsNotProcessed = true;
	if (tempoMap.GetSelected()->size() > 1 && skipped != 0 && (g_pointsNotProcessed))
	{
		char buffer[512];
		_snprintf(buffer, sizeof(buffer), __LOCALIZE_VERFMT("%d of the selected points didn't get processed because some points would end up with illegal BPM or position. Would you like to be warned if it happens again?", "sws_mbox"), skipped);
		int userAnswer = ShowMessageBox(buffer, __LOCALIZE("SWS - Warning", "sws_mbox"), 4);
		if (userAnswer == 7)
			g_pointsNotProcessed = false;	
	}
};

void EditTempoGradual(COMMAND_T* ct)
{
	// Get tempo map
	BR_TempoChunk tempoMap;
	if (tempoMap.GetSelected()->size() == 0)
		return;

	// Get values and type of operation to be performed
	bool percentage = false;
	double diff;
	if (GetFirstDigit((int)ct->user) == 1)
		diff = (double)ct->user / 1000;
	else
	{
		diff = (double)ct->user / 200000;
		percentage = true;
	}	

	// Loop through selected points and perform BPM calculations
	Undo_BeginBlock2(NULL);
	int skipped	= 0;
	for (size_t i = 0; i < tempoMap.GetSelected()->size(); ++i)
	{
		// Get tempo points
		int id = (*tempoMap.GetSelected())[i];
		double t0, t1, t2, t3;
		double b0, b1, b2, b3;
		bool s0, s1, s2;
		
		bool P0 = tempoMap.GetPoint(id-1, &t0, &b0, &s0);
		bool P1 = tempoMap.GetPoint(id,   &t1, &b1, &s1); if (!s1){return;}
		bool P2 = tempoMap.GetPoint(id+1, &t2, &b2, &s2);
		bool P3 = tempoMap.GetPoint(id+2, &t3, &b3, NULL);
	
		// Calculate percentage of the selected point
		double bDiff;
		if (percentage)
			bDiff = b1*diff;
		else
			bDiff = diff;
	
		double Nt1 = t1;
		double Nt2 = t2;
		
		// If point behind selected point don't exist, fake them as square
		if (!P0)
			s0 = false;
	
		///// CALCULATE BPM VALUES /////	
		////////////////////////////////
	
		///// CURRENT POINT /////
	
		// If previous point's shape is linear...
		if (s0)		
		{
			double m0 = (b0+b1)*(t1-t0) / 480;
			Nt1 = t0 + 480*m0 / (b0+b1+bDiff);
		}
	
		///// NEXT POINT /////
		
		// If next point exists...
		if (P2)
		{	
			// If point after next point exists...	
			if (P3)
			{
				// If next points is linear...
				if (s2)
				{
					double m1 = (b1+b2)*(t2-t1) / 480;
					double m2 = (b2+b3)*(t3-t2) / 480;
					double f1 = 480*m1; 
					double f2 = 480*m2;
					double bp = (b1+bDiff) - b3;
					if (bp == 0) // fix for a situation when a new b1 and b3 are the same
						bp -= 0.0000001;		
					double a = bp*(Nt1+t3) + f1+f2;
					double b = bp*(Nt1*t3) + f1*t3 + f2*Nt1;
					Nt2 = (a - sqrt(pow(a,2) - 4*bp*b)) / (2*bp);
					b2 = f2 / (t3-Nt2) - b3;	
				}
	
				// If next points is square...
				else
				{
					double m2 = b2*(t3-t2) / 240;
					double m1 = (b1+b2)*(t2-t1) / 480;
					double f1 = 480*m1; 
					double f2 = 240*m2;
					double bp = b1+bDiff;
					double a = bp*(Nt1+t3) + f1+f2;
					double b = bp*(Nt1*t3) + f1*t3 + f2*Nt1;
					Nt2 = (a - sqrt(pow(a,2) - 4*bp*b)) / (2*bp);
					b2 = f2 / (t3-Nt2);	
				}
			}
	
			// If point after next point doesn't exist...
			else
			{
				double m1 = (b1+b2)*(t2-t1)/ 480;
				b2 = (480*m1) / (t2-Nt1) - (b1+bDiff);
			}
		}

		///// SET NEW BPM /////	
		///////////////////////

		// Readjust variables for unexisting points so they can pass next if statement
		if (!P0)
			t0 = Nt1-1;
		if(!P2)
		{
			Nt2 = t1+1;
			b2 = 120;
		}
		if(!P3)
			t3 = Nt2+1;

		// IF statement acting as a safety net for illogical calculations
		if ((b1+bDiff)>=MIN_BPM && (b1+bDiff)<=MAX_BPM && b2>=MIN_BPM && b2<=MAX_BPM && (Nt1-t0)>=MIN_TEMPO_DIST && (Nt2-Nt1)>=MIN_TEMPO_DIST && (t3-Nt2)>=MIN_TEMPO_DIST)
		{
			// Current point
			tempoMap.SetPoint(id, Nt1, b1+bDiff, s1);
	
			// Next point
			if (P2)
				tempoMap.SetPoint(id+1, Nt2, b2, s2);
		}
		else
			++skipped;
	}
	tempoMap.Commit();
	Undo_EndBlock2 (NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL);

	// Warn user if some points weren't processed
	static bool g_pointsNotProcessed = true;
	if (tempoMap.GetSelected()->size() > 1 && skipped != 0 && (g_pointsNotProcessed))
	{
		char buffer[512];
		_snprintf(buffer, sizeof(buffer), __LOCALIZE_VERFMT("%d of the selected points didn't get processed because some points would end up with illegal BPM or position. Would you like to be warned if it happens again?", "sws_mbox"), skipped);
		int userAnswer = ShowMessageBox(buffer, __LOCALIZE("SWS - Warning", "sws_mbox"), 4);
		if (userAnswer == 7)
			g_pointsNotProcessed = false;	
	}
};

void TempoShapeLinear (COMMAND_T* ct)
{
	// Get tempo map (with effCreate mode)
	BR_TempoChunk tempoMap (true);
	if (tempoMap.GetSelected()->size() == 0)
		return;
	
	// Get splitting options
	if (g_tempoShapeSplitRatio == -1 || g_tempoShapeSplitMiddle == -1)
	{
		int split; char splitRatio[128];
		LoadOptionsTempoShape (split, splitRatio, sizeof(splitRatio));
		SetTempoShapeGlobalVariable (split, splitRatio, sizeof(splitRatio));
	}

	// Check if middle point is to be split
	bool split = false;
	if (g_tempoShapeSplitMiddle == 0)
		split = false;
	else if (g_tempoShapeSplitMiddle == 1)
		if (g_tempoShapeSplitRatio != 0)
			split = true;

	// Loop through selected points and perform BPM calculations
	Undo_BeginBlock2(NULL);
	int skipped = 0;
	int count = tempoMap.CountPoints()-1;
	for (size_t i = 0; i < tempoMap.GetSelected()->size(); ++i)
	{
		// Get tempo points
		int id = (*tempoMap.GetSelected())[i];
		double t0, t1, b0, b1; 
		bool s0;		
		
		tempoMap.GetPoint(id, &t0, &b0, &s0);
		if (s0)
			continue;

		bool P1;
		if (id < count)
		{
			P1 = true;
			tempoMap.GetPoint(id+1, &t1, &b1, NULL);
		}
		else
			P1 = false;

		// Get middle point's position and BPM
		double position, bpm = 120, measure = b0*(t1-t0) / 240;
		if (P1)
			FindMiddlePoint(position, bpm, measure, b0, b1, t0, t1);

		///// SET NEW SHAPE /////
		/////////////////////////
		
		// Don't split middle point
		///////////////////////////////////////////////////////////////////////////
		if (!split)
		{
			// Create middle point if needed
			if (P1 && b0 != b1)
			{	
				// Check if value and position is legal, if not, skip
				if (bpm>=MIN_BPM && bpm<=MAX_BPM && (position-t0)>=MIN_TEMPO_DIST && (t1-position)>=MIN_TEMPO_DIST)
					tempoMap.CreatePoint(0, position, bpm, true);
				else
				{
					++skipped;
					continue;
				}
			}		
		}
		// Split middle point
		///////////////////////////////////////////////////////////////////////////
		else
		{
			// Create middle point if needed
			if (P1 && b0 != b1)
			{
				double position1, position2, bpm1, bpm2;
				SplitMiddlePoint (position1, position2, bpm1, bpm2, g_tempoShapeSplitRatio, measure, b0, bpm, b1, t0, position, t1);
				
				// Check if value and position is legal, if not, skip
				if (bpm1>=MIN_BPM && bpm1<=MAX_BPM && bpm2>=MIN_BPM && bpm2<=MAX_BPM && (position1-t0)>=MIN_TEMPO_DIST && (position2-position1)>=MIN_TEMPO_DIST && (t1-position2)>=MIN_TEMPO_DIST)
				{
					tempoMap.CreatePoint(0, position1, bpm1, true);
					tempoMap.CreatePoint(0, position2, bpm2, true);
				}
				else
				{
					++skipped;
					continue;
				}
			}
		}
		// Change shape of the selected point
		///////////////////////////////////////////////////////////////////////////
		tempoMap.SetPoint(id, t0, b0, true);
	}	
	tempoMap.Commit();
	Undo_EndBlock2 (NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL);

	// Warn user if some points weren't processed
	static bool g_pointsNotProcessed = true;
	if (skipped != 0 && (g_pointsNotProcessed))
	{
		char buffer[512];
		_snprintf(buffer, sizeof(buffer), __LOCALIZE_VERFMT("%d of the selected points didn't get processed because some points would end up with illegal BPM or position. Would you like to be warned if it happens again?", "sws_mbox"), skipped);
		int userAnswer = ShowMessageBox(buffer, __LOCALIZE("SWS - Warning", "sws_mbox"), 4);
		if (userAnswer == 7)
			g_pointsNotProcessed = false;	
	}
};

void TempoShapeSquare (COMMAND_T* ct)
{
	// Get tempo map (with effCreate mode)
	BR_TempoChunk tempoMap (true);
	if (tempoMap.GetSelected()->size() == 0)
		return;

	// Get splitting options
	if (g_tempoShapeSplitRatio == -1 || g_tempoShapeSplitMiddle == -1)
	{
		int split; char splitRatio[128];
		LoadOptionsTempoShape (split, splitRatio, sizeof(splitRatio));
		SetTempoShapeGlobalVariable (split, splitRatio, sizeof(splitRatio));
	}

	// Check if middle point is to be split
	bool split = false;
	if (g_tempoShapeSplitMiddle == 0)
		split = false;
	else if (g_tempoShapeSplitMiddle == 1)
		if (g_tempoShapeSplitRatio != 0)
			split = true;
	
	// Loop through selected points and perform BPM calculations
	Undo_BeginBlock2(NULL);
	int skipped = 0;
	int count = tempoMap.CountPoints()-1;
	for (size_t i = 0; i < tempoMap.GetSelected()->size(); ++i)
	{
		// Get tempo points
		int id = (*tempoMap.GetSelected())[i];
		double t0, t1, b0, b1, b2; 
		bool s0, s1;
				
		tempoMap.GetPoint(id, &t1, &b1, &s1);
		if (!s1)
			continue;

		bool P0 = tempoMap.GetPoint(id-1, &t0, &b0, &s0);		
		bool P2;
		if (id < count)
		{
			P2 = true;
			tempoMap.GetPoint(id+1, NULL, &b2, NULL);
		}
		else
			P2 = false;

		// Get new bpm of selected point
		double Nb1;
		if (P2 && b1 != b2)
			Nb1 = (b1+b2) / 2;
		else
			Nb1 = b1;

		// Check if new bpm is legal, if not, skip
		if(Nb1 < MIN_BPM || Nb1 > MAX_BPM)
		{
			++skipped;
			continue;
		}

		///// SET NEW SHAPE /////
		/////////////////////////

		// Create middle point(s) is needed
		///////////////////////////////////////////////////////////////////////////////
		if (P0 && P2 && s0 && Nb1 != b2)
		{
			// Get middle point's position and BPM
			double position, bpm = 120, measure = (b0+b1)*(t1-t0) / 480;
			FindMiddlePoint(position, bpm, measure, b0, Nb1, t0, t1);

			// Don't split middle point
			///////////////////////////////////////////////////////////////////////////
			if (!split)
			{
				if (bpm<= MAX_BPM && bpm>=MIN_BPM && (position-t0)>=MIN_TEMPO_DIST && (t1-position)>=MIN_TEMPO_DIST)
					tempoMap.CreatePoint(0, position, bpm, true);
				else
				{
					++skipped;
					continue;
				}				
			}
			// Split middle point
			///////////////////////////////////////////////////////////////////////////
			else
			{
				double position1, position2, bpm1, bpm2;
				SplitMiddlePoint (position1, position2, bpm1, bpm2, g_tempoShapeSplitRatio, measure, b0, bpm, Nb1, t0, position, t1);

				if (bpm1>=MIN_BPM && bpm1<=MAX_BPM && bpm2>=MIN_BPM && bpm2<=MAX_BPM && (position1-t0)>=MIN_TEMPO_DIST && (position2-position1)>=MIN_TEMPO_DIST && (t1-position2)>=MIN_TEMPO_DIST)
				{
					tempoMap.CreatePoint(0, position1, bpm1, true);
					tempoMap.CreatePoint(0, position2, bpm2, true);					
				}
				else
				{
					++skipped;
					continue;
				}
			}
		}
		// Change shape of the selected point
		///////////////////////////////////////////////////////////////////////////////
		tempoMap.SetPoint(id, t1, Nb1, false);
	}
	tempoMap.Commit();
	Undo_EndBlock2 (NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL);

	// Warn user if some points weren't processed
	static bool g_pointsNotProcessed = true;
	if (skipped != 0 && (g_pointsNotProcessed))
	{
		char buffer[512];
		_snprintf(buffer, sizeof(buffer), __LOCALIZE_VERFMT("%d of the selected points didn't get processed because some points would end up with illegal BPM or position. Would you like to be warned if it happens again?", "sws_mbox"), skipped);
		int userAnswer = ShowMessageBox(buffer, __LOCALIZE("SWS - Warning", "sws_mbox"), 4);
		if (userAnswer == 7)
			g_pointsNotProcessed = false;	
	}
};

void TempoAtGrid (COMMAND_T* ct)
{
	// Get tempo map (with effCreate mode) and grid
	BR_TempoChunk tempoMap (true);
	if (tempoMap.GetSelected()->size() == 0)
		return;
	double grid = *(double*)GetConfigVar("projgriddiv");
	
	// Loop through selected points
	Undo_BeginBlock2(NULL);
	int count = tempoMap.CountPoints()-1;
	for (int i = 0; i < tempoMap.GetSelected()->size(); ++i)
	{		
		// Get tempo points
		int id = (*tempoMap.GetSelected())[i];
		double t0, t1, b0, b1;
		bool s0;	

		tempoMap.GetPoint(id, &t0, &b0, &s0);
		tempoMap.GetPoint(id+1, &t1, &b1, NULL);
		
		// If last tempo point is selected, fake second point as if it's at the end of project (markers and regions included)
		if (id == count)
		{
			b1 = b0;
			t1 = EndOfProject (true, true);
		}

		// If point is square, set "fake" bpm for second point (needed for TempoAtPosition)
		if (!s0)
			b1 = b0;

		// This part is really only relevant in certain cases involving grid division longer than measure's length
		// Grid diving starts again from the start of the measure in which tempo marker is so we need to calculate 
		// the offset between where grid line really is and where it would be if we just divided QN with grid spacing.
		double beat; int measure; GetTempoTimeSigMarker(NULL, id, NULL, &measure, &beat, NULL, NULL, NULL, NULL); 
		double offset =  grid - fmod(TimeMap_timeToQN(TimeMap2_beatsToTime(0, 0, &measure)), grid);
		
		// Find first grid line and then loop through the rest creating tempo points
		double pGridLn = t0, gridLn = TimeMap_timeToQN (t0);
		gridLn = TimeMap_QNToTime(gridLn-offset - fmod(gridLn,grid));		// it can be before tempo point but next while should correct that
		while (true)
		{
			// Search for the next grid line								// max grid division of 1/256 at max tempo of 960 creates a grid line
			while (gridLn < pGridLn + MAX_GRID_DIV)							// every 0.00097 s... so let it also be our safety net to prevent accidental
				gridLn = TimeMap_QNToTime(TimeMap_timeToQN (gridLn)+grid);	// creation of multiple points around grid line and the end point of tempo transition
																			
			// Create points until the next point
			if (gridLn < t1 - MAX_GRID_DIV)
				tempoMap.CreatePoint(0, gridLn, TempoAtPosition (b0, b1, t0, t1, gridLn), s0);
			else
				break;

			pGridLn = gridLn;
		}		
	}
	tempoMap.Commit();
	Undo_EndBlock2 (NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL);
};

void ConvertMarkersToTempoDialog (COMMAND_T* = NULL)
{
	static HWND hwnd = CreateDialog (g_hInst, MAKEINTRESOURCE(IDD_BR_MARKERS_TO_TEMPO), g_hwndParent, ConvertMarkersToTempoProc);

	// Check dialog's state and hide/show it accordingly
	if (g_convertMarkersToTempoDialog)
	{	
		KillTimer(hwnd, 1);	
		ShowWindow(hwnd, SW_HIDE);
		g_convertMarkersToTempoDialog = false;
	}
	
	else
	{		
		// Detect timebase
		bool cancel = false;
		if (*(int*)GetConfigVar("itemtimelock") != 0)
		{	
			int answer = MessageBox(g_hwndParent, __LOCALIZE("Project timebase is not set to time. Do you want to set it now?","sws_DLG_166"), __LOCALIZE("SWS - Warning","sws_mbox"), MB_YESNOCANCEL);
			if (answer == 6)
				*(int*)GetConfigVar("itemtimelock") = 0;
			if (answer == 2)
				cancel = true;
		}	
		
		if (!cancel)
		{
			SetTimer(hwnd, 1, 100, NULL);
			ShowWindow(hwnd, SW_SHOW);
			SetFocus(hwnd);
			g_convertMarkersToTempoDialog = true;
		}
	}
};

void SelectAdjustTempoDialog (COMMAND_T* = NULL)
{
	static HWND hwnd = CreateDialog (g_hInst, MAKEINTRESOURCE(IDD_BR_SELECT_ADJUST_TEMPO), g_hwndParent, SelectAdjustTempoProc);

	// Check dialog's state and hide/show it accordingly
	if (g_selectAdjustTempoDialog)
	{	
		KillTimer(hwnd, 1);
		CallDeselTempoDialog(false, hwnd); // create and/or hide child dialog and pass parent's handle to it
		ShowWindow(hwnd, SW_HIDE);
		g_selectAdjustTempoDialog = false;
	}

	else
	{
		SetTimer(hwnd, 1, 500, NULL);
		ShowWindow(hwnd, SW_SHOW);
		SetFocus(hwnd);
		g_selectAdjustTempoDialog = true;
	}
};

void TempoShapeOptionsDialog (COMMAND_T* = NULL)
{
	static HWND hwnd = CreateDialog (g_hInst, MAKEINTRESOURCE(IDD_BR_TEMPO_SHAPE_OPTIONS), g_hwndParent, TempoShapeOptionsProc);

	// Check dialog's state and hide/show it accordingly
	if (g_tempoShapeDialog)
	{	
		ShowWindow(hwnd, SW_HIDE);
		g_tempoShapeDialog = false;
	}

	else
	{
		ShowWindow(hwnd, SW_SHOW);
		SetFocus(hwnd);
		g_tempoShapeDialog = true;
	}
};

void RandomizeTempoDialog (COMMAND_T* = NULL)
{
	// Temporary change of preferences - prevent reselection of points in time selection
	int envClickSegMode = *(int*)GetConfigVar("envclicksegmode");
	*(int*)GetConfigVar("envclicksegmode") = 1;

	Undo_BeginBlock2(NULL);
	DialogBox (g_hInst, MAKEINTRESOURCE(IDD_BR_RANDOMIZE_TEMPO), g_hwndParent, RandomizeTempoProc);
	Undo_EndBlock2 (NULL, __LOCALIZE("Randomize selected tempo markers","sws_undo"), UNDO_STATE_ALL);

	// Restore preferences back to the previous state
	*(int*)GetConfigVar("envclicksegmode") = envClickSegMode;
};