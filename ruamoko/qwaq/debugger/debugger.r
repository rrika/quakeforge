#include <QF/keys.h>
#include <Array.h>
#include <string.h>
#include <types.h>

#include "ui/curses.h"
#include "ui/listener.h"
#include "ui/proxyview.h"
#include "ui/scrollbar.h"
#include "ui/tableview.h"
#include "ui/window.h"
#include "debugger/debugger.h"
#include "debugger/typeencodings.h"
#include "editor/editor.h"
#include "qwaq-app.h"

@implementation Debugger
+(Debugger *)withTarget:(qdb_target_t)target
{
	return [[[self alloc] initWithTarget:target] autorelease];
}

-(qdb_target_t)target
{
	return target;
}

-initWithTarget:(qdb_target_t) target
{
	if (!(self = [super init])) {
		return nil;
	}
	self.target = target;

	Extent s = [application size];
	files = [[Array array] retain];
	source_window = [Window withRect: {nil, s}];
	[application addView:source_window];

	source_scrollbar = [ScrollBar vertical:s.height - 2 at:{s.width - 1, 1}];
	[source_window insert:source_scrollbar];

	return self;
}

-(void)dealloc
{
	[files release];
	[locals_data release];
	[super dealloc];
}

-(Editor *) find_file:(string) filename
{
	Editor     *file;
	for (int i = [files count]; i-- > 0; ) {
		file = [files objectAtIndex: i];
		if ([file filename] == filename) {
			return file;
		}
	}
	Rect rect = {{1, 1}, [source_window size]};
	rect.extent.width -= 2;
	rect.extent.height -= 2;
	file = [Editor withRect:rect file:filename];
	[files addObject: file];
	return file;
}

-(void) setup
{
	qdb_state_t state = qdb_get_state (target);

	current_file = [self find_file: state.file];
	file_proxy = [ProxyView withView: current_file];
	[[current_file gotoLine:state.line - 1] highlightLine];
	[[current_file onEvent] addListener: self :@selector(proxy_event::)];
	[current_file setVerticalScrollBar:source_scrollbar];
	//FIXME id<View>?
	[source_window insertSelected: (View *) file_proxy];
	[source_window setTitle: [current_file filename]];
	[source_window redraw];

	locals_window = [Window withRect:{{0, 0}, {40, 10}}];
	[locals_window setBackground: color_palette[064]];
	[locals_window setTitle: "Locals"];
	locals_data = [[LocalsData withTarget:target] retain];
	locals_view = [TableView withRect:{{1, 1}, {38, 8}}];
	[locals_view addColumn:[TableViewColumn named:"name" width:12]];
	[locals_view addColumn:[[TableViewColumn named:"value" width:26]
							setGrowMode:gfGrowHiX]];
	ScrollBar *sb = [ScrollBar vertical:8 at:{39, 1}];
	[locals_view setVerticalScrollBar:sb];
	[locals_view setDataSource:locals_data];
	[locals_window insertSelected: locals_view];
	[locals_window insert: sb];
	[application addView: locals_window];

	[[locals_view onEvent] addListener:self :@selector(proxy_event::)];
}

-(void) show_line
{
	qdb_state_t state = qdb_get_state (target);
	Editor     *file = [self find_file: state.file];

	if (current_file != file) {
		[current_file setVerticalScrollBar:nil];
		[[current_file onEvent] removeListener:self :@selector(proxy_event::)];
		[file_proxy setView:file];
		[[file onEvent] addListener:self :@selector(proxy_event::)];
		[file setVerticalScrollBar:source_scrollbar];
		[source_window setTitle: [file filename]];
		current_file = file;
	}
	[[current_file gotoLine:state.line - 1] highlightLine];
	[source_window redraw];
}

-(void)update_watchvars
{
	qdb_state_t state = qdb_get_state (target);
	[locals_data setFunction:state.func];
	[locals_data fetchData];
	[locals_view redraw];
}

static int
proxy_event (Debugger *self, id proxy, qwaq_event_t *event)
{
	if (event.what == qe_mouseclick && !(event.mouse.buttons & 0x78)) {
		if (proxy == self.current_file) {
			printf ("%s\n", [proxy getWordAt: {event.mouse.x, event.mouse.y}]);
			[self.source_window redraw];
			return 1;
		}
	} else if (event.what == qe_keydown) {
		switch (event.key.code) {
			case QFK_F7:
				qdb_set_trace (self.target, 1);
				self.last_state = qdb_get_state (self.target);
				qdb_continue (self.target);
				return 1;
		}
	}
	return 0;
}

-(void)proxy_event:(id)proxy :(qwaq_event_t *)event
{
	if (proxy_event (self, proxy, event)) {
		event.what = qe_none;
	}
}

-stop:(prdebug_t)reason
{
	if (!file_proxy) {
		[self setup];
	}
	[self show_line];
	[self update_watchvars];
	return self;
}

-trace
{
	qdb_state_t state = qdb_get_state (target);

	// stop only if the progs have not advanced (may be a broken jump)
	// or the progs have advanced to a different source line
	if (last_state.staddr != state.staddr
		&& last_state.func == state.func
		&& last_state.file == state.file
		&& last_state.line == state.line) {
		last_state = state;
		qdb_continue (self.target);
		return self;
	}
	[self stop:prd_trace];
	return self;
}

-handleDebugEvent
{
	if (qdb_get_event (target, &event)) {
		switch (event.what) {
			case prd_none:
				break;	// shouldn't happen
			case prd_trace:
				[self trace];
				break;
			case prd_breakpoint:
			case prd_watchpoint:
				[self stop:event.what];
				break;
			case prd_subenter:
				[self stop:event.what];
				break;
			case prd_subexit:
				[self stop:event.what];
				break;
			case prd_terminate:
				wprintf(stdscr, "Program ended: %d\n", event.exit_code);
				[self stop:event.what];
				break;
			case prd_runerror:
			case prd_error:
				wprintf(stdscr, "%s\n", event.message);
				[self stop:event.what];
				break;
		}
	}
	return self;
}

@end
