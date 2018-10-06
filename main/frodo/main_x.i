/*
 *  main_x.i - Main program, X specific stuff
 *
 *  Frodo (C) 1994-1997,2002 Christian Bauer
 */

#include "Version.h"


extern int init_graphics(void);


// Global variables
char Frodo::prefs_path[256] = "";


/*
 *  Create application object and start it
 */

int main(int argc, char **argv)
{
	Frodo *the_app;

	timeval tv;
	gettimeofday(&tv, NULL);
	srand(tv.tv_usec);

	printf("%s by Christian Bauer\n", VERSION_STRING);
	if (!init_graphics())
		return 0;
	fflush(stdout);

	the_app = new Frodo();
	the_app->ArgvReceived(argc, argv);
	the_app->ReadyToRun();
	delete the_app;

	return 0;
}


/*
 *  Constructor: Initialize member variables
 */

Frodo::Frodo()
{
	TheC64 = NULL;
}


/*
 *  Process command line arguments
 */

void Frodo::ArgvReceived(int argc, char **argv)
{
	if (argc == 2)
		strncpy(prefs_path, argv[1], 255);
}


/*
 *  Arguments processed, run emulation
 */

void Frodo::ReadyToRun(void)
{
	getcwd(AppDirPath, 256);

	// Load preferences
	if (!prefs_path[0]) {
		char *home = getenv("HOME");
		if (home != NULL && strlen(home) < 240) {
			strncpy(prefs_path, home, 200);
			strcat(prefs_path, "/");
		}
		strcat(prefs_path, ".frodorc");
	}
	ThePrefs.Load(prefs_path);

	// Create and start C64
	TheC64 = new C64;
	if (load_rom_files())
		TheC64->Run();
	delete TheC64;
}


Prefs *Frodo::reload_prefs(void)
{
	static Prefs newprefs;
	newprefs.Load(prefs_path);
	return &newprefs;
}
