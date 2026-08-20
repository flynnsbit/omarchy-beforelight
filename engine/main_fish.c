#include <SDL2/SDL.h>
#include "beforelight_input.h"
#include <SDL2/SDL_image.h>
#include "assets/fish_angel.h"
#include "assets/fish_butterfly.h"
#include "assets/fish_clown.h"
#include "assets/fish_flounder.h"
#include "assets/fish_guppy.h"
#include "assets/fish_jelly.h"
#include "assets/fish_minnow.h"
#include "assets/fish_red.h"
#include "assets/fish_seahorse.h"
#include "assets/fish_sprite.h"
#include "assets/fish_striped.h"
#include "assets/seafloor.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h> // for getopt

#define WINDOW_WIDTH 0  // fullscreen
#define WINDOW_HEIGHT 0
#define SPRITE_SIZE 145
#define FISH_FRAME_COUNT 4  // not used

extern char *optarg;

static void usage(const char *prog) {
    fprintf(stderr, "Usage: %s [options]\n", prog);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -t N    Number of fish (default: all)\n");
    fprintf(stderr, "  -m N    Number of bubbles (default: all)\n");
    fprintf(stderr, "  -s F    Speed multiplier (default: 1.0)\n");
    fprintf(stderr, "  -f 0|1  Fullscreen (1=yes, 0=windowed) (default: 1)\n");
    fprintf(stderr, "  -h      Show this help\n");
}

struct AnimParam {
    float fly_duration;
    float delay;
    int flap_direction; // 1 or -1 or 0 for toast
};

struct AnimParam anim_params[11] = {
    {18.2, 0.0, 0}, // ltr slowed
    {18.2, 0.0, 1}, // rtl slowed
    {9.1f, 0.0, 0}, // ltr-fast slowed
    {9.1f, 0.0, 1}, // rtl-fast slowed
    {18.2, 0.0, 1}, // rtl-delay1 slowed
    {18.2, 0.0, 1}, // rtl-delay-2 slowed
    {18.2, 0.0, 1}, // rtl-delay1 or similar slowed
    {18.2, 0.0, 1}, // rtl-delay2 slowed
    {18.2, 0.0, 0}, // bubble rise slowed
    {18.2, 4.0, 0}, // bubble rise delay slowed
    {18.2, 8.0, 0}, // bubble rise delay2 slowed
};

struct Pos {
    float top_pct;
};

const struct Pos poses[9] = {
    {-15}, // 0 row1
    {5},   // 1 row2
    {25},  // 2 row3
    {45},  // 3 row4
    {65},  // 4 row5
    {85},  // 5 row6
    {10},  // 6 bubble left
    {50},  // 7 bubble middle
    {85}   // 8 bubble right
};

typedef struct Entity {
    int is_toaster;
    int anim_type;
    int pos_index;
    int toast_type;
} Entity;

const Entity entities[] = {
    {0, 0, 5, 1}, // butterfly ltr row6 fish-butterfly.png
    {0, 3, 0, 4}, // jelly rtl-fast row1 fish-jelly.png
    {0, 1, 1, 3}, // guppy rtl row2 fish-guppy.png
    {0, 4, 1, 0}, // angel rtl-delay1 row2 fish-angel.png
    {0, 5, 2, 7}, // seahorse rtl-delay-2 row3 fish-seahorse.png
    {0, 7, 3, 6}, // red rtl-delay2 row4 fish-red.png
    {0, 0, 3, 4}, // jelly ltr row4 fish-jelly.png
    {0, 1, 4, 5}, // minnow rtl row5 fish-minnow.png
    {0, 2, 4, 7}, // seahorse ltr-fast row5 fish-seahorse.png
    {0, 3, 5, 0}, // angel rtl-fast row6 fish-angel.png
    {0, 0, 3, 2}, // flounder ltr row4 fish-flounder.png
    {0, 1, 2, 9}, // striped rtl row3 fish-striped.png
    {0, 3, 1, 10}, // clown rtl-fast row2 fish-clown.png
    {1, 8, 6, 4}, // bubble left
    {1, 9, 7, 4}, // bubble middle
    {1, 10, 8, 4}, // bubble right
};

#define MAX_BUBBLES 64

