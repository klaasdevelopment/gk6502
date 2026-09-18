#include "machine.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint8_t memory[65536];
static uint8_t rd(void *ctx,uint16_t a,bool debug) { (void)ctx;(void)debug;return memory[a]; }
static void wr(void *ctx,uint16_t a,uint8_t v) { (void)ctx;memory[a]=v; }
static Cpu cpu;
static void init(bool cmos) {
    memset(memory,0,sizeof(memory));memory[0xfffd]=2;
    assert(cpu_init(&cpu,cmos,NULL,rd,wr));
}
static void program(const uint8_t *p,size_t n) { memcpy(memory+0x200,p,n);vrEmu6502SetPC(cpu.core,0x200); }
static void steps(unsigned n) { while (n--) assert(cpu_step(&cpu)>0); }
static void functional(const char *file,bool cmos,uint16_t start,uint16_t success) {
    init(cmos);FILE *f=fopen(file,"rb"); assert(f);
    assert(fread(memory,1,sizeof(memory),f)==sizeof(memory));fclose(f);
    vrEmu6502SetPC(cpu.core,start);
    for (unsigned n=0;n<100000000;n++) {
        uint16_t pc=vrEmu6502GetPC(cpu.core);
        if (cpu_step(&cpu)<0) { fprintf(stderr,"%s: %s\n",file,cpu.error);abort(); }
        if (vrEmu6502GetPC(cpu.core)==pc) {
            if (pc!=success) { fprintf(stderr,"%s trapped at %04X after %u instructions\n",file,pc,n);abort(); }
            printf("PASS %s (%u instructions)\n",file,n);cpu_destroy(&cpu);return;
        }
    }
    fprintf(stderr,"%s timed out\n",file);abort();
}
static void decimal(bool cmos) {
    init(cmos);
    const char *file=cmos?"build/decimal_65c02.bin":"build/decimal_6502.bin";
    FILE *f=fopen(file,"rb");assert(f);
    assert(fread(memory+0x200,1,0xfe00,f)==0xfe00);fclose(f);
    for (unsigned n=0;n<100000000;n++) {
        uint16_t pc=vrEmu6502GetPC(cpu.core);assert(cpu_step(&cpu)>0);
        if (vrEmu6502GetPC(cpu.core)==pc) {
            if (memory[11]) {
                fprintf(stderr,"decimal %s failed: operands %02x %02x, actual %02x flags %02x, expected %02x N %02x V %02x Z %02x C %02x\n",
                    file,memory[0],memory[1],memory[4],memory[5],memory[6],memory[7],memory[8],memory[9],memory[10]);abort();
            }
            printf("PASS %s (all operands, carries, flags)\n",file);cpu_destroy(&cpu);return;
        }
    }
    abort();
}
static void focused(bool cmos) {
    init(cmos);
    assert(vrEmu6502GetStatus(cpu.core)&FlagI);
    assert(vrEmu6502GetStackPointer(cpu.core)==0xfd);
    const uint8_t arithmetic[]={0xf8,0x18,0xa9,0x50,0x69,0x50};
    program(arithmetic,sizeof(arithmetic));steps(3);
    assert(cpu_step(&cpu)==(cmos?3:2));
    assert(vrEmu6502GetAcc(cpu.core)==0);
    uint8_t flags=vrEmu6502GetStatus(cpu.core);
    assert((flags&(FlagC|FlagV))==(FlagC|FlagV));
    assert(!!(flags&FlagZ)==cmos);assert(!!(flags&FlagN)!=cmos);
    const uint8_t branch[]={0xa9,1,0xd0,0xfc};program(branch,sizeof(branch));
    assert(cpu_step(&cpu)==2); assert(cpu_step(&cpu)==3);
    vrEmu6502SetPC(cpu.core,0x2fd);memory[0x2fd]=0xd0;memory[0x2fe]=2;
    assert(cpu_step(&cpu)==4); assert(vrEmu6502GetPC(cpu.core)==0x301);
    const uint8_t indexed[]={0xa2,1,0xbd,0xff,0x30};program(indexed,sizeof(indexed));steps(1);
    assert(cpu_step(&cpu)==5);
    const uint8_t indirect[]={0x6c,0xff,0x30};program(indirect,sizeof(indirect));
    memory[0x30ff]=0x34;memory[0x3000]=0x12;memory[0x3100]=0x56;
    steps(1);assert(vrEmu6502GetPC(cpu.core)==(cmos?0x5634:0x1234));
    const uint8_t intr[]={0xf8,0x58,0xea};program(intr,sizeof(intr));steps(2);
    memory[0xfffe]=0;memory[0xffff]=4;memory[0x400]=0x40;
    *vrEmu6502Int(cpu.core)=IntRequested;
    assert(cpu_step(&cpu)==7);assert(vrEmu6502GetPC(cpu.core)==0x400);
    assert(!!(vrEmu6502GetStatus(cpu.core)&FlagD)!=cmos);
    *vrEmu6502Int(cpu.core)=IntCleared;assert(cpu_step(&cpu)==6);
    assert(vrEmu6502GetPC(cpu.core)==0x202);assert(vrEmu6502GetStatus(cpu.core)&FlagD);
    cpu_reset(&cpu);assert(vrEmu6502GetStatus(cpu.core)&FlagI);
    assert(!!(vrEmu6502GetStatus(cpu.core)&FlagD)!=cmos);
    if (!cmos) { memory[0x200]=0x02; assert(cpu_step(&cpu)==-1); assert(strstr(cpu.error,"$0200")); }
    else {
        memory[0x200]=0xcb;steps(1);assert(cpu.waiting);steps(1);
        cpu_reset(&cpu);assert(!cpu.waiting);
        memory[0x200]=0xdb;steps(1);assert(cpu.stopped);steps(1);
        cpu_reset(&cpu);assert(!cpu.stopped);
    }
    cpu_destroy(&cpu);
}
static void board(bool cmos) {
    Machine *m=calloc(1,sizeof(*m));assert(m);m->rom[0x3ffd]=0xc0;
    assert(machine_init(m,cmos));
    machine_write(m,0x7fff,0xab);assert(machine_read(m,0x7fff,false)==0xab);
    machine_write(m,0x8000,0);assert(machine_read(m,0x8000,false)==0xff);
    machine_write(m,0xc000,0xab);assert(machine_read(m,0xc000,false)==0);
    machine_write(m,0xbffe,0x95);assert(m->acia.control==0x95);
    assert(machine_read(m,0xa000,false)==2);
    assert(acia_receive(&m->acia,'A'));assert(machine_read(m,0xbffe,false)==0x83);
    assert(machine_read(m,0xbfff,true)=='A');assert(m->acia.full);
    assert(machine_read(m,0xa001,false)=='A');assert(machine_read(m,0xa000,false)==2);
    assert(acia_receive(&m->acia,'B'));assert(!acia_receive(&m->acia,'C'));
    assert(machine_read(m,0xa000,false)&0x20);assert(machine_read(m,0xa001,false)=='B');
    assert(machine_read(m,0xa000,false)==2);
    machine_input(m,'X');acia_receive(&m->acia,'Y');machine_boot(m,false);
    assert(!m->count && !m->acia.full && m->acia.control==3);
    assert(m->ram[0x7fff]==0xab);assert(vrEmu6502GetPC(m->cpu.core)==0xc000);
    assert(*vrEmu6502Int(m->cpu.core)==IntCleared);
    machine_boot(m,true);for (size_t i=0;i<sizeof(m->ram);i++) assert(!m->ram[i]);
    assert(vrEmu6502GetPC(m->cpu.core)==0xc000);
    cpu_destroy(&m->cpu);free(m);
}
int main(void) {
    focused(false);focused(true);board(false);board(true);decimal(false);decimal(true);
    puts("PASS CPU timing/interrupt/reset and board tests");
    functional("tests/functional/6502_functional_test.bin",false,0x400,0x3469);
    functional("tests/functional/65C02_extended_opcodes_test.bin",true,0x400,0x24f1);
    return 0;
}
