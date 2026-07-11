typedef short SAMPLE;

#define	MODE_RECORD	0
#define	MODE_PLAY	1
#define	LOW_PASS_BAND	1
#define	HIGH_PASS_BAND	2

class	PulseAudio
{
public:
				PulseAudio(char *device, int direction, int in_number_of_samples, int in_hz, int in_number_of_channels, SAMPLE *default_buffer);
				~PulseAudio();

	void		Record();
	void		Play();
	void		Stop();
	void		Pause();
	void		Resume();
	void		Run();
	void		Filter(int band, float frequency);
	void		Reverb(double delay, double decay);
	void		Compress(double low_cutoff, double high_cutoff, double percent);

	int			SampleAudioFile();

	int			failure;
	int			stop;
	int			stopped;
	int			pause;
	int			mini_pause;
	int			mute;
	int			mode;
	int			shutdown;
	SAMPLE		*buffer;
	int			number_of_samples;
	int			number_of_channels;
	int			sample_rate;
	int			ch;
	int			sample_count;
	void		(*sample_ready_cb)(void *in_win, PulseAudio *);
	pa_simple	*stream;
	int			hz;
	int			buffer_size;
	double		volume1;
	double		volume2;
	char		*device;
	double		average;
	SAMPLE		local_peak;
	int			is_microphone;
	void		*p_mp3;
	void		*p_wav;
	void		*p_flac;
	int			repeating;
	int			low_pass;
	float		low_pass_frequency;
	int			high_pass;
	float		high_pass_frequency;
	int			reverb;
	double		reverb_delay;
	double		reverb_decay;
	int			compress;
	double		compress_high;
	double		compress_low;
	double		compress_percent;
	int			ndi_capture;
	char		ndi_path[256];
};

