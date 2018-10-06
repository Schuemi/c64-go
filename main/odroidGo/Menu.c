/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */


#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "LibOdroidGo.h"
#include "odroid_audio.h"
#include "ugui.h"
#include "odroid_input.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <sys/stat.h>
#include <sys/dirent.h>

#include "utils.h"

#include "minini.h"
#include "keyboard.h"

#define MAX_OBJECTS 6


#define MENU_TEXTBOX_ID 1
#define FILES_TEXTBOX_ID 2 
#define MSG_TEXTBOX_ID 3 
#define IMG_COMMODORE_ID 4


#define FILES_MAX_ROWS   20
#define FILES_MAX_LENGTH 31
#define FILES_HEADER_LENGTH 27
#define FILES_MAX_LENGTH_NAME 256


#define WINDOW_BG_COLOR 0xacf0
#define WINDOW_TITLE_COLOR 0x6B4B
typedef unsigned short pixelp;


pixelp* pixelBuffer;

UG_GUI   gui;
UG_WINDOW window;
UG_TEXTBOX menuTextBox;
UG_OBJECT obj_buff_wnd_1[MAX_OBJECTS];

UG_WINDOW fileWindow;
UG_TEXTBOX fileTextBox;
UG_OBJECT obj_buff_wnd_file[MAX_OBJECTS];

UG_WINDOW msgWindow;
UG_TEXTBOX msgTextBox;
UG_OBJECT obj_buff_wnd_msg[MAX_OBJECTS];

UG_IMAGE commodoreLogo;
UG_BMP commodoreLogoBmp;

int m_width = 320;
int m_height = 240;
int currentSelectedItem = 0;
char* menuTxt;
bool keyPressed;
int lastPressedKey;
int lastSelectedItem = 0;
int holdDownCounter = 0;
char* selectedFile;
char* lastOpenedFileFullPath;

int position = 0;
int selPosition = 0;
#ifdef WITH_WLAN  
    #define MENU_ITEMS 18
#else
    #define MENU_ITEMS 15
#endif

#define MENU_ITEM_LOADFILE      0
#define MENU_ITEM_SAVEFILE      1
#define MENU_ITEM_DELETEFILE    2
#define MENU_ITEM_CHANGEDISK    4
#define MENU_ITEM_EJECTDISK     5
#define MENU_ITEM_SWITCHJOY     6
#define MENU_ITEM_1541          7
#define MENU_ITEM_RESETBASIC    9
#define MENU_ITEM_RESETNAV      10
#define MENU_ITEM_AUDIO         12

#ifdef WITH_WLAN
    #define MENU_ITEM_MULTISERVER   14
    #define MENU_ITEM_MULTICLIENT   15
    #define MENU_ITEM_ABOUT         17
#else
    #define MENU_ITEM_ABOUT         14
#endif





static const struct {
    const char* menuItem[MENU_ITEMS];

    
} menuItems = {   
   "Load file",
   "Save state",  
   "Delete state",   
   "",
   "Change disk",
   "Eject disks",
   "Switch joystick port",
   "1541 emulation",
   "",
   "Reset to Basic",
   "Reset to NAV",
   "",
   "Audio output",
#ifdef WITH_WLAN
   "",
   "Start multiplayer server",
   "Start multiplayer client",
#endif
   "",
   "About"
   
   
};
static uint16_t pixelBlend(uint16_t a, uint16_t b)
{


  char r0 = (a >> 11) & 0x1f;
  char g0 = (a >> 5) & 0x3f;
  char b0 = (a) & 0x1f;

  char r1 = (b >> 11) & 0x1f;
  char g1 = (b >> 5) & 0x3f;
  char b1 = (b) & 0x1f;

  uint16_t rv = ((r1 - r0) >> 1) + r0;
  uint16_t gv = ((g1 - g0) >> 1) + g0;
  uint16_t bv = ((b1 - b0) >> 1) + b0;

  return (rv << 11) | (gv << 5) | (bv);
}

int DrawuGui(uint16_t* uGuiMenu, int y) {
    ili9341_write_frame_rectangleLE(0,y,m_width,m_height - y, uGuiMenu);
    return 0;
}

