#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <math.h>

#include <thread>

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Double_Window.H>
#include <FL/platform.H>
#include <FL/fl_draw.H>

#include "common.h"
#include "intro.h"

#define	INTERVAL	0.015

void	strip_lf(char *cp)
{
	while(*cp != '\0')
	{
		if((*cp == 10) || (*cp == 13))
		{
			*cp = '\0';
		}
		cp++;
	}
}

void	redraw_cb(void *v)
{
	IntroWindow *win = (IntroWindow *)v;
	if(win->sudden_stop == 0)
	{
		win->redraw();
		Fl::repeat_timeout(win->interval, redraw_cb, win);
	}
	else
	{
		win->hide();
		Fl::delete_widget(win);
	}
}

IntroWindow::IntroWindow(int ww, int hh, double in_interval) : Fl_Double_Window(ww, hh)
{
int	loop;

	sudden_stop = 0;
	strcpy(message, "");
	interval = in_interval;
	ndi_notice = 0;
	image_cnt = 0;
	for(loop = 0;loop < 128;loop++)
	{
		image[loop] = NULL;
	}
	Fl::add_timeout(0.0, redraw_cb, this);
}

IntroWindow::~IntroWindow()
{
int	loop;

	for(loop = 0;loop < image_cnt;loop++)
	{
		if(image[loop] != NULL)
		{
			free(image[loop]);
		}
	}
}

int		IntroWindow::handle(int event)
{
	int flag = 0;
	if(event == FL_KEYBOARD)
	{
		int key = Fl::event_key();
		if(key == FL_Escape)
		{
			if(placard == 1)
			{
				flag = 1;
			}
		}
	}
	if(flag == 0)
	{
		flag = Fl_Double_Window::handle(event);
	}
	return(flag);
}

void	IntroWindow::draw()
{
int	loop;

	if(placard == 1)
	{
		int nn = 32 * (Fl::h() / 1080);
		fl_color(FL_WHITE);
		fl_font(FL_HELVETICA, nn);
		fl_draw(message, 0, 0, w(), h() + 150, FL_ALIGN_CENTER);
		int start_x = (Fl::w() / 2) - ((image_cnt * 162) / 2);
		int start_y = (Fl::h() / 2) + 120;
		for(loop = 0;loop < image_cnt;loop++)
		{
			fl_draw_image((const uchar *)image[loop], start_x + (loop * 162), start_y, 160, 90, 4);
		}
		fl_color(FL_WHITE);
		fl_font(FL_HELVETICA, 64);
		fl_draw("DVPTZ", 0, 0, w(), h() - 250, FL_ALIGN_CENTER);

		char buf[256];
		sprintf(buf, "version %s", DVPTZ_VERSION_NUMBER);
		fl_font(FL_HELVETICA, 16);
		fl_draw(buf, 0, 0, w(), h() - 165, FL_ALIGN_CENTER);

		fl_font(FL_HELVETICA, 24);
		sprintf(buf, "Copyright %c%c %d, Mark Lasersohn", 0xC2, 0xA9, CURRENT_YEAR);
		fl_draw(buf, 0, 0, w(), h() - 60, FL_ALIGN_CENTER);
		if(ndi_notice == 1)
		{
			char buf[256];
			sprintf(buf, "NDI® is a registered trademark of Vizrt NDI AB");
			fl_font(FL_HELVETICA, 16);
			fl_draw(buf, 0, 0, w(), h(), FL_ALIGN_CENTER);
		}
	}
}

ClockWindow::ClockWindow(int ww, int hh) : IntroWindow(ww, hh, INTERVAL)
{
int	loop;

	placard = 1;

	stage_cnt = 0;
	tick = 0;
	inc = 1.0;

	srand48(time(0));
	stage_cnt = (int)(drand48() * 274.0);
	tick = (int)(drand48() * 34.0);
	inc = (drand48() * 1.75) + 1.0;

	int cnt = time(0) % 200;
	for(loop = 0;loop < 8192;loop++)
	{
		stage[loop] = cnt;
		cnt++;
		if(cnt > 200) cnt = 0;
		my_inc[loop] = 1;
	}
	border(0);
	color(FL_BLACK);
	end();
}

ClockWindow::~ClockWindow()
{
}

int		ClockWindow::handle(int event)
{
	int flag = 0;
	if(flag == 0)
	{
		flag = IntroWindow::handle(event);
	}
	return(flag);
}

