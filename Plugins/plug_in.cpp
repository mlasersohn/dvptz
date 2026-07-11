#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>

#include <iostream>
#include <string>
#include <memory>
#include <thread>
#include <chrono>

#include <osg/MatrixTransform>
#include <osgViewer/Viewer>
#include <osgViewer/CompositeViewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/TrackballManipulator>
#include <osgGA/TerrainManipulator>
#include <osgGA/FlightManipulator>
#include <osgGA/DriveManipulator>
#include <osgDB/ReadFile>

#include <Ultralight/Ultralight.h>
#include <AppCore/AppCore.h>

extern "C" 
{
#include <libavutil/avassert.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

#include	<vorbis/codec.h>
#include	<vorbis/vorbisenc.h>
#include	<vorbis/vorbisfile.h>

#include <pulse/simple.h>
#include <pulse/error.h>

#include <cef_app.h>
#include <cef_client.h>
#include <cef_parser.h>
#include <cef_render_handler.h>
#include <cef_life_span_handler.h>
#include <cef_load_handler.h>
#include <wrapper/cef_helpers.h>

#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/video.hpp>
#include <opencv2/core.hpp>

#include	<FL/Fl.H>
#include	<FL/Fl_Window.H>
#include	<FL/Fl_Double_Window.H>
#include	<FL/Fl_Input.H>
#include	<FL/Fl_Box.H>
#include	<FL/Fl_Button.H>
#include	<FL/Fl_Repeat_Button.H>
#include	<FL/Fl_Slider.H>
#include	<FL/Fl_Hor_Fill_Slider.H>
#include	<FL/Fl_Hor_Slider.H>
#include	<FL/Fl_Value_Slider.H>
#include	<FL/Fl_Input.H>
#include	<FL/Fl_Int_Input.H>
#include	<FL/Fl_Float_Input.H>
#include	<FL/Fl_Scroll.H>
#include	<FL/Fl_Pack.H>
#include	<FL/Fl_Menu_Button.H>
#include	<FL/Fl_Light_Button.H>
#include	<FL/Fl_Toggle_Button.H>
#include	<FL/Fl_Chart.H>
#include	<FL/Fl_Shared_Image.H>
#include	<FL/Fl_Choice.H>
#include	<FL/fl_draw.H>
#include	<FL/fl_ask.H>
#include	<FL/Fl_File_Chooser.H>
#include	<FL/Fl_Browser.H>
#include	<FL/Fl_Hold_Browser.H>
#include	<FL/Fl_Select_Browser.H>
#include	<FL/Fl_Output.H>
#include	<FL/Fl_Multiline_Output.H>
#include	<FL/Fl_Multiline_Input.H>
#include	<FL/Fl_Image_Surface.H>
#include	<FL/Fl_Color_Chooser.H>

#include        <X11/Xlib.h>
#include        <sys/shm.h>
#include        <sys/ipc.h>
#include        <X11/extensions/XShm.h>
#include        <X11/extensions/Xfixes.h>
#include        <X11/extensions/Xcomposite.h>
#include        <X11/extensions/Xrender.h>
#include        <X11/XKBlib.h>

#include	<visca/libvisca.h>
#include	<cairo.h>

#include	<vlc/vlc.h>
#include	</home/laser/SSD/OpenCV/MiscSource/osg.h>

#include    <Processing.NDI.Lib.h>
#include    <Processing.NDI.DynamicLoad.h>

using namespace cv;
using namespace dnn;
using namespace std;

#include    <fftw3.h>
#include    "libircclient.h"

#include	<cjson/cJSON.h>

#include "../MiscSource/image_memory.h"
#include "../Build/irc.h"
#include "../Build/PulseAudio.h"
#include "../Build/vlc_window.h"
#include "../Build/render_html.h"
#include "../Build/html_window.h"
#include "../Build/embed_app.h"
#include "../Build/common.h"
#include "../Build/dvptz.h"

class Mine : public Fl_Box
{
public:
	Mine(MyWin *in_win, int xx, int yy, int ww, int hh);
	~Mine();
	void	draw();
	int	handle(int event);

	MyWin	*my_win;
	int	cnt;
	int	advance;
};

Mine::Mine(MyWin *in_win, int xx, int yy, int ww, int hh) : Fl_Box(xx, yy, ww, hh)
{
	my_win = in_win;
	cnt = 0;
	advance = 0;
}

Mine::~Mine()
{
void	repeat_fltk_cb(void *v);

	Fl::remove_timeout(repeat_fltk_cb);
}

void	Mine::draw()
{
	Fl_Box::draw();
	if(my_win != NULL)
	{
		Camera *cam = my_win->camera[cnt];
		if(cam != NULL)
		{
			Mat local_mat;
			cv::resize(cam->mat, local_mat, cv::Size(w() - 10, h() - 10));
			fl_draw_image(local_mat.ptr(), x() + 5, y() + 5, w() - 10, h() - 10, 4);
			fl_color(FL_WHITE);
			fl_rect(x() + 10, y() + 10, 25, 25);
			fl_line(x() + 10, y() + 10, x() + 35, y() + 35);
			fl_line(x() + 35, y() + 10, x() + 10, y() + 35);
		}
		if(advance == 1)
		{
			cnt++;
			if(cnt >= my_win->source_cnt)
			{
				cnt = 0;
			}
			advance = 0;
		}
	}
}

int	Mine::handle(int event)
{
	int flag = 0;
	if(event == FL_PUSH)
	{
		int xx = Fl::event_x();
		int yy = Fl::event_y();
		int dx = xx - x();
		int dy = yy - y();
		if((dx < 35) && (dy < 35))
		{
			hide();
			delete this;
		}
		else
		{
			my_win->displayed_source = cnt;
		}
		flag = 1;
	}
	if(flag == 0)
	{
		flag = Fl_Box::handle(event);
	}
	return(flag);
}

void	repeat_fltk_cb(void *v)
{
	Mine *mine = (Mine *)v;
	mine->advance = 1;
	mine->redraw();
	Fl::repeat_timeout(1.0, repeat_fltk_cb, mine);
}

extern "C" 
{
	void	filter_opencv_sepia(int width, int height, int depth, unsigned char *ptr)
	{
		if(depth == 4)
		{
			cv::Mat mat = cv::Mat(height, width, CV_8UC4, ptr);
			cvtColor(mat, mat, cv::COLOR_BGRA2RGBA);
			cv::Mat kern = (cv::Mat_<float>(4,4) <<  0.272, 0.534, 0.131, 0,
				0.349, 0.686, 0.168, 0,
				0.393, 0.769, 0.189, 0,
				0, 0, 0, 1);
			transform(mat, mat, kern);
			cvtColor(mat, mat, cv::COLOR_RGBA2BGRA);
		}
		else
		{
			cv::Mat mat = cv::Mat(height, width, CV_8UC3, ptr);
			cvtColor(mat, mat, cv::COLOR_BGR2RGB);
			cv::Mat kern = (cv::Mat_<float>(4,4) <<  0.272, 0.534, 0.131, 0,
				0.349, 0.686, 0.168, 0,
				0.393, 0.769, 0.189, 0,
				0, 0, 0, 1);
			transform(mat, mat, kern);
			cvtColor(mat, mat, cv::COLOR_RGB2BGR);
		}
	}
	int global_frame_cnt = 0;
	void	filter_opencv_rectangle(int width, int height, int depth, unsigned char *ptr)
	{
		if(depth == 4)
		{
			cv::Mat mat = cv::Mat(height, width, CV_8UC4, ptr);
			rectangle(mat, cv::Point(0, global_frame_cnt), cv::Point(width - 1, global_frame_cnt + 4), cv::Scalar(255, 235, 80), cv::FILLED);
			global_frame_cnt++;
			if(global_frame_cnt > (height + 8))
			{
				global_frame_cnt = 0;
			}
		}
		else
		{
			cv::Mat mat = cv::Mat(height, width, CV_8UC3, ptr);
			rectangle(mat, cv::Point(100, 100), cv::Point(700, 700), cv::Scalar(255, 235, 80), cv::FILLED);
		}
	}
	void	audio_filter_cow(int number_of_samples, int number_of_channels, int sample_count, int hz, int buffer_size, double volume, void *buffer)
	{
printf("number_of_samples: %d number_of_channels: %d sample_count: %d hz: %d buffer_size: %d volume: %f buffer: %p\n", number_of_samples, number_of_channels, sample_count, hz, buffer_size, volume, buffer);
	}
	void	audio_filter_cow2(int number_of_samples, int number_of_channels, int sample_count, int hz, int buffer_size, double volume, void *buffer)
	{
printf("ALSO number_of_samples: %d number_of_channels: %d sample_count: %d hz: %d buffer_size: %d volume: %f buffer: %p\n", number_of_samples, number_of_channels, sample_count, hz, buffer_size, volume, buffer);
	}
	void	*pseudo_camera(char *name, int *width, int *height, int *depth)
	{
		static void *ptr = NULL;
		int xx, yy;

		ptr = malloc(1920 * 1080 * 4);
		*width = 1920;
		*height = 1080;
		*depth = 4;
		char *cp = (char *)ptr;
		for(yy = 0;yy < 1080;yy++)
		{
			for(xx = 0;xx < 1920;xx++)
			{
				if((xx >= 0) && (xx < (1920 / 3)))
				{
					*cp = 255;
					*(cp + 1) = 0;
					*(cp + 2) = 0;
					*(cp + 3) = 0;
				}
				else if((xx >= (1920 / 3)) && (xx < ((1920 / 3) * 2)))
				{
					*cp = 0;
					*(cp + 1) = 255;
					*(cp + 2) = 0;
					*(cp + 3) = 0;
				}
				else if((xx >= ((1920 / 3) * 2)) && (xx < 1920))
				{
					*cp = 0;
					*(cp + 1) = 0;
					*(cp + 2) = 255;
					*(cp + 3) = 0;
				}
				cp += 4;
			}
		}
		return(ptr);
	}
	void	*camera_test_pattern(char *name, int *width, int *height, int *depth)
	{
		static void *ptr = NULL;
		int xx, yy;

		ptr = malloc(1920 * 1080 * 4);
		*width = 1920;
		*height = 1080;
		*depth = 4;
		char *cp = (char *)ptr;
		for(yy = 0;yy < 1080;yy++)
		{
			for(xx = 0;xx < 1920;xx++)
			{
				if((xx >= 0) && (xx < (1920 / 3)))
				{
					*cp = 255;
					*(cp + 1) = 0;
					*(cp + 2) = 0;
					*(cp + 3) = 0;
				}
				else if((xx >= (1920 / 3)) && (xx < ((1920 / 3) * 2)))
				{
					*cp = 0;
					*(cp + 1) = 255;
					*(cp + 2) = 0;
					*(cp + 3) = 0;
				}
				else if((xx >= ((1920 / 3) * 2)) && (xx < 1920))
				{
					*cp = 0;
					*(cp + 1) = 0;
					*(cp + 2) = 255;
					*(cp + 3) = 0;
				}
				cp += 4;
			}
		}
		return(ptr);
	}
	void	local_cb(Fl_Widget *w, void *v)
	{
		Fl_Button *b = (Fl_Button *)v;
		b->do_callback();
		Fl_Window *win = w->window();
		if(win != NULL)
		{
			win->hide();
			win->parent()->remove(win);
			delete win;
		}
	}
	void	cancel_cb(Fl_Widget *w, void *v)
	{
		Fl_Button *b = (Fl_Button *)v;
		Fl_Window *win = w->window();
		if(win != NULL)
		{
			win->hide();
			win->parent()->remove(win);
			delete win;
		}
	}
	int	handle_events(Fl_Window *win, int event)
	{
		int loop;
		int flag = 0;
		if(event == FL_PUSH)
		{
			if(Fl::event_button() == 3)
			{
				int xx = Fl::event_x_root();
				int yy = Fl::event_y_root();
				Fl_Window *cow = new Fl_Window(xx, yy, 170, 800);
				cow->box(FL_FRAME_BOX);
				cow->color(FL_BLACK);
				cow->set_non_modal();
				cow->border(0);

				Fl_Box *title = new Fl_Box(0, 0, 170, 20, "Commands");
				title->box(FL_FLAT_BOX);
				title->color(FL_RED);
				title->labelcolor(FL_WHITE);
				title->labelsize(12);
				title->align(FL_ALIGN_INSIDE | FL_ALIGN_CENTER);

				int cnt = 1;
				Fl_Button *b = new Fl_Button(0, cnt * 20, 170, 20, "Cancel");
				b->box(FL_NO_BOX);
				b->labelcolor(FL_YELLOW);
				b->labelsize(12);
				b->clear_visible_focus();
				b->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);
				b->callback(cancel_cb, b);
				b->shortcut(FL_Escape);
				cnt++;

				MyWin *my_win = (MyWin *)win;
				for(loop = 0;loop < my_win->button_group->children();loop++)
				{
					Fl_Widget *wid = my_win->button_group->child(loop);
					if(wid != NULL)
					{
						if(wid->visible())
						{
							char *lbl = (char *)wid->label();
							if(lbl != NULL)
							{
								if(strlen(lbl) > 0)
								{
									Fl_Button *b = new Fl_Button(0, cnt * 20, 170, 20, lbl);
									b->box(FL_NO_BOX);
									b->labelcolor(FL_WHITE);
									b->labelsize(12);
									b->clear_visible_focus();
									b->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);
									b->callback(local_cb, wid);
									cnt++;
								}
							}
						}
					}
				}
				cow->end();
				cow->resize(cow->x(), cow->y(), cow->w(), cnt * 20);
				my_win->add(cow);
				cow->show();
				flag = 1;
			}
		}
		return(flag);
	}
	void	fltk_test(Fl_Window *in_win)
	{
		MyWin *my_win = (MyWin *)in_win;
		int ww = (my_win->display_width / 4) + 10;
		int hh = (my_win->display_height / 4) + 10;
		Mine *win = new Mine(my_win, (my_win->w() / 2) - 640, (Fl::h() / 2) - 360, ww, hh);
		win->box(FL_FRAME_BOX);
		win->color(FL_YELLOW);
		win->show();
		Fl::add_timeout(1.0, repeat_fltk_cb, win);
		my_win->resize_grp->add(win);
	}
	void	transition_simple1(int width, int height, int depth, unsigned char *ptr1, unsigned char *ptr2, unsigned char *ptr3, double amount)
	{
		if(depth == 4)
		{
			cv::Mat mat1 = cv::Mat(height, width, CV_8UC4, ptr1);
			cv::Mat mat2 = cv::Mat(height, width, CV_8UC4, ptr2);
			addWeighted(mat2, amount, mat1, 1.0 - amount, 0.0, mat2);
		}
	}
	void	transition_random(int width, int height, int depth, unsigned char *ptr1, unsigned char *ptr2, unsigned char *ptr3, double amount)
	{
		int row = 0;
		int col = 0;
		if(depth == 4)
		{
			cv::Mat mat1 = cv::Mat(height, width, CV_8UC4, ptr1);
			cv::Mat mat2 = cv::Mat(height, width, CV_8UC4, ptr2);
			cv::Mat out = cv::Mat(height, width, CV_8UC4, ptr3);
			for(row = 0;row < height;row++)
			{
				for(col = 0;col < width;col++)
				{
					double val = (drand48() * 8.0);
					if(val < amount)
					{
						cv::Vec4b source_pixel = mat2.at<cv::Vec4b>(row, col);
						out.at<cv::Vec4b>(row, col) = source_pixel;
					}
				}
			}
		}
	}
	void	transition_circular(int width, int height, int depth, unsigned char *ptr1, unsigned char *ptr2, unsigned char *ptr3, double amount)
	{
		int row = 0;
		int col = 0;
		if(depth == 4)
		{
			double wider = width;
			if(height > width) wider = height;
			double radius = wider / 2.0;
			radius *= amount;
			double center_x = width / 2.0;
			double center_y = height / 2.0;

			cv::Mat mat1 = cv::Mat(height, width, CV_8UC4, ptr1);
			cv::Mat mat2 = cv::Mat(height, width, CV_8UC4, ptr2);
			cv::Mat out = cv::Mat(height, width, CV_8UC4, ptr3);
			for(row = 0;row < height;row++)
			{
				for(col = 0;col < width;col++)
				{
					double dx = abs(center_x - col);
					double dy = abs(center_y - row);
					double dist = sqrt((dx * dx) + (dy * dy));
					if(dist < radius)
					{
						cv::Vec4b source_pixel = mat2.at<cv::Vec4b>(row, col);
						out.at<cv::Vec4b>(row, col) = source_pixel;
					}
				}
			}
		}
	}
	void	transition_rectangular(int width, int height, int depth, unsigned char *ptr1, unsigned char *ptr2, unsigned char *ptr3, double amount)
	{
		int row = 0;
		int col = 0;
		if(depth == 4)
		{
			double wider = width;
			if(height > width) wider = height;
			double radius = wider / 2.0;
			radius *= amount;
			double center_x = width / 2.0;
			double center_y = height / 2.0;

			cv::Mat mat1 = cv::Mat(height, width, CV_8UC4, ptr1);
			cv::Mat mat2 = cv::Mat(height, width, CV_8UC4, ptr2);
			cv::Mat out = cv::Mat(height, width, CV_8UC4, ptr3);
			for(row = 0;row < height;row++)
			{
				for(col = 0;col < width;col++)
				{
					double dx = abs(center_x - col);
					double dy = abs(center_y - row);
					if((dx < radius) && (dy < radius))
					{
						cv::Vec4b source_pixel = mat2.at<cv::Vec4b>(row, col);
						out.at<cv::Vec4b>(row, col) = source_pixel;
					}
				}
			}
		}
	}
}