void pixelSet(UG_S16 ul_x,UG_S16 ul_y, UG_COLOR ul_color, char alpha){
    if (ul_x > m_width || ul_y > m_height) return;
    uint16_t* pix = &(pixelBuffer[ul_x + (ul_y*m_width)]);
    
    if (alpha == 1) ul_color = pixelBlend(*pix, ul_color);
    if (alpha == 2) if(*pix > 0x5555) ul_color = *pix; else  ul_color = WINDOW_BG_COLOR;
    *pix = ul_color;
}

void window_callback( UG_MESSAGE* msg ) {
    
}
void odroidFrodoGUI_initMenu() {
   
  
   pixelBuffer = (pixelp*)heap_caps_malloc(m_width*m_height*sizeof(pixelp), MALLOC_CAP_SPIRAM);
   memset(pixelBuffer, 0, m_width*m_height*sizeof(pixelp));
   
   
   
   selectedFile = (char*)heap_caps_malloc(FILES_MAX_LENGTH_NAME+1, MALLOC_CAP_SPIRAM);
   lastOpenedFileFullPath = (char*)malloc(1024);
   lastOpenedFileFullPath[0] = 0;
   
   menuTxt = (char*)heap_caps_malloc(1000, MALLOC_CAP_SPIRAM);
   
   UG_Init(&gui,pixelSet,m_width, m_height);
   
   UG_WindowCreate ( &window , obj_buff_wnd_1 , MAX_OBJECTS, window_callback);
   UG_WindowResize(&window, 10, 10, 310, 220);
   UG_WindowSetTitleTextFont ( &window , &FONT_8X8 ) ;
   //UG_WindowSetTitleText ( &window , "ODROID-GO C64" ) ;
   UG_WindowSetStyle( &window , WND_STYLE_3D | WND_STYLE_HIDE_TITLE ) ;
   UG_WindowSetBackColor( &window , WINDOW_BG_COLOR );
   UG_WindowSetTitleColor( &window , WINDOW_TITLE_COLOR );
   
   UG_ImageCreate(&window, &commodoreLogo, IMG_COMMODORE_ID, 0, 0, commodore.width, commodore.height-2);
   commodoreLogoBmp.bpp = BMP_BPP_16;
   commodoreLogoBmp.height = commodore.height-2;
   commodoreLogoBmp.width = commodore.width;
   commodoreLogoBmp.p = commodore.pixel_data + commodore.width*2;
   UG_ImageShow(&window, IMG_COMMODORE_ID);
   
   UG_ImageSetBMP(&window,IMG_COMMODORE_ID, &commodoreLogoBmp);
   
   UG_TextboxCreate(&window, &menuTextBox, MENU_TEXTBOX_ID, 10, 30, 290, 160); 
   UG_TextboxSetAlignment(&window, MENU_TEXTBOX_ID, ALIGN_TOP_LEFT);
   UG_TextboxSetForeColor(&window, MENU_TEXTBOX_ID, C_BLACK);
   UG_TextboxSetFont ( &window , MENU_TEXTBOX_ID, &FONT_8X8 ) ;

   
   UG_TextboxShow(&window, MENU_TEXTBOX_ID);
   UG_WindowShow(&window);
   
   
   
   
}

