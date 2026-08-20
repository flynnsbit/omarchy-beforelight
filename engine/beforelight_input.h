#pragma once

#include <SDL2/SDL.h>
#include <stdlib.h>

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
