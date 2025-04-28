#include "framebuffer.h"

#include "dcache.h"
#include "event.h"
#include "printf.h"
#include "utils.h"
// #include "core.h"

static Framebuffer real_fb __attribute__((aligned(16)));

static Framebuffer kernel_fb __attribute__((aligned(16)));

static SpinLock fb_lock;

Framebuffer* fb_get(void) {
    auto buffer = get_running_task(getCoreID())->frameBuffer;
    return buffer.operator->();
}

Framebuffer* get_kernel_fb(void) {
    return &kernel_fb;
}

Framebuffer* get_real_fb(void) {
    return &real_fb;
}

static constexpr int x_start = 0;
static constexpr int y_start = 0;

static int x = x_start;
static int y = y_start;

static bool fb_init_done = false;

int fb_init(void) {
    printf("Starting framebuffer initialization...\n");

    mbox[0] = 35 * 4;  // buffer size in bytes
    mbox[1] = MBOX_REQUEST;

    // set physical width/height (actual size of display)
    mbox[2] = MBOX_TAG_SETPHYSIZE;
    mbox[3] = 8;
    mbox[4] = 8;
    mbox[5] = 640;  // physical width
    mbox[6] = 480;  // physical height

    // set virtual width/height (where youcan draw)
    mbox[7] = MBOX_TAG_SETVIRSIZE;
    mbox[8] = 8;
    mbox[9] = 8;
    mbox[10] = 640;  // virtual width
    mbox[11] = 480;  // virtual height

    // set bit depth
    mbox[12] = MBOX_TAG_SETDEPTH;
    mbox[13] = 4;
    mbox[14] = 4;
    mbox[15] = 32;  // 32 bits per pixel

    // set pixel order
    mbox[16] = MBOX_TAG_SETPIXELORDER;
    mbox[17] = 4;
    mbox[18] = 4;
    mbox[19] = 1;  // RGB

    // set framebuffer
    mbox[20] = MBOX_TAG_GETFB;
    mbox[21] = 8;
    mbox[22] = 8;
    mbox[23] = 4096;  // framebuffer size
    mbox[24] = 0;

    // set pitch
    mbox[25] = MBOX_TAG_GETPITCH;
    mbox[26] = 4;
    mbox[27] = 4;
    mbox[28] = 0;  // pitch

    mbox[29] = MBOX_TAG_LAST;

    printf("Sending mailbox call...\n");
    if (mbox_call(MBOX_CH_PROP)) {
        printf("Mailbox call successful\n");
        if (mbox[23]) {
            printf("Got framebuffer address: %x\n", mbox[23]);
            mbox[23] &= 0x3FFFFFFF;
            real_fb.width = mbox[10];
            real_fb.height = mbox[11];
            real_fb.pitch = mbox[28];
            real_fb.isrgb = mbox[19];
            real_fb.buffer = (void*)((unsigned long)mbox[23]);
            real_fb.size = mbox[24];

            kernel_fb.width = mbox[10];
            kernel_fb.height = mbox[11];
            kernel_fb.pitch = mbox[28];
            kernel_fb.isrgb = mbox[19];
            kernel_fb.buffer = (void*)((unsigned long)mbox[23]);
            kernel_fb.size = mbox[24];

            fb_init_done = true;
            clean_dcache_line(&fb_init_done);
            printf("Framebuffer initialized:\n");
            printf("Width: %d, Height: %d\n", real_fb.width, real_fb.height);
            printf("Pitch: %d, Buffer size: %d\n", real_fb.pitch, real_fb.size);
            printf("Buffer address: %x\n", (unsigned long)real_fb.buffer);

            printf("Powering on HDMI...\n");
            mbox[0] = 35 * 4;
            mbox[1] = 0;
            mbox[2] = 0x48004;
            mbox[3] = 8;
            mbox[4] = 8;
            mbox[5] = 1;
            mbox[6] = 0;
            mbox[7] = 0;
            mailbox_write(MBOX_CH_PROP, gpu_convert_address((void*)mbox));
            mailbox_read(MBOX_CH_PROP);
            printf("HDMI power on complete\n");

            return 1;
        } else {
            printf("Failed to get framebuffer address\n");
        }
    } else {
        printf("Mailbox call failed\n");
    }
    printf("Failed to initialize framebuffer\n");
    return 0;
}

void draw_pixel(int x, int y, unsigned int color) {
    Framebuffer* fb = fb_get();
    if (x >= 0 && x < (int)fb->width && y >= 0 && y < (int)fb->height) {
        unsigned int* ptr = (unsigned int*)fb->buffer;
        ptr[y * (fb->pitch / 4) + x] = color;
    }
}

