#include <SDL2/SDL.h>
#include "beforelight_input.h"
#include <SDL2/SDL_image.h>
#include "assets/toaster_sprite.h"
#include "assets/toast0.h"
#include "assets/toast1.h"
#include "assets/toast2.h"
#include "assets/toast3.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h> // for getopt
extern char *optarg;

#define WINDOW_WIDTH 0  // fullscreen
#define WINDOW_HEIGHT 0
#define SPRITE_SIZE 64
#define TOASTER_FRAME_COUNT 4

static void usage(const char *prog) {
    fprintf(stderr, "Usage: %s [options]\n", prog);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -t N    Number of toasters (default: all)\n");
    fprintf(stderr, "  -m N    Number of toast pieces (default: all)\n");
    fprintf(stderr, "  -s F    Speed multiplier (default: 1.0)\n");
    fprintf(stderr, "  -f 0|1  Fullscreen (1=yes, 0=windowed) (default: 1)\n");
    fprintf(stderr, "  -h      Show this help\n");
}

struct AnimParam {
    float fly_duration;
    float delay;
    int flap_direction; // 1 or -1 or 0 for toast
};

const struct AnimParam anim_params[14] = {
    {10.0, 0.0, 1}, // t1
    {16.0, 0.0, -1}, // t2
    {24.0, 0.0, 1}, // t3
    {10.0, 5.0, 1}, // t4
    {24.0, 4.0, -1}, // t5
    {24.0, 8.0, 1}, // t6
    {24.0, 12.0, -1}, // t7
    {24.0, 16.0, 1}, // t8
    {24.0, 20.0, -1}, // t9
    // tst1-tst4
    {10.0, 0.0, 0}, // tst1
    {16.0, 0.0, 0}, // tst2
    {24.0, 0.0, 0}, // tst3
    {24.0, 12.0, 0}, // tst4
};

struct Pos {
    float right_pct;
    float top_pct;
};

const struct Pos poses[34] = {
    [ 6] = {-2, -17},
    [ 7] = {10, -19},
    [ 8] = {20, -18},
    [ 9] = {30, -20},
    [10] = {40, -21},
    [11] = {50, -18},
    [12] = {60, -20},
    [13] = {-17, 10},
    [14] = {-19, 20},
    [15] = {-21, 30},
    [16] = {-23, 50},
    [17] = {-25, 70},
    [18] = {0, -26},
    [19] = {10, -20},
    [20] = {20, -36},
    [21] = {30, -24},
    [22] = {40, -33},
    [23] = {60, -40},
    [24] = {-26, 10},
    [25] = {-36, 30},
    [26] = {-29, 50},
    [27] = {0, -46},
    [28] = {10, -56},
    [29] = {20, -49},
    [30] = {30, -60},
    [31] = {-46, 10},
    [32] = {-56, 20},
    [33] = {-49, 30},
};

typedef struct Entity {
    int is_toaster;
    int anim_type;
    int pos_index;
    int toast_type;
} Entity;

