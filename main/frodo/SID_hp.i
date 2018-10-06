/*
 *  SID_hp.i - 6581 emulation, HP-UX specific stuff
 *
 *  Lutz Vieweg <lkv@mania.robin.de>
 */

extern "C" {
	#include <sys/audio.h>
	#include <unistd.h>
	#include <fcntl.h>
	#include <sys/ioctl.h>
}

#define TXBUFSIZE  16384 // bytes, not samples
#define TXFRAGSIZE 1024  // samples, not bytes
#define TXLOWWATER (TXBUFSIZE-(TXFRAGSIZE*2)) // bytes, not samples

/*
 *  Initialization
 */

void DigitalRenderer::init_sound(void)
{
	ready = false;
	
	if ((fd = open("/dev/audio", O_WRONLY | O_NDELAY, 0)) < 0) {
		fprintf(stderr, "unable to open /dev/audio -> no sound\n");
		return;
	}
	
	int flags;
	if ((flags = fcntl (fd, F_GETFL, 0)) < 0) {
		fprintf(stderr, "unable to set non-blocking mode for /dev/audio -> no sound\n");
		return;
	}
	flags |= O_NDELAY;
	if (fcntl (fd, F_SETFL, flags) < 0) {
		fprintf(stderr, "unable to set non-blocking mode for /dev/audio -> no sound\n");
		return;
	}
	
	if (ioctl(fd, AUDIO_SET_DATA_FORMAT, AUDIO_FORMAT_LINEAR16BIT)) {
		fprintf(stderr, "unable to select 16bit-linear sample format -> no sound\n");
		return;
	}
	
	if (ioctl(fd, AUDIO_SET_SAMPLE_RATE, 44100)) {
		fprintf(stderr, "unable to select 44.1kHz sample-rate -> no sound\n");
		return;
	}
	
	if (ioctl(fd, AUDIO_SET_CHANNELS, 1)) {
		fprintf(stderr, "unable to select 1-channel playback -> no sound\n");
		return;
	}
	
	// choose between:
	// AUDIO_OUT_SPEAKER
	// AUDIO_OUT_HEADPHONE
	// AUDIO_OUT_LINE
	if (ioctl(fd, AUDIO_SET_OUTPUT, AUDIO_OUT_SPEAKER)) {
		fprintf(stderr, "unable to select audio output -> no sound\n");
		return;
	}

	{
		// set volume:
		audio_describe description;
		audio_gains gains;
		if (ioctl(fd, AUDIO_DESCRIBE, &description)) {
			fprintf(stderr, "unable to get audio description -> no sound\n");
			return;
		}
		if (ioctl (fd, AUDIO_GET_GAINS, &gains)) {
			fprintf(stderr, "unable to get gain values -> no sound\n");
			return;
		}
		
		float volume = 1.0;
		gains.transmit_gain = (int)((float)description.min_transmit_gain +
                                  (float)(description.max_transmit_gain
                                          - description.min_transmit_gain)
                                          * volume);
		if (ioctl (fd, AUDIO_SET_GAINS, &gains)) {
			fprintf(stderr, "unable to set gain values -> no sound\n");
			return;
		}
	}
	
	if (ioctl(fd, AUDIO_SET_TXBUFSIZE, TXBUFSIZE)) {
		fprintf(stderr, "unable to set transmission buffer size -> no sound\n");
		return;
	}
	
	sound_calc_buf = new int16[TXFRAGSIZE];
	
	linecnt = 0;
	
	ready = true;
	return;
}


/*
 *  Destructor
 */

DigitalRenderer::~DigitalRenderer()
{
	if (fd > 0) {
		if (ready) {
			ioctl(fd, AUDIO_DRAIN);
		}
		ioctl(fd, AUDIO_RESET);
		close(fd);
	}
}


/*
 *  Pause sound output
 */

void DigitalRenderer::Pause(void)
{
}


/*
 * Resume sound output
 */

void DigitalRenderer::Resume(void)
{
}


/*
 *  Fill buffer, sample volume (for sampled voice)
 */

void DigitalRenderer::EmulateLine(void)
{
	sample_buf[sample_in_ptr] = volume;
	sample_in_ptr = (sample_in_ptr + 1) % SAMPLE_BUF_SIZE;

	// testing the audio status at each raster-line is
	// really not necessary. Let's do it every 50th one..
	// ought to be enough by far.

	linecnt--;
	if (linecnt < 0 && ready) {
		linecnt = 50;
		
		// check whether we should add some more data to the
		// transmission buffer.

		if (ioctl(fd, AUDIO_GET_STATUS, &status)) {
			fprintf(stderr,"fatal: unable to get audio status\n");
			exit(20);
		}
		
		if (status.transmit_buffer_count < TXLOWWATER) {
			// add one sound fragment..
			calc_buffer(sound_calc_buf, TXFRAGSIZE*2);
			
			// since we've checked for enough space in the transmission buffer,
			// it is an error if the non-blocking write returns anything but
			// TXFRAGSIZE*2
			if (TXFRAGSIZE*2 != write (fd, sound_calc_buf, TXFRAGSIZE*2)) {
				fprintf(stderr,"fatal: write to audio-device failed\n");
				exit(20);
			}
		}
	}
}

