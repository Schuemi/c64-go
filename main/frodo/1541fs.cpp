/*
 *  1541fs.cpp - 1541 emulation in host file system
 *
 *  Frodo (C) 1994-1997,2002 Christian Bauer

 *
 * Notes:
 * ------
 *
 *  - If the directory is opened (file name "$"), a temporary file
 *    with the structure of a 1541 directory file is created and
 *    opened. It can then be accessed in the same way as all other
 *    files.
 *
 * Incompatibilities:
 * ------------------
 *
 *  - No "raw" directory reading
 *  - No relative/sequential/user files
 *  - Only "I" and "UJ" commands implemented
 */

#include "sysdeps.h"

#include "1541fs.h"
#include "IEC.h"
#include "main.h"
#include "Prefs.h"

#ifdef __riscos__
#include "ROlib.h"
#endif


// Access modes
enum {
	FMODE_READ, FMODE_WRITE, FMODE_APPEND
};

// File types
enum {
	FTYPE_PRG, FTYPE_SEQ
};

// Prototypes
static bool match(char *p, char *n);


/*
 *  Constructor: Prepare emulation
 */

FSDrive::FSDrive(IEC *iec, char *path) : Drive(iec)
{
	strcpy(orig_dir_path, path);
	dir_path[0] = 0;

	if (change_dir(orig_dir_path)) {
		for (int i=0; i<16; i++)
			file[i] = NULL;

		Reset();

		Ready = true;
	}
}


/*
 *  Destructor
 */

FSDrive::~FSDrive()
{
	if (Ready) {
		close_all_channels();
		Ready = false;
	}
}


/*
 *  Change emulation directory
 */

bool FSDrive::change_dir(char *dirpath)
{
#ifndef __riscos__
	DIR *dir;

	if ((dir = opendir(dirpath)) != NULL) {
		closedir(dir);
		strcpy(dir_path, dirpath);
		strncpy(dir_title, dir_path, 16);
		return true;
	} else
		return false;
#else
	int Info[4];

	if ((ReadCatalogueInfo(dirpath,Info) & 2) != 0)	// Directory or image file
	{
	  strcpy(dir_path, dirpath);
	  strncpy(dir_title, dir_path, 16);
	  return true;
	}
	else
	{
	  return false;
	}
#endif
}


/*
 *  Open channel
 */

uint8 FSDrive::Open(int channel, char *filename)
{
	set_error(ERR_OK);

	// Channel 15: Execute file name as command
	if (channel == 15) {
		execute_command(filename);
		return ST_OK;
	}

	// Close previous file if still open
	if (file[channel]) {
		fclose(file[channel]);
		file[channel] = NULL;
	}

	if (filename[0] == '$')
		return open_directory(channel, filename+1);

	if (filename[0] == '#') {
		set_error(ERR_NOCHANNEL);
		return ST_OK;
	}

	return open_file(channel, filename);
}


/*
 *  Open file
 */

uint8 FSDrive::open_file(int channel, char *filename)
{
	char plainname[NAMEBUF_LENGTH];
	int filemode = FMODE_READ;
	int filetype = FTYPE_PRG;
	bool wildflag = false;
	char *mode = "rb";
	
	convert_filename(filename, plainname, &filemode, &filetype, &wildflag);

	// Channel 0 is READ PRG, channel 1 is WRITE PRG
	if (!channel) {
		filemode = FMODE_READ;
		filetype = FTYPE_PRG;
	}
	if (channel == 1) {
		filemode = FMODE_WRITE;
		filetype = FTYPE_PRG;
	}

	// Wildcards are only allowed on reading
	if (wildflag) {
		if (filemode != FMODE_READ) {
			set_error(ERR_SYNTAX33);
			return ST_OK;
		}
		find_first_file(plainname);
	}

	// Select fopen() mode according to file mode
	switch (filemode) {
		case FMODE_READ:
			mode = "rb";
			break;
		case FMODE_WRITE:
			mode = "wb";
			break;
		case FMODE_APPEND:
			mode = "ab";
			break;
	}

	// Open file
#ifndef __riscos__
	if (chdir(dir_path))
		set_error(ERR_NOTREADY);
	else if ((file[channel] = fopen(plainname, mode)) != NULL) {
		if (filemode == FMODE_READ)	// Read and buffer first byte
			read_char[channel] = fgetc(file[channel]);
	} else
		set_error(ERR_FILENOTFOUND);
	chdir(AppDirPath);
#else
	{
	  char fullname[NAMEBUF_LENGTH];

  	  // On RISC OS make a full filename
	  sprintf(fullname,"%s.%s",dir_path,plainname);
	  if ((file[channel] = fopen(fullname, mode)) != NULL)
	  {
	    if (filemode == FMODE_READ)
	    {
	      read_char[channel] = fgetc(file[channel]);
	    }
	  }
	  else
	  {
	    set_error(ERR_FILENOTFOUND);
	  }
	}
#endif

	return ST_OK;
}