int odroidFrodoGUI_getKey() {
    odroid_gamepad_state out_state;
    odroid_input_gamepad_read(&out_state);
    keyPressed = false;
    for (int i = 0; i < ODROID_INPUT_MAX; i++) if (out_state.values[i]) keyPressed = true;
    if (keyPressed && lastPressedKey != -1){
        holdDownCounter++;
        if (holdDownCounter > 200) return lastPressedKey;
        return -1;
    }
    holdDownCounter = 0;
    
    if (!keyPressed) lastPressedKey = -1; else {
        if (out_state.values[ODROID_INPUT_UP]) lastPressedKey = ODROID_INPUT_UP;
        if (out_state.values[ODROID_INPUT_DOWN]) lastPressedKey = ODROID_INPUT_DOWN;
        if (out_state.values[ODROID_INPUT_LEFT]) lastPressedKey = ODROID_INPUT_LEFT;
        if (out_state.values[ODROID_INPUT_RIGHT]) lastPressedKey = ODROID_INPUT_RIGHT;
        if (out_state.values[ODROID_INPUT_A]) lastPressedKey = ODROID_INPUT_A;
        if (out_state.values[ODROID_INPUT_B]) lastPressedKey = ODROID_INPUT_B;
        if (out_state.values[ODROID_INPUT_MENU]) lastPressedKey = ODROID_INPUT_MENU;
    }
    return lastPressedKey;
}
int odroidFmsxGUI_getKey_block() {
    int key;
    do{
        vTaskDelay(1 / portTICK_PERIOD_MS);
        key = odroidFrodoGUI_getKey();
    }while (key == -1);
    return key;
}
int odroidFmsxGUI_getKey_block_lock() {
    int key;
    int currKey = odroidFrodoGUI_getKey();
    do{
        vTaskDelay(1 / portTICK_PERIOD_MS);
        key = odroidFrodoGUI_getKey();
    }while (key == -1 || currKey == key);
    return key;
}




