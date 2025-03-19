#ifndef _FCS_H
#define _FCS_H

#include <stdint.h>
#include <stddef.h>

void crc32_init();
uint32_t crc32_compute(const uint8_t *, size_t);

#endif