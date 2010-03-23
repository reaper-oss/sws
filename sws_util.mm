/******************************************************************************
/ sws_util.mm
/
/ Copyright (c) 2010 Tim Payne (SWS)

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

void SWS_GetDateString(int time, char* buf, int bufsize)
{
	NSDate* macTime = [NSDate dateWithTimeIntervalSince1970:time];
	NSDateFormatter *macTimeFmt = [[[NSDateFormatter alloc] init] autorelease];
	[macTimeFmt setFormatterBehavior:NSDateFormatterBehavior10_4];
	[macTimeFmt setDateStyle:NSDateFormatterShortStyle];
	NSString* s = [macTimeFmt stringFromDate:macTime];
	[s getCString:buf maxLength:bufsize];
}

void SWS_GetTimeString(int time, char* buf, int bufsize)
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

int GetCustomColors(COLORREF custColors[])
{
	NSColorPanel* cp = [NSColorPanel sharedColorPanel];
	[[cp valueForKey:@"_colorSwatch"] readColors];
	
	// Get the NSColorSwatch's internal mutable array of NSColors
	NSMutableArray *colors = [[cp valueForKey:@"_colorSwatch"] valueForKey:@"colors"];
	for (int i = 0; i < 16; i++)
	{
		NSColor* col = [colors objectAtIndex:i*10];
		col = [col colorUsingColorSpaceName:@"NSCalibratedRGBColorSpace"];
		
		custColors[i] = ((int)([col redComponent] * 255) << 16) | ((int)([col greenComponent] * 255) << 8) | (int)([col blueComponent] * 255);
	}
		
	return 0;
}

void SetCustomColors(COLORREF custColors[])
{
	NSColorPanel* cp = [NSColorPanel sharedColorPanel];
	[[cp valueForKey:@"_colorSwatch"] readColors];
	
	// Get the NSColorSwatch's internal mutable array of NSColors
	NSMutableArray *colors = [[cp valueForKey:@"_colorSwatch"] valueForKey:@"colors"];
	for (int i = 0; i < 16; i++)
	{
		NSColor* col = [NSColor colorWithCalibratedRed:(custColors[i]>>16)/255.0 green:((custColors[i]&0xFF00)>>8)/255.0 blue:(custColors[i]&0xFF)/255.0 alpha:1.0];
		[colors replaceObjectAtIndex:i*10 withObject:col];
	}
	[[cp valueForKey:@"_colorSwatch"] writeColors];
}

void ShowColorChooser(COLORREF initialCol)
{
	NSColorPanel *colorPanel = [NSColorPanel sharedColorPanel];
	NSColor* col = [NSColor colorWithCalibratedRed:(initialCol>>16)/255.0 green:((initialCol&0xFF00)>>8)/255.0 blue:(initialCol&0xFF)/255.0 alpha:1.0];
	[colorPanel setColor:col];
	[[NSApplication sharedApplication] orderFrontColorPanel:(id)g_hwndParent];
}

bool GetChosenColor(COLORREF* pColor)
{
	if (![[NSColorPanel sharedColorPanel] isVisible])
	{
		if (pColor)
		{
			NSColor* col = [[NSColorPanel sharedColorPanel] color];
			col = [col colorUsingColorSpaceName:@"NSCalibratedRGBColorSpace"];
			*pColor = ((int)([col redComponent] * 255) << 16) | ((int)([col greenComponent] * 255) << 8) | (int)([col blueComponent] * 255);
		}
		return true;
	}
	return false;
}
		
void HideColorChooser()
{
	ShowWindow((HWND)[NSColorPanel sharedColorPanel], SW_HIDE);
}

void EnableColumnResize(HWND h)
{
	if (!h || ![(id)h isKindOfClass:[SWELL_ListView class]]) return;
	SWELL_ListView *v=(SWELL_ListView *)h;
	[v setAllowsColumnResizing:YES];
}
