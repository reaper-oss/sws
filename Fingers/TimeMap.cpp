#include "stdafx.h"

#include "TimeMap.h"

double TimeToBeat(double time)
{
    return TimeMap2_timeToBeats(0, time, NULL, NULL, NULL, NULL);
}

double BeatToTime(double beat)
{
    return TimeMap2_beatsToTime(0, beat, NULL);
}

int TimeToMeasure(double time)
{
    int measure = 0;
    TimeMap2_timeToBeats(0, time, &measure, NULL, NULL, NULL);
    return measure;
}

int BeatToMeasure(double beat)
{
    double time = BeatToTime(beat);
    return TimeToMeasure(time);
}

double MeasureToTime(int measure)
{
    return TimeMap2_beatsToTime(0, 0.0f, &measure);
}

int BeatsInMeasure(int measure)
{
    double time = MeasureToTime(measure);
    int measureLength = 0;
    TimeMap2_timeToBeats(0, time, &measure, &measureLength, NULL, NULL);
    return measureLength;
}

double BeatsTillMeasure(int measure)
{
    double time = MeasureToTime(measure);
    return TimeMap2_timeToBeats(0, time, NULL, NULL, NULL, NULL);
}

double BPMAtTime(double time)
{
    return TimeMap2_GetDividedBpmAtTime(0, time);
}

double QNtoTime(double qn)
{
    return TimeMap2_QNToTime(0, qn);
}
double TimeToQN(double t)
{
    return TimeMap2_timeToQN(0, t);
}

double BPMatTime(double t)
{
    return TimeMap2_GetDividedBpmAtTime(0, t);
}
