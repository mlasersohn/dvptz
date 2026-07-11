#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Slider.H>
#include <FL/Fl_Nice_Slider.H>
#include <FL/Fl_Value_Slider.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Repeat_Button.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/fl_draw.H>

#include <vlc/vlc.h>
#include <alsa/asoundlib.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include	"common.h"
#include	"video_player.h"

int	is_url(char *in)
{
	int flag = 0;
	char *cp = in;
	while((*cp != '\0') && (flag == 0))
	{
		if(strncmp(cp, "://", strlen("://")) == 0)
		{
			flag = 1;
		}
		cp++;
	}
	return(flag);
}

unsigned video_format_setup_cb(void **opaque, char *chroma, unsigned *width, unsigned *height, unsigned *pitches, unsigned *lines)
{
	AppContext *ctx = (AppContext *)(*opaque);
	memcpy(chroma, "RV24", 4);
	ctx->video_width = *width;
	ctx->video_height = *height;
	*pitches = (*width) * 3;
	*lines = *height;
	
	pthread_mutex_lock(&ctx->video_mutex);
	size_t sz = (size_t)(*width) * (size_t)(*height) * 3;
	ctx->pixel_buffer = (unsigned char *)realloc(ctx->pixel_buffer, sz);
	memset(ctx->pixel_buffer, 0, sz);
	pthread_mutex_unlock(&ctx->video_mutex);
	
	int ww = ctx->video_width;
	int hh = ctx->video_height;
	if(ww > Fl::w()) ww = Fl::w();
	if(hh > Fl::h() - 60) hh = Fl::h() - 60;
	ctx->window->resize(ctx->window->x(), ctx->window->y(), ww, hh);

	ctx->control_group->resize(10, ctx->window->h() - 50, ctx->window->w() - 20, 50);
	ctx->scrub_slider->resize(10, ctx->scrub_slider->y(), ww - 20, 20);
	ctx->retreat_frame_button->resize((ctx->window->w() / 2) - 22, ctx->retreat_frame_button->y(), 14, 14);
	ctx->pause_button->resize((ctx->window->w() / 2) - 8, ctx->pause_button->y(), 14, 14);
	ctx->advance_frame_button->resize((ctx->window->w() / 2) + 7, ctx->advance_frame_button->y(), 14, 14);
	ctx->reset_button->resize((ctx->window->w() / 2) - 50, ctx->reset_button->y(), 14, 14);
	ctx->snapshot_button->resize((ctx->window->w() / 2) + 25, ctx->snapshot_button->y(), 14, 14);
	ctx->speed_slider->resize((ctx->window->w() / 2) + 100, ctx->speed_slider->y(), 150, 14);
	ctx->volume_slider->resize((ctx->window->w() / 2) + 300, ctx->volume_slider->y(), 150, 14);
	ctx->times_box->resize((ctx->window->w() / 2) - 200, ctx->times_box->y(), 120, 14);
	ctx->filename_box->resize(20, ctx->filename_box->y(), 420, 14);
	Fl::awake();
	
	return 1;
}

void video_format_cleanup_cb(void *opaque)
{
}

void *video_lock_cb(void *opaque, void **planes)
{
	AppContext *ctx = (AppContext *)opaque;
	pthread_mutex_lock(&ctx->video_mutex);
	// Point LibVLC straight to our raw buffer array
	*planes = ctx->pixel_buffer;
	return NULL; 
}

void video_unlock_cb(void *opaque, void *picture, void *const *planes)
{
	AppContext *ctx = (AppContext *)opaque;
	pthread_mutex_unlock(&ctx->video_mutex);
}

void video_display_cb(void *opaque, void *picture)
{
	AppContext *ctx = (AppContext *)opaque;
	ctx->video_surface->redraw();
	Fl::awake();
}

int audio_format_setup_cb(void **opaque, char *format, unsigned *rate, unsigned *channels)
{
	AppContext *ctx = (AppContext *)(*opaque);
	memcpy(format, "S16N", 4);
	ctx->audio_rate = *rate;
	ctx->audio_channels = *channels;
	if(snd_pcm_open(&ctx->alsa_handle, "default", SND_PCM_STREAM_PLAYBACK, 0) >= 0)
	{
		snd_pcm_set_params(ctx->alsa_handle,
						   SND_PCM_FORMAT_S16_LE,
						   SND_PCM_ACCESS_RW_INTERLEAVED,
						   ctx->audio_channels,
						   ctx->audio_rate,
						   1,	   // Allow ALSA resampling internally if necessary
						   500000); // 0.5s max safe processing latency boundary
	}
	return(0);
}

