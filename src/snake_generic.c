#include "snake_generic.h"

#include "../peripherals/usb_hid_keys.h"
#include "libk.h"

using namespace K;

// Game state
SnakeGameState sg_game_state;
bool sg_is_running = false;
bool sg_is_connected = false;

// Private variables
static uint32_t last_heartbeat_time = 0;
static uint32_t last_frame_time = 0;
static const uint32_t HEARTBEAT_INTERVAL_MS = 2000;
static const uint32_t FRAME_INTERVAL_MS = 100;

// Core game functions
void SG_Init(void) {
    // Initialize game state
    memset(&sg_game_state, 0, sizeof(sg_game_state));
    sg_is_running = true;
    sg_is_connected = false;

    // Initialize platform
    if (!SG_PlatformInit()) {
        SG_PlatformPrintMessage("Platform initialization failed");
        sg_is_running = false;
        return;
    }

    // Initialize network
    if (!SG_PlatformNetInit("127.0.0.1", 25566)) {
        SG_PlatformPrintMessage("Network initialization failed");
        sg_is_running = false;
        return;
    }

    // Connect to server
    if (!SG_PlatformNetConnect()) {
        SG_PlatformPrintMessage("Failed to connect to server");
        sg_is_running = false;
        return;
    }

    // Join game
    if (!SG_PlatformNetSendJoin()) {
        SG_PlatformPrintMessage("Failed to join game");
        sg_is_running = false;
        return;
    }

    sg_is_connected = true;
    last_heartbeat_time = SG_PlatformGetTicksMs();
    last_frame_time = last_heartbeat_time;
}

void SG_ProcessFrame(void) {
    if (!sg_is_running) {
        return;
    }

    uint32_t current_time = SG_PlatformGetTicksMs();

    // Check for incoming network data
    static uint8_t net_buffer[4096];
    int received = SG_PlatformNetReceive(net_buffer, sizeof(net_buffer));
    if (received > 0) {
        // Parse game state update
        SG_PlatformNetParseGameState(net_buffer, received, &sg_game_state);
    }

    // Send heartbeat if needed
    if (current_time - last_heartbeat_time >= HEARTBEAT_INTERVAL_MS) {
        SG_PlatformNetSendHeartbeat();
        last_heartbeat_time = current_time;
    }

    // Get input
    int key = SG_PlatformGetKey();
    if (key != 0) {
        SG_HandleInput(key);
    }

    // Update display at fixed intervals
    if (current_time - last_frame_time >= FRAME_INTERVAL_MS) {
        SG_PlatformDrawFrame(&sg_game_state);
        last_frame_time = current_time;
    } else {
        SG_PlatformSleepMs(1);  // Yield CPU
    }
}

// not actually generic.
void SG_HandleInput(int key_code) {
    // Map key code to direction and send input
    uint8_t direction = SNAKE_DIR_UP;
    bool handled = true;

    // These values should be defined by the platform
    // Using generic values for demonstration
    switch (key_code) {
        case KEY_W:
            direction = SNAKE_DIR_UP;
            break;
        case KEY_D:
            direction = SNAKE_DIR_RIGHT;
            break;
        case KEY_S:
            direction = SNAKE_DIR_DOWN;
            break;
        case KEY_A:
            direction = SNAKE_DIR_LEFT;
            break;
        case KEY_ESC:
            sg_is_running = false;
            return;
        default:
            handled = false;
            break;
    }

    if (handled) {
        SG_PlatformNetSendInput(direction);
    }
}

void SG_Shutdown(void) {
    SG_PlatformNetShutdown();
    SG_PlatformShutdown();
    sg_is_running = false;
}

bool SG_IsRunning(void) {
    return sg_is_running;
}