#ifndef GK_MACHINE_H
#define GK_MACHINE_H
#include "cpu.h"
#include "acia.h"
#define INPUT_CAPACITY 65536
typedef struct {
    Cpu cpu;
    Acia acia;
    uint8_t ram[0x8000], rom[0x4000], input[INPUT_CAPACITY];
    size_t head, count;
} Machine;
bool machine_init(Machine *, bool cmos);
void machine_boot(Machine *, bool cold);
uint8_t machine_read(void *, uint16_t, bool);
void machine_write(void *, uint16_t, uint8_t);
bool machine_input(Machine *, uint8_t);
int machine_step(Machine *);
#endif
