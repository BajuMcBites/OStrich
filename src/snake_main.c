#include "snake_generic.h"

int snake_generic_main() {
    // Initialize the game
    SG_Init();

    // Main game loop
    while (SG_IsRunning()) {
        SG_ProcessFrame();
    }

    // Cleanup
    SG_Shutdown();

    return 0;
}