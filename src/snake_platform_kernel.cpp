#include <cstdint>

#include "framebuffer.h"
#include "keyboard.h"
#include "ksocket.h"
#include "libk.h"
#include "listener.h"
#include "printf.h"
#include "snake_generic.h"
#include "timer.h"

// Core game functions
void SG_Init(void);
void SG_ProcessFrame(void);
void SG_Shutdown(void);
void SG_HandleInput(int key_code);
bool SG_IsRunning(void);

static int LAST_KEY_PRESSED = -1;
static uint32_t* fb_buffer = nullptr;

uint32_t SERVER_IP = 0;
uint16_t SERVER_PORT = 0;

void key_listener(key_event* event) {
    LAST_KEY_PRESSED = event->keycode;
}
// Platform stubs - must be implemented by the platform
// Graphics
bool SG_PlatformInit(void) {
    get_event_handler()->register_listener(KEYBOARD_EVENT,
                                           new Listener<key_event*>(std::move(key_listener)));
    fb_clear(0xFFFFFFFF);
    fb_buffer = (uint32_t*)fb_get()->buffer;
    return true;
}

void SG_PlatformShutdown(void) {
    return;
}

// Constants for drawing
#define GRID_CELL_SIZE 10
#define SNAKE_COLOR 0xFF0000FF        // Red
#define FOOD_COLOR 0xFFFF0000         // Blue
#define OTHER_SNAKE_COLOR 0xFF00FF00  // Green
#define GRID_COLOR 0xFF808080         // Gray
#define BG_COLOR 0xFFFFFFFF           // White

// Helper function to draw a rectangle
void draw_rect(int x, int y, int width, int height, uint32_t color) {
    uint32_t* ptr = fb_buffer + y * (fb_get()->pitch / 4) + x;

    for (int dy = 0; dy < height; dy++) {
        for (int dx = 0; dx < width; dx++) {
            ptr[dx] = color;
        }
        ptr += fb_get()->pitch / 4;
    }
}

void SG_PlatformDrawFrame(struct SnakeGameStateData* state) {
    if (!fb_buffer) {
        return;
    }

    // Clear the screen
    fb_clear(BG_COLOR);

    Framebuffer* fb = fb_get();
    int width = fb->width;
    int height = fb->height;

    // Draw border - using rectangles instead of individual pixels
    draw_rect(0, 0, width, 1, GRID_COLOR);           // Top
    draw_rect(0, height - 1, width, 1, GRID_COLOR);  // Bottom
    draw_rect(0, 0, 1, height, GRID_COLOR);          // Left
    draw_rect(width - 1, 0, 1, height, GRID_COLOR);  // Right

    // Draw food
    for (int i = 0; i < state->food_count; i++) {
        int x = state->food[i].x * GRID_CELL_SIZE;
        int y = state->food[i].y * GRID_CELL_SIZE;

        // Draw a filled square for food
        draw_rect(x, y, GRID_CELL_SIZE, GRID_CELL_SIZE, FOOD_COLOR);
    }

    // Draw players
    for (int i = 0; i < state->player_count; i++) {
        // Select color based on whether it's the main player
        uint32_t color = (i == 0) ? SNAKE_COLOR : OTHER_SNAKE_COLOR;

        // Draw the snake body
        for (int j = 0; j < state->players[i].length; j++) {
            int x = state->players[i].body[j].x * GRID_CELL_SIZE;
            int y = state->players[i].body[j].y * GRID_CELL_SIZE;

            // Head is slightly brighter
            uint32_t segment_color = (j == 0) ? 0xFF0000FF : color;

            // Draw a filled square for snake segment
            draw_rect(x, y, GRID_CELL_SIZE, GRID_CELL_SIZE, segment_color);
        }
    }

    // Draw score and info text
    char score_text[64];
    int score = state->players[0].length - 3;  // Starting length is 3
    K::strcpy(score_text, "Score: ");

    // Convert score to string manually
    int temp_score = score;
    char score_str[16];
    int idx = 0;

    // Handle negative numbers
    if (temp_score < 0) {
        score_str[idx++] = '-';
        temp_score = -temp_score;
    }

    // Convert to digits in reverse
    int start_idx = idx;
    do {
        score_str[idx++] = '0' + (temp_score % 10);
        temp_score /= 10;
    } while (temp_score > 0);
    score_str[idx] = '\0';

    // Reverse the digits
    for (int i = start_idx, j = idx - 1; i < j; i++, j--) {
        char temp = score_str[i];
        score_str[i] = score_str[j];
        score_str[j] = temp;
    }

    K::strcat(score_text, score_str);
    K::strcat(score_text, "  Players: ");

    // Convert player count to string
    temp_score = state->player_count;
    idx = 0;
    do {
        score_str[idx++] = '0' + (temp_score % 10);
        temp_score /= 10;
    } while (temp_score > 0);
    score_str[idx] = '\0';

    // Reverse the digits
    for (int i = 0, j = idx - 1; i < j; i++, j--) {
        char temp = score_str[i];
        score_str[i] = score_str[j];
        score_str[j] = temp;
    }

    K::strcat(score_text, score_str);
    fb_print(score_text, 0xFF000000);
}