void audio_format_cleanup_cb(void *opaque)
{
	AppContext *ctx = (AppContext *)opaque;
	if(ctx->alsa_handle != NULL)
	{
		snd_pcm_close(ctx->alsa_handle);
		ctx->alsa_handle = NULL;
	}
}

void audio_play_cb(void *opaque, const void *samples, unsigned count, int64_t pts)
{
	AppContext *ctx = (AppContext *)opaque;
	if((ctx->alsa_handle != NULL) && (ctx->is_user_scrubbing == 0))
	{
		ctx->window->ApplyVolume((int16_t *)samples, count);
		snd_pcm_sframes_t written = snd_pcm_writei(ctx->alsa_handle, samples, count);
		if(written < 0)
		{
			// Handles recovery automatically and safely prepares the stream
			snd_pcm_recover(ctx->alsa_handle, written, 0); 
		}
	}
}

void audio_pause_cb(void *opaque, int64_t pts) { }
void audio_resume_cb(void *opaque, int64_t pts) { }

void audio_flush_cb(void *opaque, int64_t pts)
{
	AppContext *ctx = (AppContext *)opaque;
	if (ctx->alsa_handle != NULL)
	{
		snd_pcm_drop(ctx->alsa_handle);
		snd_pcm_prepare(ctx->alsa_handle); // Restores device to PREPARED state cleanly
	}
}

void audio_drain_cb(void *opaque)
{
	AppContext *ctx = (AppContext *)opaque;
	snd_pcm_state_t state = snd_pcm_state(ctx->alsa_handle);
	if(state == SND_PCM_STATE_RUNNING) 
	{
		snd_pcm_drain(ctx->alsa_handle);
	}
	else
	{
		snd_pcm_drop(ctx->alsa_handle);
	}
}

void log_callback(void *data, int level, const libvlc_log_t *ctx, const char *fmt, va_list args)
{
}

void tracking_timer_cb(void *userdata)
{
	AppContext *ctx = (AppContext *)userdata;
	if((ctx->mp != NULL) && (ctx->is_user_scrubbing == 0))
	{
		float current_progress = libvlc_media_player_get_position(ctx->mp);
		if((current_progress >= 0.0f) && (current_progress <= 1.0f))
		{
			ctx->scrub_slider->value(current_progress);
			ctx->scrub_slider->redraw();
		}
	}
	Fl::repeat_timeout(0.033, tracking_timer_cb, userdata);
}

void window_close_callback(Fl_Widget* widget, void* userdata) 
{
	AppContext* ctx = (AppContext*)userdata;
	if(ctx->mp) 
	{
		// 1. Tell LibVLC to stop playing immediately
		libvlc_media_player_stop(ctx->mp);
	}
	if(ctx->alsa_handle) 
	{
		// 2. Clear any remaining audio frames out of the hardware device
		snd_pcm_drop(ctx->alsa_handle);
	}
	// 3. Hide the window so FLTK handles the closure
	widget->hide();
}

void slider_scrub_cb(Fl_Widget *widget, void *userdata)
{
extern time_t precise_time();
static time_t old = 0;

	AppContext *ctx = (AppContext *)userdata;
	Fl_Slider *slider = (Fl_Slider *)widget;
	switch(Fl::event()) 
	{
		case(FL_PUSH):
		{
			ctx->is_user_scrubbing = 1;
		}
		break;
		case(FL_DRAG):
		{
			libvlc_media_player_set_position(ctx->mp, slider->value());
		}
		break;
		case(FL_RELEASE):
		{
			ctx->is_user_scrubbing = 0;
			libvlc_media_player_set_position(ctx->mp, slider->value());
		}
		break;
	}
}

void volume_slider_cb(Fl_Widget *w, void *v)
{
	AppContext *ctx = (AppContext *)v;
	Fl_Slider *b = (Fl_Slider *)w;
	double vv = b->value();
	ctx->window->volume = vv;
}

void advance_frame_cb(Fl_Widget *w, void *v)
{
	AppContext *ctx = (AppContext *)v;
	libvlc_time_t tt2 = libvlc_media_player_get_time(ctx->mp);
	libvlc_media_player_set_time(ctx->mp, tt2 + 10);
}