typedef struct RisingBubble {
    float base_x;
    float y;
    float speed;
    float radius;
    float wobble_amp;
    float wobble_freq;
    float wobble_phase;
    float age;
} RisingBubble;

static int seafloor_band(int H) {
    int band = H / 5;
    if (band < 96) band = 96;
    if (band > H / 3) band = H / 3;
    return band;
}

static void spawn_bubble(RisingBubble *b, int W, int H, int band) {
    float sand_top = (float)(H - band);
    b->base_x = 24.0f + (float)(rand() % (W > 48 ? W - 48 : 1));
    b->y = sand_top + 8.0f + (float)(rand() % 40);
    b->radius = 3.0f + (float)(rand() % 90) / 10.0f;
    /* Bigger bubbles rise slower, like glass in water. */
    b->speed = (18.0f + (12.0f - b->radius) * 2.2f);
    if (b->speed < 10.0f) b->speed = 10.0f;
    b->wobble_amp = 6.0f + b->radius * 0.8f;
    b->wobble_freq = 1.1f + (float)(rand() % 80) / 100.0f;
    b->wobble_phase = (float)(rand() % 628) / 100.0f;
    b->age = 0.0f;
}

static void draw_filled_circle(SDL_Renderer *r, int cx, int cy, int radius) {
    if (radius < 1) return;
    for (int dy = -radius; dy <= radius; dy++) {
        int span = (int)sqrtf((float)(radius * radius - dy * dy));
        SDL_RenderDrawLine(r, cx - span, cy + dy, cx + span, cy + dy);
    }
}

static void draw_circle_outline(SDL_Renderer *r, int cx, int cy, int radius) {
    int x = radius, y = 0, err = 0;
    while (x >= y) {
        SDL_RenderDrawPoint(r, cx + x, cy + y);
        SDL_RenderDrawPoint(r, cx + y, cy + x);
        SDL_RenderDrawPoint(r, cx - y, cy + x);
        SDL_RenderDrawPoint(r, cx - x, cy + y);
        SDL_RenderDrawPoint(r, cx - x, cy - y);
        SDL_RenderDrawPoint(r, cx - y, cy - x);
        SDL_RenderDrawPoint(r, cx + y, cy - x);
        SDL_RenderDrawPoint(r, cx + x, cy - y);
        if (err <= 0) {
            y += 1;
            err += 2 * y + 1;
        }
        if (err > 0) {
            x -= 1;
            err -= 2 * x + 1;
        }
    }
}

static void draw_bubble(SDL_Renderer *r, float x, float y, float radius, float age) {
    int cx = (int)(x + 0.5f);
    int cy = (int)(y + 0.5f);
    int rad = (int)(radius + 0.5f);
    if (rad < 2) rad = 2;
    float fade_in = age < 0.35f ? age / 0.35f : 1.0f;
    float pop = 1.0f;
    if (cy < 48) {
        pop = (float)cy / 48.0f;
        if (pop < 0.0f) pop = 0.0f;
        rad = (int)(rad * (1.0f + (1.0f - pop) * 0.55f));
    }
    Uint8 a = (Uint8)(fade_in * pop * 150.0f);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 210, 235, 255, (Uint8)(a * 0.35f));
    draw_filled_circle(r, cx, cy, rad);
    SDL_SetRenderDrawColor(r, 230, 248, 255, a);
    draw_circle_outline(r, cx, cy, rad);
    if (rad > 3) draw_circle_outline(r, cx, cy, rad - 1);
    int hx = cx - rad / 3;
    int hy = cy - rad / 3;
    int hr = rad > 5 ? rad / 4 : 1;
    SDL_SetRenderDrawColor(r, 255, 255, 255, (Uint8)(a * 0.9f));
    draw_filled_circle(r, hx, hy, hr);
}

