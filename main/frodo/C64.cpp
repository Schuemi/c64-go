/*
 *  C64.cpp - Put the pieces together
 *
 *  Frodo (C) 1994-1997,2002 Christian Bauer
 */

#include "sysdeps.h"
#include "C64.h"
#include "CPUC64.h"
#include "CPU1541.h"
#include "VIC.h"
#include "SID.h"
#include "CIA.h"
#include "REU.h"
#include "IEC.h"
#include "1541job.h"
#include "Display.h"
#include "Prefs.h"

#include "esp_heap_caps.h"

#include "UserPortInterface.h"

#include "UserPort_4Player.h"


#if defined(__unix) && !defined(__svgalib__)
#include "CmdPipe.h"
#endif


#ifdef FRODO_SC
bool IsFrodoSC = true;
#else
bool IsFrodoSC = false;
#endif

#ifdef VERBOSE_VIDEO
int64_t mtimer = 0;
char scounter = 0;
#endif
/*
 *  Constructor: Allocate objects and memory
 */

C64::C64()
{
	int i,j;
	uint8 *p;

	// The thread is not yet running
	thread_running = false;
	quit_thyself = false;
	have_a_break = false;
        TheUserPortCardridge = NULL;
	// System-dependent things
	c64_ctor1();

	// Open display
	TheDisplay = new C64Display(this);

	// Allocate RAM/ROM memory
	/*RAM = new uint8[0x10000];
	Basic = new uint8[0x2000];
	Kernal = new uint8[0x2000];
	Char = new uint8[0x1000];
	Color = new uint8[0x0400];
	RAM1541 = new uint8[0x0800];
	ROM1541 = new uint8[0x4000];*/
        
        
        RAM = (uint8_t*)heap_caps_calloc(1, 0x10000, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
        if (! RAM){
            printf ("Not enough DMA RAM for 64K main memory. Have to use SPI RAM\n");
            RAM = (uint8_t*)heap_caps_calloc(1, 0x10000, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        }
        Basic = (uint8_t*)heap_caps_calloc(1, 0x2000, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        Kernal = (uint8_t*)heap_caps_calloc(1, 0x2000, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        Char = (uint8_t*)heap_caps_calloc(1, 0x1000, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
        Color = (uint8_t*)heap_caps_calloc(1, 0x0400, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
        RAM1541 = (uint8_t*)heap_caps_calloc(1, 0x0800, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        ROM1541 = (uint8_t*)heap_caps_calloc(1, 0x4000, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if(! mp_isMultiplayer())
            for (int i = 0; i < MAX_SAVE_STATES_MEMORY; i++) SaveStateMemory[i] = (uint8_t*)heap_caps_calloc(1, 0x11000, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        else
             SaveStateMemory[i] = 0;
        
        printf("RAM: %p\n", RAM);
        printf("Basic: %p\n", Basic);
        printf("Kernal: %p\n", Kernal);
        printf("Char: %p\n", Char);
        printf("Color: %p\n", Color);
        printf("RAM1541: %p\n", RAM1541);
        printf("ROM1541: %p\n", ROM1541);
        for (int i = 0; i < MAX_SAVE_STATES_MEMORY; i++) printf("SaveStateMemory: %p\n", SaveStateMemory[i]);
        
                
	// Create the chips
	TheCPU = new MOS6510(this, RAM, Basic, Kernal, Char, Color);

	TheJob1541 = new Job1541(RAM1541);
	TheCPU1541 = new MOS6502_1541(this, TheJob1541, TheDisplay, RAM1541, ROM1541);

	TheVIC = TheCPU->TheVIC = new MOS6569(this, TheDisplay, TheCPU, RAM, Char, Color);
	TheSID = TheCPU->TheSID = new MOS6581(this);
	TheCIA1 = TheCPU->TheCIA1 = new MOS6526_1(TheCPU, TheVIC);
	TheCIA2 = TheCPU->TheCIA2 = TheCPU1541->TheCIA2 = new MOS6526_2(TheCPU, TheVIC, TheCPU1541);
	TheIEC = TheCPU->TheIEC = new IEC(TheDisplay);
	TheREU = TheCPU->TheREU = new REU(TheCPU);
        
        
        
	// Initialize RAM with powerup pattern
	for (i=0, p=RAM; i<512; i++) {
		for (j=0; j<64; j++)
			*p++ = 0;
		for (j=0; j<64; j++)
			*p++ = 0xff;
	}

	// Initialize color RAM with random values
	for (i=0, p=Color; i<1024; i++)
		*p++ = rand() & 0x0f;

	// Clear 1541 RAM
	memset(RAM1541, 0, 0x800);

	// Open joystick drivers if required
	open_close_joysticks(false, false, ThePrefs.Joystick1On, ThePrefs.Joystick2On);
	joykey = 0xff;
        joykey2 = 0xff;
#ifdef FRODO_SC
	CycleCounter = 0;
#endif

	// System-dependent things
	c64_ctor2();
}


/*
 *  Destructor: Delete all objects
 */

C64::~C64()
{
	open_close_joysticks(ThePrefs.Joystick1On, ThePrefs.Joystick2On, false, false);

	delete TheJob1541;
	delete TheREU;
	delete TheIEC;
	delete TheCIA2;
	delete TheCIA1;
	delete TheSID;
	delete TheVIC;
	delete TheCPU1541;
	delete TheCPU;
	delete TheDisplay;

	/*delete[] RAM;
	delete[] Basic;
	delete[] Kernal;
	delete[] Char;
	delete[] Color;
	delete[] RAM1541;
	delete[] ROM1541;*/
        free(RAM);
        free(Basic);
        free(Kernal);
        free(Char);
        free(Color);
        free(RAM1541);
        free(ROM1541);

	c64_dtor();
}
/*
 * Insert User Port Cartridge
 */
bool C64::insertUserPortCartridge(int type) {
    if (type == UserPortInterface::TYPE_4PLAYER_PROTOVISION){
        // insert 4 player interface
        TheUserPortCardridge = new UserPort_4Player();
        TheCIA2->insertUserPortCard(TheUserPortCardridge);
        return true;
    }
    
    return false;
}

/*
 *  Reset C64
 */

void C64::Reset(void)
{
	TheCPU->AsyncReset();
	TheCPU1541->AsyncReset();
	TheSID->Reset();
	TheCIA1->Reset();
	TheCIA2->Reset();
	TheIEC->Reset();
}


/*
 *  NMI C64
 */

void C64::NMI(void)
{

    TheCPU->AsyncNMI();
}


/*
 *  The preferences have changed. prefs is a pointer to the new
 *   preferences, ThePrefs still holds the previous ones.
 *   The emulation must be in the paused state!
 */

void C64::NewPrefs(Prefs *prefs)
{
	open_close_joysticks(ThePrefs.Joystick1On, ThePrefs.Joystick2On, prefs->Joystick1On, prefs->Joystick2On);
	PatchKernal(prefs->FastReset, prefs->Emul1541Proc);

	TheDisplay->NewPrefs(prefs);

#ifdef __riscos__
	// Changed order of calls. If 1541 mode hasn't changed the order is insignificant.
	if (prefs->Emul1541Proc) {
		// New prefs have 1541 enabled ==> if old prefs had disabled free drives FIRST
		TheIEC->NewPrefs(prefs);
		TheJob1541->NewPrefs(prefs);
	} else {
		// New prefs has 1541 disabled ==> if old prefs had enabled free job FIRST
		TheJob1541->NewPrefs(prefs);
		TheIEC->NewPrefs(prefs);
	}
#else
	TheIEC->NewPrefs(prefs);
	TheJob1541->NewPrefs(prefs);
#endif

	TheREU->NewPrefs(prefs);
	TheSID->NewPrefs(prefs);

	// Reset 1541 processor if turned on and dont go back
	if (!ThePrefs.Emul1541Proc && prefs->Emul1541Proc) {
		TheCPU1541->AsyncReset();
                MaxAvailableGoBack = 0;
        }
        // dont go back, if turned off
	if (ThePrefs.Emul1541Proc && !prefs->Emul1541Proc) {
		MaxAvailableGoBack = 0;
        }
        
}


/*
 *  Patch kernal IEC routines
 */

void C64::PatchKernal(bool fast_reset, bool emul_1541_proc)
{
	if (fast_reset) {
		Kernal[0x1d84] = 0xa0;
		Kernal[0x1d85] = 0x00;
	} else {
		Kernal[0x1d84] = orig_kernal_1d84;
		Kernal[0x1d85] = orig_kernal_1d85;
	}

	if (emul_1541_proc) {
		Kernal[0x0d40] = 0x78;
		Kernal[0x0d41] = 0x20;
		Kernal[0x0d23] = 0x78;
		Kernal[0x0d24] = 0x20;
		Kernal[0x0d36] = 0x78;
		Kernal[0x0d37] = 0x20;
		Kernal[0x0e13] = 0x78;
		Kernal[0x0e14] = 0xa9;
		Kernal[0x0def] = 0x78;
		Kernal[0x0df0] = 0x20;
		Kernal[0x0dbe] = 0xad;
		Kernal[0x0dbf] = 0x00;
		Kernal[0x0dcc] = 0x78;
		Kernal[0x0dcd] = 0x20;
		Kernal[0x0e03] = 0x20;
		Kernal[0x0e04] = 0xbe;
	} else {
		Kernal[0x0d40] = 0xf2;	// IECOut
		Kernal[0x0d41] = 0x00;
		Kernal[0x0d23] = 0xf2;	// IECOutATN
		Kernal[0x0d24] = 0x01;
		Kernal[0x0d36] = 0xf2;	// IECOutSec
		Kernal[0x0d37] = 0x02;
		Kernal[0x0e13] = 0xf2;	// IECIn
		Kernal[0x0e14] = 0x03;
		Kernal[0x0def] = 0xf2;	// IECSetATN
		Kernal[0x0df0] = 0x04;
		Kernal[0x0dbe] = 0xf2;	// IECRelATN
		Kernal[0x0dbf] = 0x05;
		Kernal[0x0dcc] = 0xf2;	// IECTurnaround
		Kernal[0x0dcd] = 0x06;
		Kernal[0x0e03] = 0xf2;	// IECRelease
		Kernal[0x0e04] = 0x07;
	}

	// 1541
	ROM1541[0x2ae4] = 0xea;		// Don't check ROM checksum
	ROM1541[0x2ae5] = 0xea;
	ROM1541[0x2ae8] = 0xea;
	ROM1541[0x2ae9] = 0xea;
	ROM1541[0x2c9b] = 0xf2;		// DOS idle loop
	ROM1541[0x2c9c] = 0x00;
	ROM1541[0x3594] = 0x20;		// Write sector
	ROM1541[0x3595] = 0xf2;
	ROM1541[0x3596] = 0xf5;
	ROM1541[0x3597] = 0xf2;
	ROM1541[0x3598] = 0x01;
	ROM1541[0x3b0c] = 0xf2;		// Format track
	ROM1541[0x3b0d] = 0x02;
}


/*
 *  Save RAM contents
 */

void C64::SaveRAM(char *filename)
{
	FILE *f;

	if ((f = fopen(filename, "wb")) == NULL)
		ShowRequester("RAM save failed.", "OK", NULL);
	else {
		fwrite((void*)RAM, 1, 0x10000, f);
		fwrite((void*)Color, 1, 0x400, f);
		if (ThePrefs.Emul1541Proc)
			fwrite((void*)RAM1541, 1, 0x800, f);
		fclose(f);
	}
}


/*
 *  Save CPU state to memory
 *
 *  0: Error / Instruction not completed
 *  size_t: OK
 * 
 */
size_t C64::SaveCPUStateMemory(uint8 *memory)
{
    MOS6510State state;
    TheCPU->GetState(&state);

    if (!state.instruction_complete)
            return 0;

    memcpy(memory, RAM, 0x10000);
    memory += 0x10000;
    
    memcpy(memory, Color, 0x400);
    memory += 0x400;
    
    memcpy(memory, &state, sizeof(MOS6510State));
    
    
    return 0x10000 + 0x400 + sizeof(MOS6510State);
}
/*
 *  Save CPU state to snapshot
 *
 *  0: Error
 *  1: OK
 *  -1: Instruction not completed
 */

int C64::SaveCPUState(FILE *f)
{
	MOS6510State state;
	TheCPU->GetState(&state);

	if (!state.instruction_complete)
		return -1;

	int i = fwrite(RAM, 0x10000, 1, f);
	i += fwrite(Color, 0x400, 1, f);
	i += fwrite((void*)&state, sizeof(state), 1, f);

	return i == 3;
}


/*
 *  Load CPU state from snapshot
 */
size_t C64::LoadCPUStateMemory(uint8 *memory)
{
    MOS6510State state;
    memcpy(RAM, memory, 0x10000);
    memory += 0x10000;
    memcpy(Color, memory, 0x400);
    memory += 0x400;
    memcpy(&state, memory, sizeof(MOS6510State));
    TheCPU->SetState(&state);
    return 0x10000 + 0x400 + sizeof(MOS6510State);
    
}
bool C64::LoadCPUState(FILE *f, char version)
{
	MOS6510State state;

	int i = fread(RAM, 0x10000, 1, f);
	i += fread(Color, 0x400, 1, f);
	i += fread((void*)&state, sizeof(state), 1, f);

	if (i == 3) {
		TheCPU->SetState(&state);
		return true;
	} else
		return false;
}

/*
 *  Save 1541 state to mem
 *
 *  0: Error / Instruction not completed
 *  size_t: OK
 * 
 */

size_t C64::Save1541StateMemory(uint8 *memory)
{
	MOS6502State state;
	TheCPU1541->GetState(&state);

	if (!state.idle && !state.instruction_complete)
		return 0;
        memcpy(memory,RAM1541, 0x800);
        memory += 0x800;
        memcpy(memory,&state, sizeof(MOS6502State));
        return 0x800 + sizeof(MOS6502State);
}


/*
 *  Save 1541 state to snapshot
 *
 *  0: Error
 *  1: OK
 *  -1: Instruction not completed
 */

int C64::Save1541State(FILE *f)
{
	MOS6502State state;
	TheCPU1541->GetState(&state);

	if (!state.idle && !state.instruction_complete)
		return -1;

	int i = fwrite(RAM1541, 0x800, 1, f);
	i += fwrite((void*)&state, sizeof(state), 1, f);

	return i == 2;
}


/*
 *  Load 1541 state from snapshot
 */
size_t C64::Load1541StateMemory(uint8 *memory) 
{
    MOS6502State state;
    memcpy(RAM1541, memory, 0x800);
    memory += 0x800;
    memcpy(&state, memory, sizeof(MOS6502State));
    TheCPU1541->SetState(&state);
    return 0x800 + sizeof(MOS6502State);
}
bool C64::Load1541State(FILE *f, char version)
{
	MOS6502State state;

	int i = fread(RAM1541, 0x800, 1, f);
	i += fread((void*)&state, sizeof(state), 1, f);

	if (i == 2) {
            if( version == 0) state.IECLines = 0xc0;
		TheCPU1541->SetState(&state);
		return true;
	} else
		return false;
}


/*
 *  Save VIC state to snapshot
 */
size_t C64::SaveVICStateMemory(uint8 *memory)
{
    MOS6569State state;
    TheVIC->GetState(&state);
    memcpy(memory, &state, sizeof(MOS6569State));
    return  sizeof(MOS6569State);
}

bool C64::SaveVICState(FILE *f)
{
	MOS6569State state;
	TheVIC->GetState(&state);
	return fwrite((void*)&state, sizeof(state), 1, f) == 1;
}


/*
 *  Load VIC state from snapshot
 */
size_t C64::LoadVICStateMemory(uint8 *memory)
{
    MOS6569State state;
    memcpy(&state, memory, sizeof(MOS6569State));
    TheVIC->SetState(&state);
    return sizeof(MOS6569State);
}
bool C64::LoadVICState(FILE *f, char version)
{
	MOS6569State state;

	if (fread((void*)&state, sizeof(state), 1, f) == 1) {
		TheVIC->SetState(&state);
		return true;
	} else
		return false;
}


/*
 *  Save SID state to snapshot
 */
size_t C64::SaveSIDStateMemory(uint8 *memory)
{
	MOS6581State state;
	TheSID->GetState(&state);
        memcpy(memory, &state, sizeof(MOS6581State));
        return sizeof(MOS6581State);
	
}
bool C64::SaveSIDState(FILE *f)
{
	MOS6581State state;
	TheSID->GetState(&state);
	return fwrite((void*)&state, sizeof(state), 1, f) == 1;
}


/*
 *  Load SID state from snapshot
 */
size_t C64::LoadSIDStateMemory(uint8 *memory)
{
    MOS6581State state;
    memcpy(&state, memory, sizeof(MOS6581State));
    TheSID->SetState(&state);
    return sizeof(MOS6581State);
}
bool C64::LoadSIDState(FILE *f, char version)
{
	MOS6581State state;

	if (fread((void*)&state, sizeof(state), 1, f) == 1) {
		TheSID->SetState(&state);
		return true;
	} else
		return false;
}


/*
 *  Save CIA states to snapshot
 */
size_t C64::SaveCIAStateMemory(uint8 *memory)
{
    MOS6526State state;
    TheCIA1->GetState(&state);
    memcpy(memory, &state, sizeof(MOS6526State));
    memory+=sizeof(MOS6526State);
    TheCIA2->GetState(&state);
    memcpy(memory, &state, sizeof(MOS6526State));
    return 2*sizeof(MOS6526State);
}
bool C64::SaveCIAState(FILE *f)
{
	MOS6526State state;
	TheCIA1->GetState(&state);

	if (fwrite((void*)&state, sizeof(state), 1, f) == 1) {
		TheCIA2->GetState(&state);
		return fwrite((void*)&state, sizeof(state), 1, f) == 1;
	} else
		return false;
}


/*
 *  Load CIA states from snapshot
 */
size_t C64::LoadCIAStateMemory(uint8 *memory)
{
    MOS6526State state;
    memcpy(&state, memory, sizeof(MOS6526State));
    TheCIA1->SetState(&state);
    memory += sizeof(MOS6526State);
    memcpy(&state, memory, sizeof(MOS6526State));
    TheCIA2->SetState(&state);
    return 2*sizeof(MOS6526State);
    
}
bool C64::LoadCIAState(FILE *f, char version)
{
	MOS6526State state;

	if (fread((void*)&state, sizeof(state), 1, f) == 1) {
            if (version == 0) state.IECLines = 0xd0;
		TheCIA1->SetState(&state);
		if (fread((void*)&state, sizeof(state), 1, f) == 1) {
                        if (version == 0) state.IECLines = 0xd0;
			TheCIA2->SetState(&state);
			return true;
		} else
			return false;
	} else
		return false;
}


/*
 *  Save 1541 GCR state to snapshot
 */

size_t C64::Save1541JobStateMemory(uint8 *memory)
{
	Job1541State state;
	TheJob1541->GetState(&state);
	memcpy(memory, &state, sizeof(Job1541State));
        return sizeof(Job1541State);
}


bool C64::Save1541JobState(FILE *f)
{
	Job1541State state;
	TheJob1541->GetState(&state);
	return fwrite((void*)&state, sizeof(state), 1, f) == 1;
}


/*
 *  Load 1541 GCR state from snapshot
 */
size_t C64::Load1541JobStateMemory(uint8 *memory)
{
    Job1541State state;
    memcpy(&state, memory, sizeof(Job1541State));
    TheJob1541->SetState(&state);
    return sizeof(Job1541State);
}
bool C64::Load1541JobState(FILE *f, char version)
{
	Job1541State state;

	if (fread((void*)&state, sizeof(state), 1, f) == 1) {
		TheJob1541->SetState(&state);
		return true;
	} else
		return false;
}


#define SNAPSHOT_HEADER "FrodoSnapshot"
#define SNAPSHOT_1541 1

#define ADVANCE_CYCLES	\
	TheVIC->EmulateCycle(); \
	TheCIA1->EmulateCycle(); \
	TheCIA2->EmulateCycle(); \
	TheCPU->EmulateCycle(); \
	if (ThePrefs.Emul1541Proc) { \
		TheCPU1541->CountVIATimers(1); \
		if (!TheCPU1541->Idle) \
			TheCPU1541->EmulateCycle(); \
	}




#ifdef FRODO_SC
#error Sorry, you have to add in SaveSnapshotMemory and LoadSnapshotMemory FRODO_SC instructions first.
#endif

void C64::SaveSnapshotMemory()
{
    uint8_t *memory = SaveStateMemory[CurrentSaveStateMemory];
    if (! memory) return;
    uint8_t *start = memory;
    CurrentSaveStateMemory++;
    if (CurrentSaveStateMemory == MAX_SAVE_STATES_MEMORY)  CurrentSaveStateMemory = 0;
    memory += SaveVICStateMemory(memory);
    memory += SaveSIDStateMemory(memory);
    memory += SaveCIAStateMemory(memory);
    memory += SaveCPUStateMemory(memory);
    memory[0] = 0; memory++;// No delay
    
    if (ThePrefs.Emul1541Proc) {
        
        memory += Save1541StateMemory(memory);
        memory[0] = 0; memory++;// No delay

        memory += Save1541JobStateMemory(memory);
    }
    MaxAvailableGoBack++;
    if (MaxAvailableGoBack > MAX_SAVE_STATES_MEMORY) MaxAvailableGoBack = MAX_SAVE_STATES_MEMORY;
    
}


bool C64::LoadSnapshotMemory()
{
    if (MaxAvailableGoBack == 0) return false;
    CurrentSaveStateMemory -= 1;
    if (CurrentSaveStateMemory < 0)  CurrentSaveStateMemory = MAX_SAVE_STATES_MEMORY - 1;
    uint8 delay;
    uint8_t *memory = SaveStateMemory[CurrentSaveStateMemory];
    if (! memory) return false;
    
    memory += LoadVICStateMemory(memory);
    memory += LoadSIDStateMemory(memory);
    memory += LoadCIAStateMemory(memory);	
    memory += LoadCPUStateMemory(memory);
    delay = memory[0];memory++;	// Number of cycles the 6510 is ahead of the previous chips (not used yet)
    if (ThePrefs.Emul1541Proc) {
                                        
        memory += Load1541StateMemory(memory);
        delay = memory[0];memory++;	// Number of cycles the 6510 is ahead of the previous chips (not used yet)
        memory += Load1541JobStateMemory(memory);

    }
    LoadVICStateMemory(SaveStateMemory[CurrentSaveStateMemory]); // copyed this from fileload (don't know why it is necessary): Load VIC data twice in SL (is REALLY necessary sometimes!)
    MaxAvailableGoBack--;
    if (MaxAvailableGoBack < 0) MaxAvailableGoBack = 0;
    return true;		
	
}



/*
 *  Save snapshot (emulation must be paused and in VBlank)
 *
 *  To be able to use SC snapshots with SL, SC snapshots are made thus that no
 *  partially dealt with instructions are saved. Instead all devices are advanced
 *  cycle by cycle until the current instruction has been finished. The number of
 *  cycles this takes is saved in the snapshot and will be reconstructed if the
 *  snapshot is loaded into FrodoSC again.
 */

void C64::SaveSnapshot(char *filename)
{
	FILE *f;
	uint8 flags;
	uint8 delay;
	int stat;

	if ((f = fopen(filename, "wb")) == NULL) {
		ShowRequester("Unable to open snapshot file", "OK", NULL);
		return;
	}

	fprintf(f, "%s%c", SNAPSHOT_HEADER, 10);
	fputc(1, f);	// Version number 1
	flags = 0;
	if (ThePrefs.Emul1541Proc)
		flags |= SNAPSHOT_1541;
	fputc(flags, f);
        SaveVICState(f);
        SaveSIDState(f);
        SaveCIAState(f);

#ifdef FRODO_SC
	delay = 0;
	do {
		if ((stat = SaveCPUState(f)) == -1) {	// -1 -> Instruction not finished yet
			ADVANCE_CYCLES;	// Advance everything by one cycle
			delay++;
		}
	} while (stat == -1);
	fputc(delay, f);	// Number of cycles the saved CPUC64 lags behind the previous chips
#else
	SaveCPUState(f);
	fputc(0, f);		// No delay
#endif

	if (ThePrefs.Emul1541Proc) {
		fwrite(ThePrefs.DrivePath[0], 256, 1, f);
#ifdef FRODO_SC
		delay = 0;
		do {
			if ((stat = Save1541State(f)) == -1) {
				ADVANCE_CYCLES;
				delay++;
			}
		} while (stat == -1);
		fputc(delay, f);
#else
		
                Save1541State(f);
		fputc(0, f);	// No delay
#endif
		
                Save1541JobState(f);
	}
	fclose(f);

#ifdef __riscos__
	TheWIMP->SnapshotSaved(true);
#endif
}


/*
 *  Load snapshot (emulation must be paused and in VBlank)
 */

bool C64::LoadSnapshot(char *filename)
{
	FILE *f;
        char version = -1;
	if ((f = fopen(filename, "rb")) != NULL) {
		char Header[] = SNAPSHOT_HEADER;
		char *b = Header, c = 0;
		uint8 delay, i;

		// For some reason memcmp()/strcmp() and so forth utterly fail here.
		while (*b > 32) {
			if ((c = fgetc(f)) != *b++) {
				b = NULL;
				break;
			}
		}
		if (b != NULL) {
			uint8 flags;
			bool error = false;
#ifndef FRODO_SC
			long vicptr;	// File offset of VIC data
#endif

			while (c != 10)
				c = fgetc(f);	// Shouldn't be necessary
			version = fgetc(f);
                        if (version != 0 && version != 1) {
				ShowRequester("Unknown snapshot format", "OK", NULL);
				fclose(f);
				return false;
			}
			flags = fgetc(f);
#ifndef FRODO_SC
			vicptr = ftell(f);
#endif

			error |= !LoadVICState(f, version);
			error |= !LoadSIDState(f, version);
			error |= !LoadCIAState(f, version);
			error |= !LoadCPUState(f, version);

			delay = fgetc(f);	// Number of cycles the 6510 is ahead of the previous chips
#ifdef FRODO_SC
			// Make the other chips "catch up" with the 6510
			for (i=0; i<delay; i++) {
				TheVIC->EmulateCycle();
				TheCIA1->EmulateCycle();
				TheCIA2->EmulateCycle();
			}
#endif
			if ((flags & SNAPSHOT_1541) != 0) {
				Prefs *prefs = new Prefs(ThePrefs);
	
				// First switch on emulation
				error |= (fread(prefs->DrivePath[0], 256, 1, f) != 1);
				prefs->Emul1541Proc = true;
				NewPrefs(prefs);
				ThePrefs = *prefs;
				delete prefs;
	
				// Then read the context
				error |= !Load1541State(f, version);
	
				delay = fgetc(f);	// Number of cycles the 6502 is ahead of the previous chips
#ifdef FRODO_SC
				// Make the other chips "catch up" with the 6502
				for (i=0; i<delay; i++) {
					TheVIC->EmulateCycle();
					TheCIA1->EmulateCycle();
					TheCIA2->EmulateCycle();
					TheCPU->EmulateCycle();
				}
#endif
				Load1541JobState(f, version);
#ifdef __riscos__
				TheWIMP->ThePrefsToWindow();
#endif
			} else if (ThePrefs.Emul1541Proc) {	// No emulation in snapshot, but currently active?
				Prefs *prefs = new Prefs(ThePrefs);
				prefs->Emul1541Proc = false;
				NewPrefs(prefs);
				ThePrefs = *prefs;
				delete prefs;
#ifdef __riscos__
				TheWIMP->ThePrefsToWindow();
#endif
			}

#ifndef FRODO_SC
			fseek(f, vicptr, SEEK_SET);
			LoadVICState(f, version);	// Load VIC data twice in SL (is REALLY necessary sometimes!)
#endif
			fclose(f);
	
			if (error) {
				ShowRequester("Error reading snapshot file", "OK", NULL);
				//Reset();
				return false;
			} else
				return true;
		} else {
			fclose(f);
			ShowRequester("Not a Frodo snapshot file", "OK", NULL);
			return false;
		}
	} else {
		ShowRequester("Can't open snapshot file", "OK", NULL);
		return false;
	}
}


#ifdef __BEOS__
#include "C64_Be.i"
#endif

#ifdef AMIGA
#include "C64_Amiga.i"
#endif

#ifdef __unix
#include "C64_x.i"
#endif

#ifdef ESP32
#include "C64_ESP32.i"
#endif
#ifdef __mac__
#include "C64_mac.i"
#endif

#ifdef WIN32
#include "C64_WIN32.i"
#endif

#ifdef __riscos__
#include "C64_Acorn.i"
#endif
