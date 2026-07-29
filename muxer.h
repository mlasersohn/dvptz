#define NUM_CHANNELS		(1)

#define PA_SAMPLE_TYPE  paInt16
typedef short SAMPLE;
#define SAMPLE_SILENCE  (0)
#define PRINTF_S_FORMAT "%d"

#define STREAM_PIX_FMT		AV_PIX_FMT_YUV420P
#define SCALE_FLAGS		SWS_BICUBIC

#define	CHECK_TYPE_AUDIO		0
#define	CHECK_TYPE_VIDEO		1
#define	CHECK_TYPE_CONTAINER	2

// COW - FORCE THIS TO MATCH MIXER AND MIC
#define FRAMES_PER_BUFFER   (1024)

#ifdef av_ts2str
#undef av_ts2str
av_always_inline char *av_ts2str(int ts)
{
    thread_local char str[AV_TS_MAX_STRING_SIZE]; 
    memset(str, 0, sizeof(str));
    return av_ts_make_string(str, ts);
}
#endif

#ifdef av_err2str
#undef av_err2str
av_always_inline char* av_err2str(int errnum)
{
    thread_local char str[AV_ERROR_MAX_STRING_SIZE]; 
    memset(str, 0, sizeof(str));
    return av_make_error_string(str, AV_ERROR_MAX_STRING_SIZE, errnum);
}
#endif

// ---------------------------------------------------------------------
// Audio channel-layout compatibility shim
//
// FFmpeg 5.1 (libavutil 57.24.100) replaced the old uint64_t bitmask
// channel API - AVCodecContext::channels / channel_layout,
// AVFrame::channels / channel_layout, AVCodec::channel_layouts, and the
// AV_CH_LAYOUT_* macros - with the new AVChannelLayout struct based API:
// AVCodecContext::ch_layout, AVFrame::ch_layout, AVCodec::ch_layouts, and
// AV_CHANNEL_LAYOUT_* macros. The old API is fully removed in later
// releases, so code that only used the old fields will not build against
// current headers. Everything below lets the rest of this file use plain
// "mono/stereo/nb_channels" calls that work against either header set.
#if defined(LIBAVUTIL_VERSION_INT) && LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57, 24, 100)
#define MUXER_HAS_NEW_CHANNEL_LAYOUT 1
#else
#define MUXER_HAS_NEW_CHANNEL_LAYOUT 0
#endif

// Pick mono/stereo (honoring what the codec actually supports) and set it
// on an AVCodecContext. Mirrors the old add_stream() logic exactly, just
// routed through whichever API is available.
static inline void mux_configure_codec_channels(AVCodecContext *c, const AVCodec *codec, int nb_channels)
{
#if MUXER_HAS_NEW_CHANNEL_LAYOUT
	AVChannelLayout want = (nb_channels == 1) ? (AVChannelLayout)AV_CHANNEL_LAYOUT_MONO
	                                           : (AVChannelLayout)AV_CHANNEL_LAYOUT_STEREO;
	av_channel_layout_copy(&c->ch_layout, &want);
	if(codec->ch_layouts)
	{
		int i;
		av_channel_layout_copy(&c->ch_layout, &codec->ch_layouts[0]);
		for(i = 0; codec->ch_layouts[i].nb_channels != 0; i++)
		{
			AVChannelLayout stereo = AV_CHANNEL_LAYOUT_STEREO;
			if(av_channel_layout_compare(&codec->ch_layouts[i], &stereo) == 0)
			{
				av_channel_layout_copy(&c->ch_layout, &stereo);
			}
		}
	}
#else
	c->channels = av_get_channel_layout_nb_channels(c->channel_layout);
	c->channel_layout = (nb_channels == 1) ? AV_CH_LAYOUT_MONO : AV_CH_LAYOUT_STEREO;
	if(codec->channel_layouts)
	{
		int i;
		c->channel_layout = codec->channel_layouts[0];
		for(i = 0; codec->channel_layouts[i]; i++)
		{
			if(codec->channel_layouts[i] == AV_CH_LAYOUT_STEREO)
			{
				c->channel_layout = AV_CH_LAYOUT_STEREO;
			}
		}
	}
	c->channels = av_get_channel_layout_nb_channels(c->channel_layout);
#endif
}

// Number of channels currently configured on an AVCodecContext.
static inline int mux_codec_channel_count(AVCodecContext *c)
{
#if MUXER_HAS_NEW_CHANNEL_LAYOUT
	return c->ch_layout.nb_channels;
#else
	return c->channels;
#endif
}

// Set a plain mono/stereo layout directly on an AVFrame.
static inline void mux_set_frame_channel_layout(AVFrame *frame, int nb_channels)
{
#if MUXER_HAS_NEW_CHANNEL_LAYOUT
	AVChannelLayout want = (nb_channels == 1) ? (AVChannelLayout)AV_CHANNEL_LAYOUT_MONO
	                                           : (AVChannelLayout)AV_CHANNEL_LAYOUT_STEREO;
	av_channel_layout_copy(&frame->ch_layout, &want);
#else
	frame->channel_layout = (nb_channels == 1) ? AV_CH_LAYOUT_MONO : AV_CH_LAYOUT_STEREO;
#endif
}

