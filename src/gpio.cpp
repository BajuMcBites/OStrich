#include "gpio.h"

#include "printf.h"
#include "timer.h"

gpio_pin_t pins[MAX_GPIO_PINS];

int gpio_request(unsigned idx, const char *pin_name) {
    if (idx >= MAX_GPIO_PINS || pins[idx].in_use) {
        return -1;
    }

    if (pin_name == nullptr) {
        pins[idx].pin_name[0] = '\0';
    } else {
        for (int i = 0; i < 25; i++) {
            pins[idx].pin_name[i] = pin_name[i];
            if (pin_name[i] == '\0') {
                break;
            }
        }
    }
    pins[idx].in_use = true;
    return 0;
}

#define GPFSEL(idx) (GPFSEL0 + (idx / 10))

int gpio_set_direction(unsigned idx, unsigned direction) {
    if (idx >= MAX_GPIO_PINS || !pins[idx].in_use) {
        return -1;
    }
    pins[idx].direction = direction;
    volatile unsigned int *ctrl_reg = GPFSEL(idx);

    unsigned level_idx = idx % 10;

    *ctrl_reg |= (direction << (level_idx * 3));  // Set the direction to output

    return 0;
}

int gpio_set_value(unsigned idx, unsigned value) {
    if (idx >= MAX_GPIO_PINS) {
        return -1;
    }
    pins[idx].value = value;

    volatile unsigned int *ctrl_reg = GPSET1;

    if (value)
        *ctrl_reg |= (1 << (idx));
    else
        *ctrl_reg &= ~(1 << (idx));

    return 0;
}

int gpio_pull(unsigned idx, unsigned val) {
    if (idx >= MAX_GPIO_PINS || !pins[idx].in_use) {
        return -1;
    }
    pins[idx].pull = val;
    *GPPUD = val;
    wait_msec(GPIO_DELAY);

    if (idx >= 32) {
        *GPPUDCLK1 = (1 << (idx - 32));
        wait_msec(GPIO_DELAY);
        *GPPUD = 0;
        *GPPUDCLK1 = 0;
    } else {
        *GPPUDCLK0 = (1 << (idx - 00));
        wait_msec(GPIO_DELAY);
        *GPPUD = 0;
        *GPPUDCLK0 = 0;
    }
    return 0;
}

int gpio_set_high(unsigned idx, unsigned val) {
    if (idx >= MAX_GPIO_PINS || !pins[idx].in_use) {
        return -1;
    }
    pins[idx].high = val;
    unsigned offset = idx > 31 ? 32 : 0;

    if (val)
        *(GPHEN0 + (offset / 8)) |= (val << (idx - offset));
    else
        *(GPHEN0 + (offset / 8)) &= ~(val << (idx - offset));

    return 0;
}

void print_str(char *str) {
    unsigned idx = 0;
    while (str[idx] != 0) {
        printf("%c", str[idx]);
        idx++;
    }
}

void print_binary_groupings(int num) {
    for (int i = 0; i < 30; i++) {
        int group[3];
        for (int j = 0; j < 3; j++) {
            group[j] = (num >> (i + j)) & 1;
        }

        for (int j = 2; j >= 0; j--) {
            printf("%d", group[j]);
        }

        i += 2;
        printf("\t");
    }
    printf("\n");
}

void gpio_print_state(void) {
    printf("\n\nGPIO state ::: \n \n");

    volatile unsigned int *ptr = GPFSEL0;
    printf("\t 0\t 1\t 2\t 3\t 4\t 5\t 6\t 7\t 8\t 9\n");
    for (int i = 0; i < 6; i++) {
        printf("GPFSEL%d: ", i);
        print_binary_groupings(ptr[i]);
    }
    printf("\n\n");
    printf("LEV0: 0x%x\n", *GPLEV0);
    printf("LEV1: 0x%x\n", *GPLEV1);
}

void gpio_init(void) {
    for (int i = 0; i < MAX_GPIO_PINS; i++) {
        pins[i].in_use = 0;
        pins[i].pin_name[0] = '\0';
        pins[i].high = 0;
        pins[i].direction = 0;
    }

    int gpio_led = 17;
    int gpio_button = 18;

    // Request GPIO pins
    gpio_request(gpio_led, "LED");
    gpio_request(gpio_button, "BUTTON");

    // Set direction (input for button, output for LED)
    gpio_set_direction(gpio_button, DIR_OUTPUT);
    gpio_set_direction(gpio_led, DIR_INPUT);
    gpio_set_value(gpio_led, 0);  // Initial state: OFF
}