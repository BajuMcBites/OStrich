#include "keyboard.h"
#include "listener.h"
#include "printf.h"
#include "snake_generic.h"

// Core game functions
void SG_Init(void);
void SG_ProcessFrame(void);
void SG_Shutdown(void);
void SG_HandleInput(int key_code);
bool SG_IsRunning(void);

void key_listener(key_event* event) {
    SG_HandleInput(event->keycode);
}

// Platform stubs - must be implemented by the platform
// Graphics
bool SG_PlatformInit(void) {
    get_event_handler()->register_listener(KEYBOARD_EVENT,
                                           new Listener<key_event*>(std::move(key_listener)));
    return true;
}

void SG_PlatformShutdown(void) {
    return;
}

void SG_PlatformDrawFrame(SnakeGameState* state) {
    return;
}

void SG_PlatformPrintMessage(const char* message) {
    printf("%s\n", message);
}

// Input
// int SG_PlatformGetKey(void) {
//     return get_key_event();
// }

// Timing
uint32_t SG_PlatformGetTicksMs(void) {
    return get_ticks_ms();
}

void SG_PlatformSleepMs(uint32_t ms) {
    sleep_ms(ms);
}

// Networking
bool SG_PlatformNetInit(const char* server_host, int server_port);
void SG_PlatformNetShutdown(void);
int SG_PlatformNetSend(const void* data, int length);
int SG_PlatformNetReceive(void* buffer, int buffer_size);
bool SG_PlatformNetConnect(void);
bool SG_PlatformNetSendJoin(void);
bool SG_PlatformNetSendInput(uint8_t direction);
bool SG_PlatformNetSendHeartbeat(void);
bool SG_PlatformNetParseGameState(const uint8_t* data, int length, SnakeGameState* state);