void retreat_frame_cb(Fl_Widget *w, void *v)
{
	AppContext *ctx = (AppContext *)v;
	libvlc_time_t tt2 = libvlc_media_player_get_time(ctx->mp);
	libvlc_media_player_set_time(ctx->mp, tt2 - 10);
}

void toggle_pause_cb(Fl_Widget *w, void *v)
{
	AppContext *ctx = (AppContext *)v;
	Fl_Button *b = (Fl_Button *)w;
	if(libvlc_media_player_is_playing(ctx->mp))
	{
		b->label("@-4>");
		ctx->advance_frame_button->show();
		ctx->retreat_frame_button->show();
		ctx->snapshot_button->show();
	}
	else
	{
		b->label("@#-4||");
		ctx->advance_frame_button->hide();
		ctx->retreat_frame_button->hide();
		ctx->snapshot_button->hide();
	}
	libvlc_media_player_pause(ctx->mp);
}

void vp_reset_button_cb(Fl_Widget *w, void *v)
{
	AppContext *ctx = (AppContext *)v;
	if(libvlc_media_player_is_playing(ctx->mp))
	{
		ctx->pause_button->label("@#-4||");
		libvlc_media_player_stop(ctx->mp);
		libvlc_media_player_play(ctx->mp);
	}
	else
	{
		ctx->pause_button->label("@-4>");
		libvlc_media_player_stop(ctx->mp);
		libvlc_media_player_play(ctx->mp);
		libvlc_media_player_set_pause(ctx->mp, 1);
	}
}

void vp_snapshot_button_cb(Fl_Widget *w, void *v)
{
	AppContext *ctx = (AppContext *)v;
	if(!libvlc_media_player_is_playing(ctx->mp))
	{
		ctx->snap = 1;
	}
}

void speed_slider_cb(Fl_Widget *w, void *v)
{
	AppContext *ctx = (AppContext *)v;
	Fl_Slider *b = (Fl_Slider *)w;
	double val = b->value();
	libvlc_media_player_set_rate(ctx->mp, val);
}

void *restart_playback(void *mp) 
{
	libvlc_media_player_t *player = (libvlc_media_player_t *)mp;
	libvlc_media_player_stop(player);
	libvlc_media_player_play(player);
	return NULL;
}

void on_end_reached(const struct libvlc_event_t *event, void *user_data) 
{
	if(event->type == libvlc_MediaPlayerEndReached) 
	{
		AppContext *ctx = (AppContext *)user_data;
		pthread_t thread;
		pthread_create(&thread, NULL, (void* (*)(void*))restart_playback, ctx->mp);
		pthread_detach(thread);
	}
}

void	hide_controls_cb(void *v)
{
	VideoWindow *vw = (VideoWindow *)v;
	int diff = time(0) - vw->last_moved;
	int yy = Fl::event_y();
	int from_bottom = vw->h() - yy;
	if((diff > 4) && (from_bottom > 50))
	{
		vw->show_controls = 0;
		vw->HideControls();
	}
	else
	{
		vw->show_controls = 1;
		vw->ShowControls();
	}
	Fl::repeat_timeout(0.1, hide_controls_cb, vw);
}

