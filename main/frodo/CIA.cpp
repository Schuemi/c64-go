/*
 *  CIA.cpp - 6526 emulation
 *
 *  Frodo (C) 1994-1997,2002 Christian Bauer
 *

 *
 * Notes:
 * ------
 *
 *  - The EmulateLine() function is called for every emulated raster
 *    line. It counts down the timers and triggers interrupts if
 *    necessary.
 *  - The TOD clocks are counted by CountTOD() during the VBlank, so
 *    the input frequency is 50Hz
 *  - The fields KeyMatrix and RevMatrix contain one bit for each
 *    key on the C64 keyboard (0: key pressed, 1: key released).
 *    KeyMatrix is used for normal keyboard polling (PRA->PRB),
 *    RevMatrix for reversed polling (PRB->PRA).
 *
 * Incompatibilities:
 * ------------------
 *
 *  - The TOD clock should not be stopped on a read access, but
 *    latched
 *  - The SDR interrupt is faked
 */

#include "sysdeps.h"

#include "CIA.h"
#include "CPUC64.h"
#include "CPU1541.h"
#include "VIC.h"
#include "Prefs.h"


/*
 *  Constructors
 */

MOS6526::MOS6526(MOS6510 *CPU) : the_cpu(CPU) {}
MOS6526_1::MOS6526_1(MOS6510 *CPU, MOS6569 *VIC) : MOS6526(CPU), the_vic(VIC) {}
MOS6526_2::MOS6526_2(MOS6510 *CPU, MOS6569 *VIC, MOS6502_1541 *CPU1541) : MOS6526(CPU), the_vic(VIC), the_cpu_1541(CPU1541) {}


/*
 *  Reset the CIA
 */

void MOS6526::Reset(void)
{
	pra = prb = ddra = ddrb = 0;

	ta = tb = 0xffff;
	latcha = latchb = 1;

	tod_10ths = tod_sec = tod_min = tod_hr = 0;
	alm_10ths = alm_sec = alm_min = alm_hr = 0;

	sdr = icr = cra = crb = int_mask = 0;

	tod_halt = ta_cnt_phi2 = tb_cnt_phi2 = tb_cnt_ta = false;
	tod_divider = 0;
}

void MOS6526_1::Reset(void)
{
	MOS6526::Reset();

	// Clear keyboard matrix and joystick states
	for (int i=0; i<8; i++)
		KeyMatrix[i] = RevMatrix[i] = 0xff;

	Joystick1 = Joystick2 = 0xff;
	prev_lp = 0x10;
}

void MOS6526_2::Reset(void)
{
	MOS6526::Reset();

	// VA14/15 = 0
	the_vic->ChangedVA(0);

	// IEC
	IECLines = 0xd0;
}


/*
 *  Get CIA state
 */

void MOS6526::GetState(MOS6526State *cs)
{
	cs->pra = pra;
	cs->prb = prb;
	cs->ddra = ddra;
	cs->ddrb = ddrb;

	cs->ta_lo = ta & 0xff;
	cs->ta_hi = ta >> 8;
	cs->tb_lo = tb & 0xff;
	cs->tb_hi = tb >> 8;
	cs->latcha = latcha;
	cs->latchb = latchb;
	cs->cra = cra;
	cs->crb = crb;

	cs->tod_10ths = tod_10ths;
	cs->tod_sec = tod_sec;
	cs->tod_min = tod_min;
	cs->tod_hr = tod_hr;
	cs->alm_10ths = alm_10ths;
	cs->alm_sec = alm_sec;
	cs->alm_min = alm_min;
	cs->alm_hr = alm_hr;

	cs->sdr = sdr;

	cs->int_data = icr;
	cs->int_mask = int_mask;
}


/*
 *  Restore CIA state
 */

