#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <semaphore.h>
#include <pthread.h>

#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/video.hpp>

#include <vlc/vlc.h>

#include <pulse/simple.h>
#include <pulse/error.h>

#include	<FL/Fl.H>
#include	<FL/Fl_Window.H>
#include	<FL/Fl_Double_Window.H>
#include	<FL/fl_draw.H>

using namespace cv;
using namespace dnn;
using namespace std;

#include	"vlc_window.h"

int	create_task(int (*funct)(int *), void *flag);

void silent_log_callback(void *data, int level, const libvlc_log_t *ctx, const char *fmt, va_list args)
{
}

VLC_Window::VLC_Window(char *in_path, int ww, int hh)
{
int	loop;

	strcpy(path, in_path);
	volume = 0.5;
	muted = 0;
	stored_volume = volume;
	width = ww;
	height = hh;
	done = 0;
	videoBuffer = NULL;
	audioBuffer = NULL;
	videoBufferSize = 0;
	audioBufferSize = 0;
	media_player = NULL;
	video_cnt = 0;
	video_play_cnt = 0;
	last_video_pts = 0;
	audio_cnt = 0;
	audio_play_cnt = 0;
	audio_complete = 0;
	audio_tracks = -1;
	old_pts = 0;
	paused = 0;
	for(loop = 0;loop < BUFFER_LIMIT;loop++)
	{
		video_frame[loop] = NULL;
		audio_frame[loop] = NULL;
	}
	int ret = 1;
	int error;
	simple = NULL;
	libvlc_instance_t *vlcInstance;
	void *pUserData = this;

	char smem_options[1000];
	sprintf(smem_options,
		 ":sout=#transcode{vcodec=RV24,acodec=s16le}:smem{"
		 "video-prerender-callback=%lld,"
		 "video-postrender-callback=%lld,"
		 "audio-prerender-callback=%lld,"
		 "audio-postrender-callback=%lld,"
		 "audio-data=%lld,"
		 "video-data=%lld},"
	  , (long long int)(intptr_t)(void*)&cbVideoPrerender
	  , (long long int)(intptr_t)(void*)&cbVideoPostrender
	  , (long long int)(intptr_t)(void*)&cbAudioPrerender
	  , (long long int)(intptr_t)(void*)&cbAudioPostrender
	  , (long long int)this
	  , (long long int)this);

    const char * const vlc_args[] = {
          "-I", "dummy", // Don't use any interface
          "--ignore-config", // Don't use VLC's config
          "--quiet",
          "--no-video-title-show",
          "--no-xlib",
          "--verbose=0" // Don't be verbose
           };

	// We launch VLC
	vlcInstance = libvlc_new(sizeof(vlc_args) / sizeof(vlc_args[0]), vlc_args);
	libvlc_log_set(vlcInstance, silent_log_callback, NULL);

	media_player = libvlc_media_player_new(vlcInstance);

	libvlc_event_manager_t *eventManager = libvlc_media_player_event_manager(media_player);
	libvlc_event_attach(eventManager, libvlc_MediaPlayerTimeChanged, handleEvent, pUserData);
	libvlc_event_attach(eventManager, libvlc_MediaPlayerEndReached, handleEvent, pUserData);
	libvlc_event_attach(eventManager, libvlc_MediaPlayerPositionChanged, handleEvent, pUserData);

	if(access(path, 0) == 0)
	{
		media = libvlc_media_new_path(vlcInstance, path);
	}
	else
	{
		media = libvlc_media_new_location(vlcInstance, path);
	}
	libvlc_media_add_option(media, smem_options);
	libvlc_media_add_option(media, ":smem-options=#transcode{vcodec=RV32}:smem{no-time-sync}");
	libvlc_media_player_set_media(media_player, media);
}

VLC_Window::~VLC_Window()
{
int	loop;
void	play_cb(void *v);

	done = 1;
	Fl::remove_timeout(play_cb);
	usleep(100000);
	if(simple != NULL)
	{
		pa_simple_free(simple);
	}
	libvlc_media_release(media);
	libvlc_media_player_release(media_player);
	for(loop = 0;loop < BUFFER_LIMIT;loop++)
	{
		if(video_frame[loop] != NULL)
		{
			free(video_frame[loop]);
			video_frame[loop] = NULL;
		}
		if(audio_frame[loop] != NULL)
		{
			free(audio_frame[loop]);
			audio_frame[loop] = NULL;
		}
	}
}

void	vlc_window_start_cb(void *v)
{
	VLC_Window *vlc = (VLC_Window *)v;
	vlc->Start();
}

void	VLC_Window::Start()
{
int	play_audio(int *v);
void	play_cb(void *v);

	current_frame = 0;
	Fl::add_timeout(0.03, play_cb, this);
	create_task((int (*)(int *))play_audio, (void *)this);
	libvlc_media_player_play(media_player);
}

