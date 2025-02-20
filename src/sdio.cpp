#include "sdio.h"

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

unsigned long sd_scr[2], sd_rca = 0, ccs = 0, sd_hv;

int sd_read_block() {
    return 0;
}

int sd_write_block() {
    return 0;
}

void wait_cycles(int c) {
    while (c-- > 0) {
        asm volatile("nop");
    }
}

int sd_int(unsigned int mask) {
    unsigned int r, m = mask | INT_ERROR_MASK;
    int cnt = 100000;
    while (!(*SD_INTERRUPT & m) && cnt--) wait_msec(1);
    r = *SD_INTERRUPT;
    if (cnt <= 0 || (r & INT_CMD_TIMEOUT) || (r & INT_DATA_TIMEOUT)) {
        printf("here a %d, %d, %d\n", cnt, (r & INT_CMD_TIMEOUT), (r & INT_DATA_TIMEOUT));
        *SD_INTERRUPT = r;
        return -2;
    } else if (r & INT_ERROR_MASK) {
        printf("here b\n");
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

int send_command(int command, int arg, int *res) {
    if (command & APP_CMD) {
        int resp;
        int status = send_command(CMD_SEND_APP | (sd_rca ? 0x00020000 : 0), sd_rca, &resp);
        if (sd_rca && !resp) {
            printf("ERROR: failed to send SD APP command\n");
            return -1;
        }
        command &= ~APP_CMD;
    }

    if (sd_status(SR_CMD_INHIBIT)) {
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

unsigned int send_op_cond(int arg) {
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

    unsigned int op_cond_resp = send_op_cond(0x51ff8000);

    if (op_cond_resp == -1 || !(op_cond_resp & 0x00ff8000)) {
        return op_cond_resp < 0 ? op_cond_resp : -op_cond_resp;
    }

    if (op_cond_resp & 0x40000000) ccs = 0x1;

    if (send_command(CMD_ALL_SEND_CID, 0, nullptr)) return -1;

    if (send_command(CMD_SEND_REL_ADDR, 0, (int *)&sd_rca)) return -1;

    sd_rca &= 0xffff0000;

    if ((r = sd_clk(25000000)) || send_command(CMD_CARD_SELECT, sd_rca, nullptr) ||
        sd_status(SR_DAT_INHIBIT))
        return -1;

    *SD_BLKSIZECNT = (1 << 16) | 8;

    if (send_command(ACMD_SEND_SCR, 0, nullptr) || sd_int(0x00000020)) {
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

    return 0;
}

int sd_init() {
    return init_gpio_pins() == 0 && init_cmd_seq() == 0 ? 0 : -1;
}