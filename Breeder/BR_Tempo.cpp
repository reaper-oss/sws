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
#include "BR_EnvTools.h"
#include "BR_TempoDlg.h"
#include "BR_Util.h"
#include "../reaper/localize.h"

// Globals
static bool g_convertMarkersToTempoDialog = false;
static bool g_selectAdjustTempoDialog = false;
static bool g_tempoShapeDialog = false;
double g_tempoShapeSplitRatio = -1;
int g_tempoShapeSplitMiddle = -1;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//							Commands
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MoveTempo (COMMAND_T* ct)
{
	// Get tempo map
	BR_EnvChunk tempoMap(GetTempoEnv());
	if (tempoMap.Count() <= 1)
		return;

	// Get selected points/closest point
	vector<int> points;
	double cursor = GetCursorPositionEx(NULL);
	if (ct->user == 3)
	{
		// Get first point behind cursor
		int id = tempoMap.FindPrevious(cursor);
		double pos1;
		if (!tempoMap.GetPoint(id, &pos1, NULL, NULL, NULL))
			return;

		// Compare with the next point to find the closest
		double pos2;
		if (tempoMap.GetPoint(id+1, &pos2, NULL, NULL, NULL))
		{
			double len1 = cursor - pos1;
			double len2 = pos2 - cursor;

			if (len1 >= len2 || id == 0)	// if cursor is equally spaced between two points or first point
				points.push_back(id+1);		// (which can't be moved) is the closest, move the point to the right
			else
				points.push_back(id);
		}
		else
			points.push_back(id);
	}
	else
		points = *tempoMap.GetSelected();

	// Get amount of movement to be performed on the selected tempo points
	double tDiff;
	if (ct->user == 2 || ct->user == -2)
	{
		tDiff = 1 / GetHZoomLevel() * (double)ct->user / 2;
	}
	else if (ct->user == 3)
	{
		double cTime;
		tempoMap.GetPoint(points.front(), &cTime, NULL, NULL, NULL);
		tDiff = cursor - cTime;
		if (tDiff == 0)
			return;
	}
	else
	{
		tDiff = (double)ct->user/10000;
	}
	
	// Loop through selected points
	int skipped	= 0;
	for (size_t i = 0; i < points.size(); ++i)
	{
		int id = points[i];
		if (id == 0) // Skip first tempo point
			continue;

		// Get tempo points
		double t1, t2, t3;
		double b1, b2, b3, Nb1, Nb2;
		int s1, s2;

		tempoMap.GetPoint(id-1, &t1, &b1, &s1, NULL);
		tempoMap.GetPoint(id,   &t2, &b2, &s2, NULL);
		bool P3 = tempoMap.GetPoint(id+1, &t3, &b3, NULL, NULL);
		double Nt2 = t2+tDiff;

		///// CALCULATE BPM VALUES /////
		////////////////////////////////	
			
		// Current point
		if (s2 == 1)
			Nb2 = b2*(t3-t2) / (t3-Nt2);
		else
			Nb2 = (b2+b3)*(t3-t2) / (t3-Nt2) - b3;

		// Corner case when the last point is selected
		if (!P3)
		{
			Nb2 = b2; 		// b2 stays the same
			t3 = Nt2 + 1;	// t3 is faked so it can pass legality check
		}

		// Previous point
		if (s1 == 1)
			Nb1 = b1*(t2-t1) / (Nt2-t1);
		else
			Nb1 = (b1+b2)*(t2-t1) / (Nt2-t1) - Nb2;

		// Check new BPM values are legal
		if (Nb2 < MIN_BPM || Nb2 > MAX_BPM || Nb1 < MIN_BPM || Nb1 > MAX_BPM)
			SKIP(skipped, 1);

		// Check new time position is legal
		if ((Nt2-t1) < MIN_TEMPO_DIST || (t3 - Nt2) < MIN_TEMPO_DIST)
			SKIP(skipped, 1);

		///// CHECK POINTS BEFORE PREVIOUS POINT /////
		/////////////////////////////////////////////		
		
		int direction = -1, prevID = id - 2;
		bool doP0 = true, possibleP0 = true; 
		
		// Go through points backwards looking for linear points
		while (true) 
		{
			double b; int s;
			bool P = tempoMap.GetPoint(prevID, NULL, &b, &s, NULL);
			direction *= -1;
			
			// If current points in the loop is square or does not exist, break
			if (s == 1 || !P) 
			{
				if (prevID == id - 2)	// If first point behind previous is square there is no need to adjust anything
					doP0 = false;
				direction *= -1;		// Correct direction and ID since this point isn't taken into account
				prevID++;
				break;
			}		

			// Otherwise check if new BPM is possible
			double newBpm = b - direction*(Nb1 - b1);
			if (newBpm > MAX_BPM || newBpm < MIN_BPM)
			{
				possibleP0 = false;
				break;
			}			
			prevID--;
		}
		
		// Check if previous points can be adjusted
		if (!possibleP0)
			SKIP(skipped, 1);
		
		///// SET NEW BPM VALUES /////
		/////////////////////////////
				
		// Points before previous (if needed)
		if (doP0)
		{	
			for (int i = prevID; i <= id-2 ; ++i)
			{
				double b;
				tempoMap.GetPoint(i, NULL, &b, NULL, NULL);
				b -=direction*(Nb1-b1); 
				tempoMap.SetPoint(i, NULL, &b, NULL, NULL);
				direction *= -1;
			}
		}

		// Previous point
		tempoMap.SetPoint(id-1, NULL, &Nb1, NULL, NULL);
					
		// Selected point
		tempoMap.SetPoint(id, &Nt2, &Nb2, NULL, NULL);	
	}	
	
	// Commit changes
	PreventUIRefresh(1); // prevent jumpy cursor
	if (tempoMap.Commit())
	{
		if (ct->user == 3)
			SetEditCurPos2(NULL, cursor, false, false); // Keep cursor position when moving to closest tempo marker
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL, -1);
	}
	PreventUIRefresh(-1);		

	// Warn user if some points weren't processed
	static bool s_warnUser = true;
	if (s_warnUser && skipped != 0 && (points.size() > 1 || ct->user == 3))
	{
		char buffer[512];
		_snprintf(buffer, sizeof(buffer), __LOCALIZE_VERFMT("%d of the selected points didn't get processed because some points would end up with illegal BPM or position. Would you like to be warned if it happens again?", "sws_mbox"), skipped);
		int userAnswer = ShowMessageBox(buffer, __LOCALIZE("SWS - Warning", "sws_mbox"), 4);
		if (userAnswer == 7)
			s_warnUser = false;	
	}
};

