#ifndef _SNAKE_H
#define _SNAKE_H

#include <stdint.h>

#include "keyboard.h"

#define SCALE 16
#define WIDTH 680
#define HEIGHT 480

#define DIRECTION_NONE ((1 << 4) | (1 << 0))
#define DIRECTION_UP ((1 << 4) | (0 << 0))
#define DIRECTION_DOWN ((1 << 4) | (2 << 0))
#define DIRECTION_RIGHT ((2 << 4) | (1 << 0))
#define DIRECTION_LEFT ((0 << 4) | (1 << 0))
#define QUEUE_SIZE 680
#define FPS 20

#define MAX_PLAYERS 4
#define MAX_FOOD 5

#define MAX_BODY_LENGTH 100

#define MSG_JOIN 1
#define MSG_JOIN_ACK 2
#define MSG_INPUT 3
#define MSG_STATE_UPDATE 4
#define MSG_HEARTBEAT 5

static const uint32_t colors[] = {
    0xFF0000FF,  // Red
    0x00FF00FF,  // Green
    0x0000FFFF,  // Blue
    0xFFFF00FF   // Yellow
};

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

// struct snake_t {
//     uint16_t x[MAX_BODY_LENGTH];
//     uint16_t y[MAX_BODY_LENGTH];
//     uint8_t cur_length;
//     uint8_t direction;
// };

typedef struct {
    Listener<struct key_event *> *key_listener;
    int num_players;
    int num_food;

    // snake info
    // struct snake_t {
    //     uint8_t cur_length;
    //     uint8_t max_length;
    //     uint8_t x;
    //     uint8_t y;
    //     uint8_t direction;
    //     uint8_t prior_direction;
    // } __attribute__((packed)) snake[MAX_PLAYERS];
    struct snake_t {
        uint64_t uuid;
        uint16_t x;
        uint16_t y;
        uint8_t cur_length;
        uint8_t direction;
    } snakes[MAX_PLAYERS];

    // food info
    struct food_t {
        uint8_t x;
        uint8_t y;
        struct flags {
            bool draw : 1;
            bool generate : 1;
        } __attribute__((packed)) flags;
    } __attribute__((packed)) food[MAX_FOOD];

} __attribute__((packed)) game_state;

void init_snake();

#endif