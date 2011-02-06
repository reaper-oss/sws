/******************************************************************************
/ RenderTrack.cpp
/
/ Copyright (c) 2011 Shane StClair
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
#include "RenderTrack.h"


RenderTrack::RenderTrack() { 
	entireProject = false;
	duplicateNumber = 0;
}

string RenderTrack::zeroPadInt(int num, int digits ){
    std::ostringstream ss;
    ss << setw( digits ) << setfill( '0' ) << num;
    return ss.str();
}

string RenderTrack::getPaddedTrackNumber( int padLength ){
	return zeroPadInt( trackNumber, padLength );
}

string RenderTrack::getDuplicateNumberString(){
    std::ostringstream ss;
    ss << duplicateNumber;
    return ss.str();
}

string RenderTrack::getFileName( string ext = "", int trackNumberPad = 2 ){
	string fileName = "";
	if( trackNumberPad ) fileName += getPaddedTrackNumber( trackNumberPad ) + " ";
	fileName += sanitizedTrackName;	
	if( duplicateNumber > 1 ) fileName += "(" + getDuplicateNumberString() + ")";
	if( !ext.empty() ) fileName += "." + ext;
	return fileName;
}