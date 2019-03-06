/*
 *  C64_x.i - Put the pieces together, X specific stuff
 *
 *  Frodo (C) 1994-1997,2002 Christian Bauer
 *  Unix stuff by Bernd Schmidt/Lutz Vieweg
 */

#include "main.h"
#include "stdbool.h"
#include "esp_timer.h"
#include "odroid_display.h"

#include "LibOdroidGo.h"
#include "UserPort_4Player.h"
#include "Prefs.h"
static struct timeval tv_start;

#ifndef HAVE_USLEEP
/*
 *  NAME:
 *      usleep     -- This is the precision timer for Test Set
 *                    Automation. It uses the select(2) system
 *                    call to delay for the desired number of
 *                    micro-seconds. This call returns ZERO
 *                    (which is usually ignored) on successful
 *                    completion, -1 otherwise.
 *
 *  ALGORITHM:
 *      1) We range check the passed in microseconds and log a
 *         warning message if appropriate. We then return without
 *         delay, flagging an error.
 *      2) Load the Seconds and micro-seconds portion of the
 *         interval timer structure.
 *      3) Call select(2) with no file descriptors set, just the
 *         timer, this results in either delaying the proper
 *         ammount of time or being interupted early by a signal.
 *
 *  HISTORY:
 *      Added when the need for a subsecond timer was evident.
 *
 *  AUTHOR:
 *      Michael J. Dyer                   Telephone:   AT&T 414.647.4044
 *      General Electric Medical Systems        GE DialComm  8 *767.4044
 *      P.O. Box 414  Mail Stop 12-27         Sect'y   AT&T 414.647.4584
 *      Milwaukee, Wisconsin  USA 53201                      8 *767.4584
 *      internet:  mike@sherlock.med.ge.com     GEMS WIZARD e-mail: DYER
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/types.h>
#include "UserPort_4Player.h"
#include "CPUC64.h"
#include "1541job.h"

int usleep(unsigned long int microSeconds)
{
        unsigned int            Seconds, uSec;
        int                     nfds, readfds, writefds, exceptfds;
        struct  timeval         Timer;

        nfds = readfds = writefds = exceptfds = 0;

        if( (microSeconds == (unsigned long) 0)
                || microSeconds > (unsigned long) 4000000 )
        {
                errno = ERANGE;         /* value out of range */
                perror( "usleep time out of range ( 0 -> 4000000 ) " );
                return -1;
        }

        Seconds = microSeconds / (unsigned long) 1000000;
        uSec    = microSeconds % (unsigned long) 1000000;

        Timer.tv_sec            = Seconds;
        Timer.tv_usec           = uSec;

        if( select( nfds, &readfds, &writefds, &exceptfds, &Timer ) < 0 )
        {
                perror( "usleep (select) failed" );
                return -1;
        }

        return 0;
}
#endif


/*
 *  Constructor, system-dependent things
 */

void C64::c64_ctor1(void)
{
    //printf("C64::c64_ctor1\n");
}

void C64::c64_ctor2(void)
{
   // printf("C64::c64_ctor2\n");
}


/*
 *  Destructor, system-dependent things
 */

void C64::c64_dtor(void)
{
}


/*
 *  Start main emulation thread
 */

void C64::Run(void)
{

    // Reset chips
	TheCPU->Reset();
	TheSID->Reset();
	TheCIA1->Reset();
	TheCIA2->Reset();
	TheCPU1541->Reset();
        
        ThePrefs.LimitSpeed = true;
        
	// Patch kernal IEC routines
	orig_kernal_1d84 = Kernal[0x1d84];
	orig_kernal_1d85 = Kernal[0x1d85];
	PatchKernal(ThePrefs.FastReset, ThePrefs.Emul1541Proc);

	quit_thyself = false;
        
        insertUserPortCartridge(UserPortInterface::TYPE_4PLAYER_PROTOVISION);
        c64_started();
        
	thread_func();
}


/*
 *  Vertical blank: Poll keyboard and joysticks, update window, get WLAN remote state
 */
