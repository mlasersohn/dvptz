#ifndef TRANSPARENT_WINDOW
#define TRANSPARENT_WINDOW 1
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

struct TW_Rect { int x, y, w, h; };
extern void collect_widget_rects(Fl_Group *grp, const TW_Rect &clip, std::vector<TW_Rect> &out, int off_x = 0, int off_y = 0);

// ---------------------------------------------------------------------
// (rebuild_shape logic now lives as ShapedDialog::rebuild_shape(),
// below, so the mask's backing storage has a well-defined lifetime
// instead of a function-local Fl_Bitmap or a fragile static buffer.)
// ---------------------------------------------------------------------

class ShapedDialog;   // forward declaration; ShapeAwareScroll below needs it

extern ShapedDialog	*registered_shaped_dialog[1024];
extern int				registered_shaped_dialog_cnt;

// ---------------------------------------------------------------------
// Fl_Scroll has no native "viewport scrolled" callback -- dragging its
// internal scrollbar is handled entirely inside Fl_Scroll itself, with
// nothing exposed to hook. The reliable way to detect that the
// viewport moved (mouse wheel, scrollbar drag, or keyboard) is to
// compare xposition()/yposition() on every draw() call, since draw()
// fires on every scroll-triggered redraw regardless of cause.
// ---------------------------------------------------------------------
class ShapeAwareScroll : public Fl_Scroll
{
public:
	ShapeAwareScroll(int x, int y, int w, int h, ShapedDialog *owner, const char *l = 0)
		: Fl_Scroll(x, y, w, h, l), owner_(owner), last_x_(0), last_y_(0)
	{
	}

	void draw() override;   // defined after ShapedDialog is complete

private:
	ShapedDialog *owner_;
	int		   last_x_, last_y_;
};

// ---------------------------------------------------------------------
// Example dialog: a window subclass that keeps its shape in sync with
// its own size. Widget layout inside is completely ordinary FLTK --
// nothing about them needs to know this technique is in use at all.
//
// The shape mask's backing pixel buffer AND the Fl_Bitmap wrapper are
// owned as members here (not function-locals) so they live at least as
// long as the window itself. Fl_Window::shape() may retain the
// Fl_Image* it's given for later reuse (e.g. to reapply the shape when
// the window is re-realized) rather than fully consuming it
// synchronously inside the call -- a function-local Fl_Bitmap destroyed
// the instant rebuild_shape() returns would leave that a dangling
// pointer, which is a very plausible crash source.
// ---------------------------------------------------------------------
class ShapedDialog : public Fl_Double_Window
{
public:
	ShapedDialog(int x, int y, int w, int h, const char *l = 0) : Fl_Double_Window(x, y, w, h, l), shape_bitmap_(nullptr)
	{
		old_w = w;
		old_h = h;
		no_shape = 1;
		registered_shaped_dialog[registered_shaped_dialog_cnt] = this;
		registered_shaped_dialog_cnt++;
	}
	ShapedDialog(int w, int h, const char *l = 0) : Fl_Double_Window(w, h, l), shape_bitmap_(nullptr)
	{
		old_w = w;
		old_h = h;
		no_shape = 1;
		registered_shaped_dialog[registered_shaped_dialog_cnt] = this;
		registered_shaped_dialog_cnt++;
	}
	~ShapedDialog() override
	{
		int loop;
		for(loop = 0;loop < registered_shaped_dialog_cnt;loop++)
		{
			if(registered_shaped_dialog[loop] == this)
			{
				registered_shaped_dialog[loop] = NULL;
			}
		}
		delete shape_bitmap_;
	}
	int handle(int event) override
	{
		int flag = 0;
		if(event == FL_PUSH)
		{
			push_x = Fl::event_x();
			push_y = Fl::event_y();
		}
		if(event == FL_DRAG)
		{
			int drag_x = Fl::event_x();
			int drag_y = Fl::event_y();
			int diff_x = drag_x - push_x;
			int diff_y = drag_y - push_y;
			resize(x() + diff_x, y() + diff_y, w(), h());
		}
		if(flag == 0)
		{
			flag = Fl_Double_Window::handle(event);
		}
		return(flag);
	}
	void resize(int X, int Y, int W, int H) override
	{
		Fl_Double_Window::resize(X, Y, W, H);
		if(shown())
		{
			rebuild_shape();
			old_w = W;
			old_h = H;
		}
	}
	// Rebuilds the shape mask from the current widget layout and size.
	// Call once after show(), and again any time layout or size changes.
	void rebuild_shape()
	{
		int W = w();
		int H = h();
		if (W <= 0 || H <= 0) return;

		int stride = (W + 7) / 8;
		shape_bits_.assign(stride * H, 0);   // start fully "unset" (hole)

		if(no_shape == 0)
		{
			std::vector<TW_Rect> rects;
			collect_widget_rects(this, TW_Rect{0, 0, W, H}, rects);

			for(const TW_Rect &r : rects)
			{
				int x0 = r.x < 0 ? 0 : r.x;
				int y0 = r.y < 0 ? 0 : r.y;
				int x1 = r.x + r.w > W ? W : r.x + r.w;
				int y1 = r.y + r.h > H ? H : r.y + r.h;

				for(int py = y0; py < y1; py++)
				{
					uchar *row = &shape_bits_[py * stride];
					for (int px = x0; px < x1; px++)
						row[px / 8] |= (1 << (px % 8));   // set = part of window
				}
			}
		}
		else
		{
			shape_bits_.assign(stride * H, 255);   // start fully "set" (solid)
		}
		// Replace the old Fl_Bitmap only AFTER building the new one's
		// backing data, and only delete the OLD wrapper once the new
		// shape() call has already taken effect -- never leave a
		// window in a state where shape() points at freed memory.
		Fl_Bitmap *new_bitmap = new Fl_Bitmap(shape_bits_.data(), W, H);
		shape(new_bitmap);
		delete shape_bitmap_;
		shape_bitmap_ = new_bitmap;
	}
	int				no_shape;
private:
	std::vector<uchar> shape_bits_;
	Fl_Bitmap		 *shape_bitmap_;
	int				push_x;
	int				push_y;
	int				old_w;
	int				old_h;
};

#endif
