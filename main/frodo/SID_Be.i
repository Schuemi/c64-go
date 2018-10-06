/*
 *  SID_Be.i - 6581 emulation, Be specific stuff
 *
 *  Frodo (C) 1994-1997,2002 Christian Bauer
 */

#include <MediaKit.h>

#include "C64.h"


/*
 *  Initialization, open subscriber
 */

void DigitalRenderer::init_sound(void)
{
	in_stream = false;

	the_stream = new BDACStream();
	the_sub = new BSubscriber("Frodo SID emulation");
	ready = the_sub->Subscribe(the_stream) == B_NO_ERROR;
	if (ready) {
		the_stream->SetSamplingRate(SAMPLE_FREQ);
		the_stream->SetStreamBuffers(SAMPLE_FREQ / CALC_FREQ * 4, 4);	// Must be called after EnterStream()
		the_sub->EnterStream(NULL, true, this, stream_func, NULL, true);
		in_stream = true;
	}
}


/*
 *  Destructor, close subscriber
 */

DigitalRenderer::~DigitalRenderer()
{
	if (ready) {
		if (in_stream) {
			the_sub->ExitStream(true);
			in_stream = false;
		}
		the_stream->SetStreamBuffers(4096, 8);
		the_sub->Unsubscribe();
		ready = false;
	}
	delete the_sub;
	delete the_stream;
}


/*
 *  Sample volume (for sampled voice)
 */

void DigitalRenderer::EmulateLine(void)
{
	sample_buf[sample_in_ptr] = volume;
	sample_in_ptr = (sample_in_ptr + 1) % SAMPLE_BUF_SIZE;
}


/*
 *  Pause sound output
 */

void DigitalRenderer::Pause(void)
{
	if (in_stream) {
		the_sub->ExitStream(true);
		in_stream = false;
	}
}


/*
 *  Resume sound output
 */

void DigitalRenderer::Resume(void)
{
	if (!in_stream) {
		the_sub->EnterStream(NULL, true, this, stream_func, NULL, true);
		in_stream = true;
	}
}


/*
 *  Stream function 
 */

bool DigitalRenderer::stream_func(void *arg, char *buf, size_t count, void *header)
{
	((DigitalRenderer *)arg)->calc_buffer((int16 *)buf, count);
	((DigitalRenderer *)arg)->the_c64->SoundSync();
	return true;
}