VideoWindow::VideoWindow(void *in_win, char *filenames[65], int ww, int hh, char *lbl) : Fl_Double_Window(ww, hh, lbl)
{
int		loop;

	my_window = in_win;
	for(loop = 0;loop < 64;loop++)
	{
		filename[loop] = filenames[loop];
	}
	ctx = (AppContext *)calloc(1, sizeof(AppContext));
	ctx->width = 0;
	ctx->height = 0;
	pthread_mutex_init(&ctx->video_mutex, NULL);

	const char * const vlc_args[] = {
		  "-I", "dummy", // Don't use any interface
		  "--ignore-config", // Don't use VLC's config
		  "--quiet", 
		  "--no-video-title-show",
		  "--no-xlib",
		  "--verbose=0" // Don't be verbose
		   };

	// We launch VLC
	ctx->vlc_inst = libvlc_new(sizeof(vlc_args) / sizeof(vlc_args[0]), vlc_args);
	libvlc_log_set(ctx->vlc_inst, log_callback, NULL);
	
	libvlc_media_t *media = NULL;
	if(is_url(filename[0]))
	{
		media = libvlc_media_new_location(ctx->vlc_inst, filename[0]);
	}
	else
	{
		media = libvlc_media_new_path(ctx->vlc_inst, filename[0]);
	}
	ctx->mp = libvlc_media_player_new_from_media(media);

	libvlc_event_manager_t *ev_manager = libvlc_media_player_event_manager(ctx->mp);
	libvlc_event_attach(ev_manager, libvlc_MediaPlayerEndReached, on_end_reached, (void *)ctx);
	libvlc_media_release(media);

	libvlc_video_set_format_callbacks(ctx->mp, video_format_setup_cb, video_format_cleanup_cb);
	libvlc_video_set_callbacks(ctx->mp, video_lock_cb, video_unlock_cb, video_display_cb, ctx);
	
	libvlc_audio_set_format_callbacks(ctx->mp, audio_format_setup_cb, audio_format_cleanup_cb);
	libvlc_audio_set_callbacks(ctx->mp, audio_play_cb, audio_pause_cb, audio_resume_cb, audio_flush_cb, audio_drain_cb, ctx);

	Fl::option(Fl::OPTION_VISIBLE_FOCUS, false);
	ctx->window = this;
	
	ctx->video_surface = new VideoSurface(0, 0, ww, hh, ctx);

	ctx->control_group = new Fl_Group(0, 0, ww, 50);
	ctx->scrub_slider = new Fl_Slider(10, 0, 780, 25);
	ctx->scrub_slider->box(FL_FRAME);
	ctx->scrub_slider->slider(FL_FRAME_BOX);
	ctx->scrub_slider->color(FL_BLACK);
	ctx->scrub_slider->selection_color(FL_BLACK);
	ctx->scrub_slider->type(FL_HOR_SLIDER);
	ctx->scrub_slider->bounds(0.0, 1.0);
	ctx->scrub_slider->value(0.0);
	ctx->scrub_slider->when(FL_WHEN_RELEASE | ctx->scrub_slider->when());
	ctx->scrub_slider->callback(slider_scrub_cb, ctx);

	ctx->retreat_frame_button = new Fl_Repeat_Button(10, 30, 15, 15, "@#-4<-");
	ctx->retreat_frame_button->box(FL_FRAME_BOX);
	ctx->retreat_frame_button->color(FL_BLACK);
	ctx->retreat_frame_button->labelcolor(FL_WHITE);
	ctx->retreat_frame_button->hide();
	ctx->retreat_frame_button->tooltip("Retreat one frame");
	ctx->retreat_frame_button->callback(retreat_frame_cb, ctx);

	ctx->pause_button = new Fl_Button(10, 30, 15, 15, "@#-4||");
	ctx->pause_button->box(FL_FRAME_BOX);
	ctx->pause_button->color(FL_BLACK);
	ctx->pause_button->labelcolor(FL_WHITE);
	ctx->pause_button->tooltip("Toggle pause");
	ctx->pause_button->callback(toggle_pause_cb, ctx);

	ctx->advance_frame_button = new Fl_Repeat_Button(10, 30, 15, 15, "@#-4->");
	ctx->advance_frame_button->box(FL_FRAME_BOX);
	ctx->advance_frame_button->color(FL_BLACK);
	ctx->advance_frame_button->labelcolor(FL_WHITE);
	ctx->advance_frame_button->tooltip("Advance one frame");
	ctx->advance_frame_button->hide();
	ctx->advance_frame_button->callback(advance_frame_cb, ctx);

	ctx->reset_button = new Fl_Button(10, 30, 15, 15, "@#-4|<");
	ctx->reset_button->box(FL_FRAME_BOX);
	ctx->reset_button->color(FL_BLACK);
	ctx->reset_button->labelcolor(FL_WHITE);
	ctx->reset_button->tooltip("Reset to beginning");
	ctx->reset_button->callback(vp_reset_button_cb, ctx);

	ctx->snapshot_button = new Fl_Button(10, 30, 15, 15, "@#-4circle");
	ctx->snapshot_button->box(FL_FRAME_BOX);
	ctx->snapshot_button->color(FL_BLACK);
	ctx->snapshot_button->labelcolor(FL_RED);
	ctx->snapshot_button->tooltip("Save current frame to a PNG file");
	ctx->snapshot_button->hide();
	ctx->snapshot_button->callback(vp_snapshot_button_cb, ctx);

	ctx->speed_slider = new Fl_Value_Slider(10, 30, 150, 20, "Speed");
	ctx->speed_slider->box(FL_FRAME);
	ctx->speed_slider->color(FL_WHITE);
	ctx->speed_slider->selection_color(FL_WHITE);
	ctx->speed_slider->labelcolor(FL_YELLOW);
	ctx->speed_slider->textcolor(FL_WHITE);
	ctx->speed_slider->labelsize(9);
	ctx->speed_slider->bounds(0.1, 10.0);
	ctx->speed_slider->step(0.1);
	ctx->speed_slider->value(1.0);
	ctx->speed_slider->type(FL_HOR_NICE_SLIDER);
	ctx->speed_slider->align(FL_ALIGN_LEFT);
	ctx->speed_slider->tooltip("Adjust playback speed");
	ctx->speed_slider->callback(speed_slider_cb, ctx);

	ctx->volume_slider = new Fl_Value_Slider(10, 30, 150, 20, "Volume");
	ctx->volume_slider->box(FL_FRAME);
	ctx->volume_slider->color(FL_WHITE);
	ctx->volume_slider->selection_color(FL_WHITE);
	ctx->volume_slider->labelcolor(FL_YELLOW);
	ctx->volume_slider->textcolor(FL_WHITE);
	ctx->volume_slider->labelsize(9);
	ctx->volume_slider->bounds(0.0, 1.0);
	ctx->volume_slider->value(0.5);
	ctx->volume_slider->type(FL_HOR_NICE_SLIDER);
	ctx->volume_slider->align(FL_ALIGN_LEFT);
	ctx->volume_slider->tooltip("Adjust playback volume");
	ctx->volume_slider->callback(volume_slider_cb, ctx);

	ctx->times_box = new Fl_Box(10, 30, 120, 20, "");
	ctx->times_box->box(FL_FRAME);
	ctx->times_box->color(FL_BLACK);
	ctx->times_box->labelcolor(FL_WHITE);
	ctx->times_box->labelsize(0);
	ctx->times_box->align(FL_ALIGN_INSIDE | FL_ALIGN_CENTER);

	ctx->filename_box = new Fl_Box(10, 30, 420, 20, filename[0]);
	ctx->filename_box->box(FL_NO_BOX);
	ctx->filename_box->color(FL_BLACK);
	ctx->filename_box->labelcolor(FL_WHITE);
	ctx->filename_box->labelsize(0);
	ctx->filename_box->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);
	ctx->control_group->end();

	ctx->window->end();
	popup = NULL;
	show_controls = 0;
	direction = 0;
	last_moved = time(0);
	volume = 0.5;
	Fl::add_timeout(0.033, tracking_timer_cb, ctx);
	Fl::add_timeout(0.1, hide_controls_cb, this);
	callback(window_close_callback, ctx);
}

