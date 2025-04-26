// doomgeneric_ostrich.c
// — your “port” layer for Ostrich OS —

#include <stdint.h>
#include <stddef.h>
#include "doomgeneric.h"

// -----------------------------------------------------------------------------
// 1) Platform hooks you *must* fill in

// Called once at startup.  You should:
//   • initialize your window / framebuffer
//   • set DG_ScreenBuffer, DG_ScreenWidth & DG_ScreenHeight
void DG_Init(void)
{
    // DG_ScreenBuffer = (pixel_t*)fb_get();
    // TODO: framebuffer_init(&DG_ScreenBuffer, &DG_ScreenWidth, &DG_ScreenHeight);
}

// Copy the 8-bit screen buffer into your display each frame.
void DG_DrawFrame(void)
{
    // TODO: framebuffer_blit(DG_ScreenBuffer, DG_ScreenWidth, DG_ScreenHeight);
}

// Sleep for approximately ms milliseconds.
void DG_SleepMs(uint32_t ms)
{
    // TODO: os_sleep(ms);
}

// Return “milliseconds since startup.”
uint32_t DG_GetTicksMs(void)
{
    // TODO: return os_monotonic_ms();
    return 0;
}

// Poll for one key event.
//   • *pressed = 1 for keydown, 0 for keyup
//   • *key     = scan code or ASCII
// Return 1 if an event was delivered, 0 otherwise.
int DG_GetKey(int *pressed, unsigned char *key)
{
    // TODO:
    // if (input_event_pending()) {
    //     *pressed = is_key_down();
    //     *key     = translate_key();
    //     return 1;
    // }
    return 0;
}

// Optional: Doom will call this to set the window title.
void DG_SetWindowTitle(const char *title)
{
    // TODO: os_set_window_title(title);
}

// -----------------------------------------------------------------------------
// 2) Main entrypoint — just like the other backends

int main(int argc, char **argv)
{
    doomgeneric_Create(argc, argv);

    while(1){
        doomgeneric_Tick();
    }

    return 0;
}
