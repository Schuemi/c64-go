/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
#include "LibOdroidGo.h"
#include "sysdeps.h"
#include "C64.h"
#include "IEC.h"
#include "Prefs.h"
#include "Display.h"

#include <stddef.h>

#include "freertos/FreeRTOS.h"
#include "SID.h"
#include <freertos/task.h>


static C64* theC64 = NULL;

void C64_setInstance(void* c64Instance) {
    theC64 = (C64*)c64Instance;
}
bool C64_SaveSnapshot(char *filename) {
    if (! theC64) return false;
    theC64->SaveSnapshot(filename);
    
    return true;
}
bool C64_LoadSnapshot(char *filename) {
    if (! theC64) return false;
    return theC64->LoadSnapshot(filename);
}
bool c64_isNAVRunning() {
    if (! theC64) return false;
    return theC64->TheDisplay->isNAVrunning();
}
void reloadDiskNAV(int dsk) {
    if (! theC64) return;
    return theC64->TheDisplay->reloadDiskNAV(dsk);
}
void C64_CloseAllChannels() {
    Prefs p = ThePrefs;
    for (int i=0; i<4; i++)
        strncpy(p.DrivePath[i], "", 256);
    
    theC64->NewPrefs(&p);
    ThePrefs = p;
}
bool C64_getJoystickSwap() {
  return ThePrefs.JoystickSwap; 
}
bool C64_SwitchJoystickPort() {
    if (! ThePrefs.JoystickSwap)
        ThePrefs.JoystickSwap= true;
    else
        ThePrefs.JoystickSwap= false;
    
    return ThePrefs.JoystickSwap;
}
void C64_sendKeys(const char* keys) {
    theC64->TheDisplay->sendKeys(keys);
}
bool C64_InsertDisc(int deviceNumber, const char* filename) {
    if (deviceNumber < 8) return false;
    if (! theC64){
        // c64 is not running yet, we can change the prefs without newPrefs
        ThePrefs.DriveType[deviceNumber-8] = DRVTYPE_D64;
        strncpy(ThePrefs.DrivePath[deviceNumber - 8], filename, 256);
        return true;
    }
    Prefs p = ThePrefs;
    p.DriveType[deviceNumber-8] = DRVTYPE_D64;
    strncpy(p.DrivePath[deviceNumber - 8], filename, 256);
    theC64->NewPrefs(&p);
    ThePrefs = p;
    return true;
}
void C64_setStdKeymapping() {
    if (! theC64) return;
    theC64->TheDisplay->setStdKeymapping();
}
void C64_setKeymapping(char odroidKey, int c64Key) {
    if (! theC64) return;
    theC64->TheDisplay->setKeymapping(odroidKey, c64Key);
}

void C64_Reset() {
    if (! theC64) return;
    theC64->Reset();
}

void C64_PauseAudio() {
    if (! theC64) return;
    theC64->TheSID->PauseSound();
}

void C64_ResumeAudio() {
    if (! theC64) return;
    theC64->TheSID->ResumeSound();
}

void C64_1541emluation( char on ) {
    if (! theC64){
        // c64 is not running yet, we can change the prefs without newPrefs
        if (on)ThePrefs.Emul1541Proc = true; else ThePrefs.Emul1541Proc = false;
        return;
    }
    Prefs p = ThePrefs;
    if (on)p.Emul1541Proc = true; else p.Emul1541Proc = false;
    theC64->NewPrefs(&p);
    ThePrefs = p;
}
void C64_setFrameSkip( int frames ) {
    
    ThePrefs.SkipFrames = frames;
    
}

char C64_is1541emluation(  ) {
    return ThePrefs.Emul1541Proc;
}