int main(int argc, char *argv[]) {
    int opt;
    int fish_count = 33; // increased max default by 3
    int bubble_count = 15;
    float speed_mult = 1.0f;
    int do_fullscreen = 1;

    while ((opt = getopt(argc, argv, "t:m:s:f:h")) != -1) {
        switch (opt) {
            case 't':
                fish_count = atoi(optarg);
                break;
            case 'm':
                bubble_count = atoi(optarg);
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

    size_t entity_count = sizeof(entities) / sizeof(entities[0]);
    float entity_speed_mult[entity_count];
    float entity_delay[entity_count];
    float random_row_pct[entity_count];
    for (size_t j = 0; j < entity_count; j++) {
        entity_speed_mult[j] = 0.8f + (rand() % 10) * 0.1f;
        // Reduce initial delay for fish for quicker appearance
        if (entities[j].is_toaster == 0) {
            entity_delay[j] = (rand() % 500) * 0.001f; // 0.0s - 0.5s
        } else {
            entity_delay[j] = (rand() % 1000) * 0.01f; // keep bubbles slower
        }
        if (entities[j].is_toaster == 0) {
            random_row_pct[j] = 5.0f + (rand() % 81);
        }
    }
    // Force very first fish to appear immediately
    for (size_t j = 0; j < entity_count; j++) {
        if (entities[j].is_toaster == 0) { entity_delay[j] = 0.0f; break; }
    }

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("SDL_Init Error: %s", SDL_GetError());
        return 1;
    }

    if (!(IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF | IMG_INIT_WEBP))) {
        SDL_Log("IMG_Init Error: %s", IMG_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Fish Aquarium", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS | SDL_WINDOW_RESIZABLE);
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

    // Calculate margin for off-screen start/end
    int fish_size = SPRITE_SIZE / 2;
    float margin_pct = 1.0f + 2.0f * fish_size / (float)W;

    // Load textures
    SDL_Texture *fish_texs[11], *bg_tex;
    SDL_Surface *surf;
    int bg_w = 0, bg_h = 0;
    for (int i = 0; i < 11; i++) {
        const unsigned char *data = NULL;
        unsigned int len = 0;
        switch (i) {
            case 0: data = fish_angel; len = fish_angel_len; break;
            case 1: data = fish_butterfly; len = fish_butterfly_len; break;
            case 2: data = fish_flounder; len = fish_flounder_len; break;
            case 3: data = fish_guppy; len = fish_guppy_len; break;
            case 4: data = fish_jelly; len = fish_jelly_len; break;
            case 5: data = fish_minnow; len = fish_minnow_len; break;
            case 6: data = fish_red; len = fish_red_len; break;
            case 7: data = fish_seahorse; len = fish_seahorse_len; break;
            case 8: data = fish_sprite; len = fish_sprite_len; break;
            case 9: data = fish_striped; len = fish_striped_len; break;
            case 10: data = fish_clown; len = fish_clown_len; break;
        }
        SDL_RWops *rw = SDL_RWFromConstMem(data, len);
        if (!rw) {
            SDL_Log("Error creating RWops for embedded fish texture %d: %s", i, SDL_GetError());
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            IMG_Quit();
            SDL_Quit();
            return 1;
        }
        surf = IMG_Load_RW(rw, 1); // 1 to autoclose
        if (!surf) {
            SDL_Log("Error loading embedded fish texture %d: %s", i, IMG_GetError());
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            IMG_Quit();
            SDL_Quit();
            return 1;
        }
        fish_texs[i] = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_FreeSurface(surf);
    }

    // Background seabed
    SDL_RWops *rw_bg = SDL_RWFromConstMem(seafloor, seafloor_len);
    if (!rw_bg) {
        SDL_Log("Error creating RWops for embedded seafloor: %s", SDL_GetError());
        // Skip background
        bg_tex = NULL;
    } else {
        surf = IMG_Load_RW(rw_bg, 1); // 1 to autoclose
        if (!surf) {
            SDL_Log("Error loading embedded seafloor: %s", IMG_GetError());
            // Skip background
            bg_tex = NULL;
        } else {
            bg_tex = SDL_CreateTextureFromSurface(renderer, surf);
            bg_w = surf->w;
            bg_h = surf->h;
            SDL_FreeSurface(surf);
        }
    }

    int band = seafloor_band(H);
    if (bubble_count < 1) bubble_count = 1;
    if (bubble_count > MAX_BUBBLES) bubble_count = MAX_BUBBLES;
    RisingBubble bubbles[MAX_BUBBLES];
    for (int i = 0; i < bubble_count; i++) {
        spawn_bubble(&bubbles[i], W, H, band);
        bubbles[i].y -= (float)(rand() % (H > 80 ? H - 80 : 1));
        bubbles[i].age = 0.4f + (float)(rand() % 40) / 10.0f;
    }

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
        float dt = 1.0f / 60.0f;

        /* Match the seafloor overlay's water band: sampled RGB(32, 85, 139). */
        SDL_SetRenderDrawColor(renderer, 32, 85, 139, 255);
        SDL_RenderClear(renderer);

        // One seafloor band across the bottom — do not tile stacked copies.
        if (bg_tex) {
            int band = H / 5;
            if (band < 96) band = 96;
            if (band > H / 3) band = H / 3;
            SDL_Rect bgrect = {0, H - band, W, band};
            SDL_RenderCopy(renderer, bg_tex, NULL, &bgrect);
        } else {
            SDL_SetRenderDrawColor(renderer, 139, 69, 19, 255);
            SDL_RenderFillRect(renderer, &(SDL_Rect){0, H - 100, W, 100});
        }

        size_t entity_count = sizeof(entities) / sizeof(entities[0]);

        /* Rising bubbles — original drawn circles with a slow side-to-side wobble. */
        for (int i = 0; i < bubble_count; i++) {
            RisingBubble *b = &bubbles[i];
            b->age += dt * speed_mult;
            b->y -= b->speed * dt * speed_mult;
            if (b->y < -b->radius * 3.0f) {
                spawn_bubble(b, W, H, band);
            }
            float x = b->base_x + sinf(b->age * b->wobble_freq + b->wobble_phase) * b->wobble_amp;
            draw_bubble(renderer, x, b->y, b->radius, b->age);
        }

        // Render fish (is_toaster==0)
        int drawn_fish = 0;
        for (size_t i = 0; i < entity_count; i++) {
            const Entity ent = entities[i];
            if (ent.is_toaster != 0) continue;
            if (drawn_fish >= fish_count) continue;
            const struct AnimParam ap = anim_params[ent.anim_type];

            // Animation timing
            float local_time = time_s - (ap.delay + entity_delay[i]);
            if (local_time < 0) continue;

            float current_top_pct = random_row_pct[i];

            float effective_duration = ap.fly_duration * entity_speed_mult[i] * 1.2f;
            float cycle_time = fmodf(local_time, effective_duration);
            float fly_f = cycle_time / effective_duration;

            // Calculate fish size (fixed 50% smaller)
            int fish_size = SPRITE_SIZE / 2;

            // Calculate start position
            float start_y = (current_top_pct / 100.0f * H) - fish_size / 2.0f;

            // Fish swim animation
            int direction = ap.flap_direction;
            float start_left_pct, end_left_pct;
            if (direction == 0) { // ltr
                start_left_pct = -margin_pct;
                end_left_pct = 1.0f + margin_pct;
            } else { // rtl
                start_left_pct = 1.0f + margin_pct;
                end_left_pct = -margin_pct;
            }
            float delta_left_pct = end_left_pct - start_left_pct;
            float current_left_pct = start_left_pct + delta_left_pct * fly_f;

            float current_x = current_left_pct * W - fish_size / 2.0f;
            float current_y = start_y;
            SDL_Rect dstrect = {(int)current_x, (int)current_y, fish_size, fish_size};

            // Fish sprite animation (2 frames)
            float flap_cycle = fmodf(local_time, 0.6f); // 2 frames, 0.3s each
            int flap_frame = (int)(flap_cycle / 0.3f) % 2;
            SDL_Rect srcrect = {flap_frame * SPRITE_SIZE, 0, SPRITE_SIZE, SPRITE_SIZE};

            SDL_RendererFlip flip = (direction == 1) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
            SDL_RenderCopyEx(renderer, fish_texs[ent.toast_type], &srcrect, &dstrect, 0.0, NULL, flip);
            drawn_fish++;
        }



        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60fps
    }

    // Cleanup - restore cursor visibility
    system("hyprctl eval 'hl.config({ cursor = { invisible = false } })' >/dev/null 2>&1 || hyprctl keyword cursor:invisible false >/dev/null 2>&1");

    // Cleanup
    for (int i = 0; i < 11; i++) {
        SDL_DestroyTexture(fish_texs[i]);
    }
    if (bg_tex) SDL_DestroyTexture(bg_tex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
