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

// This function MakeCursorFromData is (C) Cockos and is copied from WDL:swell-kb.mm
static NSCursor* MakeCursorFromData(unsigned char* data, int hotspot_x, int hotspot_y)
{
  NSCursor *c=NULL;
  NSBitmapImageRep* bmp = [[NSBitmapImageRep alloc] 
    initWithBitmapDataPlanes:0
    pixelsWide:16
    pixelsHigh:16
    bitsPerSample:8
    samplesPerPixel:2  
    hasAlpha:YES
    isPlanar:NO 
    colorSpaceName:NSCalibratedWhiteColorSpace
    bytesPerRow:(16*2)
    bitsPerPixel:16]; 
  
  if (bmp)
  {
    unsigned char* p = [bmp bitmapData];
    if (p)
    {  
      int i;
      for (i = 0; i < 16*16; ++i)
      {
        // tried 4 bits per sample and memcpy, didn't work
        p[2*i] = (data[i]&0xF0) | data[i]>>4;
        p[2*i+1] = (data[i]<<4) | (data[i]&0xf);
      }
  
      NSImage *img = [[NSImage alloc] init];
      if (img)
      {
        [img addRepresentation:bmp];  
        NSPoint hs = { hotspot_x, hotspot_y };
        c = [[NSCursor alloc] initWithImage:img hotSpot:hs];
        [img release];
      }   
    }
    [bmp release];
  }
  return c;
}

// Code here modified from Cockos WDL!
// Supports all the zoom cursors:
// IDC_ZOOMIN, IDC_ZOOMOUT, IDC_ZOOMUNDO, IDC_ZOOMDRAG
HCURSOR SWS_LoadCursor(int id)
{
  // bytemaps are (white<<4)|(alpha)
  const unsigned char B = 0xF;
  const unsigned char W = 0xFF;
  //const unsigned char G = 0xF8;
  
  static NSCursor* carr[4] = { 0, 0, 0, 0 };
  
  NSCursor** pc=0;
  if (id == IDC_ZOOMIN) pc = &carr[0];
  else if (id == IDC_ZOOMOUT) pc = &carr[1];
  else if (id == IDC_ZOOMUNDO) pc = &carr[2];
  else if (id == IDC_ZOOMDRAG) pc = &carr[3];
  else return 0;
  
  if (!(*pc))
  {
    if (id == IDC_ZOOMIN)
    {
      static unsigned char p[16*16] = 
      {
        0, 0, 0, 0, B, B, B, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, B, B, W, W, W, B, B, 0, 0, 0, 0, 0, 0, 0,
        0, B, W, W, B, B, B, W, W, B, 0, 0, 0, 0, 0, 0,
        0, B, W, B, B, W, B, B, W, B, 0, 0, 0, 0, 0, 0,
        B, W, B, B, B, W, B, B, B, W, B, 0, 0, 0, 0, 0,
        B, W, B, W, W, W, W, W, B, W, B, 0, 0, 0, 0, 0,
        B, W, B, B, B, W, B, B, B, W, B, 0, 0, 0, 0, 0,
        0, B, W, B, B, W, B, B, W, B, 0, 0, 0, 0, 0, 0,
        0, B, W, W, B, B, B, W, W, B, 0, 0, 0, 0, 0, 0,
        0, 0, B, B, W, W, W, B, B, W, B, 0, 0, 0, 0, 0,
        0, 0, 0, 0, B, B, B, 0, 0, B, W, B, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, B, W, B, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, B, W, B, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, B, 0,
      };
      *pc = MakeCursorFromData(p, 5, 5);
    }
    else if (id == IDC_ZOOMOUT)
    {
      static unsigned char p[16*16] = 
      {
        0, 0, 0, 0, B, B, B, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, B, B, W, W, W, B, B, 0, 0, 0, 0, 0, 0, 0,
        0, B, W, W, B, B, B, W, W, B, 0, 0, 0, 0, 0, 0,
        0, B, W, B, B, B, B, B, W, B, 0, 0, 0, 0, 0, 0,
        B, W, B, B, B, B, B, B, B, W, B, 0, 0, 0, 0, 0,
        B, W, B, W, W, W, W, W, B, W, B, 0, 0, 0, 0, 0,
        B, W, B, B, B, B, B, B, B, W, B, 0, 0, 0, 0, 0,
        0, B, W, B, B, B, B, B, W, B, 0, 0, 0, 0, 0, 0,
        0, B, W, W, B, B, B, W, W, B, 0, 0, 0, 0, 0, 0,
        0, 0, B, B, W, W, W, B, B, W, B, 0, 0, 0, 0, 0,
        0, 0, 0, 0, B, B, B, 0, 0, B, W, B, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, B, W, B, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, B, W, B, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, B, 0,
      };
     *pc = MakeCursorFromData(p, 5, 5);   
    }
    else if (id == IDC_ZOOMUNDO)
    {
      static unsigned char p[16*16] = 
      {
        0, 0, 0, 0, B, B, B, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, B, B, W, W, W, B, B, 0, 0, 0, 0, 0, 0, 0,
        0, B, W, W, B, B, B, W, W, B, 0, 0, 0, 0, 0, 0,
        0, B, W, B, 0, 0, 0, B, W, B, 0, 0, 0, 0, 0, 0,
        B, W, B, W, B, 0, B, W, B, W, B, 0, 0, 0, 0, 0,
        B, W, B, W, B, 0, B, W, B, W, B, 0, 0, 0, 0, 0,
        B, W, B, W, B, B, B, W, B, W, B, 0, 0, 0, 0, 0,
        0, B, W, B, W, W, W, B, W, B, 0, 0, 0, 0, 0, 0,
        0, B, W, W, B, B, B, W, W, B, 0, 0, 0, 0, 0, 0,
        0, 0, B, B, W, W, W, B, B, W, B, 0, 0, 0, 0, 0,
        0, 0, 0, 0, B, B, B, 0, 0, B, W, B, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, B, W, B, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, B, W, B, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, B, 0,
      };
      *pc = MakeCursorFromData(p, 5, 5);        
    }
    else if (id == IDC_ZOOMDRAG)
    {
      static unsigned char p[16*16] = 
      {
        0, 0, 0, 0, B, B, B, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, B, B, W, W, W, B, B, 0, 0, 0, 0, 0, 0, 0,
        0, B, W, W, B, B, B, W, W, B, 0, 0, 0, 0, 0, 0,
        0, B, W, B, B, W, B, B, W, B, 0, 0, 0, 0, 0, 0,
        B, W, B, B, W, W, W, B, B, W, B, 0, 0, 0, 0, 0,
        B, W, B, B, B, B, B, B, B, W, B, 0, 0, 0, 0, 0,
        B, W, B, B, W, W, W, B, B, W, B, 0, 0, 0, 0, 0,
        0, B, W, B, B, W, B, B, W, B, 0, 0, 0, 0, 0, 0,
        0, B, W, W, B, B, B, W, W, B, 0, 0, 0, 0, 0, 0,
        0, 0, B, B, W, W, W, B, B, W, B, 0, 0, 0, 0, 0,
        0, 0, 0, 0, B, B, B, 0, 0, B, W, B, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, B, W, B, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, B, W, B, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, B, 0,
      };
      *pc = MakeCursorFromData(p, 5, 5);        
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

