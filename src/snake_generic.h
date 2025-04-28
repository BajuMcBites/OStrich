/*
 * snake_generic.h - Main interface for the Snake game
 * Platform-independent header
 */

#ifndef SNAKE_GENERIC_H
#define SNAKE_GENERIC_H

#include <stdbool.h>
#include <stdint.h>

/*
 * Game Constants
 */
#define SG_GRID_WIDTH 40
#define SG_GRID_HEIGHT 20
#define SG_MAX_PLAYERS 8
#define SG_MAX_FOOD 10
#define SG_MAX_BODY_LENGTH 100
#define SG_INITIAL_BODY_LENGTH 3
#define SG_UUID_LENGTH 37 /* 36 chars + null terminator */

/*
 * Game Enumerations
 */
typedef enum { SG_DIR_NONE = 0, SG_DIR_UP, SG_DIR_RIGHT, SG_DIR_DOWN, SG_DIR_LEFT } SG_Direction;

typedef enum {
    SG_MSG_JOIN = 1,
    SG_MSG_LEAVE,
    SG_MSG_UPDATE,
    SG_MSG_STATE,
    SG_MSG_RESET
} SG_MessageType;

/*
 * Game Structures
 */
typedef struct {
    uint8_t x;
    uint8_t y;
} SG_Position;

typedef struct {
    bool active;
    uint8_t id;
    char uuid[SG_UUID_LENGTH];
    char name[16];
    SG_Direction direction;
    SG_Direction next_direction;
    uint16_t score;
    uint16_t body_length;
    SG_Position body[SG_MAX_BODY_LENGTH];
} SG_Player;

typedef struct {
    bool running;
    uint32_t frame_count;
    uint32_t timestamp;

    /* Game state */
    uint8_t player_count;
    uint8_t local_player_id;
    SG_Player players[SG_MAX_PLAYERS];
    uint8_t food_count;
    SG_Position food[SG_MAX_FOOD];

    /* Networking */
    char server_address[64];
    uint16_t server_port;
    char player_uuid[SG_UUID_LENGTH];
    char status_message[256];
} SG_GameState;

/*
 * Core Game Functions
 */
void SG_Init(void);
void SG_ProcessFrame(void);
void SG_Shutdown(void);

/*
 * Game Utility Functions
 */
void SG_UpdateGameState(void);
void SG_UpdatePlayerPosition(SG_Player* player);
bool SG_CheckCollision(const SG_Player* player);
void SG_SpawnFood(void);
bool SG_HandleInput(SG_Direction direction);
void SG_ResetPlayer(SG_Player* player, uint8_t id);

/*
 * Platform Stubs - Graphics
 */
void SG_PlatformInit(void);
void SG_PlatformShutdown(void);
void SG_PlatformDrawFrame(void);
void SG_PlatformDrawStatus(const char* message);
bool SG_PlatformShouldQuit(void);

/*
 * Platform Stubs - Input
 */
bool SG_PlatformGetInput(SG_Direction* direction);

/*
 * Platform Stubs - Timing
 */
uint32_t SG_PlatformGetTicks(void);
void SG_PlatformSleep(uint32_t ms);

/*
 * Platform Stubs - Networking
 */
bool SG_PlatformNetInit(const char* server_address, uint16_t server_port);
void SG_PlatformNetShutdown(void);
bool SG_PlatformNetJoinGame(void);
bool SG_PlatformNetSendInput(SG_Direction direction);
bool SG_PlatformNetSendMessage(SG_MessageType type, const void* data, size_t data_size);
bool SG_PlatformNetReceiveMessage(void* buffer, size_t buffer_size, size_t* bytes_received);
void SG_PlatformNetGenerateUUID(char* uuid_buffer, size_t buffer_size);

/* Global game state */
extern SG_GameState sg_game_state;

#endif /* SNAKE_GENERIC_H */