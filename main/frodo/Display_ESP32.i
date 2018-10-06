/*
 * Display_Acorn.i
 *
 * Handles redraws and suchlike as well as the keyboard
 * Frodo (C) 1994-1997 by Christian Bauer
 * Acorn port by Andreas Dehmel, 1997
 *
 */


#include "C64.h"


#include "SAM.h"
#include "VIC.h"

#include "stdbool.h"

#include "odroid_display.h"
#include "odroid_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "LibOdroidGo.h"
#include "keyboard.h"
#include "Prefs.h"
#include "SID.h"
#include "IEC.h"

#include "LibOdroidGo.h"
#include "Display.h"



#define ILI9341_COLOR(r, g, b)\
	(((uint16_t)b) >> 3) |\
	       ((((uint16_t)g) << 3) & 0x07E0) |\
	       ((((uint16_t)r) << 8) & 0xf800)


#define IntKey_MinCode   3	// Scan from ShiftLeft (leave out Shift, Ctrl, Alt)
#define IntKey_MaxCode   124
#define IntKey_Copy	105

typedef uint8_t pixel;




#define HEIGHT_DISPLAY 240
#define WIDTH_DISPLAY 320

/////////// Video Stuff /////////
QueueHandle_t videoQueue;
uint16_t VideoTaskCommand = 1;
uint16_t* ili9341_palette;
pixel* framebuffer[2];
pixel* writeBuffer;
pixel* sendBuffer;
///////////////////////////////
int startCounter = 0;
///////// virtual keyboard stuff
#define CURSOR_MAX_WIDTH (22)
#define CURSOR_MAX_HEIGHT (22)
int keyPressA = -1;
int keyPressB = -1;
char showKeyboard = 0;
char reDrawKeyboard = 0;
char reDrawCursor = 0;
char flipScreen = 0;

const char* writeKeys = NULL;

int cursorX = 10;
int cursorY = 10;
uint16_t* cursor;
uint16_t* cursorBackGround;
int backGroundX = -1;
int backGroundY = -1;
int backGroundHeight = -1;
int backGroundWidth = -1;
int keyPress = 0;


char navRunning = 1;

#define KEYS_COUNT 66
struct keyPosition{
    uint16_t x1; 
    uint16_t y1; 
    uint16_t x2; 
    uint16_t y2;
    uint16_t key;
};

uint16_t keyMapping[ODROID_INPUT_MAX];

static const struct {
    uint8_t keys;
    struct keyPosition kyPos[KEYS_COUNT];
    
} keyPositions = {
    KEYS_COUNT,
   //row 1, 17keys
    0xc,11,0x1a,25,KEY_BAK,
    0x1c,11,0x2a,25,'1',
    0x2c,11,0x3a,25,'2',
    0x3c,11,0x4a,25,'3',
    0x4c,11,0x5a,25,'4',
    0x5c,11,0x6a,25,'5',
    0x6c,11,0x7a,25,'6',
    0x7c,11,0x8a,25,'7',
    0x8c,11,0x9a,25,'8',
    0x9c,11,0xaa,25,'9',
    0xac,11,0xba,25,'0',
    0xbc,11,0xca,25,'+',
    0xcc,11,0xda,25,'-',
    0xdc,11,0xea,25,KEY_POUND,
    0xec,11,0xfa,25,KEY_CLR,
    0xfc,11,0x10a,25,KEY_DEL,
    0x117,11,0x12d,25,KEY_F1,
    
    //row 2, 15keys + 1 spezial
    0xc,27,0x22,41,KEY_CTL,
    0x24,27,0x32,41,'Q',
    0x34,27,0x42,41,'W',
    0x44,27,0x52,41,'E',
    0x54,27,0x62,41,'R',
    0x64,27,0x72,41,'T',
    0x74,27,0x82,41,'Y',
    0x84,27,0x92,41,'U',
    0x94,27,0xa2,41,'I',
    0xa4,27,0xb2,41,'O',
    0xb4,27,0xc2,41,'P',
    0xc4,27,0xd2,41,'@',
    0xd4,27,0xe2,41,'*',
    0xe4,27,0xf2,41,'^',
    0xf4,27,0x10a,41,KEY_RESTORE, // spezial key, directly connected to the NMI 
    0x117,27,0x12d,41,KEY_F3,
    
    //row 3, 15 keys + 1 spezial
    0xc,43,0x1a,57,KEY_R_S,
    0x1c,43,0x2a,57,KEY_SLO, // shift lock (spezial key)
    0x2c,43,0x3a,57,'A',
    0x3c,43,0x4a,57,'S',
    0x4c,43,0x5a,57,'D',
    0x5c,43,0x6a,57,'F',
    0x6c,43,0x7a,57,'G',
    0x7c,43,0x8a,57,'H',
    0x8c,43,0x9a,57,'J',
    0x9c,43,0xaa,57,'K',
    0xac,43,0xba,57,'L',
    0xbc,43,0xca,57,':',
    0xcc,43,0xda,57,';',
    0xdc,43,0xea,57,'=',
    0xec,43,0x10a,57,'\n',
    0x117,43,0x12d,57,KEY_F5,
    
    //row 4, 16keys
    0xc,59,0x1a,73,KEY_COMM,
    0x1c,59,0x32,73,KEY_SHL,
    0x34,59,0x42,73,'Z',
    0x44,59,0x52,73,'X',
    0x54,59,0x62,73,'C',
    0x64,59,0x72,73,'V',
    0x74,59,0x82,73,'B',
    0x84,59,0x92,73,'N',
    0x94,59,0xa2,73,'M',
    0xa4,59,0xb2,73,',',
    0xb4,59,0xc2,73,'.',
    0xc4,59,0xd2,73,'/',
    0xd4,59,0xea,73,KEY_SHR,
    0xec,59,0xfa,73,KEY_CUD,
    0xfc,59,0x10a,73,KEY_CLR,
    0x117,59,0x12d,73,KEY_F7,
    
    //row 5, 1key
    59,75,203,89,' '
    
    
};
// I do this load of a (hopefully) not existing file from LW8 to switch to it as standard LW. Do anyone knows a better way? 
//const char* C64Display::toWrite = "LOAD \"NAV96\",11\nLOAD \"...\",8\nRUN\n";   
/*
 some tests
 * //const char* C64Display::toWrite = "LOAD \"$\",11\nLIST\n~HSH~CUD~CUD~CUD~CUD~HSHLOAD~CLR~CLR~CLR~CLR~CLR~CLR~CLR~CLR~CLR~CLR~CLR~CLR~CLR~CLR~CLR~CLR~CLR~CLR~CLR,11   \nRUN\n";
 //const char* C64Display::toWrite = "LOAD \"$\",8\nLIST\n~HSH~CUD~CUD~CUD~CUD~CUD~CUD~CUD~CUD~CUD~CUD~CUD~CUD~CUD~CUD~CUD~CUD~CUD~CUD~HSHLOAD~CLR~CLR~CLR~CLR~CLR~CLR~CLR~CLR~CLR~CLR~CLR~CLR~CLR~CLR~CLR~CLR~CLR~CLR~CLR,8   \nRUN                             \n";
    
 */