/*
 *  Analyze file name, get access mode and type
 */

void FSDrive::convert_filename(char *srcname, char *destname, int *filemode, int *filetype, bool *wildflag)
{
	char *p, *q;
	int i;

	// Search for ':', p points to first character after ':'
	if ((p = strchr(srcname, ':')) != NULL)
		p++;
	else
		p = srcname;

	// Convert char set of the remaining string -> destname
	q = destname;
	for (i=0; i<NAMEBUF_LENGTH && (*q++ = conv_from_64(*p++, true)); i++) ;

	// Look for mode parameters seperated by ','
	p = destname;
	while ((p = strchr(p, ',')) != NULL) {

		// Cut string after the first ','
		*p++ = 0;

		switch (*p) {
			case 'p':
				*filetype = FTYPE_PRG;
				break;
			case 's':
				*filetype = FTYPE_SEQ;
				break;
			case 'r':
				*filemode = FMODE_READ;
				break;
			case 'w':
				*filemode = FMODE_WRITE;
				break;
			case 'a':
				*filemode = FMODE_APPEND;
				break;
		}
	}

	// Search for wildcards
	*wildflag = (strchr(destname, '?') != NULL) || (strchr(destname, '*') != NULL);
}


/*
 *  Find first file matching wildcard pattern and get its real name
 */

// Return true if name 'n' matches pattern 'p'
static bool match(char *p, char *n)
{
	if (!*p)		// Null pattern matches everything
		return true;

	do {
		if (*p == '*')	// Wildcard '*' matches all following characters
			return true;
		if ((*p != *n) && (*p != '?'))	// Wildcard '?' matches single character
			return false;
		p++; n++;
	} while (*p);

	return !*n;
}

void FSDrive::find_first_file(char *name)
{
#ifndef __riscos__
	DIR *dir;
	struct dirent *de;

	// Open directory for reading and skip '.' and '..'
	if ((dir = opendir(dir_path)) == NULL)
		return;
	de = readdir(dir);
	while (de && (0 == strcmp(".", de->d_name) || 0 == strcmp("..", de->d_name))) 
		de = readdir(dir);

	while (de) {

		// Match found? Then copy real file name
		if (match(name, de->d_name)) {
			strncpy(name, de->d_name, NAMEBUF_LENGTH);
			closedir(dir);
			return;
		}

		// Get next directory entry
		de = readdir(dir);
	}

	closedir(dir);
#else
	dir_env de;
	char Buffer[NAMEBUF_LENGTH];

	de.offset = 0; de.buffsize = NAMEBUF_LENGTH; de.match = name;
	do
	{
	  de.readno = 1;
	  if (ReadDirName(dir_path,Buffer,&de) != NULL) {de.offset = -1;}
	  else if (de.offset != -1)
	  {
	    if (match(name,Buffer))
	    {
	      strncpy(name, Buffer, NAMEBUF_LENGTH);
	      return;
	    }
	  }
	}
	while (de.offset != -1);
#endif
}


/*
 *  Open directory, create temporary file
 */

