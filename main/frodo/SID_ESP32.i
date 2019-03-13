#include <esp_heap_caps.h>
#include "odroid_audio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include <string.h>

#include "VIC.h"
#include "LibOdroidGo.h"
#include "minIni.h"


#define AUDIO_BUFFER_SAMPLES 128

//#define NO_SOUND
typedef short sample;

DigitalRenderer* dg;

static sample *streamAudioBuffer;

uint16_t audioBufferSize;
QueueHandle_t audioQueue;
ODROID_AUDIO_SINK sink = ODROID_AUDIO_SINK_SPEAKER;

char stop = 0;
char noSound = 0;
char audioPause = 0;
char speedUpPause = 0;
int volLevel = 0;
void DigitalRenderer::audioTask(void* arg)
{
  // sound
  uint16_t* param;
printf("audioTask: starting\n");
  while(!stop)
  {
      
      xQueuePeek(audioQueue, &param, portMAX_DELAY);
      
#ifndef NO_SOUND
      
    if (ThePrefs.LimitSpeed){
        if (speedUpPause) {speedUpPause = 0; odroid_audio_volume_set((odroid_volume_level)volLevel);}
        dg->calc_buffer(streamAudioBuffer, audioBufferSize*sizeof(sample));
        for (int i = audioBufferSize - 1; i >= 0; i--) {
            streamAudioBuffer[i*2] = streamAudioBuffer[i];
            streamAudioBuffer[(i*2)+1] = streamAudioBuffer[i];
        }
        
        if (! stop && ! audioPause) odroid_audio_submit(streamAudioBuffer, audioBufferSize);
        
    } else {
        if (! speedUpPause) {
            odroid_audio_volume_set(ODROID_VOLUME_LEVEL0);
            speedUpPause = 1;
            memset(streamAudioBuffer, 0, audioBufferSize*sizeof(sample));
            if (! stop && ! audioPause) odroid_audio_submit(streamAudioBuffer, 6);
        }
        
    }
#endif
    xQueueReceive(audioQueue, &param, portMAX_DELAY);
  }

  printf("audioTask: exiting.\n");
  odroid_audio_terminate();

  vTaskDelete(NULL);

  
}


void DigitalRenderer::init_sound(void) {
   
    
    
    stop = 0;
    char buf[3];
    
    dg = this;
           
    streamAudioBuffer = (sample*)heap_caps_malloc(AUDIO_BUFFER_SAMPLES*sizeof(sample)*2, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    
    if (! streamAudioBuffer){ printf("streamAudioBuffer failed!\n"); return;}
        
    audioQueue = xQueueCreate(1, sizeof(uint16_t*));
    
    sink = (ODROID_AUDIO_SINK)ini_getl("C64", "DAC", ODROID_AUDIO_SINK_SPEAKER, FRODO_CONFIG_FILE);
    odroid_audio_init(sink, 44100);
    
    volLevel = ini_getl("C64", "VOLUME", ODROID_VOLUME_LEVEL1, FRODO_CONFIG_FILE);
    if (volLevel > ODROID_VOLUME_LEVEL4 || volLevel <  0) volLevel = ODROID_VOLUME_LEVEL0;
    odroid_audio_volume_set((odroid_volume_level)volLevel);
    xTaskCreatePinnedToCore(&audioTask, "audioTask", 1024, NULL, 5, NULL, 1);
    ready = true;
}


void DigitalRenderer::changeVolumeLevel(void) {
    if (noSound) return;
    volLevel++;
    if (volLevel > ODROID_VOLUME_LEVEL4) volLevel = ODROID_VOLUME_LEVEL0;
    ini_putl("C64", "VOLUME", volLevel, FRODO_CONFIG_FILE);
    printf("set volume to %d\n", volLevel);
    odroid_audio_volume_set((odroid_volume_level)volLevel);
    
}


const void* tempPtr = (void*)0x1234;
void DigitalRenderer::EmulateLine(void) {
   if (noSound) return;
    if (ready)
  {
    static int divisor = 0;
    static int to_output = 0;
    

   
	sample_buf[sample_in_ptr] = volume;
	sample_in_ptr = (sample_in_ptr + 1) % SAMPLE_BUF_SIZE;

    /*
     * Now see how many samples have to be added for this line
     */
    divisor += SAMPLE_FREQ;
    while (divisor >= 0)
	divisor -= TOTAL_RASTERS*SCREEN_FREQ, to_output++;

    /*
     * Calculate the sound (on the 2nd CPU) data only when we have enough to fill
     * the buffer entirely.
     */
    if (to_output >= AUDIO_BUFFER_SAMPLES) {
        
	int datalen = AUDIO_BUFFER_SAMPLES;
	to_output -= datalen;
	audioBufferSize = datalen;
        xQueueSend(audioQueue, &tempPtr, portMAX_DELAY);
       
    }  
    
  
        
   }

}


void DigitalRenderer::Pause(void) {
    if (noSound) return;
    void* tempPtr = (void*)0x1234;
    audioPause=1;
    xQueueSend(audioQueue, &tempPtr, portMAX_DELAY); // to wait until sound was send
    odroid_audio_terminate();
}
void DigitalRenderer::Resume(void) {
    if (noSound) return;
    odroid_audio_init(sink, 44100);
    odroid_audio_volume_set((odroid_volume_level)volLevel);
    audioPause=0;
}   

DigitalRenderer::~DigitalRenderer() {
    if (noSound) return;

    stop=1;
    xQueueSend(audioQueue, &tempPtr, portMAX_DELAY); // to wait until sound was send
    free(streamAudioBuffer);
   
}