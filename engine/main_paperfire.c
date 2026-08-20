#include <SDL.h>
#include "beforelight_input.h"
#include <SDL_image.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h> // for getopt
#include <stdbool.h>

extern char *optarg;

#define PI 3.14159f
#define FIRE_GRID_SIZE 80  // 80x80 fire grid
#define MAX_PARTICLES 1000

typedef struct {
    float x, y;
    float vx, vy;
    float life;      // 0-1 how much life remaining
    int type;        // 0=ember, 1=ash, 2=smoke
    float size;
    SDL_Color color;
} Particle;

typedef struct {
    float fire_intensity[FIRE_GRID_SIZE][FIRE_GRID_SIZE];  // 0-1 fire level
    float burn_level[FIRE_GRID_SIZE][FIRE_GRID_SIZE];      // 0-1 burn progress
    float ash_level[FIRE_GRID_SIZE][FIRE_GRID_SIZE];       // 0-1 ash coverage
    Particle particles[MAX_PARTICLES];
    int particle_count;
} FireSystem;

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
                if (speed_mult > 5.0f) speed_mult = 5.0f;
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

    if (!(IMG_Init(IMG_INIT_PNG))) {
        SDL_Log("IMG_Init Error: %s", IMG_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Paper Fire", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS | SDL_WINDOW_RESIZABLE);
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

    // Initialize fire system
    FireSystem fire_sys;
    memset(&fire_sys, 0, sizeof(FireSystem));

    // Ignition points - start fire at corners
    fire_sys.fire_intensity[5][FIRE_GRID_SIZE-5] = 0.8f;          // Bottom left
    fire_sys.fire_intensity[FIRE_GRID_SIZE-5][FIRE_GRID_SIZE-5] = 0.8f;  // Bottom right
    fire_sys.fire_intensity[FIRE_GRID_SIZE/2][FIRE_GRID_SIZE-5] = 0.6f;  // Bottom center

    // Scale paper to fill screen proportionally
    int paper_width = W;
    int paper_height = H;

    // Create paper texture (creamy white paper) - full screen size
    SDL_Texture *paper_tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, paper_width, paper_height);
    SDL_SetTextureBlendMode(paper_tex, SDL_BLENDMODE_BLEND);

    // Render paper background
    SDL_SetRenderTarget(renderer, paper_tex);
    SDL_SetRenderDrawColor(renderer, 255, 250, 245, 255);  // Creamy paper color
    SDL_RenderClear(renderer);

    // Add some paper texture variation
    for (int y = 0; y < paper_height; y += 4) {
        for (int x = 0; x < paper_width; x += 4) {
            int variation = (rand() % 20) - 10;  // -10 to +10
            int alpha = 255 + variation;
            if (alpha > 255) alpha = 255;
            if (alpha < 245) alpha = 245;
            SDL_SetRenderDrawColor(renderer, 255, 250, 245, (Uint8)alpha);
            SDL_Rect texel = {x, y, 4, 4};
            SDL_RenderFillRect(renderer, &texel);
        }
    }

    SDL_SetRenderTarget(renderer, NULL);  // Back to main renderer

    // Animation phases and timing
    float animation_time = 0;
    const float paper_appear_time = 2.0f;     // Paper fades in

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
                quit = 1;
            } else if (e.type == SDL_MOUSEMOTION) {
                // Only quit on mouse motion after 2 seconds to prevent immediate quit
                if (beforelight_should_quit_motion(&bl_input, start_time, &e)) {
                    quit = 1;
                }
            }
        }

        animation_time += 0.016f * speed_mult;

        // Update fire simulation
        // Spread fire intensity
        float new_intensity[FIRE_GRID_SIZE][FIRE_GRID_SIZE];
        memcpy(new_intensity, fire_sys.fire_intensity, sizeof(new_intensity));

        for (int y = 1; y < FIRE_GRID_SIZE-1; y++) {
            for (int x = 1; x < FIRE_GRID_SIZE-1; x++) {
                if (fire_sys.fire_intensity[x][y] > 0.1f) {
                    // Spread to neighbors
                    float spread_amount = fire_sys.fire_intensity[x][y] * 0.15f * speed_mult;
                    new_intensity[x-1][y] += spread_amount * 0.5f;
                    new_intensity[x+1][y] += spread_amount * 0.5f;
                    new_intensity[x][y-1] += spread_amount * 0.5f;
                    new_intensity[x][y+1] += spread_amount * 0.5f;

                    // Reduce current intensity
                    new_intensity[x][y] -= fire_sys.fire_intensity[x][y] * 0.1f * speed_mult;
                }

                // Cap intensity
                if (new_intensity[x][y] > 1.0f) new_intensity[x][y] = 1.0f;
                if (new_intensity[x][y] < 0) new_intensity[x][y] = 0;
            }
        }

        memcpy(fire_sys.fire_intensity, new_intensity, sizeof(new_intensity));

        // Update burn levels and create particles
        for (int y = 0; y < FIRE_GRID_SIZE; y++) {
            for (int x = 0; x < FIRE_GRID_SIZE; x++) {
                if (fire_sys.fire_intensity[x][y] > 0.5f) {
                    fire_sys.burn_level[x][y] += fire_sys.fire_intensity[x][y] * 0.02f * speed_mult;
                    if (fire_sys.burn_level[x][y] > 1.0f) fire_sys.burn_level[x][y] = 1.0f;

                    // Create embers/smoke particles occasionally
                    if (rand() % 200 < 3 && fire_sys.particle_count < MAX_PARTICLES) {
                        int idx = fire_sys.particle_count++;
                        Particle *p = &fire_sys.particles[idx];

                        // Position relative to paper (now fullscreen)
                        float paper_x = x * (paper_width / (float)FIRE_GRID_SIZE);
                        float paper_y = y * (paper_height / (float)FIRE_GRID_SIZE);

                        p->x = paper_x + (rand() % 10 - 5);
                        p->y = paper_y;
                        p->vx = (rand() % 40 - 20) / 10.0f;
                        p->vy = -(rand() % 20 + 10) / 10.0f;  // Upward
                        p->life = 1.0f;
                        p->size = 2 + rand() % 3;
                        p->type = rand() % 3;  // Mix of ember/ash/smoke

                        if (p->type == 0) {  // Ember - glowy red/orange
                            p->color.r = 255; p->color.g = 100 + rand() % 100; p->color.b = 0; p->color.a = 255;
                        } else if (p->type == 1) {  // Ash - dark gray
                            int gray = 50 + rand() % 100;
                            p->color.r = p->color.g = p->color.b = gray; p->color.a = 200;
                        } else {  // Smoke - light gray, transparent
                            int gray = 150 + rand() % 100;
                            p->color.r = p->color.g = p->color.b = gray; p->color.a = 100;
                            p->vy = -(rand() % 30 + 5) / 10.0f;  // Gentler rise
                        }
                    }
                }

                // Turn burn into ash
                if (fire_sys.burn_level[x][y] > 0.8f) {
                    fire_sys.ash_level[x][y] += 0.01f * speed_mult;
                    if (fire_sys.ash_level[x][y] > 1.0f) fire_sys.ash_level[x][y] = 1.0f;
                }
            }
        }

        // Update particles
        for (int i = 0; i < fire_sys.particle_count; i++) {
            Particle *p = &fire_sys.particles[i];
            if (p->life <= 0) continue;

            p->x += p->vx * speed_mult;
            p->y += p->vy * speed_mult;

            // Apply gravity/wind
            if (p->type == 1) {  // Ash falls
                p->vy += 0.1f * speed_mult;
            } else if (p->type == 2) {  // Smoke rises
                p-> vy -= 0.05f * speed_mult;
                p->vx += (sinf(animation_time + i) * 0.2f) * speed_mult;  // Drift with wind
            }

            // Fade out
            p->life -= 0.01f * speed_mult;
            if (p->type == 2) {  // Smoke fades slower
                p->life -= 0.005f * speed_mult;
            }
        }

        // Remove dead particles
        int write_idx = 0;
        for (int read_idx = 0; read_idx < fire_sys.particle_count; read_idx++) {
            if (fire_sys.particles[read_idx].life > 0) {
                fire_sys.particles[write_idx++] = fire_sys.particles[read_idx];
            }
        }
        fire_sys.particle_count = write_idx;

        // Rendering
        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);  // Dark background
        SDL_RenderClear(renderer);

        // Paper now fills entire screen (no positioning needed)

        // Fade in paper
        float paper_alpha = 1.0f;
        if (animation_time < paper_appear_time) {
            paper_alpha = animation_time / paper_appear_time;
        }

        // Render paper with burn deformation
        SDL_SetTextureAlphaMod(paper_tex, (Uint8)(paper_alpha * 255));

        // Render burned areas as overlay
        SDL_Texture *burn_tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, paper_width, paper_height);
        SDL_SetTextureBlendMode(burn_tex, SDL_BLENDMODE_BLEND);
        SDL_SetRenderTarget(renderer, burn_tex);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);  // Transparent base
        SDL_RenderClear(renderer);

        // Draw burn layers
        for (int y = 0; y < FIRE_GRID_SIZE; y++) {
            for (int x = 0; x < FIRE_GRID_SIZE; x++) {
                int px = x * (paper_width / FIRE_GRID_SIZE);
                int py = y * (paper_height / FIRE_GRID_SIZE);
                int pw = (paper_width / FIRE_GRID_SIZE) + 1;
                int ph = (paper_height / FIRE_GRID_SIZE) + 1;

                if (fire_sys.ash_level[x][y] > 0) {
                    // Ash - dark gray to black
                    int gray = 255 - (int)(fire_sys.ash_level[x][y] * 255);
                    SDL_SetRenderDrawColor(renderer, gray, gray, gray, 255);
                    SDL_Rect burn_rect = {px, py, pw, ph};
                    SDL_RenderFillRect(renderer, &burn_rect);
                } else if (fire_sys.burn_level[x][y] > 0) {
                    // Burning - yellow to red to black
                    float burn = fire_sys.burn_level[x][y];
                    int r = 255, g = (int)(burn * 255), b = 0;
                    if (burn > 0.5f) {
                        r = 255 - (int)((burn - 0.5f) * 2 * 255);
                        g = 128 - (int)((burn - 0.5f) * 256);
                    }
                    int alpha = (int)(fire_sys.fire_intensity[x][y] * 200);
                    SDL_SetRenderDrawColor(renderer, r, g, b, alpha);
                    SDL_Rect burn_rect = {px, py, pw, ph};
                    SDL_RenderFillRect(renderer, &burn_rect);
                }
            }
        }

        SDL_SetRenderTarget(renderer, NULL);  // Back to main renderer

        // Render paper (fullscreen)
        SDL_Rect paper_rect = {0, 0, paper_width, paper_height};
        SDL_RenderCopy(renderer, paper_tex, NULL, &paper_rect);

        // Render burn overlay
        SDL_SetTextureBlendMode(burn_tex, SDL_BLENDMODE_MOD);
        SDL_RenderCopy(renderer, burn_tex, NULL, &paper_rect);

        SDL_DestroyTexture(burn_tex);

        // Render particles
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
        for (int i = 0; i < fire_sys.particle_count; i++) {
            Particle *p = &fire_sys.particles[i];
            if (p->life <= 0) continue;

            int alpha = (int)(p->life * p->color.a);
            SDL_SetRenderDrawColor(renderer, p->color.r, p->color.g, p->color.b, alpha);

            int size = (int)(p->size * p->life);
            if (size < 1) size = 1;

            SDL_Rect particle_rect = {(int)p->x - size/2, (int)p->y - size/2, size, size};
            SDL_RenderFillRect(renderer, &particle_rect);
        }
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60fps

        // Check if fire has reached the top of the screen - reset when top rows are burning
        bool fire_at_top = false;
        for (int x = 0; x < FIRE_GRID_SIZE; x++) {
            if (fire_sys.fire_intensity[x][2] > 0.3f || fire_sys.fire_intensity[x][3] > 0.3f) {
                fire_at_top = true;
                break;
            }
        }

        // Reset animation when fire reaches the top
        if (fire_at_top && animation_time > 3.0f) {  // Allow some initial burn time
            animation_time = 0;
            // Reset fire system
            memset(&fire_sys, 0, sizeof(FireSystem));
            fire_sys.fire_intensity[5][FIRE_GRID_SIZE-5] = 0.8f;
            fire_sys.fire_intensity[FIRE_GRID_SIZE-5][FIRE_GRID_SIZE-5] = 0.8f;
            fire_sys.fire_intensity[FIRE_GRID_SIZE/2][FIRE_GRID_SIZE-5] = 0.6f;
        }
    }

    // Cleanup - restore cursor visibility
    system("hyprctl eval 'hl.config({ cursor = { invisible = false } })' >/dev/null 2>&1 || hyprctl keyword cursor:invisible false >/dev/null 2>&1");

    // Cleanup
    SDL_DestroyTexture(paper_tex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
