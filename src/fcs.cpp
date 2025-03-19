#include "fcs.h"

static uint32_t crc32_table[256];

void crc32_init() {
    uint32_t poly = 0x04C11DB7;
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t crc = i << 24;
        for (int j = 0; j < 8; j++) {
            if (crc & 0x80000000) {
                crc = (crc << 1) ^ poly;
            } else {
                crc = crc << 1;
            }
        }
        crc32_table[i] = crc;
    }
}

uint32_t crc32_compute(const uint8_t *data, size_t length) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < length; i++) {
        uint8_t index = (crc >> 24) ^ data[i];
        crc = (crc << 8) ^ crc32_table[index];
    }
    return crc ^ 0xFFFFFFFF;
}