void MOS6526::SetState(MOS6526State *cs)
{
	pra = cs->pra;
	prb = cs->prb;
	ddra = cs->ddra;
	ddrb = cs->ddrb;

	ta = (cs->ta_hi << 8) | cs->ta_lo;
	tb = (cs->tb_hi << 8) | cs->tb_lo;
	latcha = cs->latcha;
	latchb = cs->latchb;
	cra = cs->cra;
	crb = cs->crb;

	tod_10ths = cs->tod_10ths;
	tod_sec = cs->tod_sec;
	tod_min = cs->tod_min;
	tod_hr = cs->tod_hr;
	alm_10ths = cs->alm_10ths;
	alm_sec = cs->alm_sec;
	alm_min = cs->alm_min;
	alm_hr = cs->alm_hr;

	sdr = cs->sdr;

	icr = cs->int_data;
	int_mask = cs->int_mask;

	tod_halt = false;
	ta_cnt_phi2 = ((cra & 0x21) == 0x01);
	tb_cnt_phi2 = ((crb & 0x61) == 0x01);
	tb_cnt_ta = ((crb & 0x61) == 0x41);
}


/*
 *  Read from register (CIA 1)
 */

uint8 MOS6526_1::ReadRegister(uint16 adr)
{
	switch (adr) {
		case 0x00: {
			uint8 ret = pra | ~ddra, tst = (prb | ~ddrb) & Joystick1;
			if (!(tst & 0x01)) ret &= RevMatrix[0];	// AND all active columns
			if (!(tst & 0x02)) ret &= RevMatrix[1];
			if (!(tst & 0x04)) ret &= RevMatrix[2];
			if (!(tst & 0x08)) ret &= RevMatrix[3];
			if (!(tst & 0x10)) ret &= RevMatrix[4];
			if (!(tst & 0x20)) ret &= RevMatrix[5];
			if (!(tst & 0x40)) ret &= RevMatrix[6];
			if (!(tst & 0x80)) ret &= RevMatrix[7];
			return ret & Joystick2;
		}
		case 0x01: {
			uint8 ret = ~ddrb, tst = (pra | ~ddra) & Joystick2;
			if (!(tst & 0x01)) ret &= KeyMatrix[0];	// AND all active rows
			if (!(tst & 0x02)) ret &= KeyMatrix[1];
			if (!(tst & 0x04)) ret &= KeyMatrix[2];
			if (!(tst & 0x08)) ret &= KeyMatrix[3];
			if (!(tst & 0x10)) ret &= KeyMatrix[4];
			if (!(tst & 0x20)) ret &= KeyMatrix[5];
			if (!(tst & 0x40)) ret &= KeyMatrix[6];
			if (!(tst & 0x80)) ret &= KeyMatrix[7];
			return (ret | (prb & ddrb)) & Joystick1;
		}
		case 0x02: return ddra;
		case 0x03: return ddrb;
		case 0x04: return ta;
		case 0x05: return ta >> 8;
		case 0x06: return tb;
		case 0x07: return tb >> 8;
		case 0x08: tod_halt = false; return tod_10ths;
		case 0x09: return tod_sec;
		case 0x0a: return tod_min;
		case 0x0b: tod_halt = true; return tod_hr;
		case 0x0c: return sdr;
		case 0x0d: {
			uint8 ret = icr;		// Read and clear ICR
			icr = 0;
			the_cpu->ClearCIAIRQ();	// Clear IRQ
			return ret;
		}
		case 0x0e: return cra;
		case 0x0f: return crb;
	}
	return 0;	// Can't happen
}


/*
 *  Read from register (CIA 2)
 */

uint8 MOS6526_2::ReadRegister(uint16 adr)
{
	switch (adr) {
		case 0x00:
			return (pra | ~ddra) & 0x3f
				| IECLines & the_cpu_1541->IECLines;
		case 0x01: return prb | ~ddrb;
		case 0x02: return ddra;
		case 0x03: return ddrb;
		case 0x04: return ta;
		case 0x05: return ta >> 8;
		case 0x06: return tb;
		case 0x07: return tb >> 8;
		case 0x08: tod_halt = false; return tod_10ths;
		case 0x09: return tod_sec;
		case 0x0a: return tod_min;
		case 0x0b: tod_halt = true; return tod_hr;
		case 0x0c: return sdr;
		case 0x0d: {
			uint8 ret = icr;		// Read and clear ICR
			icr = 0;
			the_cpu->ClearNMI();	// Clear NMI
			return ret;
		}
		case 0x0e: return cra;
		case 0x0f: return crb;
	}
	return 0;	// Can't happen
}


