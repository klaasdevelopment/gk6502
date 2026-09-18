#include "machine.h"
#include <string.h>
uint8_t machine_read(void *ctx, uint16_t a, bool debug) {
    Machine *m=ctx;
    if (a<0x8000) return m->ram[a];
    if (a>=0xc000) return m->rom[a-0xc000];
    if (a>=0xa000) return acia_read(&m->acia,a&1,debug);
    return 0xff;
}
void machine_write(void *ctx, uint16_t a, uint8_t v) {
    Machine *m=ctx;
    if (a<0x8000) m->ram[a]=v;
    else if (a>=0xa000 && a<0xc000) acia_write(&m->acia,a&1,v);
}
bool machine_init(Machine *m, bool cmos) {
    acia_reset(&m->acia);
    return cpu_init(&m->cpu,cmos,m,machine_read,machine_write);
}
void machine_boot(Machine *m, bool cold) {
    if (cold) memset(m->ram,0,sizeof(m->ram));
    m->head=m->count=0;
    acia_reset(&m->acia);
    cpu_reset(&m->cpu);
}
bool machine_input(Machine *m, uint8_t v) {
    if (m->count==INPUT_CAPACITY) return false;
    m->input[(m->head+m->count)%INPUT_CAPACITY]=v; m->count++; return true;
}
int machine_step(Machine *m) {
    /* ACIA /IRQ goes to the host's CTS, never the CPU's IRQ pin. */
    if (m->count && !m->acia.full && (m->acia.control&3)!=3) {
        acia_receive(&m->acia,m->input[m->head]);
        m->head=(m->head+1)%INPUT_CAPACITY; m->count--;
    }
    return cpu_step(&m->cpu);
}
