#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <stdio.h>

#include "QF/ui/view.h"
#if 0
typedef struct {
	struct {
		int         xlen, ylen;
	};
	int         bol_suppress;
	struct {
		struct {
			int         xpos, ypos;
		};
		struct {
			int         xrel, yrel;
		};
		struct {
			int         xabs, yabs;
		};
	} expect;
} testdata_t;

#define array_size(array) (sizeof (array) / sizeof(array[0]))

static testdata_t right_down_views[] = {
	{{ 48, 8}, 0, { {  0, 0}, {  0,  0}, {  8, 16} }},	// 0
	{{128, 8}, 0, { { 48, 0}, { 48,  0}, { 56, 16} }},
	{{ 64, 8}, 0, { {176, 0}, {176,  0}, {184, 16} }},

	{{ 32, 8}, 0, { {  0, 0}, {  0,  8}, {  8, 24} }},	// 3
	{{224, 8}, 0, { { 32, 0}, { 32,  8}, { 40, 24} }},

	{{ 64, 8}, 0, { {  0, 0}, {  0, 16}, {  8, 32} }},	// 5
	{{ 64, 8}, 0, { { 64, 0}, { 64, 16}, { 72, 32} }},
	{{ 64, 8}, 0, { {128, 0}, {128, 16}, {136, 32} }},
	{{ 32, 8}, 0, { {192, 0}, {192, 16}, {200, 32} }},

	{{288, 8}, 0, { {  0, 0}, {  0, 24}, {  8, 40} }},	// 9

	{{ 48, 8}, 0, { {  0, 0}, {  0, 32}, {  8, 48} }},	// 10
	{{128, 8}, 0, { { 48, 0}, { 48, 32}, { 56, 48} }},
	{{ 64, 8}, 0, { {176, 0}, {176, 32}, {184, 48} }},
};
#define right_down_count array_size(right_down_views)

static testdata_t right_up_views[] = {
	{{ 48, 8}, 0, { {  0, 0}, {  0, 32}, {  8, 48} }},	// 0
	{{128, 8}, 0, { { 48, 0}, { 48, 32}, { 56, 48} }},
	{{ 64, 8}, 0, { {176, 0}, {176, 32}, {184, 48} }},

	{{ 32, 8}, 0, { {  0, 0}, {  0, 24}, {  8, 40} }},	// 3
	{{224, 8}, 0, { { 32, 0}, { 32, 24}, { 40, 40} }},

	{{ 64, 8}, 0, { {  0, 0}, {  0, 16}, {  8, 32} }},	// 5
	{{ 64, 8}, 0, { { 64, 0}, { 64, 16}, { 72, 32} }},
	{{ 64, 8}, 0, { {128, 0}, {128, 16}, {136, 32} }},
	{{ 32, 8}, 0, { {192, 0}, {192, 16}, {200, 32} }},

	{{288, 8}, 0, { {  0, 0}, {  0,  8}, {  8, 24} }},	// 9

	{{ 48, 8}, 0, { {  0, 0}, {  0,  0}, {  8, 16} }},	// 10
	{{128, 8}, 0, { { 48, 0}, { 48,  0}, { 56, 16} }},
	{{ 64, 8}, 0, { {176, 0}, {176,  0}, {184, 16} }},
};
#define right_up_count array_size(right_up_views)

static testdata_t left_down_views[] = {
	{{ 48, 8}, 0, { {208, 0}, {208,  0}, {216, 16} }},	// 0
	{{128, 8}, 0, { { 80, 0}, { 80,  0}, { 88, 16} }},
	{{ 64, 8}, 0, { { 16, 0}, { 16,  0}, { 24, 16} }},

	{{ 32, 8}, 0, { {224, 0}, {224,  8}, {232, 24} }},	// 3
	{{224, 8}, 0, { {  0, 0}, {  0,  8}, {  8, 24} }},

	{{ 64, 8}, 0, { {192, 0}, {192, 16}, {200, 32} }},	// 5
	{{ 64, 8}, 0, { {128, 0}, {128, 16}, {136, 32} }},
	{{ 64, 8}, 0, { { 64, 0}, { 64, 16}, { 72, 32} }},
	{{ 32, 8}, 0, { { 32, 0}, { 32, 16}, { 40, 32} }},

	{{288, 8}, 0, { {-32, 0}, {-32, 24}, {-24, 40} }},	// 9

	{{ 48, 8}, 0, { {208, 0}, {208, 32}, {216, 48} }},	// 10
	{{128, 8}, 0, { { 80, 0}, { 80, 32}, { 88, 48} }},
	{{ 64, 8}, 0, { { 16, 0}, { 16, 32}, { 24, 48} }},
};
#define left_down_count array_size(left_down_views)

