/*
 *  CIA.h - 6526 emulation
 *
 *  Frodo (C) 1994-1997,2002 Christian Bauer
 */

#ifndef _CIA_H
#define _CIA_H

#include "Prefs.h"


class MOS6510;
class MOS6502_1541;
class MOS6569;
struct MOS6526State;


class MOS6526 {
public:
	MOS6526(MOS6510 *CPU);

	void Reset(void);
	void GetState(MOS6526State *cs);
	void SetState(MOS6526State *cs);
#ifdef FRODO_SC
	void CheckIRQs(void);
	void EmulateCycle(void);
#else
	void EmulateLine(int cycles);
#endif
	void CountTOD(void);
	virtual void TriggerInterrupt(int bit)=0;

protected:
	MOS6510 *the_cpu;	// Pointer to 6510

	uint8 pra, prb, ddra, ddrb;

	uint16 ta, tb, latcha, latchb;

	uint8 tod_10ths, tod_sec, tod_min, tod_hr;
	uint8 alm_10ths, alm_sec, alm_min, alm_hr;

	uint8 sdr, icr, cra, crb;
	uint8 int_mask;

	int tod_divider;	// TOD frequency divider

	bool tod_halt,		// Flag: TOD halted
		 ta_cnt_phi2,	// Flag: Timer A is counting Phi 2
		 tb_cnt_phi2,	// Flag: Timer B is counting Phi 2
	     tb_cnt_ta;		// Flag: Timer B is counting underflows of Timer A

#ifdef FRODO_SC
	bool ta_irq_next_cycle,		// Flag: Trigger TA IRQ in next cycle
		 tb_irq_next_cycle,		// Flag: Trigger TB IRQ in next cycle
		 has_new_cra,			// Flag: New value for CRA pending
		 has_new_crb;			// Flag: New value for CRB pending
	char ta_state, tb_state;	// Timer A/B states
	uint8 new_cra, new_crb;		// New values for CRA/CRB
#endif
};


class MOS6526_1 : public MOS6526 {
public:
	MOS6526_1(MOS6510 *CPU, MOS6569 *VIC);

	void Reset(void);
	uint8 ReadRegister(uint16 adr);
	void WriteRegister(uint16 adr, uint8 byte);
	virtual void TriggerInterrupt(int bit);

	uint8 KeyMatrix[8];	// C64 keyboard matrix, 1 bit/key (0: key down, 1: key up)
	uint8 RevMatrix[8];	// Reversed keyboard matrix

	uint8 Joystick1;	// Joystick 1 AND value
	uint8 Joystick2;	// Joystick 2 AND value

private:
	void check_lp(void);

	MOS6569 *the_vic;

	uint8 prev_lp;		// Previous state of LP line (bit 4)
};


class MOS6526_2 : public MOS6526{
public:
	MOS6526_2(MOS6510 *CPU, MOS6569 *VIC, MOS6502_1541 *CPU1541);

	void Reset(void);
	uint8 ReadRegister(uint16 adr);
	void WriteRegister(uint16 adr, uint8 byte);
	virtual void TriggerInterrupt(int bit);

	uint8 IECLines;		// State of IEC lines (bit 7 - DATA, bit 6 - CLK, bit 4 - ATN)

private:
	MOS6569 *the_vic;
	MOS6502_1541 *the_cpu_1541;
};


// CIA state
struct MOS6526State {
	uint8 pra;
	uint8 ddra;
	uint8 prb;
	uint8 ddrb;
	uint8 ta_lo;
	uint8 ta_hi;
	uint8 tb_lo;
	uint8 tb_hi;
	uint8 tod_10ths;
	uint8 tod_sec;
	uint8 tod_min;
	uint8 tod_hr;
	uint8 sdr;
	uint8 int_data;		// Pending interrupts
	uint8 cra;
	uint8 crb;
						// Additional registers
	uint16 latcha;		// Timer latches
	uint16 latchb;
	uint8 alm_10ths;	// Alarm time
	uint8 alm_sec;
	uint8 alm_min;
	uint8 alm_hr;
	uint8 int_mask;		// Enabled interrupts
};


/*
 *  Emulate CIA for one cycle/raster line
 */

#ifdef FRODO_SC
inline void MOS6526::CheckIRQs(void)
{
	// Trigger pending interrupts
	if (ta_irq_next_cycle) {
		ta_irq_next_cycle = false;
		TriggerInterrupt(1);
	}
	if (tb_irq_next_cycle) {
		tb_irq_next_cycle = false;
		TriggerInterrupt(2);
	}
}
#else
inline void MOS6526::EmulateLine(int cycles)
{
	unsigned long tmp;

	// Timer A
	if (ta_cnt_phi2) {
		ta = tmp = ta - cycles;		// Decrement timer

		if (tmp > 0xffff) {			// Underflow?
			ta = latcha;			// Reload timer

			if (cra & 8) {			// One-shot?
				cra &= 0xfe;
				ta_cnt_phi2 = false;
			}
			TriggerInterrupt(1);
			if (tb_cnt_ta) {		// Timer B counting underflows of Timer A?
				tb = tmp = tb - 1;	// tmp = --tb doesn't work
				if (tmp > 0xffff) goto tb_underflow;
			}
		}
	}

	// Timer B
	if (tb_cnt_phi2) {
		tb = tmp = tb - cycles;		// Decrement timer

		if (tmp > 0xffff) {			// Underflow?
tb_underflow:
			tb = latchb;

			if (crb & 8) {			// One-shot?
				crb &= 0xfe;
				tb_cnt_phi2 = false;
				tb_cnt_ta = false;
			}
			TriggerInterrupt(2);
		}
	}
}
#endif

#endif
