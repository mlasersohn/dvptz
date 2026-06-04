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

#include <libavutil/pixfmt.h>
#include <libavutil/samplefmt.h>
#include <libavutil/avutil.h>
#include <libavutil/channel_layout.h>
#include <libavutil/pixdesc.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

#include	<fftw3.h>

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
#include	<FL/Fl_Flex.H>
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
#include	<FL/Fl_Tabs.H>
#include	<FL/Fl_Roller.H>
#include	<FL/Fl_Wizard.H>

#include	<X11/Xlib.h>
#include	<sys/shm.h>
#include	<sys/ipc.h>
#include	<X11/extensions/XShm.h>
#include	<X11/extensions/Xfixes.h>
#include	<X11/XKBlib.h>

#include	<visca/libvisca.h>

#include	<vlc/vlc.h>
#include	<cairo.h>

#include	"libircclient.h"
#include	<Processing.NDI.Lib.h>

#include	<cjson/cJSON.h>

using namespace cv;
using namespace dnn;
using namespace std;

#include "image_memory.h"
#include "PulseAudio.h"
#include "vlc_window.h"
#include "html_window.h"
#include "embed_app.h"
#include "osg.h"
#include "dvptz.h"
#include "muxer.h"

extern long int	precise_time();
int				my_find_codec_by_id(int type, int in_id, char *result);

void	crash_webcam()
{
	int xx = 12;
	int yy = 6 + 6;
	int nn = xx / (xx - yy);
}

int		create_task(int (*funct)(int *), void *flag);

MyFormat	*global_my_format[1024];
int			global_my_format_cnt = 0;
char		*global_log[128];
int			global_log_cnt = 0;

void logging_callback(void* ptr, int level, const char* fmt, va_list vl)
{
char	part[1024];
char	buf[2048];
int		type;

	if(global_log_cnt < 128)
	{
		av_log_format_line(ptr, level, fmt, vl, part, 1024, &type);
		char *cp = part;
		while(*cp != '\0')
		{
			if(*cp == '@') *cp = '_';
			if((*cp == 10) || (*cp == 13)) *cp = '\0';
			cp++;
		}
		snprintf(buf, 2048, "%ld: %s", time(0), part);
		global_log[global_log_cnt] = strdup(buf);
		global_log_cnt++;
	}
}

Muxer::Muxer(MyWin *in_win, ReviewWin *in_review, int from_raw)
{
	my_window = in_win;
	frame_ptr = NULL;
	review = in_review;
	mixer = NULL;
	raw = from_raw;
	raw_done = 0;
	mute = 0;
	not_audible = 0;
	stop_activity = 0;
	url = NULL;
	fresh_image = 0;
	in_simple_record = 0;
	oc = NULL;
	raw_frame = NULL;

	encode_video = 0;
	encode_audio = 0;
	current_frame = 0;
	realtime_factor = 1;
	video_st = { 0 };
	audio_st = { 0 };
	have_video = 0;
	have_audio = 0;
	recording = 0;
	video_frames = 0;
	audio_samples = 0;
	start_time = 0;
	frame_timecode = 0.0;
	paused = 0;
	time_to_write_frame = 0;
	average_time_to_write_frame = 1000000000.0;

	original_width = 0;
	original_height = 0;

	using_simple_stream = 0;
	simple_pulse_stream = NULL;
	desktop_monitor = NULL;

	number_of_samples = 0;;
	recordedSamples = NULL;
	number_of_audio_samples = 0;

 	av_log_set_callback(logging_callback);
}

Muxer::~Muxer()
{
	if(oc != NULL)
	{
		close_stream(oc, &video_st);
		close_stream(oc, &audio_st);
		avformat_free_context(oc);
		oc = NULL;
	}
	if(raw_frame != NULL)
	{
		free(raw_frame);
		raw_frame = NULL;
	}
	if(recordedSamples != NULL)
	{
		free(recordedSamples);
		recordedSamples = NULL;
	}
	if(simple_pulse_stream != NULL)
	{
		pa_simple_free(simple_pulse_stream);
		simple_pulse_stream = NULL;
	}
}

int Muxer::write_frame(AVFormatContext *fmt_ctx, AVCodecContext *c, AVStream *st, AVFrame *frame)
{
int ret;

	// send the frame to the encoder
	time_t start = precise_time();
	ret = avcodec_send_frame(c, frame);
	if(ret > -1) 
	{
		while(ret >= 0) 
		{
			AVPacket pkt = { 0 };
			ret = avcodec_receive_packet(c, &pkt);
			if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			{
				break;
			}
			else if(ret > -1) 
			{
				// rescale output packet timestamp values from codec to stream timebase
				av_packet_rescale_ts(&pkt, c->time_base, st->time_base);
				pkt.stream_index = st->index;
		
				ret = av_interleaved_write_frame(fmt_ctx, &pkt);
				av_packet_unref(&pkt);
				if(ret < 0) 
				{
					fprintf(stdout, "Error: Failed while writing output packet: %s\n", av_err2str(ret));
					time_to_write_frame += 1000000;
				}
			}
			else
			{
				fprintf(stdout, "Error: failed encoding a frame: %s\n", av_err2str(ret));
				time_to_write_frame += 1000000;
			}
		}
	}
	else
	{
		fprintf(stdout, "Error: While sending a frame to the encoder: %s\n", av_err2str(ret));
		time_to_write_frame += 1000000;
	}
	time_t elapsed = precise_time() - start;
	time_to_write_frame += elapsed;
	int div = 1;
	if(video_frames > 0)
	{
		div = video_frames;
	}
	average_time_to_write_frame = (double)((double)time_to_write_frame / 1000000.0) / (double)div;
	return ret == AVERROR_EOF ? 1 : 0;
}

enum AVCodecID	codec_by_name(char *name)
{
	AVCodecID id = (AVCodecID)0;
	AVCodec *codec = (AVCodec *)avcodec_find_encoder_by_name(name);
	if(codec != NULL)
	{
		id = codec->id;
	}
	return(id);
}

