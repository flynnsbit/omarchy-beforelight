#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* User-owned real directory only. Rejects a symlink or foreign owner. */
static int beforelight_ensure_cache_dir(char *dir, size_t n) {
    const char *home = getenv("HOME");
    const char *cache = getenv("XDG_CACHE_HOME");
    if (cache && cache[0])
        snprintf(dir, n, "%s/beforelight", cache);
    else
        snprintf(dir, n, "%s/.cache/beforelight", home && home[0] ? home : ".");

    struct stat st;
    if (lstat(dir, &st) == 0) {
        if (S_ISLNK(st.st_mode) || !S_ISDIR(st.st_mode) || st.st_uid != getuid())
            return -1;
        if ((st.st_mode & 0777) != 0700 && chmod(dir, 0700) != 0)
            return -1;
        return 0;
    }
    if (mkdir(dir, 0700) != 0)
        return -1;
    if (lstat(dir, &st) != 0 || S_ISLNK(st.st_mode) || !S_ISDIR(st.st_mode) || st.st_uid != getuid())
        return -1;
    return 0;
}

/* Native-resolution screenshot via grim stdout. No named file, so no symlink follow. */
static SDL_Surface *beforelight_capture_screen(void) {
    char dir[512];
    if (beforelight_ensure_cache_dir(dir, sizeof(dir)) != 0)
        SDL_Log("Ignoring untrusted cache dir");

    FILE *fp = popen("grim -t png - 2>/dev/null", "r");
    if (!fp)
        return NULL;

    size_t cap = 256 * 1024, len = 0;
    unsigned char *buf = (unsigned char *)malloc(cap);
    if (!buf) {
        pclose(fp);
        return NULL;
    }
    for (;;) {
        if (len + 65536 > cap) {
            size_t ncap = cap * 2;
            unsigned char *nbuf = (unsigned char *)realloc(buf, ncap);
            if (!nbuf) {
                free(buf);
                pclose(fp);
                return NULL;
            }
            buf = nbuf;
            cap = ncap;
        }
        size_t n = fread(buf + len, 1, cap - len, fp);
        len += n;
        if (n == 0)
            break;
    }
    int st = pclose(fp);
    if (st != 0 || len < 24) {
        free(buf);
        return NULL;
    }

    SDL_RWops *rw = SDL_RWFromConstMem(buf, (int)len);
    SDL_Surface *surf = rw ? IMG_Load_RW(rw, 1) : NULL;
    free(buf);
    return surf;
}
