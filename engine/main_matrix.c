#include <SDL2/SDL.h>
#include "beforelight_input.h"
#include <SDL2/SDL_ttf.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h> // for getopt

extern char *optarg;

static void usage(const char *prog) {
    fprintf(stderr, "Usage: %s [options]\n", prog);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -s F    Speed multiplier (default: 1.0)\n");
    fprintf(stderr, "  -f 0|1  Fullscreen (1=yes, 0=windowed) (default: 1)\n");
    fprintf(stderr, "  -h      Show this help\n");
}

#define MAX_STREAMS 200
#define MAX_CHARS_PER_STREAM 35
#define FONT_SIZE 12

// Matrix character set - mix of katakana and ASCII symbols
const char *matrix_chars =
// Japanese Katakana and Hiragana symbols commonly used in Matrix effect
"アイウエオカキクケコサシスセソタチツテトナニヌネノ"
"ハヒフヘホマミムメモヤユヨラリルレロワヲン"
// ASCII symbols and digits
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
"0123456789@#$%^&*()-+=[]{}|;:,.<>?";

typedef struct {
    int column_x;               // X position of the stream
    float y_offset;             // Current Y offset
    float speed;                // Fall speed (pixels per frame)
    char chars[MAX_CHARS_PER_STREAM];           // Character sequence
    unsigned char brightness[MAX_CHARS_PER_STREAM];  // Brightness (0-255)
    int length;                 // Current length of trail
    int active;                 // Is this stream active
} MatrixStream;

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

    if (TTF_Init() == -1) {
        SDL_Log("TTF_Init Error: %s", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    // Create window
    SDL_Window *window = SDL_CreateWindow("Code Rain", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS | SDL_WINDOW_RESIZABLE);
    if (window == NULL) {
        SDL_Log("SDL_CreateWindow Error: %s", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    beforelight_grab_cursor();

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL) {
        SDL_Log("SDL_CreateRenderer Error: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
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

    // Load font
    TTF_Font *font = NULL;
    const char *font_paths[] = {
        "/usr/share/fonts/noto/NotoSans-Regular.ttf",
        "/usr/share/fonts/noto/NotoSansMono-Regular.ttf",
        "/usr/share/fonts/TTF/CaskaydiaMonoNerdFont-Regular.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationMono-Bold.ttf",
        "/usr/share/fonts/truetype/freefont/FreeMonoBold.ttf",
        "/usr/share/fonts/truetype/ttf-dejavu/DejaVuSansMono-Bold.ttf",
        "/usr/share/fonts/TTF/DejaVuSansMono-Bold.ttf",
        "/usr/share/fonts/TTF/FreeMonoBold.ttf"
    };
    for (size_t i = 0; i < sizeof(font_paths) / sizeof(char*); i++) {
        font = TTF_OpenFont(font_paths[i], FONT_SIZE);
        if (font) break;
    }
    if (!font) {
        SDL_Log("Error: Could not load a monospace font. Install SDL_ttf compatible fonts.");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Calculate character dimensions
    int char_width, char_height;
    TTF_SizeText(font, "0", &char_width, &char_height);

    // Calculate exact number of columns needed to cover screen without gaps
    // Use ceiling division: (W + char_width - 1) / char_width
    int num_columns = (W + char_width - 1) / char_width;
    if (num_columns > MAX_STREAMS) num_columns = MAX_STREAMS;

    // Initialize streams - create one for each available column to maximize coverage
    MatrixStream streams[MAX_STREAMS];
    for (int i = 0; i < MAX_STREAMS; i++) {
        streams[i].column_x = -1;  // Mark as inactive initially
        streams[i].y_offset = 0;
        streams[i].speed = 0;
        streams[i].length = 0;
        streams[i].active = 0;
    }

    // Create initial streams - try to fill most columns immediately
    int streams_to_create = MAX_STREAMS;  // Use all available streams
    if (streams_to_create > num_columns) streams_to_create = num_columns;

    for (int i = 0; i < streams_to_create; i++) {
        streams[i].column_x = i * char_width;  // Distribute evenly across screen
        streams[i].y_offset = -(rand() % (H * 2));  // Scatter starting positions more
        streams[i].speed = 0.5f + (rand() % 8) / 2.0f;  // Speed 0.5-4.0
        streams[i].active = 1;

        // Initialize characters and brightness
        streams[i].length = 18 + rand() % 17;  // 18-35 characters
        for (int c = 0; c < streams[i].length; c++) {
            streams[i].chars[c] = matrix_chars[rand() % strlen(matrix_chars)];
            streams[i].brightness[c] = (unsigned char)(40 + rand() % 215);  // 40-255
        }
        // Make the lead character brightest
        streams[i].brightness[0] = 255;
    }


    // Main loop
    SDL_Event e;
    int quit = 0;
    Uint32 last_time = SDL_GetTicks();
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
        float dt = (current_time - last_time) / 16.666f;  // Normalized to 60fps
        last_time = current_time;

        // Clear screen with black
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // For full coverage, spawn streams densely across the screen area
        // Each inactive stream gets a random position within bounds
        int active_count = 0;
        for (int i = 0; i < MAX_STREAMS; i++) {
            if (streams[i].active) active_count++;
        }

        // Maintain maximum streams for blanket coverage
        while (active_count < MAX_STREAMS - 10) {  // Keep 190+ active streams
            for (int i = 0; i < MAX_STREAMS; i++) {
                if (!streams[i].active) {
                    streams[i].column_x = rand() % (W + 100);  // Overshoot screen edges
                    streams[i].y_offset = -(rand() % (H / 4));
                    streams[i].speed = 0.5f + (rand() % 20) / 4.0f;  // Speed 0.5-5.5
                    streams[i].active = 1;

                    streams[i].length = 15 + rand() % 20;
                    for (int c = 0; c < streams[i].length; c++) {
                        streams[i].chars[c] = matrix_chars[rand() % strlen(matrix_chars)];
                        streams[i].brightness[c] = (unsigned char)(30 + rand() % 225);
                    }
                    streams[i].brightness[0] = 255;
                    active_count++;
                    break;
                }
            }
        }

        // Update and render all active streams
    for (int i = 0; i < MAX_STREAMS; i++) {
        if (!streams[i].active) continue;

            // Update stream position
            streams[i].y_offset += streams[i].speed * speed_mult * dt;

            // Update brightness for fade effect
            for (int c = streams[i].length - 1; c >= 1; c--) {
                // Fade trailing characters
                if (streams[i].brightness[c] > 10) {
                    streams[i].brightness[c] -= (unsigned char)(dt * 5 * speed_mult);  // Fade rate
                }
            }

            // Add some random brightening effects
            if (rand() % 200 < 3) {  // Rare brightening
                int random_char = rand() % streams[i].length;
                streams[i].brightness[random_char] = 255;
            }

            // Render the stream
            for (int c = 0; c < streams[i].length; c++) {
                float char_y = streams[i].y_offset - (c * char_height);

                // Skip characters that are off-screen
                if (char_y < -char_height || char_y > H) continue;

                // Create text surface for this character
                char char_str[2] = {streams[i].chars[c], '\0'};
                int alpha = streams[i].brightness[c];

                SDL_Color green = {0, 255, 0, (Uint8)alpha};  // Lime green with alpha

                SDL_Surface *text_surf = TTF_RenderText_Blended(font, char_str, green);
                if (text_surf) {
                    SDL_Texture *text_tex = SDL_CreateTextureFromSurface(renderer, text_surf);
                    SDL_FreeSurface(text_surf);

                    if (text_tex) {
                        SDL_SetTextureAlphaMod(text_tex, alpha);
                        SDL_Rect dst_rect = {
                            streams[i].column_x,
                            (int)char_y,
                            text_surf->w,
                            text_surf->h
                        };
                        SDL_RenderCopy(renderer, text_tex, NULL, &dst_rect);
                        SDL_DestroyTexture(text_tex);
                    }
                }
            }

            // Remove stream when it goes off screen
            if (streams[i].y_offset > H + streams[i].length * char_height) {
                streams[i].active = 0;
            }
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60fps
    }


    // Cleanup
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