void	VLC_Window::ResetFrames()
{
int	loop;

	current_frame = 0;
	video_cnt = 0;
	video_play_cnt = 0;
	last_video_pts = 0;
	audio_cnt = 0;
	audio_play_cnt = 0;
	audio_complete = 0;
	last_audio_pts = 0;
	old_pts = 0;
	for(loop = 0;loop < BUFFER_LIMIT;loop++)
	{
		if(video_frame[loop] != NULL)
		{
			free(video_frame[loop]);
			video_frame[loop] = NULL;
		}
		if(audio_frame[loop] != NULL)
		{
			free(audio_frame[loop]);
			audio_frame[loop] = NULL;
		}
	}
}

void	VLC_Window::Draw()
{
unsigned int local_w, local_h;

	double pos = libvlc_media_player_get_position(media_player);
	libvlc_video_get_size(media_player, 0, &local_w, &local_h);
	int use_w = width;
	int use_h = height;
	int ran_loop = 0;
	int cnt = 0;
	while((video_frame[video_play_cnt] == NULL) && (cnt < 1024))
	{
		cnt++;
		video_play_cnt++;
		if(video_play_cnt >= BUFFER_LIMIT)
		{
			video_play_cnt = 0;
		}
	}
	int start = video_play_cnt;
	if(audio_tracks > 0)
	{
		while((video_pts[start] < last_audio_pts) && (video_frame[start] != NULL))
		{
			ran_loop++;
			if((video_pts[start] < last_audio_pts) && (video_frame[start] != NULL))
			{
				free(video_frame[start]);
				video_frame[start] = NULL;
			}
			start++;
			if(start >= BUFFER_LIMIT)
			{
				start = 0;
			}
			current_frame++;
		}
		video_play_cnt = start;
		if(video_frame[video_play_cnt] != NULL)
		{
			Mat src = Mat(local_h, local_w, CV_8UC3, video_frame[video_play_cnt]);
			cv::cvtColor(src, mat, COLOR_RGB2RGBA);
			cv::resize(mat, mat, Size(use_w, use_h));

			free(video_frame[video_play_cnt]);
			video_frame[video_play_cnt] = NULL;
			video_play_cnt++;
			if(video_play_cnt >= BUFFER_LIMIT)
			{
				video_play_cnt = 0;
			}
			current_frame++;
		}
	}
	else
	{
		audio_complete = 1;
		if(video_pts[video_play_cnt] <= last_video_pts)
		{
			if(video_frame[video_play_cnt] != NULL)
			{
				Mat src = Mat(local_h, local_w, CV_8UC3, video_frame[video_play_cnt]);
				cvtColor(src, mat, COLOR_RGB2RGBA);
				cv::resize(mat, mat, Size(use_w, use_h));
				free(video_frame[video_play_cnt]);
				video_frame[video_play_cnt] = NULL;
				video_play_cnt++;
				if(video_play_cnt >= BUFFER_LIMIT)
				{
					video_play_cnt = 0;
				}
				current_frame++;
			}
			else
			{
				video_play_cnt++;
				if(video_play_cnt >= BUFFER_LIMIT)
				{
					video_play_cnt = 0;
				}
			}
		}
		else
		{
		}
	}
}

void	VLC_Window::Despool()
{
	int start = video_play_cnt;
	while((video_pts[start] < last_audio_pts) && (video_frame[start] != NULL))
	{
		if((video_pts[start] < last_audio_pts) && (video_frame[start] != NULL))
		{
			free(video_frame[start]);
			video_frame[start] = NULL;
		}
		start++;
		if(start >= BUFFER_LIMIT)
		{
			start = 0;
		}
		current_frame++;
	}
	video_play_cnt = start;
}

int	play_audio(int *v)
{
	VLC_Window *dw = (VLC_Window *)v;
	dw->PlayAudio();
	return(0);
}

void	VLC_Window::PlayAudio()
{
int	error;
int	loop;

	while((done == 0) && (audio_tracks != 0))
	{
		audio_complete = 0;
		if(simple != NULL)
		{
			if(audio_frame[audio_play_cnt] != NULL)
			{
				if(paused == 0)
				{
					last_audio_pts = audio_pts[audio_play_cnt];
					short int *cc = (short int *)audio_frame[audio_play_cnt];
					for(loop = 0;loop < (audio_sz / 2);loop++)
					{
						short int val = *cc;
						val = (short int)((double)val * volume);
						*cc = val;
						cc++;
					}
					if(pa_simple_write(simple, audio_frame[audio_play_cnt], (size_t)audio_sz, &error) < 0) 
					{
						fprintf(stderr, "pa_simple_write() failed: %s\n", pa_strerror(error));
					}
					free(audio_frame[audio_play_cnt]);
					audio_frame[audio_play_cnt] = NULL;
					audio_play_cnt++;
					if(audio_play_cnt >= BUFFER_LIMIT)
					{
						audio_play_cnt = 0;
					}
				}
				else
				{
					usleep(100000);
				}
			}
			else
			{
				usleep(10000);
			}
		}
		else
		{
			usleep(10000);
		}
	}
	audio_complete = 1;
}

