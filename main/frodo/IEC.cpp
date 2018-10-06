/*
 *  IEC.cpp - IEC bus routines, 1541 emulation (DOS level)
 *
 *  Frodo (C) 1994-1997,2002 Christian Bauer
 *

 *
 * Notes:
 * ------
 *
 *  - There are three kinds of devices on the IEC bus: controllers,
 *    listeners and talkers. We are always the controller and we
 *    can additionally be either listener or talker. There can be
 *    only one listener and one talker active at the same time (the
 *    real IEC bus allows multiple listeners, but we don't).
 *  - There is one Drive object for every emulated drive (8..11).
 *    A pointer to one of them is stored in "listener"/"talker"
 *    when talk()/listen() is called and is used by the functions
 *    called afterwards.
 *  - The Drive objects have four virtual functions so that the
 *    interface to them is independent of their implementation:
 *      Open() opens a channel
 *      Close() closes a channel
 *      Read() reads from a channel
 *      Write() writes to a channel
 *  - The EOI/EOF signal is special on the IEC bus in that it is
 *    Sent before the last byte, not after it.
 */

#include "sysdeps.h"

#include "IEC.h"
#include "1541fs.h"
#include "1541d64.h"
#include "1541t64.h"
#include "Prefs.h"
#include "Display.h"


/*
 *  Constructor: Initialize variables
 */

IEC::IEC(C64Display *display) : the_display(display)
{
	int i;

	// Create drives 8..11
	for (i=0; i<4; i++)
		drive[i] = NULL;	// Important because UpdateLEDs is called from the drive constructors (via set_error)

	if (!ThePrefs.Emul1541Proc)
		for (i=0; i<4; i++) {
			if (ThePrefs.DriveType[i] == DRVTYPE_DIR)
				drive[i] = new FSDrive(this, ThePrefs.DrivePath[i]);
			else if (ThePrefs.DriveType[i] == DRVTYPE_D64)
				drive[i] = new D64Drive(this, ThePrefs.DrivePath[i]);
			else
				drive[i] = new T64Drive(this, ThePrefs.DrivePath[i]);
		}

	listener_active = talker_active = false;
	listening = false;
}


/*
 *  Destructor: Delete drives
 */

IEC::~IEC()
{
	for (int i=0; i<4; i++)
		delete drive[i];
}


/*
 *  Reset all drives
 */

void IEC::Reset(void)
{
	for (int i=0; i<4; i++)
		if (drive[i] != NULL && drive[i]->Ready)
			drive[i]->Reset();

	UpdateLEDs();
}


/*
 *  Preferences have changed, prefs points to new preferences,
 *  ThePrefs still holds the previous ones. Check if drive settings
 *  have changed.
 */

void IEC::NewPrefs(Prefs *prefs)
{
	// Delete and recreate all changed drives
	for (int i=0; i<4; i++)
		if ((ThePrefs.DriveType[i] != prefs->DriveType[i]) || strcmp(ThePrefs.DrivePath[i], prefs->DrivePath[i]) || ThePrefs.Emul1541Proc != prefs->Emul1541Proc) {
                   
                        delete drive[i];
                   
			drive[i] = NULL;	// Important because UpdateLEDs is called from drive constructors (via set_error())
			if (!prefs->Emul1541Proc) {
				if (prefs->DriveType[i] == DRVTYPE_DIR)
					drive[i] = new FSDrive(this, prefs->DrivePath[i]);
				else if (prefs->DriveType[i] == DRVTYPE_D64)
					drive[i] = new D64Drive(this, prefs->DrivePath[i]);
				else
					drive[i] = new T64Drive(this, prefs->DrivePath[i]);
			}
		}

	UpdateLEDs();
}


/*
 *  Update drive LED display
 */

void IEC::UpdateLEDs(void)
{
	if (drive[0] != NULL && drive[1] != NULL && drive[2] != NULL && drive[3] != NULL)
		the_display->UpdateLEDs(drive[0]->LED, drive[1]->LED, drive[2]->LED, drive[3]->LED);
}


/*
 *  Output one byte
 */

uint8 IEC::Out(uint8 byte, bool eoi)
{
	if (listener_active) {
		if (received_cmd == CMD_OPEN)
			return open_out(byte, eoi);
		if (received_cmd == CMD_DATA)
			return data_out(byte, eoi);
		return ST_TIMEOUT;
	} else
		return ST_TIMEOUT;
}


/*
 *  Output one byte with ATN (Talk/Listen/Untalk/Unlisten)
 */

uint8 IEC::OutATN(uint8 byte)
{
	received_cmd = sec_addr = 0;	// Command is sent with secondary address
	switch (byte & 0xf0) {
		case ATN_LISTEN:
			listening = true;
			return listen(byte & 0x0f);
		case ATN_UNLISTEN:
			listening = false;
			return unlisten();
		case ATN_TALK:
			listening = false;
			return talk(byte & 0x0f);
		case ATN_UNTALK:
			listening = false;
			return untalk();
	}
	return ST_TIMEOUT;
}