void DeleteTempo (COMMAND_T* ct)
{
	// Get tempo map
	BR_EnvChunk tempoMap(GetTempoEnv());
	if (!tempoMap.GetSelected()->size())
		return;
	int offset = 0;

	// Loop through selected points and perform BPM calculations
	int skipped = 0;
	for (size_t i = 0; i < tempoMap.GetSelected()->size(); ++i)
	{
		int id = tempoMap.GetSelected()->at(i) + offset;		
		if (id == 0)
			continue;

		// Get tempo points
		double t1, t2, t3, t4;
		double b1, b2, b3, b4;
		int s0, s1, s2, s3;
	
		tempoMap.GetPoint(id, &t2, &b2, &s2, NULL);
		bool P0 = tempoMap.GetPoint(id-2, NULL, NULL, &s0, NULL);
		bool P1 = tempoMap.GetPoint(id-1, &t1, &b1, &s1, NULL);
		bool P3 = tempoMap.GetPoint(id+1, &t3, &b3, &s3, NULL);
		bool P4 = tempoMap.GetPoint(id+2, &t4, &b4, NULL, NULL);

		// Hold new values
		double Nt1, Nb1;
		double Nt3, Nb3;

		// If previous point doesn't exist, fake it as square
		if (!P0)
			s0 = 1;
				
		// Get P2-P3 length
		double m2;
		if (s2 == 1)
			m2 = b2*(t3-t2) / 240;
		else
			m2 = (b2+b3)*(t3-t2) / 480;


		///// CALCULATE BPM VALUES /////
		////////////////////////////////
		if (P3)
		{
			// If point behind previous is square...
			if (s0 == 1)
			{
				// If previous point is square...
				if (s1 == 1)
				{
					Nt1 = t1;
					Nb1 = (240*m2 + b1*(t2-t1)) / (t3-t1);
				}

				// If previous point is linear...
				else
				{
					Nt1 = t1;
					Nb1 = (480*m2 + (b1+b2)*(t2-t1)) / (t3-t1) - b3;
				}

				// Check new value is legal
				if (Nb1 > MAX_BPM || Nb1 < MIN_BPM)
					SKIP(skipped, 1);

				// Next point stays the same
				P3 = false;
			}

			// If point behind previous is linear...
			else
			{
				// If P4 exists...
				if (P4)
				{
					// If previous point is square...
					if (s1 == 1)
					{
						// If next point is square...
						if (s3 == 1)
						{
							Nt3 = t2 + 240*m2 / b1;
							Nb3 = b3*(t4-t3) / (t4-Nt3);
						}

						// If next is linear...
						else
						{
							Nt3 = t2 + 240*m2 / b1;
							Nb3 = (b3+b4)*(t4-t3) / (t4-Nt3) - b4;
						}
					}

					// If previous point is linear...
					else
					{
						// If next point is square...
						if (s3 == 1)
						{
							double f1 = (b1+b2)*(t2-t1) + 480*m2; 
							double f2 = b3*(t4-t3);
							double a = b1;
							double b = (a*(t1+t4) + f1+f2) / 2;
							double c = a*(t1*t4) + f1*t4 + f2*t1;
							Nt3 = c / (b + sqrt(pow(b,2) - a*c));
							Nb3 = f2 / (t4 - Nt3);	
						}	
	
						// If next point is linear...
						else
						{
							double f1 = (b1+b2)*(t2-t1) + 480*m2; 
							double f2 = (b3+b4)*(t4-t3);
							double a = b1-b4;
							double b = (a*(t1+t4) + f1+f2) / 2;
							double c = a*(t1*t4) + f1*t4 + f2*t1;
							Nt3 = c / (b + sqrt(pow(b,2) - a*c));
							Nb3 = f2 / (t4 - Nt3) - b4;	
						}			
					}

					// Check new position is legal
					if ((Nt3 - t1) < MIN_TEMPO_DIST || (t4 - Nt3) < MIN_TEMPO_DIST)
						SKIP(skipped, 1);
				}

				// If P4 does not exist
				else
				{
					// If previous point is square...
					if (s1 == 1)
					{
						Nt3 = t2 + 240*m2 / b1;
						Nb3 = b3;
					}

					// If previous point is linear...
					else
					{
						Nt3 = t3;
						Nb3 = (480*m2 + (b1+b2)*(t2-t1)) / (t3-t1) - b1;
					}

					// Check new position is legal
					if ((Nt3 - t1) < MIN_TEMPO_DIST)
						SKIP(skipped, 1);
				}

				// Check new value is legal
				if (Nb3 > MAX_BPM || Nb3 < MIN_BPM)
					SKIP(skipped, 1);

				// Previous point stays the same
				P1 = false;
			}
		}
		else
		{
			// No surrounding points get edited
			P1 = false;
			P3 = false;
		}

		///// SET NEW BPM /////	
		///////////////////////

		// Previous point
		if (P1)
			tempoMap.SetPoint(id-1, &Nt1, &Nb1, NULL, NULL);					
			
		// Next point
		if (P3)
			tempoMap.SetPoint(id+1, &Nt3, &Nb3, NULL, NULL);
					
		// Delete point
		tempoMap.DeletePoint(id);			
	}
	
	// Commit changes
	if (tempoMap.Commit())
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL, -1);

	// Warn user if some points weren't processed
	static bool s_warnUser = true;
	if (s_warnUser && skipped != 0)
	{
		char buffer[512];
		_snprintf(buffer, sizeof(buffer), __LOCALIZE_VERFMT("%d of the selected points didn't get processed because some points would end up with illegal BPM or position. Would you like to be warned if it happens again?", "sws_mbox"), skipped);
		int userAnswer = ShowMessageBox(buffer, __LOCALIZE("SWS - Warning", "sws_mbox"), 4);
		if (userAnswer == 7)
			s_warnUser = false;	
	}
};

