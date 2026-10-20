// shape_dialog_demo.cpp
//
// "See-through gaps, opaque interactive widgets" for a floating,
// movable dialog -- using the X11 Shape extension via FLTK's native
// Fl_Window::shape(), NOT per-pixel alpha compositing.
//
// Every widget here is completely ordinary FLTK (Fl_Button, Fl_Input,
// Fl_Browser, whatever you like) -- no cairo, no custom visual, no
// custom colormap, none of the machinery that kept crashing in the
// ARGB approach. The window's actual visible/clickable REGION is cut
// down to the union of its widgets' rectangles; everywhere else isn't
// part of the window at all, so:
//   - the desktop/video behind shows through with no blending, no
//	 compositor required (this even works without Mutter/Cinnamon
//	 compositing active, unlike the ARGB or opacity approaches)
//   - by default, mouse clicks in the see-through gaps pass through to
//	 whatever window is stacked underneath, rather than being
//	 captured by the dialog
//
// TRADE-OFF vs true alpha: the cutout edge is hard/binary (aliased),
// not a soft translucent blend. There is no "70% opacity panel" look
// here -- a pixel is either a normal opaque widget pixel, or it isn't
// part of the window.
//
// UNVERIFIED SPOT (please confirm on your system): Fl_Bitmap "set"
// (1) vs "unset" (0) bit polarity for Fl_Window::shape() -- i.e.
// whether 1-bits mean "visible" or "excluded" -- is what I'd expect
// from XBM convention, but I have not been able to compile/run this
// to confirm. If the effect is inverted on first test, invert
// widget_mask_set()'s fill logic (fill 0 as the base, OR in 1s vs.
// fill 1 as the base, AND out with 0s).
//
// BUILD:
//   g++ -std=c++11 shape_dialog_demo.cpp -o shape_dialog_demo \
//	   $(fltk-config --cxxflags) $(fltk-config --ldflags)
//
// RUN:
//   FLTK_BACKEND=x11 ./shape_dialog_demo

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Bitmap.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Scroll.H>
#include <FL/platform.H>

#include <X11/extensions/shape.h>

#include <vector>
#include <cstring>
#include <cstdio>
#include <algorithm>

#include	"transparent_window.h"

ShapedDialog	*registered_shaped_dialog[1024];
int				registered_shaped_dialog_cnt = 0;

// ---------------------------------------------------------------------
// Recursively collect the window-relative rectangle of every visible
// widget under (and including) grp, clipped to the viewport of every
// enclosing Fl_Scroll (nested scrolls intersect correctly). FLTK widget
// x()/y() are already window-relative (not parent-relative), so no
// coordinate translation is needed regardless of nesting depth.
//
// Fl_Scroll clips its children only at DRAW time -- a child scrolled
// out of view still reports its full, unclipped rect via x()/y()/w()/h().
// Left unhandled, that would punch solid holes in the mask outside the
// actual visible viewport. `clip` tracks the intersection of every
// Fl_Scroll ancestor's own rect seen so far; each widget's contribution
// to the mask is intersected with it before being added.
//
// DESIGN CHOICE: Fl_Scroll's own bounding rect is deliberately NOT
// added to the mask (only its clipped children are) -- so empty space
// inside the scroll viewport (below a short list, between rows) stays
// see-through rather than reading as one solid panel. If you want the
// whole scroll pane to be opaque regardless of content, add an rect for
// the Fl_Scroll itself (clipped to `clip`) the same way ordinary
// widgets are added below.
// ---------------------------------------------------------------------
static TW_Rect intersect_rect(const TW_Rect &a, const TW_Rect &b)
{
	int x0 = std::max(a.x, b.x);
	int y0 = std::max(a.y, b.y);
	int x1 = std::min(a.x + a.w, b.x + b.w);
	int y1 = std::min(a.y + a.h, b.y + b.h);
	if (x1 <= x0 || y1 <= y0)
		return TW_Rect{ 0, 0, 0, 0 };   // empty
	return TW_Rect{ x0, y0, x1 - x0, y1 - y0 };
}

void collect_widget_rects(Fl_Group *grp, const TW_Rect &clip, std::vector<TW_Rect> &out, int off_x, int off_y)
{
	for(int i = 0; i < grp->children(); i++)
	{
		Fl_Widget *c = grp->child(i);
		if (!c->visible())
			continue;

		TW_Rect own{ c->x() + off_x, c->y() + off_y, c->w(), c->h() };

		TW_Rect visible = intersect_rect(own, clip);
		if((visible.w > 0) && (visible.h > 0))
			out.push_back(visible);

		if(Fl_Group *sub = dynamic_cast<Fl_Window *>(c))
		{
			TW_Rect child_clip = clip;
			if (dynamic_cast<Fl_Scroll *>(sub))
				child_clip = intersect_rect(clip, own);   // narrow to viewport
			collect_widget_rects(sub, child_clip, out, c->x(), c->y());
		}
		else if(Fl_Group *sub = dynamic_cast<Fl_Group *>(c))
		{
			TW_Rect child_clip = clip;
			if (dynamic_cast<Fl_Scroll *>(sub))
				child_clip = intersect_rect(clip, own);   // narrow to viewport
			collect_widget_rects(sub, child_clip, out);
		}
		else
		{
			TW_Rect visible = intersect_rect(own, clip);
			if (visible.w > 0 && visible.h > 0)
				out.push_back(visible);
		}
	}
}

void ShapeAwareScroll::draw()
{
	Fl_Scroll::draw();
	if (owner_ && (xposition() != last_x_ || yposition() != last_y_))
	{
		last_x_ = xposition();
		last_y_ = yposition();
		owner_->rebuild_shape();
	}
}
