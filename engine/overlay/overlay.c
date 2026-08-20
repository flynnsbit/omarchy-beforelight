/* LD_PRELOAD shim: intercept SDL2 SDL_CreateWindow (what the savers call)
 * and create an SDL3 window with a custom Wayland role on Overlay. */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <wayland-client.h>

#include "wlr-layer-shell-unstable-v1-client-protocol.h"

/* SDL3's CreateWindow/SetWindowFullscreen differ from the SDL2 symbols savers import. */
#define SDL_CreateWindow SDL_CreateWindow_SDL3
#define SDL_SetWindowFullscreen SDL_SetWindowFullscreen_SDL3
#include <SDL3/SDL.h>
#undef SDL_CreateWindow
#undef SDL_SetWindowFullscreen

#define NS "beforelight"
#define ANCHOR_ALL (ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM | \
                    ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT)

static int g_debug;
static int g_hooked;
static struct zwlr_layer_shell_v1 *g_shell;
static struct wl_surface *g_surface;
static SDL_Window *g_window;
static int g_configured;
static uint32_t g_cfg_w;
static uint32_t g_cfg_h;

static void dbg(const char *msg) {
    if (g_debug)
        fprintf(stderr, "beforelight-overlay: %s\n", msg);
}

static void layer_configure(void *data, struct zwlr_layer_surface_v1 *layer,
                            uint32_t serial, uint32_t width, uint32_t height) {
    (void)data;
    g_configured = 1;
    if (width > 0)
        g_cfg_w = width;
    if (height > 0)
        g_cfg_h = height;
    zwlr_layer_surface_v1_ack_configure(layer, serial);
    if (g_debug)
        fprintf(stderr, "beforelight-overlay: configure %ux%u\n", width, height);
}

static void layer_closed(void *data, struct zwlr_layer_surface_v1 *layer) {
    (void)data;
    (void)layer;
}

static const struct zwlr_layer_surface_v1_listener layer_listener = {
    .configure = layer_configure,
    .closed = layer_closed,
};

static void registry_global(void *data, struct wl_registry *registry, uint32_t name,
                            const char *iface, uint32_t version) {
    (void)data;
    if (strcmp(iface, zwlr_layer_shell_v1_interface.name) == 0) {
        uint32_t v = version < 4 ? version : 4;
        g_shell = wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, v);
    }
}

static void registry_remove(void *data, struct wl_registry *registry, uint32_t name) {
    (void)data;
    (void)registry;
    (void)name;
}

static const struct wl_registry_listener registry_listener = {
    .global = registry_global,
    .global_remove = registry_remove,
};

static int attach_layer(SDL_Window *window) {
    SDL_PropertiesID props = SDL_GetWindowProperties(window);
    struct wl_display *display = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, NULL);
    struct wl_surface *surface = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, NULL);
    if (!display || !surface) {
        dbg("no wayland display/surface");
        return -1;
    }

    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, NULL);
    wl_display_roundtrip(display);
    if (!g_shell) {
        dbg("zwlr_layer_shell_v1 missing");
        wl_registry_destroy(registry);
        return -1;
    }

    struct zwlr_layer_surface_v1 *layer = zwlr_layer_shell_v1_get_layer_surface(
        g_shell, surface, NULL, ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY, NS);
    if (!layer) {
        dbg("get_layer_surface failed");
        return -1;
    }

    zwlr_layer_surface_v1_add_listener(layer, &layer_listener, NULL);
    zwlr_layer_surface_v1_set_anchor(layer, ANCHOR_ALL);
    zwlr_layer_surface_v1_set_exclusive_zone(layer, -1);
    zwlr_layer_surface_v1_set_keyboard_interactivity(
        layer, ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_EXCLUSIVE);
    zwlr_layer_surface_v1_set_size(layer, 0, 0);
    g_surface = surface;
    g_window = window;
    g_configured = 0;
    wl_surface_commit(surface);

    for (int i = 0; i < 80 && !g_configured; i++) {
        if (wl_display_dispatch(display) < 0)
            break;
    }
    if (!g_configured)
        dbg("timed out waiting for configure");
    else if (g_window && g_cfg_w > 0 && g_cfg_h > 0) {
        SDL_SetWindowSize(g_window, (int)g_cfg_w, (int)g_cfg_h);
        if (g_debug) {
            int lw = 0, lh = 0, pw = 0, ph = 0;
            SDL_GetWindowSize(g_window, &lw, &lh);
            SDL_GetWindowSizeInPixels(g_window, &pw, &ph);
            fprintf(stderr, "beforelight-overlay: window %dx%d pixels %dx%d cfg %ux%u\n",
                    lw, lh, pw, ph, g_cfg_w, g_cfg_h);
        }
    }
    return 0;
}

/* SDL2 ABI. The saver binary imports this from libSDL2; LD_PRELOAD wins. */
SDL_Window *SDL_CreateWindow(const char *title, int x, int y, int w, int h, unsigned flags) {
    (void)x;
    (void)y;
    (void)flags;
    g_debug = getenv("BEFORELIGHT_DEBUG") != NULL;
    dbg("SDL_CreateWindow hook");

    const char *off = getenv("BEFORELIGHT_OVERLAY");
    if ((off && strcmp(off, "0") == 0) || g_hooked) {
        /* Should not happen for a one-window saver; still create overlay-less. */
    }
    g_hooked = 1;

    SDL_PropertiesID props = SDL_CreateProperties();
    if (title)
        SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, title);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, w > 0 ? w : 800);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, h > 0 ? h : 600);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_WAYLAND_SURFACE_ROLE_CUSTOM_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_HIDDEN_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_FULLSCREEN_BOOLEAN, false);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_BORDERLESS_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_HIGH_PIXEL_DENSITY_BOOLEAN, true);

    SDL_Window *window = SDL_CreateWindowWithProperties(props);
    SDL_DestroyProperties(props);
    if (!window) {
        dbg(SDL_GetError());
        return NULL;
    }
    if (attach_layer(window) != 0)
        dbg("overlay attach failed");
    SDL_ShowWindow(window);
    return window;
}

/* SDL2 ABI: savers call this after create. Custom-role surfaces have no xdg-toplevel. */
int SDL_SetWindowFullscreen(SDL_Window *window, unsigned flags) {
    (void)window;
    (void)flags;
    dbg("SetWindowFullscreen ignored (layer overlay)");
    return 0;
}

__attribute__((constructor))
static void init(void) {
    g_debug = getenv("BEFORELIGHT_DEBUG") != NULL;
    dbg("loaded");
}