// COW COW
void print_codec_info(const AVCodec* codec) 
{
    if(!codec) 
	{
        std::cerr << "Codec not found!" << std::endl;
        return;
    }
    std::cout << "--- Codec Information ---" << std::endl;
    std::cout << "Name: " << codec->name << std::endl;
    std::cout << "Long Name: " << codec->long_name << std::endl;
    std::cout << "Type: "
              << av_get_media_type_string(codec->type) << std::endl;
    std::cout << "ID: " << codec->id << std::endl;
    std::cout << "Capabilities (Flags): " << std::hex << codec->capabilities << std::dec << std::endl;

    // Check and print specific capabilities
    std::cout << "Capabilities Details:" << std::endl;
    if(codec->capabilities & AV_CODEC_CAP_DR1) 
	{
        std::cout << " - Supports direct rendering method 1" << std::endl;
    }
    if(codec->capabilities & AV_CODEC_CAP_SLICE_THREADS) 
	{
        std::cout << " - Supports slice-level multithreading" << std::endl;
    }
    if(codec->capabilities & AV_CODEC_CAP_FRAME_THREADS) 
	{
        std::cout << " - Supports frame-level multithreading" << std::endl;
    }
    if(codec->capabilities & AV_CODEC_CAP_EXPERIMENTAL) 
	{
        std::cout << " - Experimental codec" << std::endl;
    }
    if(codec->capabilities & AV_CODEC_CAP_SMALL_LAST_FRAME) 
	{
        std::cout << " - Can handle small last frames" << std::endl;
    }
    if(codec->type == AVMEDIA_TYPE_VIDEO) 
	{
        std::cout << "\nSupported Pixel Formats:" << std::endl;
        if(codec->pix_fmts) 
		{
            const AVPixelFormat* p = codec->pix_fmts;
            while(*p != AV_PIX_FMT_NONE) 
			{
                const char* fmt_name = av_get_pix_fmt_name(*p);
                std::cout << " - " << (fmt_name ? fmt_name : "Unknown") << std::endl;
                p++;
            }
        } 
		else 
		{
            std::cout << " - None listed (generic support)" << std::endl;
        }
    } 
	else if(codec->type == AVMEDIA_TYPE_AUDIO) 
	{
        std::cout << "\nSupported Sample Formats:" << std::endl;
        if(codec->sample_fmts) 
		{
            const AVSampleFormat* p = codec->sample_fmts;
            while(*p != AV_SAMPLE_FMT_NONE) 
			{
                const char* fmt_name = av_get_sample_fmt_name(*p);
                std::cout << " - " << (fmt_name ? fmt_name : "Unknown") << std::endl;
                p++;
            }
        }
        std::cout << "\nSupported Sample Rates:" << std::endl;
        if(codec->supported_samplerates) 
		{
            const int* p = codec->supported_samplerates;
            while(*p != 0) 
			{
                std::cout << " - " << *p << " Hz" << std::endl;
                p++;
            }
        }
        std::cout << "\nSupported Channel Layouts:" << std::endl;
        if(codec->channel_layouts) 
		{
            const uint64_t* p = codec->channel_layouts;
            while(*p != 0) 
			{
                char buf[128];
                av_get_channel_layout_string(buf, sizeof(buf), 0, *p);
                std::cout << " - " << buf << std::endl;
                p++;
            }
        }
    }
    std::cout << "-------------------------" << std::endl;
}
// COW COW

// Add an output stream.
void Muxer::add_stream(int use_nvidia, OutputStream *ost, AVFormatContext *oc, const AVCodec **codec, enum AVCodecID codec_id, int in_width, int in_height, double in_fps, double in_hz)
{
	AVCodecContext *c;
	int i;

	// find the encoder
	// COW - Forced "h264_nvenc" to use nvidia hardware to encode
	// GPU *codec = avcodec_find_encoder_by_name("h264_nvenc");
	// CPU *codec = avcodec_find_encoder(codec_id);

	if((codec_id == AV_CODEC_ID_H264) && (use_nvidia == 1))
	{
		*codec = avcodec_find_encoder_by_name("h264_nvenc");
		if(*codec == NULL)
		{
			*codec = avcodec_find_encoder(codec_id);
		}
	}
	else
	{
		*codec = avcodec_find_encoder(codec_id);
	}
	if(!(*codec)) 
	{
		fprintf(stdout, "Error: Could not find encoder for '%s'\n", avcodec_get_name(codec_id));
		exit(1);
	}
	// COW COW
	// COW COW print_codec_info(*codec);
	// COW COW
	ost->st = avformat_new_stream(oc, NULL);
	if(!ost->st) 
	{
		fprintf(stdout, "Error: Could not allocate stream\n");
		exit(1);
	}
	ost->st->id = oc->nb_streams - 1;
	c = avcodec_alloc_context3(*codec);
	if(c) 
	{
		ost->enc = c;

		switch ((*codec)->type) 
		{
			case AVMEDIA_TYPE_AUDIO:
			{
				if(url != NULL)
				{
					c->sample_fmt = AV_SAMPLE_FMT_FLTP;
				}
				else
				{
					c->sample_fmt = (*codec)->sample_fmts ?  (*codec)->sample_fmts[0] : AV_SAMPLE_FMT_S16;
				}
				c->bit_rate = 128000;
				c->sample_rate = -1;
				if((*codec)->supported_samplerates) 
				{
					for(i = 0; (*codec)->supported_samplerates[i]; i++) 
					{
						if((*codec)->supported_samplerates[i] == (int)in_hz)
						{
							c->sample_rate = (int)in_hz;
						}
					}
				}
				else
				{
					c->sample_rate = (int)in_hz;
				}
				if(c->sample_rate != -1)
				{
					c->channels = av_get_channel_layout_nb_channels(c->channel_layout);
					if(used_channels == 1)
					{
						c->channel_layout = AV_CH_LAYOUT_MONO;
					}
					else
					{
						c->channel_layout = AV_CH_LAYOUT_STEREO;
					}
					if((*codec)->channel_layouts) 
					{
						c->channel_layout = (*codec)->channel_layouts[0];
						for(i = 0; (*codec)->channel_layouts[i]; i++) 
						{
							if((*codec)->channel_layouts[i] == AV_CH_LAYOUT_STEREO)
							{
								c->channel_layout = AV_CH_LAYOUT_STEREO;
							}
						}
					}
					c->channels = av_get_channel_layout_nb_channels(c->channel_layout);
					ost->st->time_base = (AVRational){ 1, c->sample_rate };
				}
				else
				{
					fprintf(stdout, "Error: Illegal sample rate %d for this codec\n", (int)in_hz);
					if((*codec)->supported_samplerates) 
					{
						for(i = 0; (*codec)->supported_samplerates[i]; i++) 
						{
							fprintf(stdout, "%d ", (*codec)->supported_samplerates[i]);
						}
						fprintf(stdout, "\n");
					}
				}
			}
			break;
			case AVMEDIA_TYPE_VIDEO:
			{
				c->codec_id = codec_id;
				c->bit_rate = (in_height * in_width * 24 * in_fps) / 1024; 

				// COW - does there need to be a minimum? 
				// COW REMOVED if(c->bit_rate < 2500000) c->bit_rate = 2500000;
				if(c->bit_rate < 2500000) c->bit_rate = 2500000;

				// Resolution must be a multiple of two.
				c->width = in_width;
				c->height = in_height;

				// timebase: This is the fundamental unit of time (in seconds) in terms
				// of which frame timestamps are represented. For fixed-fps content,
				// timebase should be 1/framerate and timestamp increments should be
				// identical to 1.

				ost->st->time_base = (AVRational){ 1, (int)in_fps };
				c->time_base = ost->st->time_base;

				c->gop_size = 30; /* emit one intra frame every 30 frames at most */
				c->pix_fmt = STREAM_PIX_FMT;
				if((*codec)->pix_fmts)
				{
					// COW - TRY TO USE FORMAT SUPPLIED BY CODEC
					c->pix_fmt = (*codec)->pix_fmts[0];
				}
				if(c->codec_id == AV_CODEC_ID_MPEG2VIDEO) 
				{
					// just for testing, we also add B-frames
					c->max_b_frames = 2;
				}
				if(c->codec_id == AV_CODEC_ID_MPEG1VIDEO) 
				{
					// Needed to avoid using macroblocks in which some coeffs overflow.
					// This does not happen with normal video, it just happens here as
					// the motion of the chroma plane does not match the luma plane.
					c->mb_decision = 2;
				}
			}
			break;
			default:
			break;
		}
		// Some formats want stream headers to be separate.
		if(oc->oformat->flags & AVFMT_GLOBALHEADER)
		{
			c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
		}
	}
	else
	{
		fprintf(stdout, "Error: Could not alloc an encoding context\n");
	}
}

