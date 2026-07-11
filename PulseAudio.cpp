#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>

#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"
#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"
#define DR_FLAC_IMPLEMENTATION
#include "dr_flac.h"

#include <pulse/pulseaudio.h>
#include <pulse/error.h>
#include <pulse/simple.h>

#include "PulseAudio.h"

#include <FL/filename.H>

int		create_task(int (*funct)(int *), void *flag);
int		pulse_record_all(int *flag);
int		pulse_play_all(int *flag);
long int	precise_time(void);

extern "C" {
int cow_simple_read(pa_simple *p, void *data, size_t length, int *rerror);
}

// Structure to hold the state for one or more channels
struct LPFState 
{
	float last_out_l = 0.0f;
	float last_out_r = 0.0f; // Added for stereo support
	float alpha = 0.1f;
} lpf_state;

SAMPLE	*extract_audio_file_samples(char *filename, int& number_of_samples, int &channels, int& sample_rate)
{
void	*p_mp3 = NULL;
void	*p_wav = NULL;
void	*p_flac = NULL;
int		loop;

	SAMPLE *total_buf = NULL;
	int sample_cnt = 0;
	int failure = 1;
	static drmp3 mp3;
	static drwav wav;
	const char *extension = fl_filename_ext((const char *)filename);
	if(extension != NULL)
	{
		if(strcmp(extension, ".mp3") == 0)
		{
			if(drmp3_init_file(&mp3, filename, NULL))
			{
				p_mp3 = &mp3;
				failure = 0;
			}
		}
		else if(strcmp(extension, ".wav") == 0)
		{
			if(drwav_init_file(&wav, filename, NULL))
			{
				p_wav = &wav;
				failure = 0;
			}
		}
		else if(strcmp(extension, ".flac") == 0)
		{
			p_flac = drflac_open_file(filename, NULL);
			if(p_flac != NULL)
			{
				failure = 0;
			}
		}
	}
	if(failure == 0)
	{
		channels = 0;
		sample_rate = 0;
		if(p_mp3 != NULL)
		{
			drmp3_seek_to_pcm_frame((drmp3 *)p_mp3, 0);
			drmp3 *mp3 = (drmp3 *)p_mp3;
			channels = mp3->channels;
			sample_rate = mp3->sampleRate;
		}
		else if(p_wav != NULL)
		{
			drwav_seek_to_pcm_frame((drwav *)p_wav, 0);
			drwav *wav = (drwav *)p_wav;
			channels = wav->channels;
			sample_rate = wav->sampleRate;
		}
		else if(p_flac != NULL)
		{
			drflac_seek_to_pcm_frame((drflac *)p_flac, 0);
			drflac *flac = (drflac *)p_flac;
			channels = flac->channels;
			sample_rate = flac->sampleRate;
		}
		if(channels != 0)
		{
			SAMPLE buf[(size_t)number_of_samples * (size_t)channels * sizeof(SAMPLE)];
			size_t total_sz = 0;
			int done = 0;
			while(done == 0)
			{
				int n = 0;
				if(p_mp3 != NULL)
				{
					drmp3_uint64 framesRead = drmp3_read_pcm_frames_s16((drmp3 *)p_mp3, number_of_samples, buf);
					n = (int)framesRead;
				}
				else if(p_wav != NULL)
				{
					drwav_uint64 framesRead = drwav_read_pcm_frames_s16((drwav *)p_wav, number_of_samples, buf);
					n = (int)framesRead;
				}
				else if(p_flac != NULL)
				{
					drflac_uint64 framesRead = drflac_read_pcm_frames_s16((drflac *)p_flac, number_of_samples, buf);
					n = (int)framesRead;
				}
				if(n > 0)
				{
					size_t sz_in_bytes = (size_t)n * (size_t)channels * sizeof(SAMPLE);
					total_sz += sz_in_bytes;
					total_buf = (SAMPLE *)realloc(total_buf, total_sz);
					if(total_buf != NULL)
					{
						memcpy(&total_buf[sample_cnt * channels], buf, sz_in_bytes);
						sample_cnt += n;
					}
				}
				else
				{
					done = 1;
				}
			}
			if(p_mp3 != NULL)
			{
				drmp3_uninit((drmp3 *)p_mp3);
			}
			else if(p_wav != NULL)
			{
				drwav_uninit((drwav *)p_wav);
			}
		}
	}
	number_of_samples = sample_cnt;
	return(total_buf);
}

