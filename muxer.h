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
	AVFrame *alloc_audio_frame(enum AVSampleFormat sample_fmt, uint64_t channel_layout, int sample_rate, int nb_samples);
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
