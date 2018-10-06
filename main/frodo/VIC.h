/*
 *  VIC.h - 6569R5 emulation (line based)
 *
 *  Frodo (C) 1994-1997,2002 Christian Bauer
 */

#ifndef _VIC_H
#define _VIC_H


// Define this if you want global variables instead of member variables
#if defined(__i386) || defined(mc68000) || defined(__MC68K__)
#define GLOBAL_VARS
#endif

// Define this if you have a processor that can do unaligned accesses quickly
#if defined(__i386) || defined(mc68000) || defined(__MC68K__)
#define CAN_ACCESS_UNALIGNED
#endif


// Total number of raster lines (PAL)
const unsigned TOTAL_RASTERS = 0x138;

// Screen refresh frequency (PAL)
const unsigned SCREEN_FREQ = 50;


class MOS6510;
class C64Display;
class C64;
struct MOS6569State;


class MOS6569 {
public:
	MOS6569(C64 *c64, C64Display *disp, MOS6510 *CPU, uint8 *RAM, uint8 *Char, uint8 *Color);

	uint8 ReadRegister(uint16 adr);
	void WriteRegister(uint16 adr, uint8 byte);
#ifdef FRODO_SC
	bool EmulateCycle(void);
#else
	int EmulateLine(void);
#endif
	void ChangedVA(uint16 new_va);	// CIA VA14/15 has changed
	void TriggerLightpen(void);		// Trigger lightpen interrupt
	void ReInitColors(void);
	void GetState(MOS6569State *vd);
	void SetState(MOS6569State *vd);

#ifdef FRODO_SC
	uint8 LastVICByte;
#endif

private:
#ifndef GLOBAL_VARS
	void vblank(void);
	void raster_irq(void);

	uint16 mx[8];				// VIC registers
	uint8 my[8];
	uint8 mx8;
	uint8 ctrl1, ctrl2;
	uint8 lpx, lpy;
	uint8 me, mxe, mye, mdp, mmc;
	uint8 vbase;
	uint8 irq_flag, irq_mask;
	uint8 clx_spr, clx_bgr;
	uint8 ec, b0c, b1c, b2c, b3c, mm0, mm1;
	uint8 sc[8];

	uint8 *ram, *char_rom, *color_ram; // Pointers to RAM and ROM
	C64 *the_c64;				// Pointer to C64
	C64Display *the_display;	// Pointer to C64Display
	MOS6510 *the_cpu;			// Pointer to 6510

	uint8 colors[256];			// Indices of the 16 C64 colors (16 times mirrored to avoid "& 0x0f")

	uint8 ec_color, b0c_color, b1c_color,
	      b2c_color, b3c_color;	// Indices for exterior/background colors
	uint8 mm0_color, mm1_color;	// Indices for MOB multicolors
	uint8 spr_color[8];			// Indices for MOB colors

	uint32 ec_color_long;		// ec_color expanded to 32 bits

	uint8 matrix_line[40];		// Buffer for video line, read in Bad Lines
	uint8 color_line[40];		// Buffer for color line, read in Bad Lines

#ifdef __POWERPC__
	double chunky_tmp[0x180/8];	// Temporary line buffer for speedup
#endif
	uint8 *chunky_line_start;	// Pointer to start of current line in bitmap buffer
	int xmod;					// Number of bytes per row

	uint16 raster_y;				// Current raster line
	uint16 irq_raster;			// Interrupt raster line
	uint16 dy_start;				// Comparison values for border logic
	uint16 dy_stop;
	uint16 rc;					// Row counter
	uint16 vc;					// Video counter
	uint16 vc_base;				// Video counter base
	uint16 x_scroll;				// X scroll value
	uint16 y_scroll;				// Y scroll value
	uint16 cia_vabase;			// CIA VA14/15 video base

	uint16 mc[8];				// Sprite data counters

	int display_idx;			// Index of current display mode
	int skip_counter;			// Counter for frame-skipping

	long pad0;	// Keep buffers long-aligned
	uint8 spr_coll_buf[0x180];	// Buffer for sprite-sprite collisions and priorities
	uint8 fore_mask_buf[0x180/8];	// Foreground mask for sprite-graphics collisions and priorities
#ifndef CAN_ACCESS_UNALIGNED
	uint8 text_chunky_buf[40*8];	// Line graphics buffer
#endif

	bool display_state;			// true: Display state, false: Idle state
	bool border_on;				// Flag: Upper/lower border on (Frodo SC: Main border flipflop)
	bool frame_skipped;			// Flag: Frame is being skipped
	uint8 bad_lines_enabled;	// Flag: Bad Lines enabled for this frame
	bool lp_triggered;			// Flag: Lightpen was triggered in this frame

#ifdef FRODO_SC
	uint8 read_byte(uint16 adr);
	void matrix_access(void);
	void graphics_access(void);
	void draw_graphics(void);
	void draw_sprites(void);
	void draw_background(void);

	int cycle;					// Current cycle in line (1..63)

	uint8 *chunky_ptr;			// Pointer in chunky bitmap buffer (this is where out output goes)
	uint8 *fore_mask_ptr;		// Pointer in fore_mask_buf