uint8 FSDrive::open_directory(int channel, char *filename)
{
	char buf[] = "\001\004\001\001\0\0\022\042                \042 00 2A";
	char str[NAMEBUF_LENGTH];
	char pattern[NAMEBUF_LENGTH];
	char *p, *q;
	int i;
	int filemode;
	int filetype;
	bool wildflag;

#ifndef __riscos__
	DIR *dir;
	struct dirent *de;
	struct stat statbuf;

	// Special treatment for "$0"
	if (filename[0] == '0' && filename[1] == 0)
		filename += 1;

	// Convert filename ('$' already stripped), filemode/type are ignored
	convert_filename(filename, pattern, &filemode, &filetype, &wildflag);

	// Open directory for reading and skip '.' and '..'
	if ((dir = opendir(dir_path)) == NULL) {
		set_error(ERR_NOTREADY);
		return ST_OK;
	}
	de = readdir(dir);
	while (de && (0 == strcmp(".", de->d_name) || 0 == strcmp("..", de->d_name))) 
		de = readdir(dir);

	// Create temporary file
	if ((file[channel] = tmpfile()) == NULL) {
		closedir(dir);
		return ST_OK;
	}

	// Create directory title
	p = &buf[8];
	for (i=0; i<16 && dir_title[i]; i++)
		*p++ = conv_to_64(dir_title[i], false);
	fwrite(buf, 1, 32, file[channel]);

	// Create and write one line for every directory entry
	while (de) {

		// Include only files matching the pattern
		if (match(pattern, de->d_name)) {

			// Get file statistics
			chdir(dir_path);
			stat(de->d_name, &statbuf);
			chdir(AppDirPath);

			// Clear line with spaces and terminate with null byte
			memset(buf, ' ', 31);
			buf[31] = 0;

			p = buf;
			*p++ = 0x01;	// Dummy line link
			*p++ = 0x01;

			// Calculate size in blocks (254 bytes each)
			i = (statbuf.st_size + 254) / 254;
			*p++ = i & 0xff;
			*p++ = (i >> 8) & 0xff;

			p++;
			if (i < 10) p++;	// Less than 10: add one space
			if (i < 100) p++;	// Less than 100: add another space

			// Convert and insert file name
			strcpy(str, de->d_name);
			*p++ = '\"';
			q = p;
			for (i=0; i<16 && str[i]; i++)
				*q++ = conv_to_64(str[i], true);
			*q++ = '\"';
			p += 18;

			// File type
			if (S_ISDIR(statbuf.st_mode)) {
				*p++ = 'D';
				*p++ = 'I';
				*p++ = 'R';
			} else {
				*p++ = 'P';
				*p++ = 'R';
				*p++ = 'G';
			}

			// Write line
			fwrite(buf, 1, 32, file[channel]);
		}

		// Get next directory entry
		de = readdir(dir);
	}
#else
	dir_full_info di;
	dir_env de;

	// Much of this is very similar to the original
	if ((filename[0] == '0') && (filename[1] == 0)) {filename++;}
	// Concatenate dir_path and pattern in buffer pattern ==> read subdirs!
	strcpy(pattern,dir_path);
	convert_filename(filename, pattern + strlen(pattern), &filemode, &filetype, &wildflag);

	// We don't use tmpfile() -- problems involved!
	DeleteFile(RO_TEMPFILE);	// first delete it, if it exists
	if ((file[channel] = fopen(RO_TEMPFILE,"wb+")) == NULL)
	{
	  return(ST_OK);
	}
	de.offset = 0; de.buffsize = NAMEBUF_LENGTH; de.match = filename;

	// Create directory title - copied from above
	p = &buf[8];
	for (i=0; i<16 && dir_title[i]; i++)
		*p++ = conv_to_64(dir_title[i], false);
	fwrite(buf, 1, 32, file[channel]);

	do
	{
	  de.readno = 1;
	  if (ReadDirNameInfo(pattern,&di,&de) != NULL) {de.offset = -1;}
	  else if (de.offset != -1)	// don't have to check for match here
	  {
	    memset(buf,' ',31); buf[31] = 0;	// most of this: see above
	    p = buf; *p++ = 0x01; *p++ = 0x01;
	    i = (di.length + 254) / 254; *p++ = i & 0xff; *p++ = (i>>8) & 0xff;
	    p++;
	    if (i < 10)  {*p++ = ' ';}
	    if (i < 100) {*p++ = ' ';}
	    strcpy(str, di.name);
	    *p++ = '\"'; q = p;
	    for (i=0; (i<16 && str[i]); i++)
	    {
	      *q++ = conv_to_64(str[i], true);
	    }
	    *q++ = '\"'; p += 18;
	    if ((di.otype & 2) == 0)
	    {
	      *p++ = 'P'; *p++ = 'R'; *p++ = 'G';
	    }
	    else
	    {
	      *p++ = 'D'; *p++ = 'I'; *p++ = 'R';
	    }
	    fwrite(buf, 1, 32, file[channel]);
	  }
	}
	while (de.offset != -1);
#endif

	// Final line
	fwrite("\001\001\0\0BLOCKS FREE.             \0\0", 1, 32, file[channel]);

	// Rewind file for reading and read first byte
	rewind(file[channel]);
	read_char[channel] = fgetc(file[channel]);

#ifndef __riscos
	// Close directory
	closedir(dir);
#endif

	return ST_OK;
}


/*
 *  Close channel
 */

