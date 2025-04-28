#ifndef SNAKE_GENERIC_H
#define SNAKE_GENERIC_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Game constants
#define SNAKE_MAX_PLAYERS 16
#define SNAKE_MAX_FOOD 20
#define SNAKE_MAX_BODY_LENGTH 100

// Direction constants
#define SNAKE_DIR_UP 0
#define SNAKE_DIR_RIGHT 1
#define SNAKE_DIR_DOWN 2
#define SNAKE_DIR_LEFT 3

// Message types
#define SNAKE_MSG_JOIN 1
#define SNAKE_MSG_JOIN_ACK 2
#define SNAKE_MSG_INPUT 3
#define SNAKE_MSG_STATE_UPDATE 4
#define SNAKE_MSG_HEARTBEAT 5

// Game structures
typedef struct {
    uint16_t x;
    uint16_t y;
} SnakePosition;

typedef struct {
    SnakePosition body[SNAKE_MAX_BODY_LENGTH];
    uint8_t length;
    uint8_t direction;
    char player_id[37];  // UUID string
} SnakePlayer;

struct SnakeGameStateData {
    uint8_t msg_type;
    uint32_t timestamp;
    uint8_t player_count;
    uint8_t food_count;
    SnakePosition food[5];   // MAX_FOOD
    SnakePlayer players[4];  // MAX_PLAYERS. I am always player 0.
};

// Game state (global)
extern SnakeGameStateData sg_game_state;
extern bool sg_is_running;
extern bool sg_is_connected;

// Core game functions
void SG_Init(void);
void SG_ProcessFrame(void);
void SG_Shutdown(void);
void SG_HandleInput(int key_code);
bool SG_IsRunning(void);

// Platform stubs - must be implemented by the platform
// Graphics
bool SG_PlatformInit(void);
void SG_PlatformShutdown(void);
void SG_PlatformDrawFrame(SnakeGameStateData* state);
void SG_PlatformPrintMessage(const char* message);

// Input
int SG_PlatformGetKey(void);

// Timing
uint32_t SG_PlatformGetTicksMs(void);
void SG_PlatformSleepMs(uint32_t ms);

// Networking
bool SG_PlatformNetInit(uint32_t server_host, int server_port);
void SG_PlatformNetShutdown(void);
int SG_PlatformNetSend(const void* data, int length);
int SG_PlatformNetReceive(void* buffer);
bool SG_PlatformNetConnect(void);
bool SG_PlatformNetSendJoin(void);
bool SG_PlatformNetSendInput(uint8_t direction);
bool SG_PlatformNetSendHeartbeat(void);
bool SG_PlatformNetParseGameState(const uint8_t* data, int length, SnakeGameStateData* state);

int snake_generic_main();

#ifdef __cplusplus
}
#endif

#endif  // SNAKE_GENERIC_H