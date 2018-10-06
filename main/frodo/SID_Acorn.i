/*
 * SID_Acorn.i
 *
 * RISC OS specific parts of the sound emulation
 * Frodo (C) 1994-1997,2002 Christian Bauer
 * Acorn port by Andreas Dehmel, 1997
 *
 */

#include "C64.h"



void DigitalRenderer::init_sound(void)
{
  _kernel_oserror *err;

  ready = false; sound_buffer = NULL;
  if ((DigitalRenderer_ReadState() & DRState_Active) != 0)
  {
    _kernel_oserror dra;

    dra.errnum = 0; sprintf(dra.errmess,"Can't claim sound system -- already active!");
    Wimp_ReportError(&dra,1,TASKNAME); return;
  }
  // Try starting up the renderer
  sndbufsize = 2*224; linecnt = 0;
  if ((err = DigitalRenderer_Activate(1,sndbufsize,1000000/SAMPLE_FREQ)) != NULL)
  {
    Wimp_ReportError(err,1,TASKNAME); return;
  }
  sound_buffer = new uint8[sndbufsize];
  ready = true;
}




DigitalRenderer::~DigitalRenderer()
{
  if (ready)
  {
    _kernel_oserror *err;

    delete sound_buffer;
    if ((err = DigitalRenderer_Deactivate()) != NULL)
    {
      Wimp_ReportError(err,1,TASKNAME);
    }
  }
}




void DigitalRenderer::EmulateLine(void)
{
  if (ready)
  {
    sample_buf[sample_in_ptr++] = volume;
    // faster than modulo; usually there shouldn't be a loop (while)...
    while (sample_in_ptr >= SAMPLE_BUF_SIZE) {sample_in_ptr -= SAMPLE_BUF_SIZE;}

    // A similar approach to the HP variant: check every <number> of lines if
    // new sample needed.
    if (--linecnt < 0)
    {
      int status;

      linecnt = the_c64->PollSoundAfter;
      if ((status = DigitalRenderer_ReadState()) > 0)
      {
        if ((status & DRState_NeedData) != 0)
        {
          calc_buffer(sound_buffer, sndbufsize);
          DigitalRenderer_NewSample(sound_buffer);
        }
      }
    }
  }
}




void DigitalRenderer::Pause(void)
{
  if (ready) {DigitalRenderer_Pause();}
}




void DigitalRenderer::Resume(void)
{
  if (ready) {DigitalRenderer_Resume();}
}
