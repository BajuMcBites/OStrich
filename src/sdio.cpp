#include "sdio.h"

#include "atomic.h"
#include "printf.h"
#include "stdint.h"
#include "timer.h"
#include "uart.h"

#define HOST_SPEC_NUM 0x00ff0000
#define HOST_SPEC_NUM_SHIFT 16

#define SR_READ_AVAILABLE 0x00000800
#define SR_APP_CMD 0x00000020

#define INT_CMD_TIMEOUT 0x00010000
#define INT_DATA_TIMEOUT 0x00100000
#define INT_ERROR_MASK 0x017E8000

unsigned char DEBUG = 1;

#define HANDLE_ERROR(cmd, cmd_ret, ret) \
    cmd_ret = cmd;                      \
    if (cmd_ret != SD_OK) {             \
        if (DEBUG) {                    \
            print_sd_error(cmd_ret);    \
        }                               \
        return ret;                     \
    }

/**
 * Print what type of SD error occurred.
 *
 */
void print_sd_error(int sd_err) {
    switch (sd_err) {
        case SD_ERROR:
            printf("SD Error occurred.\n");
            break;
        case SD_TIMEOUT:
            printf("SD Timeout occurred.\n");
            break;
        case SD_OK:
            printf("ALERT: probably should not be printing if SD_OK happened...\n");
            break;
        default:
            printf("Invalid SD error code. What???\n");
    }
}

/**
 * SCR: SD Control Register
 * RCA: Relative Card Address, used to address the card.
 * CCS: Card Capacity Status, info about whether the card is standard (SDSC) or high (SDHC/SDXC)
 * capacity. HV: Host Version, the host controller's version number
 */
unsigned long sd_scr[2], sd_rca = 0, ccs = 0, sd_hv;
uint32_t sd_csd[4];
void wait_cycles(int c) {
    while (c-- > 0) {
        asm volatile("nop");
    }
}

/**
 * Poll interrupt register for specific interrupt types.
 *
 * */
int sd_int(unsigned int mask) {
    unsigned int r, m = mask | INT_ERROR_MASK;
    int cnt = 100000;
    while (!(*SD_INTERRUPT & m) && cnt--) wait_msec(1);
    r = *SD_INTERRUPT;
    // printf("response: %x\n", r);
    if (cnt <= 0 || (r & INT_CMD_TIMEOUT) || (r & INT_DATA_TIMEOUT)) {
        printf("sd_interrupt reg: cmd / data timeout %d, %d, %d\n", cnt, (r & INT_CMD_TIMEOUT),
               (r & INT_DATA_TIMEOUT));
        *SD_INTERRUPT = r;
        return -2;
    } else if (r & INT_ERROR_MASK) {
        printf("interrupt error\n");
        *SD_INTERRUPT = r;
        return -1;
    }
    *SD_INTERRUPT = mask;
    return 0;
}

int sd_status(unsigned int mask) {
    int cnt = 500000;
    while ((*SD_STATUS & mask) && !(*SD_INTERRUPT & INT_ERROR_MASK) && cnt--) {
        wait_msec(1);
    }
    return (cnt <= 0 || (*SD_INTERRUPT & INT_ERROR_MASK)) ? -2 : 0;
}

int send_command(unsigned int command, unsigned int arg, int *res) {
    // printf("sending command %x\n", command);
    if (command & APP_CMD) {
        int resp;
        int status = send_command(CMD_SEND_APP | (sd_rca ? 0x00020000 : 0), sd_rca, &resp);
        if ((sd_rca && !resp) || status) {
            printf("ERROR: failed to send SD APP command\n");
            return -1;
        }
        command &= ~APP_CMD;
    }

    if (sd_status(SR_CMD_INHIBIT)) {
        printf("ERROR: SDIO Controller is busy.");
        return -1;
    }

    *SD_INTERRUPT = *SD_INTERRUPT;
    *SD_ARG_REG = arg;
    *SD_CMD_REG = command;

    int reg = 0;

    if (command == CMD_SEND_IF_COND || command == CMD_SEND_APP) {
        wait_msec(100);
    } else if (command == ACMD_SEND_OP_COND) {
        wait_msec(1000);
    }

    if ((reg = sd_int(0x1))) {
        printf("MMC: failed to send command %x\n", command);
        return -1;
    }

    if (res) {
        *res = *SD_RESP_REG_0;
    }

    if (command == CMD_ALL_SEND_CID) {
        *res |= *SD_RESP_REG_1;
        *res |= *SD_RESP_REG_2;
        *res |= *SD_RESP_REG_3;
    }

    if (command == CMD_CARD_SELECT) {
        sd_rca = arg;
    }

    // printf("MMC: command %x arg %x, response = %d\n", command, arg,
    //  (res ? *res : 0));

    if (command == CMD_SEND_REL_ADDR) {
        return (((*res & 0x1fff)) | ((*res & 0x2000) << 6) | ((*res & 0x4000) << 8) |
                ((*res & 0x8000) << 8)) &
               0xfff9c004;
    }

    return 0;
}