void draw_char(int x, int y, char c, unsigned int color) {
    unsigned int idx = 0;
    if (c >= 32 && c <= 127) {
        idx = c - 32;
    }

    for (int row = 0; row < 8; row++) {
        unsigned char line = font[idx][row];
        for (int col = 0; col < 8; col++) {
            if (line & (1 << (7 - col))) {
                draw_pixel(x + col, y + row, color);
            }
        }
    }
}

void fb_scroll(int scroll_pixels) {
    Framebuffer* fb = fb_get();
    int row_pixels = fb->pitch / 4;  // number of pixels per row (each pixel is 4 bytes)
    int rows_to_move = fb->height - scroll_pixels;
    unsigned int* buf = (unsigned int*)fb->buffer;

    // Copy each row upward: for each row, copy the row located scroll_pixels below.
    for (int row = 0; row < rows_to_move; row++) {
        for (int col = 0; col < row_pixels; col++) {
            buf[row * row_pixels + col] = buf[(row + scroll_pixels) * row_pixels + col];
        }
    }

    // Clear the bottom scroll_pixels rows (fill with BLACK)
    for (int row = rows_to_move; row < fb->height; row++) {
        for (int col = 0; col < row_pixels; col++) {
            buf[row * row_pixels + col] = BLACK;
        }
    }
}

void fb_print(const char* str, unsigned int color) {
    fb_lock.lock();
    if (!fb_init_done) {
        fb_lock.unlock();
        return;
    }
    Framebuffer* fb = fb_get();
    while (*str) {
        char c = *str++;
        if (c == '\n') {
            // when newline, move down by 10 pixels
            if (y + 10 >= fb->height) {
                // move down if out of room
                fb_scroll(10);
                y = fb->height - 10;
            } else {
                y += 10;
            }
            x = x_start;
        } else {
            draw_char(x, y, c, color);
            x += 8;
            if (x >= fb->width) {
                if (y + 10 >= fb->height) {
                    fb_scroll(10);
                    y = fb->height - 10;
                } else {
                    y += 10;
                }
                x = x_start;
            }
        }
    }
    fb_lock.unlock();
}

void fb_clear(unsigned int color) {
    Framebuffer* fb = fb_get();
    unsigned int* fb_ptr = (unsigned int*)fb->buffer;

    for (unsigned int y = 0; y < fb->height; y++) {
        for (unsigned int x = 0; x < fb->width; x++) {
            fb_ptr[y * (fb->pitch / 4) + x] = color;
        }
    }
}

void fb_blank(unsigned int color) {
    Framebuffer* fb = get_real_fb();
    unsigned int* fb_ptr = (unsigned int*)fb->buffer;

    for (unsigned int y = 0; y < fb->height; y++) {
        for (unsigned int x = 0; x < fb->width; x++) {
            fb_ptr[y * (fb->pitch / 4) + x] = color;
        }
    }
}

void fb_draw_image(int x, int y, const unsigned int* image_data, int width, int height) {
    Framebuffer* fb = fb_get();
    unsigned int* fb_ptr = (unsigned int*)fb->buffer;

    for (int img_y = 0; img_y < height; img_y++) {
        for (int img_x = 0; img_x < width; img_x++) {
            int fb_x = x + img_x;
            int fb_y = y + img_y;
            if (fb_x >= 0 && fb_x < (int)fb->width && fb_y >= 0 && fb_y < (int)fb->height) {
                unsigned int fb_pos = fb_y * (fb->pitch / 4) + fb_x;
                unsigned int img_pos = img_y * width + img_x;
                fb_ptr[fb_pos] = image_data[img_pos];
            }
        }
    }
}

void fb_draw_scaled_image(int x, int y, const unsigned int* image_data, int width, int height,
                          int scale) {
    Framebuffer* fb = fb_get();
    unsigned int* fb_ptr = (unsigned int*)fb->buffer;

    for (int img_y = 0; img_y < height; img_y++) {
        for (int img_x = 0; img_x < width; img_x++) {
            unsigned int pixel = image_data[img_y * width + img_x];
            for (int sy = 0; sy < scale; sy++) {
                for (int sx = 0; sx < scale; sx++) {
                    int fb_x = x + (img_x * scale) + sx;
                    int fb_y = y + (img_y * scale) + sy;
                    if (fb_x >= 0 && fb_x < (int)fb->width && fb_y >= 0 && fb_y < (int)fb->height) {
                        fb_ptr[fb_y * (fb->pitch / 4) + fb_x] = pixel;
                    }
                }
            }
        }
    }
}

