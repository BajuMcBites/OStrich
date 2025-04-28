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

int get_queue_size() {
    int adj = 0;
    if (enqueue_ptr < dequeue_ptr) {
        adj += QUEUE_SIZE;
    }
    return (enqueue_ptr + adj) - (dequeue_ptr) + 1;
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

void clear_undo(int n) {
    while (n--) {
        int idx = (dequeue_ptr++ % QUEUE_SIZE);
        undo::undo_u::rectangle_undo *pixels = &undo_queue[idx].pixels.rectangle;
        draw_rectangle(pixels->x, pixels->y, pixels->width, pixels->height, undo_queue[idx].color);
    }
}

int render(game_state *state) {
    // draw_rectangle(0, 0, WIDTH, HEIGHT, 0x11FFFFFF);
    for (size_t i = 0; i < state->num_players; i++) {
        game_state::snake_t *snake = &state->snakes[i];

        if (snake->x < 0 || snake->y < 0 || snake->x >= WIDTH / SCALE ||
            snake->y >= HEIGHT / SCALE) {
            continue;
        }
        // Naively render the snake.
        draw_rectangle(state->snakes[i].x * SCALE, state->snakes[i].y * SCALE, SCALE, SCALE,
                       colors[i]);
    }

    return 0;
}

void snake_on_key_press(struct key_event *event) {
    if (event->flags.released) return;
    if (event->keycode == KEY_W) {
        state.snakes[0].direction = event->flags.pressed ? DIRECTION_UP : DIRECTION_NONE;
    } else if (event->keycode == KEY_D) {
        state.snakes[0].direction = event->flags.pressed ? DIRECTION_RIGHT : DIRECTION_NONE;
    } else if (event->keycode == KEY_S) {
        state.snakes[0].direction = event->flags.pressed ? DIRECTION_DOWN : DIRECTION_NONE;
    } else if (event->keycode == KEY_A) {
        state.snakes[0].direction = event->flags.pressed ? DIRECTION_LEFT : DIRECTION_NONE;
    }
}

void init() {
    K::memset(&state, 0, sizeof(state));

    state.snakes[0] = {.x = (WIDTH / (2 * SCALE)),
                       .y = (HEIGHT / (2 * SCALE)),
                       .cur_length = 1,
                       .direction = DIRECTION_NONE};
    state.food[0] = {.x = 75, .y = 15, .flags = {.draw = false, .generate = true}};

    state.key_listener = new Listener<key_event *>(&snake_on_key_press);
    get_event_handler()->register_listener(KEYBOARD_EVENT, state.key_listener);
    fb_clear(0xFFFFFFFF);
}

void reset() {
    fb_clear(0xFFFFFFFF);
    init_snake();
}

void handle_join_ack(UDPSocket *socket) {
    uint8_t buffer[17];
    volatile int i = 1000000;
    int nbytes = socket->recv(buffer);
    printf("nbytes == %d\n", nbytes);
    if (nbytes == 9 && buffer[0] == MSG_JOIN_ACK) {
        K::memcpy(&state.snakes[0].uuid, buffer + 1, 8);
        printf("Obtain Snake's UUID from Server: %x\n", state.snakes[0].uuid);
    }
}

void request_join(UDPSocket *socket) {
    char join_msg[17] = {0};
    join_msg[0] = MSG_JOIN;
    socket->send_udp((const uint8_t *)join_msg, sizeof(join_msg));
    handle_join_ack(socket);
}

void send_update(UDPSocket *socket) {
    char input_msg[13] = {0};
    input_msg[0] = MSG_INPUT;
    K::memcpy(input_msg + 1, &state.snakes[0].uuid, 8);
    K::memcpy(input_msg + 9, &state.snakes[0].x, 2);
    K::memcpy(input_msg + 11, &state.snakes[0].y, 2);

    uint64_t sent_uuid = *((uint64_t *)(input_msg + 1));

    socket->send_udp((const uint8_t *)input_msg, sizeof(input_msg));
}

void update_game_state(UDPSocket *socket) {
    static char buffer[1024];

    state.snakes[0].x += (state.snakes[0].direction >> 4) - 1;
    state.snakes[0].y += (state.snakes[0].direction & 0x0F) - 1;
    
    state.snakes[0].x %= WIDTH / SCALE;
    state.snakes[0].y %= WIDTH / SCALE;
    

    send_update(socket);

    // Wait for game state.
    uint32_t timestamp;

    int nbytes = socket->recv((uint8_t *)buffer);

    if (buffer[0] == MSG_STATE_UPDATE) {  // Only type of message we can receive.
        memcpy(&timestamp, buffer + 1, 4);
        state.num_players = buffer[5];
        uint32_t offset = 6;
        for (int i = 0; i < state.num_players; i++) {
            uint64_t player_uuid;
            memcpy(&player_uuid, buffer + offset, 8);
            offset += 8;
            uint16_t x;
            uint16_t y;
            memcpy(&x, buffer + offset, 2);
            offset += 2;
            memcpy(&y, buffer + offset, 2);
            offset += 2;
            printf("player_uuid = %x, x = %d, y = %d\n", player_uuid, x, y);
            if(state.snakes[0].uuid == player_uuid) continue;

            bool matched = false;
            for(size_t i = 0; i < MAX_PLAYERS; i++){
                if(state.snakes[i].uuid == player_uuid){
                    state.snakes[i].x = x;
                    state.snakes[i].y = y;
                }
            }
            if(matched) continue;
            for(size_t i = 0; i < MAX_PLAYERS; i++){
                if(state.snakes[i].uuid == 0x00){
                    state.snakes[i].x = x;
                    state.snakes[i].y = y;
                    state.snakes[i].uuid = player_uuid;
                }
            }
        }
        // printf("state.num_players = %d\n", state.num_players);
    }

    game_state::snake_t *snake = &state.snakes[0];

    // char *food_buffer = buffer + 7;
    // char *snake_buffer = food_buffer + (state.num_food * 2);

    // for (size_t i = 0; i < MAX_FOOD; i++) {
    //     if (i < state.num_food) {
    //         state.food[i].x = food_buffer[i * 2];
    //         state.food[i].y = food_buffer[i * 2 + 1];
    //     } else {
    //         state.food[i].x = 0;
    //         state.food[i].y = 0;
    //     }
    // }

    // possible bug if someone disconnects but whatever.
    // for (size_t i = 1; i < MAX_PLAYERS; i++) {
    //     game_state::snake_t *snake = &state.snakes[i];
    //     snake->cur_length = snake_buffer[i * 2];
    //     snake->direction = snake_buffer[i * 2 + 1];
    //     snake->x = snake_buffer[i + 2 + 2];
    //     snake->y = snake_buffer[i + 2 + 2];
    // }
}

void init_snake() {
    UDPSocket socket(/* localhost but its unused internally */ 0xc0a8d04b,
                     /* port (also unused?)*/ 25566);

    init();

    request_join(&socket);

    fb_pitch = fb_get()->pitch >> 2;
    fb_size = fb_get()->size;
    fb_buffer = (uint32_t *)fb_get()->buffer;

    while (1) {
        update_game_state(&socket);  // updates global game state
        // we don't need to tick because the server will do that.
        if (render(&state)) {
            break;
        }
        wait_msec(1000000 / FPS);
    }
    socket.close();
    reset();
}

int main() {
    init_snake();
    return 0;
}