void cbAudioPrerender(void *p_audio_data, uint8_t** pp_pcm_buffer , unsigned int size)
{
	VLC_Window *dw = (VLC_Window *)p_audio_data;
	if((size > dw->audioBufferSize) || (!dw->audioBuffer))
	{
		if(dw->audioBuffer) free(dw->audioBuffer);
		dw->audioBuffer = (uint8_t *)malloc(size);
		dw->audioBufferSize = size;
	}
	*pp_pcm_buffer = dw->audioBuffer;
}

void cbAudioPostrender(void *p_audio_data, uint8_t *p_pcm_buffer, unsigned int channels, unsigned int rate, unsigned int nb_samples, unsigned int bits_per_sample, unsigned int size, int64_t pts)
{
static const	pa_sample_spec ss = {
	.format = PA_SAMPLE_S16LE,
	.rate = rate,
	.channels = (unsigned char)channels
};

	VLC_Window *dw = (VLC_Window *)p_audio_data;
	if(dw->simple == NULL)
	{
		int error = 0;
		if(!(dw->simple = pa_simple_new(NULL, "VLC_Window", PA_STREAM_PLAYBACK, NULL, "playback", &ss, NULL, NULL, &error)))
		{
			fprintf(stderr, "pa_simple_new() failed: %s\n", pa_strerror(error));
		}
	}
	long int inc = 5805;
	if(dw->old_pts > 0)
	{
		inc = (pts - dw->old_pts) / 4;
	}
	dw->audio_sz = size / 4;

	dw->audio_frame[dw->audio_cnt] = (unsigned char *)malloc(dw->audio_sz);
	memcpy(dw->audio_frame[dw->audio_cnt], p_pcm_buffer, dw->audio_sz);
	dw->audio_pts[dw->audio_cnt] = pts;
	dw->audio_cnt++;
	if(dw->audio_cnt >= BUFFER_LIMIT)
	{
		dw->audio_cnt = 0;
	}
	dw->audio_frame[dw->audio_cnt] = (unsigned char *)malloc(dw->audio_sz);
	memcpy(dw->audio_frame[dw->audio_cnt], p_pcm_buffer + dw->audio_sz, dw->audio_sz);
	dw->audio_pts[dw->audio_cnt] = pts + inc;
	dw->audio_cnt++;
	if(dw->audio_cnt >= BUFFER_LIMIT)
	{
		dw->audio_cnt = 0;
	}
	dw->audio_frame[dw->audio_cnt] = (unsigned char *)malloc(dw->audio_sz);
	memcpy(dw->audio_frame[dw->audio_cnt], p_pcm_buffer + (dw->audio_sz * 2), dw->audio_sz);
	dw->audio_pts[dw->audio_cnt] = pts + (inc * 2);
	dw->audio_cnt++;
	if(dw->audio_cnt >= BUFFER_LIMIT)
	{
		dw->audio_cnt = 0;
	}
	dw->audio_frame[dw->audio_cnt] = (unsigned char *)malloc(dw->audio_sz);
	memcpy(dw->audio_frame[dw->audio_cnt], p_pcm_buffer + (dw->audio_sz * 3), dw->audio_sz);
	dw->audio_pts[dw->audio_cnt] = pts + (inc * 3);
	dw->audio_cnt++;
	if(dw->audio_cnt >= BUFFER_LIMIT)
	{
		dw->audio_cnt = 0;
	}
	dw->old_pts = pts;
}

void cbVideoPrerender(void *p_video_data, uint8_t **pp_pixel_buffer, int size)
{
	VLC_Window *dw = (VLC_Window *)p_video_data;
	if((size > dw->videoBufferSize) || (!dw->videoBuffer))
	{
		if(dw->videoBuffer) free(dw->videoBuffer);
		dw->videoBuffer = (uint8_t *)malloc(size);
		dw->videoBufferSize = size;
	}
	*pp_pixel_buffer = dw->videoBuffer;
}

