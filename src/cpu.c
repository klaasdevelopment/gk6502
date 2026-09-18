#include "cpu.h"
#include <stdio.h>
#include <string.h>
/* The upstream callbacks have no context argument. All CPU calls are serialized. */
static Cpu *active;
static uint8_t read_bus(uint16_t a, bool debug) { return active->read(active->bus, a, debug); }
static void write_bus(uint16_t a, uint8_t v) { active->write(active->bus, a, v); }
bool cpu_init(Cpu *c, bool cmos, void *bus, BusRead read, BusWrite write) {
    memset(c, 0, sizeof(*c));
    c->cmos=cmos; c->bus=bus; c->read=read; c->write=write; active=c;
    c->core=vrEmu6502New(cmos ? CPU_W65C02 : CPU_6502, read_bus, write_bus);
    c->cycles=7;
    return c->core != NULL;
}
void cpu_destroy(Cpu *c) { vrEmu6502Destroy(c->core); c->core=NULL; }
void cpu_reset(Cpu *c) {
    active=c; vrEmu6502Reset(c->core);
    c->waiting=c->stopped=false; c->error[0]=0; c->cycles+=7;
}
int cpu_step(Cpu *c) {
    active=c;
    if (c->error[0]) return -1;
    bool interrupt=*vrEmu6502Nmi(c->core)==IntRequested ||
        (*vrEmu6502Int(c->core)==IntRequested && !(vrEmu6502GetStatus(c->core)&FlagI));
    if (c->waiting && (*vrEmu6502Int(c->core)==IntRequested || interrupt)) c->waiting=false;
    if (c->stopped || c->waiting) { c->cycles++; return 1; }
    uint16_t pc=vrEmu6502GetPC(c->core);
    uint8_t op=c->read(c->bus,pc,true);
    if (!interrupt && !strcmp(vrEmu6502OpcodeToMnemonicStr(c->core,op),"err")) {
        snprintf(c->error,sizeof(c->error),"unsupported opcode $%02X at $%04X",op,pc);
        return -1;
    }
    int cycles=vrEmu6502InstCycle(c->core);
    if (c->cmos && !interrupt) { c->waiting=op==0xcb; c->stopped=op==0xdb; }
    c->cycles+=(unsigned)cycles;
    return cycles;
}
