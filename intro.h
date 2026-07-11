#ifndef	INTRO_H
#define	INTRO_H	1

class	IntroWindow : public Fl_Double_Window
{
public:
			IntroWindow(int ww, int hh, double in_interval);
			~IntroWindow();
	void	draw();
	int		handle(int event);

	char	message[4096];
	int		sudden_stop;
	int		placard;
	double	interval;
	char	*image[128];
	int		image_cnt;
	int		ndi_notice;
};

class	ClockWindow : public IntroWindow
{
public:
			ClockWindow(int ww, int hh);
			~ClockWindow();
	void	draw();
	int		handle(int event);

	void	DrawBox(int depth, int xx, int yy, int sz);
	void	Rect(int xx, int yy, int ww, int hh, int i_stage);

	int		tick;
	double	inc;
	int		stage[8192];
	int		my_inc[8192];
	int		stage_cnt;
};

class	SinWindow : public IntroWindow
{
public:
			SinWindow(int ww, int hh);
			~SinWindow();

	void	draw();
	int		handle(int event);

	double	angle;
	double	dir;
};

#endif