////////////////////////

enum cursorTypeEnum{
    CURSOR_ARROW,
    CURSOR_HAND
};
enum cursorTypeEnum cursorType = CURSOR_ARROW;


void C64Display::videoTask(void* arg)
{
  
  uint16_t* param;
  while(1)
  {
    xQueuePeek(videoQueue, &param, portMAX_DELAY);
    VideoTaskCommand = 0;
    //copy buffer to slower spi, so we have the fast DMA buffer agin for the next frame. It's fast enough for this thread. With WIFI I have not enough space for two DMA buffers.
    // ( I wonder, why a relative slow wifi connections needs so much DMA buffer...)
    memcpy(sendBuffer, writeBuffer, DISPLAY_X * DISPLAY_Y*sizeof(pixel));
    
    ili9341_write_frame_C64((uint8_t*)sendBuffer, ili9341_palette, showKeyboard, flipScreen);
    
    
    if (showKeyboard) {
        if (reDrawKeyboard || reDrawCursor) {
            uint16_t* cp;
            uint16_t* bg;    

            const uint16_t* keyb = (uint16_t*)(c64_keyboard.pixel_data + c64_keyboard.width*2);
            const uint16_t* op;
            int imageWidth;
            int imageHeight;

            if (cursorType == CURSOR_ARROW){
                op = (uint16_t*)cursor_image.pixel_data;
                imageWidth = cursor_image.width;
                imageHeight = cursor_image.height;
            } else{
                op = (uint16_t*)hand_image.pixel_data;
                imageWidth = hand_image.width;
                imageHeight = hand_image.height;

            }


            int cursorWidth = imageWidth;
            int cursorHeight = imageHeight;
            if (cursorHeight + cursorY > c64_keyboard.height - 1) cursorHeight = (c64_keyboard.height - cursorY) - 1;
            // recover background
            if (backGroundX != -1){
                ili9341_write_frame_rectangleLE(backGroundX, (HEIGHT_DISPLAY - (c64_keyboard.height - 1)) + backGroundY, backGroundWidth, backGroundHeight, (uint16_t*)cursorBackGround);
            }
            // copy backgroud in cursor and background
            cp = cursor;
            bg = cursorBackGround;
            for (int y = 0; y < cursorHeight; y++) {
                for (int x = 0; x < cursorWidth; x++) {
                    uint16_t p = keyb[(cursorX+x) + ((cursorY + y)*c64_keyboard.width)];
                    *cp++ = p;
                    *bg++ = p;
                }
            }


            backGroundX = cursorX;
            backGroundY = cursorY;
            backGroundWidth = cursorWidth;
            backGroundHeight = cursorHeight;

            cp = cursor;
            for (int y = 0; y < imageHeight; y++) {
                for (int x = 0; x < imageWidth; x++) {
                    uint16_t p = *op++;
                    if (p != 0xFFFF && y > 0) *cp++ = p; else cp++; // TODO: had a problem with gimp: the first row is not right, don't know why.
                }
            }
            // TODO: the same in my keyboard image. the first row is crap, so i down't draw it. have to find out why GIMP's first line is crap...
            if (reDrawKeyboard) ili9341_write_frame_rectangleLE(0, (HEIGHT_DISPLAY - (c64_keyboard.height - 1)), c64_keyboard.width, c64_keyboard.height - 1, (uint16_t*)(c64_keyboard.pixel_data + c64_keyboard.width*2));
            ili9341_write_frame_rectangleLE(cursorX, (HEIGHT_DISPLAY - (c64_keyboard.height - 1)) + cursorY, cursorWidth, cursorHeight, (uint16_t*)cursor);
            reDrawKeyboard = 0;
            reDrawCursor = 0;

    
        }
    }
     
     
    
    xQueueReceive(videoQueue, &param, portMAX_DELAY);
  }

  

  vTaskDelete(NULL);

  while (1) {}
}

