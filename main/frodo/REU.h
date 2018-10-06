/*
 *  REU.h - 17xx REU emulation
 *
 *  Frodo (C) 1994-1997,2002 Christian Bauer
 */

#ifndef _REU_H
#define _REU_H


class MOS6510;
class Prefs;

class REU {
public:
	REU(MOS6510 *CPU);
	~REU();

	void NewPrefs(Prefs *prefs);
	void Reset(void);
	uint8 ReadRegister(uint16 adr);
	void WriteRegister(uint16 adr, uint8 byte);
	void FF00Trigger(void);

private:
	void open_close_reu(int old_size, int new_size);
	void execute_dma(void);

	MOS6510 *the_cpu;	// Pointer to 6510

	uint8 *ex_ram;		// REU expansion RAM

	uint32 ram_size;		// Size of expansion RAM
	uint32 ram_mask;		// Expansion RAM address bit mask

	uint8 regs[16];		// REU registers
};

#endif