/* COW REMOVED
float	PulseAudo::LowPassFilter(float frequency, float input)
{
static float	prevOutput = 0.0;
static float	prevInput = 0.0;

	float x = tanf(M_PI * frequency / (double)sample_rate);
	float output = x * input + x * prevInput - (x - 1) * prevOutput;

	output /= (x + 1);
	prevOutput = output;
	prevInput = input;
	
	return output;
}
*/

void	PulseAudio::Compress(double in_low_cutoff, double in_high_cutoff, double percent)
{
int	loop;
static int max = -1000000;

	int high_cutoff = (int)(32768.0 * in_high_cutoff);
	int low_cutoff = (int)(32768.0 * in_low_cutoff);
	int cnt = buffer_size / sizeof(SAMPLE);
	for(loop = 0;loop < cnt;loop++)
	{
		int nn = buffer[loop];
		int sign = 1;
		if(nn < 0) sign = -1;
		int ann = abs(nn);
		if(ann > high_cutoff)
		{
			int p = (ann / 100) * percent;
			ann -= p;
		}
		else if(ann < low_cutoff)
		{
			int p = (ann / 100) * percent;
			ann += p;
		}
		if(sign == -1)
		{
			ann *= -1;
		}
		buffer[loop] = ann;
	}
}

void	PulseAudio::Reverb(double delay, double decay)
{
static SAMPLE	reverb[88200];
static int		record_cnt = 0;
static int		play_cnt = 0;
static int		init = 0;
int				loop;

	int cnt = buffer_size / sizeof(SAMPLE);
	int delay_cnt = (int)((double)sample_rate * delay);
	for(loop = 0;loop < cnt;loop++)
	{
		if((record_cnt > delay_cnt) || (init == 1))
		{
			if(init == 1)
			{
				int nn = buffer[loop];
				nn += (int)((double)reverb[play_cnt] * (1.0 - decay));
				nn /= (2.0 - decay);
				buffer[loop] = nn;
				
				play_cnt++;
				if(play_cnt >= 88200)
				{
					play_cnt = 0;
				}
			}
			else
			{
				init = 1;
			}
		}
		reverb[record_cnt] = buffer[loop];
		record_cnt++;
		if(record_cnt >= 88200)
		{
			record_cnt = 0;
		}
	}
}


// Calculates and updates the alpha value based on cutoff and sample rate.
void update_lpf_alpha(LPFState &state, float cutoff_hz, float sample_rate) 
{
	float dt = 1.0f / sample_rate;
	float rc = 1.0f / (2.0f * M_PI * cutoff_hz);
	state.alpha = dt / (rc + dt);
}

// Processes a buffer of 16-bit PCM samples.
// buffer: Pointer to the audio data
// num_samples: Number of individual samples in the buffer
// channels: 1 for mono, 2 for stereo
// state: Persistent filter state
void apply_low_pass_filter(int16_t *buffer, size_t num_samples, int channels, LPFState &state) 
{
	for(size_t i = 0; i < num_samples; i += channels) 
	{
		// Process Left Channel (or Mono)
		float in_l = (float)buffer[i];
		state.last_out_l = state.last_out_l + state.alpha * (in_l - state.last_out_l);
		buffer[i] = (int16_t)state.last_out_l;

		// Process Right Channel if stereo
		if(channels == 2) 
		{
			float in_r = (float)buffer[i + 1];
			state.last_out_r = state.last_out_r + state.alpha * (in_r - state.last_out_r);
			buffer[i + 1] = (int16_t)state.last_out_r;
		}
	}
}

void	PulseAudio::Filter(int band, float frequency)
{
	float use = (frequency * 800.0);
	update_lpf_alpha(lpf_state, use, 44100);
	apply_low_pass_filter(buffer, number_of_samples, ch, lpf_state);
}