const char* odroidFmsxGUI_chooseFile(const char *Ext) {
   
   DIR *D;
   struct dirent *DP;
   struct stat ST;
   

   char* Buf;
   int BufSize = 256;
   char* txtFiles;
   char* shownFiles[FILES_MAX_ROWS];
   
   
   txtFiles = heap_caps_malloc(FILES_MAX_ROWS*(FILES_MAX_LENGTH + 5) + 1, MALLOC_CAP_SPIRAM);
   for (int i = 0; i < FILES_MAX_ROWS; i++) {
       shownFiles[i] = heap_caps_malloc(FILES_MAX_LENGTH_NAME+1, MALLOC_CAP_SPIRAM);
   }
   
   Buf = (char*)heap_caps_malloc(256, MALLOC_CAP_SPIRAM);
   char* txtFilesPosition;
   
   
   
   int i, r, s, fileCount;

   
   bool isDir[FILES_MAX_ROWS];
   bool switchedPage = true;
   
   UG_WindowCreate ( &fileWindow , obj_buff_wnd_file , MAX_OBJECTS, window_callback);
   UG_WindowResize(&fileWindow, 20, 20, 300, 210);
   UG_WindowSetTitleTextFont ( &fileWindow , &FONT_8X8 ) ;
   
   if(!getcwd(Buf,BufSize-2)) strncpy(Buf,"Choose File", 256);
   txtFilesPosition = Buf;
   if (strlen(txtFilesPosition) > FILES_HEADER_LENGTH) txtFilesPosition += strlen(txtFilesPosition) - FILES_HEADER_LENGTH;
  
   
   
   UG_WindowSetTitleText ( &fileWindow , txtFilesPosition ) ;
   UG_WindowSetBackColor( &fileWindow , WINDOW_BG_COLOR );
   UG_WindowSetTitleColor( &fileWindow , WINDOW_TITLE_COLOR );
   UG_TextboxCreate(&fileWindow, &fileTextBox, FILES_TEXTBOX_ID, 1, 1, 274, 80); 
   
   UG_TextboxSetAlignment(&fileWindow, FILES_TEXTBOX_ID, ALIGN_TOP_LEFT);
   UG_TextboxSetForeColor(&fileWindow, FILES_TEXTBOX_ID, C_BLACK);
   UG_TextboxSetFont ( &fileWindow , FILES_TEXTBOX_ID, &FONT_8X8 ) ;
   UG_TextboxShow(&fileWindow, FILES_TEXTBOX_ID);
   UG_WindowShow(&fileWindow);
   
   int keyNumPressed;
   
   if((D=_opendir("."))){

        
        fileCount = 0;
        // count how many files we have here
        for (_rewinddir(D); (DP=_readdir(D));){
            if (DP->d_type==DT_DIR || hasExt(DP->d_name, Ext)) {fileCount++;}
        }
       
        do {
            
            txtFilesPosition = txtFiles;
         
         // read a new Page
         if (switchedPage){
            // first, go to the position
            _rewinddir(D);
            for (s=0; s < position && (DP=_readdir(D)); s++){
                if (DP->d_type!=DT_DIR && !hasExt(DP->d_name, Ext)) {s--; continue;}
            }
            // than read the next FILES_MAX_ROWS files
            for(i=0;(i < FILES_MAX_ROWS && (DP=_readdir(D)));i++) {
               isDir[i] = false;
               if (DP->d_type==DT_DIR) isDir[i] = true;
               if (isDir[i] == false && !hasExt(DP->d_name, Ext)) {i--; continue;}
               strncpy(shownFiles[i],DP->d_name,FILES_MAX_LENGTH_NAME);
            }
            switchedPage = false;
            if (i == 0) {
                // nothing on this page, go to the first page, if we are not already
                if (position >= FILES_MAX_ROWS) {
                    position = 0;
                    selPosition = 0;
                    switchedPage = true;
                    continue;
                }
                
            }
        } 
       
        /// Draw the TextBox
        r = i;
        for(i=0;i<r;i++) {
           if (selPosition == i + position) *txtFilesPosition = 16;  else *txtFilesPosition = 0x20;
           txtFilesPosition++;
           if (isDir[i]) *txtFilesPosition = 0xfd; else   *txtFilesPosition = 0xfe;
           txtFilesPosition++;*txtFilesPosition = 0x20;txtFilesPosition++;
           strncpy(txtFilesPosition, shownFiles[i], FILES_MAX_LENGTH);
           if (strlen(shownFiles[i]) < FILES_MAX_LENGTH) txtFilesPosition += strlen(shownFiles[i]); else txtFilesPosition += FILES_MAX_LENGTH;
           *txtFilesPosition = '\n';
           txtFilesPosition++;
          
        }
        
        
        *txtFilesPosition = 0;
         
        UG_WindowShow(&fileWindow); // force a window update
        UG_TextboxSetText( &fileWindow , FILES_TEXTBOX_ID, txtFiles);
        
         UG_Update();
         DrawuGui(pixelBuffer, 0);
         keyNumPressed = odroidFmsxGUI_getKey_block();
         
         if (keyNumPressed == ODROID_INPUT_RIGHT) selPosition+= FILES_MAX_ROWS;
         if (keyNumPressed == ODROID_INPUT_LEFT)  selPosition-= FILES_MAX_ROWS;
         if (keyNumPressed == ODROID_INPUT_DOWN)  selPosition++;
         if (keyNumPressed == ODROID_INPUT_UP)    selPosition--;
         
         
         if (selPosition < 0) {
             // go to the last page
             selPosition = fileCount -1;
             position = FILES_MAX_ROWS * (int)(fileCount / FILES_MAX_ROWS);
             if (position == fileCount) position -= FILES_MAX_ROWS;
             switchedPage = true;
             
         }
         if (i != FILES_MAX_ROWS && selPosition >= position + i) { 
             // go to the first page
             selPosition = 0; position = 0;  switchedPage = true;
         }
         
         if (selPosition >= FILES_MAX_ROWS + position){ 
             // go to next page
             position += FILES_MAX_ROWS; selPosition = position;  switchedPage = true;
         }
         
         if (selPosition < position){ 
             // go to previous page
             position -= FILES_MAX_ROWS; 
             if (position < 0) position = 0;
             selPosition = position + FILES_MAX_ROWS - 1; 
             switchedPage = true;
         }
         if (selPosition - position >= 0 && selPosition - position < FILES_MAX_ROWS && keyNumPressed == ODROID_INPUT_A && isDir[selPosition - position]) {
            free(txtFiles);
            free(Buf);
            UG_TextboxDelete(&fileWindow, FILES_TEXTBOX_ID);
            UG_WindowDelete(&fileWindow);
            chdir(shownFiles[selPosition - position]);
            selPosition = 0;
            for (int i = 0; i < FILES_MAX_ROWS; i++) free(shownFiles[i]);
            closedir(D);
            char* cDir = malloc(1024);
            getcwd(cDir, 1024);
            ini_puts("C64", "LASTDIR", cDir, FRODO_CONFIG_FILE);
            free(cDir);
            return odroidFmsxGUI_chooseFile(Ext);
         }
        if(keyNumPressed == ODROID_INPUT_B) {
            free(txtFiles);
            free(Buf);
            UG_TextboxDelete(&fileWindow, FILES_TEXTBOX_ID);
            UG_WindowDelete(&fileWindow);
            chdir("..");
            selPosition = 0;
            for (int i = 0; i < FILES_MAX_ROWS; i++) free(shownFiles[i]);
            closedir(D);
            char* cDir = malloc(1024);
            getcwd(cDir, 1024);
            ini_puts("C64", "LASTDIR", cDir, FRODO_CONFIG_FILE);
            free(cDir);
            return odroidFmsxGUI_chooseFile(Ext);
            
        }
         

        } while(keyNumPressed != ODROID_INPUT_A && keyNumPressed != ODROID_INPUT_MENU);
        
        
        closedir(D);
    }
   if(keyNumPressed == ODROID_INPUT_A) {
       strncpy(selectedFile, shownFiles[selPosition - position], FILES_MAX_LENGTH_NAME);
   }
   
   free(txtFiles);
   free(Buf);
   for (int i = 0; i < FILES_MAX_ROWS; i++) free(shownFiles[i]);
   UG_TextboxDelete(&fileWindow, FILES_TEXTBOX_ID);
   UG_WindowDelete(&fileWindow);
   
   if(keyNumPressed == ODROID_INPUT_A) {
            return selectedFile;
   }
   return NULL;
}
char* odroidFmsxGUI_insertVaribaleMenuInfo(char* position, int item) {
    if (item == MENU_ITEM_1541) {
        char emu1541 = C64_is1541emluation();
        if (emu1541) memcpy(position, " (ON) ", 6);  else memcpy(position, " (OFF)", 6);
        position+= 6;
    }
    if (item == MENU_ITEM_AUDIO) {
        ODROID_AUDIO_SINK sink = (ODROID_AUDIO_SINK)ini_getl("C64", "DAC", ODROID_AUDIO_SINK_SPEAKER, FRODO_CONFIG_FILE);
        if (sink == ODROID_AUDIO_SINK_SPEAKER) memcpy(position, " (SPCK)", 7);  else memcpy(position, " (DAC) ", 7);
        position+= 7;
    }
    if (item == MENU_ITEM_SWITCHJOY) {
        bool sw = C64_getJoystickSwap();
        if (sw) memcpy(position, " (Port 2)", 9);  else memcpy(position, " (Port 1)", 9);
        position+= 9;
    }
    
            
    *position = '\n'; position++;
    return position;
}
void odroidFmsxGUI_selectMenuItem(int item) {
    
    char* menuPos = menuTxt;
    int len = 0;
    for (int i = 0; i < MENU_ITEMS; i++) {
        if (item == i) *menuPos = 16;  else *menuPos = 0x20;
        menuPos++;*menuPos = 0x20;menuPos++;
        len = strlen(menuItems.menuItem[i]);
        memcpy(menuPos, menuItems.menuItem[i], len);
        menuPos += len;
        ////////// write som extra infos in the menu //////////
        menuPos =odroidFmsxGUI_insertVaribaleMenuInfo(menuPos, i);
        
    }
    
    *menuPos = 0;
    UG_TextboxSetText( &window , MENU_TEXTBOX_ID, menuTxt);
    UG_TextboxSetAlignment(&window , MENU_TEXTBOX_ID,ALIGN_H_LEFT|ALIGN_V_CENTER);
}

