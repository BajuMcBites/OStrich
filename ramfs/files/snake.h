#ifndef _SNAKE_H
#define _SNAKE_H

#include <stdint.h>

struct undo {
    union undo_u {
        uint64_t raw;
        struct rectangle_undo {
            uint16_t x;
            uint16_t y;
            uint16_t width;
            uint16_t height;
        } __attribute__((packed)) rectangle;
    } pixels;
    uint32_t color;
} __attribute__((packed));

typedef struct {
    // snake info
    struct snake_t {
        uint8_t cur_length;
        uint8_t max_length;
        uint8_t x;
        uint8_t y;
        uint8_t direction;
    } __attribute__((packed)) snake;

    // food info
    struct food_t {
        uint8_t x;
        uint8_t y;
        struct flags {
            bool draw : 1;
            bool generate : 1;
        } __attribute__((packed)) flags;
    } __attribute__((packed)) food;
} __attribute__((packed)) game_state;

void init_snake();

#endif