/*
 *  Prefs.cpp - Global preferences
 *
 *  Frodo (C) 1994-1997,2002 Christian Bauer
 */

#include "sysdeps.h"

#include "Prefs.h"
#include "Display.h"
#include "C64.h"
#include "main.h"


// These are the active preferences
Prefs ThePrefs;

// These are the preferences on disk
Prefs ThePrefsOnDisk;


/*
 *  Constructor: Set up preferences with defaults
 */

Prefs::Prefs()
{
	NormalCycles = 63;
	BadLineCycles = 23;
	CIACycles = 63;
	FloppyCycles = 64;
	SkipFrames = 2;
	LatencyMin = 80;
	LatencyMax = 120;
	LatencyAvg = 280;
	ScalingNumerator = 2;
	ScalingDenominator = 2;

	for (int i=0; i<4; i++)
		DriveType[i] = DRVTYPE_D64;

	strcpy(DrivePath[0], "");
        strcpy(DrivePath[1], "");
	strcpy(DrivePath[2], "");
	strcpy(DrivePath[3], "");

	strcpy(ViewPort, "Default");
	strcpy(DisplayMode, "Default");

	SIDType = SIDTYPE_DIGITAL;
	REUSize = REU_NONE;
	DisplayType = DISPTYPE_WINDOW;

	SpritesOn = true;
	SpriteCollisions = true;
	Joystick1On = true;
	Joystick2On = false;
	JoystickSwap = true;
	LimitSpeed = true;
	FastReset = true;
	CIAIRQHack = false;
	MapSlash = true;
	Emul1541Proc = false;
	SIDFilters = true;
	DoubleScan = true;
	HideCursor = false;
	DirectSound = true;	
	ExclusiveSound = false;
	AutoPause = false;
	PrefsAtStartup = false;
	SystemMemory = false;
	AlwaysCopy = false;
	SystemKeys = true;
	ShowLEDs = true;
}


/*
 *  Check if two Prefs structures are equal
 */

bool Prefs::operator==(const Prefs &rhs) const
{
	return (1
		&& NormalCycles == rhs.NormalCycles
		&& BadLineCycles == rhs.BadLineCycles
		&& CIACycles == rhs.CIACycles
		&& FloppyCycles == rhs.FloppyCycles
		&& SkipFrames == rhs.SkipFrames
		&& LatencyMin == rhs.LatencyMin
		&& LatencyMax == rhs.LatencyMax
		&& LatencyAvg == rhs.LatencyAvg
		&& ScalingNumerator == rhs.ScalingNumerator
		&& ScalingDenominator == rhs.ScalingNumerator
		&& DriveType[0] == rhs.DriveType[0]
		&& DriveType[1] == rhs.DriveType[1]
		&& DriveType[2] == rhs.DriveType[2]
		&& DriveType[3] == rhs.DriveType[3]
		&& strcmp(DrivePath[0], rhs.DrivePath[0]) == 0
		&& strcmp(DrivePath[1], rhs.DrivePath[1]) == 0
		&& strcmp(DrivePath[2], rhs.DrivePath[2]) == 0
		&& strcmp(DrivePath[3], rhs.DrivePath[3]) == 0
		&& strcmp(ViewPort, rhs.ViewPort) == 0
		&& strcmp(DisplayMode, rhs.DisplayMode) == 0
		&& SIDType == rhs.SIDType
		&& REUSize == rhs.REUSize
		&& DisplayType == rhs.DisplayType
		&& SpritesOn == rhs.SpritesOn
		&& SpriteCollisions == rhs.SpriteCollisions
		&& Joystick1On == rhs.Joystick1On
		&& Joystick2On == rhs.Joystick2On
		&& JoystickSwap == rhs.JoystickSwap
		&& LimitSpeed == rhs.LimitSpeed
		&& FastReset == rhs.FastReset
		&& CIAIRQHack == rhs.CIAIRQHack
		&& MapSlash == rhs.MapSlash
		&& Emul1541Proc == rhs.Emul1541Proc
		&& SIDFilters == rhs.SIDFilters
		&& DoubleScan == rhs.DoubleScan
		&& HideCursor == rhs.HideCursor
		&& DirectSound == rhs.DirectSound
		&& ExclusiveSound == rhs.ExclusiveSound
		&& AutoPause == rhs.AutoPause
		&& PrefsAtStartup == rhs.PrefsAtStartup
		&& SystemMemory == rhs.SystemMemory
		&& AlwaysCopy == rhs.AlwaysCopy
		&& SystemKeys == rhs.SystemKeys
		&& ShowLEDs == rhs.ShowLEDs
	);
}

