#include "acia.h"
void acia_reset(Acia *a) {
    a->control=3; a->data=a->errors=0; a->full=a->status_read=false;
}
uint8_t acia_read(Acia *a, bool data, bool debug) {
    if (!data) {
        uint8_t s=2 | (a->full ? 1 : 0) | a->errors;
        if (((a->control & 0x80) && (a->full || a->errors)) ||
            ((a->control & 0x60)==0x20)) s|=0x80;
        if (!debug) a->status_read=true;
        return s;
    }
    uint8_t v=a->data;
    if (!debug) {
        a->full=false;
        a->errors &= 0x20; /* FE/PE clear on data read; overrun needs status then data. */
        if (a->status_read) a->errors=0;
        a->status_read=false;
    }
    return v;
}
void acia_write(Acia *a, bool data, uint8_t v) {
    if (!data) {
        if ((v&3)==3) acia_reset(a);
        else a->control=v;
    } else if ((a->control&3)!=3 && (a->control&0x60)!=0x60 && a->output) {
        a->output(a->context,v);
    }
}
bool acia_receive(Acia *a, uint8_t v) {
    if ((a->control&3)==3) return false;
    if (a->full) { a->errors|=0x20; return false; }
    a->data=v; a->full=true; a->status_read=false; return true;
}
