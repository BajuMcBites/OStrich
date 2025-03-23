#include "snake.h"

#include "framebuffer.h"
#include "libk.h"
#include "printf.h"
#include "rand.h"

#define SCALE 8
#define SCALE_BITS 3

#define DIRECTION_UP ((1 << 4) | (0 << 0))
#define DIRECTION_DOWN ((1 << 4) | (2 << 0))
#define DIRECTION_RIGHT ((2 << 4) | (1 << 0))
#define DIRECTION_LEFT ((0 << 4) | (1 << 0))
#define QUEUE_SIZE 680

int fb_pitch;
int fb_size;
uint32_t *fb_buffer;
Rand rand;

uint8_t screen[680 / 8][480 / 8];

uint16_t enqueue_ptr = 0;
uint16_t dequeue_ptr = 0;
undo undo_queue[QUEUE_SIZE];

int enqueue() {
    return (enqueue_ptr++ % QUEUE_SIZE);
}

int dequeue() {
    return (dequeue_ptr++ % QUEUE_SIZE);
}

int get_queue_size() {
    int adj = 0;
    if (enqueue_ptr < dequeue_ptr) {
        adj += QUEUE_SIZE;
    }
    return (enqueue_ptr + adj) - (dequeue_ptr) + 1;
}

game_state init() {
    return {.snake = {.cur_length = 0,
                      .max_length = 3,
                      .x = (680 >> 5),
                      .y = (480 >> 5),
                      .direction = DIRECTION_RIGHT},
            .food = {.x = 75, .y = 15, .flags = {.draw = false, .generate = true}}};
}

int tick(game_state *state) {
    game_state::snake_t *snake = &state->snake;
    game_state::food_t *food = &state->food;

    if (snake->x == food->x) {
        snake->direction = snake->y > food->y ? DIRECTION_UP : DIRECTION_DOWN;
    } else {
        snake->direction = snake->x > food->x ? DIRECTION_LEFT : DIRECTION_RIGHT;
    }

    snake->x += ((snake->direction >> 4) & 0xF) - 1;
    snake->y += (snake->direction & 0xF) - 1;

    if (snake->x == food->x && snake->y == food->y) {
        snake->max_length += 1;
        food->flags.generate = true;
    }
    if (food->flags.generate) {
        uint8_t x = (85 * (rand.random() + 1000)) / 2000;
        uint8_t y = (60 * (rand.random() + 1000)) / 2000;
        food->x = x;
        food->y = y;
        food->flags.generate = false;
        food->flags.draw = true;
    }
    return 0;
}

void draw_rectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color) {
    uint32_t ptr = y * fb_pitch + x;
    uint32_t bottom_left = ((y + height) * fb_pitch) + (x + width);

    for (size_t y = 0; y < height; y++, ptr += fb_pitch - width) {
        for (size_t x = 0; x < width; x++, ptr++) {
            *(fb_buffer + ptr) = color;
        }
    }
}

int check_bounds(uint8_t x, uint8_t y) {
    return x >= (680 >> 3) || y >= (480 >> 3);
}

void clear_undo(int n) {
    while (n--) {
        int idx = dequeue();
        undo::undo_u::rectangle_undo *pixels = &undo_queue[idx].pixels.rectangle;
        draw_rectangle(pixels->x, pixels->y, pixels->width, pixels->height, undo_queue[idx].color);
    }
}

int render(game_state *state) {
    game_state::snake_t *snake = &state->snake;
    game_state::food_t *food = &state->food;

    uint16_t x = snake->x;
    uint16_t y = snake->y;

    if (check_bounds(x, y)) return 1;

    uint32_t ptr = (x * SCALE) * fb_pitch + (y * SCALE);

    if (get_queue_size() > snake->max_length) {
        clear_undo(get_queue_size() - snake->max_length);
    }

    // draw updated head
    draw_rectangle(x << SCALE_BITS, y << SCALE_BITS, SCALE, SCALE, 0xFF0000FF);

    if (food->flags.draw) {
        draw_rectangle(food->x << SCALE_BITS, food->y << SCALE_BITS, SCALE, SCALE, 0xFFF0000);
        food->flags.draw = false;
    }

    undo_queue[enqueue()] = {
        .pixels = {.rectangle = {.x = x * SCALE, .y = y * SCALE, .width = SCALE, .height = SCALE}},
        .color = 0xFFFFFFFF};

    return 0;
}

#define FPS 5

void game_delay() {
    for (int i = 0; i < 120000000 / FPS; i++) {
    }
}

#define KEY_READY ((volatile unsigned int *)(0x3F000000 + 0x18))

void init_snake() {
    game_state state = init();
    fb_pitch = fb_get()->pitch >> 2;
    fb_size = fb_get()->size;
    fb_buffer = (uint32_t *)fb_get()->buffer;

    while (1) {
        if (tick(&state) || render(&state)) {
            break;
        }
        game_delay();
    }
}

int main() {
    init_snake();
    return 0;
}