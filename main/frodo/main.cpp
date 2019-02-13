/*
 *  main.cpp - Main program
 *
 *  Frodo (C) 1994-1997,2002 Christian Bauer
 */

#include "sysdeps.h"

#include "main.h"
#include "C64.h"
#include "Display.h"
#include "Prefs.h"
#include "SAM.h"
#include "LibOdroidGo.h"


// Global variables
char AppDirPath[1024];	// Path of application directory


// ROM file names
#ifdef __riscos__
#define BASIC_ROM_FILE	"FrodoRsrc:Basic_ROM"
#define KERNAL_ROM_FILE	"FrodoRsrc:Kernal_ROM"
#define CHAR_ROM_FILE	"FrodoRsrc:Char_ROM"
#define FLOPPY_ROM_FILE	"FrodoRsrc:1541_ROM"
#else
#define BASIC_ROM_FILE	"/sd/roms/c64/bios/Basic ROM"
#define KERNAL_ROM_FILE	"/sd/roms/c64/bios/Kernal ROM"
#define CHAR_ROM_FILE	"/sd/roms/c64/bios/Char ROM"
#define FLOPPY_ROM_FILE	"/sd/roms/c64/bios/1541 ROM"
#endif

#include "Version.h"


/*
 *  Load C64 ROM files
 */
Frodo::Frodo(void) {
    TheC64 = NULL;
}


Frodo::~Frodo(void)
{
  if (TheC64 != NULL) {delete TheC64;}
}


void Frodo::ReadyToRun(void)
{
   
    getcwd(AppDirPath, 256);

    
    strncpy(prefs_path, "/sd/odroid/data/c64/Frodo.fpr", 255);
    ThePrefs.Load(prefs_path);
    
    
    c64_beforeStart();
    // Create and start C64
    TheC64 = new C64;
    C64_setInstance(TheC64);
    bool res = load_rom_files();
    
    if (res){
            TheC64->Run();
    }

    delete TheC64;
}


bool Frodo::load_rom_files(void)
{
	FILE *file;

	// Load Basic ROM
	if ((file = fopen(BASIC_ROM_FILE, "rb")) != NULL) {
		if (fread(TheC64->Basic, 1, 0x2000, file) != 0x2000) {
			ShowRequester("Can't read 'Basic ROM'.", "Quit");
			return false;
		}
		fclose(file);
	} else {
		ShowRequester("Can't find 'Basic ROM'.", "Quit");
		return false;
	}

	// Load Kernal ROM
	if ((file = fopen(KERNAL_ROM_FILE, "rb")) != NULL) {
		if (fread(TheC64->Kernal, 1, 0x2000, file) != 0x2000) {
			ShowRequester("Can't read 'Kernal ROM'.", "Quit");
			return false;
		}
		fclose(file);
	} else {
		ShowRequester("Can't find 'Kernal ROM'.", "Quit");
		return false;
	}

	// Load Char ROM
	if ((file = fopen(CHAR_ROM_FILE, "rb")) != NULL) {
		if (fread(TheC64->Char, 1, 0x1000, file) != 0x1000) {
			ShowRequester("Can't read 'Char ROM'.", "Quit");
			return false;
		}
		fclose(file);
	} else {
		ShowRequester("Can't find 'Char ROM'.", "Quit");
		return false;
	}

	// Load 1541 ROM
	if ((file = fopen(FLOPPY_ROM_FILE, "rb")) != NULL) {
		if (fread(TheC64->ROM1541, 1, 0x4000, file) != 0x4000) {
			ShowRequester("Can't read '1541 ROM'.", "Quit");
			return false;
		}
		fclose(file);
	} else {
		ShowRequester("Can't find '1541 ROM'.", "Quit");
		return false;
	}

	return true;
}

void frodo_main(void) {
    Frodo *the_app;
 
    printf("%s by Christian Bauer\n", VERSION_STRING);
    //if (!init_graphics())
    //        return 0;
    fflush(stdout);

    the_app = new Frodo();
    //the_app->ArgvReceived(argc, argv);
    the_app->ReadyToRun();
    delete the_app;

   
}