int send_op_cond(int arg) {
    unsigned int r, cnt = 6;
    while (!(r & 0x80000000) && cnt--) {
        wait_cycles(200);
        if (send_command(ACMD_SEND_OP_COND, arg, (int *)&r) == -2) {
            return -1;
        }
    }

    if (!(r & 0x80000000) || !cnt) {
        return -1;
    }

    return r;
}

int sd_clk(unsigned int f) {
    unsigned int d, c = 41666666 / f, x, s = 32, h = 0;
    int cnt = 100000;
    while ((*SD_STATUS & (SR_CMD_INHIBIT | SR_DAT_INHIBIT)) && cnt--) wait_msec(1);
    if (cnt <= 0) {
        return -2;
    }

    *SD_CTRL1 &= ~C1_CLK_EN;
    wait_msec(10);
    x = c - 1;
    if (!x)
        s = 0;
    else {
        if (!(x & 0xffff0000u)) {
            x <<= 16;
            s -= 16;
        }
        if (!(x & 0xff000000u)) {
            x <<= 8;
            s -= 8;
        }
        if (!(x & 0xf0000000u)) {
            x <<= 4;
            s -= 4;
        }
        if (!(x & 0xc0000000u)) {
            x <<= 2;
            s -= 2;
        }
        if (!(x & 0x80000000u)) {
            x <<= 1;
            s -= 1;
        }
        if (s > 0) s--;
        if (s > 7) s = 7;
    }
    if (sd_hv > 1)
        d = c;
    else
        d = (1 << s);
    if (d <= 2) {
        d = 2;
        s = 0;
    }
    if (sd_hv > 1) h = (d & 0x300) >> 2;
    d = (((d & 0x0ff) << 8) | h);
    *SD_CTRL1 = (*SD_CTRL1 & 0xffff003f) | d;

    wait_msec(10);
    *SD_CTRL1 |= C1_CLK_EN;
    wait_msec(10);
    cnt = 10000;
    while (!(*SD_CTRL1 & C1_CLK_STABLE) && cnt--) wait_msec(10);
    if (cnt <= 0) {
        return -2;
    }
    return 0;
}

int init_gpio_pins() {
    const unsigned gpio_cd = 47;
    gpio_request(gpio_cd, "GPIO_CD");
    gpio_set_direction(gpio_cd, DIR_ALT3);
    gpio_pull(gpio_cd, GPPUD_PULL_UP);
    gpio_set_high(gpio_cd, 1);

    const unsigned gpio_clk = 48;
    const unsigned gpio_cmd = 49;
    gpio_request(gpio_clk, "GPIO_CLK");
    gpio_set_direction(gpio_clk, DIR_ALT3);
    gpio_pull(gpio_clk, GPPUD_PULL_UP);

    gpio_request(gpio_cmd, "GPIO_CMD");
    gpio_set_direction(gpio_cmd, DIR_ALT3);
    gpio_pull(gpio_cmd, GPPUD_PULL_UP);

    char names[4][10] = {"GPIO_DAT0", "GPIO_DAT1", "GPIO_DAT2", "GPIO_DAT3"};

    for (int i = 0; i < 4; i++) {
        const unsigned gpio_dat_i = 50 + i;
        const char *pin_name = names[i];

        gpio_request(gpio_dat_i, pin_name);
        gpio_set_direction(gpio_dat_i, DIR_ALT3);
        gpio_pull(gpio_dat_i, GPPUD_PULL_UP);
    }

    sd_hv = (*SD_SLOTISR_VER & HOST_SPEC_NUM) >> HOST_SPEC_NUM_SHIFT;
    // Reset the card.
    *SD_CTRL0 = 0;
    *SD_CTRL1 |= C1_SRST_HC;
    long cnt = 10000;
    do {
        wait_msec(10);
    } while ((*SD_CTRL1 & C1_SRST_HC) && cnt--);
    if (cnt <= 0) {
        return -1;
    }
    *SD_CTRL1 |= C1_CLK_INTLEN | C1_TOUNIT_MAX;
    wait_msec(10);

    // Set clock to setup frequency.
    int r;
    if ((r = sd_clk(400000))) return r;
    *SD_INTERRUPT_EN = 0xffffffff;
    *SD_INTERRUPT_MASK = 0xffffffff;

    return 0;
}

