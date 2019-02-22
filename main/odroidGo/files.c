/*
 * The MIT License
 *
 * Copyright 2018 Schuemi.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */


#include "LibOdroidGo.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>

#include "odroid_display.h"
#include <stdarg.h>
#include <unistd.h>


char* buffer;
char* fullCurrentDir;

struct dirent dirInfo;
struct dirent firstDirEntry;
bool haveFirstDirEntry = false;
unsigned int tmpFileNumber = 0;

bool hasExt(const char *file, const char *Ext) {
    
    const char* p = strstr(file,".");
    if (! p) return false;
    int lenFile = strlen(file);
    const char* cExt = Ext;
    
    while(cExt[0] != 0) {
        int lenExtension = strlen(cExt);
        if (lenFile < lenExtension) continue;
        p = file;
        p += lenFile;
        p -= lenExtension;
        if (!strcasecmp(p, cExt)) return true;
        cExt += strlen(cExt) + 2;
        
    }
    
    return false;
}

char* cutExtension(char* file) {
    register int i = strlen(file)-1;
    while(i>=0) {
        if (file[i] == '.') {
            file[i] = 0;
            return file;
        }
        i--;
    }
    return file;
}
char* changeExtension(char* file, const char* newExt) {
    register int i = strlen(file)-1;
    while(i>=0) {
        if (file[i] == '.') {
            memcpy(&file[i], newExt, strlen(newExt) + 1);
            return file;
        }
        i--;
    }
    return file;
}

const char* getFileName(const char* file) {
    register int i = strlen(file)-1;
    while(i>=0) {
        if (file[i] == '/') {
            return &file[i+1];
        }
        i--;
    }
    return file;
}


char* getPath(char* file) {
    register int i = strlen(file)-1;
    while(i>=0) {
        if (file[i] == '/') {
            file[i] = 0;
            return file;
        }
        i--;
    }
    return file;
}



int initFiles(){
    fullCurrentDir = malloc(612);
    buffer = malloc(1024);
   
    if (!fullCurrentDir){ printf("malloc fullCurrentDir failed!\n"); return 0; }
    
    strncpy(fullCurrentDir, FRODO_ROOT_GAMESDIR, 612);
    
}
FILE * _tmpfile()
{
    char buf[512];
    snprintf(buf, 512, FRODO_ROOT_DATADIR"/tmp%u", tmpFileNumber++);
    return _fopen(buf, "w+");
}
int chdir(const char *path)
{
    if (path == 0) return -1;
    if (path[0] == 0) return -1;
    
    if (!strcmp(path, "..")){
        if (strlen(fullCurrentDir) > strlen(FRODO_ROOT_GAMESDIR)) getPath(fullCurrentDir);
        return 0;
    }
    if (path[0] == '/'){
        // warum schmiert er ab?
        /*if (strstr(path, FRODO_ROOT_GAMESDIR) == path) {
            DIR* d = opendir(path);
            if (d != NULL) {
                strncpy(fullCurrentDir, path, 1024);
                closedir(d);
                return 0;
            }
            return -1;
        }*/
        return -1;
    }    
    
    int len = strlen(fullCurrentDir);
    if (len > 610) return -1;
    fullCurrentDir[len] = '/';
    fullCurrentDir[len+1] = 0;
    
    strncpy((fullCurrentDir + strlen(fullCurrentDir)), path, 610 - strlen(fullCurrentDir));
    
    return 0;
}

char *getcwd(char *buf, size_t size)
{
    strncpy(buf, fullCurrentDir, size);
    return buf;
}
char* getFullPath(char* buffer, const char* fileName, int bufferLength){
    if (fileName[0] != '/'){
        getcwd(buffer, bufferLength);
        int len = strlen(buffer);
        *(buffer + strlen(buffer)) = '/';
        strncpy((buffer + len + 1), fileName, bufferLength - (len + 1));
        
    } else {
        strncpy(buffer, fileName, 1024);
    }
    return buffer;
}

DIR* _opendir(const char* name)
{
    STOP_DISPLAY_FUNCTION();
    DIR* d;
    if (!strcmp(name, ".")) {
        d = opendir(getcwd(buffer, 1024));
    } else d = opendir(name);
    
    RESUME_DISPLAY_FUNCTION();
    return d;

}
size_t _fread(_PTR __restrict p, size_t _size, size_t _n, FILE *__restrict f) {
    
    size_t s;
    STOP_DISPLAY_FUNCTION();
    s= fread(p, _size, _n, f);
    RESUME_DISPLAY_FUNCTION();
    return s;
    /*size_t size = _size*_n;
    size_t readed = 0;
    for (int i = 0; i < size; i+=0x100) {
        int toRead = 0x100;
        if (i + 0x100 > size) toRead = size%0x100;
        STOP_DISPLAY_FUNCTION();
        size_t r =fread(p+readed, toRead, 1, f);
        RESUME_DISPLAY_FUNCTION();
        if (r == 1) readed += toRead;
        if (r == 0) break;
        
    }
    
    return _n;*/
}
size_t _fwrite(const _PTR __restrict p , size_t _size, size_t _n, FILE * f) {
    // i had problems with fwrite (sd write errors, if the datasize is > 0x100) 
    
    size_t size = _size*_n;
    int written = 0;
    for (int i = 0; i < size; i+=0x100) {
        int toWrite = 0x100;
        if (i + 0x100 > size) toWrite = size%0x100;
        STOP_DISPLAY_FUNCTION();
        written +=fwrite(p+i, toWrite, 1, f);
        RESUME_DISPLAY_FUNCTION();
    }
    if (written > 0) {
        fflush(f);
        fsync(fileno(f));
        
    }
    return written;
}