// Define a simple 16x16 smiley face image
const unsigned int smiley_face[16 * 16] = {
    0x00000000, 0x00000000, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF,
    0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0x00000000, 0x00000000,
    0x00000000, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF,
    0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0x00000000,
    0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF,
    0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF,
    0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0x000000FF, 0x000000FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF,
    0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0x000000FF, 0x000000FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF,
    0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0x000000FF, 0x000000FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF,
    0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF,
    0x000000FF, 0x000000FF, 0x000000FF, 0x000000FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF,
    0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF,
    0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF,
    0x00000000, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF,
    0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0x00000000,
    0x00000000, 0x00000000, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF,
    0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0xFFFF00FF, 0x00000000, 0x00000000};

// =======================
// Multicore Animation Code
// =======================

#define MAX_CORES 4

struct AnimationState {
    int x;
    int y;
    int dx;
    int dy;
    int size;
    const unsigned int* image;
    int width;
    int height;
    unsigned int color;
} animation[MAX_CORES];

unsigned int tint_color(unsigned int original, unsigned int tint) {
    if ((original & 0xFF000000) == 0) return 0;

    unsigned char r = (tint >> 16) & 0xFF;
    unsigned char g = (tint >> 8) & 0xFF;
    unsigned char b = tint & 0xFF;

    return (original & 0xFF000000) | (r << 16) | (g << 8) | b;
}

void init_animation(void) {
    printf("Setting white background...\n");
    unsigned int* ptr = (unsigned int*)real_fb.buffer;
    for (unsigned int y = 0; y < real_fb.height; y++) {
        for (unsigned int x = 0; x < real_fb.width; x++) {
            ptr[y * (real_fb.pitch / 4) + x] = 0xFFFFFFFF;
        }
    }
    int core_id = getCoreID();
    if (core_id >= MAX_CORES) return;

    // define starting positions and tint colors for each core
    static const struct {
        int x, y;
        unsigned int color;
    } core_configs[MAX_CORES] = {
        {100, 100, 0xFFFF0000},  // Core 0: Red
        {300, 100, 0xFF00FF00},  // Core 1: Green
        {100, 300, 0xFF0000FF},  // Core 2: Blue
        {300, 300, 0xFFFF00FF}   // Core 3: Purple
    };

    animation[core_id].x = core_configs[core_id].x;
    animation[core_id].y = core_configs[core_id].y;
    animation[core_id].dx = 5;
    animation[core_id].dy = 5;
    animation[core_id].size = 4;  // Scale factor
    animation[core_id].image = smiley_face;
    animation[core_id].width = 16;
    animation[core_id].height = 16;
    animation[core_id].color = core_configs[core_id].color;
}

void update_animation(void) {
    int core_id = getCoreID();
    if (core_id >= MAX_CORES) return;

    Framebuffer* fb = fb_get();
    struct AnimationState* anim = &animation[core_id];

    // Update position based on speed
    anim->x += anim->dx;
    anim->y += anim->dy;

    // Bounce off the screen edges
    if (anim->x <= 0 || anim->x >= (int)fb->width - anim->width * anim->size) {
        anim->dx = -anim->dx;
    }
    if (anim->y <= 0 || anim->y >= (int)fb->height - anim->height * anim->size) {
        anim->dy = -anim->dy;
    }

    // Draw the image with core-specific tinting at the new position
    for (int img_y = 0; img_y < anim->height; img_y++) {
        for (int img_x = 0; img_x < anim->width; img_x++) {
            unsigned int pixel = anim->image[img_y * anim->width + img_x];
            pixel = tint_color(pixel, anim->color);

            for (int sy = 0; sy < anim->size; sy++) {
                for (int sx = 0; sx < anim->size; sx++) {
                    int fb_x = anim->x + (img_x * anim->size) + sx;
                    int fb_y = anim->y + (img_y * anim->size) + sy;

                    if (fb_x >= 0 && fb_x < (int)fb->width && fb_y >= 0 && fb_y < (int)fb->height) {
                        unsigned int* fb_ptr = (unsigned int*)fb->buffer;
                        fb_ptr[fb_y * (fb->pitch / 4) + fb_x] = pixel;
                    }
                }
            }
        }
    }

    for (volatile int i = 0; i < 1000000; i++) {
        asm volatile("nop");
    }
    create_event(update_animation);
}

void clear_screen(void) {
    Framebuffer* fb = fb_get();
    unsigned int* fb_ptr = (unsigned int*)fb->buffer;
    for (unsigned int y = 0; y < fb->height; y++) {
        for (unsigned int x = 0; x < fb->width; x++) {
            fb_ptr[y * (fb->pitch / 4) + x] = BLACK;
        }
    }
}