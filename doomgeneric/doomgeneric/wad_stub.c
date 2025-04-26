/* wad_stub.c — embed doom.wad via xxd into doom_wad.h */

#include "w_file.h"    // gives you wad_file_t plus the real W_OpenFile, W_Read, W_CloseFile prototypes
#include "wad_data.h"  // your xxd output: extern const unsigned char doom_wad[]; extern const unsigned int doom_wad_len;
#include <string.h>    // strcmp, memcpy
#include <stdlib.h>    // malloc, free
#include <sys/stat.h>

int M_FileExists(const char *path)
{
    if (strcmp(path, "doom.wad")==0)
        return 1;          // pretend our embedded wad is on disk
    // otherwise fall back to the real filesystem check:
    struct stat st;
    return (stat(path, &st)==0);
}

// Override the engine’s file‐open.  If someone asks for “doom.wad”,
// hand them back our in‐memory mapping; otherwise fall back to NULL
// (so the normal filesystem search will pick up e.g. freedoom1.wad, etc).
wad_file_t *W_OpenFile(char *path)
{
    if (strcmp(path, "doom.wad") == 0) {
        wad_file_t *f = malloc(sizeof *f);
        if (!f) return NULL;
        f->mapped = doom_wad;
        f->length = doom_wad_len;
        return f;
    }
    return NULL;
}

// Override the engine’s read.  Always succeed (up to the embedded size).
size_t W_Read(wad_file_t *f, unsigned int offset, void *buf, size_t len)
{
    if (offset + len > f->length) {
        if (offset >= f->length) return 0;
        len = f->length - offset;
    }
    memcpy(buf, f->mapped + offset, len);
    return len;
}

// Override the engine’s close.  Just free our struct.
void W_CloseFile(wad_file_t *f)
{
    free(f);
}
