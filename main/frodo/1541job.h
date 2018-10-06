/*
 *  1541job.h - Emulation of 1541 GCR disk reading/writing
 *
 *  Frodo (C) 1994-1997,2002 Christian Bauer
 */

#ifndef _1541JOB_H
#define _1541JOB_H


class MOS6502_1541;
class Prefs;
struct Job1541State;

class Job1541 {
public:
	Job1541(uint8 *ram1541);
	~Job1541();

	void GetState(Job1541State *state);
	void SetState(Job1541State *state);
	void NewPrefs(Prefs *prefs);
	void MoveHeadOut(void);
	void MoveHeadIn(void);
	bool SyncFound(void);
	uint8 ReadGCRByte(void);
	uint8 WPState(void);
	void WriteSector(void);
	void FormatTrack(void);

private:
	void open_d64_file(char *filepath);
	void close_d64_file(void);
	bool read_sector(int track, int sector, uint8 *buffer);
	bool write_sector(int track, int sector, uint8 *buffer);
	void format_disk(void);
	int secnum_from_ts(int track, int sector);
	int offset_from_ts(int track, int sector);
	void gcr_conv4(uint8 *from, uint8 *to);
	void sector2gcr(int track, int sector);
	void disk2gcr(void);

	uint8 *ram;				// Pointer to 1541 RAM
	FILE *the_file;			// File pointer for .d64 file
	int image_header;		// Length of .d64/.x64 file header

	uint8 id1, id2;			// ID of disk
	uint8 *error_info; //[683];	// Sector error information (1 byte/sector)

	uint8 *gcr_data;		// Pointer to GCR encoded disk data
	uint8 *gcr_ptr;			// Pointer to GCR data under R/W head
	uint8 *gcr_track_start;	// Pointer to start of GCR data of current track
	uint8 *gcr_track_end;	// Pointer to end of GCR data of current track
	int current_halftrack;	// Current halftrack number (2..70)

	bool write_protected;	// Flag: Disk write-protected
	bool disk_changed;		// Flag: Disk changed (WP sensor strobe control)
};

// 1541 GCR state
struct Job1541State {
	int current_halftrack;
	uint32 gcr_ptr;
	bool write_protected;
	bool disk_changed;
};


/*
 *  Check if R/W head is over SYNC
 */

inline bool Job1541::SyncFound(void)
{
	if (*gcr_ptr == 0xff)
		return true;
	else {
		gcr_ptr++;		// Rotate disk
		if (gcr_ptr == gcr_track_end)
			gcr_ptr = gcr_track_start;
		return false;
	}
}


/*
 *  Read one GCR byte from disk
 */

inline uint8 Job1541::ReadGCRByte(void)
{
	uint8 byte = *gcr_ptr++;	// Rotate disk
	if (gcr_ptr == gcr_track_end)
		gcr_ptr = gcr_track_start;
	return byte;
}


/*
 *  Return state of write protect sensor
 */

inline uint8 Job1541::WPState(void)
{
	if (disk_changed) {	// Disk change -> WP sensor strobe
		disk_changed = false;
		return write_protected ? 0x10 : 0;
	} else
		return write_protected ? 0 : 0x10;
}

#endif
