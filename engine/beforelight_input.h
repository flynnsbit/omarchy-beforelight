#pragma once

#include <SDL2/SDL.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct {
    int armed;
    int origin_x;
    int origin_y;
} BeforelightInput;

static inline int beforelight_env_int(const char *name, int fallback) {
    const char *value = getenv(name);
    if (!value || !value[0])
        return fallback;
    return atoi(value);
}

/* Dismiss on real movement, not a one-pixel twitch or the click that started preview. */
static inline int beforelight_should_quit_motion(BeforelightInput *state, Uint32 start_time, const SDL_Event *event) {
    Uint32 grace_ms = (Uint32)beforelight_env_int("BEFORELIGHT_GRACE_MS", 500);
    int threshold = beforelight_env_int("BEFORELIGHT_MOVE_PX", 28);
    if (threshold < 8)
        threshold = 8;
    if ((SDL_GetTicks() - start_time) < grace_ms)
        return 0;
    if (!state->armed) {
        state->origin_x = event->motion.x;
        state->origin_y = event->motion.y;
        state->armed = 1;
        return 0;
    }
    int dx = event->motion.x - state->origin_x;
    int dy = event->motion.y - state->origin_y;
    return (dx * dx + dy * dy) >= (threshold * threshold);
}

/* Same hide/restore for every saver. Hyprland cursor:invisible is required on
 * the overlay path (SDL_ShowCursor is not enough). Restore on atexit and
 * SIGTERM so a stopped preview cannot leave the pointer gone. */
static int beforelight_cursor_held;

static inline void beforelight_release_cursor(void) {
    if (!beforelight_cursor_held)
        return;
    beforelight_cursor_held = 0;
    SDL_ShowCursor(SDL_ENABLE);
    system("hyprctl eval 'hl.config({ cursor = { invisible = false } })' >/dev/null 2>&1");
}

static inline void beforelight_cursor_signal(int sig) {
    (void)sig;
    beforelight_release_cursor();
    _exit(0);
}

static inline void beforelight_grab_cursor(void) {
    if (beforelight_cursor_held)
        return;
    beforelight_cursor_held = 1;
    SDL_ShowCursor(SDL_DISABLE);
    system("hyprctl eval 'hl.config({ cursor = { invisible = true } })' >/dev/null 2>&1");
    atexit(beforelight_release_cursor);
    signal(SIGTERM, beforelight_cursor_signal);
    signal(SIGINT, beforelight_cursor_signal);
    signal(SIGHUP, beforelight_cursor_signal);
}