void odroidFrodoGUI_setLastLoadedFile(const char* file) {
    if (file != NULL)
        strncpy(lastOpenedFileFullPath, file, 1024);
    else 
        lastOpenedFileFullPath[0] = 0;
}

// msg Box: max 33 letters in one row!

void odroidFrodoGUI_msgBox(const char* title, const char* msg, char waitKey) {
   int rows = 1;
   const char* p = msg;
   while(*p++ != 0){
       if (*p == '\n') rows++;
   }
   
   // if there is a old instance
   UG_TextboxDelete(&msgWindow, MSG_TEXTBOX_ID);
   UG_WindowDelete(&msgWindow);
   
   
   UG_WindowCreate ( &msgWindow , obj_buff_wnd_file , MAX_OBJECTS, window_callback);
   UG_WindowResize(&msgWindow, 20, 20, 300, 54 + (rows*8));
  
   
   
   UG_WindowSetTitleTextFont ( &msgWindow , &FONT_8X8 ) ;
   UG_WindowSetTitleText ( &msgWindow , title ) ;
   UG_WindowSetBackColor( &msgWindow , WINDOW_BG_COLOR );
   UG_WindowSetTitleColor( &msgWindow , WINDOW_TITLE_COLOR );
   
   UG_TextboxCreate(&msgWindow, &msgTextBox, MSG_TEXTBOX_ID, 6, 6, 274, 12+rows*8); 
   
   UG_TextboxSetAlignment(&msgWindow, MSG_TEXTBOX_ID, ALIGN_TOP_LEFT);
   UG_TextboxSetForeColor(&msgWindow, MSG_TEXTBOX_ID, C_BLACK);
   UG_TextboxSetFont ( &msgWindow , MSG_TEXTBOX_ID, &FONT_8X8 ) ;
   UG_TextboxShow(&msgWindow, MSG_TEXTBOX_ID);
   UG_WindowShow(&msgWindow);
   
   UG_TextboxSetText( &msgWindow , MSG_TEXTBOX_ID, msg);
   
   
   UG_Update();
   DrawuGui(pixelBuffer, 0);
       
   if (waitKey && odroidFmsxGUI_getKey_block_lock()){}
   
  
   
}
char saveState(const char* fileName) {
    char res = 1;
    if (! fileName) {
        char* stateFileName = (char*)heap_caps_malloc(1024, MALLOC_CAP_SPIRAM);
        char* stateFileNameF = (char*)heap_caps_malloc(1024, MALLOC_CAP_SPIRAM);
        strncpy(stateFileName, lastOpenedFileFullPath, 1024);
        cutExtension(stateFileName);
        snprintf(stateFileNameF, 1024, "%s.sta", stateFileName);
        if (!C64_SaveSnapshot(stateFileNameF)){
            res = 0;
        }
        free(stateFileName);
        free(stateFileNameF);
    } else {
        C64_SaveSnapshot(fileName);
    }
    return res;
}
char loadState(const char* fileName) {
    char res = 1;
    printf("loading state...\n");
    if (! fileName) {
        char* stateFileName = (char*)heap_caps_malloc(1024, MALLOC_CAP_SPIRAM);
        char* stateFileNameF = (char*)heap_caps_malloc(1024, MALLOC_CAP_SPIRAM);
        strncpy(stateFileName, lastOpenedFileFullPath, 1024);
        cutExtension(stateFileName);
        snprintf(stateFileNameF, 1024, "%s.sta", stateFileName);
        printf("file: %s\n", stateFileNameF);
        if (! fileExist(stateFileNameF))
            res = 0;
        else
            if (!C64_LoadSnapshot(stateFileNameF))   res = 0;
        
        
         printf("result: %d\n", res);   
        free(stateFileName);
        free(stateFileNameF);
    } else {
        if (! fileExist(fileName))
            res = 0;
        else
           if (! C64_LoadSnapshot(fileName)) res = 0;
        
    }
    return res;
}