C64Display::C64Display(C64 *the_c64) : TheC64(the_c64)
{

    ili9341_palette = (uint16*)heap_caps_malloc(256*sizeof(uint16), MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    
    for (int i = 0; i < 2; i++) {
        
        if (i == 0) framebuffer[i] = (pixel*)heap_caps_malloc(DISPLAY_X * DISPLAY_Y*sizeof(pixel), MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
        if (i == 1)  framebuffer[i] = (pixel*)heap_caps_malloc(DISPLAY_X * DISPLAY_Y*sizeof(pixel), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        
      
        if (!framebuffer[i]){ printf("malloc framebuffer%d failed!\n", i); return; }
        memset(framebuffer[i], 0, DISPLAY_X * DISPLAY_Y*sizeof(pixel));
        printf("malloc framebuffer%d OKAY!\n", i);
    }
    
    writeBuffer = framebuffer[0];
    sendBuffer = framebuffer[1];
    
    cursor = (uint16_t*)heap_caps_malloc(CURSOR_MAX_WIDTH*CURSOR_MAX_HEIGHT*2, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    if (!cursor){ printf("malloc cursor failed!\n"); return; }
    
    cursorBackGround= (uint16_t*)heap_caps_malloc(CURSOR_MAX_WIDTH*CURSOR_MAX_HEIGHT*2, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    if (!cursorBackGround){ printf("malloc cursorBackGround failed!\n"); return; }
    
    //printf("bufmem: %p\n", bufmem);
    videoQueue = xQueueCreate(1, sizeof(uint16_t*));
    xTaskCreatePinnedToCore(&videoTask, "videoTask", 2048, NULL, 5, NULL, 1);
  

    keyPressed = 0;
    writePosition = 0;
    holdShift = 0;
    runningStartSeqence = 0;
    
    
    /// initial keymapping ///
    setStdKeymapping();
}


C64Display::~C64Display(void)
{
  free(ili9341_palette);
  for (int i = 0; i < 2; i++) {
      free(framebuffer[i]);
  }
  free(cursor);
  free(cursorBackGround);
}


void C64Display::Speedometer(int speed) {
    printf("speed: %d\n", speed);
}

uint8 *C64Display::BitmapBase(void)
{
    //printf("BitmapBase\n");
    return (uint8 *)writeBuffer;
}


void C64Display::InitColors(uint8 *colors)
{
   // printf("init colors\n");
    int i;
    
    for (i=0; i< 256; i++) {
        ili9341_palette[i] = ILI9341_COLOR(palette_red[i & 0x0f], palette_green[i & 0x0f], palette_blue[i & 0x0f]);
        ili9341_palette[i]=( ili9341_palette[i] << 8) | (( ili9341_palette[i] >> 8) & 0xFF);
        colors[i] = i;
    }
        
}


int C64Display::BitmapXMod(void)
{
  return DISPLAY_X;
}


// This routine reads the raw keyboard data from the host machine. Not entirely
// conformant with Acorn's rules but the only way to detect multiple simultaneous
// keypresses.

// (from Display_x.i)
/*
  C64 keyboard matrix:

    Bit 7   6   5   4   3   2   1   0
  0    CUD  F5  F3  F1  F7 CLR RET DEL
  1    SHL  E   S   Z   4   A   W   3
  2     X   T   F   C   6   D   R   5
  3     V   U   H   B   8   G   Y   7
  4     N   O   K   M   0   J   I   9
  5     ,   @   :   .   -   L   P   +
  6     /   ^   =  SHR HOM  ;   *   £
  7    R/S  Q   C= SPC  2  CTL  <-  1
*/


void C64Display::getBitByteKey(char* bit, char* byte, char key){
    if (key == KEY_CUD || key == KEY_F5 || key == KEY_F3 ||key == KEY_F1 ||key == KEY_F7 ||key == KEY_CLR ||key == '\n' ||key == KEY_DEL)  *byte = 0;
    if (key == KEY_SHL || key == 'E' || key == 'S' ||key == 'Z' ||key == '4' ||key == 'A' ||key == 'W' ||key == '3')  *byte = 1;
    if (key == 'X' || key == 'T' || key == 'F' ||key == 'C' ||key == '6' ||key == 'D' ||key == 'R' ||key == '5')  *byte = 2;
    if (key == 'V' || key == 'U' || key == 'H' ||key == 'B' ||key == '8' ||key == 'G' ||key == 'Y' ||key == '7')  *byte = 3;
    if (key == 'N' || key == 'O' || key == 'K' ||key == 'M' ||key == '0' ||key == 'J' ||key == 'I' ||key == '9')  *byte = 4;
    if (key == ',' || key == '@' || key == ':' ||key == '.' ||key == '-' ||key == 'L' ||key == 'P' ||key == '+')  *byte = 5;
    if (key == '/' || key == '^' || key == '=' ||key == KEY_SHR ||key == KEY_HOM ||key == ';' ||key == '*' ||key == KEY_POUND)  *byte = 6;
    if (key == KEY_R_S || key == 'Q' || key == KEY_COMM ||key == ' ' ||key == '2' ||key == KEY_CTL ||key == KEY_BAK ||key == '1')  *byte = 7;
    
    if (key == KEY_CUD || key == KEY_SHL || key == 'X' || key == 'V' || key == 'N' || key == ',' || key == '/' || key == KEY_R_S ) *bit = 7;
    if (key == KEY_F5 || key == 'E' || key == 'T' || key == 'U' || key == 'O' || key == '@' || key == '^' || key == 'Q' ) *bit = 6;
    if (key == KEY_F3 || key == 'S' || key == 'F' || key == 'H' || key == 'K' || key == ':' || key == '=' || key == KEY_COMM ) *bit = 5;
    if (key == KEY_F1 || key == 'Z' || key == 'C' || key == 'B' || key == 'M' || key == '.' || key == KEY_SHR || key == ' ' ) *bit = 4;
    if (key == KEY_F7 || key == '4' || key == '6' || key == '8' || key == '0' || key == '-' || key == KEY_HOM || key == '2' ) *bit = 3;
    if (key == KEY_CLR || key == 'A' || key == 'D' || key == 'G' || key == 'J' || key == 'L' || key == ';' || key == KEY_CTL ) *bit = 2;
    if (key == '\n' || key == 'W' || key == 'R' || key == 'Y' || key == 'I' || key == 'P' || key == '*' || key == KEY_BAK ) *bit = 1;
    if (key == KEY_DEL || key == '3' || key == '5' || key == '7' || key == '9' || key == '+' || key == KEY_POUND || key == '1' ) *bit = 0;
    
}
void C64Display::pressKey(uint8 *key_matrix, uint8 *rev_matrix, char key) {
    char c64_byte = 99;
    char c64_bit = 99;
    if (key == KEY_SLO) if (! holdShift)  holdShift = 1;  else holdShift = 2;
    if (key == KEY_RESTORE) TheC64->NMI();
    
    getBitByteKey(&c64_bit, &c64_byte, key);
    
    if (c64_bit < 8 && c64_byte < 8) {
        key_matrix[c64_byte] &= ~(1 << c64_bit);
        rev_matrix[c64_bit] &= ~(1 << c64_byte);
    }
   
}
void C64Display::unpressKey(uint8 *key_matrix, uint8 *rev_matrix, char key) {
    char c64_byte = 99;
    char c64_bit = 99;
    getBitByteKey(&c64_bit, &c64_byte, key);
    if (c64_bit < 8 && c64_byte < 8) {
        key_matrix[c64_byte] |= (1 << c64_bit);
	rev_matrix[c64_bit] |= (1 << c64_byte);
    }
}
void C64Display::sendKeys(const char* keys) {
    writePosition = 0;
    writeKeys = keys;
    
}
void C64Display::writeSomeKeys(uint8 *key_matrix, uint8 *rev_matrix) {
    if (writeKeys == NULL) return;
    
    if (runningStartSeqence == 0)    runningStartSeqence = 1;
   
    
    if (runningStartSeqence){
        runningStartSeqence++;
        if (runningStartSeqence == 6) {
            switch(writeKeys[writePosition]) {
                case '~':
                    if (!strncmp(&writeKeys[writePosition+1], "HSH", 3))  pressKey(key_matrix, rev_matrix, KEY_SLO); 
                    if (!strncmp(&writeKeys[writePosition+1], "CUD", 3))  pressKey(key_matrix, rev_matrix, KEY_CUD); 
                    if (!strncmp(&writeKeys[writePosition+1], "KF5", 3))  pressKey(key_matrix, rev_matrix, KEY_F5); 
                    if (!strncmp(&writeKeys[writePosition+1], "KF3", 3))  pressKey(key_matrix, rev_matrix, KEY_F3); 
                    if (!strncmp(&writeKeys[writePosition+1], "KF1", 3))  pressKey(key_matrix, rev_matrix, KEY_F1); 
                    if (!strncmp(&writeKeys[writePosition+1], "KF7", 3))  pressKey(key_matrix, rev_matrix, KEY_F7); 
                    if (!strncmp(&writeKeys[writePosition+1], "CLR", 3))  pressKey(key_matrix, rev_matrix, KEY_CLR); 
                    if (!strncmp(&writeKeys[writePosition+1], "DEL", 3))  pressKey(key_matrix, rev_matrix, KEY_DEL); 
                    if (!strncmp(&writeKeys[writePosition+1], "CLR", 3))  pressKey(key_matrix, rev_matrix, KEY_CLR); 
                    if (!strncmp(&writeKeys[writePosition+1], "SHR", 3))  pressKey(key_matrix, rev_matrix, KEY_SHR); 
                    if (!strncmp(&writeKeys[writePosition+1], "HOM", 3))  pressKey(key_matrix, rev_matrix, KEY_HOM); 
                    if (!strncmp(&writeKeys[writePosition+1], "R_S", 3))  pressKey(key_matrix, rev_matrix, KEY_R_S); 
                    if (!strncmp(&writeKeys[writePosition+1], "COM", 3))  pressKey(key_matrix, rev_matrix, KEY_COMM); 
                    if (!strncmp(&writeKeys[writePosition+1], "CTL", 3))  pressKey(key_matrix, rev_matrix, KEY_CTL); 
                    if (!strncmp(&writeKeys[writePosition+1], "BAK", 3))  pressKey(key_matrix, rev_matrix, KEY_BAK); 
                    if (!strncmp(&writeKeys[writePosition+1], "POU", 3))  pressKey(key_matrix, rev_matrix, KEY_POUND); 
                    
                    writePosition+= 3;
                    break;
                case '"':
                    pressKey(key_matrix, rev_matrix, KEY_SHL);
                    pressKey(key_matrix, rev_matrix, '2');
                    break;
                case '$':
                    pressKey(key_matrix, rev_matrix, KEY_SHL);
                    pressKey(key_matrix, rev_matrix, '4');
                    break;
                default:
                    pressKey(key_matrix, rev_matrix, writeKeys[writePosition]);
            }

        
        }
    
        if (runningStartSeqence == 12){
            runningStartSeqence = 1;
            // remove all Keys
            for (int i = 0; i < 8; i++) {key_matrix[i] = 0xff; rev_matrix[i] = 0xff;}
            if (holdShift) pressKey(key_matrix, rev_matrix, KEY_SHL);
            keyPressed = 0;
            writePosition++;
            if (!writeKeys[writePosition]){ runningStartSeqence = 0; writeKeys = NULL;}
        }
    }
}


/* joystick:
up: ‭   11111110‬
down:  ‭11111101‬
left: ‭ 11111011‬
right: ‭11110111‬
button:‭11101111‬
 */
void C64Display::fastMode(bool on)
{
    if (on) {
        ThePrefs.SkipFrames = 10;
        ThePrefs.LimitSpeed = false;
    } else {
        ThePrefs.SkipFrames = 2;
        ThePrefs.LimitSpeed = true;
    }
}
void C64Display::pressMappedKey(uint8 *key_matrix, uint8 *rev_matrix, uint8 *joystick1, uint8 *joystick2, int key) {
    if (keyMapping[key] < 0x100) {
        // its a keyboard key
        pressKey(key_matrix, rev_matrix, (char)keyMapping[key]);
    } else {
        // joysickt or fast key
        switch(keyMapping[key]) {
            case JOY1_UP:
               *joystick1 &= ~(1); 
               break;
            case JOY1_DOWN:
               *joystick1 &= ~(1 << 1);
               break;
            case JOY1_LEFT:
               *joystick1 &= ~(1 << 2);
               break;
            case JOY1_RIGHT:
               *joystick1 &= ~(1 << 3);
               break;
            case JOY1_BTN:
               *joystick1 &= ~(1 << 4);
               break;
            case JOY2_UP:
               *joystick2 &= ~(1); 
               break;
            case JOY2_DOWN:
               *joystick2 &= ~(1 << 1);
               break;
            case JOY2_LEFT:
               *joystick2 &= ~(1 << 2);
               break;
            case JOY2_RIGHT:
               *joystick2 &= ~(1 << 3);
               break;
            case JOY2_BTN:
               *joystick2 &= ~(1 << 4);
               break;
            case KEY_FM:
               fastMode(true);
               break;
        }
        
    }
}
void C64Display::unpressMappedKey(uint8 *key_matrix, uint8 *rev_matrix, uint8 *joystick1, uint8 *joystick2, int key) {
    if (keyMapping[key] < 0x100) {
        // its a keyboard key
        unpressKey(key_matrix, rev_matrix, (char)keyMapping[key]);
    } else {
        // joysickt or fast key
        switch(keyMapping[key]) {
            case JOY1_UP:
               *joystick1 |= (1); 
               break;
            case JOY1_DOWN:
               *joystick1 |= (1 << 1);
               break;
            case JOY1_LEFT:
               *joystick1 |= (1 << 2);
               break;
            case JOY1_RIGHT:
               *joystick1 |= (1 << 3);
               break;
            case JOY1_BTN:
               *joystick1 |= (1 << 4);
               break;
            case JOY2_UP:
               *joystick2 |= (1); 
               break;
            case JOY2_DOWN:
               *joystick2 |= (1 << 1);
               break;
            case JOY2_LEFT:
               *joystick2 |= (1 << 2);
               break;
            case JOY2_RIGHT:
               *joystick2 |= (1 << 3);
               break;
            case JOY2_BTN:
               *joystick2 |= (1 << 4);
               break;
            case KEY_FM:
               fastMode(false);
               break;
        }
        
    }
}

void C64Display::PollKeyboard(uint8 *key_matrix, uint8 *rev_matrix, uint8 *joystick1, uint8 *joystick2)
{
    
    odroid_gamepad_state out_state;
    odroid_input_gamepad_read(&out_state);
    if (startCounter < 5000)   startCounter++;
    if (startCounter == 1) {
        c64_started();
    }
    
    if (writeKeys) {
        writeSomeKeys(key_matrix, rev_matrix);
        return;
    }
   
    
     ///////////// Vkeyboard keypress
    if (! keyPress && !showKeyboard && out_state.values[ODROID_INPUT_A] && out_state.values[ODROID_INPUT_MENU]){
        // spezial key, open vKeyboard
        //unpress all keys / joystick
        *joystick1 = 0xff;
        for (int i = 0; i < 8; i++) {key_matrix[i] = 0xff;}
        showVirtualKeyboard();
        keyPress = 1;
        return;
    }
    if (! keyPress && showKeyboard && out_state.values[ODROID_INPUT_MENU]){
        //unpress all keys / joystick
        *joystick1 = 0xff;
        for (int i = 0; i < 8; i++) {key_matrix[i] = 0xff;}
        
        hideVirtualKeyboard();
        keyPress = 1;
        return;
    }
    
    //////// if keypress is 1 I should wait until all keys are released
    if (keyPress == 1 && ! out_state.values[ODROID_INPUT_A] && ! out_state.values[ODROID_INPUT_MENU]){
        bool keyNotReleased = false;
        for (int i = 0; i < ODROID_INPUT_MAX; i++) {
            if (out_state.values[i]){
                keyNotReleased = true;
                break;
            }
        }
        if (! keyNotReleased) keyPress = 0;
        
        return;
    }
   ///////////////////////////////////
   
   //////////////////////////////
    // update LED
    if (old_led_state[0] != led_state[0]){
        old_led_state[0] = led_state[0];
        if (led_state[0] == DRVLED_ON) {
            odroid_system_led_set(1);
        } else {
            odroid_system_led_set(0);
        }
        /* This is wrong. Sometimes DRVLED_ERROR will come if you should simply change the disc. TODO: blink the blue light if DRVLED_ERROR 
         * 
         * if (led_state[0] == DRVLED_ERROR) {
            showKeyboard = 0;
            odroidFrodoGUI_msgBox("ERROR", "Sorry, drive error on drive 8.\nThis image does not work :(\n", 1);

        }*/
    }
      ////////////////////////
    if (! showKeyboard) {
        if (keyPress == 1) return; // should wait until all keys are released
        holdShift = 0;
        
        // this should update the navRunning variable, so i dont have to check it so often with the displaycontent
        if (out_state.values[ODROID_INPUT_A] && navRunning == 1) {
            navRunning = isNAVrunning();
            if (navRunning) navRunning = 2;
        }
        if (! out_state.values[ODROID_INPUT_A] && navRunning == 2) navRunning = 1;
        
        
        if (! navRunning) {
            if (out_state.values[ODROID_INPUT_UP]) pressMappedKey(key_matrix, rev_matrix, joystick1, joystick2, ODROID_INPUT_UP);
            if (out_state.values[ODROID_INPUT_DOWN]) pressMappedKey(key_matrix, rev_matrix, joystick1, joystick2, ODROID_INPUT_DOWN);
            if (out_state.values[ODROID_INPUT_LEFT]) pressMappedKey(key_matrix, rev_matrix, joystick1, joystick2, ODROID_INPUT_LEFT);
            if (out_state.values[ODROID_INPUT_RIGHT]) pressMappedKey(key_matrix, rev_matrix, joystick1, joystick2, ODROID_INPUT_RIGHT);
            if (out_state.values[ODROID_INPUT_A]) pressMappedKey(key_matrix, rev_matrix, joystick1, joystick2, ODROID_INPUT_A);
            if (out_state.values[ODROID_INPUT_B]) pressMappedKey(key_matrix, rev_matrix, joystick1, joystick2, ODROID_INPUT_B);
            if (out_state.values[ODROID_INPUT_START]) pressMappedKey(key_matrix, rev_matrix, joystick1, joystick2, ODROID_INPUT_START);
            if (out_state.values[ODROID_INPUT_SELECT]) pressMappedKey(key_matrix, rev_matrix, joystick1, joystick2, ODROID_INPUT_SELECT);
            if (!out_state.values[ODROID_INPUT_UP]) unpressMappedKey(key_matrix, rev_matrix, joystick1, joystick2, ODROID_INPUT_UP);
            if (!out_state.values[ODROID_INPUT_DOWN]) unpressMappedKey(key_matrix, rev_matrix, joystick1, joystick2, ODROID_INPUT_DOWN);
            if (!out_state.values[ODROID_INPUT_LEFT]) unpressMappedKey(key_matrix, rev_matrix, joystick1, joystick2, ODROID_INPUT_LEFT);
            if (!out_state.values[ODROID_INPUT_RIGHT]) unpressMappedKey(key_matrix, rev_matrix, joystick1, joystick2, ODROID_INPUT_RIGHT);
            if (!out_state.values[ODROID_INPUT_A]) unpressMappedKey(key_matrix, rev_matrix, joystick1, joystick2, ODROID_INPUT_A);
            if (!out_state.values[ODROID_INPUT_B]) unpressMappedKey(key_matrix, rev_matrix, joystick1, joystick2, ODROID_INPUT_B);
            if (!out_state.values[ODROID_INPUT_START]) unpressMappedKey(key_matrix, rev_matrix, joystick1, joystick2, ODROID_INPUT_START);
            if (!out_state.values[ODROID_INPUT_SELECT]) unpressMappedKey(key_matrix, rev_matrix, joystick1, joystick2, ODROID_INPUT_SELECT);
        } else {
            // KEYMAPPING when nav is running
            if (out_state.values[ODROID_INPUT_UP]) *joystick1 &= ~(1);
            if (out_state.values[ODROID_INPUT_DOWN]) *joystick1 &= ~(1 << 1);
            if (out_state.values[ODROID_INPUT_LEFT]) *joystick1 &= ~(1 << 2);
            if (out_state.values[ODROID_INPUT_RIGHT]) *joystick1 &= ~(1 << 3);
            if (out_state.values[ODROID_INPUT_A]) *joystick1 &= ~(1 << 4);
            if (out_state.values[ODROID_INPUT_B]) pressKey(key_matrix, rev_matrix, ' ');
            if (out_state.values[ODROID_INPUT_START]) pressKey(key_matrix, rev_matrix, KEY_R_S);
            if (out_state.values[ODROID_INPUT_SELECT]) fastMode(true);
            
            if (!out_state.values[ODROID_INPUT_SELECT]) fastMode(false);
            if (!out_state.values[ODROID_INPUT_UP]) *joystick1 |= (1);
            if (!out_state.values[ODROID_INPUT_DOWN]) *joystick1 |= (1 << 1);
            if (!out_state.values[ODROID_INPUT_LEFT]) *joystick1 |= (1 << 2);
            if (!out_state.values[ODROID_INPUT_RIGHT]) *joystick1 |= (1 << 3);
            if (!out_state.values[ODROID_INPUT_A]) *joystick1 |= (1 << 4);
            if (!out_state.values[ODROID_INPUT_B]) unpressKey(key_matrix, rev_matrix, ' ');
            if (!out_state.values[ODROID_INPUT_START]) unpressKey(key_matrix, rev_matrix, KEY_R_S);
            
            
        }
        if (out_state.values[ODROID_INPUT_MENU]) {
            TheC64->TheSID->PauseSound();
            odroidFrodoGUI_showMenu();
            keyPress = 1;
            TheC64->TheSID->ResumeSound();
        }
        

        
        
        
        if (out_state.values[ODROID_INPUT_VOLUME] && keyPress == 0 ) {
            keyPress = 2;
            TheC64->TheSID->changeVolumeLevel();
            
        }
        if (!out_state.values[ODROID_INPUT_VOLUME] && keyPress == 2 )  keyPress = 0;
        
        odroid_keyPress(out_state);
        
        
    } else {
                
        if (holdShift == 1) pressKey(key_matrix, rev_matrix, KEY_SHL);
        if (holdShift == 2){ unpressKey(key_matrix, rev_matrix, KEY_SHL); holdShift = 0; }
        
        if (out_state.values[ODROID_INPUT_UP]) moveCursor( 0,-2 );
        if (out_state.values[ODROID_INPUT_DOWN]) moveCursor( 0,+2 );
        if (out_state.values[ODROID_INPUT_LEFT]) moveCursor( -2,0 );
        if (out_state.values[ODROID_INPUT_RIGHT]) moveCursor( 2,0 );
        
        if (out_state.values[ODROID_INPUT_VOLUME] && keyPress == 0)  {
            keyPress = 3;
            pressKey(key_matrix, rev_matrix, KEY_SHL);  
        }
        if (! out_state.values[ODROID_INPUT_VOLUME] && keyPress == 3)  {
            keyPress = 0;
            unpressKey(key_matrix, rev_matrix, KEY_SHL);  
        }
        if (out_state.values[ODROID_INPUT_SELECT] && keyPress == 0)  {
            keyPress = 4;
            doFlipScreen();
        }
        if (! out_state.values[ODROID_INPUT_SELECT] && keyPress == 4)  {
            keyPress = 0;
        }
        
        if (out_state.values[ODROID_INPUT_A] && keyPressA == -1) {
            keyPressA = mousePress();
            if (keyPressA != -1) pressKey(key_matrix, rev_matrix,  (char)(keyPressA & 0xff));  
        }
        if (out_state.values[ODROID_INPUT_B] && keyPressB == -1) {
            keyPressB = mousePress();
            if (keyPressB != -1) pressKey(key_matrix, rev_matrix,  (char)(keyPressB & 0xff));  
        }
        if (!out_state.values[ODROID_INPUT_A] && keyPressA != -1) {
            unpressKey(key_matrix, rev_matrix,  (char)(keyPressA & 0xff));  
            keyPressA = -1;
        }   
        if (!out_state.values[ODROID_INPUT_B] && keyPressB != -1) {
            unpressKey(key_matrix, rev_matrix,  (char)(keyPressB & 0xff));  
            keyPressB = -1;
        }
        
             
    }
    
       
}




/*
 *  Prefs may have changed
 */

void C64Display::NewPrefs(Prefs *prefs)
{
}


void C64Display::Update(void)
{
    
  
    xQueueSend(videoQueue, (void*)&VideoTaskCommand, portMAX_DELAY);
   // printf("display update\n");
    
}





// Requester dialogue box
long ShowRequester(char *str, char *button1, char *button2)
{
    printf("%s\n", str);
  return(1);
}


void C64Display::setStdKeymapping() {
    setKeymapping(ODROID_INPUT_UP, JOY1_UP);
    setKeymapping(ODROID_INPUT_DOWN, JOY1_DOWN);
    setKeymapping(ODROID_INPUT_LEFT, JOY1_LEFT);
    setKeymapping(ODROID_INPUT_RIGHT, JOY1_RIGHT);
    setKeymapping(ODROID_INPUT_A, JOY1_BTN);
    setKeymapping(ODROID_INPUT_B, ' ');
    setKeymapping(ODROID_INPUT_START, KEY_R_S);
    setKeymapping(ODROID_INPUT_SELECT, KEY_FM);
    
}
void C64Display::setKeymapping(char odroidKey, int c64Key) {
    keyMapping[odroidKey] = c64Key;
}
////////////////// virtual keyboard / cursor /////////////////////
int C64Display::getKey(int pressX, int pressY) {
    for (int i = 0; i < keyPositions.keys; i++){
        if (keyPositions.kyPos[i].x1<=pressX && keyPositions.kyPos[i].x2>=pressX && keyPositions.kyPos[i].y1<=pressY && keyPositions.kyPos[i].y2>=pressY){
            return keyPositions.kyPos[i].key;
        }
    }
    return -1;
}
void C64Display::doFlipScreen() {
    if (flipScreen) flipScreen = 0; else flipScreen = 1;
}
int C64Display::mousePress() {
    return getKey(cursorX + 4, cursorY);
}
void C64Display::moveCursor(int x, int y) {
    cursorX += x;
    cursorY += y;
    if (cursorX < 2) cursorX = 2;
    if (cursorY < 2) cursorY = 2;
    if (cursorX > WIDTH_DISPLAY - CURSOR_MAX_WIDTH) cursorX = WIDTH_DISPLAY - CURSOR_MAX_WIDTH;
    if (cursorY > c64_keyboard.height - 3) cursorY = c64_keyboard.height - 3;
    
    
    int k = getKey(cursorX + 4, cursorY);
    if (k == -1) cursorType = CURSOR_ARROW; else cursorType = CURSOR_HAND;
    
    reDrawCursor = 1;
}
void C64Display::showVirtualKeyboard() {
    if (showKeyboard) return;
    showKeyboard = 1;
    reDrawKeyboard = 1;
    
}
void C64Display::hideVirtualKeyboard() {
    showKeyboard = 0;
    reDrawKeyboard = 0;
    flipScreen = 0;
}
void C64Display::reloadDiskNAV(int dsk) {
    if (isNAVrunning()){
        if (dsk == 8) sendKeys("@8\n");
        if (dsk == 9) sendKeys("@9\n");
        if (dsk == 10) sendKeys("@10\n");
    }
    
}

bool C64Display::isNAVrunning() {
    // i am looking for the "NAV" logo in the top left. We have a problem, if the cursor is in this area in this moment. 
    //There is a better way to determ if nav is running, i think, but this works for now.
    unsigned short crc = 0xFFFF;
    unsigned char x;
    for (int yp = 34; yp < 42; yp++) {
        for (int xp = 32; xp < 78; xp++) {
            for (int b = 0; b < 2; b++) {
                pixel *p=sendBuffer + (xp+(yp*DISPLAY_X)); 
                char bv;
                if (!b) bv = ili9341_palette[*p]; else bv = ili9341_palette[*p] >> 8;
                x = crc >> 8 ^ bv;
                x ^= x>>4;
                crc = (crc << 8) ^ ((unsigned short)(x << 12)) ^ ((unsigned short)(x <<5)) ^ ((unsigned short)x);
            }
            
        }
    }
    printf("nav: %04x\n", crc);
    if (crc == 0x6E2A) return true;
    return false;
}