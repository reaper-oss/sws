/******************************************************************************
/ sws_util.mm
/
/ Copyright (c) 2010 Tim Payne (SWS), Dominik Martin Drzic

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
#include "../WDL/swell/swell-dlggen.h"


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

static int RGBfromNSColor(NSColor *col)
{
  int r = (int) ([col redComponent] * 255.0 + 0.5) ;
  int g = (int) ([col greenComponent] * 255.0 + 0.5) ;
  int b = (int) ([col blueComponent] * 255.0 + 0.5) ;
  if (r<0)r=0; else if (r>255) r=255;
  if (g<0)g=0; else if (g>255) g=255;
  if (b<0)b=0; else if (b>255) b=255;

  return (r << 16) | (g << 8) | b;
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

		custColors[i] = RGBfromNSColor(col);
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
			*pColor = RGBfromNSColor(col);
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

// Modified MakeCursorFromData from Cockos WDL (supports 32x32 unlike original function which supports 16x16)
static NSCursor* SWS_MakeCursorFromData(unsigned char* data, int hotspot_x, int hotspot_y)
{
  NSCursor *c=NULL;
  NSBitmapImageRep* bmp = [[NSBitmapImageRep alloc]
    initWithBitmapDataPlanes:0
    pixelsWide:32
    pixelsHigh:32
    bitsPerSample:8
    samplesPerPixel:2
    hasAlpha:YES
    isPlanar:NO
    colorSpaceName:NSCalibratedWhiteColorSpace
    bytesPerRow:(32*2)
    bitsPerPixel:16];

  if (bmp)
  {
    unsigned char* p = [bmp bitmapData];
    if (p)
    {
      int i;
      for (i = 0; i < 32*32; ++i)
      {
        // tried 4 bits per sample and memcpy, didn't work
        p[2*i] = (data[i]&0xF0) | data[i]>>4;
        p[2*i+1] = (data[i]<<4) | (data[i]&0xf);
      }

      NSImage *img = [[NSImage alloc] init];
      if (img)
      {
        [img addRepresentation:bmp];
        NSPoint hs = NSMakePoint(hotspot_x, hotspot_y);
        c = [[NSCursor alloc] initWithImage:img hotSpot:hs];
        [img release];
      }
    }
    [bmp release];
  }
  return c;
}

// Modified LoadCursor from Cockos WDL
// Supports these cursors:
// IDC_GRID_WARP
// IDC_ENV_PEN_GRID, IDC_ENV_PT_ADJ_VERT
// IDC_MISC_SPEAKER
// IDC_ZOOM_DRAG, IDC_ZOOM_IN, IDC_ZOOM_OUT, IDC_ZOOM_UNDO
HCURSOR SWS_LoadCursor(int id)
{
  // bytemaps are (white<<4)|(alpha)
  const unsigned char B = 0xF;
  const unsigned char W = 0xFF;
  //const unsigned char G = 0xF8;

  static NSCursor* carr[8]; // set to NULL by compiler

  NSCursor** pc=0;

  int  index = 0;
  bool found = false;


  if (!found && id == IDC_ENV_PEN_GRID)    found = true; else if (!found) ++index;
  if (!found && id == IDC_ENV_PT_ADJ_VERT) found = true; else if (!found) ++index;
  if (!found && id == IDC_GRID_WARP)       found = true; else if (!found) ++index;
  if (!found && id == IDC_MISC_SPEAKER)    found = true; else if (!found) ++index;
  if (!found && id == IDC_ZOOM_DRAG)       found = true; else if (!found) ++index;
  if (!found && id == IDC_ZOOM_IN)         found = true; else if (!found) ++index;
  if (!found && id == IDC_ZOOM_OUT)        found = true; else if (!found) ++index;
  if (!found && id == IDC_ZOOM_UNDO)       found = true; else if (!found) ++index;

  if (!found)
    return NULL;
  pc = &carr[index];

  if (!(*pc))
  {
    if (id == IDC_ENV_PEN_GRID)
    {
      static unsigned char p[32*32] =
      {
        0,0,0,0,0,0,0,0,0,0,0,0,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,B,W,B,0,0,0,B,0,0,B,0,0,B,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,B,W,W,B,B,0,0,B,0,0,B,0,0,B,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,B,W,W,B,B,0,0,0,B,0,0,B,0,0,B,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,B,W,W,B,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,B,W,W,B,B,0,0,0,0,0,B,0,0,B,0,0,B,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,B,W,W,B,B,0,0,0,0,0,0,B,0,0,B,0,0,B,0,0,0,0,0,0,0,0,
        0,0,0,0,0,B,W,W,B,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,B,W,W,B,B,0,0,0,0,0,0,0,0,B,0,0,B,0,0,B,0,0,0,0,0,0,0,0,
        0,0,0,B,W,W,B,B,0,0,0,0,0,0,0,0,0,B,0,0,B,0,0,B,0,0,0,0,0,0,0,0,
        0,0,B,B,W,B,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,B,B,B,B,0,0,0,0,0,0,0,0,0,0,0,B,0,0,B,0,0,B,0,0,0,0,0,0,0,0,
        0,B,B,B,B,0,0,0,0,0,B,B,0,0,0,0,0,B,0,0,B,0,0,B,0,0,0,0,0,0,0,0,
        0,B,B,0,0,0,0,0,B,B,0,0,B,B,0,0,0,B,0,0,B,0,0,B,0,0,0,0,0,0,0,0,
        B,0,0,B,B,0,0,B,0,0,0,0,0,0,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,B,B,0,0,0,0,0,0,0,0,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
      };
        *pc = SWS_MakeCursorFromData(p, 2, 14);
    }
    else if (id == IDC_ENV_PT_ADJ_VERT)
    {
      static unsigned char p[32*32] =
      {
        0,0,0,0,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,B,W,W,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,B,W,W,W,W,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        B,W,W,W,W,W,W,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        B,B,B,B,W,B,B,B,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,B,0,B,0,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,B,W,B,W,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,B,W,W,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,B,W,W,W,W,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,B,W,W,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,B,W,B,W,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,B,0,B,0,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        B,B,B,B,W,B,B,B,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        B,W,W,W,W,W,W,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,B,W,W,W,W,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,B,W,W,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
      };
        *pc = SWS_MakeCursorFromData(p, 4, 12);
    }
    else if (id == IDC_GRID_WARP)
    {
      static unsigned char p[32*32] =
      {
        0,0,0,0,0,0,0,0,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,B,B,0,0,0,B,0,0,0,B,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,B,W,B,0,0,0,B,0,0,0,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,B,W,W,B,B,B,B,B,B,B,B,B,W,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        B,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        B,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,B,W,W,B,B,B,B,B,B,B,B,B,W,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,B,W,B,0,0,0,B,0,0,0,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,B,B,0,0,0,B,0,0,0,B,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
      };
        *pc = SWS_MakeCursorFromData(p, 8, 10);
    }
    else if (id == IDC_MISC_SPEAKER)
    {
      static unsigned char p[32*32] =
      {
        0,0,0,0,0,0,0,B,B,B,0,0,0,0,0,0,0,0,W,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,B,B,W,B,0,0,0,0,0,0,0,W,B,W,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,B,B,W,W,B,0,0,0,0,0,W,0,0,W,B,W,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,B,B,W,W,W,B,0,0,0,0,W,B,W,0,0,W,B,W,0,0,0,0,0,0,0,0,0,0,
        B,B,B,B,B,W,W,W,W,B,0,0,W,0,0,W,B,W,0,W,B,W,0,0,0,0,0,0,0,0,0,0,
        B,B,W,B,W,W,W,W,W,B,0,W,B,W,0,W,B,W,0,0,W,B,W,0,0,0,0,0,0,0,0,0,
        B,B,W,B,W,W,W,W,W,B,0,0,W,B,W,0,W,B,W,0,W,B,W,0,0,0,0,0,0,0,0,0,
        B,B,W,B,W,W,W,W,W,B,0,0,W,B,W,0,W,B,W,0,W,B,W,0,0,0,0,0,0,0,0,0,
        B,B,W,B,W,W,W,W,W,B,0,0,W,B,W,0,W,B,W,0,W,B,W,0,0,0,0,0,0,0,0,0,
        B,B,W,B,W,W,W,W,W,B,0,W,B,W,0,W,B,W,0,0,W,B,W,0,0,0,0,0,0,0,0,0,
        B,B,B,B,B,W,W,W,W,B,0,0,W,0,0,W,B,W,0,W,B,W,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,B,B,W,W,W,B,0,0,0,0,W,B,W,0,0,W,B,W,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,B,B,W,W,B,0,0,0,0,0,W,0,0,W,B,W,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,B,B,W,B,0,0,0,0,0,0,0,W,B,W,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,B,B,B,0,0,0,0,0,0,0,0,W,0,0,0,0,0,0,0,0,0,0,0,0,0,
      };
        *pc = SWS_MakeCursorFromData(p, 10, 7);
    }
    else if (id == IDC_ZOOM_DRAG)
    {
      static unsigned char p[32*32] =
      {
        0,0,0,0,B,B,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,B,B,W,W,W,B,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,B,W,W,B,B,B,W,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,B,W,B,B,W,B,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        B,W,B,B,W,W,W,B,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        B,W,B,B,B,B,B,B,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        B,W,B,B,W,W,W,B,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,B,W,B,B,W,B,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,B,W,W,B,B,B,W,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,B,B,W,W,W,B,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,B,B,B,0,0,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
      };
      *pc = SWS_MakeCursorFromData(p, 5, 5);
    }
    else if (id == IDC_ZOOM_IN)
    {
      static unsigned char p[32*32] =
      {
        0,0,0,0,B,B,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,B,B,W,W,W,B,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,B,W,W,B,B,B,W,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,B,W,B,0,0,0,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        B,W,B,B,B,B,B,B,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        B,W,B,W,W,W,W,W,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        B,W,B,B,B,B,B,B,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,B,W,B,0,0,0,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,B,W,W,B,B,B,W,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,B,B,W,W,W,B,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,B,B,B,0,0,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
      };
      *pc = SWS_MakeCursorFromData(p, 5, 5);
    }
    else if (id == IDC_ZOOM_OUT)
    {
      static unsigned char p[32*32] =
      {
        0,0,0,0,B,B,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,B,B,W,W,W,B,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,B,W,W,B,B,B,W,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,B,W,B,W,W,W,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        B,W,B,B,B,B,B,B,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        B,W,B,W,W,W,W,W,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        B,W,B,B,B,B,B,B,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,B,W,B,W,W,W,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,B,W,W,B,B,B,W,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,B,B,W,W,W,B,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,B,B,B,0,0,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
      };
     *pc = SWS_MakeCursorFromData(p, 5, 5);
    }
    else if (id == IDC_ZOOM_UNDO)
    {
      static unsigned char p[32*32] =
      {
        0,0,0,0,B,B,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,B,B,W,W,W,B,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,B,W,W,B,B,B,W,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,B,W,B,0,0,0,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        B,W,B,W,B,0,B,W,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        B,W,B,W,B,0,B,W,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        B,W,B,W,B,B,B,W,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,B,W,B,W,W,W,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,B,W,W,B,B,B,W,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,B,B,W,W,W,B,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,B,B,B,0,0,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,B,W,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,B,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
      };
      *pc = SWS_MakeCursorFromData(p, 5, 5);
    }

  }
  return (HCURSOR)*pc;
}

void mouse_event(DWORD dwFlags, DWORD dx, DWORD dy, DWORD dwData, ULONG_PTR dwExtraInfo)
{
	CGEventRef e = NULL;
	int h = CGDisplayPixelsHigh(CGMainDisplayID());

	switch(dwFlags)
	{
		case MOUSEEVENTF_LEFTDOWN:  e = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseDown,  CGPointMake(dx, h-dy), kCGMouseButtonLeft);  break;
		case MOUSEEVENTF_LEFTUP:    e = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseUp,    CGPointMake(dx, h-dy), kCGMouseButtonLeft);  break;
		case MOUSEEVENTF_RIGHTDOWN: e = CGEventCreateMouseEvent(NULL, kCGEventRightMouseDown, CGPointMake(dx, h-dy), kCGMouseButtonRight); break;
		case MOUSEEVENTF_RIGHTUP:   e = CGEventCreateMouseEvent(NULL, kCGEventRightMouseUp,   CGPointMake(dx, h-dy), kCGMouseButtonRight); break;
	}

	if (e)
	{
		CGEventPost(kCGHIDEventTap, e);
		CFRelease(e);
	}
}


//JFB List view code here == modified from Cockos WDL!
// Overrides some wdl's list view funcs to avoid cast issues
// (useful with native list views which are SWELL_ListView but that were not instanciated by the extension..)

int ListView_GetSelectedCountCast(HWND h)
{
    if (!h) return 0;
//  if (![(id)h isKindOfClass:[SWELL_ListView class]]) return 0;

    SWELL_ListView *tv=(SWELL_ListView*)h;
    return [tv numberOfSelectedRows];
}

int ListView_GetItemCountCast(HWND h)
{
    if (!h) return 0;
//    if (![(id)h isKindOfClass:[SWELL_ListView class]]) return 0;

    SWELL_ListView *tv=(SWELL_ListView*)h;
    if (tv->m_lbMode || !(tv->style & LVS_OWNERDATA))
    {
        if (!tv->m_items) return 0;

        return tv->m_items->GetSize();
    }
    return tv->ownermode_cnt;
}

bool ListView_GetItemCast(HWND h, LVITEM *item)
{
    if (!item) return false;
    if ((item->mask&LVIF_TEXT)&&item->pszText && item->cchTextMax > 0) item->pszText[0]=0;
    item->state=0;
    if (!h) return false;
//    if (![(id)h isKindOfClass:[SWELL_ListView class]]) return false;


    SWELL_ListView *tv=(SWELL_ListView*)h;
    if (tv->m_lbMode || !(tv->style & LVS_OWNERDATA))
    {
        if (!tv->m_items) return false;

        SWELL_ListView_Row *row=tv->m_items->Get(item->iItem);
        if (!row) return false;

        if (item->mask & LVIF_PARAM) item->lParam=row->m_param;
        if (item->mask & LVIF_TEXT) if (item->pszText && item->cchTextMax>0)
        {
            char *p=row->m_vals.Get(item->iSubItem);
            lstrcpyn(item->pszText,p?p:"",item->cchTextMax);
        }
        if (item->mask & LVIF_STATE)
        {
            if (item->stateMask & (0xff<<16))
            {
                item->state|=row->m_imageidx<<16;
            }
        }
    }
    else
    {
        if (item->iItem <0 || item->iItem >= tv->ownermode_cnt) return false;
    }
    if (item->mask & LVIF_STATE)
    {
        if ((item->stateMask&LVIS_SELECTED) && [tv isRowSelected:item->iItem]) item->state|=LVIS_SELECTED;
        if ((item->stateMask&LVIS_FOCUSED) && [tv selectedRow] == item->iItem) item->state|=LVIS_FOCUSED;
    }

    return true;
}

void ListView_GetItemTextCast(HWND hwnd, int item, int subitem, char *text, int textmax)
{
    LVITEM it={LVIF_TEXT,item,subitem,0,0,text,textmax,};
    ListView_GetItemCast(hwnd,&it);
}

void ListView_RedrawItemsCast(HWND h, int startitem, int enditem)
{
//    if (!h || ![(id)h isKindOfClass:[SWELL_ListView class]]) return;
    if (!h) return;
    SWELL_ListView *tv=(SWELL_ListView*)h;
    if (!tv->m_items) return;
    [tv reloadData];
}

bool ListView_SetItemStateCast(HWND h, int ipos, int state, int statemask)
{
    int doref=0;
//    if (!h || ![(id)h isKindOfClass:[SWELL_ListView class]]) return false;
    if (!h) return false;
    SWELL_ListView *tv=(SWELL_ListView*)h;
    static int _is_doing_all;

    if (ipos == -1)
    {
        int x;
        int n=ListView_GetItemCountCast(h);
        _is_doing_all++;
        for (x = 0; x < n; x ++)
            ListView_SetItemStateCast(h,x,state,statemask);
        _is_doing_all--;
        ListView_RedrawItemsCast(h,0,n-1);
        return true;
    }

    if (tv->m_lbMode || !(tv->style & LVS_OWNERDATA))
    {
        if (!tv->m_items) return false;
        SWELL_ListView_Row *row=tv->m_items->Get(ipos);
        if (!row) return false;
        if (statemask & (0xff<<16))
        {
            if (row->m_imageidx!=((state>>16)&0xff))
            {
                row->m_imageidx=(state>>16)&0xff;
                doref=1;
            }
        }
    }
    else
    {
        if (ipos<0 || ipos >= tv->ownermode_cnt) return 0;
    }
    bool didsel=false;
    if (statemask & LVIS_SELECTED)
    {
        if (state & LVIS_SELECTED)
        {
            bool isSingle = tv->m_lbMode ? !(tv->style & LBS_EXTENDEDSEL) : !!(tv->style&LVS_SINGLESEL);
            if (![tv isRowSelected:ipos]) { didsel=true;  [tv selectRowIndexes:[NSIndexSet indexSetWithIndex:ipos] byExtendingSelection:isSingle?NO:YES]; }
        }
        else
        {
            if ([tv isRowSelected:ipos]) { didsel=true; [tv deselectRow:ipos];  }
        }
    }
    if (statemask & LVIS_FOCUSED)
    {
        if (state&LVIS_FOCUSED)
        {
        }
        else
        {

        }
    }

    if (!_is_doing_all)
    {
        if (didsel)
        {
            static int __rent;
            if (!__rent)
            {
                __rent=1;
                NMLISTVIEW nm={{(HWND)h,[tv tag],LVN_ITEMCHANGED},ipos,0,state,};
                SendMessage(GetParent(h),WM_NOTIFY,[tv tag],(LPARAM)&nm);
                __rent=0;
            }
        }
        if (doref)
            ListView_RedrawItemsCast(h,ipos,ipos);
    }
    return true;
}

BOOL IsWindowEnabled(HWND hwnd)
{
    if (!hwnd) return FALSE;
    SWELL_BEGIN_TRY
    id bla=(id)hwnd;

    if ([bla isKindOfClass:[NSWindow class]]) bla = [bla contentView];

    if (bla && [bla respondsToSelector:@selector(isEnabled:)])
    {
        return [bla isEnabled] == YES ? TRUE : FALSE;
    }
    SWELL_END_TRY(;)
    return FALSE;
}

void SWS_ShowTextScrollbar(HWND hwnd, bool show)
{
    if (!hwnd) return;

    DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);
    if (!show) dwStyle &= ~WS_HSCROLL;
    else dwStyle |= WS_HSCROLL;
    SetWindowLong(hwnd, GWL_STYLE, dwStyle);

    SWELL_TextView *tv=(SWELL_TextView*)hwnd;
    [tv setHorizontallyResizable:show?NO:YES];

    NSScrollView *sc = [tv enclosingScrollView];
    [sc setHasHorizontalScroller:show?YES:NO];
}