void EditTempo (COMMAND_T* ct)
{
	// Get tempo map
	BR_EnvChunk tempoMap(GetTempoEnv());
	if (!tempoMap.GetSelected()->size())
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
	int skipped	= 0;
	for (size_t i = 0; i < tempoMap.GetSelected()->size(); ++i)
	{
		int id = tempoMap.GetSelected()->at(i);

		// Hold new values for selected and surrounding points
		double Nt1, Nt3, Nb1, Nb3;
		vector<double> selPos;
		vector<double> selBpm;

		///// CURRENT POINT /////
		/////////////////////////

		// Get all consequentially selected points into a vector with their new values. In case there is consequential selection, middle
		// musical position of a transition between first and last selected point will preserve it's time position (where possible)
		int offset = 0;
		if (tempoMap.GetSelection(id+1))
		{
			// Store new values for selected points into vectors
			double musicalMiddle = 0;
			vector<double> musicalLength;
			while (true)
			{
				// Once unselected point is encountered, break
				if (!tempoMap.GetSelection(id+offset))
				{
					--offset;	 // since this point is not taken into account, correct offset
					i += offset; // In case of consequential selection, points are threated as one big transition, so skip them all
					break;
				}

				// Get point currently in the loop and surrounding points
				double t0, t1, t2, b0, b1, b2;
				int s0, s1;
				bool P0 = tempoMap.GetPoint(id+offset-1, &t0, &b0, &s0, NULL);
				tempoMap.GetPoint(id+offset,   &t1, &b1, &s1, NULL);
				tempoMap.GetPoint(id+offset+1, &t2, &b2, NULL, NULL);
	
				// Get musical length for every transition but the last
				double mLen = 0;
				if (tempoMap.GetSelection(id+offset+1))
				{
					if (s1 == 1)
						mLen = b1*(t2-t1) / 240; 
					else
						mLen = (b1+b2)*(t2-t1) / 480;
				}
				musicalMiddle += mLen;
	
				// Get new BPM
				double bpm = b1 + (percentage ? (b1*diff) : (diff));
				if (bpm < MIN_BPM)
					bpm = MIN_BPM;
				else if (bpm > MAX_BPM)
					bpm = MAX_BPM;
	
				// Get new position (special case for the first point)
				double position;
				if (!offset)
				{
					if (P0 && s0 == 0)
						position = (t1*(b0+b1) + t0*(bpm-b1)) / (b0 + bpm);
					else
						position = t1;
				}
				else
				{
					if (s0 == 0)
						position = (480*musicalLength.back() + selPos.back()*(bpm + selBpm.back())) / (bpm + selBpm.back());
					else
						position = (240*musicalLength.back() + selPos.back()*selBpm.back()) / selBpm.back();
				}

				// Store new values
				selPos.push_back(position);
				selBpm.push_back(bpm);
				musicalLength.push_back(mLen);
				++offset;
			}
									
			// Find time position of musical middle and move all point so time position of musical middle is preserved (only if previous point exists)
			musicalMiddle /= 2;	
			if (tempoMap.GetPoint(id-1, NULL, NULL, NULL, NULL))
			{
				double temp = 0;
				for (int i = 0; i < (int)selPos.size()-1; ++i)
				{
					temp += musicalLength[i];
					if (temp >= musicalMiddle)
					{
						// Get length between the points that contain musical middle
						double len = musicalMiddle - (temp-musicalLength[i]);
	
						// Find original time position of musical middle
						double t0, t1, b0, b1; int s0;
						tempoMap.GetPoint(id+i,   &t0, &b0, &s0, NULL);
						tempoMap.GetPoint(id+i+1, &t1, &b1, NULL, NULL);	
						double prevPos = t0 + PositionAtMeasure (b0, (s0 == 1 ? b0 : b1), t1-t0, len);
	
						// Find new time position of musical middle
						double newPos = t0 + PositionAtMeasure (selBpm[i], (s0 == 1 ? selBpm[i] : selBpm[i+1]), selPos[i+1] - selPos[i], len);
	
						// Reset time positions of selected points
						double diff = newPos - prevPos;
						for (size_t i = 0; i < selPos.size(); ++i)
							selPos[i] -= diff;	
						break;
					}
				}
			}
		}
		else
		{
			// Get selected point
			double t, b;
			int s;
			tempoMap.GetPoint(id, &t, &b, &s, NULL);

			// Get new BPM
			b += percentage ? (b*diff) : (diff);
			if (b < MIN_BPM)
				b = MIN_BPM;
			else if (b > MAX_BPM)
				b = MAX_BPM;

			// Store it
			selPos.push_back(t);
			selBpm.push_back(b);		
		}

		///// PREVIOUS POINT /////
		//////////////////////////

		// Get points before selected points
		double t0, t1, b0, b1;
		int s0, s1;		
		bool P0 = tempoMap.GetPoint(id-2, &t0, &b0, &s0, NULL);
		bool P1 = tempoMap.GetPoint(id-1, &t1, &b1, &s1, NULL);

		if (P1)
		{
			// Get first selected point (old and new)
			double t2, b2;
			tempoMap.GetPoint(id, &t2, &b2, NULL, NULL);		
			double Nb2 = selBpm.front();
			double Nt2 = selPos.front();

			// If point behind previous doesn't exist, fake it
			if (!P0)
				s0 = 1;

			// If point behind previous is square...
			if (s0 == 1)
			{
				// If previous point is square...
				if (s1 == 1)
				{
					Nt1 = t1;
					Nb1 = b1*(t2-t1) / (Nt2-Nt1);
				}
		
				// If previous point is linear...
				else
				{
					Nt1 = t1;
					Nb1 = (b1+b2)*(t2-t1) / (Nt2-Nt1) - Nb2;
				}
			} 
	
			// If point behind previous is linear...
			else
			{
				// If previous point is square...
				if (s1 == 1)
				{
					double f1 = (b0+b1) *(t1-t0); 
					double f2 = b1*(t2-t1);
					double a = b0;
					double b = (a*(t0+Nt2) + f1+f2) / 2;
					double c = a*(t0*Nt2) + f1*Nt2 + f2*t0;
					Nt1 = c / (b + sqrt(pow(b,2) - a*c));
					Nb1 = f2 / (Nt2 - Nt1);		
				}
	
				// If previous point is linear...
				else
				{
					double f1 = (b0+b1)*(t1-t0); 
					double f2 = (b1+b2)*(t2-t1);
					double a = b0 - Nb2;
					double b = (a*(t0+Nt2) + f1+f2) / 2;
					double c = a*(t0*Nt2) + f1*Nt2 + f2*t0;
					Nt1 = c / (b + sqrt(pow(b,2) - a*c));
					Nb1 = f2 / (Nt2 - Nt1) - Nb2;
				}
			}

			// If point behind previous doesn't exist, fake it's position so it can pass legality check
			if (!P0)
				t0 = Nt1 - 1;

			// Check new value is legal
			if (Nb1 > MAX_BPM || Nb1 < MIN_BPM)
				SKIP(skipped, offset+1);		
			if ((Nt1-t0) < MIN_TEMPO_DIST || (Nt2 - Nt1) < MIN_TEMPO_DIST)
				SKIP(skipped, offset+1 );
		}

		///// NEXT POINT /////
		//////////////////////

		// Get points after selected points
		double t3, t4, b3, b4;
		int s3;		
		bool P3 = tempoMap.GetPoint(id+offset+1, &t3, &b3, &s3, NULL);
		bool P4 = tempoMap.GetPoint(id+offset+2, &t4, &b4, NULL, NULL);

		if (P3)
		{
			// Get last selected point (old and new)
			double t2, b2; int s2;
			tempoMap.GetPoint(id+offset, &t2, &b2, &s2, NULL);			
			double Nb2 = selBpm.back();
			double Nt2 = selPos.back();
			
			// If last selected point is square...
			if (s2 == 1)
			{
				if (P4)
				{
					// If next point is square...
					if (s3 == 1)
					{
						Nt3 = (b2*(t3-t2) + Nb2*Nt2) / Nb2;
						Nb3 = b3*(t4-t3) / (t4-Nt3);			
					}
	
					// If next point is linear...
					else
					{
						Nt3 = (b2*(t3-t2) + Nb2*Nt2) / Nb2;
						Nb3 = (b3+b4)*(t4-t3) / (t4-Nt3) - b4;
					}
				}
				else
				{
					Nt3 = (b2*(t3-t2) + Nb2*Nt2) / Nb2;
					Nb3 = b3;
				}
			}
	
			// If last selected point is linear...
			else
			{
				if (P4)
				{
					// If next point is square...
					if (s3 == 1)
					{
						double f1 = (b2+b3)*(t3-t2);
						double f2 = b3*(t4-t3);
						double a = Nb2;
						double b = (a*(Nt2+t4) + f1+f2) / 2;
						double c = a*(Nt2*t4) + f1*t4 + f2*Nt2;
						Nt3 = c / (b + sqrt(pow(b,2) - a*c));
						Nb3 = f2 / (t4-Nt3);
					}
	
					// If next point is linear...
					else
					{
						double f1 = (b2+b3)*(t3-t2);
						double f2 = (b3+b4)*(t4-t3);
						double a = Nb2 - b4;
						double b = (a*(Nt2+t4) + f1+f2) / 2;
						double c = a*(Nt2*t4) + f1*t4 + f2*Nt2;
						Nt3 = c / (b + sqrt(pow(b,2) - a*c));
						Nb3 = f2 / (t4-Nt3) - b4;
					}
				}
				else
				{
					Nt3 = t3;
					Nb3 = (b2+b3)*(t3-t2) / (Nt3-Nt2) - Nb2;
				}	
			}

			// If point after the next doesn't exist fake it's position so it can pass legality check
			if (!P4)
				t4 = Nt3 + 1;

			// Check new value is legal
			if (Nb3 > MAX_BPM || Nb3 < MIN_BPM)
				SKIP(skipped, offset+1);		
			if ((Nt3-Nt2) < MIN_TEMPO_DIST || (t4 - Nt3) < MIN_TEMPO_DIST)
				SKIP(skipped, offset+1);
		}

		///// SET BPM /////
		///////////////////

		// Previous point
		if (P1)
			tempoMap.SetPoint(id-1, &Nt1, &Nb1, NULL, NULL);

		// Current point
		for (int i = 0; i < (int)selPos.size(); ++i)
			tempoMap.SetPoint(id+i, &selPos[i], &selBpm[i], NULL, NULL);

		if (P3)
			tempoMap.SetPoint(id+offset+1, &Nt3, &Nb3, NULL, NULL);
	}
	
	// Commit changes
	if (tempoMap.Commit())
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL, -1);

	// Warn user if some points weren't processed
	static bool s_warnUser = true;
	if (s_warnUser && skipped != 0 && tempoMap.GetSelected()->size() > 1)
	{
		char buffer[512];
		_snprintf(buffer, sizeof(buffer), __LOCALIZE_VERFMT("%d of the selected points didn't get processed because some points would end up with illegal BPM or position. Would you like to be warned if it happens again?", "sws_mbox"), skipped);
		int userAnswer = ShowMessageBox(buffer, __LOCALIZE("SWS - Warning", "sws_mbox"), 4);
		if (userAnswer == 7)
			s_warnUser = false;	
	}
};

