/******************************************************************************
/ sws_util.mm
/
/ Copyright (c) 2009 Tim Payne (SWS)

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

#import <Cocoa/Cocoa.h>
#include "stdafx.h"
#include "../WDL/swell/swell-internal.h"

void GetDateString(int time, char* buf, int bufsize)
{
	NSDate* macTime = [NSDate dateWithTimeIntervalSince1970:time];
	NSDateFormatter *macTimeFmt = [[[NSDateFormatter alloc] init] autorelease];
	[macTimeFmt setFormatterBehavior:NSDateFormatterBehavior10_4];
	[macTimeFmt setDateStyle:NSDateFormatterShortStyle];
	NSString* s = [macTimeFmt stringFromDate:macTime];
	[s getCString:buf maxLength:bufsize];
}

void GetTimeString(int time, char* buf, int bufsize)
{
	NSDate* macTime = [NSDate dateWithTimeIntervalSince1970:time];
	NSDateFormatter *macTimeFmt = [[[NSDateFormatter alloc] init] autorelease];
	[macTimeFmt setFormatterBehavior:NSDateFormatterBehavior10_4];
	[macTimeFmt setTimeStyle:NSDateFormatterShortStyle];
	NSString* s = [macTimeFmt stringFromDate:macTime];
	[s getCString:buf maxLength:bufsize];
}

void SetColumnArrows(HWND h, int iSortCol)
{
	if (!h || ![(id)h isKindOfClass:[SWELL_ListView class]]) return;
	SWELL_ListView *v=(SWELL_ListView *)h;
	if (!v->m_cols) return;
	for (int i = 0; i < v->m_cols->GetSize(); i++)
	{
		if (i == abs(iSortCol)-1)
		{
			if (iSortCol > 0)
				[v setIndicatorImage:[NSImage imageNamed:@"NSAscendingSortIndicator"] inTableColumn:v->m_cols->Get(i)];
			else
				[v setIndicatorImage:[NSImage imageNamed:@"NSDescendingSortIndicator"] inTableColumn:v->m_cols->Get(i)];
		}
		else
			[v setIndicatorImage:0 inTableColumn:v->m_cols->Get(i)];
	}			
}

int GetCustomColors()
{
	[[[NSColorPanel sharedColorPanel] valueForKey:@"_colorSwatch"] readColors];
	
	// Get the NSColorSwatch's internal mutable array of NSColors
	NSMutableArray *colors = [[[NSColorPanel sharedColorPanel] valueForKey:@"_colorSwatch"] valueForKey:@"colors"];
	
	NSEnumerator *e = [colors objectEnumerator];
	NSColor *color;
	while(color = [e nextObject])
	{
		if ([color colorSpace] == [NSColorSpace genericGrayColorSpace] && [color whiteComponent] == 1.0)
			continue;
//		[userSwatchColors addObject:color];
	}
		
	return 0;
}