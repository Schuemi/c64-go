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
 * File:   LibOdroidGo.h
 * Author: Schuemi
 *
 * Created on 25. Juli 2018, 09:57
 */

#ifndef LIBODROIDGO_H
#define LIBODROIDGO_H


#include <stdint.h>
#include <stdio.h>

#include <dirent.h>
#include <sys/stat.h>

#include <stdbool.h>

#include "odroid_input.h"

#define FRODO_CONFIG_FILE "/sd/odroid/data/c64/config.ini"
#define FRODO_ROOT_GAMESDIR "/sd/roms/c64"
#define FRODO_ROOT_DATADIR "/sd/odroid/data/c64"

#ifdef __cplusplus
extern "C" {
#endif
    
    
#ifdef WITH_WLAN
//////////////// Multiplayer ///////////////
typedef enum  {
    MULTIPLAYER_NOT_CONNECTED,
    MULTIPLAYER_INIT,
    MULTIPLAYER_CONNECTED_SERVER,
    MULTIPLAYER_CONNECTED_CLIENT
    
}MULTIPLAYER_STATE;

void server_init();
void client_init();
void server_wait_for_player();
void client_try_connect();
MULTIPLAYER_STATE getMultiplayState();
void exchangeNetworkState(unsigned char *key_matrix, unsigned char *rev_matrix, unsigned char *joystick1, unsigned char *joystick2);
const char* getMPFileName();
char mp_isMultiplayer();
char mp_isServer();
#endif

///////////// function keys ////////////////////
void c64_beforeStart();
void c64_started();
void odroid_keyPress(odroid_gamepad_state out_state);

#define KEY_SPC 0x20
#define KEY_CUD 129
#define KEY_F5 130
#define KEY_F3 131
#define KEY_F1 132
#define KEY_F7 133
#define KEY_CLR 134
#define KEY_DEL 135
#define KEY_SHL 136
#define KEY_SHR 137
#define KEY_HOM 138
#define KEY_R_S 139
#define KEY_COMM 140
#define KEY_CTL 141
#define KEY_BAK 142
#define KEY_POUND 143
#define KEY_SLO 144  // shift lock, no actual key, it holds the KEY_SHL until it is released
#define KEY_RESTORE 145 // this is no key, directly connected to the NMI 


#define JOY1_UP 1 << 8
#define JOY1_DOWN 2 << 8
#define JOY1_LEFT 3 << 8
#define JOY1_RIGHT 4 << 8
#define JOY1_BTN 5 << 8

#define JOY2_UP 6 << 8
#define JOY2_DOWN 7 << 8
#define JOY2_LEFT 8 << 8
#define JOY2_RIGHT 9 << 8
#define JOY2_BTN 10 << 8

#define KEY_FM 11 << 8 // this is no key, its the fast Mode Button

////////////// Menu //////////////////
typedef enum  {
    MENU_ACTION_NONE,
    MENU_ACTION_CHANGED_AUDIO_TYPE
} MENU_ACTION;

void odroidFrodoGUI_initMenu();
MENU_ACTION odroidFrodoGUI_showMenu();
void odroidFrodoGUI_setLastLoadedFile(const char* file);
void odroidFrodoGUI_msgBox(const char* title, const char* msg, char waitKey);
int odroidFrodoGUI_getKey_block();
int odroidFrodoGUI_getKey();
///////////////// AUDIO ////////////////////////



//////// C64 c++ wrapper to c /////////////////////////
void C64_setInstance(void* c64Instance);
bool C64_SaveSnapshot(char *filename);
bool C64_LoadSnapshot(char *filename);
bool c64_isNAVRunning();
void reloadDiskNAV(int dsk);
void C64_CloseAllChannels();
void C64_Reset();
bool C64_InsertDisc(int deviceNumber, const char* filename);
bool C64_SwitchJoystickPort();
bool C64_getJoystickSwap();
void C64_sendKeys(const char* keys);
void C64_PauseAudio();
void C64_ResumeAudio();
void C64_setStdKeymapping();
void C64_setKeymapping(char odroidKey, int c64Key);
void C64_1541emluation( char on );
char C64_is1541emluation(  );
void C64_setFrameSkip( int frames ) ;
#if defined(ESP32)
void C64_SAM();
#endif
///////////////////////////////////////////
///////////////////// FILES ////////////////////////////////////

void loadKeyMappingFromGame(const char* gameFileName);
char LoadKeyMapping(char* KeyFile);

int initFiles();
char* cutExtension(char* file);
char* changeExtension(char* file, const char* newExt);
const char* getFileName(const char* file);
char* getPath(char* file);
char* getFullPath(char* buffer, const char* fileName, int bufferLength);
bool hasExt(const char *file, const char *Ext);

DIR* _opendir(const char* name);
int _closedir(DIR* f);
void _rewinddir(DIR* pdir);
struct dirent* _readdir(DIR* pdir);
int _stat( const char *__restrict __path, struct stat *__restrict __sbuf );
FILE* _fopen(const char *__restrict _name, const char *__restrict _type);
void _seekdir(DIR* pdir, long loc);
long _telldir(DIR* pdir);
int _fclose(FILE* file);
int _remove(const char * f);
int _rename(const char * f, const char * nf);
size_t _fwrite(const _PTR __restrict p , size_t _size, size_t _n, FILE * f);
int _fseek(FILE * f, long a, int b);
long _ftell( FILE * f);
void _rewind(FILE* f);
size_t _fread(_PTR __restrict p, size_t _size, size_t _n, FILE *__restrict f);



#ifdef __cplusplus
}
#endif

#endif
