#define _POSIX_C_SOURCE 200809L
#include "machine.h"
#include "config.h"
#include <errno.h>
#include <inttypes.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>
static struct termios saved;
static bool raw;
static volatile sig_atomic_t interrupted;
static bool output_error;
static void restore(void) { if (raw) { tcsetattr(STDIN_FILENO,TCSANOW,&saved); raw=false; } }
static void signal_handler(int sig) { interrupted=sig; }
static void output(void *ctx, uint8_t v) {
    (void)ctx;
    if (fputc(v,stdout)==EOF) output_error=true;
}
static void discard_pending_input(void) {
    if (raw) { tcflush(STDIN_FILENO,TCIFLUSH); return; }
    /* Drain only the bytes already pending, not future input from a live pipe. */
    int pending=0;
    if (ioctl(STDIN_FILENO,FIONREAD,&pending)==0) {
        uint8_t discard[4096];
        while (pending>0) {
            size_t want=(size_t)pending<sizeof(discard)?(size_t)pending:sizeof(discard);
            ssize_t n=read(STDIN_FILENO,discard,want);
            if (n<=0) break;
            pending-=(int)n;
        }
    }
}
static void usage(FILE *f) {
    fprintf(f,"Usage: gk6502 [--config PATH] [--max-cycles N]\n"
              "Ctrl-W: warm boot; Ctrl-B: cold boot; Ctrl-]: exit.\n");
}
int main(int argc, char **argv) {
    const char *path="emulator.properties"; uint64_t limit=0;
    for (int i=1;i<argc;i++) {
        if (!strcmp(argv[i],"--help")) { usage(stdout); return 0; }
        if (!strcmp(argv[i],"--config") && i+1<argc) path=argv[++i];
        else if (!strcmp(argv[i],"--max-cycles") && i+1<argc) {
            char *end; const char *s=argv[++i]; errno=0;
            limit=strtoull(s,&end,10);
            if (errno || !*s || *end || *s=='-' || !limit) { fprintf(stderr,"invalid cycle limit\n"); return 1; }
        } else { usage(stderr); return 1; }
    }
    Config config={0}; char error[4608];
    if (!config_load(path,&config,error,sizeof(error))) { fprintf(stderr,"%s\n",error); return 1; }
    Machine *m=calloc(1,sizeof(*m));
    if (!m) { perror("machine allocation"); return 1; }
    FILE *rom=fopen(config.rom,"rb");
    if (!rom) { fprintf(stderr,"cannot open ROM %s: %s\n",config.rom,strerror(errno)); free(m); return 1; }
    size_t bytes=fread(m->rom,1,sizeof(m->rom),rom);
    int extra=fgetc(rom); bool read_error=ferror(rom); fclose(rom);
    if (read_error || bytes!=sizeof(m->rom) || extra!=EOF) {
        fprintf(stderr,"ROM %s must contain exactly 16384 bytes\n",config.rom); free(m); return 1;
    }
    if (!machine_init(m,config.cmos)) { fprintf(stderr,"CPU allocation failed\n"); free(m); return 1; }
    m->acia.output=output;
    struct sigaction action={0}; action.sa_handler=signal_handler; sigemptyset(&action.sa_mask);
    sigaction(SIGINT,&action,NULL); sigaction(SIGTERM,&action,NULL); sigaction(SIGHUP,&action,NULL);
    sigaction(SIGPIPE,&action,NULL);
    if (isatty(STDIN_FILENO)) {
        if (tcgetattr(STDIN_FILENO,&saved)<0) { perror("terminal settings"); cpu_destroy(&m->cpu); free(m); return 1; }
        struct termios t=saved;
        t.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL|IXON);
        t.c_oflag &= ~OPOST; t.c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);
        t.c_cflag=(t.c_cflag & ~(CSIZE|PARENB))|CS8; t.c_cc[VMIN]=1; t.c_cc[VTIME]=0;
        if (tcsetattr(STDIN_FILENO,TCSANOW,&t)<0) { perror("raw terminal"); cpu_destroy(&m->cpu); free(m); return 1; }
        raw=true; atexit(restore);
    }
    setvbuf(stdout,NULL,_IONBF,0);
    bool eof=false, quit=false, previous_cr=false; int result=0;
    while (!quit && !interrupted && !output_error) {
        if (!eof && m->count<INPUT_CAPACITY-4096) {
            struct pollfd p={STDIN_FILENO,POLLIN,0};
            int ready=poll(&p,1,0);
            if (ready<0 && errno!=EINTR) { perror("poll"); result=1; break; }
            if (ready>0 && (p.revents&(POLLIN|POLLHUP|POLLERR))) {
                uint8_t input[4096]; ssize_t n=read(STDIN_FILENO,input,sizeof(input));
                if (!n) eof=true;
                else if (n<0 && errno!=EINTR && errno!=EAGAIN) { perror("input"); result=1; break; }
                for (ssize_t j=0;j<n;j++) {
                    uint8_t v=input[j];
                    if (v==0x1d) { quit=true; break; }
                    if (v==0x17 || v==0x02) {
                        machine_boot(m,v==0x02); previous_cr=false;
                        discard_pending_input();
                        break; /* Discard the remainder of the pending input batch. */
                    }
                    if (v=='\n' && previous_cr) { previous_cr=false; continue; }
                    previous_cr=v=='\r';
                    machine_input(m,v=='\n' ? '\r' : v);
                }
            }
        }
        for (unsigned i=0;i<1024 && !quit && !interrupted;i++) {
            if (limit && m->cpu.cycles>=limit) {
                fprintf(stderr,"cycle limit reached (%" PRIu64 ")\n",limit); result=2; quit=true; break;
            }
            if (machine_step(m)<0) { fprintf(stderr,"%s\n",m->cpu.error); result=1; quit=true; break; }
        }
        /* WAI/STP have no interrupt source on this board, but resets still work. */
        if (m->cpu.waiting || m->cpu.stopped) poll(NULL,0,1);
    }
    if (interrupted) result=128+interrupted;
    if (output_error) { fprintf(stderr,"serial output failed\n"); result=1; }
    restore(); cpu_destroy(&m->cpu); free(m); return result;
}
