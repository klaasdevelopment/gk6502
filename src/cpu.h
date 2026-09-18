#ifndef GK_CPU_H
#define GK_CPU_H
#include "vrEmu6502.h"
#include <stddef.h>
typedef uint8_t (*BusRead)(void *, uint16_t, bool);
typedef void (*BusWrite)(void *, uint16_t, uint8_t);
typedef struct {
    VrEmu6502 *core;
    void *bus;
    BusRead read;
    BusWrite write;
    bool cmos, waiting, stopped;
    uint64_t cycles;
    char error[96];
} Cpu;
bool cpu_init(Cpu *, bool cmos, void *, BusRead, BusWrite);
void cpu_destroy(Cpu *);
void cpu_reset(Cpu *);
int cpu_step(Cpu *);
#endif
