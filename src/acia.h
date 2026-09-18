#ifndef GK_ACIA_H
#define GK_ACIA_H
#include <stdbool.h>
#include <stdint.h>
typedef struct {
    uint8_t control, data, errors;
    bool full, status_read;
    void (*output)(void *, uint8_t);
    void *context;
} Acia;
void acia_reset(Acia *);
uint8_t acia_read(Acia *, bool data, bool debug);
void acia_write(Acia *, bool data, uint8_t);
bool acia_receive(Acia *, uint8_t);
#endif
