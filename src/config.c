#define _POSIX_C_SOURCE 200809L
#include "config.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static char *trim(char *s) {
    while (isspace((unsigned char)*s)) s++;
    char *e=s+strlen(s);
    while (e>s && isspace((unsigned char)e[-1])) *--e=0;
    return s;
}
bool config_load(const char *path, Config *cfg, char *error, size_t size) {
    FILE *f=fopen(path,"r");
    if (!f) { snprintf(error,size,"cannot open properties file %s",path); return false; }
    char *line=NULL; size_t cap=0, number=0; ssize_t len;
    unsigned seen=0; const char *why=NULL; char rom[4096]={0};
    while ((len=getline(&line,&cap,f))>=0) {
        number++;
        if (memchr(line,0,(size_t)len)) { why="embedded NUL"; break; }
        char *s=trim(line);
        if (!*s || *s=='#') continue;
        char *eq=strchr(s,'=');
        if (!eq) { why="expected key=value"; break; }
        *eq++=0; char *key=trim(s), *v=trim(eq);
        unsigned bit=!strcmp(key,"rom.file") ? 1 : !strcmp(key,"cpu.model") ? 2 : 0;
        if (!bit) { why="unknown property"; break; }
        if (seen&bit) { why="duplicate property"; break; }
        if (!*v) { why="empty property value"; break; }
        seen|=bit;
        if (bit==1) {
            if (strlen(v)>=sizeof(rom)) { why="ROM path too long"; break; }
            strcpy(rom,v);
        } else {
            if (strcmp(v,"6502") && strcmp(v,"65c02")) { why="cpu.model must be 6502 or 65c02"; break; }
            cfg->cmos=!strcmp(v,"65c02");
        }
    }
    if (!why && ferror(f)) why="error reading properties";
    if (!why && seen!=3) why="missing rom.file or cpu.model";
    free(line); fclose(f);
    if (why) { snprintf(error,size,"%s:%zu: %s",path,number?number:1,why); return false; }
    const char *slash=strrchr(path,'/');
    int n;
    if (rom[0]=='/' || !slash) n=snprintf(cfg->rom,sizeof(cfg->rom),"%s",rom);
    else n=snprintf(cfg->rom,sizeof(cfg->rom),"%.*s/%s",(int)(slash-path),path,rom);
    if (n<0 || (size_t)n>=sizeof(cfg->rom)) { snprintf(error,size,"%s: ROM path too long",path); return false; }
    return true;
}