int init_cmd_seq() {
    int r;
    if (send_command(CMD_GO_IDLE_STATE, 0, nullptr) || send_command(CMD_SEND_IF_COND, 0x1AA, &r) ||
        r != 0x1AA) {
        return -1;
    }

    int op_cond_resp = send_op_cond(0x51ff8000);

    if (op_cond_resp == -1 || !(op_cond_resp & 0x00ff8000)) {
        return op_cond_resp < 0 ? op_cond_resp : -op_cond_resp;
    }

    // Check CCS value for card.
    if (op_cond_resp & 0x40000000) ccs = SCR_SUPP_CCS;

    if (send_command(CMD_ALL_SEND_CID, 0, nullptr)) return -1;

    if (send_command(CMD_SEND_REL_ADDR, 0, (int *)&sd_rca)) return -1;

    sd_rca &= 0xffff0000;

    if ((r = sd_clk(25000000)) || send_command(CMD_CARD_SELECT, sd_rca, nullptr) ||
        sd_status(SR_DAT_INHIBIT))
        return -1;

    // Set block size and block count for data transfers.
    *SD_BLKSIZECNT = (1 << 16) /* 1 block / transfer */ | 8 /* block size in bytes */;

    if (send_command(ACMD_SEND_SCR, 0, nullptr) || sd_int(0x00000020)) {
        printf("error sending scr\n");
        return -1;
    }

    sd_scr[0] = sd_scr[1] = 0;

    r = 0;
    long cnt = 100000;
    while (r < 2 && cnt--) {
        if (*SD_STATUS & SR_READ_AVAILABLE)
            sd_scr[r++] = *SD_DATA;
        else
            wait_msec(1);
    }
    if (r != 2) {
        return -1;
    }
    if (sd_scr[0] & 0x00000400) {
        if (send_command(ACMD_SET_BUT_WIDTH, sd_rca | 2, nullptr)) {
            return -1;
        }
        *SD_CTRL0 |= 0x2;
    }
    sd_scr[0] &= ~0x00000001;
    sd_scr[0] |= ccs;

    // Query the card capacity during initialization.
    // NOT WORKING ATM.
    sd_get_num_blocks();
    return 0;
}

// Helper function to extract bits from a 128-bit CSD array.
// 'start_bit' is the bit number (127=MSB, 0=LSB) and 'num_bits' is the field width.
uint32_t extract_bits(const uint32_t *csd, int start_bit, int num_bits) {
    uint32_t result = 0;
    for (int i = 0; i < num_bits; i++) {
        int bit_pos = start_bit - i;
        int word = bit_pos / 32;  // Which 32-bit word
        int bit = bit_pos % 32;   // Bit index within that word
        result = (result << 1) | ((csd[word] >> bit) & 1);
    }
    return result;
}

// Add this global variable at the top with other globals
uint32_t sd_card_blocks = (NUM_SD_MB_AVAILABLE * 1024 * 1024) / SD_BLK_SIZE;  // 1MB card.

/**
 * Retrieves the total number of blocks available on the SD card.
 * The first call reads from the card's CSD register and caches the result.
 * Subsequent calls return the cached value for performance.
 *
 * @return The total number of blocks (sectors) on the card, or 0 on failure
 * THIS IS NOT WORKING ATM.
 */
