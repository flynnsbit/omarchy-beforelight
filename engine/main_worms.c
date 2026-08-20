#include <SDL2/SDL.h>
#include "beforelight_input.h"
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h> // for getopt

#define PI 3.141592653589793f

// Convert HSV (0-360,0-1,0-1) to SDL_Color (RGB)
static SDL_Color hsv_to_rgb(float h, float s, float v) {
    float c = v * s;
    float x = c * (1 - fabsf(fmodf(h / 60.0f, 2) - 1));
    float m = v - c;
    float r=0,g=0,b=0;
    if (h < 60) { r=c; g=x; b=0; }
    else if (h < 120) { r=x; g=c; b=0; }
    else if (h < 180) { r=0; g=c; b=x; }
    else if (h < 240) { r=0; g=x; b=c; }
    else if (h < 300) { r=x; g=0; b=c; }
    else { r=c; g=0; b=x; }
    SDL_Color col = { (Uint8)((r+m)*255), (Uint8)((g+m)*255), (Uint8)((b+m)*255), 255 };
    return col;
}

extern char *optarg;

static void usage(const char *prog) {
    fprintf(stderr, "Usage: %s [options]\n", prog);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -n N    Number of worms (default: 5)\n");
    fprintf(stderr, "  -l N    Trail length (segments per worm, default: 100)\n");
    fprintf(stderr, "  -s F    Speed multiplier (default: 1.0)\n");
    fprintf(stderr, "  -f 0|1  Fullscreen (1=yes, 0=windowed) (default: 1)\n");
    fprintf(stderr, "  -w F    Wiggle factor (0=straight, 1=max wiggle) (default: 0.02)\n");
    fprintf(stderr, "  -a 0|1  Audio (1=on, 0=off) (default: 1)\n");
    fprintf(stderr, "  -h      Show this help\n");
}

typedef struct Worm {
    float x, y;
    float vx, vy;
    SDL_Color color;
    int length;
    SDL_Point *segments;
} Worm;