int _closedir(DIR* f) {
    int res;
    STOP_DISPLAY_FUNCTION();
    res = closedir(f);
    RESUME_DISPLAY_FUNCTION();
    return res;
}
void _rewinddir(DIR* pdir) {
    STOP_DISPLAY_FUNCTION();
    rewinddir(pdir);
    RESUME_DISPLAY_FUNCTION();
}
void _rewind(FILE* f) {
    STOP_DISPLAY_FUNCTION();
    rewind(f);
    RESUME_DISPLAY_FUNCTION();
}

struct dirent* _readdir(DIR* pdir)
{
    STOP_DISPLAY_FUNCTION();
    if (telldir(pdir) == 0) {
        // the first dir I should send is the ".." dir to go one dir up.
        
        dirInfo.d_ino  = 0;
        dirInfo.d_type = DT_DIR;
        strncpy(dirInfo.d_name, "..", 3);
        
        firstDirEntry = *(readdir(pdir));
        haveFirstDirEntry = true;
        RESUME_DISPLAY_FUNCTION();
        return &dirInfo;
        
    }
    if (haveFirstDirEntry){
        haveFirstDirEntry = false;
        RESUME_DISPLAY_FUNCTION();
        return &firstDirEntry;
        
    }
    struct dirent* r = readdir(pdir);
    RESUME_DISPLAY_FUNCTION();
    return r;
    
}
int _stat( const char *__restrict __path, struct stat *__restrict __sbuf ){

    STOP_DISPLAY_FUNCTION();
    if (!strcmp(__path, "..")) {
        RESUME_DISPLAY_FUNCTION();
        __sbuf->st_mode = 0x41FF;
        return 0;
    }
    getFullPath(buffer, __path, 1024);
    int res =  stat(buffer, __sbuf);
    RESUME_DISPLAY_FUNCTION();
    return res;
}
int _fscanf(FILE *__restrict f, const char *__restrict c, ...) {
    STOP_DISPLAY_FUNCTION();
    va_list args;
    va_start(args, c);
    int res = vfscanf(f,c, args);
    va_end(args);
    RESUME_DISPLAY_FUNCTION();
    return res;
    
}
int _fprintf(FILE *__restrict f, const char *__restrict c, ...){
    STOP_DISPLAY_FUNCTION();
    va_list args;
    va_start(args, c);
    int res = vfprintf(f,c, args);
    va_end(args);
    RESUME_DISPLAY_FUNCTION();
    return res;
}
int _fgetc(FILE * f)
{
    STOP_DISPLAY_FUNCTION();
    int res = fgetc(f);
    RESUME_DISPLAY_FUNCTION();
    return res;
}

int _fputc(int i, FILE * f)
{
    STOP_DISPLAY_FUNCTION();
    int res = fputc(i,f);
    RESUME_DISPLAY_FUNCTION();
    return res;
}
int _fputs(const char *__restrict c, FILE *__restrict f)
{
    STOP_DISPLAY_FUNCTION();
    int res = fputs(c,f);
    RESUME_DISPLAY_FUNCTION();
    return res;
}

char * _fgets(char *__restrict c, int i, FILE *__restrict f)
{
    STOP_DISPLAY_FUNCTION();
    char * res = fgets(c, i, f);
    RESUME_DISPLAY_FUNCTION();
    return res;
}


int _fseek(FILE * f, long a, int b) {
    STOP_DISPLAY_FUNCTION();
    int ret = fseek(f, a, b);
    RESUME_DISPLAY_FUNCTION();
    return ret;
}
long _ftell( FILE * f) {
    STOP_DISPLAY_FUNCTION();
    long r = ftell(f);
    RESUME_DISPLAY_FUNCTION();
    return r;
}
int openfiles = 0;
FILE* _fopen(const char *__restrict _name, const char *__restrict _type) {
       
    if (_name[0] != '/') {
        getFullPath(buffer, _name, 1024);
    } else {
        strncpy(buffer, _name, 1024);
    }
    
    STOP_DISPLAY_FUNCTION();
    FILE* f = fopen(buffer, _type);
    RESUME_DISPLAY_FUNCTION();
    //printf("fopen: %s (%s) %p\n", buffer, _type, f);
    if (f)openfiles++;
    //printf("open files: %d (%d)\n",openfiles,f);
    
    return f;
    
}
int _fclose(FILE* file) {
    STOP_DISPLAY_FUNCTION();
    fflush(file);
    int res =  fclose(file);
    RESUME_DISPLAY_FUNCTION();
    if (!res)openfiles--;
    //printf("open files: %d\n",openfiles );
    return res;
}
long _telldir(DIR* pdir){
    STOP_DISPLAY_FUNCTION();
    long r = telldir(pdir);
    if (r > 0) r++;
    RESUME_DISPLAY_FUNCTION();
    return r;
}
void _seekdir(DIR* pdir, long loc){
    STOP_DISPLAY_FUNCTION();
    if (loc > 0) loc--;
     seekdir(pdir, loc);
    RESUME_DISPLAY_FUNCTION();
}
int _remove(const char * f) {
    int res;
    STOP_DISPLAY_FUNCTION();
    res = remove(f);
    RESUME_DISPLAY_FUNCTION();
    return res;
}

int _rename(const char * f, const char * nf) {
    int res;
    STOP_DISPLAY_FUNCTION();
    res = rename(f, nf);
    RESUME_DISPLAY_FUNCTION();
    return res;
}