/* COW COW
void	PulseAudio::Filter(int band, float frequency)
{
float Kf = frequency;
float FilterAcc = 0.0;  // Filter Accumulator
float LowPassOut = 0.0;  // LowPass Output
float HighPassOut = 0.0;  // HighPass Output
int	loop;

	SAMPLE *in = buffer;
	int cnt = buffer_size / sizeof(SAMPLE);
	for(loop = 0;loop < cnt;loop++)
	{
		float FilterIn = (float)in[loop];
		FilterAcc = FilterAcc + (Kf * (FilterIn - FilterAcc));
		LowPassOut = FilterAcc;
		HighPassOut = FilterIn - FilterAcc;
		if(band == LOW_PASS_BAND)
		{
			in[loop] = (SAMPLE)LowPassOut;
		}
		else if(band == HIGH_PASS_BAND)
		{
			in[loop] = (SAMPLE)HighPassOut;
		}
	}
}
*/

int	pulse_record(int *flag)
{
void	read_pulse_mic(PulseAudio *pa, int any_recording);

	PulseAudio *pa = (PulseAudio *)flag;
	while(pa->stop == 0)
	{
		read_pulse_mic(pa, 1);
	}
	pa->stopped = 1;
	return(0);
}

int	PulseAudio::SampleAudioFile()
{
int	loop;

	int n = 0;
	drmp3 *mp3 = (drmp3 *)p_mp3;
	drwav *wav = (drwav *)p_wav;
	drflac *flac = (drflac *)p_flac;
	int channels = 1;
	if(p_mp3 != NULL)
	{
		channels = mp3->channels;
	}
	else if(p_wav != NULL)
	{
		channels = wav->channels;
	}
	else if(p_flac != NULL)
	{
		channels = flac->channels;
	}
	SAMPLE buf[number_of_samples * channels];
	if(p_mp3 != NULL)
	{
		drmp3_uint64 framesRead = drmp3_read_pcm_frames_s16((drmp3 *)p_mp3, number_of_samples, buf);
		n = (int)framesRead;
	}
	else if(p_wav != NULL)
	{
		drwav_uint64 framesRead = drwav_read_pcm_frames_s16((drwav *)p_wav, number_of_samples, buf);
		n = (int)framesRead;
	}
	else if(p_flac != NULL)
	{
		drflac_uint64 framesRead = drflac_read_pcm_frames_s16((drflac *)p_flac, number_of_samples, buf);
		n = (int)framesRead;
	}
	if(n > 0)
	{
		SAMPLE *cp = (SAMPLE *)buffer;
		for(loop = 0;loop < number_of_samples * channels;loop++)
		{
			int nn = (int)buf[loop];
			*cp = (SAMPLE)nn;
			cp++;
		}
		usleep(18000);
	}
	else
	{
		if(repeating == 1)
		{
			if(p_mp3 != NULL)
			{
				drmp3_seek_to_pcm_frame((drmp3 *)p_mp3, 0);
			}
			else if(p_wav != NULL)
			{
				drwav_seek_to_pcm_frame((drwav *)p_wav, 0);
			}
			else if(p_flac != NULL)
			{
				drflac_seek_to_pcm_frame((drflac *)p_flac, 0);
			}
		}
	}
	return(n);
}

