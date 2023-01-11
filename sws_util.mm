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
	id cswatch = [cp valueForKey:@"_colorSwatch"]; // NSColorPanelFavoritesList

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
	else if (id NSFavoriteColorsStore = NSClassFromString(@"NSFavoriteColorsStore"))
	{
		// High Sierra to at least Big Sur (undocumented API)
		id cstore = [NSFavoriteColorsStore performSelector:@selector(defaultListCompatibleStore)];

		for (int i = 0; i < 16; i++)
		{
			NSColor* col = [NSColor colorWithCalibratedRed:(custColors[i]>>16)/255.0 green:((custColors[i]&0xFF00)>>8)/255.0 blue:(custColors[i]&0xFF)/255.0 alpha:1.0];
			id index = [cswatch performSelector:@selector(storeIndexForColorIndex:) withObject:reinterpret_cast<id>(i)];
			[cstore performSelector:@selector(replaceColorAtIndex:withColor:) withObject:index withObject:col];
		}
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
HCURSOR SWS_Cursor::makeFromData()
{
  if (inst)
    return inst;

  NSCursor *c = nullptr;
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
      for (size_t i = 0; i < 32*32; ++i)
      {
        p[2*i] = (data[i]&0xF0) | (data[i]>>4);
        p[2*i+1] = (data[i]<<4) | (data[i]&0xF);
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

  return inst = reinterpret_cast<HCURSOR>(c);
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

static bool g_miscTimerRunning;
void TestMiscTimerRunning()
{
  g_miscTimerRunning = true;
  plugin_register("-timer", reinterpret_cast<void *>(&TestMiscTimerRunning));
}

void WaitUntil(bool(*predicate)(void *), void *data)
{
  g_miscTimerRunning = false;
  plugin_register("timer", reinterpret_cast<void *>(&TestMiscTimerRunning));

  bool firstRun = true;
  while (!predicate(data)) {
    // Waiting 30 ms instead of 1 ms (like on Windows) to match the normal
    // frequency of REAPER's misc timer
    constexpr int DEFER_TIMER_SPEED = 30,
                  MISC_TIMER        = 0x29A;

    [[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:DEFER_TIMER_SPEED / 1000.f]];

    // REAPER's misc timer (based on NSTimer), responsible for many many things
    // from UI updates to updating GetPlayPosition(), won't be re-entered in the
    // event loop iterations above while if we're blocking it's handler.
    //
    // Wait until we're 100% sure the misc timer is suspended
    // (by waiting twice its normal fire rate) before manually triggering it.
    if (firstRun)
      firstRun = false;
    else if (!g_miscTimerRunning) {
      plugin_register("-timer", reinterpret_cast<void *>(&TestMiscTimerRunning));
      PostMessage(GetMainHwnd(), WM_TIMER, MISC_TIMER, 0);
    }
  }
}

void SWS_Mac_MakeDefaultWindowMenu(HWND hwnd)
{
  // Replace the menubar with an empty one with just an "Edit" menu
  // when the given window has focus.
  if (HMENU menu = SWELL_GetDefaultModalWindowMenu()) {
    if ((menu = SWELL_DuplicateMenu(menu))) {
      SetMenu(hwnd, menu);
      menu = GetSubMenu(menu, 0); // "REAPER" menu
      SWELL_SetMenuDestination(menu, GetMainHwnd());
    }
  }
}

void Mac_TextViewSetAllowsUndo(HWND hwnd, const bool enable)
{
  if(hwnd && [(id)hwnd isKindOfClass:[NSTextView class]])
    [(NSTextView *)hwnd setAllowsUndo:enable];
}