void SG_PlatformPrintMessage(const char* message) {
    printf("Snake Client: %s\n", message);
}

// Input
int SG_PlatformGetKey(void) {
    printf("LAST_KEY_PRESSED: %d\n", LAST_KEY_PRESSED);
    return LAST_KEY_PRESSED;
}

// Timing
uint32_t SG_PlatformGetTicksMs(void) {
    return get_systime();
}

void SG_PlatformSleepMs(uint32_t ms) {
    wait_msec(ms);
}

// Networking
bool SG_PlatformNetInit(uint32_t server_host, int server_port) {
    // Connect over UDP
    SERVER_IP = server_host;
    SERVER_PORT = server_port;
    static UDPSocket socket(SERVER_IP, SERVER_PORT);

    // Send a message to the server
    socket.send_udp((uint8_t*)"Hello, server!", 15);
    return true;
}

void SG_PlatformNetShutdown(void) {
    // Close socket.
    return;
}

int SG_PlatformNetSend(const void* data, int length) {
    static UDPSocket socket(SERVER_IP, SERVER_PORT);
    socket.send_udp((uint8_t*)data, length);
    return 0;
}
int SG_PlatformNetReceive(void* buffer) {
    static UDPSocket socket(SERVER_IP, SERVER_PORT);
    return socket.recv((uint8_t*)buffer);
}

bool SG_PlatformNetConnect(void) {
    return true;
}

bool SG_PlatformNetSendJoin(void) {
    return true;
}

bool SG_PlatformNetSendInput(uint8_t direction) {
    static UDPSocket socket(SERVER_IP, SERVER_PORT);
    socket.send_udp((uint8_t*)&direction, sizeof(direction));
    return true;
}

bool SG_PlatformNetSendHeartbeat(void) {
    static UDPSocket socket(SERVER_IP, SERVER_PORT);
    socket.send_udp((uint8_t*)"heartbeat", 9);
    return true;
}

// Helper function to convert network byte order to host byte order
uint16_t ntohs_custom(uint16_t n) {
    return ((n & 0xFF) << 8) | ((n & 0xFF00) >> 8);
}

uint32_t ntohl_custom(uint32_t n) {
    return ((n & 0xFF) << 24) | ((n & 0xFF00) << 8) | ((n & 0xFF0000) >> 8) |
           ((n & 0xFF000000) >> 24);
}

bool SG_PlatformNetParseGameState(const uint8_t* data, int length,
                                  struct SnakeGameStateData* state) {
    // Check minimum length and message type
    if (length < 7 || data[0] != SNAKE_MSG_STATE_UPDATE) {
        return false;
    }

    int offset = 1;

    // Parse timestamp (4 bytes)
    uint32_t timestamp;
    memcpy(&timestamp, data + offset, 4);
    timestamp = ntohl_custom(timestamp);
    offset += 4;

    // Parse counts (1 byte each)
    uint8_t player_count = data[offset++];
    uint8_t food_count = data[offset++];

    // Clear existing state
    state->player_count = 0;
    state->food_count = 0;
    state->timestamp = timestamp;
    state->msg_type = SNAKE_MSG_STATE_UPDATE;

    // Parse food positions (2 shorts per food)
    for (int i = 0; i < food_count && i < SNAKE_MAX_FOOD; i++) {
        if (offset + 4 > length) return false;

        // Parse x,y coordinates (2 bytes each)
        uint16_t x, y;
        memcpy(&x, data + offset, 2);
        memcpy(&y, data + offset + 2, 2);
        offset += 4;

        // Convert from network byte order
        x = ntohs_custom(x);
        y = ntohs_custom(y);

        state->food[i].x = x;
        state->food[i].y = y;
        state->food_count++;
    }

    // Parse players
    for (int i = 0; i < player_count && i < SNAKE_MAX_PLAYERS; i++) {
        if (offset + 18 > length) return false;

        // Parse UUID (16 bytes) - Convert to string and store in player_id
        uint8_t uuid_bytes[16];
        memcpy(uuid_bytes, data + offset, 16);
        offset += 16;

        // Instead of converting UUID to string, we'll just mark the player_id
        // with a placeholder since we don't need it for rendering
        K::strcpy(state->players[i].player_id, "player-");
        char num_str[2] = {'0' + i, '\0'};
        K::strcat(state->players[i].player_id, num_str);

        // Parse direction and body length (1 byte each)
        uint8_t direction = data[offset++];
        uint8_t body_length = data[offset++];

        // Set direction code directly
        state->players[i].direction = direction;

        // Parse body segments (2 shorts per segment)
        state->players[i].length = 0;
        for (int j = 0; j < body_length && j < SNAKE_MAX_BODY_LENGTH; j++) {
            if (offset + 4 > length) return false;

            // Parse x,y coordinates (2 bytes each)
            uint16_t x, y;
            memcpy(&x, data + offset, 2);
            memcpy(&y, data + offset + 2, 2);
            offset += 4;

            // Convert from network byte order
            x = ntohs_custom(x);
            y = ntohs_custom(y);

            state->players[i].body[j].x = x;
            state->players[i].body[j].y = y;
            state->players[i].length++;
        }

        state->player_count++;
    }

    return true;
}