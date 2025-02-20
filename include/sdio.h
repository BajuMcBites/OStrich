#ifndef _SDIO_H
#define _SDIO_H

#include "peripherals/sdio.h"

int sd_read_block();
int sd_write_block();
int sd_init();
void sd_get_devices();

#endif