const Entity entities[] = {
    {1, 0, 6, -1}, // toaster t1 p6
    {1, 2, 7, -1}, // t3 p7
    {0, 10, 8, 1}, // toast tst1 p8
    {1, 2, 9, -1}, // t3 p9
    {1, 0, 11, -1}, // t1 p11
    {1, 2, 12, -1}, // t3 p12
    {1, 1, 13, -1}, // t2 p13
    {0, 12, 14, 3}, // tst3 p14
    {0, 11, 16, 2}, // tst2 p16
    {1, 0, 17, -1}, // t1 p17
    {0, 11, 19, 0}, // tst2 p19
    {0, 12, 20, 3}, // tst3 p20
    {1, 1, 21, -1}, // t2 p21
    {0, 10, 24, 0}, // tst1 p24
    {1, 0, 22, -1}, // t1 p22 first
    {0, 11, 26, 2}, // tst2 p26
    {1, 0, 28, -1}, // t1 p28
    {0, 11, 30, 3}, // tst2 p30
    {1, 1, 31, -1}, // t2 p31
    {1, 0, 32, -1}, // t1 p32
    {0, 12, 33, 1}, // tst3 p33
    // wave 1
    {1, 3, 27, -1}, // t4 p27
    {1, 3, 10, -1}, // t4 p10
    {1, 3, 25, -1}, // t4 p25
    {1, 3, 29, -1}, // t4 p29
    // wave 2
    {1, 4, 15, -1}, // t5 p15
    {1, 4, 18, -1}, // t5 p18
    {1, 4, 22, -1}, // t5 p22 second
    // wave 3
    {1, 5, 6, -1}, // t6 p6 second
    {1, 5, 11, -1}, // t6 p11
    {1, 5, 15, -1}, // t6 p15
    {1, 5, 19, -1}, // t6 p19
    {1, 5, 23, -1}, // t6 p23
    // wave 5
    {0, 13, 10, 0}, // tst4 p10
    {0, 13, 23, 1}, // tst4 p23
    {0, 13, 15, 2}, // tst4 p15
    {1, 6, 7, -1}, // t7 p7
    {1, 6, 12, -1}, // t7 p12
    {1, 6, 16, -1}, // t7 p16
    {1, 6, 20, -1}, // t7 p20
    {1, 6, 24, -1}, // t7 p24
    // wave 6
    {1, 7, 8, -1}, // t8 p8
    {1, 7, 13, -1}, // t8 p13
    {1, 7, 17, -1}, // t8 p17
    {1, 7, 25, -1}, // t8 p25
    // wave 7
    {1, 8, 14, -1}, // t9 p14
    {1, 8, 18, -1}, // t9 p18
    {1, 8, 21, -1}, // t9 p21
    {1, 8, 26, -1}, // t9 p26
};

