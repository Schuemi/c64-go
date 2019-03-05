/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
#include "LibOdroidGo.h"
#include "minIni.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include "odroid_settings.h"


int lastPressedKey = -1;
char* lastGame;

void c64_beforeStart() {
    lastGame = malloc(1024);
    ini_gets("C64", "LASTGAME", "", lastGame, 1024, FRODO_CONFIG_FILE);
    //C64_1541emluation(ini_getl("C64", "1541emulation", 0, FRODO_CONFIG_FILE));
    
    C64_InsertDisc(8, "/sd/odroid/data/c64/nav96");
    C64_InsertDisc(9, "");
    C64_InsertDisc(10, "");
    C64_InsertDisc(11, "");
    
}

void c64_started() {
    
    
#ifdef WITH_WLAN    
    if (mp_isMultiplayer()) {
        strncpy(lastGame, getMPFileName(),1024);
       
    }
#endif
    
    char* lastState = malloc(1024);
    char* lastDir = malloc(1024);
    C64_setStdKeymapping();
    LoadKeyMapping(FRODO_CONFIG_FILE);
    
   
    // go to last dir, if exists
    ini_gets("C64", "LASTDIR", "", lastDir, 1024, FRODO_CONFIG_FILE);
    chdir(lastDir);
    
    
  
    if (strlen(lastGame) > 0) {
        // insert disc
        odroidFrodoGUI_msgBox("Loading...", "Loading last game, please wait", 0);
        strncpy(lastState, lastGame, 1024);
        changeExtension(lastState, ".sta");
#ifdef WITH_WLAN    
    if (mp_isMultiplayer() || ! fileExist(lastState)) {
#else
        if (! fileExist(lastState)) {
#endif
            C64_LoadSnapshot("/sd/odroid/data/c64/DEFAULT.sta");
            C64_InsertDisc(8, lastGame);
            if (! mp_isMultiplayer() || mp_isServer())  C64_sendKeys("@8\n"); // reload disc
        } else {
            C64_InsertDisc(8, lastGame);
            if (! C64_LoadSnapshot(lastState)){
                // fatal: snapshot is corrupt. Delete file and restart the esp.
                _remove(lastState);
                esp_restart();
            }
        }
        odroidFrodoGUI_setLastLoadedFile(lastGame);
        loadKeyMappingFromGame(getFileName(lastGame));
        
        
                           
    } else {
        C64_LoadSnapshot("/sd/odroid/data/c64/DEFAULT.sta");
    }
    free(lastState);
    free(lastDir);
    
   
}
void SetKeyMapping(int Key, char* mappingString) {
    
    
    if (strlen(mappingString) > 3) {
        if (!strcmp(mappingString, "JST_UP")) C64_setKeymapping(Key, JOY1_UP);
        if (!strcmp(mappingString, "JST_RIGHT")) C64_setKeymapping(Key, JOY1_RIGHT);
        if (!strcmp(mappingString, "JST_DOWN")) C64_setKeymapping(Key, JOY1_DOWN);
        if (!strcmp(mappingString, "JST_LEFT")) C64_setKeymapping(Key, JOY1_LEFT);
        if (!strcmp(mappingString, "JST_FIRE")) C64_setKeymapping(Key, JOY1_BTN);

        if (!strcmp(mappingString, "JST2_UP")) C64_setKeymapping(Key, JOY2_UP);
        if (!strcmp(mappingString, "JST2_RIGHT")) C64_setKeymapping(Key, JOY2_RIGHT);
        if (!strcmp(mappingString, "JST2_DOWN")) C64_setKeymapping(Key, JOY2_DOWN);
        if (!strcmp(mappingString, "JST2_LEFT")) C64_setKeymapping(Key, JOY2_LEFT);
        if (!strcmp(mappingString, "JST2_FIRE")) C64_setKeymapping(Key, JOY2_BTN);


        if (!strcmp(mappingString, "KEY_SPC")) C64_setKeymapping(Key, KEY_SPC);

        if (!strcmp(mappingString, "KEY_CUD")) C64_setKeymapping(Key, KEY_CUD);
        if (!strcmp(mappingString, "KEY_F5")) C64_setKeymapping(Key, KEY_F5);
        if (!strcmp(mappingString, "KEY_F3")) C64_setKeymapping(Key, KEY_F3);
        if (!strcmp(mappingString, "KEY_F1")) C64_setKeymapping(Key, KEY_F1);
        if (!strcmp(mappingString, "KEY_F7")) C64_setKeymapping(Key, KEY_F7);
        
        if (!strcmp(mappingString, "KEY_RET")) C64_setKeymapping(Key, '\n');

        if (!strcmp(mappingString, "KEY_CLR")) C64_setKeymapping(Key, KEY_CLR);
        if (!strcmp(mappingString, "KEY_DEL")) C64_setKeymapping(Key, KEY_DEL);
        if (!strcmp(mappingString, "KEY_SHL")) C64_setKeymapping(Key, KEY_SHL);
        if (!strcmp(mappingString, "KEY_SHR")) C64_setKeymapping(Key, KEY_SHR);

        if (!strcmp(mappingString, "KEY_HOM")) C64_setKeymapping(Key, KEY_HOM);
        if (!strcmp(mappingString, "KEY_R_S")) C64_setKeymapping(Key, KEY_R_S);
        if (!strcmp(mappingString, "KEY_COMM")) C64_setKeymapping(Key, KEY_COMM);
        if (!strcmp(mappingString, "KEY_CTL")) C64_setKeymapping(Key, KEY_CTL);

        if (!strcmp(mappingString, "KEY_BAK")) C64_setKeymapping(Key, KEY_BAK);
        if (!strcmp(mappingString, "KEY_POUND")) C64_setKeymapping(Key, KEY_POUND);
        if (!strcmp(mappingString, "KEY_SLO")) C64_setKeymapping(Key, KEY_SLO);
        if (!strcmp(mappingString, "KEY_RESTORE")) C64_setKeymapping(Key, KEY_RESTORE);

        if (!strcmp(mappingString, "KEY_FM")) C64_setKeymapping(Key, KEY_FM);
    } else if (strlen(mappingString) > 0) {
        C64_setKeymapping(Key,mappingString[0]);
    }   

    
    
    
}
char LoadKeyMapping(char* KeyFile) {
   
    if (! fileExist(KeyFile)) return 0;
    char buffer[16];
    int res;

    res = ini_gets("KEYMAPPING", "UP", "", buffer, 16, KeyFile);
    if (res) SetKeyMapping(ODROID_INPUT_UP, buffer);
    
    res = ini_gets("KEYMAPPING", "RIGHT", "", buffer, 16, KeyFile);
    if (res) SetKeyMapping(ODROID_INPUT_RIGHT, buffer);
    
    res = ini_gets("KEYMAPPING", "DOWN", "", buffer, 16, KeyFile);
    if (res) SetKeyMapping(ODROID_INPUT_DOWN, buffer);
    
    res = ini_gets("KEYMAPPING", "LEFT", "", buffer, 16, KeyFile);
    if (res) SetKeyMapping(ODROID_INPUT_LEFT, buffer);
    
    res = ini_gets("KEYMAPPING", "SELECT", "", buffer, 16, KeyFile);
    if (res) SetKeyMapping(ODROID_INPUT_SELECT, buffer);
    
    res = ini_gets("KEYMAPPING", "START", "", buffer, 16, KeyFile);
    if (res) SetKeyMapping(ODROID_INPUT_START, buffer);
    
    res = ini_gets("KEYMAPPING", "A", "", buffer, 16, KeyFile);
    if (res) SetKeyMapping(ODROID_INPUT_A, buffer);
    
    res = ini_gets("KEYMAPPING", "B", "", buffer, 16, KeyFile);
    if (res) SetKeyMapping(ODROID_INPUT_B, buffer);
    
      
    return 1;
    
    
}

void loadKeyMappingFromGame(const char* gameFileName) {
    const char* filename = getFileName(gameFileName);
    char* keyBoardFile = malloc(256);
    char* keyBoardFileFullPath = malloc(612);
    strncpy(keyBoardFile, filename, 256);
    keyBoardFile = cutExtension(keyBoardFile);
    snprintf(keyBoardFileFullPath, 612, "/sd/odroid/data/c64/%s.ini", keyBoardFile);
    LoadKeyMapping(keyBoardFileFullPath);
    free(keyBoardFile);
    free(keyBoardFileFullPath);
}

void odroid_keyPress(odroid_gamepad_state out_state) {
    
   
}
void c64_vblank(odroid_gamepad_state out_state) {
    
}