static testdata_t left_up_views[] = {
	{{ 48, 8}, 0, { {208, 0}, {208, 32}, {216, 48} }},	// 0
	{{128, 8}, 0, { { 80, 0}, { 80, 32}, { 88, 48} }},
	{{ 64, 8}, 0, { { 16, 0}, { 16, 32}, { 24, 48} }},

	{{ 32, 8}, 0, { {224, 0}, {224, 24}, {232, 40} }},	// 3
	{{224, 8}, 0, { {  0, 0}, {  0, 24}, {  8, 40} }},

	{{ 64, 8}, 0, { {192, 0}, {192, 16}, {200, 32} }},	// 5
	{{ 64, 8}, 0, { {128, 0}, {128, 16}, {136, 32} }},
	{{ 64, 8}, 0, { { 64, 0}, { 64, 16}, { 72, 32} }},
	{{ 32, 8}, 0, { { 32, 0}, { 32, 16}, { 40, 32} }},

	{{288, 8}, 0, { {-32, 0}, {-32,  8}, {-24, 24} }},	// 9

	{{ 48, 8}, 0, { {208, 0}, {208,  0}, {216, 16} }},	// 10
	{{128, 8}, 0, { { 80, 0}, { 80,  0}, { 88, 16} }},
	{{ 64, 8}, 0, { { 16, 0}, { 16,  0}, { 24, 16} }},
};
#define left_up_count array_size(left_up_views)

static testdata_t down_right_views[] = {
	{{ 8, 48}, 0, { { 0,  0}, {  0,  0}, {  8, 16} }},	// 0
	{{ 8,128}, 0, { { 0, 48}, {  0, 48}, {  8, 64} }},
	{{ 8, 64}, 0, { { 0,176}, {  0,176}, {  8,192} }},

	{{ 8, 32}, 0, { { 0,  0}, {  8,  0}, { 16, 16} }},	// 3
	{{ 8,224}, 0, { { 0, 32}, {  8, 32}, { 16, 48} }},

	{{ 8, 64}, 0, { { 0,  0}, { 16,  0}, { 24, 16} }},	// 5
	{{ 8, 64}, 0, { { 0, 64}, { 16, 64}, { 24, 80} }},
	{{ 8, 64}, 0, { { 0,128}, { 16,128}, { 24,144} }},
	{{ 8, 32}, 0, { { 0,192}, { 16,192}, { 24,208} }},

	{{ 8,288}, 0, { { 0,  0}, { 24,  0}, { 32, 16} }},	// 9

	{{ 8, 48}, 0, { { 0,  0}, { 32,  0}, { 40, 16} }},	// 10
	{{ 8,128}, 0, { { 0, 48}, { 32, 48}, { 40, 64} }},
	{{ 8, 64}, 0, { { 0,176}, { 32,176}, { 40,192} }},
};
#define down_right_count array_size(down_right_views)

static testdata_t up_right_views[] = {
	{{ 8, 48}, 0, { { 0,208}, {  0,208}, {  8,224} }},	// 0
	{{ 8,128}, 0, { { 0, 80}, {  0, 80}, {  8, 96} }},
	{{ 8, 64}, 0, { { 0, 16}, {  0, 16}, {  8, 32} }},

	{{ 8, 32}, 0, { { 0,224}, {  8,224}, { 16,240} }},	// 3
	{{ 8,224}, 0, { { 0,  0}, {  8,  0}, { 16, 16} }},

	{{ 8, 64}, 0, { { 0,192}, { 16,192}, { 24,208} }},	// 5
	{{ 8, 64}, 0, { { 0,128}, { 16,128}, { 24,144} }},
	{{ 8, 64}, 0, { { 0, 64}, { 16, 64}, { 24, 80} }},
	{{ 8, 32}, 0, { { 0, 32}, { 16, 32}, { 24, 48} }},

	{{ 8,288}, 0, { { 0,-32}, { 24,-32}, { 32,-16} }},	// 9

	{{ 8, 48}, 0, { { 0,208}, { 32,208}, { 40,224} }},	// 10
	{{ 8,128}, 0, { { 0, 80}, { 32, 80}, { 40, 96} }},
	{{ 8, 64}, 0, { { 0, 16}, { 32, 16}, { 40, 32} }},
};
#define up_right_count array_size(up_right_views)

static testdata_t down_left_views[] = {
	{{ 8, 48}, 0, { { 0,  0}, { 32,  0}, { 40, 16} }},	// 0
	{{ 8,128}, 0, { { 0, 48}, { 32, 48}, { 40, 64} }},
	{{ 8, 64}, 0, { { 0,176}, { 32,176}, { 40,192} }},

	{{ 8, 32}, 0, { { 0,  0}, { 24,  0}, { 32, 16} }},	// 3
	{{ 8,224}, 0, { { 0, 32}, { 24, 32}, { 32, 48} }},

	{{ 8, 64}, 0, { { 0,  0}, { 16,  0}, { 24, 16} }},	// 5
	{{ 8, 64}, 0, { { 0, 64}, { 16, 64}, { 24, 80} }},
	{{ 8, 64}, 0, { { 0,128}, { 16,128}, { 24,144} }},
	{{ 8, 32}, 0, { { 0,192}, { 16,192}, { 24,208} }},

	{{ 8,288}, 0, { { 0,  0}, {  8,  0}, { 16, 16} }},	// 9

	{{ 8, 48}, 0, { { 0,  0}, {  0,  0}, {  8, 16} }},	// 10
	{{ 8,128}, 0, { { 0, 48}, {  0, 48}, {  8, 64} }},
	{{ 8, 64}, 0, { { 0,176}, {  0,176}, {  8,192} }},
};
#define down_left_count array_size(down_left_views)

