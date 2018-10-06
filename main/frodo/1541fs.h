/*
 *  1541fs.h - 1541 emulation in host file system
 *
 *  Frodo (C) 1994-1997,2002 Christian Bauer
 */

#ifndef _1541FS_H
#define _1541FS_H

#include "IEC.h"


class FSDrive : public Drive {
public:
	FSDrive(IEC *iec, char *path);
	virtual ~FSDrive();
	virtual uint8 Open(int channel, char *filename);
	virtual uint8 Close(int channel);
	virtual uint8 Read(int channel, uint8 *byte);
	virtual uint8 Write(int channel, uint8 byte, bool eoi);
	virtual void Reset(void);

private:
	bool change_dir(char *dirpath);
	uint8 open_file(int channel, char *filename);
	uint8 open_directory(int channel, char *filename);
	void convert_filename(char *srcname, char *destname, int *filemode, int *filetype, bool *wildflag);
	void find_first_file(char *name);
	void close_all_channels(void);
	void execute_command(char *command);
	void chdir_cmd(char *dirpath);
	uint8 conv_from_64(uint8 c, bool map_slash);
	uint8 conv_to_64(uint8 c, bool map_slash);

	char dir_path[256];		// Path to directory
	char orig_dir_path[256]; // Original directory path
	char dir_title[16];		// Directory title
	FILE *file[16];			// File pointers for each of the 16 channels

	char cmd_buffer[44];	// Buffer for incoming command strings
	int cmd_len;			// Length of received command

	uint8 read_char[16];	// Buffers for one-byte read-ahead
};

#endif
