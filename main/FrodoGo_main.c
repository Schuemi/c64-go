/*
 * The MIT License
 *
 * Copyright 2018 Schuemi.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
/* 
 * File:   main.cpp
 * Author: Schuemi
 *
 * Created on 25. Juli 2018, 09:38
 */


// used online Font creator: http://dotmatrixtool.com
// set to : 8px by 8px, row major, little endian.

#include <stdint.h>
#include <esp_err.h>
#include "odroid_sdcard.h"
#include "odroid_display.h"
#include "odroid_system.h"

#include "esp_system.h"
#include "odroid_input.h"
#include "nvs_flash.h"

#include "LibOdroidGo.h"
#include "odroid_settings.h"
#include "utils.h"

#include <sys/time.h>

#include "frodo/sysdeps.h"

#define SD_BASE_PATH "/sd"


// To build set in sdkconfig:

// CONFIG_FATFS_PER_FILE_CACHE=
// otherwise I got "E (9654) diskio_sdmmc: sdmmc_read_blocks failed (257)" errors on some d64 files.

extern void frodo_main(void);
ranctx pseudoRand;


void app_main(void) {
           
  nvs_flash_init();
 

   uint8_t failure = 0;
   
   
   odroid_system_init();
   
   odroid_input_gamepad_init();
   // Display
   ili9341_prepare();
   ili9341_init();
   ili9341_write_frame_rectangleLE(0,0,320,240,NULL); // clear screen
   
   esp_err_t r = odroid_sdcard_open(SD_BASE_PATH);
    if (r != ESP_OK)
    {
    //   odroid_display_show_sderr(ODROID_SD_ERR_NOCARD);
        failure = 1;
        printf("ODROID_SD_ERR_NOCARD\n");
    }
   initFiles();
   #ifdef WITH_WLAN
        ODROID_WLAN_TYPE wlan = odroid_settings_WLAN_get();
        if (wlan == ODROID_WLAN_AP) server_init();
        if (wlan == ODROID_WLAN_STA) client_init();
        if (wlan == ODROID_WLAN_NONE) {
            // we are not in WLAN, so we can have a better random
            struct timeval tv;
            gettimeofday(&tv, NULL);
            srand(tv.tv_usec);
        }
    #endif
   
   if (! failure) { 
        if (! dirExist(FRODO_ROOT_GAMESDIR)) mkdir(FRODO_ROOT_GAMESDIR, 666);
        if (! dirExist(FRODO_ROOT_GAMESDIR"/bios")) mkdir(FRODO_ROOT_GAMESDIR"/bios", 666);
        if (! dirExist(FRODO_ROOT_DATADIR)) mkdir(FRODO_ROOT_DATADIR, 666);
   }
   
   if (! failure && (! fileExist(FRODO_ROOT_GAMESDIR"/bios/1541 ROM") || ! fileExist(FRODO_ROOT_GAMESDIR"/bios/Basic ROM") || ! fileExist(FRODO_ROOT_GAMESDIR"/bios/Char ROM") || ! fileExist(FRODO_ROOT_GAMESDIR"/bios/Kernal ROM"))) {
       failure = 2;
   }
   
   
   
   
   odroidFrodoGUI_initMenu();
   
   if (failure == 1) {
       odroidFrodoGUI_msgBox("Failed", "Could not mound SD-Card!", 1);
   } else if(failure == 2) {
       odroidFrodoGUI_msgBox("Failed", "Could not start C64!\nMissing BIOS files.", 1);         
   }else {
        writeNAV96();
        writeDEFAULT();
        #ifdef WITH_WLAN
            if (wlan == ODROID_WLAN_AP){ server_wait_for_player(); } 
            if (wlan == ODROID_WLAN_STA){ client_try_connect();} 
            

        #endif 
        frodo_main();
        
   }
   
   
   
   
}