void EditTempoGradual (COMMAND_T* ct)
{
	// Get tempo map
	BR_EnvChunk tempoMap(GetTempoEnv());
	if (!tempoMap.GetSelected()->size())
		return;

	// Struct for selected points. mLen refers to the musical length from this point to the next
	struct TempoPoint
	{
		double pos;
		double bpm;
	};

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
	int skipped	= 0;
	for (size_t i = 0; i < tempoMap.GetSelected()->size(); ++i)
	{
		int id = tempoMap.GetSelected()->at(i);

		// Hold new values for selected and next point
		double Nb1, Nt1;
		vector<double> selPos;
		vector<double> selBpm;

		///// CURRENT POINT /////
		/////////////////////////

		// Store new values for selected points into vectors
		int offset = 0;
		while (true)
		{
			// Get point currently in the loop and surrounding points
			double t0, t1, t2, b0, b1, b2;
			int s0, s1;
			bool P0 = tempoMap.GetPoint(id+offset-1, &t0, &b0, &s0, NULL);
			tempoMap.GetPoint(id+offset,   &t1, &b1, &s1, NULL);
			tempoMap.GetPoint(id+offset+1, &t2, &b2, NULL, NULL);
			
			// If square or not selected, break
			if (s1 == 1 || !tempoMap.GetSelection(id+offset))
			{
				// If first selected point is square, don't adjust i (so for loop doesn't go backwards) and let offset be -1
				if (!offset)
					--offset;
				else
				{
					--offset;	 // since this point is not taken into account, correct offset
					i += offset; // In case of consequential selection, points are threated as one big transition, so skip them all
				}
				break;
			}

			// Get new BPM
			double bpm = b1 + (percentage ? (b1*diff) : (diff));
			if (bpm < MIN_BPM)
				bpm = MIN_BPM;
			else if (bpm > MAX_BPM)
				bpm = MAX_BPM;

			// Get new position (special case for the first point)
			double position;
			if (!offset)
			{
				if (P0 && s0 == 0)
				{
					position = (t1*(b0+b1) + t0*(bpm-b1)) / (b0 + bpm);
					if (position - t0 < MIN_TEMPO_DIST) // Since point moves, check if position is legal
						break;
				}
				else
					position = t1;
			}
			else
				position = ((b0+b1)*(t1-t0) + selPos.back() * (selBpm.back() + bpm)) / (selBpm.back() + bpm);

			// Store new values
			selPos.push_back(position);
			selBpm.push_back(bpm);
			++offset;
		}

		// No linear points encountered (in that case offset is -1) / illegal position encountered?
		if (!selPos.size())
			SKIP(skipped, offset+1);
		
		///// NEXT POINT /////
		//////////////////////

		// Get points after last selected point
		double t1, t2;
		double b1, b2;
		int s2;
		bool P1 = tempoMap.GetPoint(id+offset+1, &t1, &b1, &s2, NULL);
		bool P2 = tempoMap.GetPoint(id+offset+2, &t2, &b2, NULL, NULL);
	
		// If next point exists...
		if (P1)
		{	
			// Get last selected tempo point (old and new)	
			double Nb0 = selBpm.back();
			double Nt0 = selPos.back();
			double t0, b0;
			tempoMap.GetPoint(id+offset, &t0, &b0, NULL, NULL);

			// If point after next point exists...
			if (P2)
			{
				// If next point is square...
				if (s2 == 1)
				{
					double f1 = (b0+b1)*(t1-t0); 
					double f2 = b1*(t2-t1);
					double a = Nb0;
					double b = (a*(Nt0+t2) + f1+f2) / 2;
					double c = a*(Nt0*t2) + f1*t2 + f2*Nt0;
					Nt1 = c / (b + sqrt(pow(b,2) - a*c));
					Nb1 = f2 / (t2-Nt1);	
				}

				// If next point is linear...
				else
				{
					double f1 = (b0+b1)*(t1-t0); 
					double f2 = (b1+b2)*(t2-t1);
					double a = Nb0 - b2;
					double b = (a*(Nt0+t2) + f1+f2) / 2;
					double c = a*(Nt0*t2) + f1*t2 + f2*Nt0;
					Nt1 = c / (b + sqrt(pow(b,2) - a*c));
					Nb1 = f2 / (t2-Nt1) - b2;	
				}
			}
	
			// If point after next point doesn't exist...
			else
			{
				Nt1 = t1;
				Nb1 = (b0+b1)*(t1-t0) / (t1-Nt0) - Nb0;
			}

			// If points after selected point don't exist, fake them
			if (!P1)
				Nt1 = Nt0 + 1;
			if (!P2)
				t2 = Nt1 + 1;

			// Check new value is legal
			if (Nb1 > MAX_BPM || Nb1 < MIN_BPM)
				SKIP(skipped, offset+1);		
			if ((Nt1-Nt0) < MIN_TEMPO_DIST || (t2 - Nt1) < MIN_TEMPO_DIST)
				SKIP(skipped, offset+1);
		}

		///// SET NEW BPM /////	
		///////////////////////

		// Current point(s)
		for (int i = 0; i < (int)selPos.size(); ++i)
			tempoMap.SetPoint(id+i, &selPos[i], &selBpm[i], NULL, NULL);
	
		// Next point
		if (P1)
			tempoMap.SetPoint(id+offset+1, &Nt1, &Nb1, NULL, NULL);
	}

	// Commit changes
	if (tempoMap.Commit())
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL, -1);

	// Warn user if some points weren't processed
	static bool s_warnUser = true;
	if (s_warnUser && skipped != 0 && tempoMap.GetSelected()->size() > 1 )
	{
		char buffer[512];
		_snprintf(buffer, sizeof(buffer), __LOCALIZE_VERFMT("%d of the selected points didn't get processed because some points would end up with illegal BPM or position. Would you like to be warned if it happens again?", "sws_mbox"), skipped);
		int userAnswer = ShowMessageBox(buffer, __LOCALIZE("SWS - Warning", "sws_mbox"), 4);
		if (userAnswer == 7)
			s_warnUser = false;	
	}
};