char vblankCounter = 0;
void C64::VBlank(bool draw_frame)
{
	// Poll keyboard
   
   
   
#ifdef VERBOSE_VIDEO
    scounter++;
    if (scounter == 10) {
      
        printf("%llu -- %d -- %d\n", (esp_timer_get_time() - mtimer) / 10, ThePrefs.LimitSpeed, getMultiplayState());
        mtimer = esp_timer_get_time();
        scounter = 0;

    }
#endif      

        if (draw_frame
#ifdef WITH_WLAN
         ||  ! mp_isMultiplayer()  
#endif     
                ) {
            if (mp_isMultiplayer()) for(int i = 0; i < 8 ; i++) {TheCIA1->KeyMatrix[i] = 0xff; TheCIA1->RevMatrix[i] = 0xff;}
            
            /* only to test the 4 Joysticks
            if (ThePrefs.Emul1541Proc && TheUserPortCardridge != NULL && TheUserPortCardridge->GetType() == UserPortInterface::TYPE_4PLAYER_PROTOVISION) {
                uint8_t joykey3 = 0xff;
                uint8_t joykey4 = 0xff;
                TheDisplay->PollKeyboard(TheCIA1->KeyMatrix, TheCIA1->RevMatrix, &joykey3, &joykey4);
                if (ThePrefs.JoystickSwap){
                    ((UserPort_4Player*)TheUserPortCardridge)->setJoy3(joykey4);
                    ((UserPort_4Player*)TheUserPortCardridge)->setJoy4(joykey3);
                } else {
                    ((UserPort_4Player*)TheUserPortCardridge)->setJoy3(joykey3);
                    ((UserPort_4Player*)TheUserPortCardridge)->setJoy4(joykey4);
                }
               
            } else {
                TheDisplay->PollKeyboard(TheCIA1->KeyMatrix, TheCIA1->RevMatrix, &joykey, &joykey2);
            }*/
            
            
            TheDisplay->PollKeyboard(TheCIA1->KeyMatrix, TheCIA1->RevMatrix, &joykey, &joykey2);
            #ifdef WITH_WLAN	
                   uint8_t joykey3 = 0xff;
                   uint8_t joykey4 = 0xff;
                   exchangeNetworkState(TheCIA1->KeyMatrix, TheCIA1->RevMatrix, &joykey, &joykey2, &joykey3, &joykey4);
                    if (TheUserPortCardridge != NULL && TheUserPortCardridge->GetType() == UserPortInterface::TYPE_4PLAYER_PROTOVISION) {
                        UserPort_4Player* vplayer = (UserPort_4Player*)TheUserPortCardridge;
                        vplayer->setJoy3(joykey3);
                        vplayer->setJoy4(joykey4);
                        
                    }
            #endif

            if (ThePrefs.JoystickSwap 
            #ifdef WITH_WLAN
                    || getMultiplayState() == MULTIPLAYER_CONNECTED_CLIENT
            #endif       
                    ){
                    TheCIA1->Joystick2 = joykey;
                    TheCIA1->Joystick1 = joykey2;
            }
            else {
                    TheCIA1->Joystick1 = joykey;
                    TheCIA1->Joystick2 = joykey2;
            }
        }
	// Count TOD clocks
        TheCIA1->CountTOD();
	TheCIA2->CountTOD();
          
	// Update window if needed
	if (draw_frame) {
            TheDisplay->Update();
        
        
        /* save snapshot in memory */
        
        if (! mp_isMultiplayer() && ThePrefs.LimitSpeed && vblankCounter++ > 5)  {
            vblankCounter= 0;
            SaveSnapshotMemory();
        }
       
       /* No speed check. The sound shlould automaticly slow don't to the right speed.
#ifndef WITH_WLAN       
        if (ThePrefs.LimitSpeed) {
#else   
        
        if (ThePrefs.LimitSpeed && ! (getMultiplayState() == MULTIPLAYER_CONNECTED_CLIENT)) {     
#endif
      // Calculate time between VBlanks
		struct timeval tv;
		gettimeofday(&tv, NULL);
		if ((tv.tv_usec -= tv_start.tv_usec) < 0) {
			tv.tv_usec += 1000000;
			tv.tv_sec -= 1;
		}
		tv.tv_sec -= tv_start.tv_sec;
		double elapsed_time = (double)tv.tv_sec * 1000000 + tv.tv_usec;
		speed_index = 20000 / (elapsed_time + 1) * ThePrefs.SkipFrames * 100;
                //printf("speed index: %f\n", speed_index);
		// Limit speed to 100%
		if (speed_index > 100) {
			usleep((unsigned long)(ThePrefs.SkipFrames * 20000 - elapsed_time));
			speed_index = 100;
		}

		gettimeofday(&tv_start, NULL);
        }*/
		
               
		
	}
        
        

     
}


/*
 *  Open/close joystick drivers given old and new state of
 *  joystick preferences
 */

void C64::open_close_joysticks(bool oldjoy1, bool oldjoy2, bool newjoy1, bool newjoy2)
{
    printf("open_close_joysticks\n");
}


/*
 *  Poll joystick port, return CIA mask
 */

uint8 C64::poll_joystick(int port)
{
    //printf("poll_joystick\n");
	return 0xff;

}


/*
 * The emulation's main loop
 */

void C64::thread_func(void)
{
	int linecnt = 0;

#ifdef FRODO_SC
	while (!quit_thyself) {

		// The order of calls is important here
		if (TheVIC->EmulateCycle())
			TheSID->EmulateLine();
		TheCIA1->CheckIRQs();
		TheCIA2->CheckIRQs();
		TheCIA1->EmulateCycle();
		TheCIA2->EmulateCycle();
		TheCPU->EmulateCycle();

		if (ThePrefs.Emul1541Proc) {
			TheCPU1541->CountVIATimers(1);
			if (!TheCPU1541->Idle)
				TheCPU1541->EmulateCycle();
		}
		CycleCounter++;
#else
	while (!quit_thyself) {

		// The order of calls is important here
		int cycles = TheVIC->EmulateLine();
		TheSID->EmulateLine();
#if !PRECISE_CIA_CYCLES
		TheCIA1->EmulateLine(ThePrefs.CIACycles);
		TheCIA2->EmulateLine(ThePrefs.CIACycles);
#endif

		if (ThePrefs.Emul1541Proc) {
			int cycles_1541 = ThePrefs.FloppyCycles;
			TheCPU1541->CountVIATimers(cycles_1541);

			if (!TheCPU1541->Idle) {
				// 1541 processor active, alternately execute
				//  6502 and 6510 instructions until both have
				//  used up their cycles
				while (cycles >= 0 || cycles_1541 >= 0)
					if (cycles > cycles_1541)
						cycles -= TheCPU->EmulateLine(1);
					else
						cycles_1541 -= TheCPU1541->EmulateLine(1);
			} else
				TheCPU->EmulateLine(cycles);
		} else
			// 1541 processor disabled, only emulate 6510
			TheCPU->EmulateLine(cycles);
#endif
		linecnt++;

	}

}
