/*
 *  SID_linux.i - 6581 emulation, Linux specific stuff
 *
 *  Frodo (C) 1994-1997,2002 Christian Bauer
 *  Linux sound stuff by Bernd Schmidt
 */

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/soundcard.h>

#include "VIC.h"


/*
 *  Initialization
 */

void DigitalRenderer::init_sound(void)
{
    int tmp;
    unsigned long format;

    ready = false;
    devfd = open("/dev/dsp", O_WRONLY);
    if (devfd < 0)
	return;

    ioctl(devfd, SNDCTL_DSP_GETFMTS, &format);
    if (!(format & AFMT_S16_LE))
	return;
    format = AFMT_S16_LE;
    ioctl(devfd, SNDCTL_DSP_SETFMT, &format);
	    
    /*
     * Buffer size: 2^9 == 512 bytes. Note that too large buffers will not work
     * very well: The speed of the C64 is slowed down to an average speed of 
     * 100% by the blocking write() call in EmulateLine(). If you use a buffer 
     * of, say 4096 bytes, that will happen only about every 4 frames, which
     * means that the emulation runs much faster in some frames, and much
     * slower in others.
     * On really fast machines, it might make sense to use an even smaller
     * buffer size.
     */
    tmp = 0x00100009;
    ioctl(devfd, SNDCTL_DSP_SETFRAGMENT, &tmp);
    tmp = 0;
    ioctl(devfd, SNDCTL_DSP_STEREO, &tmp);
    tmp = 44100;
    ioctl(devfd, SNDCTL_DSP_SPEED, &tmp);
    ioctl(devfd, SOUND_PCM_READ_RATE, &tmp);
    if (tmp < 43000 || tmp > 45000)
	return;

    ioctl(devfd, SNDCTL_DSP_GETBLKSIZE, &sndbufsize);
    sound_buffer = new int16[sndbufsize];
    ready = true;
}


/*
 *  Destructor
 */

DigitalRenderer::~DigitalRenderer()
{
    if (devfd >= 0)
	close(devfd);
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
 * Fill buffer, sample volume (for sampled voice)
 */

void DigitalRenderer::EmulateLine(void)
{
    static int divisor = 0;
    static int to_output = 0;
    static int buffer_pos = 0;

    if (!ready)
	return;

	sample_buf[sample_in_ptr] = volume;
	sample_in_ptr = (sample_in_ptr + 1) % SAMPLE_BUF_SIZE;

    /*
     * Now see how many samples have to be added for this line
     */
    divisor += SAMPLE_FREQ;
    while (divisor >= 0)
	divisor -= TOTAL_RASTERS*SCREEN_FREQ, to_output++;

    /*
     * Calculate the sound data only when we have enough to fill
     * the buffer entirely.
     */
    if ((buffer_pos + to_output) >= sndbufsize) {
	int datalen = sndbufsize - buffer_pos;
	to_output -= datalen;
	calc_buffer(sound_buffer + buffer_pos, datalen*2);
	write(devfd, sound_buffer, sndbufsize*2);
	buffer_pos = 0;
    }    
}