int main(int argc, char *argv[]) {
    int opt;
    int worm_count = 5;
    int trail_length = 50; // shortened default for less initial workload
    float speed_mult = 1.0f;
    int do_fullscreen = 1;
    float wiggle = 0.02f;
    int audio_enabled = 0; // default audio off; enable with -a 1

    while ((opt = getopt(argc, argv, "n:l:s:f:w:a:h")) != -1) {
        switch (opt) {
            case 'n':
                worm_count = atoi(optarg);
                if (worm_count < 1) worm_count = 1;
                if (worm_count > 50) worm_count = 50;
                break;
            case 'l':
                trail_length = atoi(optarg);
                if (trail_length < 5) trail_length = 5;
                if (trail_length > 100) trail_length = 100;
                break;
            case 's':
                speed_mult = atof(optarg);
                if (speed_mult <= 0.1f) speed_mult = 0.1f;
                if (speed_mult > 10.0f) speed_mult = 10.0f;
                break;
            case 'f':
                do_fullscreen = atoi(optarg);
                break;
            case 'w':
                wiggle = atof(optarg);
                if (wiggle < 0.0f) wiggle = 0.0f;
                if (wiggle > 1.0f) wiggle = 1.0f;
                break;
            case 'a':
                audio_enabled = atoi(optarg);
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

    if (TTF_Init() != 0) {
        SDL_Log("TTF_Init Error: %s", TTF_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    TTF_Font *font = NULL;
    const char *font_paths[] = {
        "/usr/share/fonts/noto/NotoSans-Regular.ttf",
        "/usr/share/fonts/noto/NotoSansMono-Regular.ttf",
        "/usr/share/fonts/TTF/CaskaydiaMonoNerdFont-Regular.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
        "/usr/share/fonts/TTF/DejaVuSansMono.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
        NULL
    };
    for (int i = 0; font_paths[i]; i++) {
    font = TTF_OpenFont(font_paths[i], 16); // doubled glyph size for thicker worms
        if (font) break;
    }
    if (!font) {
        SDL_Log("Cannot load font");
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    Mix_Chunk *chomp = NULL;
    if (audio_enabled) {
        if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
            SDL_Log("Mix_OpenAudio Error: %s", Mix_GetError());
            TTF_CloseFont(font);
            TTF_Quit();
            IMG_Quit();
            SDL_Quit();
            return 1;
        }
        chomp = NULL;
    }

    // Screenshot capture (from main_spotlight.c)
    SDL_Surface *screenshot_surf = NULL;
    SDL_Log("Attempting screen capture...");
    int grim_result = system("grim worms_temp.png > /dev/null 2>&1");
    if (grim_result == 0) {
        SDL_Log("Screen capture succeeded");
        screenshot_surf = IMG_Load("worms_temp.png");
        unlink("worms_temp.png");
    } else {
        SDL_Log("Screen capture failed (exit code %d), using black background", grim_result);
    }

    Uint32 flags = SDL_WINDOW_SHOWN;
    int win_w = 800;
    int win_h = 600;
    int win_x = SDL_WINDOWPOS_UNDEFINED;
    int win_y = SDL_WINDOWPOS_UNDEFINED;
    SDL_Rect bounds = {0};
    if (do_fullscreen) {
        flags |= SDL_WINDOW_BORDERLESS;
        SDL_GetDisplayBounds(0, &bounds);
        win_w = bounds.w;
        win_h = bounds.h;
        win_x = bounds.x;
        win_y = bounds.y;
    }

    SDL_Window *window = SDL_CreateWindow("Worms", win_x, win_y, win_w, win_h, flags);
    if (window == NULL) {
        SDL_Log("SDL_CreateWindow Error: %s", SDL_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    if (do_fullscreen) {
        SDL_Delay(500);
        SDL_RaiseWindow(window);
        SDL_Delay(100);
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
    if (do_fullscreen) {
        int display = SDL_GetWindowDisplayIndex(window);
        SDL_GetDisplayBounds(display, &bounds);
        W = bounds.w;
        H = bounds.h;
        SDL_RenderSetLogicalSize(renderer, W, H);
    } else {
        SDL_GetRendererOutputSize(renderer, &W, &H);
    }
    SDL_Log("Renderer size: W=%d H=%d", W, H);

    // Create background texture
    SDL_Texture *bg_tex = NULL;
    if (screenshot_surf) {
        bg_tex = SDL_CreateTextureFromSurface(renderer, screenshot_surf);
        // Keep screenshot_surf for color sampling
        if (!bg_tex) {
            SDL_Log("Cannot create texture from screenshot: %s", SDL_GetError());
            SDL_FreeSurface(screenshot_surf);
            screenshot_surf = NULL;
        }
    }

    // Create trails texture (mask: opaque black, worms make transparent)
    SDL_Texture *trails_tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, W, H);
    if (!trails_tex) {
        SDL_Log("Cannot create trails texture: %s", SDL_GetError());
        if (bg_tex) SDL_DestroyTexture(bg_tex);
        if (screenshot_surf) SDL_FreeSurface(screenshot_surf);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    // Initialize mask to transparent (showing screenshot)
    SDL_SetRenderTarget(renderer, trails_tex);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    SDL_SetRenderTarget(renderer, NULL);

    // Initialize worms
    Worm *worms = malloc(worm_count * sizeof(Worm));
    if (!worms) {
        SDL_Log("Cannot allocate worms");
        SDL_DestroyTexture(trails_tex);
        if (bg_tex) SDL_DestroyTexture(bg_tex);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    for (int i = 0; i < worm_count; i++) {
        worms[i].x = W / 2.0f;
        worms[i].y = H / 2.0f;
        float dir = (rand() % 360) * PI / 180.0f;
    float speed = 240.0f; // doubled base speed for more lively motion
    worms[i].vx = cosf(dir) * speed;
    worms[i].vy = sinf(dir) * speed;
        worms[i].color.r = rand() % 256;
        worms[i].color.g = rand() % 256;
        worms[i].color.b = rand() % 256;
        worms[i].color.a = 255;
        worms[i].length = trail_length;
        worms[i].segments = malloc(trail_length * sizeof(SDL_Point));
        if (!worms[i].segments) {
            SDL_Log("Cannot allocate segments for worm %d", i);
            // Cleanup partial
            for (int j = 0; j < i; j++) free(worms[j].segments);
            free(worms);
            SDL_DestroyTexture(trails_tex);
            if (bg_tex) SDL_DestroyTexture(bg_tex);
            if (screenshot_surf) SDL_FreeSurface(screenshot_surf);
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            IMG_Quit();
            SDL_Quit();
            return 1;
        }
        // Initialize trail with slight staggering to avoid overdraw stall
        for (int j = 0; j < trail_length; j++) {
            worms[i].segments[j].x = (int)(worms[i].x + cosf(dir) * j * 0.5f);
            worms[i].segments[j].y = (int)(worms[i].y + sinf(dir) * j * 0.5f);
        }
    }

    // Hide cursor
    system("hyprctl eval 'hl.config({ cursor = { invisible = true } })' >/dev/null 2>&1 || hyprctl keyword cursor:invisible true >/dev/null 2>&1");

    // Main loop
    SDL_Event e;
    int quit = 0;
    Uint32 start_time = SDL_GetTicks();
    BeforelightInput bl_input = {0};
    Uint32 last_ticks = start_time;

    // Pre-render base glyph surfaces (we'll modulate color later for body)
    SDL_Texture *head_tex = NULL;
    SDL_Surface *head_surf = TTF_RenderText_Solid(font, "O", (SDL_Color){255,255,255,255});
    if (head_surf) { head_tex = SDL_CreateTextureFromSurface(renderer, head_surf); SDL_FreeSurface(head_surf); }
    SDL_Surface *body_surf = TTF_RenderText_Solid(font, "-", (SDL_Color){255,255,255,255});
    SDL_Texture *body_tex_base = NULL;
    if (body_surf) { body_tex_base = SDL_CreateTextureFromSurface(renderer, body_surf); SDL_FreeSurface(body_surf); }

    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT || e.type == SDL_KEYDOWN || e.type == SDL_MOUSEBUTTONDOWN) {
                SDL_Log("Screensaver quit triggered: event type %d", e.type);
                quit = 1;
            } else if (e.type == SDL_MOUSEMOTION) {
                if (beforelight_should_quit_motion(&bl_input, start_time, &e)) {
                    SDL_Log("Screensaver quit triggered: mouse motion after grace period");
                    quit = 1;
                }
            }
        }

        // Update worms
    Uint32 now = SDL_GetTicks();
    float dt = (now - last_ticks) / 1000.0f;
    if (dt > 0.05f) dt = 0.05f; // clamp large frame gaps
    last_ticks = now;
        for (int i = 0; i < worm_count; i++) {
            Worm *w = &worms[i];
            // Squiggle: small random turn
            float turn = (rand() % 21 - 10) * wiggle;
            float cos_turn = cosf(turn);
            float sin_turn = sinf(turn);
            float new_vx = w->vx * cos_turn - w->vy * sin_turn;
            float new_vy = w->vx * sin_turn + w->vy * cos_turn;
            w->vx = new_vx;
            w->vy = new_vy;
            // Move
            w->x += w->vx * dt * speed_mult;
            w->y += w->vy * dt * speed_mult;
            // Bounce off walls
            if (w->x < 0) {
                w->vx = -w->vx;
                w->x = 0;
            } else if (w->x >= W) {
                w->vx = -w->vx;
                w->x = W - 1;
            }
            if (w->y < 0) {
                w->vy = -w->vy;
                w->y = 0;
            } else if (w->y >= H) {
                w->vy = -w->vy;
                w->y = H - 1;
            }
        }
        // Worm-worm collisions
        for (int i = 0; i < worm_count; i++) {
            for (int j = i + 1; j < worm_count; j++) {
                Worm *w1 = &worms[i];
                Worm *w2 = &worms[j];
                float radius = 10.0f;
                // Head-head collision
                float dx = w2->x - w1->x;
                float dy = w2->y - w1->y;
                float dist = sqrtf(dx*dx + dy*dy);
                if (dist < 2 * radius && dist > 0) {
                    // Separate
                    float overlap = 2 * radius - dist;
                    float nx = dx / dist;
                    float ny = dy / dist;
                    w1->x -= nx * overlap / 2;
                    w1->y -= ny * overlap / 2;
                    w2->x += nx * overlap / 2;
                    w2->y += ny * overlap / 2;
                    // Elastic collision
                    float tx = -ny;
                    float ty = nx;
                    float v1n = w1->vx * nx + w1->vy * ny;
                    float v1t = w1->vx * tx + w1->vy * ty;
                    float v2n = w2->vx * nx + w2->vy * ny;
                    float v2t = w2->vx * tx + w2->vy * ty;
                    w1->vx = v2n * nx + v1t * tx;
                    w1->vy = v2n * ny + v1t * ty;
                    w2->vx = v1n * nx + v2t * tx;
                    w2->vy = v1n * ny + v2t * ty;
                    // Play chomp sound
                    if (audio_enabled && chomp) Mix_PlayChannel(-1, chomp, 0);
                }
                // Head of w1 with tail segments of w2
                for (int k = 1; k < w2->length; k++) {
                    dx = w2->segments[k].x - w1->x;
                    dy = w2->segments[k].y - w1->y;
                    dist = sqrtf(dx*dx + dy*dy);
                    if (dist < radius && dist > 0) {
                        float overlap = radius - dist;
                        float nx = dx / dist;
                        float ny = dy / dist;
                        w1->x -= nx * overlap;
                        w1->y -= ny * overlap;
                        // Reflect velocity
                        float dot = w1->vx * nx + w1->vy * ny;
                        w1->vx -= 2 * dot * nx;
                        w1->vy -= 2 * dot * ny;
                    }
                }
                // Head of w2 with tail segments of w1
                for (int k = 1; k < w1->length; k++) {
                    dx = w1->segments[k].x - w2->x;
                    dy = w1->segments[k].y - w2->y;
                    dist = sqrtf(dx*dx + dy*dy);
                    if (dist < radius && dist > 0) {
                        float overlap = radius - dist;
                        float nx = dx / dist;
                        float ny = dy / dist;
                        w2->x -= nx * overlap;
                        w2->y -= ny * overlap;
                        // Reflect velocity
                        float dot = w2->vx * nx + w2->vy * ny;
                        w2->vx -= 2 * dot * nx;
                        w2->vy -= 2 * dot * ny;
                        // Play chomp sound
                        if (audio_enabled && chomp) Mix_PlayChannel(-1, chomp, 0);
                    }
                }
            }
        }
        // Update segments
        for (int i = 0; i < worm_count; i++) {
            Worm *w = &worms[i];
            for (int j = w->length - 1; j > 0; j--) {
                w->segments[j] = w->segments[j - 1];
            }
            w->segments[0].x = w->x;
            w->segments[0].y = w->y;
        }

        // Draw trails (make black to hide screenshot)
        SDL_SetRenderTarget(renderer, trails_tex);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // opaque black
        for (int i = 0; i < worm_count; i++) {
            Worm *w = &worms[i];
            for (int j = 0; j < w->length - 1; j++) {
                // Thickness tapering: head thick, tail thin
                int thickness = (2 + 6 * (w->length - 1 - j) / (w->length - 1)) * 2; // doubled trail thickness
                for (int t = -thickness / 2; t <= thickness / 2; t++) {
                    SDL_RenderDrawLine(renderer, w->segments[j].x + t, w->segments[j].y,
                                      w->segments[j+1].x + t, w->segments[j+1].y);
                }
            }
        }
        SDL_SetRenderTarget(renderer, NULL);

        // Render
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        if (bg_tex) {
            SDL_RenderCopy(renderer, bg_tex, NULL, NULL);
            SDL_SetTextureBlendMode(trails_tex, SDL_BLENDMODE_BLEND);
            SDL_RenderCopy(renderer, trails_tex, NULL, NULL);
        } else {
            // No screenshot, just black
        }
        // Render worms on top
        float rainbow_time = (now - start_time) / 1000.0f;
        for (int i = 0; i < worm_count; i++) {
            Worm *w = &worms[i];
            int head_w=0, head_h=0;
            if (head_tex) SDL_QueryTexture(head_tex, NULL, NULL, &head_w, &head_h);
            for (int j = 0; j < w->length; j++) {
                SDL_Texture *tex;
                SDL_Rect dst;
                double angle = (j == 0) ? atan2(w->vy, w->vx) * 180.0 / PI : 0.0;
                if (j == 0) {
                    if (!head_tex) continue;
                    dst = (SDL_Rect){w->segments[j].x - head_w/2, w->segments[j].y - head_h/2, head_w, head_h};
                    SDL_RenderCopyEx(renderer, head_tex, NULL, &dst, angle, NULL, SDL_FLIP_NONE);
                } else {
                    if (!body_tex_base) continue;
                    int bw=0,bh=0; SDL_QueryTexture(body_tex_base, NULL, NULL, &bw, &bh);
                    dst = (SDL_Rect){w->segments[j].x - bw/2, w->segments[j].y - bh/2, bw, bh};
                    // Compute hue shifting along worm length and time
                    float hue = fmodf((rainbow_time * 60.0f) + (j * 6.0f) + i * 15.0f, 360.0f);
                    SDL_Color col = hsv_to_rgb(hue, 1.0f, 1.0f);
                    // Apply modulation
                    SDL_SetTextureColorMod(body_tex_base, col.r, col.g, col.b);
                    SDL_RenderCopyEx(renderer, body_tex_base, NULL, &dst, angle, NULL, SDL_FLIP_NONE);
                }
            }
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    // Exit fullscreen on quit to show Waybar immediately
    system("(hyprctl dispatch fullscreen > /dev/null 2>&1)");
    SDL_Delay(200); // Allow Hyprland to process fullscreen exit

    // Restore cursor
    system("hyprctl eval 'hl.config({ cursor = { invisible = false } })' >/dev/null 2>&1 || hyprctl keyword cursor:invisible false >/dev/null 2>&1");

    // Cleanup
    for (int i = 0; i < worm_count; i++) {
        free(worms[i].segments);
    }
    free(worms);
    SDL_DestroyTexture(trails_tex);
    if (bg_tex) SDL_DestroyTexture(bg_tex);
    if (screenshot_surf) SDL_FreeSurface(screenshot_surf);
    if (audio_enabled) {
        if (chomp) Mix_FreeChunk(chomp);
        Mix_CloseAudio();
    }
    if (head_tex) SDL_DestroyTexture(head_tex);
    if (body_tex_base) SDL_DestroyTexture(body_tex_base);
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}