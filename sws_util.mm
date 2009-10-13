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

int GetCustomColors()
{
		id colorSwatch = nil;
		NSArray *colors = nil;
		
		// Try to get the colour swatch, load its colours and get a reference to them.
		// We're trying to access private NSColorPanel variables and methods here, so
		// we look out for an exception being thrown (which would be NSUndefinedKeyException)
		// in case they are changed. This way we can exit safely by returning nil, so that
		// the user isn't affected.
		@try
		{
			// Get the colour swatch.
			colorSwatch = [self valueForKey:@"_colorSwatch"];
			
			// Load the user-defined colours into the swatch.
			if ([colorSwatch respondsToSelector:@selector(readColors)])
				[colorSwatch performSelector:@selector(readColors)];
			
			// Get the colours.
			colors = [colorSwatch valueForKey:@"colors"];
		}
		
		@catch (NSException *e)
		{
			// NSColorPanel has been changed and no longer users all of these
			// private methods, so abandon ship.
			return nil;
		}
		
		// Find all swatch colours which aren't the default white.
		NSMutableArray *userSwatchColors = [NSMutableArray array];
		NSEnumerator *e = [colors objectEnumerator];
		NSColor *color;
		while(color = [e nextObject])
		{
			if ([color colorSpace] == [NSColorSpace genericGrayColorSpace] && [color whiteComponent] == 1.0)
				continue;
			[userSwatchColors addObject:color];
		}
		
		return (NSArray *)userSwatchColors;
	}}