/*
 *  Write to register (CIA 1)
 */

// Write to port B, check for lightpen interrupt
inline void MOS6526_1::check_lp(void)
{
	if ((prb | ~ddrb) & 0x10 != prev_lp)
		the_vic->TriggerLightpen();
	prev_lp = (prb | ~ddrb) & 0x10;
}

void MOS6526_1::WriteRegister(uint16 adr, uint8 byte)
{
	switch (adr) {
		case 0x0: pra = byte; break;
		case 0x1:
			prb = byte;
			check_lp();
			break;
		case 0x2: ddra = byte; break;
		case 0x3:
			ddrb = byte;
			check_lp();
			break;

		case 0x4: latcha = (latcha & 0xff00) | byte; break;
		case 0x5:
			latcha = (latcha & 0xff) | (byte << 8);
			if (!(cra & 1))	// Reload timer if stopped
				ta = latcha;
			break;

		case 0x6: latchb = (latchb & 0xff00) | byte; break;
		case 0x7:
			latchb = (latchb & 0xff) | (byte << 8);
			if (!(crb & 1))	// Reload timer if stopped
				tb = latchb;
			break;

		case 0x8:
			if (crb & 0x80)
				alm_10ths = byte & 0x0f;
			else
				tod_10ths = byte & 0x0f;
			break;
		case 0x9:
			if (crb & 0x80)
				alm_sec = byte & 0x7f;
			else
				tod_sec = byte & 0x7f;
			break;
		case 0xa:
			if (crb & 0x80)
				alm_min = byte & 0x7f;
			else
				tod_min = byte & 0x7f;
			break;
		case 0xb:
			if (crb & 0x80)
				alm_hr = byte & 0x9f;
			else
				tod_hr = byte & 0x9f;
			break;

		case 0xc:
			sdr = byte;
			TriggerInterrupt(8);	// Fake SDR interrupt for programs that need it
			break;

		case 0xd:
			if (ThePrefs.CIAIRQHack)	// Hack for addressing modes that read from the address
				icr = 0;
			if (byte & 0x80) {
				int_mask |= byte & 0x7f;
				if (icr & int_mask & 0x1f) { // Trigger IRQ if pending
					icr |= 0x80;
					the_cpu->TriggerCIAIRQ();
				}
			} else
				int_mask &= ~byte;
			break;

		case 0xe:
			cra = byte & 0xef;
			if (byte & 0x10) // Force load
				ta = latcha;
			ta_cnt_phi2 = ((byte & 0x21) == 0x01);
			break;

		case 0xf:
			crb = byte & 0xef;
			if (byte & 0x10) // Force load
				tb = latchb;
			tb_cnt_phi2 = ((byte & 0x61) == 0x01);
			tb_cnt_ta = ((byte & 0x61) == 0x41);
			break;
	}
}


/*
 *  Write to register (CIA 2)
 */