// audio output
AVFrame *Muxer::alloc_audio_frame(enum AVSampleFormat sample_fmt, uint64_t channel_layout, int sample_rate, int nb_samples)
{
int ret;

	AVFrame *frame = av_frame_alloc();
	if(frame) 
	{
		frame->format = sample_fmt;
		frame->channel_layout = channel_layout;
		frame->sample_rate = sample_rate;
		frame->nb_samples = nb_samples;
		if(nb_samples) 
		{
			ret = av_frame_get_buffer(frame, 0);
			if(ret < 0) 
			{
				fprintf(stdout, "Error: Failed allocating an audio buffer\n");
				exit(1);
			}
		}
	}
	else
	{
		fprintf(stdout, "Error: Failed allocating an audio frame\n");
	}
	return(frame);
}

int Muxer::open_audio(AVFormatContext *oc, const AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg)
{
AVCodecContext *c;
int nb_samples;
int ret;
AVDictionary *opt = NULL;

	c = ost->enc;
	
	// open it
	av_dict_copy(&opt, opt_arg, 0);
	ret = avcodec_open2(c, codec, &opt);
	av_dict_free(&opt);
	if(ret < 0) 
	{
		fprintf(stdout, "Error: Could not open audio codec: %s\n", av_err2str(ret));
		return(-100);
	}
	// init signal generator
	ost->t = 0;
	if(c->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
	{
		nb_samples = 1024;
	}
	else
	{
		nb_samples = c->frame_size;
	}
	number_of_audio_samples = nb_samples;

	// COW - NOTE FORCING NUMBER OF AUDIO SAMPLES TO WHAT MY MICS WANT
	nb_samples = 1024;
	c->frame_size = 1024;
	number_of_audio_samples = 1024;

	ost->frame = alloc_audio_frame(c->sample_fmt, c->channel_layout, c->sample_rate, nb_samples);
	if(used_channels == 1)
	{
		ost->tmp_frame = alloc_audio_frame(AV_SAMPLE_FMT_S16, AV_CH_LAYOUT_MONO, c->sample_rate, nb_samples);
	}
	else
	{
		ost->tmp_frame = alloc_audio_frame(AV_SAMPLE_FMT_S16, AV_CH_LAYOUT_STEREO, c->sample_rate, nb_samples);
	}
	// copy the stream parameters to the muxer
	ret = avcodec_parameters_from_context(ost->st->codecpar, c);
	if(ret < 0)
	{
		fprintf(stdout, "Error: Could not copy the stream parameters\n");
		return(-200);
	}

	// create resampler context
	ost->swr_ctx = swr_alloc();
	if(!ost->swr_ctx) 
	{
		fprintf(stdout, "Error: Could not allocate resampler context\n");
		return(-300);
	}
	// set options
	av_opt_set_int	   (ost->swr_ctx, "in_channel_count",   c->channels,	   0);
	av_opt_set_int	   (ost->swr_ctx, "in_sample_rate",	 c->sample_rate,	0);
	av_opt_set_sample_fmt(ost->swr_ctx, "in_sample_fmt",	  AV_SAMPLE_FMT_S16, 0);
	av_opt_set_int	   (ost->swr_ctx, "out_channel_count",  c->channels,	   0);
	av_opt_set_int	   (ost->swr_ctx, "out_sample_rate",	c->sample_rate,	0);
	av_opt_set_sample_fmt(ost->swr_ctx, "out_sample_fmt",	 c->sample_fmt,	 0);

	// initialize the resampling context
	if((ret = swr_init(ost->swr_ctx)) < 0) 
	{
		fprintf(stdout, "Error: Failed to initialize the resampling context\n");
		return(-400);
	}
	return(0);
}

AVFrame *Muxer::get_audio_frame(OutputStream *ost, void *in_buffer)
{
AVFrame *frame = ost->tmp_frame;
int j, i, v;
int loop;
short int raw_tmp_buffer[32768];

	if(raw == 0)
	{
		memcpy(frame->data[0], (uint8_t *)in_buffer, frame->nb_samples * 2 * used_channels);
	}
	else
	{
		int nn = frame->nb_samples * 2 * used_channels;
		int rn = read(raw_audio_fd, raw_tmp_buffer, nn);
		if(rn != nn)
		{
			raw_done = 1;
		}
		else
		{
			short int *p1 = raw_tmp_buffer;
			short int *p2 = (short int *)frame->data[0];
			for(loop = 0;loop < frame->nb_samples;loop++)
			{
				*p2++ = *p1++;
				if(used_channels == 2)
				{
					*p2++ = *p1++;
				}
			}
		}
	}
	if(raw_done == 0)
	{
		frame->pts = ost->next_pts;
		ost->next_pts += frame->nb_samples;
	}
	return(frame);
}

// Encode one audio frame and send it to the muxer
// return 1 when encoding is finished, 0 otherwise
int Muxer::write_audio_frame(AVFormatContext *oc, OutputStream *ost, void *in_buffer)
{
AVCodecContext *c;
AVFrame *frame;
int ret;
int dst_nb_samples;

	int rr = -1;
	c = ost->enc;
	frame = get_audio_frame(ost, in_buffer);
	if(frame != NULL) 
	{
		// convert samples from native format to destination codec format, using the resampler
		// compute destination number of samples
		dst_nb_samples = av_rescale_rnd(frame->nb_samples, c->sample_rate, c->sample_rate, AV_ROUND_UP);
		av_assert0(dst_nb_samples == frame->nb_samples);
		
		// when we pass a frame to the encoder, it may keep a reference to it internally;
		// make sure we do not overwrite it here
		ret = av_frame_make_writable(ost->frame);
		if(ret > -1)
		{
			// convert to destination format
			ret = swr_convert(ost->swr_ctx, ost->frame->data, dst_nb_samples, (const uint8_t **)frame->data, frame->nb_samples);
			if(ret > -1) 
			{
				frame = ost->frame;
				frame->pts = av_rescale_q(ost->samples_count, (AVRational){1, c->sample_rate}, c->time_base);
				ost->samples_count += dst_nb_samples;
				if(c->codec != NULL)
				{
					rr = write_frame(oc, c, ost->st, frame);
				}
				else
				{
					fprintf(stdout, "Error: Audio codec is NULL\n");
				}
			}
			else
			{
				fprintf(stdout, "Error: while converting\n");
			}
		}
		else
		{
			fprintf(stdout, "Error: Cannot make frame writable\n");
		}
	}
	return(rr);
}

// video output
AVFrame *Muxer::alloc_picture(enum AVPixelFormat pix_fmt, int width, int height)
{
AVFrame *picture;
int ret;

	picture = av_frame_alloc();
	if(!picture)
	{
		return(NULL);
	}
	picture->format = pix_fmt;
	picture->width  = width;
	picture->height = height;

	// allocate the buffers for the frame data
	ret = av_frame_get_buffer(picture, 0);
	if(ret < 0) 
	{
		fprintf(stdout, "Error: Could not allocate frame data.\n");
	}
	return(picture);
}

int Muxer::open_video(AVFormatContext *oc, const AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg)
{
int ret;
AVCodecContext *c = ost->enc;
AVDictionary *opt = NULL;

	av_dict_copy(&opt, opt_arg, 0);

	// open the codec
	ret = avcodec_open2(c, codec, &opt);
	av_dict_free(&opt);
	if(ret < 0) 
	{
		fprintf(stdout, "Error: Could not open video codec: %s\n", av_err2str(ret));
		return(-1);
	}
	// allocate and init a re-usable frame
	ost->frame = alloc_picture(c->pix_fmt, c->width, c->height);
	if(!ost->frame) 
	{
		fprintf(stdout, "Error: Could not allocate video frame\n");
		return(-2);
	}
	// If the output format is not YUV420P, then a temporary YUV420P
	// picture is needed too. It is then converted to the required
	// output format.
	ost->tmp_frame = NULL;
	if(c->pix_fmt != AV_PIX_FMT_YUV420P) 
	{
		ost->tmp_frame = alloc_picture(AV_PIX_FMT_YUV420P, c->width, c->height);
		if(!ost->tmp_frame) 
		{
			fprintf(stdout, "Error: Could not allocate temporary picture\n");
			return(-3);
		}
	}
	// copy the stream parameters to the muxer
	ret = avcodec_parameters_from_context(ost->st->codecpar, c);
	if(ret < 0) 
	{
		fprintf(stdout, "Error: Could not copy the stream parameters\n");
		return(-3);
	}
	return(0);
}

void Muxer::fill_yuv_image(AVFrame *pict, int frame_index, int width, int height)
{
	// Y, Cb and Cr
	pthread_mutex_lock(&my_window->mux_mutex);
	unsigned char *ptr = (unsigned char *)GetFrame();
	if(ptr != NULL)
	{
		// COW NOTE: I am no longer copying this data from one frame to the other
		// COW NOTE: This may cause the original "data" to be left unfreed
		// COW NOTE: and this data (from a Mat) to be freed in its stead
		int size = height * width;
		pict->data[0] = ptr;						// Y plane
		pict->data[1] = pict->data[0] + size;		// U plane
		pict->data[2] = pict->data[1] + size / 4;	// V plane (assuming 4:2:0 subsampling)

		// manually setting linesizes (assuming no padding for simplicity)
		pict->linesize[0] = width;
		pict->linesize[1] = width / 2;
		pict->linesize[2] = width / 2;
	}
	pthread_mutex_unlock(&my_window->mux_mutex);
}

AVFrame *Muxer::get_video_frame(OutputStream *ost)
{
	AVCodecContext *c = ost->enc;
	// when we pass a frame to the encoder, it may keep a reference to it
	// internally; make sure we do not overwrite it here
	if(av_frame_make_writable(ost->frame) > -1)
	{
		if(c->pix_fmt != AV_PIX_FMT_YUV420P) 
		{
			// as we only generate a YUV420P picture, we must convert it
			// to the codec pixel format if needed
			int no_go = 0;
			if(!ost->sws_ctx) 
			{
				ost->sws_ctx = sws_getContext(
					c->width, 
					c->height, 
					AV_PIX_FMT_YUV420P, 
					c->width, 
					c->height, 
					c->pix_fmt, 
					SCALE_FLAGS, 
					NULL, 
					NULL, 
					NULL);
				if(!ost->sws_ctx) 
				{
					fprintf(stdout, "Error: Could not initialize the conversion context\n");
					no_go = 1;
				}
			}
			if(no_go == 0)
			{
				fill_yuv_image(ost->tmp_frame, ost->next_pts, c->width, c->height);
				sws_scale(
					ost->sws_ctx, 
					(const uint8_t * const *)ost->tmp_frame->data, 
					ost->tmp_frame->linesize, 
					0, 
					c->height, 
					ost->frame->data, 
					ost->frame->linesize);
			}
		}
		else 
		{
			fill_yuv_image(ost->frame, ost->next_pts, c->width, c->height);
		}
	}
	else
	{
		fprintf(stdout, "Error: Cannot make frame writable\n");
	}
	ost->frame->pts = ost->next_pts;
	ost->next_pts++;
	return(ost->frame);
}

// encode one video frame and send it to the muxer
// return 1 when encoding is finished, 0 otherwise
int Muxer::write_video_frame(AVFormatContext *oc, OutputStream *ost)
{
	int rr = 0;
	AVFrame *frame = get_video_frame(ost);
	if(frame != NULL)
	{
		if((frame->width > 0) && (frame->height > 0))
		{
			if(ost != NULL)
			{
				if(ost->enc != NULL)
				{
					if(ost->enc->codec != NULL)
					{
						rr = write_frame(oc, ost->enc, ost->st, frame);
					}
					else
					{
						fprintf(stdout, "Error: OST->ENC->CODEC is NULL\n");
					}
				}
				else
				{
					fprintf(stdout, "Error: OST->ENC is NULL\n");
				}
			}
			else
			{
				fprintf(stdout, "Error: OST is NULL\n");
			}
		}
		else
		{
			fprintf(stdout, "Error: Video frame size is zero\n");
		}
	}
	return(rr);
}

void	Muxer::Flush()
{
	avio_flush(oc->pb);
}

void Muxer::close_stream(AVFormatContext *oc, OutputStream *ost)
{
	if(ost != NULL)
	{
		avcodec_free_context(&ost->enc);
		if(ost->frame)
		{
			av_frame_free(&ost->frame);
		}
		if(ost->tmp_frame)
		{
			av_frame_free(&ost->tmp_frame);
		}
		if(ost->sws_ctx) 
		{
			sws_freeContext(ost->sws_ctx);
		}
		if(ost->swr_ctx) 
		{
			swr_free(&ost->swr_ctx);
		}
	}
}

void	Muxer::EncodeAudioAndVideo(void *in_buffer)
{
extern long int precise_time();

	int save_encode_audio = encode_audio;
	if((realtime_factor != 1) || (not_audible == 1))
	{
		encode_audio = 1;
	}
	int encode_done = 0;
	while((encode_done == 0) && (stop_activity == 0))
	{
		if((video_st.enc != NULL) && (audio_st.enc != NULL))
		{
			if(encode_video && (!encode_audio || av_compare_ts(video_st.next_pts, video_st.enc->time_base, audio_st.next_pts, audio_st.enc->time_base) <= 0)) 
			{
				if(realtime_factor > 0)
				{
					if((current_frame % realtime_factor) == 0)
					{
						int rr = write_video_frame(oc, &video_st);
						if(rr == -1)
						{
							encode_done = 1;
						}
						if(!encode_audio)
						{
							usleep(15000);
						}
					}
				}
				else if(realtime_factor < 0)
				{
					int nn = abs(realtime_factor);
					for(int loop = 0;loop < nn;loop++)
					{
						int rr = write_video_frame(oc, &video_st);
						if(rr == -1)
						{
							encode_done = 1;
						}
					}
				}
				current_frame++;
			}
			else
			{
				encode_done = 1;
			}
		}
		else if(video_st.enc != NULL)
		{
			int rr = write_video_frame(oc, &video_st);
			if(rr == -1)
			{
				encode_done = 1;
			}
			usleep(12000);
			current_frame++;
			if(my_window != NULL)
			{
				my_window->recorded_frames = current_frame;
			}
		}
		else
		{
			encode_done = 1;
		}
	}
	if(encode_audio)
	{
		if(in_buffer != NULL)
		{
			write_audio_frame(oc, &audio_st, in_buffer);
		}
	}
	encode_audio = save_encode_audio;
}

int	CheckBadName(int type, char *name)
{
int		loop;
char	buf[256];

	int bad = 0;
	FILE *fp = NULL;
	if(type == CHECK_TYPE_AUDIO)
	{
		fp = fopen("audio_codec_bad_list.txt", "r");
	}
	else if(type == CHECK_TYPE_VIDEO)
	{
		fp = fopen("video_codec_bad_list.txt", "r");
	}
	else if(type == CHECK_TYPE_CONTAINER)
	{
		fp = fopen("container_bad_list.txt", "r");
	}
	if(fp != NULL)
	{
		while(fgets(buf, 256, fp))
		{
			if(strncasecmp(name, buf, strlen(name)) == 0)
			{
				bad = 1;
			}
		}
		fclose(fp);
	}
	return(bad);
}

int	CheckGoodName(int type, char *name)
{
int		loop;
char	buf[256];

	int good = 0;
	FILE *fp = NULL;
	if(type == CHECK_TYPE_AUDIO)
	{
		fp = fopen("audio_codec_good_list.txt", "r");
	}
	else if(type == CHECK_TYPE_VIDEO)
	{
		fp = fopen("video_codec_good_list.txt", "r");
	}
	else if(type == CHECK_TYPE_CONTAINER)
	{
		fp = fopen("container_good_list.txt", "r");
	}
	if(fp != NULL)
	{
		while(fgets(buf, 256, fp))
		{
			if(strncasecmp(name, buf, strlen(name)) == 0)
			{
				good = 1;
			}
		}
		fclose(fp);
	}
	return(good);
}

AVOutputFormat	*find_output_format_by_name(char *name)
{
	AVOutputFormat *rr = NULL;
	void *muxerState = nullptr;
	const AVOutputFormat *ofmt = av_muxer_iterate(&muxerState);
	while(ofmt) 
	{
		if(strcmp(name, ofmt->name) == 0)
		{
			rr = (AVOutputFormat *)ofmt;
		}
		ofmt = av_muxer_iterate(&muxerState);
	}
	return(rr);
}

void enumerate_codecs() 
{
	std::vector<const AVCodec*> encoderList;
	void *codecState = nullptr;
	const AVCodec *codec = av_codec_iterate(&codecState);
	while(codec) 
	{
		encoderList.push_back(codec);
		codec = av_codec_iterate(&codecState);
	}
	void *muxerState = nullptr;
	const AVOutputFormat *ofmt = av_muxer_iterate(&muxerState);
	while(ofmt) 
	{
		int bad = CheckBadName(CHECK_TYPE_CONTAINER, (char *)ofmt->name);
		int good = CheckGoodName(CHECK_TYPE_CONTAINER, (char *)ofmt->name);
		if((bad == 0) || (good == 1))
		{
			if(ofmt->extensions != NULL)
			{
				if(strlen(ofmt->extensions) > 0)
				{
					MyFormat *mine = new MyFormat((char *)ofmt->name, (char *)ofmt->extensions, (AVOutputFormat *)ofmt);
					for(auto codec : encoderList) 
					{
						if(av_codec_is_encoder(codec))
						{
							if(avformat_query_codec(ofmt, codec->id, FF_COMPLIANCE_NORMAL) == 1) 
							{
								if((codec->capabilities & AV_CODEC_CAP_EXPERIMENTAL) == 0) 
								{
									if(avcodec_get_type(codec->id) == AVMEDIA_TYPE_AUDIO) 
									{
										int no_go = 0;
										char *name = (char *)avcodec_get_name(codec->id);
										if(name != NULL)
										{
											if(name != NULL)
											{
												int bad = CheckBadName(CHECK_TYPE_AUDIO, name);
												int good = CheckGoodName(CHECK_TYPE_AUDIO, name);
												if((bad == 0) || (good == 1))
												{
													mine->AddAudio(name, codec->id);
												}
											}
										}
									}
									if(avcodec_get_type(codec->id) == AVMEDIA_TYPE_VIDEO) 
									{
										if(codec->id == AV_CODEC_ID_H263)
										{
											// COW - for some reason, this one needs to be added by hand
											mine->AddVideo((char *)avcodec_get_name(AV_CODEC_ID_H264), AV_CODEC_ID_H264);
										}
										else
										{
											int no_go = 0;
											char *name = (char *)avcodec_get_name(codec->id);
											if(name != NULL)
											{
												int bad = CheckBadName(CHECK_TYPE_VIDEO, name);
												int good = CheckGoodName(CHECK_TYPE_VIDEO, name);
												if((bad == 0) || (good == 1))
												{
													mine->AddVideo(name, codec->id);
												}
											}
										}
									}
								}
							}
						}
					}
					if((mine->video_codec_cnt > 0) && (mine->audio_codec_cnt > 0))
					{
						global_my_format[global_my_format_cnt] = mine;
						global_my_format_cnt++;
					}
					else
					{
						delete mine;
					}
				}
			}
		}
		ofmt = av_muxer_iterate(&muxerState);
	}
}

void enumerate_test(MyWin *in_win, void (*output_cb)(char *), int test_w, int test_h, int test_fps, int test_hz) 
{
int		loop;
int		video;
int		audio;
int		inner;
char	buf[1024];
char	filename[4096];
char	used_extension[256][24];

	FILE *fp = NULL;
	if(in_win != NULL)
	{
		fp = fopen("test_codecs.txt", "w");
	}
	int extensions_used = 0;
	int local_sudden_stop = 0;
	for(loop = 0;((loop < global_my_format_cnt) && (local_sudden_stop == 0));loop++)
	{
		if(in_win != NULL)
		{
			local_sudden_stop = in_win->mux_test_sudden_stop;
		}
		MyFormat *mf = global_my_format[loop];
		strcpy(buf, mf->extensions);
		char *cp = buf;
		while(*cp != '\0')
		{
			if(*cp == ',') *cp = '\0';
			cp++;
		}
		int no_go = 0;
		for(inner = 0;((inner < extensions_used) && (no_go == 0));inner++)
		{
			if(strcmp(buf, used_extension[inner]) == 0)
			{
				no_go = 1;
			}
		}
		if(no_go == 0)
		{
			strcpy(used_extension[extensions_used], buf);
			extensions_used++;
			sprintf(filename, "test_vid.%s", buf);
			for(video = 0;((video < mf->video_codec_cnt) && (local_sudden_stop == 0));video++)
			{
				if(in_win != NULL)
				{
					local_sudden_stop = in_win->mux_test_sudden_stop;
				}
				int video_id = mf->video_id[video];
				if(video_id > 0)
				{
					char video_result[4096];
					int	nn1 = my_find_codec_by_id(0, video_id, video_result);
					for(audio = 0;((audio < mf->audio_codec_cnt) && (local_sudden_stop == 0));audio++)
					{
						if(in_win != NULL)
						{
							local_sudden_stop = in_win->mux_test_sudden_stop;
						}
						int audio_id = mf->audio_id[audio];
						if(audio_id > 0)
						{
							Muxer *mux = new Muxer(NULL, NULL, 0);
							char out_buf[32768];
							char audio_result[4096];
							int	nn2 = my_find_codec_by_id(1, audio_id, audio_result);
							int err = mux->TestMux(mf->name, (AVCodecID)video_id, (AVCodecID)audio_id, filename, test_w, test_h, test_fps, test_hz);
							delete mux;
							if(err != 0)
							{
								if(output_cb != NULL)
								{
									sprintf(out_buf, "FAILED: [%s][%s]\n", video_result, audio_result);
									output_cb(out_buf);
									for(inner = 0;inner < global_log_cnt;inner++)
									{
										sprintf(out_buf, "\t%s", global_log[inner]);
										output_cb(out_buf);
									}
								}
								if((err & 1) == 1)
								{
									mf->video_id[video] = 0;
								}
								if((err & 2) == 2)
								{
									mf->audio_id[audio] = 0;
								}
							}
							else
							{
								if(output_cb != NULL)
								{
									sprintf(out_buf, "SUCCESS: [%s][%s][%s]\n", mf->name, video_result, audio_result);
									output_cb(out_buf);
									if(in_win != NULL)
									{
										fprintf(fp, "%s:%s:%d %s:%d %s\n", mf->name, filename, video_id, video_result, audio_id, audio_result);
									}
								}
							}
							global_log_cnt = 0;
						}
					}
				}
			}
		}
	}
	int go_on = 1;
	if(in_win != NULL)
	{
		fclose(fp);
		go_on = 0;
		if(in_win->mux_test_sudden_stop == 0)
		{
			go_on = 1;
		}
	}
	if(go_on == 1)
	{
		for(loop = 0;loop < global_my_format_cnt;loop++)
		{
			int v_cnt = 0;
			int a_cnt = 0;
			MyFormat *mf = global_my_format[loop];
			mf->invalid = 0;
			for(video = 0;video < mf->video_codec_cnt;video++)
			{
				if(mf->video_id[video] != 0)
				{
					v_cnt++;
				}
			}
			for(audio = 0;audio < mf->audio_codec_cnt;audio++)
			{
				if(mf->audio_id[audio] != 0)
				{
					a_cnt++;
				}
			}
			if((v_cnt == 0) || (a_cnt == 0))
			{
				mf->invalid = 1;
			}
		}
	}
	if(in_win != NULL)
	{
		in_win->mux_test_sudden_stop++;
	}
}

int	Muxer::TestMux(char *container, enum AVCodecID video_codec_id, enum AVCodecID audio_codec_id, char *output_filename, int in_width, int in_height, double in_fps, double in_rate)
{
const AVOutputFormat *fmt;
const char *filename;
const AVCodec *audio_codec, *video_codec;
int ret;
AVDictionary *opt = NULL;
int i;

	int using_video = 0;
	int using_audio = 0;
	filename = output_filename;
	double used_rate = 44100;
	AVOutputFormat *found_container = NULL;
	if(container != NULL)
	{
		found_container = find_output_format_by_name(container);
	}
	avformat_alloc_output_context2(&oc, found_container, NULL, filename);
	if(!oc)
	{
		return(1);
	}
	if(in_fps < 0.01) 
	{
		in_fps = 0.01;
	}
	add_stream(0, &video_st, oc, &video_codec, video_codec_id, in_width, in_height, in_fps, 0.0);
	add_stream(0, &audio_st, oc, &audio_codec, audio_codec_id, 0, 0, 0, used_rate);
	int video_err = open_video(oc, video_codec, &video_st, opt);
	int audio_err = open_audio(oc, audio_codec, &audio_st, opt);
	int r = 0;
	if(video_err != 0)
	{
		r = 1;
	}
	if(audio_err != 0)
	{
		r |= 2;
	}
	close_stream(oc, &video_st);
	close_stream(oc, &audio_st);
	avformat_free_context(oc);
	oc = NULL;
	return(r);
}

int	Muxer::InitMux(int use_audio, char *in_container, enum AVCodecID video_codec_id, enum AVCodecID audio_codec_id, char *video_in, char *audio_in, char *output_filename, char *in_url, char *in_desktop_mon, PulseMixer *in_mixer, int audio_device, int in_width, int in_height, double in_fps, double in_rate, int in_channels, int in_frame_cnt, int *in_crop_x, int *in_crop_y)
{
const AVOutputFormat *fmt;
const char *filename;
const AVCodec *audio_codec, *video_codec;
int ret;
AVDictionary *opt = NULL;
int i;

printf("INIT USE_AUDIO: %d\n", use_audio);
	int using_video = 0;
	int using_audio = 0;
	current_frame = 0;
	filename = output_filename;
	used_rate = in_rate;
	used_channels = in_channels;
	crop_x = in_crop_x;
	crop_y = in_crop_y;
	local_frame_cnt = in_frame_cnt;
	url = in_url;
	desktop_monitor = in_desktop_mon;
	mixer = in_mixer;
	not_audible = 0;
	original_width = in_width;
	original_height = in_height;
	if(use_audio == 2)
	{
		use_audio = 1;
		not_audible = 1;
	}
	if(raw == 0)
	{
		if(use_audio == 1)
		{
			used_rate = Open(audio_device, in_rate, in_channels);
			using_video = 1;
			if(mixer != NULL)
			{
				using_audio = 1;
			}
		}
	}
	else
	{
		raw_audio_fd = -1;
		if(audio_in != NULL)
		{
			raw_audio_fd = open(audio_in, O_RDONLY);
			if(raw_audio_fd > -1)
			{
				int hz = 0;
				int in_channels = 0;
				read(raw_audio_fd, &in_channels, sizeof(int));
				read(raw_audio_fd, &hz, sizeof(int));
				used_rate = (double)hz;
				used_channels = in_channels;
				using_audio = 1;
			}
		}
		raw_video_fd = open(video_in, O_RDONLY);
		if(raw_video_fd > -1)
		{
			int uw = 0;
			int uh = 0;
			int depth = 0;
			int ufps = 0;
			read(raw_video_fd, &uw, sizeof(int));
			read(raw_video_fd, &uh, sizeof(int));
			read(raw_video_fd, &depth, sizeof(int));
			read(raw_video_fd, &ufps, sizeof(int));
			in_width = uw;
			in_height = uh;
			raw_w = uw;
			raw_h = uh;
			raw_frame = malloc(raw_w * raw_h * depth);
			raw_frame_sz = raw_w * raw_h * depth;
			raw_depth = depth;
			using_video = 1;
		}
		raw_done = 0;
	}
	if((used_rate == 0.0) && (in_rate != 0.0))
	{
		if((using_simple_stream == 1) || (mixer != NULL))
		{
			used_rate = 44100.0;
		}
		else
		{
			used_rate = 11025.0;
		}
	}
	AVOutputFormat *found_container = NULL;
	if(in_container != NULL)
	{
		found_container = find_output_format_by_name(in_container);
	}
	// allocate the output media context
	avformat_alloc_output_context2(&oc, found_container, NULL, filename);
	if(!oc) 
	{
		printf("Could not deduce output format from file extension: using FLV.\n");
		avformat_alloc_output_context2(&oc, NULL, "flv", filename);
	}
	if(!oc)
	{
		return(1);
	}
	fmt = oc->oformat;

	// Add the audio and video streams using the default format codecs
	// and initialize the codecs.

	if(using_video == 1)
	{
		if(in_fps < 0.01) 
		{
			in_fps = 0.01;
		}
		enum AVCodecID use_video_codec = fmt->video_codec;
		if(video_codec_id > 0)
		{
			use_video_codec = video_codec_id;
		}
		// COW - Forced AV_CODEC_ID_H264 to use nvidia hardware to encode
		// GPU add_stream(1, &video_st, oc, &video_codec, AV_CODEC_ID_H264, in_width, in_height, in_fps, 0.0);
		// CPU add_stream(0, &video_st, oc, &video_codec, fmt->video_codec, in_width, in_height, in_fps, 0.0);
		// CPU IF GPU FAILS add_stream(0, &video_st, oc, &video_codec, use_video_codec, in_width, in_height, in_fps, 0.0);
		if((crop_x != NULL) && (crop_y != NULL))
		{
			add_stream(1, &video_st, oc, &video_codec, use_video_codec, 640, 480, in_fps, 0.0);
		}
		else
		{
			add_stream(1, &video_st, oc, &video_codec, use_video_codec, in_width, in_height, in_fps, 0.0);
		}
		have_video = 1;
		encode_video = 1;
	}
	if((audio_device != -1) || (using_audio == 1))
	{
		enum AVCodecID use_audio_codec = fmt->audio_codec;
		if(audio_codec_id > 0)
		{
			use_audio_codec = audio_codec_id;
		}
		add_stream(0, &audio_st, oc, &audio_codec, use_audio_codec, 0, 0, 0, used_rate);
		have_audio = 1;
		encode_audio = 1;
	}
	// Now that all the parameters are set, we can open the audio and
	// video codecs and allocate the necessary encode buffers.
	int cow_err = 0;
	if(have_video)
	{
		cow_err = open_video(oc, video_codec, &video_st, opt);
	}
	if((have_audio) && (cow_err == 0))
	{
		cow_err = open_audio(oc, audio_codec, &audio_st, opt);
	}
	// open the output file, if needed
	if(cow_err == 0)
	{
		if(url == NULL)
		{
			av_dump_format(oc, 0, filename, 1);
			ret = avio_open(&oc->pb, filename, AVIO_FLAG_WRITE);
		}
		else
		{
			av_dump_format(oc, 0, url, 1);
			ret = avio_open(&oc->pb, url, AVIO_FLAG_WRITE);
		}
		if(ret < 0) 
		{
			fprintf(stdout, "Error: Could not open '%s': %s\n", filename, av_err2str(ret));
			return(1);
		}
		// Write the stream header, if any.
		ret = avformat_write_header(oc, &opt);
		if(ret < 0) 
		{
			fprintf(stdout, "Error: Error occurred when opening output file: %s\n", av_err2str(ret));
			return(1);
		}
		Record(used_rate);
	}
	else
	{
		fprintf(stdout, "Error: Error occurred when opening either audio or video\n");
		return(1);
	}
	return(0);
}

void	Muxer::FinishMux()
{
	Stop();
	av_write_trailer(oc);
	if(have_video)
	{
		close_stream(oc, &video_st);
	}
	if(have_audio)
	{
		close_stream(oc, &audio_st);
	}
	avio_closep(&oc->pb);
	avformat_free_context(oc);
	oc = NULL;
}

int	 simple_record(int *flag)
{
	Muxer *mux = (Muxer *)flag;
	mux->in_simple_record = 1;
	if(mux->stop_activity == 0)
	{
		int nn = FRAMES_PER_BUFFER * sizeof(SAMPLE) * NUM_CHANNELS;
		int error;
		if(mux->mixer == NULL)
		{
			int n = pa_simple_read(mux->simple_pulse_stream, mux->recordedSamples, nn, &error);
		}
		else
		{
			while(mux->mixer->mix_ready != 1)
			{
				usleep(1000);
			}
			if(mux->stop_activity == 0)
			{
				SAMPLE *p1 = mux->recordedSamples;
				SAMPLE *p2 = mux->mixer->buffer;
				if((p1 != NULL) && (p2 != NULL))
				{
					for(int loop = 0;loop < mux->mixer->size;loop++)
					{
						SAMPLE one = *p2;
						*p1 = one;
						p1++;
						p2++;
					}
				}
			}
		}
		if(mux->mute == 1)
		{
			memset(mux->recordedSamples, 0, nn);
		}
		if(mux->stop_activity == 0)
		{
			mux->EncodeAudioAndVideo((void *)mux->recordedSamples);
			mux->audio_samples += FRAMES_PER_BUFFER;
		}
	}
	mux->in_simple_record = 0;
	return(0);
}

double	Muxer::Open(int audio_dev, double requested_rate, int requested_channels)
{
static const pa_sample_spec pulse_ss = {
		.format = PA_SAMPLE_S16LE,
		.rate = (uint32_t)requested_rate,
		.channels = (uint8_t)requested_channels
};

	double rate = 0.0;
	stop_activity = 0;
	no_audio = 0;
	if(mixer != NULL)
	{
		int nn = FRAMES_PER_BUFFER * sizeof(SAMPLE) * 2;
		recordedSamples = (SAMPLE *)malloc(nn);
		rate = requested_rate;
	}
	else
	{
		if(audio_dev < 0)
		{
			if(audio_dev == -2)
			{
				if(desktop_monitor == NULL)
				{
					rate = requested_rate;
				}
				if(desktop_monitor != NULL)
				{
					int error = 0;
					simple_pulse_stream = pa_simple_new(NULL, "dvptz", PA_STREAM_RECORD, desktop_monitor, "record", &pulse_ss, NULL, NULL, &error);
					if(simple_pulse_stream != NULL)
					{
						using_simple_stream = 1;
						int nn = FRAMES_PER_BUFFER * sizeof(SAMPLE) * 2;
						recordedSamples = (SAMPLE *)malloc(nn);
					}
				}
			}
			else
			{
				audio_dev = 0;
				no_audio = 1;
			}
		}
	}
	return(rate);
}

int	 mux_record(int *flag)
{
int	loop;

	Muxer *mux = (Muxer *)flag;
	if(mux != NULL)
	{
		mux->stop_activity = 0;
		while(mux->stop_activity == 0)
		{
			mux->EncodeAudioAndVideo(NULL);
		}
		mux->recording = 0;
	}
	return(0);
}

void	Muxer::Record(double rate)
{
	if(raw == 0)
	{
		paused = 0;
		start_time = time(0);
		if(no_audio == 1)
		{
			recording = 1;
			pthread_t signal_thread = create_task((int (*)(int *))mux_record, (void *)this);
		}
		else
		{
			if(recordedSamples != NULL)
			{
				recording = 1;
			}
		}
	}
	else
	{
		while(raw_done == 0)
		{
			if(encode_audio)
			{
				double audio_f = (double)audio_st.next_pts * ((double)audio_st.enc->time_base.num / (double)audio_st.enc->time_base.den);
				if(frame_timecode < audio_f)
				{
					encode_audio = !write_audio_frame(oc, &audio_st, NULL);
					encode_video = !write_video_frame(oc, &video_st);
					my_window->encoding++;
				}
				else
				{
					encode_audio = !write_audio_frame(oc, &audio_st, NULL);
				}
			}
			else
			{
				encode_video = !write_video_frame(oc, &video_st);
				my_window->encoding++;
			}
			my_window->redraw();
			Fl::check();
		}
		FinishMux();
	}
}

void	Muxer::Pause()
{
	paused = 1;
	stop_activity = 1;
}

void	Muxer::Resume()
{
	if(paused == 1)
	{
		paused = 0;
		stop_activity = 0;
		recording = 1;
		if(no_audio == 1)
		{
			pthread_t signal_thread = create_task((int (*)(int *))mux_record, (void *)this);
		}
	}
}

void	Muxer::Stop()
{
	if(raw == 0)
	{
		stop_activity = 1;
		usleep(100000);
		if(no_audio == 0)
		{
			if(mixer != NULL)
			{
				while(mixer->recording == 1)
				{
					usleep(10000);
				}
			}
			if(recordedSamples != NULL)
			{
				free(recordedSamples);
				recordedSamples = NULL;
			}
		}
		else
		{
			while(recording == 1)
			{
				usleep(10000);
			}
		}
		int nn = (int)(time(0) - start_time);
		if(nn > 0)
		{
			double samples_per_second = (double)audio_samples / (double)nn;
			double frames_per_second = (double)video_frames / (double)nn;
		}
		if(simple_pulse_stream != NULL)
		{
			pa_simple_free(simple_pulse_stream);
			simple_pulse_stream = NULL;
		}
		if(mixer != NULL)
		{
			mixer->mix_ready = 0;
			recording = 0;
		}
	}
	else
	{
		if(raw_audio_fd > -1)
		{
			close(raw_audio_fd);
		}
		if(raw_video_fd > -1)
		{
			close(raw_video_fd);
		}
	}
}

void	*Muxer::GetFrame()
{
	void *ptr = NULL;
	if(raw == 0)
	{
		while((frame_ptr == NULL) && (stop_activity == 0))
		{
			pthread_mutex_unlock(&my_window->mux_mutex);
			usleep(1000);
			pthread_mutex_lock(&my_window->mux_mutex);
		}
		if(frame_ptr != NULL)
		{
			if(no_audio == 1)
			{
				int cnt = 0;
				while((fresh_image == 0) && (cnt < 1000) && (stop_activity == 0))
				{
					usleep(100);
					cnt++;
				}
			}
			ptr = frame_ptr;
			if(fresh_image > 0)
			{
				fresh_image--;
			}
			video_frames++;
		}
	}
	else
	{
		static Mat image2;
		long int ts = 0;
		int flag = 1;
		if(flag == 1)
		{
			int nn = read(raw_video_fd, &ts, sizeof(long int));
			if(nn == sizeof(long int))
			{
				frame_timecode = (double)((double)ts / 1000.0);
				int nn = read(raw_video_fd, raw_frame, raw_frame_sz);
				if(nn == raw_frame_sz)
				{
					if(raw_depth == 3)
					{
						cv::Mat image(cv::Size(raw_w, raw_h), CV_8UC3, raw_frame);
						cvtColor(image, image, COLOR_RGB2BGR);
						if((crop_x != NULL) && (crop_y != NULL))
						{
							int cnt = video_frames;
							if(cnt >= local_frame_cnt)
							{
								cnt = local_frame_cnt - 1;
							}
							cv::Rect roi;
							roi.x = crop_x[cnt];
							roi.y = crop_y[cnt];
							roi.width = 640;
							roi.height = 480;
							Mat crop = image(roi);
							image = crop.clone();
						}
						cvtColor(image, image2, COLOR_BGR2YUV_I420);
					}
					else if(raw_depth == 4)
					{
						cv::Mat image(cv::Size(raw_w, raw_h), CV_8UC4, raw_frame);
						if((crop_x != NULL) && (crop_y != NULL))
						{
							int cnt = video_frames;
							if(cnt >= local_frame_cnt)
							{
								cnt = local_frame_cnt - 1;
							}
							cv::Rect roi;
							roi.x = crop_x[cnt];
							roi.y = crop_y[cnt];
							roi.width = 640;
							roi.height = 480;
							Mat crop = image(roi);
							image = crop.clone();
						}
						cvtColor(image, image2, COLOR_BGRA2YUV_I420);
					}
					ptr = image2.ptr();
					video_frames++;
				}
				else
				{
					raw_done = 1;
				}
			}
			else
			{
				raw_done = 1;
			}
		}
	}
	return(ptr);
}

MyFormat::MyFormat(char *in_name, char *in_extensions, AVOutputFormat *ofmt)
{
	invalid = 0;
	output_format = ofmt;
	strcpy(name, "");
	if(in_name != NULL)
	{
		strncpy(name, in_name, 1024);
	}
	strcpy(extensions, "");
	if(in_extensions != NULL)
	{
		strncpy(extensions, in_extensions, 1024);
	}
	int loop;
	for(loop = 0;loop < 1024;loop++)
	{
		video_codec[loop] = NULL;
		video_id[loop] = 0;
		audio_codec[loop] = NULL;
		audio_id[loop] = 0;
	}
	video_codec_cnt = 0;
	audio_codec_cnt = 0;
}

MyFormat::~MyFormat()
{
	int loop;
	for(loop = 0;loop < 1024;loop++)
	{
		if(video_codec[loop] != NULL)
		{
			free(video_codec[loop]);
		}
		if(audio_codec[loop] != NULL)
		{
			free(audio_codec[loop]);
		}
	}
}

void	MyFormat::AddAudio(char *in_name, int id)
{
	if((in_name != NULL) && (video_codec_cnt < 1024))
	{
		int no_go = 0;
		for(int loop = 0;loop < audio_codec_cnt;loop++)
		{
			if(strcmp(in_name, audio_codec[loop]) == 0)
			{
				no_go = 1;
			}
		}
		if(no_go == 0)
		{
			enum AVCodecID	codec_by_name(char *name);
			AVCodecID tst = codec_by_name(in_name);
			if(tst != 0)
			{
				audio_codec[audio_codec_cnt] = strdup(in_name);
				audio_id[audio_codec_cnt] = id;
				audio_codec_cnt++;
			}
			else
			{
				AVCodec *tst = (AVCodec *)avcodec_find_encoder((AVCodecID)id);
				if(tst != NULL)
				{
					audio_codec[audio_codec_cnt] = strdup(in_name);
					audio_id[audio_codec_cnt] = id;
					audio_codec_cnt++;
				}
			}
		}
	}
}

void	MyFormat::AddVideo(char *in_name, int id)
{
AVCodecID codec_by_name(char *name);

	if((in_name != NULL) && (video_codec_cnt < 1024))
	{
		int no_go = 0;
		for(int loop = 0;loop < video_codec_cnt;loop++)
		{
			if(strcmp(in_name, video_codec[loop]) == 0)
			{
				no_go = 1;
			}
		}
		if(no_go == 0)
		{
			AVCodecID tst = codec_by_name(in_name);
			if(tst != 0)
			{
				video_codec[video_codec_cnt] = strdup(in_name);
				video_id[video_codec_cnt] = id;
				video_codec_cnt++;
			}
			else
			{
				AVCodec *tst = (AVCodec *)avcodec_find_encoder((AVCodecID)id);
				if(tst != NULL)
				{
					video_codec[video_codec_cnt] = strdup(in_name);
					video_id[video_codec_cnt] = id;
					video_codec_cnt++;
				}
			}
		}
	}
}

int	my_find_codec_by_id(int type, int in_id, char *result)
{
int	loop;
int	video;
int	audio;

	int done = 0;
	for(loop = 0;((loop < global_my_format_cnt) && (done == 0));loop++)
	{
		MyFormat *mf = global_my_format[loop];
		if(type == 0)
		{
			for(video = 0;((video < mf->video_codec_cnt) && (done == 0));video++)
			{
				if(mf->video_codec[video] != NULL)
				{
					int id = mf->video_id[video];
					if(id == in_id)
					{
						strcpy(result, mf->video_codec[video]);
						done = 1;
					}
				}
			}
		}
		else if(type == 1)
		{
			for(audio = 0;((audio < mf->audio_codec_cnt) && (done == 0));audio++)
			{
				if(mf->audio_codec[audio] != NULL)
				{
					int id = mf->audio_id[audio];
					if(id == in_id)
					{
						strcpy(result, mf->audio_codec[audio]);
						done = 1;
					}
				}
			}
		}
	}
	return(done);
}

int	my_find_codec_by_name(int type, char *format_name, char *in_name)
{
int	loop;
int	video;
int	audio;

	int id = 0;
	if((format_name != NULL) && (in_name != NULL))
	{
		for(loop = 0;((loop < global_my_format_cnt) && (id == 0));loop++)
		{
			MyFormat *mf = global_my_format[loop];
			if(strcmp(mf->name, format_name) == 0)
			{
				if(type == 0)
				{
					for(video = 0;((video < mf->video_codec_cnt) && (id == 0));video++)
					{
						if(mf->video_codec[video] != NULL)
						{
							if(strcmp(mf->video_codec[video], in_name) == 0)
							{
								id = mf->video_id[video];
							}
						}
					}
				}
				else if(type == 1)
				{
					for(audio = 0;((audio < mf->audio_codec_cnt) && (id == 0));audio++)
					{
						if(mf->audio_codec[audio] != NULL)
						{
							if(strcmp(mf->audio_codec[audio], in_name) == 0)
							{
								id = mf->audio_id[audio];
							}
						}
					}
				}
			}
		}
	}
	return(id);
}