void cbVideoPostrender(void *p_video_data, uint8_t *p_pixel_buffer, int width, int height, int pixel_pitch, int size, int64_t pts) 
{
	VLC_Window *dw = (VLC_Window *)p_video_data;
	if(dw->done == 0)
	{
		if(dw->audio_tracks == -1)
		{
			dw->audio_tracks = libvlc_audio_get_track_count(dw->media_player);
		}
		dw->video_sz = size;
		dw->video_frame[dw->video_cnt] = (unsigned char *)malloc(size);
		if(dw->video_frame[dw->video_cnt] != NULL)
		{
			memcpy(dw->video_frame[dw->video_cnt], p_pixel_buffer, size);
			dw->video_pts[dw->video_cnt] = pts;
			dw->last_video_pts = pts;
			if(dw->audio_tracks < 1)
			{
				dw->audio_complete = 1;
				usleep(13000);
			}
			dw->video_cnt++;
			if(dw->video_cnt >= BUFFER_LIMIT)
			{
				dw->video_cnt = 0;
			}
		}
	}
}

static void handleEvent(const libvlc_event_t *pEvt, void *pUserData)
{
libvlc_time_t time;

	VLC_Window *dw = (VLC_Window *)pUserData;
	switch(pEvt->type)
	{
		case(libvlc_MediaPlayerTimeChanged):
		{
			time = libvlc_media_player_get_time(dw->media_player);
		}
		break;
		case(libvlc_MediaPlayerEndReached):
		{
			printf("MediaPlayerEndReached\n");
			dw->done = 1;
		}
		break;
		default:
		break;
	}	
}

void	play_cb(void *v)
{
long int precise_time(void);
static long int start = 0;

	VLC_Window *dw = (VLC_Window *)v;
	if(dw->done == 0)
	{
		if(dw->paused == 0)
		{
			start = precise_time();
			dw->Draw();
			Fl::repeat_timeout(0.03, play_cb, dw);
		}
		else
		{
			Fl::repeat_timeout(0.1, play_cb, dw);
		}
	}
	else
	{
		Fl::remove_timeout(play_cb);
		while(dw->audio_complete == 0)
		{
			usleep(100000);
		}
		dw->ResetFrames();
		dw->done = 0;
		sleep(1);
		libvlc_media_player_set_media(dw->media_player, dw->media);
		libvlc_media_player_set_time(dw->media_player, 0);
		libvlc_media_player_play(dw->media_player);
		while(dw->video_cnt < 5)
		{
			usleep(10000);
		}
		create_task((int (*)(int *))play_audio, (void *)dw);
		Fl::repeat_timeout(0.03, play_cb, dw);
	}
}

void	VLC_Window::Pause()
{
	paused = 1;
	libvlc_media_player_pause(media_player);
}

void	VLC_Window::Resume()
{
	paused = 0;
	libvlc_media_player_play(media_player);
}

void	VLC_Window::Volume(double val)
{
	volume += val;
	if(volume > 1.0) volume = 1.0;
	if(volume < 0.0) volume = 0.0;
}

void	VLC_Window::Mute()
{
	stored_volume = volume;
	volume = 0.0;
	muted = 1;
}

void	VLC_Window::UnMute()
{
	volume = stored_volume;
	muted = 0;
}

double	VLC_Window::Position()
{
	double pos = libvlc_media_player_get_position(media_player);
	return(pos);
}

void VLC_Window::Position(double new_position) 
{
	libvlc_media_player_t *mp = media_player;
	libvlc_audio_set_mute(mp, 1);
	int was_paused = 0;
	if(libvlc_media_player_is_playing(mp)) 
	{
		libvlc_media_player_set_pause(mp, 1);
		was_paused = 1;
	}
	libvlc_media_player_set_position(mp, new_position);
	libvlc_media_player_next_frame(mp);
	if(!libvlc_media_player_is_playing(mp))
	{
		libvlc_media_player_play(mp);
		usleep(50000);
	}
	libvlc_media_player_set_pause(mp, 1);
	libvlc_audio_set_mute(mp, 0);
	int cnt = 0;
	while((video_play_cnt != video_cnt) && (cnt < 1024))
	{
		Draw();
		cnt++;
	}
	libvlc_media_player_set_pause(mp, 0);
	libvlc_media_player_play(mp);
}

void VLC_Window::QuickPosition(double new_position) 
{
	libvlc_media_player_t *mp = media_player;
	libvlc_audio_set_mute(mp, 1);
	libvlc_media_player_set_position(mp, new_position);
	if(!libvlc_media_player_is_playing(mp))
	{
		libvlc_media_player_play(mp);
		usleep(100);
	}
	libvlc_media_player_set_pause(mp, 1);
	libvlc_audio_set_mute(mp, 0);
	int cnt = 0;
	while((video_play_cnt != video_cnt) && (cnt < 1024))
	{
		Despool();
		cnt++;
	}
	Draw();
	libvlc_media_player_set_pause(mp, 0);
	libvlc_media_player_play(mp);
}

void	VLC_Window::Stop()
{
	Fl::remove_timeout(play_cb);
	done = 1;
	if(audio_tracks > 0)
	{
		while(audio_complete == 0)
		{
			usleep(10000);
		}
	}
	libvlc_media_player_stop(media_player);
}