int main(int argc, char *argv[]) {
    int opt;
    int toaster_count = 30;
    int toast_count = 10;
    float speed_mult = 1.0f;
    int do_fullscreen = 1;

    while ((opt = getopt(argc, argv, "t:m:s:f:h")) != -1) {
        switch (opt) {
            case 't':
                toaster_count = atoi(optarg);
                break;
            case 'm':
                toast_count = atoi(optarg);
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

    SDL_Window *window = SDL_CreateWindow("Paper Toasters", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS | SDL_WINDOW_RESIZABLE);
    if (window == NULL) {
        SDL_Log("SDL_CreateWindow Error: %s", SDL_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    beforelight_grab_cursor();

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

    // Load textures
    SDL_Texture *toaster_tex;
    {
        SDL_RWops *rw = SDL_RWFromConstMem(toaster_sprite, toaster_sprite_len);
        if (!rw) {
            SDL_Log("Error creating RWops for embedded toaster_sprite: %s", SDL_GetError());
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            IMG_Quit();
            SDL_Quit();
            return 1;
        }
        SDL_Surface *surf = IMG_Load_RW(rw, 1); // 1 to autoclose
        if (!surf) {
            SDL_Log("Error loading embedded toaster_sprite: %s", IMG_GetError());
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            IMG_Quit();
            SDL_Quit();
            return 1;
        }
        toaster_tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_FreeSurface(surf);
    }

    SDL_Texture *toast_texs[4];
    for (int i = 0; i < 4; i++) {
        const unsigned char *data = NULL;
        unsigned int len = 0;
        switch (i) {
            case 0: data = toast0; len = toast0_len; break;
            case 1: data = toast1; len = toast1_len; break;
            case 2: data = toast2; len = toast2_len; break;
            case 3: data = toast3; len = toast3_len; break;
        }
        SDL_RWops *rw = SDL_RWFromConstMem(data, len);
        if (!rw) {
            SDL_Log("Error creating RWops for embedded toast%d: %s", i, SDL_GetError());
            SDL_DestroyTexture(toaster_tex);
            for (int j = 0; j < i; j++) SDL_DestroyTexture(toast_texs[j]);
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            IMG_Quit();
            SDL_Quit();
            return 1;
        }
        SDL_Surface *surf = IMG_Load_RW(rw, 1); // 1 to autoclose
        if (!surf) {
            SDL_Log("Error loading embedded toast%d: %s", i, IMG_GetError());
            SDL_DestroyTexture(toaster_tex);
            for (int j = 0; j < i; j++) SDL_DestroyTexture(toast_texs[j]);
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            IMG_Quit();
            SDL_Quit();
            return 1;
        }
        toast_texs[i] = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_FreeSurface(surf);
    }

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

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        size_t entity_count = sizeof(entities) / sizeof(entities[0]);

        // Render toast first (behind)
        int drawn_toast = 0;
        for (size_t i = 0; i < entity_count; i++) {
            const Entity ent = entities[i];
            if (ent.is_toaster) continue; // Skip toasters
            if (drawn_toast >= toast_count) continue; // Limit count
            const struct AnimParam ap = anim_params[ent.anim_type];
            const struct Pos pos = poses[ent.pos_index];

            // Calculate start position
            float start_x = W - (pos.right_pct / 100.0f * W) - SPRITE_SIZE / 2.0f;
            float start_y = (pos.top_pct / 100.0f * H) - SPRITE_SIZE / 2.0f;

            // Animation timing
            float local_time = time_s - ap.delay;
            if (local_time < 0) continue; // not started yet

            float cycle_time = fmodf(local_time, ap.fly_duration);
            float fly_f = cycle_time / ap.fly_duration;

            float current_x = start_x + (-1600.0f * fly_f * speed_mult);
            float current_y = start_y + (1600.0f * fly_f * speed_mult);

            SDL_Rect dstrect = {(int)current_x, (int)current_y, SPRITE_SIZE, SPRITE_SIZE};

            // Toast
            SDL_RenderCopy(renderer, toast_texs[ent.toast_type], NULL, &dstrect);
            drawn_toast++;
        }

        // Render toasters second (in front)
        int drawn_toasters = 0;
        for (size_t i = 0; i < entity_count; i++) {
            const Entity ent = entities[i];
            if (!ent.is_toaster) continue; // Skip toast
            if (drawn_toasters >= toaster_count) continue; // Limit count
            const struct AnimParam ap = anim_params[ent.anim_type];
            const struct Pos pos = poses[ent.pos_index];

            // Calculate start position
            float start_x = W - (pos.right_pct / 100.0f * W) - SPRITE_SIZE / 2.0f;
            float start_y = (pos.top_pct / 100.0f * H) - SPRITE_SIZE / 2.0f;

            // Animation timing
            float local_time = time_s - ap.delay;
            if (local_time < 0) continue; // not started yet

            float cycle_time = fmodf(local_time, ap.fly_duration);
            float fly_f = cycle_time / ap.fly_duration;

            float current_x = start_x + (-1600.0f * fly_f * speed_mult);
            float current_y = start_y + (1600.0f * fly_f * speed_mult);

            SDL_Rect dstrect = {(int)current_x, (int)current_y, SPRITE_SIZE, SPRITE_SIZE};

            // Calculate flap frame
            float flap_cycle = fmodf(local_time, 0.4f);
            int flap_frame = 0;
            if (ap.flap_direction == 1) {
                // alternate: 0->1->2->3->2->1->0
                if (flap_cycle < 0.2f) {
                    flap_frame = (int)(flap_cycle / 0.2f * 4);
                } else {
                    flap_frame = 3 - (int)((flap_cycle - 0.2f) / 0.2f * 3);
                }
            } else if (ap.flap_direction == -1) {
                // alternate-reverse: 3->2->1->0->1->2
                if (flap_cycle < 0.2f) {
                    flap_frame = 3 - (int)(flap_cycle / 0.2f * 4);
                } else {
                    flap_frame = (int)((flap_cycle - 0.2f) / 0.2f * 3);
                }
            }
            // Clamp
            if (flap_frame < 0) flap_frame = 0;
            if (flap_frame > 3) flap_frame = 3;

            SDL_Rect srcrect = {flap_frame * SPRITE_SIZE, 0, SPRITE_SIZE, SPRITE_SIZE};
            /* Sprite faces right; flight is left-and-down, so flip to face travel. */
            SDL_RenderCopyEx(renderer, toaster_tex, &srcrect, &dstrect, 0, NULL, SDL_FLIP_HORIZONTAL);
            drawn_toasters++;
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60fps
    }


    // Cleanup
    SDL_DestroyTexture(toaster_tex);
    for (int i = 0; i < 4; i++) {
        SDL_DestroyTexture(toast_texs[i]);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