// Configure a freshly-allocated SwrContext's in/out channel counts from a
// codec context (both sides use the same channel count in this file).
static inline void mux_swr_set_channels(struct SwrContext *swr, AVCodecContext *c)
{
#if MUXER_HAS_NEW_CHANNEL_LAYOUT
	av_opt_set_chlayout(swr, "in_chlayout",  &c->ch_layout, 0);
	av_opt_set_chlayout(swr, "out_chlayout", &c->ch_layout, 0);
#else
	av_opt_set_int(swr, "in_channel_count",  c->channels, 0);
	av_opt_set_int(swr, "out_channel_count", c->channels, 0);
#endif
}
// ---------------------------------------------------------------------

class	MyWin;
class	PulseMixer;

class	MyFormat
{
public:
		MyFormat(char *in_name, char *in_extensions, AVOutputFormat *ofmt);
		~MyFormat();
		void	AddAudio(char *in_name, int id);
		void	AddVideo(char *in_name, int id);

	AVOutputFormat	*output_format;
	int		invalid;
	char	name[1024];
	char	extensions[1024];
	char	*video_codec[1024];
	int		video_id[1024];
	int		video_codec_cnt;
	char	*audio_codec[1024];
	int		audio_id[1024];
	int		audio_codec_cnt;
};

// a wrapper around a single output AVStream
typedef struct OutputStream
{
        AVStream *st;
        AVCodecContext *enc;

        // pts of the next frame that will be generated
        int64_t next_pts;
        int samples_count;

        AVFrame *frame;
        AVFrame *tmp_frame;

        float t, tincr, tincr2;

        struct SwsContext *sws_ctx;
        struct SwrContext *swr_ctx;
} OutputStream;

class	Muxer
{
public:
		Muxer(MyWin *, Camera *, ReviewWin *, int);
		~Muxer();

	double	Open(int audio_dev, double rate, int channels);
	void	*GetFrame();
	void	Stop();
	void	Pause();
	void	Resume();
	void	Record(double);

	int write_frame(AVFormatContext *fmt_ctx, AVCodecContext *c, AVStream *st, AVFrame *frame);
	void add_stream(int use_nvidia, OutputStream *ost, AVFormatContext *oc, const AVCodec **codec, enum AVCodecID codec_id, int in_width, int in_height, double in_fps, double in_hz);
	AVFrame *alloc_audio_frame(enum AVSampleFormat sample_fmt, int nb_channels, int sample_rate, int nb_samples);
	int open_audio(AVFormatContext *oc, const AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg);
	AVFrame *get_audio_frame(OutputStream *ost, void *in_buffer);
	int write_audio_frame(AVFormatContext *oc, OutputStream *ost, void *in_buffer);
	AVFrame *alloc_picture(enum AVPixelFormat pix_fmt, int width, int height);
	int open_video(AVFormatContext *oc, const AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg);
	void fill_yuv_image(AVFrame *pict, int frame_index, int width, int height);
	AVFrame *get_video_frame(OutputStream *ost);
	int write_video_frame(AVFormatContext *oc, OutputStream *ost);
	void close_stream(AVFormatContext *oc, OutputStream *ost);
	void EncodeAudioAndVideo(void *in_buffer);
	int TestMux(char *in_container, enum AVCodecID video_codec_id, enum AVCodecID audio_codec_id, char *output_filename, int in_width, int in_height, double in_fps, double in_rate);
	void	Flush();

	int InitMux(int encode_audio, char *container, enum AVCodecID video_codec_id, enum AVCodecID audio_codec_id, char *video_in, char *audio_in, char *output_filename, char *url, char *desk_mon, PulseMixer *in_mixer, int audio_device, int in_width, int in_height, double in_fps, double in_rate, int in_channels, int in_frame_cnt, int *crop_x, int *crop_y);
	void FinishMux();

	MyWin	*my_window;
	PulseMixer	*mixer;
	ReviewWin	*review;
	char		filename[4096];
	Camera		*camera;
	int	number_of_audio_samples;
	int	encode_audio;
	int	encode_video;
	int	current_frame;
	int	realtime_factor;
	OutputStream	video_st;
	OutputStream	audio_st;
	AVFormatContext	*oc;
	int		have_video;
	int		have_audio;
	int		stop_activity;
	int		no_audio;
	SAMPLE		*recordedSamples;
	int		number_of_samples;
	void		*frame_ptr;
	int		recording;
	int		video_frames;
	int		audio_samples;
	int		start_time;
	int		paused;
	double		frame_timecode;
	char		*url;
	int		mute;
	int		not_audible;
	int		used_channels;
	int		used_rate;

	int		raw;
	int		raw_video_fd;
	void		*raw_frame;
	int		raw_frame_sz;
	int		raw_audio_fd;
	int		raw_done;
	int		raw_w;
	int		raw_h;
	int		raw_depth;
	int		fresh_image;

	int		*crop_x;
	int		*crop_y;
	int		local_frame_cnt;

	int		using_simple_stream;
	pa_simple	*simple_pulse_stream;
	char		*desktop_monitor;

	int			original_width;
	int			original_height;

	time_t		time_to_write_frame;
	double		average_time_to_write_frame;
	int			in_simple_record;
};