	uint16 matrix_base;			// Video matrix base
	uint16 char_base;			// Character generator base
	uint16 bitmap_base;			// Bitmap base

	bool is_bad_line;			// Flag: Current line is bad line
	bool draw_this_line;		// Flag: This line is drawn on the screen
	bool ud_border_on;			// Flag: Upper/lower border on
	bool vblanking;				// Flag: VBlank in next cycle

	bool border_on_sample[5];	// Samples of border state at different cycles (1, 17, 18, 56, 57)
	uint8 border_color_sample[0x180/8];	// Samples of border color at each "displayed" cycle

	uint8 ref_cnt;				// Refresh counter
	uint8 spr_exp_y;			// 8 sprite y expansion flipflops
	uint8 spr_dma_on;			// 8 flags: Sprite DMA active
	uint8 spr_disp_on;			// 8 flags: Sprite display active
	uint8 spr_draw;				// 8 flags: Draw sprite in this line
	uint16 spr_ptr[8];			// Sprite data pointers
	uint16 mc_base[8];			// Sprite data counter bases

	uint16 raster_x;			// Current raster x position

	int ml_index;				// Index in matrix/color_line[]
	uint8 gfx_data, char_data, color_data, last_char_data;
	uint8 spr_data[8][4];		// Sprite data read
	uint8 spr_draw_data[8][4];	// Sprite data for drawing

	uint32 first_ba_cycle;		// Cycle when BA first went low
#else
	uint8 *get_physical(uint16 adr);
	void make_mc_table(void);
	void el_std_text(uint8 *p, uint8 *q, uint8 *r);
	void el_mc_text(uint8 *p, uint8 *q, uint8 *r);
	void el_std_bitmap(uint8 *p, uint8 *q, uint8 *r);
	void el_mc_bitmap(uint8 *p, uint8 *q, uint8 *r);
	void el_ecm_text(uint8 *p, uint8 *q, uint8 *r);
	void el_std_idle(uint8 *p, uint8 *r);
	void el_mc_idle(uint8 *p, uint8 *r);
	void el_sprites(uint8 *chunky_ptr);
	int el_update_mc(int raster);

	uint16 mc_color_lookup[4];

	bool border_40_col;			// Flag: 40 column border
	uint8 sprite_on;			// 8 flags: Sprite display/DMA active

	uint8 *matrix_base;			// Video matrix base
	uint8 *char_base;			// Character generator base
	uint8 *bitmap_base;			// Bitmap base
#endif
#endif
};


// VIC state
struct MOS6569State {
	uint8 m0x;				// Sprite coordinates
	uint8 m0y;
	uint8 m1x;
	uint8 m1y;
	uint8 m2x;
	uint8 m2y;
	uint8 m3x;
	uint8 m3y;
	uint8 m4x;
	uint8 m4y;
	uint8 m5x;
	uint8 m5y;
	uint8 m6x;
	uint8 m6y;
	uint8 m7x;
	uint8 m7y;
	uint8 mx8;

	uint8 ctrl1;			// Control registers
	uint8 raster;
	uint8 lpx;
	uint8 lpy;
	uint8 me;
	uint8 ctrl2;
	uint8 mye;
	uint8 vbase;
	uint8 irq_flag;
	uint8 irq_mask;
	uint8 mdp;
	uint8 mmc;
	uint8 mxe;
	uint8 mm;
	uint8 md;

	uint8 ec;				// Color registers
	uint8 b0c;
	uint8 b1c;
	uint8 b2c;
	uint8 b3c;
	uint8 mm0;
	uint8 mm1;
	uint8 m0c;
	uint8 m1c;
	uint8 m2c;
	uint8 m3c;
	uint8 m4c;
	uint8 m5c;
	uint8 m6c;
	uint8 m7c;
							// Additional registers
	uint8 pad0;
	uint16 irq_raster;		// IRQ raster line
	uint16 vc;				// Video counter
	uint16 vc_base;			// Video counter base
	uint8 rc;				// Row counter
	uint8 spr_dma;			// 8 Flags: Sprite DMA active
	uint8 spr_disp;			// 8 Flags: Sprite display active
	uint8 mc[8];			// Sprite data counters
	uint8 mc_base[8];		// Sprite data counter bases
	bool display_state;		// true: Display state, false: Idle state
	bool bad_line;			// Flag: Bad Line state
	bool bad_line_enable;	// Flag: Bad Lines enabled for this frame
	bool lp_triggered;		// Flag: Lightpen was triggered in this frame
	bool border_on;			// Flag: Upper/lower border on (Frodo SC: Main border flipflop)

	uint16 bank_base;		// VIC bank base address
	uint16 matrix_base;		// Video matrix base
	uint16 char_base;		// Character generator base
	uint16 bitmap_base;		// Bitmap base
	uint16 sprite_base[8];	// Sprite bases

							// Frodo SC:
	int cycle;				// Current cycle in line (1..63)
	uint16 raster_x;		// Current raster x position
	int ml_index;			// Index in matrix/color_line[]
	uint8 ref_cnt;			// Refresh counter
	uint8 last_vic_byte;	// Last byte read by VIC
	bool ud_border_on;		// Flag: Upper/lower border on
};

#endif
