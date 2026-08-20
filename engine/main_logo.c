#include <SDL2/SDL.h>
#include "beforelight_input.h"
#include <SDL2/SDL_image.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h> // for getopt
#include "assets/omarchy_logo.h"

extern char *optarg;

#define PI 3.14159f

static void usage(const char *prog) {
    fprintf(stderr, "Usage: %s [options]\n", prog);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -s F    Speed multiplier (default: 1.0)\n");
    fprintf(stderr, "  -f 0|1  Fullscreen (1=yes, 0=windowed) (default: 1)\n");
    fprintf(stderr, "  -h      Show this help\n");
}

int main(int argc, char *argv[]) {
    int opt;
    float speed_mult = 1.0f;
    int do_fullscreen = 1;

    while ((opt = getopt(argc, argv, "s:f:h")) != -1) {
        switch (opt) {
            case 's':
                speed_mult = atof(optarg);
                if (speed_mult <= 0.1f) speed_mult = 0.1f;
                if (speed_mult > 10.0f) speed_mult = 10.0f;
                break;
            case 'f':
                do_fullscreen = atoi(optarg);
                break;
            case 'h':
            default:
                usage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    setenv("SDL_VIDEODRIVER", "wayland", 1); // Force Wayland for Hyprland
    srand(time(NULL));

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("SDL_Init Error: %s", SDL_GetError());
        return 1;
    }

    if (!(IMG_Init(IMG_INIT_PNG))) {
        SDL_Log("IMG_Init Error: %s", IMG_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Logo", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS | SDL_WINDOW_RESIZABLE);
    if (window == NULL) {
        SDL_Log("SDL_CreateWindow Error: %s", SDL_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL) {
        SDL_Log("SDL_CreateRenderer Error: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    if (do_fullscreen) {
        if (SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN) != 0) {
            SDL_Log("Warning: Failed to set fullscreen: %s", SDL_GetError());
        }
    }

    int W, H;
    SDL_GetRendererOutputSize(renderer, &W, &H);

    // Load logo texture
    SDL_RWops *rw = SDL_RWFromConstMem(omarchy_logo, omarchy_logo_len);
    if (!rw) {
        SDL_Log("Error creating RWops for embedded logo texture: %s", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    SDL_Surface *surf = IMG_Load_RW(rw, 1); // 1 to autoclose
    if (!surf) {
        SDL_Log("Error loading embedded logo texture: %s", IMG_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    int logo_w = surf->w, logo_h = surf->h;
    SDL_Texture *logo_tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_FreeSurface(surf);

    // Initialize bouncing physics for logo position
    float x = W / 2.0f, y = H / 2.0f, vx = 150.0f, vy = 100.0f;

    // Normalize loop time (50 second cycle as per CSS)
    const float cycle_time = 50.0f;

    // Hide cursor during screensaver
    system("hyprctl eval 'hl.config({ cursor = { invisible = true } })' >/dev/null 2>&1 || hyprctl keyword cursor:invisible true >/dev/null 2>&1");

    // Main loop
    SDL_Event e;
    int quit = 0;
    Uint32 start_time = SDL_GetTicks();
    BeforelightInput bl_input = {0};

    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT || e.type == SDL_KEYDOWN || e.type == SDL_MOUSEBUTTONDOWN) {
                SDL_Log("Screensaver quit triggered: event type %d", e.type);
                quit = 1;
            } else if (e.type == SDL_MOUSEMOTION) {
                // Only quit on mouse motion after 2 seconds to prevent immediate quit
                if (beforelight_should_quit_motion(&bl_input, start_time, &e)) {
                    SDL_Log("Screensaver quit triggered: mouse motion after grace period");
                    quit = 1;
                }
            }
        }

        Uint32 current_time = SDL_GetTicks();
        float time_s = (current_time - start_time) / 1000.0f;

        // Morphing cycles
        float cycle = fmodf(time_s, cycle_time) / cycle_time;  // 0 to 1 over 50s

        // Scale parameters (simulate CSS transform)
        float scaleX = 1.0f + 0.5f * sinf(2.0f * PI * cycle);  // Oscillate ±0.5x
        float scaleY = 1.0f + 0.3f * cosf(2.0f * PI * cycle * 1.5f); // Offset oscillation

        // Rotation
        float rotation = 360.0f * sinf(PI * cycle * 2.0f);  // Full rotations

        // Update bouncing physics for the scaled logo
        const float dt = 0.016f;
        x += vx * dt * speed_mult;
        y += vy * dt * speed_mult;

        int half_w = (int)(logo_w * scaleX) / 2;
        int half_h = (int)(logo_h * scaleY) / 2;

        if (x < half_w) { x = half_w; vx = -vx; }
        if (x > W - half_w) { x = W - half_w; vx = -vx; }
        if (y < half_h) { y = half_h; vy = -vy; }
        if (y > H - half_h) { y = H - half_h; vy = -vy; }

        // Compute render rect centered on bouncing position
        SDL_Point center = {logo_w / 2, logo_h / 2};
        SDL_Rect dst_rect = {
            (int)x - (int)(logo_w * scaleX) / 2,
            (int)y - (int)(logo_h * scaleY) / 2,
            (int)(logo_w * scaleX),
            (int)(logo_h * scaleY)
        };

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // black background
        SDL_RenderClear(renderer);

        SDL_RenderCopyEx(renderer, logo_tex, NULL, &dst_rect, rotation, &center, SDL_FLIP_NONE);

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60fps
    }

    // Cleanup - restore cursor visibility
    system("hyprctl eval 'hl.config({ cursor = { invisible = false } })' >/dev/null 2>&1 || hyprctl keyword cursor:invisible false >/dev/null 2>&1");

    // Cleanup
    SDL_DestroyTexture(logo_tex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