void TempoShapeLinear (COMMAND_T* ct)
{
	// Get tempo map (with effCreate mode)
	BR_EnvChunk tempoMap (GetTempoEnv(), true);
	if (!tempoMap.GetSelected()->size())
		return;
	
	// Get splitting options
	if (g_tempoShapeSplitRatio == -1 || g_tempoShapeSplitMiddle == -1)
	{
		int split; char splitRatio[128];
		LoadOptionsTempoShape (split, splitRatio);
		SetTempoShapeGlobalVariable (split, splitRatio);
	}

	// Check if middle point is to be split
	bool split = false;
	if (g_tempoShapeSplitMiddle == 0)
		split = false;
	else if (g_tempoShapeSplitMiddle == 1)
		if (g_tempoShapeSplitRatio != 0)
			split = true;

	// Loop through selected points and perform BPM calculations
	int skipped = 0;
	int count = tempoMap.Count()-1;
	for (size_t i = 0; i < tempoMap.GetSelected()->size(); ++i)
	{
		int id = tempoMap.GetSelected()->at(i);
		
		// Skip selected point if already linear
		double t0, b0; int s0;		
		tempoMap.GetPoint(id, &t0, &b0, &s0, NULL);
		if (s0 == 0)
			continue;
		else
			s0 = 0;

		// Get next point 
		double t1, b1; bool P1;
		if (id < count)	// (since we're using effCreate this will let us know if we are dealing with the last point)
			P1 = tempoMap.GetPoint(id+1, &t1, &b1, NULL, NULL);
 		else
			P1 = false;

		// Create middle point(s) if needed
		if (P1 && b0 != b1)
		{
			// Get middle point's position and BPM
			double position, bpm, measure = b0*(t1-t0) / 240;
			FindMiddlePoint(position, bpm, measure, b0, b1, t0, t1);

			// Don't split middle point
			if (!split)
			{
				// Check if value and position is legal, if not, skip
				if (bpm>=MIN_BPM && bpm<=MAX_BPM && (position-t0)>=MIN_TEMPO_DIST && (t1-position)>=MIN_TEMPO_DIST)
					tempoMap.CreatePoint(0, position, bpm, 0, 0, false);
				else
					SKIP(skipped, 1);
			}

			// Split middle point
			else
			{
				double position1, position2, bpm1, bpm2;
				SplitMiddlePoint (position1, position2, bpm1, bpm2, g_tempoShapeSplitRatio, measure, b0, bpm, b1, t0, position, t1);
				
				// Check if value and position is legal, if not, skip
				if (bpm1>=MIN_BPM && bpm1<=MAX_BPM && bpm2>=MIN_BPM && bpm2<=MAX_BPM && (position1-t0)>=MIN_TEMPO_DIST && (position2-position1)>=MIN_TEMPO_DIST && (t1-position2)>=MIN_TEMPO_DIST)
				{
					tempoMap.CreatePoint(0, position1, bpm1, 0, 0, false);
					tempoMap.CreatePoint(0, position2, bpm2, 0, 0, false);
				}
				else
					SKIP(skipped, 1);
			}
		}

		// Change shape of the selected point
		tempoMap.SetPoint(id, NULL, NULL, &s0, NULL);
	}

	// Commit changes
	if (tempoMap.Commit())
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL, -1);

	// Warn user if some points weren't processed
	static bool s_warnUser = true;
	if (s_warnUser && skipped != 0)
	{
		char buffer[512];
		_snprintf(buffer, sizeof(buffer), __LOCALIZE_VERFMT("%d of the selected points didn't get processed because some points would end up with illegal BPM or position. Would you like to be warned if it happens again?", "sws_mbox"), skipped);
		int userAnswer = ShowMessageBox(buffer, __LOCALIZE("SWS - Warning", "sws_mbox"), 4);
		if (userAnswer == 7)
			s_warnUser = false;	
	}
};

