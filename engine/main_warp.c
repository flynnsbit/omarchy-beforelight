#include <SDL2/SDL.h>
#include "beforelight_input.h"
#include <SDL2/SDL_image.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h> // for getopt
#include "assets/star1.h"
#include "assets/star2.h"
#include "assets/star3.h"
#include "assets/star4.h"

#define PI 3.14159f

extern char *optarg;

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
    srand((unsigned int)time(NULL));

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("SDL_Init Error: %s", SDL_GetError());
        return 1;
    }

    if (!(IMG_Init(IMG_INIT_PNG))) {
        SDL_Log("IMG_Init Error: %s", IMG_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Warp", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS | SDL_WINDOW_RESIZABLE);
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

    // Load star textures from embedded assets
    SDL_Texture *star_texs[4];
    for (int i = 0; i < 4; i++) {
        const unsigned char *data;
        unsigned int len;

        // Select the appropriate star texture data
        switch (i + 1) {
            case 1:
                data = star1;
                len = star1_len;
                break;
            case 2:
                data = star2;
                len = star2_len;
                break;
            case 3:
                data = star3;
                len = star3_len;
                break;
            case 4:
                data = star4;
                len = star4_len;
                break;
            default:
                data = star1;
                len = star1_len;
        }

        SDL_RWops *rw = SDL_RWFromConstMem(data, len);
        if (!rw) {
            SDL_Log("Error creating RWops for embedded star%d texture: %s", i + 1, SDL_GetError());
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            IMG_Quit();
            SDL_Quit();
            return 1;
        }
        SDL_Surface *surf = IMG_Load_RW(rw, 1); // 1 to autoclose RWops
        if (!surf) {
            SDL_Log("Error loading embedded star%d texture: %s", i + 1, IMG_GetError());
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            IMG_Quit();
            SDL_Quit();
            return 1;
        }
        star_texs[i] = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_FreeSurface(surf);
    }

    // Warp layer data: {tex_index, delay_seconds}
    int warp_layers[18][2] = {
        {0, 0},    // stars1 delay 0
        {1, 250},  // stars2 delay 0.25
        {2, 500},  // stars3 delay 0.5
        {3, 750},  // stars4 delay 0.75
        {0, 1000}, // stars1 delay 1
        {1, 1250}, // stars2 delay 1.25
        {2, 1500}, // stars3 delay 1.5
        {3, 1750}, // stars4 delay 1.75
        {0, 2000}, // stars1 delay 2
        {1, 2250}, // stars2 delay 2.25
        {2, 2500}, // stars3 delay 2.5
        {3, 2750}, // stars4 delay 2.75
        {0, 3000}, // stars1 delay 3
        {1, 3250}, // stars2 delay 3.25
        {2, 3500}, // stars3 delay 3.5
        {3, 3750}, // stars4 delay 3.75
        {0, 4000}  // stars1 delay 4
    };

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
        float time_ms = (current_time - start_time) / 1000.0f * speed_mult;

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // black background
        SDL_RenderClear(renderer);

        // Render warp starfields
        for (int i = 0; i < 18; i++) {
            int tex_idx = warp_layers[i][0];
            float delay_ms = warp_layers[i][1] / 1000.0f;
            float local_time = fmodf(time_ms - delay_ms, 2.0f);
            float frac = local_time / 2.0f;

            // Calculate scale and opacity based on CSS keyframes
            float scale;
            uint8_t opacity;

            if (frac < 0.5f) {
                // 0% to 50%: opacity 0 to 1, scale 0.5 to ~1.0 (ease-in)
                opacity = (uint8_t)(frac * 2.0f * 255);
                scale = 0.5f + frac * 2.0f * (1.0f - 0.5f);
            } else if (frac < 0.85f) {
                // 50% to 85%: opacity 1, scale 1.0 to 2.8 (linear)
                opacity = 255;
                scale = 1.0f + (frac - 0.5f) / (0.85f - 0.5f) * (2.8f - 1.0f);
            } else {
                // 85% to 100%: opacity 1 to 0, scale 2.8 to 3.5 (linear)
                opacity = (uint8_t)((1.0f - (frac - 0.85f) / 0.15f) * 255);
                scale = 2.8f + (frac - 0.85f) / 0.15f * (3.5f - 2.8f);
            }

            // Set texture alpha
            SDL_SetTextureAlphaMod(star_texs[tex_idx], opacity);

            // Render centered and scaled
            int dst_w = (int)(W * scale);
            int dst_h = (int)(H * scale);
            int dst_x = W / 2 - dst_w / 2;
            int dst_y = H / 2 - dst_h / 2;
            SDL_Rect dst_rect = {dst_x, dst_y, dst_w, dst_h};

            SDL_RenderCopy(renderer, star_texs[tex_idx], NULL, &dst_rect);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60fps
    }

    // Cleanup
    for (int i = 0; i < 4; i++) {
        SDL_DestroyTexture(star_texs[i]);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
