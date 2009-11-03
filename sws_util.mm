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

@interface SWS_ColorChooser : NSObject
{
	bool bChose;
}
-(bool)isChose;
-(void)colorPanelAction:(id)sender;
-(bool)ChooseColor;
@end

@implementation SWS_ColorChooser
- (void)colorPanelAction:(id)sender
{
	bChose = true;
}
-(bool)isChose { return bChose; }
-(bool)ChooseColor
{
	bChose = false;
	NSColorPanel *colorPanel = [NSColorPanel sharedColorPanel];
	[colorPanel setTarget:self];
	[colorPanel setAction:@selector(colorPanelAction:)];
	[[NSApplication sharedApplication] orderFrontColorPanel:self];
	return true;
}
@end

bool ChooseColor(COLORREF* pColor)
{
	SWS_ColorChooser* cc = [SWS_ColorChooser alloc];
	[cc ChooseColor];
	NSColor* col = [[NSColorPanel sharedColorPanel] color];
	*pColor = ((int)([col redComponent] * 255) << 16) | ((int)([col greenComponent] * 255) << 8) | (int)([col blueComponent] * 255);
	return false;
}
