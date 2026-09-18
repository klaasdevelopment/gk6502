#ifndef GK_CONFIG_H
#define GK_CONFIG_H
#include <stdbool.h>
#include <stddef.h>
typedef struct { char rom[4096]; bool cmos; } Config;
bool config_load(const char *, Config *, char *, size_t);
#endif