void MOS6526_2::WriteRegister(uint16 adr, uint8 byte)
{
	switch (adr) {
		case 0x0:{
			pra = byte;
			byte = ~pra & ddra;
			the_vic->ChangedVA(byte & 3);
			uint8 old_lines = IECLines;
			IECLines = (byte << 2) & 0x80	// DATA
				| (byte << 2) & 0x40		// CLK
				| (byte << 1) & 0x10;		// ATN
			if ((IECLines ^ old_lines) & 0x10) {	// ATN changed
				the_cpu_1541->NewATNState();
				if (old_lines & 0x10)				// ATN 1->0
					the_cpu_1541->IECInterrupt();
			}
			break;
		}
		case 0x1: prb = byte; break;

		case 0x2:
			ddra = byte;
			the_vic->ChangedVA(~(pra | ~ddra) & 3);
			break;
		case 0x3: ddrb = byte; break;

		case 0x4: latcha = (latcha & 0xff00) | byte; break;
		case 0x5:
			latcha = (latcha & 0xff) | (byte << 8);
			if (!(cra & 1))	// Reload timer if stopped
				ta = latcha;
			break;

		case 0x6: latchb = (latchb & 0xff00) | byte; break;
		case 0x7:
			latchb = (latchb & 0xff) | (byte << 8);
			if (!(crb & 1))	// Reload timer if stopped
				tb = latchb;
			break;

		case 0x8:
			if (crb & 0x80)
				alm_10ths = byte & 0x0f;
			else
				tod_10ths = byte & 0x0f;
			break;
		case 0x9:
			if (crb & 0x80)
				alm_sec = byte & 0x7f;
			else
				tod_sec = byte & 0x7f;
			break;
		case 0xa:
			if (crb & 0x80)
				alm_min = byte & 0x7f;
			else
				tod_min = byte & 0x7f;
			break;
		case 0xb:
			if (crb & 0x80)
				alm_hr = byte & 0x9f;
			else
				tod_hr = byte & 0x9f;
			break;

		case 0xc:
			sdr = byte;
			TriggerInterrupt(8);	// Fake SDR interrupt for programs that need it
			break;

		case 0xd:
			if (ThePrefs.CIAIRQHack)
				icr = 0;
			if (byte & 0x80) {
				int_mask |= byte & 0x7f;
				if (icr & int_mask & 0x1f) { // Trigger NMI if pending
					icr |= 0x80;
					the_cpu->TriggerNMI();
				}
			} else
				int_mask &= ~byte;
			break;

		case 0xe:
			cra = byte & 0xef;
			if (byte & 0x10) // Force load
				ta = latcha;
			ta_cnt_phi2 = ((byte & 0x21) == 0x01);
			break;

		case 0xf:
			crb = byte & 0xef;
			if (byte & 0x10) // Force load
				tb = latchb;
			tb_cnt_phi2 = ((byte & 0x61) == 0x01);
			tb_cnt_ta = ((byte & 0x61) == 0x41);
			break;
	}
}


/*
 *  Count CIA TOD clock (called during VBlank)
 */

void MOS6526::CountTOD(void)
{
	uint8 lo, hi;

	// Decrement frequency divider
	if (tod_divider)
		tod_divider--;
	else {

		// Reload divider according to 50/60 Hz flag
		if (cra & 0x80)
			tod_divider = 4;
		else
			tod_divider = 5;

		// 1/10 seconds
		tod_10ths++;
		if (tod_10ths > 9) {
			tod_10ths = 0;

			// Seconds
			lo = (tod_sec & 0x0f) + 1;
			hi = tod_sec >> 4;
			if (lo > 9) {
				lo = 0;
				hi++;
			}
			if (hi > 5) {
				tod_sec = 0;

				// Minutes
				lo = (tod_min & 0x0f) + 1;
				hi = tod_min >> 4;
				if (lo > 9) {
					lo = 0;
					hi++;
				}
				if (hi > 5) {
					tod_min = 0;

					// Hours
					lo = (tod_hr & 0x0f) + 1;
					hi = (tod_hr >> 4) & 1;
					tod_hr &= 0x80;		// Keep AM/PM flag
					if (lo > 9) {
						lo = 0;
						hi++;
					}
					tod_hr |= (hi << 4) | lo;
					if ((tod_hr & 0x1f) > 0x11)
						tod_hr = tod_hr & 0x80 ^ 0x80;
				} else
					tod_min = (hi << 4) | lo;
			} else
				tod_sec = (hi << 4) | lo;
		}

		// Alarm time reached? Trigger interrupt if enabled
		if (tod_10ths == alm_10ths && tod_sec == alm_sec &&
			tod_min == alm_min && tod_hr == alm_hr)
			TriggerInterrupt(4);
	}
}


/*
 *  Trigger IRQ (CIA 1)
 */

void MOS6526_1::TriggerInterrupt(int bit)
{
	icr |= bit;
	if (int_mask & bit) {
		icr |= 0x80;
		the_cpu->TriggerCIAIRQ();
	}
}


/*
 *  Trigger NMI (CIA 2)
 */

void MOS6526_2::TriggerInterrupt(int bit)
{
	icr |= bit;
	if (int_mask & bit) {
		icr |= 0x80;
		the_cpu->TriggerNMI();
	}
}
