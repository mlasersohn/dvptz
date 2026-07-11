class	VideoWindow;

typedef struct AppContext
{
	libvlc_instance_t *vlc_inst;
	libvlc_media_player_t *mp;
	
	unsigned int video_width;
	unsigned int video_height;
	unsigned char *pixel_buffer;
	pthread_mutex_t video_mutex;
	
	snd_pcm_t *alsa_handle;
	unsigned int audio_rate;
	unsigned int audio_channels;
	
	VideoWindow *window;
	Fl_Widget *video_surface;
	Fl_Group *control_group;
	Fl_Slider *scrub_slider;
	Fl_Button *pause_button;
	Fl_Repeat_Button *advance_frame_button;
	Fl_Repeat_Button *retreat_frame_button;
	Fl_Button *reset_button;
	Fl_Button *snapshot_button;
	Fl_Value_Slider *speed_slider;
	Fl_Value_Slider *volume_slider;
	Fl_Box *times_box;
	Fl_Box *filename_box;
	
	int is_user_scrubbing;
	int width;
	int height;
	int snap;
} AppContext;

class VideoSurface : public Fl_Widget
{
public:
	AppContext *ctx;
	int snap;

	VideoSurface(int X, int Y, int W, int H, AppContext *context) : Fl_Widget(X, Y, W, H, NULL)
	{
		ctx = context;
		ctx->snap = 0;
	}
	virtual void draw() override
	{
		extern void	save_rgb_as_png(VideoWindow *in_win, void *rgb, int ww, int hh);
		pthread_mutex_lock(&ctx->video_mutex);
		if(ctx->pixel_buffer != NULL)
		{
			// Crucial: Use ctx->video_width/height instead of w()/h() to prevent line distortion
			// The depth parameter is set to 3 for standard 24-bit RGB
			fl_draw_image(ctx->pixel_buffer, x(), y(), ctx->video_width, ctx->video_height, 3, 0);
			if(ctx->snap == 1)
			{
				if(ctx->window != NULL)
				{
					fl_color(FL_WHITE);
					fl_rect(x() + 4, y() + 4, ctx->video_width - 8, ctx->video_height - 8);
					save_rgb_as_png(ctx->window, ctx->pixel_buffer, ctx->video_width, ctx->video_height);
				}
				ctx->snap = 0;
			}
		}
		else
		{
			fl_color(FL_BLACK);
			fl_rectf(x(), y(), w(), h());
		}
		pthread_mutex_unlock(&ctx->video_mutex);
	}
};

class	VideoWindow : public Fl_Double_Window
{
public:
				VideoWindow(void *in_win, char **filenames, int ww, int hh, char *lbl);
				~VideoWindow();
	int			handle(int event);
	void		draw();

	void		Play();
	void		Stop();
	void		Pause();
	void		ShowControls();
	void		HideControls();
	void		RaiseControls();
	void		LowerControls();
	void		NewVideo(char *filename);
	void		Times(double& total, double& current);
	void		ApplyVolume(int16_t *buffer, int frames);
	
	void		*my_window;
	AppContext	*ctx;
	time_t		last_moved;
	int			show_controls;
	int			direction;
	int			px;
	int			py;
	char		*filename[64];
	GenericPopupMenu	*popup;
	double		volume;
};