VideoWindow::~VideoWindow()
{
	libvlc_media_player_stop(ctx->mp);
	libvlc_media_player_release(ctx->mp);
	libvlc_release(ctx->vlc_inst);
	
	pthread_mutex_destroy(&ctx->video_mutex);
	if(ctx->pixel_buffer != NULL)
	{
		free(ctx->pixel_buffer);
	}
	free(ctx);
}

void	VideoWindow::draw()
{
	double total = 0.0;
	double current = 0.0;
	char buf[256];
	Times(total, current);
	sprintf(buf, "%.2f / %.2f", current, total);
	ctx->times_box->copy_label(buf);
	if(direction == 1)
	{
		if(ctx->control_group->y() > ctx->window->h() - 50)
		{
			RaiseControls();
		}
		else
		{
			direction = 0;
		}
	}
	else if(direction == -1)
	{
		if(ctx->control_group->y() < ctx->window->h())
		{
			LowerControls();
		}
		else
		{
			direction = 0;
		}
	}
	Fl_Double_Window::draw();
}

void	new_file_popup_cb(Fl_Widget *w, void *v)
{
extern int	alt_file_chooser(void *in_win, char *prompt, char *filter, char *start_path, char *current_selection, int select_dir = 0, int new_file = 0);

	VideoWindow *vw = (VideoWindow *)v;
	Fl_Hold_Browser *browser = (Fl_Hold_Browser *)w;
	char *str = (char *)browser->text(browser->value());
	if(str != NULL)
	{
		if(strcmp(str, "File or URL") == 0)
		{
			char filename[4096];
			int nn = alt_file_chooser(vw->my_window, "Select a video file or URL", "*", "./", filename);
			if(nn > 0)
			{
				vw->NewVideo(filename);
			}
		}
		else if(strcmp(str, "Cancel") == 0)
		{
		}
		else
		{
			vw->NewVideo(str);
		}
	}
	browser->window()->hide();
}

