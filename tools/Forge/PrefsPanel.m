/*
	PrefsPanel.m

	Preferences panel class

	Copyright (C) 2001 Dusk to Dawn Computing, Inc.

	Author: Jeff Teunissen <deek@d2dc.net>
	Date:	11 Nov 2001

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License as
	published by the Free Software Foundation; either version 2 of
	the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

	See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public
	License along with this program; if not, write to:

		Free Software Foundation, Inc.
		59 Temple Place - Suite 330
		Boston, MA  02111-1307, USA
*/

static const char rcsid[] = 
	"$Id$";

#ifdef HAVE_CONFIG_H
# include "Config.h"
#endif

#import <Foundation/NSGeometry.h>
#import <AppKit/NSButton.h>
#import <AppKit/NSButtonCell.h>
#import <AppKit/NSFont.h>
#import <AppKit/NSMatrix.h>
#import <AppKit/NSScrollView.h>

#import "BundleController.h"
#import "PrefsPanel.h"

@implementation PrefsPanel

- (void) initUI
{
	NSButton		*ok, *apply, *cancel, *revert;
	NSButtonCell	*prototype;
	NSScrollView	*iconScrollView;

	_topTag = 0;	// This is for the matrix

/*
	Window dimensions:

	8-pixel space on all sides
	Box is 242 pixels high, 500 wide
	Scroll area is 86 pixels tall, 500 wide
	content view size: (520, 414)
*/

	prefsViewBox = [[NSBox alloc] initWithFrame: NSMakeRect (8, 40, 500, 242)];
	[prefsViewBox setTitlePosition: NSNoTitle];
	[prefsViewBox setBorderType: NSGrooveBorder];
	NSDebugLog (@"prefsViewBox bounds: %@", NSStringFromRect ([[prefsViewBox contentView] bounds]));

	[[self contentView] addSubview: prefsViewBox];

	/* Prototype button for the matrix */
	prototype = [[[NSButtonCell alloc] init] autorelease];
	[prototype setButtonType: NSPushOnPushOffButton];
	[prototype setImagePosition: NSImageOverlaps];

	/* The matrix itself -- horizontal */
	prefsViewList = [[NSMatrix alloc] initWithFrame: NSMakeRect (8, 290, 500, 86)];
	[prefsViewList setCellSize: NSMakeSize (64, 64)];
	[prefsViewList setMode: NSRadioModeMatrix];
	[prefsViewList setPrototype: prototype];
	[prefsViewList setTarget: [self windowController]];
	[prefsViewList setAction: @selector(cellWasClicked:)];

	iconScrollView = [[NSScrollView alloc] initWithFrame: NSMakeRect (8, 290, 500, 86)];
	[iconScrollView autorelease];
	[iconScrollView setHasHorizontalScroller: YES];
	[iconScrollView setHasVerticalScroller: NO];
	[iconScrollView setDocumentView: prefsViewList];
	[[self contentView] addSubview: iconScrollView];

	/* Create the buttons */
	// OK
	ok = [[NSButton alloc] initWithFrame: NSMakeRect (244, 8, 60, 24)];
	[ok autorelease];

	[ok setTitle: _(@"OK")];
	[ok setTarget: [self windowController]];
	[ok setAction: @selector(savePreferencesAndCloseWindow:)];
	[[self contentView] addSubview: ok];

	// Cancel
	cancel = [[NSButton alloc] initWithFrame: NSMakeRect (312, 8, 60, 24)];
	[cancel autorelease];

	[cancel setTitle: _(@"Cancel")];
	[cancel setTarget: self];
	[cancel setAction: @selector(close)];
	[[self contentView] addSubview: cancel];

	// Apply
	apply = [[NSButton alloc] initWithFrame: NSMakeRect (380, 8, 60, 24)];
	[apply autorelease];

	[apply setTitle: _(@"Apply")];
	[apply setTarget: [self windowController]];
	[apply setAction: @selector(savePreferences:)];
	[[self contentView] addSubview: apply];

	// Defaults
	revert = [[NSButton alloc] initWithFrame: NSMakeRect (448, 8, 60, 24)];
	[revert autorelease];

	[revert setTitle: _(@"Default")];
	[revert setTarget: [self windowController]];
	[revert setAction: @selector(resetToDefaults:)];
	[[self contentView] addSubview: revert];
}

- (void) dealloc
{
	NSDebugLog (@"PrefsPanel -dealloc");
	[prefsViewBox release];

	[super dealloc];
}

- (void) addPrefsViewButton: (id <PrefsViewController>) aController
{
	NSButtonCell	*button = [[NSButtonCell alloc] init];

	[button setTag: _topTag++];
	[button setTitle: [aController buttonCaption]];
	[button setFont: [NSFont systemFontOfSize: 9]];
	[button setImage: [aController buttonImage]];
	[button setImagePosition: NSImageAbove];
	[button setTarget: aController];
	[button setAction: [aController buttonSelector]];
	[prefsViewList addColumnWithCells: [NSArray arrayWithObject: button]];
	[prefsViewList sizeToCells];
	[prefsViewList setNeedsDisplay: YES];
}

- (NSBox *) prefsViewBox
{
	return prefsViewBox;
}

- (NSMatrix *) prefsViewList
{
	return prefsViewList;
}

@end