static testdata_t up_left_views[] = {
	{{ 8, 48}, 0, { { 0,208}, { 32,208}, { 40,224} }},	// 0
	{{ 8,128}, 0, { { 0, 80}, { 32, 80}, { 40, 96} }},
	{{ 8, 64}, 0, { { 0, 16}, { 32, 16}, { 40, 32} }},

	{{ 8, 32}, 0, { { 0,224}, { 24,224}, { 32,240} }},	// 3
	{{ 8,224}, 0, { { 0,  0}, { 24,  0}, { 32, 16} }},

	{{ 8, 64}, 0, { { 0,192}, { 16,192}, { 24,208} }},	// 5
	{{ 8, 64}, 0, { { 0,128}, { 16,128}, { 24,144} }},
	{{ 8, 64}, 0, { { 0, 64}, { 16, 64}, { 24, 80} }},
	{{ 8, 32}, 0, { { 0, 32}, { 16, 32}, { 24, 48} }},

	{{ 8,288}, 0, { { 0,-32}, {  8,-32}, { 16,-16} }},	// 9

	{{ 8, 48}, 0, { { 0,208}, {  0,208}, {  8,224} }},	// 10
	{{ 8,128}, 0, { { 0, 80}, {  0, 80}, {  8, 96} }},
	{{ 8, 64}, 0, { { 0, 16}, {  0, 16}, {  8, 32} }},
};
#define up_left_count array_size(up_left_views)

static void
print_view (view_t *view)
{
	printf ("%s[%3d %3d %3d %3d %3d %3d %3d %3d]\n",
			view->parent ? "    " : "****",
			view->xpos, view->ypos, view->xlen, view->ylen,
			view->xrel, view->yrel, view->xabs, view->yabs);
	view_draw (view);
}

static int
test_flow (testdata_t *child_views, int count, void (*flow) (view_t *))
{
	view_t     *flow_view = view_new (0, 0, 256, 256, grav_northwest);
	flow_view->setgeometry = flow;
	flow_view->flow_size = 1;
	flow_view->draw = print_view;

	for (int i = 0; i < count; i++) {
		testdata_t *td = &child_views[i];
		view_t     *child = view_new (0, 0, td->xlen, td->ylen, grav_flow);
		child->bol_suppress = td->bol_suppress;
		child->draw = print_view;
		view_add (flow_view, child);
	}

	view_move (flow_view, 8, 16);
	int         ret = 0;

	for (int i = 0; i < flow_view->num_children; i++) {
		testdata_t *td = &child_views[i];
		view_t     *child = flow_view->children[i];
		if (child->xpos != td->expect.xpos
			|| child->ypos != td->expect.ypos
			|| child->xrel != td->expect.xrel
			|| child->yrel != td->expect.yrel
			|| child->xabs != td->expect.xabs
			|| child->yabs != td->expect.yabs) {
			ret = 1;
			printf ("child %d misflowed:\n"
					"    [%3d %3d         %3d %3d %3d %3d]\n", i,
					td->expect.xpos, td->expect.ypos,
					td->expect.xrel, td->expect.yrel,
					td->expect.xabs, td->expect.yabs);
			print_view (child);
		}
	}
	return ret;
}

int
main (void)
{
	int         ret = 0;

	if (test_flow (right_down_views, right_down_count, view_flow_right_down)) {
		printf ("right-down failed\n");
		ret = 1;
	}
	if (test_flow (right_up_views, right_up_count, view_flow_right_up)) {
		printf ("right-up failed\n");
		ret = 1;
	}
	if (test_flow (left_down_views, left_down_count, view_flow_left_down)) {
		printf ("left-down failed\n");
		ret = 1;
	}
	if (test_flow (left_up_views, left_up_count, view_flow_left_up)) {
		printf ("left-up failed\n");
		ret = 1;
	}

	if (test_flow (down_right_views, down_right_count, view_flow_down_right)) {
		printf ("down-right failed\n");
		ret = 1;
	}
	if (test_flow (up_right_views, up_right_count, view_flow_up_right)) {
		printf ("up-right failed\n");
		ret = 1;
	}
	if (test_flow (down_left_views, down_left_count, view_flow_down_left)) {
		printf ("down-left failed\n");
		ret = 1;
	}
	if (test_flow (up_left_views, up_left_count, view_flow_up_left)) {
		printf ("up-left failed\n");
		ret = 1;
	}
	return ret;
}
#endif

int
main (void)
{
	printf ("FIXME: redo for ECS\n");
	return 1;
}