MENU_ACTION odroidFrodoGUI_showMenu() {
   
   char stopMenu = 0;
   odroidFmsxGUI_selectMenuItem(currentSelectedItem);
   MENU_ACTION ret = MENU_ACTION_NONE;
   
   
   
   // wait until the menu button is not pressed anymore
   int keyPressed;
   do {
       keyPressed = odroidFrodoGUI_getKey();
   }while (keyPressed == ODROID_INPUT_MENU);
   
  
   /// now listen for another button

    do {
       int c = 0;
       
       
       // redraw menu
       odroidFmsxGUI_selectMenuItem(currentSelectedItem);
       ////////////////
       
       
       UG_WindowShow(&window); // force a window update
       UG_Update();
       DrawuGui(pixelBuffer, 0);
      
       keyPressed = odroidFmsxGUI_getKey_block();
       if (keyPressed == ODROID_INPUT_DOWN) c = 1;
       if (keyPressed == ODROID_INPUT_UP) c = -1;
       
       currentSelectedItem += c;
       
       if (currentSelectedItem < 0)currentSelectedItem = MENU_ITEMS - 1;
       if (currentSelectedItem >= MENU_ITEMS)currentSelectedItem = 0;
       
       if (menuItems.menuItem[currentSelectedItem][0] == 0) currentSelectedItem += c;
           
           
       if (lastSelectedItem != currentSelectedItem) {
           lastSelectedItem = currentSelectedItem;
           odroidFmsxGUI_selectMenuItem(currentSelectedItem);
       }
       
       if (keyPressed == ODROID_INPUT_A) {
           const char* lastSelectedFile = NULL;
           printf("selected item: %d\n", currentSelectedItem);
           
           switch(currentSelectedItem){
               ///////////// Load File ///////////////////
               case MENU_ITEM_LOADFILE:   
                   lastSelectedFile = odroidFmsxGUI_chooseFile(".d64\0\0"); 
                   if (lastSelectedFile != NULL) {
                       odroidFrodoGUI_msgBox("Loading...", "Please wait while loading", 0);
                       char* fullPath = (char*)malloc(1024);
                       getFullPath(fullPath, lastSelectedFile, 1024);
                       C64_InsertDisc(8, fullPath);
                       
                       strncpy(lastOpenedFileFullPath, fullPath, 1024);
                       ini_puts("C64", "LASTGAME", lastOpenedFileFullPath, FRODO_CONFIG_FILE);
                       if (loadState(NULL)) {
                            C64_setStdKeymapping();
                            loadKeyMappingFromGame(lastSelectedFile);
                       } else {
                           if (c64_isNAVRunning()) reloadDiskNAV(8); else  esp_restart();
                       }
                      
                       
                       
                       
                       free(fullPath);
                       stopMenu = true;
                       
                   }
                   break; 
                   
                //////////////// Save State ////////////////////
               case MENU_ITEM_SAVEFILE:
                   if (lastOpenedFileFullPath[0] != 0) {
                        odroidFrodoGUI_msgBox("Please wait...", "Please wait while saving", 0);
                        if (!saveState(NULL)) {
                            odroidFrodoGUI_msgBox("Error", "Could not save state", 1);
                        }
                   } else {
                       odroidFrodoGUI_msgBox("Save state", "You didn't load a game, so you\ncan't save the state.", 1);
                   }
                   stopMenu = true;
                   break;
               /////////////// delete state //////////////////  
               case MENU_ITEM_DELETEFILE:
                    {
                        const char* delFile = odroidFmsxGUI_chooseFile(".sta\0\0"); 
                        if (delFile) {
                             char* fullPath = (char*)malloc(1024);
                             getFullPath(fullPath, delFile, 1024);
                            _remove(fullPath);
                            free(fullPath);
                        }
                       
                    }
                   break;
                //////////////// change Disk ////////////////////
               case MENU_ITEM_CHANGEDISK:
               
                   if (c64_isNAVRunning()) {
                        odroidFrodoGUI_msgBox("Change disc", "NAV is running. \nPlease use \"Load File\"", 1);
                        break;
                   }
                   char* file = odroidFmsxGUI_chooseFile(".d64\0\0"); 
                   if (file != NULL) {
                       odroidFrodoGUI_msgBox("Loading...", "Please wait while loading", 0);
                       char* fullPath = (char*)malloc(1024);
                       getFullPath(fullPath, file, 1024);
                       C64_InsertDisc(8, fullPath);
                       free(fullPath);
                       stopMenu = true;
                       
                   }
               
                   break;  
                //////////////// Eject Disks ////////////////////
               case MENU_ITEM_EJECTDISK:
                  
                   C64_CloseAllChannels();
                  ini_puts("C64", "LASTGAME", "", FRODO_CONFIG_FILE);
                  stopMenu = true;
                  break;
                //// switch joystick port
               case MENU_ITEM_SWITCHJOY:
                  if (C64_SwitchJoystickPort())
                      odroidFrodoGUI_msgBox("Joystick", "Joystick is now on Port 2", 1);
                  else
                      odroidFrodoGUI_msgBox("Joystick", "Joystick is now on Port 1", 1);
                  break;
                  
                //// 1541 emulation
               case MENU_ITEM_1541:
               {
                  char emu1541 = C64_is1541emluation();
                  if (! emu1541){
                      odroidFrodoGUI_msgBox("1541", "Please wait while activating 1541", 0); 
                      C64_1541emluation(1);
                      odroidFrodoGUI_msgBox("1541 Emulation", "The 1541 Emulation is now on", 1);
                  }
                  else{
                      C64_1541emluation(0);
                      odroidFrodoGUI_msgBox("1541 Emulation", "The 1541 Emulation is now off", 1);
                      
                  }
                  
               }
                  break;
                ////////// reset c64  to basic 
               case MENU_ITEM_RESETBASIC:
                   C64_Reset();
                   stopMenu = true;
                   break;
               ////////// reset c64  to NAV (delete last game and reboot) 
               case MENU_ITEM_RESETNAV:
                   ini_puts("C64", "LASTGAME", "", FRODO_CONFIG_FILE);
                   esp_restart();
                   stopMenu = true;
                   break;
               case MENU_ITEM_AUDIO:
               {
                   ODROID_AUDIO_SINK sink = (ODROID_AUDIO_SINK)ini_getl("C64", "DAC", ODROID_AUDIO_SINK_SPEAKER, FRODO_CONFIG_FILE);
                   if (sink == ODROID_AUDIO_SINK_SPEAKER){
                       ini_putl("C64", "DAC", ODROID_AUDIO_SINK_DAC, FRODO_CONFIG_FILE);
                       odroidFrodoGUI_msgBox("Audio", " Audio is now: DAC\n\nPlease turn the Device OFF and\nON again to take effect.", 1);
                   }else {
                       ini_putl("C64", "DAC", ODROID_AUDIO_SINK_SPEAKER, FRODO_CONFIG_FILE);
                       odroidFrodoGUI_msgBox("Audio", " Audio is now: Speaker\n\nPlease turn the Device OFF and\nON again to take effect.", 1);
                   }
                   ret = MENU_ACTION_CHANGED_AUDIO_TYPE;
               }
               
               break;
#ifdef WITH_WLAN                   
               case MENU_ITEM_MULTISERVER:
                   lastSelectedFile = odroidFmsxGUI_chooseFile(".d64\0\0"); 
                   if (lastSelectedFile != NULL) {
                       getFullPath(lastOpenedFileFullPath, lastSelectedFile, 1024);
                       odroid_settings_RomFilePath_set(lastOpenedFileFullPath);
                       odroid_settings_WLAN_set(ODROID_WLAN_AP);
                       fflush(stdout);
                       esp_restart();
                   }
                   
                   stopMenu = true;
                   break;
               case MENU_ITEM_MULTICLIENT:
                   odroidFrodoGUI_msgBox("Multiplayer", " Please wait...", 0);
                   odroid_settings_WLAN_set(ODROID_WLAN_STA);
                   fflush(stdout);
                   esp_restart();
                   stopMenu = true;
                   break;
#endif                   

               case MENU_ITEM_ABOUT:
                  odroidFrodoGUI_msgBox("About", " \nFrodo\n\n by Christian Bauer\n\n ported by Jan P. Schuemann\n\nThanks to the ODROID-GO community\n\nHave fun!\n ", 1);
               break;
           };
       }
       if (keyPressed == ODROID_INPUT_MENU) stopMenu = true;
       
       
   }while (!stopMenu);
   
   return ret;
    
}

