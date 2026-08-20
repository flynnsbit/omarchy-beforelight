#include <SDL2/SDL.h>
#include "beforelight_input.h"
#include <SDL2/SDL_image.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h> // for getopt

#define PI 3.14159f

#define WINDOW_WIDTH 0  // fullscreen
#define WINDOW_HEIGHT 0
#define SPRITE_SIZE 145
#define FISH_FRAME_COUNT 4  // not used

extern char *optarg;

void drawCircleOutline(SDL_Renderer *renderer, int centerX, int centerY, int radius, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    int inner_radius = radius - 1; // 1 pixel thick ring
    for (int y = centerY - radius; y <= centerY + radius; y++) {
        int dy = y - centerY;
        int dy_squared = dy * dy;
        int dx_outer_squared = radius * radius - dy_squared;
        if (dx_outer_squared > 0) {
            int dx_outer = (int)sqrtf(dx_outer_squared);
            int dx_inner = (inner_radius > 0 && dy_squared < inner_radius * inner_radius) ? (int)sqrtf(inner_radius * inner_radius - dy_squared) : 0;
            SDL_RenderDrawLine(renderer, centerX - dx_outer, y, centerX - dx_inner, y);
            SDL_RenderDrawLine(renderer, centerX + dx_inner, y, centerX + dx_outer, y);
        }
    }
}

static void usage(const char *prog) {
    fprintf(stderr, "Usage: %s [options]\n", prog);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -t N    Number of drops (default: 120)\n");
    fprintf(stderr, "  -s F    Speed multiplier (default: 1.0)\n");
    fprintf(stderr, "  -f 0|1  Fullscreen (1=yes, 0=windowed) (default: 1)\n");
    fprintf(stderr, "  -h      Show this help\n");
}

struct Pos {
    float top_pct, left_pct;
};

typedef struct Entity {
    int anim_type;
    int pos_index;
    int toast_type;
} Entity;

int main(int argc, char *argv[]) {
    int opt;
    float speed_mult = 1.0f;
    int do_fullscreen = 1;
    int num_drops = 120;

    while ((opt = getopt(argc, argv, "t:s:f:h")) != -1) {
        switch (opt) {
            case 't':
                num_drops = atoi(optarg);
                if (num_drops < 1) num_drops = 1;
                if (num_drops > 400) num_drops = 400;
                break;
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

    // Removed setenv for style testing
    srand(time(NULL));

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("SDL_Init Error: %s", SDL_GetError());
        return 1;
    }

    if (!(IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF | IMG_INIT_WEBP))) {
        SDL_Log("IMG_Init Error: %s", IMG_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Hard Rain", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS | SDL_WINDOW_RESIZABLE);
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

    struct Pos *poses = malloc((size_t)num_drops * sizeof(struct Pos));
    Entity *entities = malloc((size_t)num_drops * sizeof(Entity));
    if (!poses || !entities) {
        SDL_Log("Out of memory");
        return 1;
    }

    for (int i = 0; i < num_drops; i++) {
        poses[i] = (struct Pos){rand() % 100, rand() % 100};
        entities[i] = (Entity){i * (0.5f / (num_drops / 10.0f + 0.1f)), i, rand() % 8};
    }

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

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // black background
        SDL_RenderClear(renderer);

        // Render rain drops as growing outline circles
        for (int i = 0; i < num_drops; i++) {
            const Entity ent = entities[i];
            const struct Pos pos = poses[ent.pos_index];

            float x = pos.left_pct * W / 100.0f;
            float y = pos.top_pct * H / 100.0f;

            int ix = (int)x;
            int iy = (int)y;

            // Animate radius (grow only from small to large)
            float fly_duration = 5.0f; // 5 second grow
            float local_time = time_s - ent.anim_type;
            if (local_time < 0) continue;
            local_time = fmodf(local_time, fly_duration);
            float factor = local_time / fly_duration;
            int radius = 10 + (int)(90.0f * factor);

            // Draw outline circle with cycling RGB colors
            uint32_t time_mod = (uint32_t)(time_s * 10); // Slower color cycling
            SDL_Color color = {(time_mod + (uint32_t)i * 30) % 256, (time_mod + (uint32_t)i * 60) % 256, (time_mod + (uint32_t)i * 90) % 256, 255};
            drawCircleOutline(renderer, ix, iy, radius, color);
        }



        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60fps
    }

    free(poses);
    free(entities);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
