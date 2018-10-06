/*
 *  main.h - Main program
 *
 *  Frodo (C) 1994-1997,2002 Christian Bauer
 */

#ifndef _MAIN_H
#define _MAIN_H


class C64;

// Global variables
extern char AppDirPath[1024];	// Path of application directory

extern "C" {
    void frodo_main(void);
}


/*
 *  BeOS specific stuff
 */

#ifdef __BEOS__
#include <AppKit.h>
#include <StorageKit.h>

// Global variables
extern bool FromShell;			// true: Started from shell, SAM can be used
extern BEntry AppDirectory;		// Application directory


// Message codes
const uint32 MSG_STARTUP = 'strt';			// Start emulation
const uint32 MSG_PREFS = 'pref';			// Show preferences editor
const uint32 MSG_PREFS_DONE = 'pdon';		// Preferences editor closed
const uint32 MSG_RESET = 'rset';			// Reset C64
const uint32 MSG_NMI = 'nmi ';				// Raise NMI
const uint32 MSG_SAM = 'sam ';				// Invoke SAM
const uint32 MSG_NEXTDISK = 'ndsk';		// Insert next disk in drive 8
const uint32 MSG_TOGGLE_1541 = '1541';		// Toggle processor-level 1541 emulation
const uint32 MSG_OPEN_SNAPSHOT = 'opss';	// Open snapshot file
const uint32 MSG_SAVE_SNAPSHOT = 'svss';	// Save snapshot file
const uint32 MSG_OPEN_SNAPSHOT_RETURNED = 'opsr';	// Open snapshot file panel returned
const uint32 MSG_SAVE_SNAPSHOT_RETURNED = 'svsr';	// Save snapshot file panel returned


// Application signature
const char APP_SIGNATURE[] = "application/x-vnd.cebix-Frodo";


// Application class
class Frodo : public BApplication {
public:
	Frodo();
	virtual void ArgvReceived(int32 argc, char **argv);
	virtual void RefsReceived(BMessage *message);
	virtual void ReadyToRun(void);
	virtual void MessageReceived(BMessage *msg);
	virtual bool QuitRequested(void);
	virtual void AboutRequested(void);

	C64 *TheC64;

private:
	bool load_rom_files(void);

	char prefs_path[1024];	// Pathname of current preferences file
	bool prefs_showing;		// true: Preferences editor is on screen

	BMessenger this_messenger;
	BFilePanel *open_panel;
	BFilePanel *save_panel;
};

#endif


/*
 *  AmigaOS specific stuff
 */

#ifdef AMIGA

class Frodo {
public:
	Frodo();
	void ArgvReceived(int argc, char **argv);
	void ReadyToRun(void);
	void RunPrefsEditor(void);

	C64 *TheC64;

private:
	bool load_rom_files(void);

	char prefs_path[256];	// Pathname of current preferences file
};

// Global variables
extern Frodo *be_app;	// Pointer to Frodo object

#endif


/*
 *  X specific stuff
 */

#ifdef __unix

class Prefs;

class Frodo {
public:
	Frodo();
	void ArgvReceived(int argc, char **argv);
	void ReadyToRun(void);
	static Prefs *reload_prefs(void);

	C64 *TheC64;

private:
	bool load_rom_files(void);

	static char prefs_path[256];	// Pathname of current preferences file
};

#endif


/*
 *  Mac specific stuff
 */

#ifdef __mac__

class Frodo {
public:
	Frodo();

	void Run(void);
	C64 *TheC64;

private:
	bool load_rom_files(void);
};

#endif


/*
 *  WIN32 specific stuff
 */

#ifdef WIN32

class Frodo {
public:
	Frodo();
	~Frodo();
	void ArgvReceived(int argc, char **argv);
	void ReadyToRun();
	void RunPrefsEditor();

	C64 *TheC64;
	char prefs_path[256];	// Pathname of current preferences file

private:
	bool load_rom_files();
};

// Global variables
extern Frodo *TheApp;	// Pointer to Frodo object
extern HINSTANCE hInstance;
extern int nCmdShow;
extern HWND hwnd;

// Command line options.
extern BOOL full_screen;

#if defined(DEBUG)

inline void Debug(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	char tmp[256];
	vsprintf(tmp, format, args);
	va_end(args);
	OutputDebugString(tmp);
}

#else

inline void Debug(const char *format, ...)
{
}

#endif

#define DebugResult(message, val) \
	Debug("%s: 0x%x (%d)\n", message, val, HRESULT_CODE(val))

#endif


/*
 *  RiscOS specific stuff
 */

#ifdef __riscos__

class Frodo
{
  public:
  Frodo(void);
  ~Frodo(void);
  void ReadyToRun(void);

  C64 *TheC64;

  private:
  bool load_rom_files(void);
};

#endif


/*
 *  PSX specific stuff
 */

#ifdef __PSXOS__

class Frodo {
public:
	Frodo();
	void ReadyToRun(void);

	C64 *TheC64;

private:
	bool load_rom_files(void);

	char prefs_path[256];	// Pathname of current preferences file
};

// Global variables
extern Frodo *be_app;	// Pointer to Frodo object

#endif
#ifdef ESP32

class Frodo {
public:
	Frodo(void);
        ~Frodo(void);
	void ReadyToRun(void);

	C64 *TheC64;

private:
	bool load_rom_files(void);

	char prefs_path[256];	// Pathname of current preferences file
};

// Global variables
extern Frodo *be_app;	// Pointer to Frodo object
#endif


#endif
