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

void drawFilledCircle(SDL_Renderer *renderer, int centerX, int centerY, int radius, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    int r_squared = radius * radius;
    for (int y = centerY - radius; y <= centerY + radius; y++) {
        int dy = y - centerY;
        int dx_squared = r_squared - dy * dy;
        if (dx_squared > 0) {
            int dx = (int)sqrtf(dx_squared);
            SDL_RenderDrawLine(renderer, centerX - dx, y, centerX + dx, y);
        }
    }
}

static void usage(const char *prog) {
    fprintf(stderr, "Usage: %s [options]\n", prog);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -s F    Speed multiplier (default: 1.0)\n");
    fprintf(stderr, "  -f 0|1  Fullscreen (1=yes, 0=windowed) (default: 1)\n");
    fprintf(stderr, "  -h      Show this help\n");
}

typedef struct Ball {
    float x, y, vx, vy;
    SDL_Color color;
} Ball;

Ball balls[10];

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

    SDL_Window *window = SDL_CreateWindow("Bouncing Balls", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS | SDL_WINDOW_RESIZABLE);
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

    for(int i=0; i<10; i++) {
        balls[i].x = (float)(rand() % (W - 40));
        balls[i].y = (float)(rand() % (H - 40));
        balls[i].vx = (float)(rand() % 400 - 200);
        balls[i].vy = (float)(rand() % 400 - 200);
        balls[i].color = (SDL_Color){(uint8_t)(rand() % 256), (uint8_t)(rand() % 256), (uint8_t)(rand() % 256), 255};
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

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // black background
        SDL_RenderClear(renderer);

        // Update physics
        const float dt = 0.016f; // ~60fps
        int ball_size = 40;

        for(int i=0; i<10; i++) {
            balls[i].x += balls[i].vx * dt * speed_mult;
            balls[i].y += balls[i].vy * dt * speed_mult;

            // Wall collisions
            if (balls[i].x < 0 || balls[i].x > W - ball_size) {
                balls[i].vx = -balls[i].vx;
                balls[i].x = fmax(0, fmin(W - ball_size, balls[i].x));
            }
            if (balls[i].y < 0 || balls[i].y > H - ball_size) {
                balls[i].vy = -balls[i].vy;
                balls[i].y = fmax(0, fmin(H - ball_size, balls[i].y));
            }
        }

        // Ball-ball collisions
        for(int i=0; i<10; i++) {
            for(int j=i+1; j<10; j++) {
                Ball *b1 = &balls[i];
                Ball *b2 = &balls[j];
                float dx = b2->x - b1->x;
                float dy = b2->y - b1->y;
                float dist = sqrtf(dx*dx + dy*dy);
                if (dist < ball_size && dist > 0) {
                    // Separate
                    float overlap = ball_size - dist;
                    float nx = dx / dist;
                    float ny = dy / dist;
                    b1->x -= nx * overlap / 2;
                    b1->y -= ny * overlap / 2;
                    b2->x += nx * overlap / 2;
                    b2->y += ny * overlap / 2;

                    // Elastic collision (conservation of momentum/KE)
                    float tx = -ny;
                    float ty = nx;
                    float v1n = b1->vx * nx + b1->vy * ny;
                    float v1t = b1->vx * tx + b1->vy * ty;
                    float v2n = b2->vx * nx + b2->vy * ny;
                    float v2t = b2->vx * tx + b2->vy * ty;

                    // Swap normal velocities for elastic collision
                    b1->vx = v2n * nx + v1t * tx;
                    b1->vy = v2n * ny + v1t * ty;
                    b2->vx = v1n * nx + v2t * tx;
                    b2->vy = v1n * ny + v2t * ty;
                }
            }
        }

        // Render balls
        for(int i=0; i<10; i++) {
            int ix = (int)balls[i].x;
            int iy = (int)balls[i].y;
            int center_x = ix + 20;
            int center_y = iy + 20;
            drawFilledCircle(renderer, center_x, center_y, 20, balls[i].color);
        }



        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60fps
    }

    // Cleanup
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
