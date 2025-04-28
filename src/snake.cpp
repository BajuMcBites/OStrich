#include "snake.h"

#include "dwc.h"
#include "framebuffer.h"
#include "libk.h"
#include "listener.h"
#include "peripherals/timer.h"
#include "peripherals/usb_hid_keys.h"
#include "printf.h"
#include "rand.h"
#include "timer.h"

int fb_pitch;
int fb_size;
uint32_t *fb_buffer;
Rand rand;

uint8_t screen[WIDTH / SCALE][HEIGHT / SCALE];

uint16_t enqueue_ptr = 0;
uint16_t dequeue_ptr = 0;
undo undo_queue[QUEUE_SIZE];

game_state state;

int get_queue_size() {
    int adj = 0;
    if (enqueue_ptr < dequeue_ptr) {
        adj += QUEUE_SIZE;
    }
    return (enqueue_ptr + adj) - (dequeue_ptr) + 1;
}

int tick(game_state *state) {
    game_state::snake_t *snake = &state->snake;
    game_state::food_t *food = &state->food;
    snake->prior_direction = snake->direction;

    snake->x += ((snake->direction >> 4) & 0xF) - 1;
    snake->y += (snake->direction & 0xF) - 1;

    if (snake->x == food->x && snake->y == food->y) {
        snake->max_length += 1;
        food->flags.generate = true;
    }
    if (food->flags.generate) {
        uint8_t x = ((WIDTH / SCALE) * (rand.random() + 1000)) / 2000;
        uint8_t y = ((HEIGHT / SCALE) * (rand.random() + 1000)) / 2000;
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
            *((uint32_t *)fb_get()->buffer + ptr) = color;
        }
    }
}

void clear_undo(int n) {
    while (n--) {
        int idx = (dequeue_ptr++ % QUEUE_SIZE);
        undo::undo_u::rectangle_undo *pixels = &undo_queue[idx].pixels.rectangle;
        draw_rectangle(pixels->x, pixels->y, pixels->width, pixels->height, undo_queue[idx].color);
    }
}

int render(game_state *state) {
    game_state::snake_t *snake = &state->snake;
    game_state::food_t *food = &state->food;

    uint16_t x = snake->x;
    uint16_t y = snake->y;

    if (x < 0 || y < 0 || x >= WIDTH / SCALE || y >= HEIGHT / SCALE) return 1;

    uint32_t ptr = (x * SCALE) * fb_pitch + (y * SCALE);

    if (get_queue_size() > snake->max_length) {
        clear_undo(get_queue_size() - snake->max_length);
    }

    draw_rectangle(x * SCALE, y * SCALE, SCALE, SCALE, 0xFF0000FF);

    if (food->flags.draw) {
        draw_rectangle(food->x * SCALE, food->y * SCALE, SCALE, SCALE, 0xFFF0000);
        food->flags.draw = false;
    }

    undo_queue[enqueue_ptr++ % QUEUE_SIZE] = {
        .pixels = {.rectangle = {.x = x * SCALE, .y = y * SCALE, .width = SCALE, .height = SCALE}},
        .color = 0xFFFFFFFF};

    return 0;
}

void snake_on_key_press(struct key_event *event) {
    if (event->flags.released) return;

    if (event->keycode == KEY_W && state.snake.prior_direction != DIRECTION_DOWN) {
        state.snake.direction = DIRECTION_UP;
    } else if (event->keycode == KEY_D && state.snake.prior_direction != DIRECTION_LEFT) {
        state.snake.direction = DIRECTION_RIGHT;
    } else if (event->keycode == KEY_S && state.snake.prior_direction != DIRECTION_UP) {
        state.snake.direction = DIRECTION_DOWN;
    } else if (event->keycode == KEY_A && state.snake.prior_direction != DIRECTION_RIGHT) {
        state.snake.direction = DIRECTION_LEFT;
    }
}

void init() {
    state.snake = {.cur_length = 0,
                   .max_length = 3,
                   .x = (WIDTH / (2 * SCALE)),
                   .y = (HEIGHT / (2 * SCALE)),
                   .direction = DIRECTION_RIGHT,
                   .prior_direction = 0};
    state.food = {.x = 75, .y = 15, .flags = {.draw = false, .generate = true}};

    state.key_listener = {.handler = (void (*)(key_event *))&snake_on_key_press, .next = nullptr};

    event_handler->register_listener(KEYBOARD_EVENT, &state.key_listener);
    fb_clear(0xFFFFFFFF);
}

void reset() {
    event_handler->unregister_listener(KEYBOARD_EVENT, state.key_listener._id);
    fb_clear(0xFFFFFFFF);
    init_snake();
}

void init_snake() {
    init();
    fb_pitch = fb_get()->pitch >> 2;
    fb_size = fb_get()->size;
    fb_buffer = (uint32_t *)fb_get()->buffer;

    while (1) {
        if (tick(&state) || render(&state)) {
            break;
        }
        wait_msec(1000000 / FPS);
    }
    reset();
}

int main() {
    init_snake();
    return 0;
}