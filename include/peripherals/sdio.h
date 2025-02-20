#ifndef _D_SDIO_H
#define _D_SDIO_H

#include "../gpio.h"

#define SDIO_BASE 0x3F300000

#define SD_BLKSIZECNT ((volatile unsigned int *)(MMIO_BASE + 0x00300004))
#define SD_ARG_REG (volatile int *)(SDIO_BASE + 0x008)
#define SD_CMD_REG (volatile int *)(SDIO_BASE + 0x00C)
#define SD_RESP_REG_0 (volatile int *)(SDIO_BASE + 0x010)
#define SD_RESP_REG_1 (volatile int *)(SDIO_BASE + 0x014)
#define SD_RESP_REG_2 (volatile int *)(SDIO_BASE + 0x018)
#define SD_RESP_REG_3 (volatile int *)(SDIO_BASE + 0x01C)
#define SD_DATA (volatile int *)(SDIO_BASE + 0x020)
#define SD_STATUS (volatile int *)(SDIO_BASE + 0x024)
#define SD_CTRL0 (volatile int *)(SDIO_BASE + 0x028)
#define SD_CTRL1 (volatile int *)(SDIO_BASE + 0x02C)
#define SD_INTERRUPT (volatile int *)(SDIO_BASE + 0x030)
#define SD_INTERRUPT_MASK (volatile int *)(SDIO_BASE + 0x034)
#define SD_INTERRUPT_EN (volatile int *)(SDIO_BASE + 0x038)
#define SD_SLOTISR_VER (volatile int *)(SDIO_BASE + 0x0FC)

#define SR_DAT_INHIBIT 0x00000002
#define SR_CMD_INHIBIT 0x00000001

#define C1_SRST_HC 0x01000000
#define C1_TOUNIT_DIS 0x000f0000
#define C1_TOUNIT_MAX 0x000e0000
#define C1_CLK_EN 0x00000004
#define C1_CLK_STABLE 0x00000002
#define C1_CLK_INTLEN 0x00000001

#define CMD_GO_IDLE_STATE 0
#define CMD_SEND_CID 2
#define CMD_SEND_IF_COND 0x08020000
#define CMD_SEND_STATUS 13
#define CMD_WRITE_SINGLE_BLOCK 24
#define CMD_IO_RW_DIRECT 52
#define CMD_IO_RW_EXTENDED 53
#define CMD_ALL_SEND_CID 0x02010000
#define CMD_SEND_REL_ADDR 0x03020000
#define CMD_CARD_SELECT 0x07030000
#define CMD_SEND_APP \
    0x37000000  // command used to say that the next command will be an application
                // specific command
#define CMD_SEND_CSD 9
#define CMD_READ_SINGLE 0x11220010
#define CMD_STOP_TRANS 0x0C030000

#define APP_CMD 0x80000000

#define ACMD_SEND_SCR (0x33220010 | APP_CMD)
#define ACMD_SEND_OP_COND (0x29020000 | APP_CMD)
#define ACMD_SET_BUT_WIDTH (0x06020000 | APP_CMD)

#define CMD53_MANUFACTURER_ID 0x0
#define CMD53_CARD_IO 0x1
#define CMD53_PRODUCT_ID1 0x2
#define CMD53_PRODUCT_ID2 0x2
#define CMD53_CARD_CAPABILITIES 0x8

#endif