void	read_pulse_mic(PulseAudio *pa, int any_recording)
{
int	loop;

	pa->stopped = 0;
	int error = 0;
	if((pa->shutdown == 0) && (pa->failure == 0))
	{
		if((pa->pause == 0) && (pa->buffer != NULL))
		{
			if(pa->stop == 0)
			{
				if((pa->stream != NULL) || (pa->is_microphone == 0))
				{
					int n = 0;
					if(pa->is_microphone)
					{
						n = cow_simple_read(pa->stream, pa->buffer, pa->buffer_size, &error);
						int nn = pa->buffer_size / sizeof(SAMPLE);
						if(pa->low_pass == 1)
						{
							pa->Filter(LOW_PASS_BAND, pa->low_pass_frequency);
						}
						if(pa->high_pass == 1)
						{
							pa->Filter(HIGH_PASS_BAND, pa->high_pass_frequency);
						}
						if(pa->reverb == 1)
						{
							pa->Reverb(pa->reverb_delay, pa->reverb_decay);
						}
						if(pa->compress == 1)
						{
							pa->Compress(pa->compress_low, pa->compress_high, pa->compress_percent);
						}
					}
					else if(pa->ndi_capture == 1)
					{
						n = pa->buffer_size;
					}
					else
					{
						n = pa->SampleAudioFile();
						if(n == 0)
						{
							pa->stop = 1;
							memset(pa->buffer, 0, pa->buffer_size);
						}
					}
					if(n < 0)
					{
						pa->failure = 1;
					}
					if(pa->mute == 1)
					{
						memset(pa->buffer, 0, pa->buffer_size);
					}
					else
					{
						if(pa->buffer_size > 0)
						{
							int total = 0;
							int nn = pa->buffer_size / sizeof(SAMPLE);
							if(nn > 0)
							{
								pa->local_peak = 0;
								int max = 0;
								for(loop = 0;loop < nn;loop++)
								{
									total += abs(pa->buffer[loop]);
									int tst = abs(pa->buffer[loop]);
									if(tst > max)
									{
										max = tst;
									}
								}
								double avg = (double)(total / nn);
								if(avg > 0.0)
								{
									pa->average = avg;
								}
								pa->local_peak = max;
							}
						}
					}
					if(pa->sample_ready_cb != NULL)
					{
						pa->sample_ready_cb(NULL, pa);
					}
				}
				else
				{
					pa->failure = 1;
				}
			}
			else
			{
				pa->stopped = 1;
			}
		}
		else if(any_recording == 0)
		{
			if(pa->buffer != NULL)
			{
				if((pa->stream != NULL) || (pa->is_microphone == 0))
				{
					int n = 0;
					if(pa->is_microphone)
					{
						n = cow_simple_read(pa->stream, pa->buffer, pa->buffer_size, &error);
					}
					if(n < 0)
					{
						pa->failure = 1;
					}
					if(pa->buffer_size > 0)
					{
						int total = 0;
						int nn = pa->buffer_size / sizeof(SAMPLE);
						if(nn > 0)
						{
							int max = 0;
							pa->local_peak = 0;
							for(loop = 0;loop < nn;loop++)
							{
								total += abs(pa->buffer[loop]);
								int tst = abs(pa->buffer[loop]);
								if(tst > max)
								{
									max = tst;
								}
							}
							double avg = (double)(total / nn);
							if(avg > 0.0)
							{
								pa->average = avg;
							}
							pa->average = pa->buffer[0];
							pa->local_peak = max;
						}
					}
					memset(pa->buffer, 0, pa->buffer_size);
				}
				else
				{
					pa->failure = 1;
				}
			}
		}
		if(pa->stop == 1)
		{
			pa->stopped = 1;
		}
	}
	else
	{
		pa->stopped = 1;
	}
}

int	pulse_play(int *flag)
{
	PulseAudio *pa = (PulseAudio *)flag;
	while(pa->stop == 0)
	{
		pa->stopped = 0;
		if(pa->pause == 0)
		{
			if(pa->sample_ready_cb != NULL)
			{
				pa->sample_ready_cb(NULL, pa);
			}
			if(pa->buffer != NULL)
			{
				if(pa->mute == 1)
				{
					memset(pa->buffer, 0, pa->buffer_size);
				}
				int error = 0;
				int n = pa_simple_write(pa->stream, pa->buffer, pa->buffer_size, &error);
			}
		}
		else
		{
			if(pa->buffer != NULL)
			{
				memset(pa->buffer, 0, pa->buffer_size);
			}
			usleep(10000);
		}
	}
	pa->stopped = 1;
	return(0);
}