int		VideoWindow::handle(int event)
{
int		loop;

	int flag = 0;
	if(event == FL_MOVE)
	{
		int xx = Fl::event_x();
		int yy = Fl::event_y();
		int dx = px - xx;
		int dy = py - yy;
		if((dx > 0) || (dy > 0))
		{
			show_controls = 1;
			last_moved = time(0);
		}
		px = xx;
		py = yy;
		flag = 1;
	}
	else if(event == FL_PUSH)
	{
		if(Fl::event_state(FL_BUTTON3) == FL_BUTTON3)
		{
			if(popup == NULL)
			{
				popup = new GenericPopupMenu(Fl::event_x_root(), Fl::event_y_root(), 260, 300);
				popup->browser->callback(new_file_popup_cb, this);
			}
			else
			{
				popup->resize(Fl::event_x_root(), Fl::event_y_root(), popup->w(), popup->h());
			}
			if(popup != NULL)
			{
				popup->browser->clear();
				for(loop = 0;loop < 64;loop++)
				{
   					if(filename[loop] != NULL)
					{
						if(access(filename[loop], F_OK) == 0)
						{
							popup->browser->add(filename[loop]);
						}
					}
				}
				popup->browser->add("File or URL");
				popup->browser->add("Cancel");
				popup->set_non_modal();
				popup->Fit();
				popup->show();
			}
		}
	}
	if(flag == 0)
	{
		flag = Fl_Double_Window::handle(event);
	}
	return(flag);
}

void	VideoWindow::ApplyVolume(int16_t *buffer, int frames)
{
	int32_t sample;
	int total_samples = frames * ctx->audio_channels;
	for(int i = 0; i < total_samples; i++) 
	{
		sample = buffer[i];
		sample = (int32_t)(sample * volume);
		
		if (sample > 32767) sample = 32767;
		if (sample < -32768) sample = -32768;
		
		buffer[i] = (int16_t)sample;
	}
}

void	VideoWindow::LowerControls()
{
	int position = ctx->control_group->y() + 5;
	ctx->control_group->resize(10, position, ctx->window->w() - 20, 50);
}

void	VideoWindow::RaiseControls()
{
	int position = ctx->control_group->y() - 5;
	ctx->control_group->resize(10, position, ctx->window->w() - 20, 50);
}

void	VideoWindow::HideControls()
{
	direction = -1;
}

void	VideoWindow::ShowControls()
{
	direction = 1;
}

void	VideoWindow::Play()
{
	libvlc_media_player_play(ctx->mp);
}

void	VideoWindow::Stop()
{
	libvlc_media_player_stop(ctx->mp);
}

void	VideoWindow::Pause()
{
	libvlc_media_player_pause(ctx->mp);
}

void	VideoWindow::NewVideo(char *in_filename)
{
	int was_paused = 0;
	if(!libvlc_media_player_is_playing(ctx->mp))
	{
		was_paused = 1;
	}
	libvlc_media_t *new_media = NULL;
	if(is_url(in_filename))
	{
		new_media = libvlc_media_new_location(ctx->vlc_inst, in_filename);
	}
	else
	{
		new_media = libvlc_media_new_path(ctx->vlc_inst, in_filename);
	}
	libvlc_media_player_set_media(ctx->mp, new_media);
	libvlc_media_player_set_position(ctx->mp, 0.0);
	libvlc_media_player_play(ctx->mp);
	ctx->filename_box->copy_label(in_filename);
	if(was_paused == 1)
	{
		ctx->pause_button->label("@#-4||");
		ctx->advance_frame_button->hide();
		ctx->retreat_frame_button->hide();
		ctx->snapshot_button->hide();
	}
}

void	VideoWindow::Times(double& total, double& current)
{
	libvlc_time_t tt1 = libvlc_media_player_get_length(ctx->mp);
	if(tt1 > 0) 
	{
		total = (double)tt1 / 1000.0;
	}
	libvlc_time_t tt2 = libvlc_media_player_get_time(ctx->mp);
	if(tt2 > 0) 
	{
		current = (double)tt2 / 1000.0;
	}
}