void TempoShapeSquare (COMMAND_T* ct)
{
	// Get tempo map (with effCreate mode)
	BR_EnvChunk tempoMap (GetTempoEnv(), true);
	if (!tempoMap.GetSelected()->size())
		return;

	// Get splitting options
	if (g_tempoShapeSplitRatio == -1 || g_tempoShapeSplitMiddle == -1)
	{
		int split; char splitRatio[128];
		LoadOptionsTempoShape (split, splitRatio);
		SetTempoShapeGlobalVariable (split, splitRatio);
	}

	// Check if middle point is to be split
	bool split = false;
	if (g_tempoShapeSplitMiddle == 0)
		split = false;
	else if (g_tempoShapeSplitMiddle == 1)
		if (g_tempoShapeSplitRatio != 0)
			split = true;
	
	// Loop through selected points and perform BPM calculations
	int skipped = 0;
	int count = tempoMap.Count()-1;
	for (size_t i = 0; i < tempoMap.GetSelected()->size(); ++i)
	{
		int id = tempoMap.GetSelected()->at(i);
				
		// Skip selected point if already square
		double t1, b1; int s1;
		tempoMap.GetPoint(id, &t1, &b1, &s1, NULL);
		if (s1 == 1) 
			continue;
		else
			s1 = 1;
		
		// Get next point
		double b2; bool P2;
		if (id < count) // (since we're using effCreate this will let us know if we are dealing with the last point)
			P2 = tempoMap.GetPoint(id+1, NULL, &b2, NULL, NULL);
		else
			P2 = false;
		
		// Get previous point
		double t0, b0; int s0;
		bool P0 = tempoMap.GetPoint(id-1, &t0, &b0, &s0, NULL);

		// Get new bpm of selected point
		double Nb1;
		if (P2 && b1 != b2)
			Nb1 = (b1+b2) / 2;
		else
			Nb1 = b1;

		// Check if new bpm is legal, if not, skip
		if (Nb1 < MIN_BPM || Nb1 > MAX_BPM)
			SKIP(skipped, 1);

		///// SET NEW SHAPE /////
		/////////////////////////

		// Create middle point(s) is needed
		if (P0 && s0 == 0 && P2 && Nb1 != b2)
		{
			// Get middle point's position and BPM
			double position, bpm = 120, measure = (b0+b1)*(t1-t0) / 480;
			FindMiddlePoint(position, bpm, measure, b0, Nb1, t0, t1);

			// Don't split middle point
			if (!split)
			{
				if (bpm<= MAX_BPM && bpm>=MIN_BPM && (position-t0)>=MIN_TEMPO_DIST && (t1-position)>=MIN_TEMPO_DIST)
					tempoMap.CreatePoint(0, position, bpm, 0, 0, false);
				else
					SKIP(skipped, 1);			
			}

			// Split middle point
			else
			{
				double position1, position2, bpm1, bpm2;
				SplitMiddlePoint (position1, position2, bpm1, bpm2, g_tempoShapeSplitRatio, measure, b0, bpm, Nb1, t0, position, t1);

				if (bpm1>=MIN_BPM && bpm1<=MAX_BPM && bpm2>=MIN_BPM && bpm2<=MAX_BPM && (position1-t0)>=MIN_TEMPO_DIST && (position2-position1)>=MIN_TEMPO_DIST && (t1-position2)>=MIN_TEMPO_DIST)
				{
					tempoMap.CreatePoint(0, position1, bpm1, 0, 0, false);
					tempoMap.CreatePoint(0, position2, bpm2, 0, 0, false);					
				}
				else
					SKIP(skipped, 1);
			}
		}

		// Change shape of the selected point
		tempoMap.SetPoint(id, NULL, &Nb1, &s1, NULL);
	}

	// Commit changes
	if (tempoMap.Commit())
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL, -1);

	// Warn user if some points weren't processed
	static bool s_warnUser = true;
	if (s_warnUser && skipped != 0)
	{
		char buffer[512];
		_snprintf(buffer, sizeof(buffer), __LOCALIZE_VERFMT("%d of the selected points didn't get processed because some points would end up with illegal BPM or position. Would you like to be warned if it happens again?", "sws_mbox"), skipped);
		int userAnswer = ShowMessageBox(buffer, __LOCALIZE("SWS - Warning", "sws_mbox"), 4);
		if (userAnswer == 7)
			s_warnUser = false;	
	}
};

