/*
   Copyright (C) 2002-2010 Karl J. Runge <runge@karlrunge.com> 
   All rights reserved.

This file is part of x11vnc.

x11vnc is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

x11vnc is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with x11vnc; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA
or see <http://www.gnu.org/licenses/>.

In addition, as a special exception, Karl J. Runge
gives permission to link the code of its release of x11vnc with the
OpenSSL project's "OpenSSL" library (or with modified versions of it
that use the same license as the "OpenSSL" library), and distribute
the linked executables.  You must obey the GNU General Public License
in all respects for all of the code used other than "OpenSSL".  If you
modify this file, you may extend this exception to your version of the
file, but you are not obligated to do so.  If you do not wish to do
so, delete this exception statement from your version.
*/

/* -- macosx_opengl.c -- */

#if (defined(__MACH__) && defined(__APPLE__))
#include <CoreFoundation/CoreFoundation.h>
#include <ApplicationServices/ApplicationServices.h>
#include <Cocoa/Cocoa.h>
#include <Carbon/Carbon.h>

#include "config.h"
#include "macosx_opengl.h"

#include <rfb/rfb.h>
#if HAVE_MACOSX_OPENGL_H
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#endif

extern int macosx_no_opengl, macosx_read_opengl;
extern CGDirectDisplayID displayID;

static CGLContextObj glContextObj;

int macosx_opengl_width = 0;
int macosx_opengl_height = 0;
int macosx_opengl_bpp = 0;

static NSMutableData *frameBufferData = nil;
static size_t frameBufferBytesPerRow;
static size_t frameBufferBitsPerPixel;

int macosx_opengl_get_width(void)
{
	return CGDisplayPixelsWide(displayID);
}

int macosx_opengl_get_height(void)
{
	return CGDisplayPixelsHigh(displayID);
}

int macosx_opengl_get_bpp(void)
{
	return 32;
}

int macosx_opengl_get_bps(void)
{
	return 8;
}

int macosx_opengl_get_spp(void)
{
	return 3;
}

void macosx_opengl_init(void)
{
	macosx_opengl_width = macosx_opengl_get_width();
	macosx_opengl_height = macosx_opengl_get_height();
	macosx_opengl_bpp = macosx_opengl_get_bpp();

	if (floor(NSAppKitVersionNumber) > floor(NSAppKitVersionNumber10_6))
	{
		if (!frameBufferData)
		{
			CGImageRef imageRef;
			if (2 > 1.0)
			{
				// Retina display.
				size_t width = macosx_opengl_get_width();
				size_t height = macosx_opengl_get_height();
				CGImageRef image = CGDisplayCreateImage(displayID);
				CGColorSpaceRef colorspace = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
				CGContextRef context = CGBitmapContextCreate(NULL, width, height,
															 CGImageGetBitsPerComponent(image),
															 CGImageGetBytesPerRow(image),
															 colorspace,
															 kCGImageAlphaNoneSkipLast);

				CGColorSpaceRelease(colorspace);
				if (context == NULL)
				{
					rfbLog("There was an error getting screen shot");
					return;
				}
				CGContextDrawImage(context, CGRectMake(0, 0, width, height), image);
				imageRef = CGBitmapContextCreateImage(context);
				CGContextRelease(context);
				CGImageRelease(image);
			}
			else
			{
				imageRef = CGDisplayCreateImage(displayID);
			}
			CGDataProviderRef dataProvider = CGImageGetDataProvider(imageRef);
			CFDataRef dataRef = CGDataProviderCopyData(dataProvider);
			frameBufferBytesPerRow = CGImageGetBytesPerRow(imageRef);
			frameBufferBitsPerPixel = CGImageGetBitsPerPixel(imageRef);
			frameBufferData = [(NSData *)dataRef mutableCopy];
			CFRelease(dataRef);

			if (imageRef != NULL)
				CGImageRelease(imageRef);
		}
	}

	macosx_read_opengl = 1;
}

void macosx_opengl_fini(void)
{
	
}

void macosx_copy_opengl(char *dest, int x, int y, unsigned int w, unsigned int h)
{
	if (frameBufferData)
	{
		CGRect rect = CGRectMake(x, y, w, h);
		CGImageRef imageRef;
		if (2 > 1.0)
		{
			// Retina display.
			CGImageRef image = CGDisplayCreateImageForRect(displayID, rect);
			CGColorSpaceRef colorspace = CGColorSpaceCreateDeviceRGB();
			CGBitmapInfo bitmapInfo = kCGBitmapByteOrder32Little | kCGImageAlphaNoneSkipFirst;
			CGContextRef context = CGBitmapContextCreate(NULL, w, h, 8, w * 4, colorspace, bitmapInfo);
			CGColorSpaceRelease(colorspace);
			if (context == NULL)
			{
				rfbLog("There was an error getting scaled images");
			}
			CGContextDrawImage(context, CGRectMake(0, 0, w, h), image);
			CGImageRelease(image);
			imageRef = CGBitmapContextCreateImage(context);
			CGContextRelease(context);
		}
		else
		{
			imageRef = CGDisplayCreateImageForRect(displayID, rect);
		}

		CGDataProviderRef dataProvider = CGImageGetDataProvider(imageRef);
		CFDataRef dataRef = CGDataProviderCopyData(dataProvider);
		size_t imgBytesPerRow = CGImageGetBytesPerRow(imageRef);
		size_t imgBitsPerPixel = CGImageGetBitsPerPixel(imageRef);
		if (imgBitsPerPixel != frameBufferBitsPerPixel)
		{
			NSLog(@"BitsPerPixel MISMATCH: frameBuffer %zu, rect image %zu", frameBufferBitsPerPixel, imgBitsPerPixel);
		}
		const char *source = ((NSData *)dataRef).bytes;

		if (h > 0) {
			//int data_size = w * macosx_opengl_bpp / 8 * h;
			int data_size = w * frameBufferBitsPerPixel / 8 * h;
			memcpy(dest, source, data_size);
		}


		if (imageRef != NULL)
			CGImageRelease(imageRef);
		[(id)dataRef release];
	}
}

#else

#endif /* __APPLE__ */