uint32_t sd_get_num_blocks() {
    // If we already queried the card, return cached value
    if (sd_card_blocks > 0) {
        return sd_card_blocks;
    }

    LockGuard<SpinLock> g{sd_lock};

    // Storage for CSD register data (128 bits = 16 bytes)
    uint32_t csd[4] = {0};
    int status = 0;
    int response = 0;

    csd[0] = *SD_RESP_REG_0;
    csd[1] = *SD_RESP_REG_1;
    csd[2] = *SD_RESP_REG_2;
    csd[3] = *SD_RESP_REG_3;

    printf("csd: %x, %x, %x, %x\n", csd[0], csd[1], csd[2], csd[3]);

    // Send CMD9 (SEND_CSD) to get the card's CSD register
    HANDLE_ERROR(send_command(CMD_SEND_CSD, sd_rca, &response), status, 0);

    // time out for 10 seconds
    for (int i = 0; i < 1000000; i++) {
        wait_msec(1);
    }

    // Wait for data to be ready
    // HANDLE_ERROR(sd_int(INT_READ_RDY), status, 0);

    printf("CMD sent. Reading from response registers.\n");
    // Read the CSD data (16 bytes = 4 words)
    csd[0] = *SD_RESP_REG_0;
    csd[1] = *SD_RESP_REG_1;
    csd[2] = *SD_RESP_REG_2;
    csd[3] = *SD_RESP_REG_3;

    printf("csd: %x, %x, %x, %x\n", csd[0], csd[1], csd[2], csd[3]);

    // Check if the card is high capacity (SDHC/SDXC) or standard capacity (SDSC)
    bool is_high_capacity = (sd_scr[0] & SCR_SUPP_CCS) != 0;
    is_high_capacity = true;
    uint32_t num_blocks = 0;

    if (is_high_capacity) {
        // For SDHC/SDXC (CSD Version 2.0)
        // C_SIZE is bits 69:48 of the CSD
        uint32_t c_size = ((csd[1] & 0x3F) << 16) | ((csd[2] & 0xFFFF0000) >> 16);
        // Capacity = (C_SIZE + 1) * 512KiB
        // Convert to 512-byte blocks: (C_SIZE + 1) * 1024
        num_blocks = (c_size + 1) * 1024;
    } else {
        // For SDSC (CSD Version 1.0)
        // Extract C_SIZE (bits 73:62), C_SIZE_MULT (bits 49:47), and READ_BL_LEN (bits 83:80)
        uint32_t c_size = ((csd[1] & 0x03FF) << 2) | ((csd[2] & 0xC0000000) >> 30);
        uint32_t c_size_mult = (csd[2] & 0x00038000) >> 15;
        uint32_t read_bl_len = (csd[1] & 0x00F00000) >> 20;

        // Calculate capacity in bytes: (C_SIZE + 1) * 2^(C_SIZE_MULT + 2) * 2^READ_BL_LEN
        uint64_t capacity_bytes =
            (uint64_t)(c_size + 1) * (1ULL << (c_size_mult + 2)) * (1ULL << read_bl_len);
        // Convert to number of 512-byte blocks
        num_blocks = capacity_bytes / SD_BLK_SIZE;
    }

    if (DEBUG) {
        printf("SD Card capacity: %u blocks (%u MB)\n", num_blocks,
               (num_blocks / 2048));  // Convert to MB (2048 blocks = 1MB)
    }

    // Cache the result for future calls
    sd_card_blocks = num_blocks;
    return num_blocks;
}

int sd_init() {
    return init_gpio_pins() == 0 && init_cmd_seq() == 0 ? 0 : -1;
}

int sd_read_block(unsigned int block_addr, unsigned char *buffer, unsigned int num_blocks) {
    LockGuard<SpinLock> g{sd_lock};
    if (DEBUG) printf("Calling sd_read_block(): %x, %x, %u\n", block_addr, buffer, num_blocks);

    int status = 0;

    // Check that we are allowed to receive data via status register.
    HANDLE_ERROR(sd_status(SR_DAT_INHIBIT), status, SD_TIMEOUT)

    unsigned int *int_buf = (unsigned int *)buffer;
    int response = 0;

    // Changing card capacity details.
    // Main difference is that high-capacity cards are block addressed, standard-capacity => byte
    // addressed. Issue read command. Unsure why we don't set this in sd_init_cmd_seq.
    int supports_ccs = sd_scr[0] & SCR_SUPP_CCS;
    int supports_set_blkcount = sd_scr[0] & SCR_SUPP_SET_BLKCNT;
    if (!supports_ccs) {
        printf("We don't support a high capacity SD card....\n");
        *SD_BLKSIZECNT = (1 << 16) | SD_BLK_SIZE;
    } else {
        if (num_blocks > 1 && supports_set_blkcount) {
            // Tell SD how many blocks to expect for multi-block transfer.
            HANDLE_ERROR(send_command(CMD_SET_BLOCKCNT, num_blocks, &response), status,
                         0 /* bytes read*/)
        }

        *SD_BLKSIZECNT = (1 << 16) | SD_BLK_SIZE;

        // Tell SD card the base logical block #.
        HANDLE_ERROR(
            send_command(num_blocks == 1 ? CMD_READ_SINGLE : CMD_READ_MULTI, block_addr, &response),
            status, SD_ERROR);
    }

    // Get blocks back.
    for (unsigned int blk_cnt = 0; blk_cnt < num_blocks; blk_cnt++) {
        // If we don't support CCS, request one block at a time.
        if (!supports_ccs) {
            uint32_t read_addr_bytes = (block_addr + blk_cnt) * SD_BLK_SIZE;
            HANDLE_ERROR(send_command(CMD_READ_SINGLE, read_addr_bytes, &response), status,
                         0 /* bytes read*/)
        }
        HANDLE_ERROR(sd_int(INT_READ_RDY), status, 0 /* bytes read*/)

        // Read one block of data from mem-mapped register.
        // Reading from mem-mapped register clears it and advances the ptr.
        // The register size is 4 bytes, which is why we cast the read buffer to int.
        for (unsigned int i = 0; i < SD_BLK_SIZE / sizeof(int32_t); i++) {
            int_buf[i] = *SD_DATA;
        }

        // Advance the buf ptr by 1 block.
        int_buf += SD_BLK_SIZE / sizeof(int32_t);
    }

    if (num_blocks > 1 && supports_ccs && !supports_set_blkcount) {
        HANDLE_ERROR(send_command(CMD_STOP_TRANS, 0, &response), status, 0 /* bytes read*/)
    }

    return num_blocks * SD_BLK_SIZE;
}