void TempoAtGrid (COMMAND_T* ct)
{
	// Get tempo map (with effCreate mode) and grid
	BR_EnvChunk tempoMap(GetTempoEnv(), true);
	if (!tempoMap.GetSelected()->size())
		return;
	double grid = *(double*)GetConfigVar("projgriddiv");
	
	// Loop through selected points
	Undo_BeginBlock2(NULL);
	int count = tempoMap.Count()-1;
	for (size_t i = 0; i < tempoMap.GetSelected()->size(); ++i)
	{		
		// Get tempo points
		int id = tempoMap.GetSelected()->at(i);
		double t0, t1, b0, b1;
		int s0;	

		tempoMap.GetPoint(id, &t0, &b0, &s0, NULL);
		tempoMap.GetPoint(id+1, &t1, &b1, NULL, NULL);
		
		// If last tempo point is selected, fake second point as if it's at the end of project (markers and regions included)
		if (id == count)
		{
			b1 = b0;
			t1 = EndOfProject (true, true);
		}

		// If point is square, set "fake" bpm for second point (needed for TempoAtPosition)
		if (s0 == 1)
			b1 = b0;

		// This part is really only relevant in certain cases involving grid division longer than measure's length
		// Grid diving starts again from the start of the measure in which tempo marker is so we need to calculate 
		// the offset between where grid line really is and where it would be if we just divided QN with grid spacing.
		double beat; int measure; GetTempoTimeSigMarker(NULL, id, NULL, &measure, &beat, NULL, NULL, NULL, NULL); 
		double offset =  grid - fmod(TimeMap_timeToQN(TimeMap2_beatsToTime(0, 0, &measure)), grid);
		
		// Find first grid line and then loop through the rest creating tempo points
		double pGridLn = t0, gridLn = TimeMap_timeToQN(t0);
		gridLn = TimeMap_QNToTime(gridLn-offset - fmod(gridLn,grid));		// it can be before tempo point but next while should correct that
		while (true)
		{
			// Search for the next grid line								// max grid division of 1/256 at max tempo of 960 creates a grid line
			while (gridLn < pGridLn + MAX_GRID_DIV)							// every 0.00097 s... so let it also be our safety net to prevent accidental
				gridLn = TimeMap_QNToTime(TimeMap_timeToQN(gridLn) + grid);	// creation of multiple points around grid line and the end point of tempo transition
																			
			// Create points until the next point
			if (gridLn < t1 - MAX_GRID_DIV)
				tempoMap.CreatePoint(0, gridLn, TempoAtPosition(b0, b1, t0, t1, gridLn), s0, 0, false);
			else
				break;
			pGridLn = gridLn;
		}		
	}
	tempoMap.Commit();
	Undo_EndBlock2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//							Dialogs
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ConvertMarkersToTempoDialog (COMMAND_T* = NULL)
{
	static HWND hwnd = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_BR_MARKERS_TO_TEMPO), g_hwndParent, ConvertMarkersToTempoProc);

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
	RefreshToolbar(NamedCommandLookup("_SWS_BRCONVERTMARKERSTOTEMPO"));
};

