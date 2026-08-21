#include <SDL2/SDL.h>
#include "beforelight_input.h"
#include "beforelight_capture.h"
#include <SDL2/SDL_image.h>
#include <math.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include "assets/omarchy_logo.h"

#define PI 3.141592653589793f

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

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("SDL_Init Error: %s", SDL_GetError());
        return 1;
    }

    if (!(IMG_Init(IMG_INIT_PNG))) {
        SDL_Log("IMG_Init Error: %s", IMG_GetError());
        SDL_Quit();
        return 1;
    }



    SDL_Surface *screenshot_surf = beforelight_capture_screen();
    if (screenshot_surf)
        SDL_Log("Native capture %dx%d", screenshot_surf->w, screenshot_surf->h);

    if (!screenshot_surf) {
        SDL_Log("Cannot capture screen, using embedded Omarchy logo as fallback");
        SDL_RWops *rw = SDL_RWFromMem(omarchy_logo, omarchy_logo_len);
        if (rw) {
            screenshot_surf = IMG_Load_RW(rw, 1);  // 1 to auto-close rw
        }
        if (!screenshot_surf) {
            SDL_Log("Failed to load embedded logo: %s", IMG_GetError());
        }
    }

    if (!screenshot_surf) {
        SDL_Log("No background available");
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    Uint32 flags = SDL_WINDOW_SHOWN;
    int win_w = 800;
    int win_h = 600;
    int win_x = SDL_WINDOWPOS_UNDEFINED;
    int win_y = SDL_WINDOWPOS_UNDEFINED;
    SDL_Rect bounds = {0};
    if (do_fullscreen) {
        flags |= SDL_WINDOW_BORDERLESS;
        SDL_GetDisplayBounds(0, &bounds); // assume display 0
        win_w = bounds.w;
        win_h = bounds.h;
        win_x = bounds.x;
        win_y = bounds.y;
    }

    // Now create window
    SDL_Window *window = SDL_CreateWindow("Spotlight", win_x, win_y, win_w, win_h, flags);
    if (window == NULL) {
        SDL_Log("SDL_CreateWindow Error: %s", SDL_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    beforelight_grab_cursor();

    if (do_fullscreen) {
        // Make window fullscreen in Hyprland to hide the bar
        SDL_Delay(500); // Allow window to be mapped and settled
        SDL_RaiseWindow(window); // Make the window active
        SDL_Delay(100); // Allow focus
        system("(hyprctl dispatch fullscreen > /dev/null 2>&1)");
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL) {
        SDL_Log("SDL_CreateRenderer Error: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    int W, H;
    SDL_GetRendererOutputSize(renderer, &W, &H);
    SDL_Log("Renderer output: W=%d H=%d", W, H);

    // Create texture from screenshot
    SDL_Texture *bg_tex = SDL_CreateTextureFromSurface(renderer, screenshot_surf);
#if SDL_VERSION_ATLEAST(2, 0, 12)
    if (bg_tex)
        SDL_SetTextureScaleMode(bg_tex, SDL_ScaleModeLinear);
#endif
    if (!bg_tex) {
        SDL_Log("Cannot create texture from screenshot: %s", SDL_GetError());
        SDL_FreeSurface(screenshot_surf);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    SDL_FreeSurface(screenshot_surf);

    // Spotlight properties
    float radius = 200.0f;

    // Geometry setup for circular spotlight
    const int segments = 64; // more segments for smoother circle
    SDL_Vertex vertices[segments + 1];
    int indices[segments * 3];

    // Initialize triangle fan indices
    for (int i = 0; i < segments; i++) {
        int a = i;
        int b = (i + 1) % segments;
        indices[i * 3] = 0;
        indices[i * 3 + 1] = a + 1;
        indices[i * 3 + 2] = b + 1;
    }

    // Get texture dimensions
    int tex_width, tex_height;
    SDL_QueryTexture(bg_tex, NULL, NULL, &tex_width, &tex_height);
    SDL_Log("Texture size: width=%d height=%d", tex_width, tex_height);
    float scale_factor = 1.0f; // stretch to fill for both modes
    float spotlight_x = W / 2.0f;
    float spotlight_y = H / 2.0f;
    float spotlight_vx = (rand() % 400 - 200) * 1.0f;
    float spotlight_vy = (rand() % 400 - 200) * 1.0f;

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

        // Update spotlight movement
        float dt = 0.016f;
        spotlight_x += spotlight_vx * dt * speed_mult;
        spotlight_y += spotlight_vy * dt * speed_mult;

        // Bounce off walls
        if (spotlight_x <= radius || spotlight_x >= W - radius) {
            spotlight_vx = -spotlight_vx;
            spotlight_x = spotlight_x <= radius ? radius : W - radius;
        }
        if (spotlight_y <= radius || spotlight_y >= H - radius) {
            spotlight_vy = -spotlight_vy;
            spotlight_y = spotlight_y <= radius ? radius : H - radius;
        }

        // Render
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Update geometry vertices for the circular spotlight
        vertices[0].position.x = spotlight_x;
        vertices[0].position.y = spotlight_y;
        vertices[0].tex_coord.x = (spotlight_x / W) * scale_factor;
        vertices[0].tex_coord.y = (spotlight_y / H) * scale_factor;
        vertices[0].color = (SDL_Color){255, 255, 255, 255};

        for (int i = 0; i < segments; i++) {
            float angle = 2.0f * PI * i / segments;
            float px = spotlight_x + cosf(angle) * radius;
            float py = spotlight_y + sinf(angle) * radius;
            vertices[i + 1].position.x = px;
            vertices[i + 1].position.y = py;
            vertices[i + 1].tex_coord.x = (px / W) * scale_factor;
            vertices[i + 1].tex_coord.y = (py / H) * scale_factor;
            vertices[i + 1].color = (SDL_Color){255, 255, 255, 255};
        }

        // Render only the circular spotlight area from the background texture
        SDL_RenderGeometry(renderer, bg_tex, vertices, segments + 1, indices, segments * 3);

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60fps
    }

    // Exit fullscreen on quit to show Waybar immediately
    system("(hyprctl dispatch fullscreen > /dev/null 2>&1)");
    SDL_Delay(200); // Allow Hyprland to process fullscreen exit

    // Cleanup
    if (bg_tex) SDL_DestroyTexture(bg_tex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