int sd_write_block(unsigned char *buffer, unsigned int block_addr, unsigned int num_blocks) {
    LockGuard<SpinLock> g{sd_lock};
    if (DEBUG) printf("Calling sd_write_block(): %x, %x, %u\n", block_addr, buffer, num_blocks);

    int status = 0;

    // Check that we are allowed to receive data via status register.
    HANDLE_ERROR(sd_status(SR_DAT_INHIBIT | SR_WRITE_AVAILABLE), status, SD_TIMEOUT)

    unsigned int *int_buf = (unsigned int *)buffer;
    int response = 0;

    // Changing card capacity details.
    // Main difference is that high-capacity cards are block addressed, standard-capacity => byte
    // addressed. Issue read command. Unsure why we don't set this in sd_init_cmd_seq.
    int supports_ccs = sd_scr[0] & SCR_SUPP_CCS;
    int supports_set_blkcount = sd_scr[0] & SCR_SUPP_SET_BLKCNT;
    if (!supports_ccs) {
        printf("sd_write_block(): We don't support a high capacity SD card....\n");
        *SD_BLKSIZECNT = (1 << 16) | SD_BLK_SIZE;
    } else {
        if (num_blocks > 1 && supports_set_blkcount) {
            // Tell SD how many blocks to expect for multi-block transfer.
            HANDLE_ERROR(send_command(CMD_SET_BLOCKCNT, num_blocks, &response), status,
                         0 /* bytes written */)
        }

        *SD_BLKSIZECNT = (1 << 16) | SD_BLK_SIZE;

        // Tell SD card the base logical block #.
        HANDLE_ERROR(send_command(num_blocks == 1 ? CMD_WRITE_SINGLE : CMD_WRITE_MULTI, block_addr,
                                  &response),
                     status, SD_ERROR);
    }

    // Get blocks back.
    for (unsigned int blk_cnt = 0; blk_cnt < num_blocks; blk_cnt++) {
        // If we don't support CCS, request one block at a time.
        if (!supports_ccs) {
            uint32_t read_addr_bytes = (block_addr + blk_cnt) * SD_BLK_SIZE;
            HANDLE_ERROR(send_command(CMD_WRITE_SINGLE, read_addr_bytes, &response), status,
                         0 /* bytes written */)
        }
        HANDLE_ERROR(sd_int(INT_WRITE_RDY), status, 0 /* bytes written */)

        // Write one block of data to mem-mapped register.
        // Writing to mem-mapped register clears it and advances the ptr.
        // The register size is 4 bytes, which is why we cast the read buffer to int.
        for (unsigned int i = 0; i < SD_BLK_SIZE / sizeof(int32_t); i++) {
            *SD_DATA = int_buf[i];
        }

        // Advance the buf ptr by 1 block.
        int_buf += SD_BLK_SIZE / sizeof(int32_t);
    }

    if (num_blocks > 1 && supports_ccs && !supports_set_blkcount) {
        HANDLE_ERROR(send_command(CMD_STOP_TRANS, 0, &response), status, 0 /* bytes written */)
    }

    return num_blocks * SD_BLK_SIZE;
}