bool Prefs::operator!=(const Prefs &rhs) const
{
	return !operator==(rhs);
}


/*
 *  Check preferences for validity and correct if necessary
 */

void Prefs::Check(void)
{
	if (SkipFrames <= 0) SkipFrames = 1;

	if (SIDType < SIDTYPE_NONE || SIDType > SIDTYPE_SIDCARD)
		SIDType = SIDTYPE_NONE;

	if (REUSize < REU_NONE || REUSize > REU_512K)
		REUSize = REU_NONE;

	if (DisplayType < DISPTYPE_WINDOW || DisplayType > DISPTYPE_SCREEN)
		DisplayType = DISPTYPE_WINDOW;

	for (int i=0; i<4; i++)
		if (DriveType[i] < DRVTYPE_DIR || DriveType[i] > DRVTYPE_T64)
			DriveType[i] = DRVTYPE_DIR;
}


/*
 *  Load preferences from file
 */

void Prefs::Load(char *filename)
{
	FILE *file;
	char line[256], keyword[256], value[256];

	if ((file = fopen(filename, "r")) != NULL) {
		while(fgets(line, 255, file)) {
			if (sscanf(line, "%s = %s\n", keyword, value) == 2) {
				if (!strcmp(keyword, "NormalCycles"))
					NormalCycles = atoi(value);
				else if (!strcmp(keyword, "BadLineCycles"))
					BadLineCycles = atoi(value);
				else if (!strcmp(keyword, "CIACycles"))
					CIACycles = atoi(value);
				else if (!strcmp(keyword, "FloppyCycles"))
					FloppyCycles = atoi(value);
				else if (!strcmp(keyword, "SkipFrames"))
					SkipFrames = atoi(value);
				else if (!strcmp(keyword, "LatencyMin"))
					LatencyMin = atoi(value);
				else if (!strcmp(keyword, "LatencyMax"))
					LatencyMax = atoi(value);
				else if (!strcmp(keyword, "LatencyAvg"))
					LatencyAvg = atoi(value);
				else if (!strcmp(keyword, "ScalingNumerator"))
					ScalingNumerator = atoi(value);
				else if (!strcmp(keyword, "ScalingDenominator"))
					ScalingDenominator = atoi(value);
				else if (!strcmp(keyword, "DriveType8"))
					if (!strcmp(value, "DIR"))
						DriveType[0] = DRVTYPE_DIR;
					else if (!strcmp(value, "D64"))
						DriveType[0] = DRVTYPE_D64;
					else
						DriveType[0] = DRVTYPE_T64;
				else if (!strcmp(keyword, "DriveType9"))
					if (!strcmp(value, "DIR"))
						DriveType[1] = DRVTYPE_DIR;
					else if (!strcmp(value, "D64"))
						DriveType[1] = DRVTYPE_D64;
					else
						DriveType[1] = DRVTYPE_T64;
				else if (!strcmp(keyword, "DriveType10"))
					if (!strcmp(value, "DIR"))
						DriveType[2] = DRVTYPE_DIR;
					else if (!strcmp(value, "D64"))
						DriveType[2] = DRVTYPE_D64;
					else
						DriveType[2] = DRVTYPE_T64;
				else if (!strcmp(keyword, "DriveType11"))
					if (!strcmp(value, "DIR"))
						DriveType[3] = DRVTYPE_DIR;
					else if (!strcmp(value, "D64"))
						DriveType[3] = DRVTYPE_D64;
					else
						DriveType[3] = DRVTYPE_T64;
				else if (!strcmp(keyword, "DrivePath8"))
					strcpy(DrivePath[0], value);
				else if (!strcmp(keyword, "DrivePath9"))
					strcpy(DrivePath[1], value);
				else if (!strcmp(keyword, "DrivePath10"))
					strcpy(DrivePath[2], value);
				else if (!strcmp(keyword, "DrivePath11"))
					strcpy(DrivePath[3], value);
				else if (!strcmp(keyword, "ViewPort"))
					strcpy(ViewPort, value);
				else if (!strcmp(keyword, "DisplayMode"))
					strcpy(DisplayMode, value);
				else if (!strcmp(keyword, "SIDType"))
					if (!strcmp(value, "DIGITAL"))
						SIDType = SIDTYPE_DIGITAL;
					else if (!strcmp(value, "SIDCARD"))
						SIDType = SIDTYPE_SIDCARD;
					else
						SIDType = SIDTYPE_NONE;
				else if (!strcmp(keyword, "REUSize")) {
					if (!strcmp(value, "128K"))
						REUSize = REU_128K;
					else if (!strcmp(value, "256K"))
						REUSize = REU_256K;
					else if (!strcmp(value, "512K"))
						REUSize = REU_512K;
					else
						REUSize = REU_NONE;
				} else if (!strcmp(keyword, "DisplayType"))
					DisplayType = strcmp(value, "SCREEN") ? DISPTYPE_WINDOW : DISPTYPE_SCREEN;
				else if (!strcmp(keyword, "SpritesOn"))
					SpritesOn = !strcmp(value, "TRUE");
				else if (!strcmp(keyword, "SpriteCollisions"))
					SpriteCollisions = !strcmp(value, "TRUE");
				else if (!strcmp(keyword, "Joystick1On"))
					Joystick1On = !strcmp(value, "TRUE");
				else if (!strcmp(keyword, "Joystick2On"))
					Joystick2On = !strcmp(value, "TRUE");
				else if (!strcmp(keyword, "JoystickSwap"))
					JoystickSwap = !strcmp(value, "TRUE");
				else if (!strcmp(keyword, "LimitSpeed"))
					LimitSpeed = !strcmp(value, "TRUE");
				else if (!strcmp(keyword, "FastReset"))
					FastReset = !strcmp(value, "TRUE");
				else if (!strcmp(keyword, "CIAIRQHack"))
					CIAIRQHack = !strcmp(value, "TRUE");
				else if (!strcmp(keyword, "MapSlash"))
					MapSlash = !strcmp(value, "TRUE");
				else if (!strcmp(keyword, "Emul1541Proc"))
					Emul1541Proc = !strcmp(value, "TRUE");
				else if (!strcmp(keyword, "SIDFilters"))
					SIDFilters = !strcmp(value, "TRUE");
				else if (!strcmp(keyword, "DoubleScan"))
					DoubleScan = !strcmp(value, "TRUE");
				else if (!strcmp(keyword, "HideCursor"))
					HideCursor = !strcmp(value, "TRUE");
				else if (!strcmp(keyword, "DirectSound"))
					DirectSound = !strcmp(value, "TRUE");
				else if (!strcmp(keyword, "ExclusiveSound"))
					ExclusiveSound = !strcmp(value, "TRUE");
				else if (!strcmp(keyword, "AutoPause"))
					AutoPause = !strcmp(value, "TRUE");
				else if (!strcmp(keyword, "PrefsAtStartup"))
					PrefsAtStartup = !strcmp(value, "TRUE");
				else if (!strcmp(keyword, "SystemMemory"))
					SystemMemory = !strcmp(value, "TRUE");
				else if (!strcmp(keyword, "AlwaysCopy"))
					AlwaysCopy = !strcmp(value, "TRUE");
				else if (!strcmp(keyword, "SystemKeys"))
					SystemKeys = !strcmp(value, "TRUE");
				else if (!strcmp(keyword, "ShowLEDs"))
					ShowLEDs = !strcmp(value, "TRUE");
			}
		}
		fclose(file);
	}
	Check();
	ThePrefsOnDisk = *this;
}