/*
 *  Output secondary address
 */

uint8 IEC::OutSec(uint8 byte)
{
	if (listening) {
		if (listener_active) {
			sec_addr = byte & 0x0f;
			received_cmd = byte & 0xf0;
			return sec_listen();
		}
	} else {
		if (talker_active) {
			sec_addr = byte & 0x0f;
			received_cmd = byte & 0xf0;
			return sec_talk();
		}
	}
	return ST_TIMEOUT;
}


/*
 *  Read one byte
 */

uint8 IEC::In(uint8 *byte)
{
	if (talker_active && (received_cmd == CMD_DATA))
		return data_in(byte);

	*byte = 0;
	return ST_TIMEOUT;
}


/*
 *  Assert ATN (for Untalk)
 */

void IEC::SetATN(void)
{
	// Only needed for real IEC
}


/*
 *  Release ATN
 */

void IEC::RelATN(void)
{
	// Only needed for real IEC
}


/*
 *  Talk-attention turn-around
 */

void IEC::Turnaround(void)
{
	// Only needed for real IEC
}


/*
 *  System line release
 */

void IEC::Release(void)
{
	// Only needed for real IEC
}


/*
 *  Listen
 */

uint8 IEC::listen(int device)
{
	if ((device >= 8) && (device <= 11)) {
		if ((listener = drive[device-8]) != NULL && listener->Ready) {
			listener_active = true;
			return ST_OK;
		}
	}

	listener_active = false;
	return ST_NOTPRESENT;
}


/*
 *  Talk
 */

uint8 IEC::talk(int device)
{
	if ((device >= 8) && (device <= 11)) {
		if ((talker = drive[device-8]) != NULL && talker->Ready) {
			talker_active = true;
			return ST_OK;
		}
	}

	talker_active = false;
	return ST_NOTPRESENT;
}


/*
 *  Unlisten
 */

uint8 IEC::unlisten(void)
{
	listener_active = false;
	return ST_OK;
}


/*
 *  Untalk
 */

uint8 IEC::untalk(void)
{
	talker_active = false;
	return ST_OK;
}


/*
 *  Secondary address after Listen
 */

uint8 IEC::sec_listen(void)
{
	switch (received_cmd) {

		case CMD_OPEN:	// Prepare for receiving the file name
			name_ptr = name_buf;
			name_len = 0;
			return ST_OK;

		case CMD_CLOSE: // Close channel
			if (listener->LED != DRVLED_ERROR) {
				listener->LED = DRVLED_OFF;		// Turn off drive LED
				UpdateLEDs();
			}
			return listener->Close(sec_addr);
	}
	return ST_OK;
}


/*
 *  Secondary address after Talk
 */

uint8 IEC::sec_talk(void)
{
	return ST_OK;
}


/*
 *  Byte after Open command: Store character in file name, open file on EOI
 */

uint8 IEC::open_out(uint8 byte, bool eoi)
{
	if (name_len < NAMEBUF_LENGTH) {
		*name_ptr++ = byte;
		name_len++;
	}

	if (eoi) {
		*name_ptr = 0;				// End string
		listener->LED = DRVLED_ON;	// Turn on drive LED
		UpdateLEDs();
		return listener->Open(sec_addr, name_buf);
	}

	return ST_OK;
}


/*
 *  Write byte to channel
 */

uint8 IEC::data_out(uint8 byte, bool eoi)
{
	return listener->Write(sec_addr, byte, eoi);
}


/*
 *  Read byte from channel
 */

uint8 IEC::data_in(uint8 *byte)
{
	return talker->Read(sec_addr, byte);
}


/*
 *  Drive constructor
 */

Drive::Drive(IEC *iec)
{
	the_iec = iec;
	LED = DRVLED_OFF;
	Ready = false;
	set_error(ERR_STARTUP);
}


/*
 *  Set error message on drive
 */

// 1541 error messages
char *Errors_1541[] = {
	"00, OK,00,00\r",
	"25,WRITE ERROR,00,00\r",
	"26,WRITE PROTECT ON,00,00\r",
	"30,SYNTAX ERROR,00,00\r",
	"33,SYNTAX ERROR,00,00\r",
	"60,WRITE FILE OPEN,00,00\r",
	"61,FILE NOT OPEN,00,00\r",
	"62,FILE NOT FOUND,00,00\r",
	"67,ILLEGAL TRACK OR SECTOR,00,00\r",
	"70,NO CHANNEL,00,00\r",
	"73,CBM DOS V2.6 1541,00,00\r",
	"74,DRIVE NOT READY,00,00\r"
};

void Drive::set_error(int error)
{
	error_ptr = Errors_1541[error];
	error_len = strlen(error_ptr);

	// Set drive condition
	if (error != ERR_OK)
		if (error == ERR_STARTUP)
			LED = DRVLED_OFF;
		else
			LED = DRVLED_ERROR;
	else if (LED == DRVLED_ERROR)
		LED = DRVLED_OFF;
	the_iec->UpdateLEDs();
}
