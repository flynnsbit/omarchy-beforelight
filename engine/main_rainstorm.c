#include <SDL2/SDL.h>
#include "beforelight_input.h"
#include <time.h>
#include <stdlib.h>
#include <unistd.h> // for getopt
#include <math.h>

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

    srand(time(NULL));

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("SDL_Init Error: %s", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Rainstorm", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS | SDL_WINDOW_RESIZABLE);
    if (window == NULL) {
        SDL_Log("SDL_CreateWindow Error: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL) {
        SDL_Log("SDL_CreateRenderer Error: %s", SDL_GetError());
        SDL_DestroyWindow(window);
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

    // Rain droplet data - single layer, full screen
    #define MAX_DROPS 150
    struct drop {
        float x, y;
    };
    struct drop drops[MAX_DROPS];

    // Initialize drops with extended horizontal coverage for angled rain
    for (int i = 0; i < MAX_DROPS; i++) {
        drops[i].x = (rand() % (W + 220)) - 110; // 10% additional coverage beyond borders
        drops[i].y = (float)(rand() % H);  // Start at random Y for distribution
    }

    // Flash timing
    float next_flash_time = 4.0f + (rand() % 4); // 4-7 seconds
    float flash_duration = 0.15f;
    float last_flash_time = -10.0f; // far past
    float current_flash_remaining = 0.0f;

    // Main loop
    SDL_Event e;
    int quit = 0;
    Uint32 start_time = SDL_GetTicks();
    BeforelightInput bl_input = {0};

    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT || e.type == SDL_KEYDOWN || e.type == SDL_MOUSEBUTTONDOWN) {
                quit = 1;
            } else if (e.type == SDL_MOUSEMOTION) {
                // Only quit on mouse motion after 2 seconds to prevent immediate quit
                if (beforelight_should_quit_motion(&bl_input, start_time, &e)) {
                    quit = 1;
                }
            }
        }

        Uint32 current_time = SDL_GetTicks();
        float time_s = (current_time - start_time) / 1000.0f;

        // Check for flash trigger
        if (current_flash_remaining <= 0.0f && time_s - last_flash_time >= next_flash_time) {
            current_flash_remaining = flash_duration;
            last_flash_time = time_s;
            next_flash_time = 4.0f + (rand() % 5); // Next flash 4-8 seconds from now
        }

        // Update drops - continuous fall with 15 deg slant and variable speed
        float base_speed = 16.0f;
        float max_speed = base_speed * 1.5f;
        float varying_speed = base_speed + (max_speed - base_speed) * 0.5f * (1 + sinf(time_s * 0.5f)); // Oscillate speed
        varying_speed *= speed_mult;

        for (int i = 0; i < MAX_DROPS; i++) {
            drops[i].y += varying_speed;  // Variable fall speed
            drops[i].x += (int)(varying_speed * 0.268f);  // tan(15°) ≈ 0.268 for slant proportional to speed

            // Respawn at top when off bottom with extended coverage
            if (drops[i].y > H + 20) {
                drops[i].x = (rand() % (W + 220)) - 110; // 10% additional coverage
                drops[i].y = -10;
            }
        }

        // Update flash
        if (current_flash_remaining > 0.0f) {
            current_flash_remaining -= 0.016f; // Assuming 60fps
        }

        // Render
        if (current_flash_remaining > 0.0f) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White flash background
        } else {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // black background
        }
        SDL_RenderClear(renderer);

        // Draw angled rain drops (white lines from top to bottom)
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 180);
        for (int i = 0; i < MAX_DROPS; i++) {
            int length = 15;
            float tan15 = 0.268f; // tan(15°)
            int dx = (int)(length * tan15);
            SDL_RenderDrawLine(renderer, (int)drops[i].x, (int)drops[i].y,
                              (int)drops[i].x + dx, (int)drops[i].y + length);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60fps
    }

    // Cleanup
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
