#ifndef _TIME_MAP_H_
#define _TIME_MAP_H_

double TimeToBeat(double time);
double BeatToTime(double beat);
int TimeToMeasure(double time);
int BeatToMeasure(double beat);
double MeasureToTime(int measure);
int BeatsInMeasure(int measure);
double BeatsTillMeasure(int measure);
double BPMAtTime(double time);
double QNtoTime(double qn);
double TimeToQN(double t);
double BPMatTime(double t);

#endif /*_TIME_MAP_H_*/
