#include <string.h>
#include <stdio.h>

#define INPUT_SAMPLES   1024				 // samples/channel per input buffer
#define LOCAL_CHANNELS	2
#define BYTES_PER_SAMP  2					// 16-bit
#define INPUT_BYTES	 (INPUT_SAMPLES * LOCAL_CHANNELS * BYTES_PER_SAMP)

#define RING_BYTES	  (INPUT_BYTES * 8)	// headroom; size to taste
#define SCRATCH_BYTES   8192				 // must cover your largest frame_bytes

static unsigned char ring[RING_BYTES];
static size_t write_pos = 0;
static size_t read_pos  = 0;
static size_t queued	= 0;				 // bytes currently buffered, unread

// bytes needed per call to simple_record, e.g. 1152 * channels * 2 for MP3,
// 1536 * channels * 2 for AC-3, etc. Set this once per active codec.
static size_t frame_bytes = 1152 * LOCAL_CHANNELS * BYTES_PER_SAMP;

static unsigned long dropped_buffers = 0;	// running count, for diagnostics

void set_frame_bytes(size_t n) 
{
	frame_bytes = n * LOCAL_CHANNELS * BYTES_PER_SAMP;
}

unsigned long get_dropped_count(void) 
{
	return(dropped_buffers);
}

// Call this with each fixed-size input buffer as it becomes ready.
// Returns 0 on success, -1 if the buffer was dropped (ring full).
int feed_input(int *muxer, unsigned char *input_buf) 
{
int	 simple_record(int *flag, unsigned char *in_buffer);

	if((queued + INPUT_BYTES) > RING_BYTES) 
	{
		dropped_buffers++;
		fprintf(stderr, "feed_input: ring overrun, dropping buffer (queued=%zu, cap=%d, total dropped=%lu)\n", queued, RING_BYTES, dropped_buffers);
		return(-1);
	}
	size_t remaining = INPUT_BYTES;
	unsigned char *src = input_buf;

	while(remaining > 0) 
	{
		size_t chunk = RING_BYTES - write_pos;	  // room to end of ring
		if(chunk > remaining)
		{
			chunk = remaining;
		}
		memcpy(ring + write_pos, src, chunk);

		write_pos = (write_pos + chunk) % RING_BYTES;
		src	+= chunk;
		remaining -= chunk;
		queued += chunk;
	}
	// drain as many full encoder-frames as are now available
	while(queued >= frame_bytes) 
	{
		if(frame_bytes > SCRATCH_BYTES) 
		{
			fprintf(stderr, "feed_input: frame_bytes (%zu) exceeds scratch buffer (%d), skipping frame\n", frame_bytes, SCRATCH_BYTES);
			read_pos = (read_pos + frame_bytes) % RING_BYTES;
			queued -= frame_bytes;
			continue;
		}
		if((read_pos + frame_bytes) <= RING_BYTES) 
		{
			// contiguous — hand the pointer straight in
			simple_record(muxer, ring + read_pos);
		} 
		else 
		{
			// wraps — stage into a linear scratch buffer first
			static unsigned char scratch[SCRATCH_BYTES];
			size_t first = RING_BYTES - read_pos;
			memcpy(scratch, ring + read_pos, first);
			memcpy(scratch + first, ring, frame_bytes - first);
			simple_record(muxer, scratch);
		}
		read_pos = (read_pos + frame_bytes) % RING_BYTES;
		queued -= frame_bytes;
	}
	return(0);
}