uint8 FSDrive::Close(int channel)
{
	if (channel == 15) {
		close_all_channels();
		return ST_OK;
	}

	if (file[channel]) {
		fclose(file[channel]);
		file[channel] = NULL;
	}

	return ST_OK;
}


/*
 *  Close all channels
 */

void FSDrive::close_all_channels(void)
{
	for (int i=0; i<15; i++)
		Close(i);

	cmd_len = 0;
}


/*
 *  Read from channel
 */

uint8 FSDrive::Read(int channel, uint8 *byte)
{
	int c;

	// Channel 15: Error channel
	if (channel == 15) {
		*byte = *error_ptr++;

		if (*byte != '\r')
			return ST_OK;
		else {	// End of message
			set_error(ERR_OK);
			return ST_EOF;
		}
	}

	if (!file[channel]) return ST_READ_TIMEOUT;

	// Read one byte
	*byte = read_char[channel];
	c = fgetc(file[channel]);
	if (c == EOF)
		return ST_EOF;
	else {
		read_char[channel] = c;
		return ST_OK;
	}
}


/*
 *  Write to channel
 */

uint8 FSDrive::Write(int channel, uint8 byte, bool eoi)
{
	// Channel 15: Collect chars and execute command on EOI
	if (channel == 15) {
		if (cmd_len >= 40)
			return ST_TIMEOUT;
		
		cmd_buffer[cmd_len++] = byte;

		if (eoi) {
			cmd_buffer[cmd_len] = 0;
			cmd_len = 0;
			execute_command(cmd_buffer);
		}
		return ST_OK;
	}

	if (!file[channel]) {
		set_error(ERR_FILENOTOPEN);
		return ST_TIMEOUT;
	}

	if (fputc(byte, file[channel]) == EOF) {
		set_error(ERR_WRITEERROR);
		return ST_TIMEOUT;
	}

	return ST_OK;
}


/*
 *  Execute command string
 */

void FSDrive::execute_command(char *command)
{
	switch (command[0]) {
		case 'I':
			close_all_channels();
			set_error(ERR_OK);
			break;

		case 'U':
			if ((command[1] & 0x0f) == 0x0a) {
				Reset();
			} else
				set_error(ERR_SYNTAX30);
			break;

		case 'G':
			if (command[1] != ':')
				set_error(ERR_SYNTAX30);
			else
				chdir_cmd(&command[2]);
			break;

		default:
			set_error(ERR_SYNTAX30);
	}
}


/*
 *  Execute 'G' command
 */

void FSDrive::chdir_cmd(char *dirpath)
{
	char str[NAMEBUF_LENGTH];
	char *p = str;

	close_all_channels();

	// G:. resets the directory path to its original setting
	if (dirpath[0] == '.' && dirpath[1] == 0) {
		change_dir(orig_dir_path);
	} else {

		// Convert directory name
		for (int i=0; i<NAMEBUF_LENGTH && (*p++ = conv_from_64(*dirpath++, false)); i++) ;

		if (!change_dir(str))
			set_error(ERR_NOTREADY);
	}
}


/*
 *  Reset drive
 */

void FSDrive::Reset(void)
{
	close_all_channels();
	cmd_len = 0;	
	set_error(ERR_STARTUP);
}


/*
 *  Conversion PETSCII->ASCII
 */

uint8 FSDrive::conv_from_64(uint8 c, bool map_slash)
{
	if ((c >= 'A') && (c <= 'Z') || (c >= 'a') && (c <= 'z'))
		return c ^ 0x20;
	if ((c >= 0xc1) && (c <= 0xda))
		return c ^ 0x80;
	if ((c == '/') && map_slash && ThePrefs.MapSlash)
#ifdef __riscos__
		return '.';	// directory separator is '.' in RO
	if (c == '.') {return('_');}	// convert dot to underscore
#else
		return '\\';
#endif
	return c;
}


/*
 *  Conversion ASCII->PETSCII
 */

uint8 FSDrive::conv_to_64(uint8 c, bool map_slash)
{
	if ((c >= 'A') && (c <= 'Z') || (c >= 'a') && (c <= 'z'))
		return c ^ 0x20;
#ifdef __riscos__
	if ((c == '.') && map_slash && ThePrefs.MapSlash)
#else
	if ((c == '\\') && map_slash && ThePrefs.MapSlash)
#endif
		return '/';
#ifdef __riscos__
	if (c == '_') {return('.');}	// convert underscore to dot
#endif
	return c;
}