/*
 *  Save preferences to file
 *  true: success, false: error
 */

bool Prefs::Save(char *filename)
{
	FILE *file;

	Check();
	if ((file = fopen(filename, "w")) != NULL) {
		fprintf(file, "NormalCycles = %d\n", NormalCycles);
		fprintf(file, "BadLineCycles = %d\n", BadLineCycles);
		fprintf(file, "CIACycles = %d\n", CIACycles);
		fprintf(file, "FloppyCycles = %d\n", FloppyCycles);
		fprintf(file, "SkipFrames = %d\n", SkipFrames);
		fprintf(file, "LatencyMin = %d\n", LatencyMin);
		fprintf(file, "LatencyMax = %d\n", LatencyMax);
		fprintf(file, "LatencyAvg = %d\n", LatencyAvg);
		fprintf(file, "ScalingNumerator = %d\n", ScalingNumerator);
		fprintf(file, "ScalingDenominator = %d\n", ScalingDenominator);
		for (int i=0; i<4; i++) {
			fprintf(file, "DriveType%d = ", i+8);
			switch (DriveType[i]) {
				case DRVTYPE_DIR:
					fprintf(file, "DIR\n");
					break;
				case DRVTYPE_D64:
					fprintf(file, "D64\n");
					break;
				case DRVTYPE_T64:
					fprintf(file, "T64\n");
					break;
			}
			fprintf(file, "DrivePath%d = %s\n", i+8, DrivePath[i]);
		}
		fprintf(file, "ViewPort = %s\n", ViewPort);
		fprintf(file, "DisplayMode = %s\n", DisplayMode);
		fprintf(file, "SIDType = ");
		switch (SIDType) {
			case SIDTYPE_NONE:
				fprintf(file, "NONE\n");
				break;
			case SIDTYPE_DIGITAL:
				fprintf(file, "DIGITAL\n");
				break;
			case SIDTYPE_SIDCARD:
				fprintf(file, "SIDCARD\n");
				break;
		}
		fprintf(file, "REUSize = ");
		switch (REUSize) {
			case REU_NONE:
				fprintf(file, "NONE\n");
				break;
			case REU_128K:
				fprintf(file, "128K\n");
				break;
			case REU_256K:
				fprintf(file, "256K\n");
				break;
			case REU_512K:
				fprintf(file, "512K\n");
				break;
		};
		fprintf(file, "DisplayType = %s\n", DisplayType == DISPTYPE_WINDOW ? "WINDOW" : "SCREEN");
		fprintf(file, "SpritesOn = %s\n", SpritesOn ? "TRUE" : "FALSE");
		fprintf(file, "SpriteCollisions = %s\n", SpriteCollisions ? "TRUE" : "FALSE");
		fprintf(file, "Joystick1On = %s\n", Joystick1On ? "TRUE" : "FALSE");
		fprintf(file, "Joystick2On = %s\n", Joystick2On ? "TRUE" : "FALSE");
		fprintf(file, "JoystickSwap = %s\n", JoystickSwap ? "TRUE" : "FALSE");
		fprintf(file, "LimitSpeed = %s\n", LimitSpeed ? "TRUE" : "FALSE");
		fprintf(file, "FastReset = %s\n", FastReset ? "TRUE" : "FALSE");
		fprintf(file, "CIAIRQHack = %s\n", CIAIRQHack ? "TRUE" : "FALSE");
		fprintf(file, "MapSlash = %s\n", MapSlash ? "TRUE" : "FALSE");
		fprintf(file, "Emul1541Proc = %s\n", Emul1541Proc ? "TRUE" : "FALSE");
		fprintf(file, "SIDFilters = %s\n", SIDFilters ? "TRUE" : "FALSE");
		fprintf(file, "DoubleScan = %s\n", DoubleScan ? "TRUE" : "FALSE");
		fprintf(file, "HideCursor = %s\n", HideCursor ? "TRUE" : "FALSE");
		fprintf(file, "DirectSound = %s\n", DirectSound ? "TRUE" : "FALSE");
		fprintf(file, "ExclusiveSound = %s\n", ExclusiveSound ? "TRUE" : "FALSE");
		fprintf(file, "AutoPause = %s\n", AutoPause ? "TRUE" : "FALSE");
		fprintf(file, "PrefsAtStartup = %s\n", PrefsAtStartup ? "TRUE" : "FALSE");
		fprintf(file, "SystemMemory = %s\n", SystemMemory ? "TRUE" : "FALSE");
		fprintf(file, "AlwaysCopy = %s\n", AlwaysCopy ? "TRUE" : "FALSE");
		fprintf(file, "SystemKeys = %s\n", SystemKeys ? "TRUE" : "FALSE");
		fprintf(file, "ShowLEDs = %s\n", ShowLEDs ? "TRUE" : "FALSE");
		fclose(file);
		ThePrefsOnDisk = *this;
		return true;
	}
	return false;
}


#ifdef __BEOS__
#include "Prefs_Be.i"
#endif

#ifdef AMIGA
#include "Prefs_Amiga.i"
#endif

#ifdef WIN32
#include "Prefs_WIN32.i"
#endif
