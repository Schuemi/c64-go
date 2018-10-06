/*
 *  Prefs.h - Global preferences
 *
 *  Frodo (C) 1994-1997,2002 Christian Bauer
 */

#ifndef _PREFS_H
#define _PREFS_H


// Drive types
enum {
	DRVTYPE_DIR,	// 1541 emulation in host file system
	DRVTYPE_D64,	// 1541 emulation in .d64 file
	DRVTYPE_T64		// 1541 emulation in .t64 file
};


// SID types
enum {
	SIDTYPE_NONE,		// SID emulation off
	SIDTYPE_DIGITAL,	// Digital SID emulation
	SIDTYPE_SIDCARD		// SID card
};


// REU sizes
enum {
	REU_NONE,		// No REU
	REU_128K,		// 128K
	REU_256K,		// 256K
	REU_512K		// 512K
};


// Display types (BeOS)
enum {
	DISPTYPE_WINDOW,	// BWindow
	DISPTYPE_SCREEN		// BWindowScreen
};


// Preferences data
class Prefs {
public:
	Prefs();
	bool ShowEditor(bool startup, char *prefs_name);
	void Check(void);
	void Load(char *filename);
	bool Save(char *filename);

	bool operator==(const Prefs &rhs) const;
	bool operator!=(const Prefs &rhs) const;

	int NormalCycles;		// Available CPU cycles in normal raster lines
	int BadLineCycles;		// Available CPU cycles in Bad Lines
	int CIACycles;			// CIA timer ticks per raster line
	int FloppyCycles;		// Available 1541 CPU cycles per line
	int SkipFrames;			// Draw every n-th frame

	int DriveType[4];		// Type of drive 8..11

	char DrivePath[4][256];	// Path for drive 8..11

	char ViewPort[256];		// Size of the C64 screen to display (Win32)
	char DisplayMode[256];	// Video mode to use for full screen (Win32)

	int SIDType;			// SID emulation type
	int REUSize;			// Size of REU
	int DisplayType;		// Display type (BeOS)
	int LatencyMin;			// Min msecs ahead of sound buffer (Win32)
	int LatencyMax;			// Max msecs ahead of sound buffer (Win32)
	int LatencyAvg;			// Averaging interval in msecs (Win32)
	int ScalingNumerator;	// Window scaling numerator (Win32)
	int ScalingDenominator;	// Window scaling denominator (Win32)

	bool SpritesOn;			// Sprite display is on
	bool SpriteCollisions;	// Sprite collision detection is on
	bool Joystick1On;		// Joystick connected to port 1 of host
	bool Joystick2On;		// Joystick connected to port 2 of host
	bool JoystickSwap;		// Swap joysticks 1<->2
	bool LimitSpeed;		// Limit speed to 100%
	bool FastReset;			// Skip RAM test on reset
	bool CIAIRQHack;		// Write to CIA ICR clears IRQ
	bool MapSlash;			// Map '/' in C64 filenames
	bool Emul1541Proc;		// Enable processor-level 1541 emulation
	bool SIDFilters;		// Emulate SID filters
	bool DoubleScan;		// Double scan lines (BeOS, if DisplayType == DISPTYPE_SCREEN)
	bool HideCursor;		// Hide mouse cursor when visible (Win32)
	bool DirectSound;		// Use direct sound (instead of wav) (Win32)
	bool ExclusiveSound;	// Use exclusive mode with direct sound (Win32)
	bool AutoPause;			// Auto pause when not foreground app (Win32)
	bool PrefsAtStartup;	// Show prefs dialog at startup (Win32)
	bool SystemMemory;		// Put view work surface in system mem (Win32)
	bool AlwaysCopy;		// Always use a work surface (Win32)
	bool SystemKeys;		// Enable system keys and menu keys (Win32)
	bool ShowLEDs;			// Show LEDs (Win32)

#ifdef __mac__
	void ChangeDisks(void);
#endif

#ifdef WIN32
private:
	static BOOL CALLBACK StandardDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	static BOOL CALLBACK WIN32DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	BOOL DialogProc(int page, HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	void SetupControls(int page);
	void SetValues(int page);
	void GetValues(int page);
	void BrowseForDevice(int id);

	static Prefs *edit_prefs;
	static char *edit_prefs_name;
	static HWND hDlg;
#endif
};


// These are the active preferences
extern Prefs ThePrefs;

// Theses are the preferences on disk
extern Prefs ThePrefsOnDisk;

#endif
