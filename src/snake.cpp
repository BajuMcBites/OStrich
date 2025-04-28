#include "snake.h"

#include "dwc.h"
#include "framebuffer.h"
#include "ksocket.h"
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
uint8_t uuid[16] = {0};

int get_queue_size() {
    int adj = 0;
    if (enqueue_ptr < dequeue_ptr) {
        adj += QUEUE_SIZE;
    }
    return (enqueue_ptr + adj) - (dequeue_ptr) + 1;
}

// Unused.
// int tick(game_state *state) {
//     game_state::snake_t *snake = &state->snake[0];
//     game_state::food_t *food = &state->food[0];
//     snake->prior_direction = snake->direction;

//     snake->x += ((snake->direction >> 4) & 0xF) - 1;
//     snake->y += (snake->direction & 0xF) - 1;

//     if (snake->x == food->x && snake->y == food->y) {
//         snake->max_length += 1;
//         food->flags.generate = true;
//     }
//     if (food->flags.generate) {
//         uint8_t x = ((WIDTH / SCALE) * (rand.random() + 1000)) / 2000;
//         uint8_t y = ((HEIGHT / SCALE) * (rand.random() + 1000)) / 2000;
//         food->x = x;
//         food->y = y;
//         food->flags.generate = false;
//         food->flags.draw = true;
//     }
//     return 0;
// }

void draw_rectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color) {
    uint32_t ptr = y * fb_pitch + x;
    uint32_t bottom_left = ((y + height) * fb_pitch) + (x + width);

    for (size_t y = 0; y < height; y++, ptr += fb_pitch - width) {
        for (size_t x = 0; x < width; x++, ptr++) {
            *(fb_buffer + ptr) = color;
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
    for (size_t i = 0; i < state->num_players; i++) {
        game_state::snake_t *snake = &state->snake[i];

        if (x < 0 || y < 0 || x >= WIDTH / SCALE || y >= HEIGHT / SCALE) return 1;

        // Naively render the snake.
        for (int j = 0; j < MAX_BODY_LENGTH; j++) {
            draw_rectangle(state.full_snake[i].x[j] * SCALE, state.full_snake[i].y[j] * SCALE,
                           SCALE, SCALE, colors[i]);
        }
    }

    for (size_t i = 0; i < state->num_food; i++) {
        game_state::food_t *food = &state->food[i];
        draw_rectangle(food->x * SCALE, food->y * SCALE, SCALE, SCALE, 0xFFF0000);
        // if (food->flags.draw) {
        //     food->flags.draw = false;
        // }
    }

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
    get_event_handler().register_listener(KEYBOARD_EVENT, &state.key_listener);
    fb_clear(0xFFFFFFFF);
}

void reset() {
    // get_event_handler().unregister_listener(KEYBOARD_EVENT, state.key_listener._id);
    fb_clear(0xFFFFFFFF);
    init_snake();
}

void handle_join_ack(UDPSocket *socket) {
    uint8_t buffer[17];
    volatile int i = 1000000;
    int nbytes = socket->recv(buffer);
    if (nbytes == 17 && buffer[0] == MSG_JOIN_ACK) {
        K::memcpy(uuid, buffer[1], 16);
    }
}

void request_join(UDPSocket *socket) {
    char join_msg[17] = {0};
    join_msg[0] = MSG_JOIN;
    join_msg[1] = 0x00;
    join_msg[2] = 0x00;
    join_msg[3] = 0x00;
    join_msg[4] = 0x00;
    join_msg[5] = 0x00;
    join_msg[6] = 0x00;
    socket->send_udp((const uint8_t *)join_msg, 17);
    handle_join_ack(socket);
}

void update_game_state(UDPSocket *socket) {
    // Ask for game state.
    char game_state_msg[17] = {0};
    game_state_msg[0] = MSG_HEARTBEAT;
    K::memcpy(game_state_msg[1], uuid, 16);
    socket->send_udp((const uint8_t *)game_state_msg, 17);

    // Wait for game state.
    uint8_t timestamp;

    char buffer[1024];
    int nbytes = socket->recv(buffer);
    if (buffer[0] == MSG_STATE_UPDATE) {  // Only type of message we can receive.
        timestamp = buffer[1];
        state.num_food = buffer[2];
        state.num_players = buffer[3];
    }

    // State update:
    // [1 byte: message type] [4 bytes: timestamp] [1 byte: players count] [1 byte: food count]
    // [food_count * 2 shorts: food coordinates]
    // [For each player: [16 bytes: player UUID] [1 byte: direction] [1 byte: snake length]
    // [snake_length * 2 shorts: snake body]]

    game_state::snake_t *snake = &state.snake[0];
    game_state::food_t *food = &state.food[0];

    char *food_buffer = buffer + 7;
    char *snake_buffer = food_buffer + (food_count * 2);

    for (size_t i = 0; i < MAX_FOOD; i++) {
        if (i < state.num_food) {
            food[i].x = food_buffer[i * 2];
            food[i].y = food_buffer[i * 2 + 1];
        } else {
            food[i].x = 0;
            food[i].y = 0;
        }
    }

    // possible bug if someone disconnects but whatever.
    for (size_t i = 0; i < MAX_PLAYERS; i++) {
        game_state::full_snake_t *snake = &state.full_snake[i];
        snake->cur_length = snake_buffer[i * 2];
        snake->direction = snake_buffer[i * 2 + 1];
        for (int j = 0; j < MAX_BODY_LENGTH; j++) {
            if (i < state.num_players) {
                snake->x[j] = snake_buffer[i * 2];
                snake->y[j] = snake_buffer[i * 2 + 1];
            } else {
                snake->x[j] = 0;
                snake->y[j] = 0;
            }
        }
    }
}

// Whatever the latest input is, send it to the server.
// Seems buggy but hopefully things run fast enough to not notice lag.
void send_input(UDPSocket *socket) {
    char input_msg[17] = {0};
    input_msg[0] = MSG_INPUT;
    K::memcpy((void *)input_msg[1], uuid, 16);
    input_msg[17] = state.snake[0].direction;
    socket->send_udp((const uint8_t *)input_msg, 17);
}

void init_snake() {
    UDPSocket socket(/* localhost but its unused internally */ 0x7F000001,
                     /* port (also unused?)*/ 25565);

    request_join(&socket);

    init();
    fb_pitch = fb_get()->pitch >> 2;
    fb_size = fb_get()->size;
    fb_buffer = (uint32_t *)fb_get()->buffer;

    while (1) {
        update_game_state(&socket);  // updates global game state
        // we don't need to tick because the server will do that.
        if (render(&state)) {
            break;
        }
        send_input(&socket);
        wait_msec(1000000 / FPS);
    }
    reset();
}

int main() {
    init_snake();
    return 0;
}