PulseAudio::PulseAudio(char *in_device, int in_mode, int in_number_of_samples, int in_hz, int in_number_of_channels, SAMPLE *default_buffer)
{
long int precise_time(void);

	p_mp3 = NULL;
	p_wav = NULL;
	p_flac = NULL;
	ch = in_number_of_channels;
	if(ch < 1) ch = 1;

	static pa_sample_spec pulse_ss =
	{
		.format = PA_SAMPLE_S16LE,
		.rate = (uint32_t)in_hz,
		.channels = (uint8_t)in_number_of_channels
	};
   	static const pa_buffer_attr ba = 
	{
		.maxlength = 8192,
			.tlength = 1,
			.prebuf = 1,
			.minreq = 1,
			.fragsize = 2048
	};
	pulse_ss.rate = (uint32_t)in_hz;
	pulse_ss.channels = (uint8_t)in_number_of_channels;

	device = NULL;
	if(in_device != NULL)
	{
		device = strdup(in_device);
	}
	failure = 0;
	shutdown = 0;
	mute = 0;
	stop = 0;
	mode = in_mode;
	volume1 = 0.0;
	volume2 = 0.0;
	average = 0.0;
	local_peak = 0;
	sample_rate = in_hz;
	number_of_channels = in_number_of_channels;
	number_of_samples = in_number_of_samples;
	buffer_size = number_of_samples * sizeof(SAMPLE) * number_of_channels;
	buffer = default_buffer;
	is_microphone = 0;
	repeating = 0;
	high_pass = 0;
	high_pass_frequency = 0.0;
	low_pass = 0;
	low_pass_frequency = 0.0;
	reverb = 0;
	reverb_delay = 0.25;
	reverb_decay = 0.25;
	compress = 0;
	compress_high = 0.75;
	compress_low = 0.25;
	compress_percent = 0.1;
	ndi_capture = 0;
	strcpy(ndi_path, "");
	stream = NULL;
	mini_pause = 0;
	if(buffer != NULL)
	{
		memset(buffer, 0, buffer_size);
	}
	if(mode == MODE_RECORD)
	{
		int error = 0;
		stream = pa_simple_new(NULL, "dvptz", PA_STREAM_RECORD, device, "record", &pulse_ss, NULL, &ba, &error);
		if(stream != NULL)
		{
			unsigned char local_buffer[32];
			int n = cow_simple_read(stream, local_buffer, 32, &error);
			if(n < 0)
			{
				failure = 1;
			}
			pause = 1;
			stop = 1;
			is_microphone = 1;
		}
		else if(device != NULL)
		{
			failure = 1;
			if(strncmp(device, "ndi://", strlen("ndi://")) == 0)
			{
				char *cp = device + strlen("ndi://");
				if(strlen(cp) > 0)
				{
					strcpy(ndi_path, cp);
					ndi_capture = 1;
					failure = 0;
				}
			}
			else
			{
				static drmp3 mp3;
				static drwav wav;
				const char *extension = fl_filename_ext((const char *)device);
				if(extension != NULL)
				{
					if(strcmp(extension, ".mp3") == 0)
					{
						if(drmp3_init_file(&mp3, device, NULL)) 
						{
							p_mp3 = &mp3;
							failure = 0;
						}
					}
					else if(strcmp(extension, ".wav") == 0)
					{
						if(drwav_init_file(&wav, device, NULL)) 
						{
							p_wav = &wav;
							failure = 0;
						}
					}
					else if(strcmp(extension, ".flac") == 0)
					{
						p_flac = drflac_open_file(device, NULL);
						if(p_flac != NULL)
						{
							failure = 0;
						}
					}
				}
			}
			pause = 1;
			stop = 1;
		}
	}
	else
	{
		int error = 0;
		stream = pa_simple_new(NULL, "webcam", PA_STREAM_PLAYBACK, device, "play", &pulse_ss, NULL, NULL, &error);
		if(stream != NULL)
		{
			pause = 1;
			stop = 1;
		}
		else
		{
			failure = 1;
		}
	}
}

void	PulseAudio::Run()
{
	if(mode == MODE_RECORD)
	{
		pthread_t signal_thread = create_task((int (*)(int *))pulse_record_all, (void *)this);
	}
	else
	{
		pthread_t signal_thread = create_task((int (*)(int *))pulse_play_all, (void *)this);
	}
}

