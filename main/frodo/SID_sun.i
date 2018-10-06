/*
 *  SID_sun.i - 6581 emulation, SUN specific stuff
 *
 *  Marc Chabanas
 * 
 *  The SID emulation will slow down the entire emulator to 100% after
 *  it is selected. It is not compatible with the SID filters so these
 *  have to be disabled if you don't want to see the performance becoming
 *  as low as 20%.
 */

extern "C" {
	#include <sys/audioio.h>
	#include <unistd.h>
	#include "VIC.h"
}

#define FRAGSIZE (SAMPLE_FREQ/CALC_FREQ)  // samples in a fragment
#define MAXBUFFERED (3*FRAGSIZE) // allow ourself a little buffering

/*
 *  Initialization
 */

void DigitalRenderer::init_sound(void)
{
 struct audio_info info;

 ready = false;
	
 if ((fd = open("/dev/audio", O_WRONLY|O_NDELAY, 0)) < 0) 
   {
    fprintf(stderr, "SID_sun : unable to open /dev/audio.\n");
    return;
   }
	
 AUDIO_INITINFO(&info);
 info.play.sample_rate = SAMPLE_FREQ;
 info.play.channels = 1;
 info.play.precision = 16;
 info.play.encoding = AUDIO_ENCODING_LINEAR;
 info.play.port = AUDIO_SPEAKER;
 if (ioctl(fd, AUDIO_SETINFO, &info)) 
   {
    fprintf(stderr,
	    "SID_sun : unable to select 16 bits/%d khz linear encoding.\n",
            SAMPLE_FREQ);
    return;
   } 
 else 
   {
    fprintf(stderr,"SID_sun : selecting 16 bits/%d khz linear encoding.\n",
            SAMPLE_FREQ);
   }

 sound_calc_buf = new int16[FRAGSIZE];
 sent_samples = 0;
 delta_samples = 0;

 ready = true;
 return;
}


/*
 *  Destructor
 */

DigitalRenderer::~DigitalRenderer()
{
 fprintf(stderr,"SID_sun : leaving audio mode.\n");
 if (fd > 0) 
   {
    if (ready) 
      ioctl(fd, AUDIO_DRAIN);
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
 static int divisor = 0;
 static int to_output = 0;

 static unsigned int sleeping;

 int ret;

 sample_buf[sample_in_ptr] = volume;
 sample_in_ptr = (sample_in_ptr + 1) % SAMPLE_BUF_SIZE;

 /*
  * Now see how many samples have to be added for this line
  */

 divisor += SAMPLE_FREQ;
 while (divisor >= 0)
    divisor -= TOTAL_RASTERS*SCREEN_FREQ, to_output++;

 if (to_output >= FRAGSIZE) /* got enough to send */
   {
    calc_buffer(sound_calc_buf, FRAGSIZE*2);

    ret = write (fd,sound_calc_buf, FRAGSIZE*2);
    if (ret != FRAGSIZE*2) /* should not happen */
      {
       fprintf(stderr,"SID_sun : write to audio device failed. Exit.\n");
       exit(-1);
      }	

    sent_samples+=FRAGSIZE;
    to_output=0;

    if (ioctl(fd, AUDIO_GETINFO, &status)) 
      {
       fprintf(stderr,"SID_sun : get audio status failed. Exit.\n");
       exit(-1);
      }
    while ((delta_samples = (sent_samples - status.play.samples)) > MAXBUFFERED)
      {
       sleeping = ((1000000*(delta_samples-MAXBUFFERED))/SAMPLE_FREQ);
       usleep(sleeping);

       if (ioctl(fd, AUDIO_GETINFO, &status))
         {
          fprintf(stderr,"SID_sun : get audio status failed. Exit.\n");
          exit(-1);
         }
      }
   }
}
