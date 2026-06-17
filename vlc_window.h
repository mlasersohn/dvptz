#define	BUFFER_LIMIT	1024

void cbAudioPrerender(void* p_audio_data, uint8_t** pp_pcm_buffer , unsigned int size);
void cbAudioPostrender(void *p_audio_data, uint8_t *p_pcm_buffer, unsigned int channels, unsigned int rate, unsigned int nb_samples, unsigned int bits_per_sample, unsigned int size, int64_t pts);
void cbVideoPrerender(void *p_video_data, uint8_t **pp_pixel_buffer, int size);
void cbVideoPostrender(void *p_video_data, uint8_t *p_pixel_buffer, int width, int height, int pixel_pitch, int size, int64_t pts);
static void handleEvent(const libvlc_event_t* pEvt, void* pUserData);

class	VLC_Window
{
public:
		VLC_Window(char *in_path, int ww, int hh);
		~VLC_Window();

	void	Draw();
	void	PlayAudio();
	void	ResetFrames();
	void	Start();
	void	Stop();
	void	Pause();
	void	Resume();
	void	Volume(double amount);
	void	Mute();
	void	UnMute();
	double	Position();
	void	Position(double pos);
	void	QuickPosition(double new_position);
	void	Despool();

	pa_simple	*simple;
	Mat		mat;

	char		path[4096];
	int		current_frame;
	int		width;
	int		height;
	int 		done;
	int		paused;
	double		scale;
	double		volume;
	int		muted;
	double		stored_volume;
	libvlc_media_t *media;
	libvlc_media_player_t	*media_player;
	uint8_t			*videoBuffer;
	uint8_t			*audioBuffer;
	unsigned int		videoBufferSize;
	unsigned int		audioBufferSize;
	int			audio_tracks;

	unsigned char	*video_frame[BUFFER_LIMIT];
	long int 	video_pts[BUFFER_LIMIT];
	int		video_cnt;
	int		video_play_cnt;
	int		video_sz;
	long int	last_video_pts;

	unsigned char	*audio_frame[BUFFER_LIMIT];
	long int 	audio_pts[BUFFER_LIMIT];
	int 		audio_cnt;
	int		audio_play_cnt;
	long int	last_audio_pts;
	int		audio_sz;
	long int	old_pts;
	int		audio_complete;

	sem_t	semaphore;
};