PulseAudio::~PulseAudio()
{
	stop = 1;
	usleep(1000);
	if(stream != NULL)
	{
		int err = 0;
		pa_simple_flush(stream, &err);
		pa_simple_free(stream);
		stream = NULL;
	}
	if(buffer != NULL)
	{
		free(buffer);
		buffer = NULL;
	}
	if(device != NULL)
	{
		free(device);
	}
}

void	PulseAudio::Play()
{
	pause = 0;
	if(stop == 1)
	{
		stop = 0;
	}
}

void	PulseAudio::Record()
{
	pause = 0;
	if(stop == 1)
	{
		stop = 0;
	}
	int err = 0;
	if(is_microphone == 1)
	{
		if(stream != NULL)
		{
			pa_simple_flush(stream, &err);
		}
	}
	else
	{
		if(ndi_capture != 1)
		{
			if(p_mp3 != NULL)
			{
				drmp3_seek_to_pcm_frame((drmp3 *)p_mp3, 0);
			}
			else if(p_wav != NULL)
			{
				drwav_seek_to_pcm_frame((drwav *)p_wav, 0);
			}
			else if(p_flac != NULL)
			{
				drflac_seek_to_pcm_frame((drflac *)p_flac, 0);
			}
		}
	}
}

void	PulseAudio::Pause()
{
	pause = 1;
}

void	PulseAudio::Resume()
{
	pause = 0;
	if(stop == 1)
	{
		stop = 0;
	}
	int err = 0;
	if(is_microphone == 1)
	{
		if(stream != NULL)
		{
			pa_simple_flush(stream, &err);
		}
	}
}

void	PulseAudio::Stop()
{
	pause = 1;
	stop = 1;
	int err = 0;
	if(stream != NULL)
	{
		pa_simple_flush(stream, &err);
	}
}

int	done = 0;

void grab_sample(PulseAudio *pa)
{
	write(STDOUT_FILENO, pa->buffer, pa->buffer_size);
}

void use_sample(void *in_win, PulseAudio *pa)
{
}

int	play_audio_process(char *device, int channels, int rate)
{
	SAMPLE *buffer = (SAMPLE *)malloc(1024 * sizeof(SAMPLE));
	PulseAudio *pa_play = new PulseAudio("alsa_output.pci-0000_00_1f.3.hdmi-stereo", MODE_PLAY, 4096, rate, channels, buffer);
	pa_play->sample_ready_cb = use_sample;
	pa_play->Play();
	while(done == 0)
	{
		sleep(1);
	}
	pa_play->Pause();
	pa_play->Stop();
	delete pa_play;

	return(0);
}

// ************************************ TEST IF AVAILABLE ********************************

int sinks = 0, sources = 0;

void sink_cb(pa_context *c, const pa_sink_info *i, int eol, void *userdata) 
{
	if(!eol && i) sinks++;
}

void source_cb(pa_context *c, const pa_source_info *i, int eol, void *userdata)
{
	if(!eol && i) sources++;
}

void context_state_cb(pa_context *c, void *userdata) 
{
	pa_mainloop_api *api = (pa_mainloop_api *)userdata;
	if(pa_context_get_state(c) == PA_CONTEXT_READY) 
	{
		pa_context_get_sink_info_list(c, sink_cb, NULL);
		pa_context_get_source_info_list(c, source_cb, NULL);
	} 
	else if(pa_context_get_state(c) == PA_CONTEXT_FAILED) 
	{
		api->quit(api, 1);
	}
}

int		test_if_pulse_devices_are_available(int& out_sinks, int& out_sources)
{
	pa_mainloop *ml = pa_mainloop_new();
	pa_context *ctx = pa_context_new(pa_mainloop_get_api(ml), "DeviceTest");

	pa_context_set_state_callback(ctx, context_state_cb, pa_mainloop_get_api(ml));
	pa_context_connect(ctx, NULL, PA_CONTEXT_NOFLAGS, NULL);

	for(int i = 0; i < 5000; i++)
	{
		pa_mainloop_iterate(ml, 0, NULL);
	}
	out_sinks = sinks;
	out_sources = sources;
	int total = sinks + sources;

	pa_context_unref(ctx);
	pa_mainloop_free(ml);
	return(total);
}