void SelectAdjustTempoDialog (COMMAND_T* = NULL)
{
	static HWND hwnd = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_BR_SELECT_ADJUST_TEMPO), g_hwndParent, SelectAdjustTempoProc);
	
	if (g_selectAdjustTempoDialog)
	{	
		KillTimer(hwnd, 1);
		UnselectNthDialog(false, hwnd); // hide child dialog
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
	RefreshToolbar(NamedCommandLookup("_SWS_BRADJUSTSELTEMPO"));
};

void TempoShapeOptionsDialog (COMMAND_T* = NULL)
{
	static HWND hwnd = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_BR_TEMPO_SHAPE_OPTIONS), g_hwndParent, TempoShapeOptionsProc);

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
	RefreshToolbar(NamedCommandLookup("_BR_TEMPO_SHAPE_OPTIONS"));
};

void RandomizeTempoDialog (COMMAND_T* = NULL)
{
	DialogBox(g_hInst, MAKEINTRESOURCE(IDD_BR_RANDOMIZE_TEMPO), g_hwndParent, RandomizeTempoProc);
};

int IsConvertMarkersToTempoVisible (COMMAND_T* = NULL)
{
	return g_convertMarkersToTempoDialog;
};

int IsSelectAdjustTempoVisible (COMMAND_T* = NULL)
{
	return g_selectAdjustTempoDialog;
};

int IsTempoShapeOptionsVisible (COMMAND_T* = NULL)
{
	return g_tempoShapeDialog;
};