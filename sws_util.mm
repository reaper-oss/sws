/******************************************************************************
/ sws_util.mm
/
/ Copyright (c) 2010 ans later Tim Payne (SWS), Jeffos, Dominik Martin Drzic

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


void SWS_GetDateString(int time, char* buf, int bufsize)
{
	NSDate* macTime = [NSDate dateWithTimeIntervalSince1970:time];
	NSDateFormatter *macTimeFmt = [[[NSDateFormatter alloc] init] autorelease];
	[macTimeFmt setFormatterBehavior:NSDateFormatterBehavior10_4];
	[macTimeFmt setDateStyle:NSDateFormatterShortStyle];
	NSString* s = [macTimeFmt stringFromDate:macTime];
	[s getCString:buf maxLength:bufsize encoding:NSUTF8StringEncoding];
}

void SWS_GetTimeString(int time, char* buf, int bufsize)
{
	NSDate* macTime = [NSDate dateWithTimeIntervalSince1970:time];
	NSDateFormatter *macTimeFmt = [[[NSDateFormatter alloc] init] autorelease];
	[macTimeFmt setFormatterBehavior:NSDateFormatterBehavior10_4];
	[macTimeFmt setTimeStyle:NSDateFormatterShortStyle];
	NSString* s = [macTimeFmt stringFromDate:macTime];
	[s getCString:buf maxLength:bufsize encoding:NSUTF8StringEncoding];
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
	id cswatch = [cp valueForKey:@"_colorSwatch"];
	NSArray *colors = NULL;
	int adv,n=0;

	if ([cswatch respondsToSelector:@selector(readColors)])
	{
		[cswatch performSelector:@selector(readColors)];

		colors = [cswatch valueForKey:@"colors"];
		adv = 10;
		n = 16;
	}
  	else if ([cswatch respondsToSelector:@selector(savedColors)])
	{
		// high sierra
		colors = [cswatch performSelector:@selector(savedColors)];
		adv=1;
		n = [colors count];
		if (n > 16) n = 16;
	}
	if (colors) for (int i = 0; i < n; i++)
	{
		NSColor* col = [colors objectAtIndex:i*adv];
		col = [col colorUsingColorSpaceName:@"NSCalibratedRGBColorSpace"];

		custColors[i] = RGBfromNSColor(col);
	}

	return 0;
}

void SetCustomColors(COLORREF custColors[])
{
	NSColorPanel* cp = [NSColorPanel sharedColorPanel];
	id cswatch = [cp valueForKey:@"_colorSwatch"];

	if ([cswatch respondsToSelector:@selector(readColors)])
	{
		[cswatch performSelector:@selector(readColors)];

		// Get the NSColorSwatch's internal mutable array of NSColors
		NSMutableArray *colors = [cswatch valueForKey:@"colors"];
		if (colors) for (int i = 0; i < 16; i++)
		{
			NSColor* col = [NSColor colorWithCalibratedRed:(custColors[i]>>16)/255.0 green:((custColors[i]&0xFF00)>>8)/255.0 blue:(custColors[i]&0xFF)/255.0 alpha:1.0];
			[colors replaceObjectAtIndex:i*10 withObject:col];
		}
		[cswatch performSelector:@selector(writeColors)];
	}
  	else if ([cswatch respondsToSelector:@selector(savedColors)])
	{
		// high sierra -- this part doesn't seem to work yet, sadly
		NSArray *colors = [cswatch performSelector:@selector(savedColors)];
		NSMutableArray *newColors = colors ? [NSMutableArray arrayWithArray:colors] : [NSMutableArray arrayWithCapacity:16];
		for (int i = 0; i < 16; i++)
		{
			NSColor* col = [NSColor colorWithCalibratedRed:(custColors[i]>>16)/255.0 green:((custColors[i]&0xFF00)>>8)/255.0 blue:(custColors[i]&0xFF)/255.0 alpha:1.0];
			if (i < [newColors count])
				[newColors replaceObjectAtIndex:i withObject:col];
			else
				[newColors addObject:col];
		}
		[cswatch performSelector:@selector(setSavedColors:) withObject:newColors];
	}
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

void SWS_ShowTextScrollbar(HWND hwnd, bool show)
{
    if (!hwnd) return;

    DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);
    if (!show) dwStyle &= ~WS_HSCROLL;
    else dwStyle |= WS_HSCROLL;
    SetWindowLong(hwnd, GWL_STYLE, dwStyle);

    NSTextView *tv=(NSTextView*)hwnd;
    [tv setHorizontallyResizable:show?NO:YES];

    NSScrollView *sc = [tv enclosingScrollView];
    [sc setHasHorizontalScroller:show?YES:NO];
}

void SetMenuItemSwatch(HMENU hMenu, UINT pos, int iSize, COLORREF color)
{
  if (!hMenu) return;
  NSMenu *m=(NSMenu *)hMenu;
  NSMenuItem *item = [m itemAtIndex:pos];
  NSSize size = NSMakeSize(iSize, iSize);
  
  if (!item)
    return;
  if (!item.image)
    item.image = [[NSImage alloc] initWithSize:(NSSize)size];

  NSColor *nscolor = [NSColor colorWithCalibratedRed:GetRValue(color)/255.0f green:GetGValue(color)/255.0f blue:GetBValue(color)/255.0f alpha:1.0f];
  [item.image lockFocus];
  [nscolor set];
  NSRectFill(NSMakeRect(0, 0, size.width, size.height));
  [item.image unlockFocus];
}