void	ClockWindow::draw()
{
	fl_color(color());
	fl_rectf(0, 0, w(), h());

	int little = h() / 9;
	int sz = h() - (little * 2);
	DrawBox(0, little + (sz / 2), little, sz);
	tick++;
	if(tick >= stage_cnt)
	{
		tick = 0;
		inc += 0.01;
		if(inc > 4) inc = 0;
	}
	IntroWindow::draw();
}

void	ClockWindow::Rect(int xx, int yy, int ww, int hh, int i_stage)
{
	if(i_stage > 0)
	{
		fl_line(xx, yy, xx + ww, yy);
	}
	if(i_stage > 1)
	{
		fl_line(xx + ww, yy, xx + ww, yy + hh);
	}
	if(i_stage > 2)
	{
		fl_line(xx + ww, yy + hh, xx, yy + hh);
	}
	if(i_stage > 3)
	{
		fl_line(xx, yy + hh, xx, yy);
	}
}

void	ClockWindow::DrawBox(int depth, int xx, int yy, int sz)
{
int	col, row;

	if(sudden_stop == 0)
	{
		int n_sz = sz / 4;
		fl_color(FL_WHITE);
		if(stage_cnt < tick)
		{
			stage[stage_cnt] += my_inc[stage_cnt];
			if((stage[stage_cnt] > 200) || (stage[stage_cnt] < 0))
			{
				my_inc[stage_cnt] *= -1;
			}
		}
		fl_color(fl_rgb_color(25, 25, 25));
		fl_rect(xx, yy, sz, sz);

		int cc = 25 + ((225.0 / 200.0) * stage[stage_cnt]);
		unsigned char rr = (cc * my_inc[stage_cnt]);
		unsigned char gg = (cc * my_inc[stage_cnt]) / 2;
		unsigned char bb = (cc * my_inc[stage_cnt]) / 4;
		fl_color(fl_rgb_color(bb, gg, rr));

		int i_stage = stage[stage_cnt] / 15;
		Rect(xx, yy, sz, sz, i_stage);
		stage_cnt += inc;
		if(stage_cnt > 273) stage_cnt = 0;
		for(row = 0;row < 4;row++)
		{
			for(col = 0;col < 4;col++)
			{
				if(depth < 2)
				{
					int little = n_sz / 7;
					DrawBox(depth + 1, xx + little + (col * n_sz), yy + little + (row * n_sz), n_sz - (little * 2));
				}
			}
		}
	}
}

int read_stdin_with_timeout(char *buffer, int size, int timeout_sec)
{
fd_set readfds;
struct timeval tv;
int retval;

	int err = 0;
	FD_ZERO(&readfds);
	FD_SET(STDIN_FILENO, &readfds);
	tv.tv_sec = timeout_sec;
	tv.tv_usec = 0;

	retval = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv);
	if(retval == -1)
	{
		// System error occurred
		err =  -1;
	}
	else if(retval == 0)
	{
		// Timeout reached with no input
		err =  1;
	}
	if(fgets(buffer, size, stdin) == NULL)
	{
		// EOF or read error
		err = -1;
	}
	return(err);
}

int	communication_thread(ClockWindow *win)
{
char	buf[4096];
int		loop;

	int quit = 0;
	int timeout = 10;
	while(quit == 0)
	{
		int err = read_stdin_with_timeout(buf, 4096, timeout);
		if(err == 0)
		{
			strip_lf(buf);
			if(strcmp(buf, "quit") == 0)
			{
				quit = 1;
				win->hide();
				Fl::delete_widget(win);
			}
			else if(strncmp(buf, "ndi notice:", strlen("ndi notice")) == 0)
			{
				win->ndi_notice = 1;
			}
			else if(strncmp(buf, "image:", strlen("image:")) == 0)
			{
				char *cp = buf + strlen("image:");
				int sz = atoi(cp);
				int nn = fileno(stdin);
				win->image[win->image_cnt] = (char *)malloc(sz);
				cp = win->image[win->image_cnt];
				for(loop = 0;loop < sz;loop++)
				{
					char num_buf[256];
					fgets(num_buf, 256, stdin);
					*cp = atoi(num_buf);
					cp++;
				}
				win->image_cnt++;
			}
			else
			{
				strcpy(win->message, buf);
				win->redraw();
			}
		}
		else if(err == -1)
		{
			quit = 1;
			win->hide();
			Fl::delete_widget(win);
		}
	}
	return(0);
}

int	main(int argc, char **argv)
{
std::thread thread_id;

	ClockWindow *win = new ClockWindow(Fl::w(), Fl::h());
	win->placard = 1;
	win->show();
	thread_id = std::thread(communication_thread, win);
	thread_id.detach();
	Fl::run();
	return(0);
}
