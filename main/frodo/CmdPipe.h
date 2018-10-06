/*
 *  CmdPipe.h
 *
 *  Frodo (C) 1994-1997,2002 Christian Bauer
 *  Tcl/Tk stuff by Lutz Vieweg
 */

#ifndef CmdPipe_h
#define CmdPipe_h

extern "C" {
	#include <stdio.h>
	#include <sys/types.h>
}

class Pipe {

protected:

	int fds[2];

public:
        
	bool fail;

	Pipe(void);
	Pipe(int fdin, int fdout) : fail(false) {
		fds[0] = fdin;
		fds[1] = fdout;
	}
	~Pipe(void);
        
	unsigned long ewrite(const void * buf, unsigned long len);
	unsigned long eread (void * buf, unsigned long len);
        
	int get_read_fd(void) const {
		return fds[0];
	}
        
	int get_write_fd(void) const {
		return fds[1];
	}
        
	int probe(void) const;
};

class CmdPipe {
        
protected:

	Pipe tocmd;
	Pipe fromcmd;
        
	int childpid;
        
public:
        
	bool fail;
        
	CmdPipe(const char * command, const char * arg, int nicediff = 0);
	~CmdPipe(void);
        
	unsigned long ewrite(const void * buf, unsigned long len) {
		return tocmd.ewrite(buf, len);
	}
        
	unsigned long eread (void * buf, unsigned long len) {
		return fromcmd.eread(buf, len);
	}
        
	int get_read_fd(void) const {
		return fromcmd.get_read_fd();
	}
        
	int get_write_fd(void) const {
		return tocmd.get_write_fd();
	}
        
	int probe(void) const {
		return fromcmd.probe();
	}

};

#endif // CmdPipe_h
