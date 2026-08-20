#include <SDL.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h> // for getopt

extern char *optarg;

#define PI 3.14159f

static void usage(const char *prog) {
    fprintf(stderr, "Usage: %s [options]\n", prog);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -s F    Speed multiplier (default: 1.0)\n");
    fprintf(stderr, "  -f 0|1  Fullscreen (1=yes, 0=windowed) (default: 1)\n");
    fprintf(stderr, "  -h      Show this help\n");
}

typedef struct {
    float x, y;
} Point;

typedef struct {
    int v1, v2; // vertex indices that connect
} Edge;

// Bear constellation
const Point bear_vertices[] = {
    {0, 0},      // 0: head bottom
    {-30, -40},  // 1: head top
    {30, -40},   // 2: head right
    {-60, 20},   // 3: left ear
    {60, 20},    // 4: right ear
    {-80, 60},   // 5: left paw front
    {80, 60},    // 6: right paw front
    {-70, 120},  // 7: left paw back
    {70, 120},   // 8: right paw back
    {-40, 80},   // 9: left front leg
    {40, 80},    // 10: right front leg
};

const Edge bear_edges[] = {
    {0, 1}, {0, 2}, {0, 3}, {0, 4},  // head connections
    {0, 5}, {0, 6}, {0, 7}, {0, 8},  // body connections
    {5, 9}, {6, 10}, {7, 9}, {8, 10}, // leg connections
    {3, 5}, {4, 6}, // ear to shoulder connections
};

// Fish constellation
const Point fish_vertices[] = {
    {0, 0},      // 0: head
    {40, -30},   // 1: top fin
    {40, 30},    // 2: bottom fin
    {80, 0},     // 3: tail base
    {120, -20},  // 4: tail top
    {120, 20},   // 5: tail bottom
    {50, -10},   // 6: dorsal fin top
    {50, 10},    // 7: ventral fin
    {-20, -15},  // 8: eye
};

const Edge fish_edges[] = {
    {0, 1}, {0, 2}, {0, 3}, {0, 6}, {0, 7}, // head fins
    {3, 4}, {3, 5}, {1, 6}, {2, 7}, // body fins
    {0, 8}, // eye
};

// Bird constellation
const Point bird_vertices[] = {
    {0, 0},      // 0: head
    {20, -20},   // 1: beak
    {-30, -40},  // 2: left wing top
    {-10, 10},   // 3: left wing bottom
    {30, -40},   // 4: right wing top
    {10, 10},    // 5: right wing bottom
    {-20, 20},   // 6: left tail
    {20, 20},    // 7: right tail
    {0, 30},     // 8: tail center
};

const Edge bird_edges[] = {
    {0, 1}, {0, 2}, {0, 3}, {0, 4}, {0, 5}, // head and wings
    {0, 6}, {0, 7}, {0, 8}, // tail
    {2, 3}, {4, 5}, {6, 7}, {6, 8}, {7, 8}, // wing and tail connections
};

// Human figure constellation
const Point human_vertices[] = {
    {0, -60},   // 0: head
    {0, 0},     // 1: torso
    {30, -30},  // 2: left arm
    {-30, -30}, // 3: right arm
    {20, 60},   // 4: left leg
    {-20, 60},  // 5: right leg
};

const Edge human_edges[] = {
    {0, 1}, {1, 2}, {1, 3}, {1, 4}, {1, 5}, // body connections
};

//// DNA double helix constellation
const Point dna_vertices[] = {
    // Left helix
    {-40, -60}, {-35, -40}, {-30, -20}, {-25, 0}, {-20, 20}, {-15, 40}, {-10, 60}, {-5, 80},
    // Right helix
    {40, -60}, {35, -40}, {30, -20}, {25, 0}, {20, 20}, {15, 40}, {10, 60}, {5, 80},
    // Base pairs (cross links)
    {-20, -45}, {20, -45}, // A-T
    {-15, -25}, {15, -25}, // C-G
    {-10, -5}, {10, -5},   // G-C
    {-5, 15}, {5, 15},     // T-A
    {0, 35}, {0, 55},      // Center twist
};

const Edge dna_edges[] = {
    // Left helix backbone
    {0,1}, {1,2}, {2,3}, {3,4}, {4,5}, {5,6}, {6,7},
    // Right helix backbone
    {8,9}, {9,10}, {10,11}, {11,12}, {12,13}, {13,14}, {14,15},
    // Base pair connections
    {7,12}, {6,11}, {5,10}, {4,9}, {3,8}, // horizontal pairs (odd pattern)
    // Additional DNA-like twists
    {16,17}, {18,19}, {20,21}, {22,23}, // base pair centers
    {24,25}, // central twist
};

// Dragon constellation
const Point dragon_vertices[] = {
    {0, -40},   // 0: head
    {20, -50},  // 1: mouth
    {40, -20},  // 2: neck
    {60, 0},    // 3: body
    {80, -10},  // 4: tail base
    {100, -20}, // 5: tail
    {20, 20},   // 6: left wing top
    {40, 40},   // 7: left wing
    {-20, 20},  // 8: right wing top
    {-40, 40},  // 9: right wing
    {30, -70},  // 10: horn
};

const Edge dragon_edges[] = {
    {0,1}, {0,2}, {2,3}, {3,4}, {4,5}, // body
    {0,6}, {6,7}, {0,8}, {8,9}, // wings
    {0,10}, // horn
};

// Flower constellation
const Point flower_vertices[] = {
    {0, 0},     // 0: center
    {0, -30},   // 1: top petal
    {30, 0},    // 2: right petal
    {0, 30},    // 3: bottom petal
    {-30, 0},   // 4: left petal
    {20, -20},  // 5: diagonal
    {-20, -20}, // 6: diagonal
    {-20, 20},  // 7: diagonal
    {20, 20},   // 8: diagonal
};

const Edge flower_edges[] = {
    {0,1}, {0,2}, {0,3}, {0,4}, // petals
    {1,5}, {5,2}, {2,8}, {8,3}, {3,7}, {7,4}, {4,6}, {6,1}, // outer ring
};

// Star constellation (5-point star)
const Point star_vertices[] = {
    {0, -50},   // 0: top
    {20, -15},  // 1: top right
    {50, -15},  // 2: right
    {30, 15},   // 3: bottom right
    {35, 50},   // 4: bottom
    {0, 30},    // 5: bottom left inner
    {-35, 50},  // 6: bottom left
    {-30, 15},  // 7: left bottom
    {-50, -15}, // 8: left
    {-20, -15}, // 9: top left
};

const Edge star_edges[] = {
    {0,2}, {2,4}, {4,6}, {6,8}, {8,0}, // outer points
    {0,5}, {2,7}, {4,9}, {6,1}, {8,3}, // inner connections
};

// Heart constellation
const Point heart_vertices[] = {
    {0, -30},   // 0: top
    {20, -20},  // 1: top curve left
    {30, 0},    // 2: left bulge
    {20, 30},   // 3: bottom left
    {0, 40},    // 4: bottom
    {-20, 30},  // 5: bottom right
    {-30, 0},   // 6: right bulge
    {-20, -20}, // 7: top curve right
};

const Edge heart_edges[] = {
    {0,1}, {1,2}, {2,3}, {3,4}, {4,5}, {5,6}, {6,7}, {7,0}, // outline
    {1,4}, {2,5}, {3,6}, // crosses
};

// Octopus constellation
const Point octopus_vertices[] = {
    {0, 0},      // 0: body
    {-20, -30},  // 1: left tentacle
    {20, -30},   // 2: right tentacle
    {-30, 10},   // 3: bottom left
    {30, 10},    // 4: bottom right
    {-10, -50},  // 5: left end
    {10, -50},   // 6: right end
    {0, -20},    // 7: top curve
};

const Edge octopus_edges[] = {
    {0,1}, {1,5}, {0,2}, {2,6}, // top tentacles
    {0,3}, {0,4}, // bottom tentacles
    {0,7}, // body line
};

// Tree constellation
const Point tree_vertices[] = {
    {0, 50},     // 0: trunk bottom
    {0, 0},      // 1: trunk top
    {-20, -20},  // 2: left branch
    {20, -20},   // 3: right branch
    {-30, -40},  // 4: left leaf
    {30, -40},   // 5: right leaf
    {0, -60},    // 6: top leaf
};

const Edge tree_edges[] = {
    {0,1}, // trunk
    {1,2}, {1,3}, // branches
    {2,4}, {3,5}, {1,6}, // leaves
};

// Butterfly constellation
const Point butterfly_vertices[] = {
    {0, 0},      // 0: body center
    {10, -20},   // 1: upper wing left
    {30, -10},   // 2: upper wing left tip
    {-10, -20},  // 3: upper wing right
    {-30, -10},  // 4: upper wing right tip
    {20, 20},    // 5: lower wing left
    {40, 30},    // 6: lower wing left tip
    {-20, 20},   // 7: lower wing right
    {-40, 30},   // 8: lower wing right tip
};

const Edge butterfly_edges[] = {
    {0,1}, {1,2}, {0,3}, {3,4}, // upper wings
    {0,5}, {5,6}, {0,7}, {7,8}, // lower wings
};

// Spaceship constellation
const Point spaceship_vertices[] = {
    {0, -20},    // 0: nose
    {20, 0},     // 1: right wing
    {-20, 0},    // 2: left wing
    {0, 20},     // 3: engine
    {10, 5},     // 4: right engine
    {-10, 5},    // 5: left engine
    {30, -10},   // 6: right thruster
    {-30, -10},  // 7: left thruster
};

const Edge spaceship_edges[] = {
    {0,1}, {0,2}, {0,3}, // main body
    {3,4}, {3,5}, // engines
    {1,6}, {2,7}, // thrusters
};

// Alien face constellation
const Point alien_vertices[] = {
    {0, -30},    // 0: head top
    {20, -10},   // 1: right ear
    {-20, -10},  // 2: left ear
    {15, 10},    // 3: right eye
    {-15, 10},   // 4: left eye
    {0, 30},     // 5: mouth
    {10, 20},    // 6: right antenna
    {-10, 20},   // 7: left antenna
};

const Edge alien_edges[] = {
    {0,1}, {0,2}, // ears
    {1,3}, {2,4}, {3,5}, {4,5}, // face
    {0,6}, {0,7}, // antennae
};

// Crystal constellation
const Point crystal_vertices[] = {
    {0, -40},    // 0: top
    {15, -10},   // 1: right top
    {-15, -10},  // 2: left top
    {20, 10},    // 3: right middle
    {-20, 10},   // 4: left middle
    {0, 40},     // 5: bottom
};

const Edge crystal_edges[] = {
    {0,1}, {0,2}, {0,3}, {0,4}, {0,5}, // from top
    {1,3}, {2,4}, {3,5}, {4,5}, // facets
};

// Unicorn constellation
const Point unicorn_vertices[] = {
    {0, -30},   // 0: head top
    {15, -20},  // 1: right ear
    {-15, -20}, // 2: left ear
    {0, 20},    // 3: body
    {25, 30},   // 4: right leg
    {-25, 30},  // 5: left leg
    {30, -40},  // 6: horn
};

const Edge unicorn_edges[] = {
    {0,1}, {0,2}, {0,3}, // head and ears to body
    {3,4}, {3,5}, {0,6}, // legs and horn
};

// Pegasus constellation (winged horse)
const Point pegasus_vertices[] = {
    {0, -20},   // 0: head
    {20, -10},  // 1: right wing upper
    {30, 10},   // 2: right wing lower
    {-20, -10}, // 3: left wing upper
    {-30, 10},  // 4: left wing lower
    {0, 30},    // 5: body
    {15, 40},   // 6: right leg
    {-15, 40},  // 7: left leg
};

const Edge pegasus_edges[] = {
    {0,1}, {1,2}, {0,3}, {3,4}, // wings
    {0,5}, {5,6}, {5,7}, // body and legs
};

// Horse constellation
const Point horse_vertices[] = {
    {0, -20},   // 0: head
    {20, 0},    // 1: neck
    {15, 40},   // 2: right leg
    {-15, 40},  // 3: left leg
    {15, 20},   // 4: mane
    {-15, 20},  // 5: mane
};

const Edge horse_edges[] = {
    {0,1}, {1,2}, {1,3}, {0,4}, {0,5}, // basic shape
};

typedef struct {
    const Point *vertices;
    const Edge *edges;
    int num_vertices;
    int num_edges;
    SDL_Color line_color;
    SDL_Color star_color;
    SDL_Color bg_color;
} Constellation;

const Constellation constellations[] = {
    {bear_vertices, bear_edges, sizeof(bear_vertices)/sizeof(Point), sizeof(bear_edges)/sizeof(Edge), {255, 128, 0, 255}, {255, 255, 0, 255}, {15, 10, 5, 255}},   // Golden bear with yellow stars
    {fish_vertices, fish_edges, sizeof(fish_vertices)/sizeof(Point), sizeof(fish_edges)/sizeof(Edge), {0, 255, 255, 255}, {255, 0, 255, 255}, {0, 15, 15, 255}},   // Cyan fish with magenta stars
    {bird_vertices, bird_edges, sizeof(bird_vertices)/sizeof(Point), sizeof(bird_edges)/sizeof(Edge), {255, 0, 255, 255}, {0, 255, 0, 255}, {15, 0, 15, 255}},   // Magenta bird with green stars
    {human_vertices, human_edges, sizeof(human_vertices)/sizeof(Point), sizeof(human_edges)/sizeof(Edge), {255, 0, 128, 255}, {128, 255, 255, 255}, {15, 0, 10, 255}}, // Pink human with turquoise stars
    {dna_vertices, dna_edges, sizeof(dna_vertices)/sizeof(Point), sizeof(dna_edges)/sizeof(Edge), {128, 0, 255, 255}, {255, 128, 0, 255}, {10, 0, 20, 255}}, // Deep purple DNA with orange stars
    {dragon_vertices, dragon_edges, sizeof(dragon_vertices)/sizeof(Point), sizeof(dragon_edges)/sizeof(Edge), {200, 100, 0, 255}, {255, 200, 0, 255}, {20, 5, 0, 255}},   // Fire dragon with flame colors
    {flower_vertices, flower_edges, sizeof(flower_vertices)/sizeof(Point), sizeof(flower_edges)/sizeof(Edge), {255, 192, 203, 255}, {255, 0, 128, 255}, {5, 15, 10, 255}}, // Pink flower with hot pink stars
    {star_vertices, star_edges, sizeof(star_vertices)/sizeof(Point), sizeof(star_edges)/sizeof(Edge), {255, 215, 0, 255}, {255, 255, 255, 255}, {5, 5, 20, 255}},   // Gold star with white stars
    {heart_vertices, heart_edges, sizeof(heart_vertices)/sizeof(Point), sizeof(heart_edges)/sizeof(Edge), {255, 105, 180, 255}, {255, 20, 147, 255}, {20, 0, 10, 255}}, // Hot pink heart with deep pink stars
    {octopus_vertices, octopus_edges, sizeof(octopus_vertices)/sizeof(Point), sizeof(octopus_edges)/sizeof(Edge), {147, 112, 219, 255}, {138, 43, 226, 255}, {10, 5, 15, 255}}, // Medium purple octopus with blue violet stars
    {tree_vertices, tree_edges, sizeof(tree_vertices)/sizeof(Point), sizeof(tree_edges)/sizeof(Edge), {34, 139, 34, 255}, {50, 205, 50, 255}, {5, 10, 5, 255}},   // Forest green tree with lime green stars
    {butterfly_vertices, butterfly_edges, sizeof(butterfly_vertices)/sizeof(Point), sizeof(butterfly_edges)/sizeof(Edge), {255, 0, 255, 255}, {255, 20, 147, 255}, {15, 0, 15, 255}}, // Magenta butterfly with deep pink stars
    {spaceship_vertices, spaceship_edges, sizeof(spaceship_vertices)/sizeof(Point), sizeof(spaceship_edges)/sizeof(Edge), {0, 191, 255, 255}, {135, 206, 250, 255}, {0, 10, 20, 255}}, // Deep sky blue spaceship with light sky blue stars
    {alien_vertices, alien_edges, sizeof(alien_vertices)/sizeof(Point), sizeof(alien_edges)/sizeof(Edge), {60, 179, 113, 255}, {152, 251, 152, 255}, {5, 10, 5, 255}}, // Medium sea green alien with pale green stars
    {crystal_vertices, crystal_edges, sizeof(crystal_vertices)/sizeof(Point), sizeof(crystal_edges)/sizeof(Edge), {176, 196, 222, 255}, {255, 250, 250, 255}, {10, 10, 20, 255}}, // Light steel blue crystal with snow stars
    {dragon_vertices, dragon_edges, sizeof(dragon_vertices)/sizeof(Point), sizeof(dragon_edges)/sizeof(Edge), {255, 20, 147, 255}, {255, 215, 0, 255}, {25, 0, 10, 255}},   // Deep pink dragon
    {flower_vertices, flower_edges, sizeof(flower_vertices)/sizeof(Point), sizeof(flower_edges)/sizeof(Edge), {0, 255, 127, 255}, {255, 165, 0, 255}, {0, 20, 10, 255}}, // Spring green flower
    {star_vertices, star_edges, sizeof(star_vertices)/sizeof(Point), sizeof(star_edges)/sizeof(Edge), {138, 43, 226, 255}, {255, 255, 255, 255}, {10, 0, 15, 255}},   // Blue violet star
    {heart_vertices, heart_edges, sizeof(heart_vertices)/sizeof(Point), sizeof(heart_edges)/sizeof(Edge), {255, 255, 0, 255}, {255, 140, 0, 255}, {20, 15, 0, 255}},  // Yellow heart
    {octopus_vertices, octopus_edges, sizeof(octopus_vertices)/sizeof(Point), sizeof(octopus_edges)/sizeof(Edge), {0, 255, 0, 255}, {255, 0, 0, 255}, {0, 20, 0, 255}},     // Lime octopus
    {tree_vertices, tree_edges, sizeof(tree_vertices)/sizeof(Point), sizeof(tree_edges)/sizeof(Edge), {255, 192, 203, 255}, {255, 105, 180, 255}, {15, 5, 10, 255}},   // Pink tree
    {butterfly_vertices, butterfly_edges, sizeof(butterfly_vertices)/sizeof(Point), sizeof(butterfly_edges)/sizeof(Edge), {255, 255, 255, 255}, {255, 0, 255, 255}, {20, 20, 20, 255}}, // White butterfly
    {spaceship_vertices, spaceship_edges, sizeof(spaceship_vertices)/sizeof(Point), sizeof(spaceship_edges)/sizeof(Edge), {255, 0, 0, 255}, {255, 255, 0, 255}, {15, 0, 0, 255}},  // Red spaceship
    {alien_vertices, alien_edges, sizeof(alien_vertices)/sizeof(Point), sizeof(alien_edges)/sizeof(Edge), {0, 255, 255, 255}, {0, 0, 255, 255}, {0, 15, 15, 255}},     // Cyan alien
    {flower_vertices, flower_edges, sizeof(flower_vertices)/sizeof(Point), sizeof(flower_edges)/sizeof(Edge), {255, 0, 255, 255}, {128, 0, 128, 255}, {15, 0, 15, 255}}, // Purple flower
    {star_vertices, star_edges, sizeof(star_vertices)/sizeof(Point), sizeof(star_edges)/sizeof(Edge), {255, 69, 0, 255}, {255, 215, 0, 255}, {10, 5, 0, 255}},      // Red orange star
    {heart_vertices, heart_edges, sizeof(heart_vertices)/sizeof(Point), sizeof(heart_edges)/sizeof(Edge), {0, 206, 209, 255}, {255, 20, 147, 255}, {0, 15, 15, 255}},  // Turquoise heart
    {crystal_vertices, crystal_edges, sizeof(crystal_vertices)/sizeof(Point), sizeof(crystal_edges)/sizeof(Edge), {255, 0, 255, 255}, {0, 255, 255, 255}, {15, 0, 20, 255}}, // Magenta crystal
    {dna_vertices, dna_edges, sizeof(dna_vertices)/sizeof(Point), sizeof(dna_edges)/sizeof(Edge), {255, 20, 147, 255}, {255, 255, 0, 255}, {20, 0, 10, 255}},     // Deep pink DNA
};

typedef struct {
    Point pos;         // current position
    Point target;      // target position
    float connect_progress; // 0-1 how connected line is
    int is_active;     // star is visible
} Star;

const int NUM_CONSTELLATIONS = sizeof(constellations) / sizeof(Constellation);

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

    SDL_Window *window = SDL_CreateWindow("Life Forms", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS | SDL_WINDOW_RESIZABLE);
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

    // Animation phases
    enum Phase {PHASE_SCATTER, PHASE_CONNECT, PHASE_HOLD, PHASE_DISSOLVE};
    int active_phases[3];
    float active_timers[3];
    int active_num_stars[3];
    Star active_stars[3][32];  // 3 groups of stars
    int active_indices[3];     // indices in constellations array
    float active_rotations[3];  // rotation for each group

    // Start with first 3 constellations in sequence
    int global_cycle_start = 0;
    for (int i = 0; i < 3; i++) {
        active_indices[i] = global_cycle_start + i;
        active_phases[i] = PHASE_SCATTER;
        active_timers[i] = 0;
        active_num_stars[i] = 0;
        active_rotations[i] = (rand() % 360) * PI / 180.0f; // random rotation in radians
    }

    // Offsets for 3 constellations to avoid overlap and stay on screen - now vertical positions
    int x_offsets[3] = {0, 0, 0};  // centered horizontally
    int y_offsets[3] = {-H/6, 0, H/6};  // positioned vertically

    // Create galaxy background stars - higher resolution
    #define GALAXY_STARS 1200
    #define TWINKLE_STARS (GALAXY_STARS / 2)  // 50% twinkle more frequently
    typedef struct {
        int x, y;
        Uint8 brightness;
        int is_twinkle;
        float twinkle_phase;
    } GalaxyStar;
    GalaxyStar galaxy_stars[GALAXY_STARS];
    for (int i = 0; i < GALAXY_STARS; i++) {
        galaxy_stars[i].x = rand() % W;
        galaxy_stars[i].y = rand() % H;
        galaxy_stars[i].brightness = (rand() % 100) + 50; // 50-149 brightness
        galaxy_stars[i].is_twinkle = (i < TWINKLE_STARS); // first 50 twinkle
        galaxy_stars[i].twinkle_phase = rand() % 360; // random start phase
    }

    // Main loop
    SDL_Event e;
    int quit = 0;

    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT || e.type == SDL_KEYDOWN || e.type == SDL_MOUSEBUTTONDOWN) {
                quit = 1;
            }
        }

        // Check if current group is all dissolved and advance to next group
        int all_dissolved = 1;
        for (int c = 0; c < 3; c++) {
            if (active_phases[c] != PHASE_DISSOLVE || active_timers[c] < 3.0f * 2) {
                all_dissolved = 0;
                break;
            }
        }
        if (all_dissolved) {
            // Randomly select 3 unique constellations without replacement
            int selected[3];
            int used_indices[30] = {0}; // track used
            for (int i = 0; i < 3; i++) {
                int index;
                do {
                    index = rand() % NUM_CONSTELLATIONS;
                } while (used_indices[index]);
                used_indices[index] = 1;
                selected[i] = index;
            }
            for (int i = 0; i < 3; i++) {
                active_indices[i] = selected[i];
                active_phases[i] = PHASE_SCATTER;
                active_timers[i] = 0;
                active_num_stars[i] = 0;
            }

            // Randomize rotations only, use guaranteed positions that never cut off
            for (int i = 0; i < 3; i++) {
                active_rotations[i] = (rand() % 360) * PI / 180.0f;
            }
            // Dynamic random placement with collision detection
            for (int attempts = 0; attempts < 200; attempts++) {
                bool placement_ok = true;

                // Random positions
                int x_candidates[3], y_candidates[3];
                int margin = 50;
                for (int c = 0; c < 3; c++) {
                    x_candidates[c] = rand() % (W - 2*margin) + margin;
                    y_candidates[c] = rand() % (H - 2*margin) + margin;
                }

                // Check bounding boxes for overlap
                for (int c = 0; c < 3 && placement_ok; c++) {
                    const Constellation *cons = &constellations[active_indices[c]];
                    float bb_left = INFINITY, bb_right = -INFINITY, bb_top = INFINITY, bb_bottom = -INFINITY;

                    // Calculate rotated bounding box relative to candidate position as center
                    for (int i = 0; i < cons->num_vertices; i++) {
                        float vx = cons->vertices[i].x * 1.0f;
                        float vy = cons->vertices[i].y * 1.0f;
                        float rot_x = vx * cos(active_rotations[c]) - vy * sin(active_rotations[c]);
                        float rot_y = vx * sin(active_rotations[c]) + vy * cos(active_rotations[c]);
                        if (rot_x < bb_left) bb_left = rot_x;
                        if (rot_x > bb_right) bb_right = rot_x;
                        if (rot_y < bb_top) bb_top = rot_y;
                        if (rot_y > bb_bottom) bb_bottom = rot_y;
                    }

                    // Check screen bounds: constellation center is at candidate, bb is relative
                    // So edges should be at least margin from screen edges
                    if (x_candidates[c] + bb_left < margin || x_candidates[c] + bb_right > W - margin ||
                        y_candidates[c] + bb_top < margin || y_candidates[c] + bb_bottom > H - margin) {
                        placement_ok = false;
                        continue;
                    }

                    // Check overlap with previous constellations
                    for (int other = 0; other < c && placement_ok; other++) {
                        // Calculate other BB
                        const Constellation *other_cons = &constellations[active_indices[other]];
                        float other_bb_left = INFINITY, other_bb_right = -INFINITY, other_bb_top = INFINITY, other_bb_bottom = -INFINITY;
                        for (int j = 0; j < other_cons->num_vertices; j++) {
                            float vx = other_cons->vertices[j].x * 1.0f;
                            float vy = other_cons->vertices[j].y * 1.0f;
                            float rot_x = vx * cos(active_rotations[other]) - vy * sin(active_rotations[other]);
                            float rot_y = vx * sin(active_rotations[other]) + vy * cos(active_rotations[other]);
                            if (rot_x < other_bb_left) other_bb_left = rot_x;
                            if (rot_x > other_bb_right) other_bb_right = rot_x;
                            if (rot_y < other_bb_top) other_bb_top = rot_y;
                            if (rot_y > other_bb_bottom) other_bb_bottom = rot_y;
                        }

                        // Check if BBs overlap
                        if (!(bb_right + x_candidates[c] + 20 < other_bb_left + x_candidates[other] ||  // right of other
                              bb_left + x_candidates[c] - 20 > other_bb_right + x_candidates[other] || // left of other
                              bb_bottom + y_candidates[c] + 20 < other_bb_top + y_candidates[other] ||  // below other
                              bb_top + y_candidates[c] - 20 > other_bb_bottom + y_candidates[other])) { // above other
                            placement_ok = false;
                        }
                    }
                }

                if (placement_ok) {
                    // Set offsets as center positions (since W/2 + offset will center it)
                    for (int c = 0; c < 3; c++) {
                        x_offsets[c] = 0;  // we use center + random shift, but to place anywhere, offset from center
                        y_offsets[c] = 0;
                        // Actually, since x = W/2 + 0 + random_pos, to place at random_pos, offset = random_pos - W/2
                        // Wait, no: x = W/2 + offset
                        // But for stars x = W/2 + offset + pos.x, pos.x is the transformed points
                        // To have the constellation center at a specific spot, offset should be center - W/2
                        // For example, to center at 300, offset = 300 - 400 = -100
                        x_offsets[c] = x_candidates[c] - W/2;
                        y_offsets[c] = y_candidates[c] - H/2;
                    }
                    break; // Success!
                }
            }
        }

        // Update all 3 constellations
        for (int c = 0; c < 3; c++) {
            const Constellation *constellation = &constellations[active_indices[c]];
            const float phase_duration = 3.0f; // seconds per phase

            active_timers[c] += 0.016f * speed_mult;

            // Phase logic for this constellation
            if (active_phases[c] == PHASE_SCATTER) {
                // Initialize star positions
                if (active_num_stars[c] == 0) {
                    active_num_stars[c] = constellation->num_vertices;
                    for (int i = 0; i < active_num_stars[c]; i++) {
                        // Scatter stars randomly around the offset
                        active_stars[c][i].pos.x = (rand() % (W/2)) - W/4.0f;
                        active_stars[c][i].pos.y = (rand() % H) - H/2.0f;
                        // Rotate target position
                        float original_x = constellation->vertices[i].x * 1.0f; // scaler reduced to 1.0 for fixed screen
                        float original_y = constellation->vertices[i].y * 1.0f;
                        active_stars[c][i].target.x = original_x * cos(active_rotations[c]) - original_y * sin(active_rotations[c]);
                        active_stars[c][i].target.y = original_x * sin(active_rotations[c]) + original_y * cos(active_rotations[c]);
                        active_stars[c][i].connect_progress = 0;
                        active_stars[c][i].is_active = 1;
                    }
                }

                // Move stars toward constellation positions
                float scatter_progress = active_timers[c] / phase_duration;
                if (scatter_progress > 1) scatter_progress = 1;

                for (int i = 0; i < active_num_stars[c]; i++) {
                    float t = scatter_progress * speed_mult;
                    if (t > 1) t = 1;
                    active_stars[c][i].pos.x += (active_stars[c][i].target.x - active_stars[c][i].pos.x) * t * 0.1f;
                    active_stars[c][i].pos.y += (active_stars[c][i].target.y - active_stars[c][i].pos.y) * t * 0.1f;
                }

                if (active_timers[c] >= phase_duration) {
                    active_phases[c] = PHASE_CONNECT;
                    active_timers[c] = 0;
                }

            } else if (active_phases[c] == PHASE_CONNECT) {
                // Gradually connect stars with lines
                float connect_progress = active_timers[c] / phase_duration;
                for (int i = 0; i < constellation->num_edges; i++) {
                    float edge_progress = connect_progress * constellation->num_edges - i;
                    if (edge_progress < 0) edge_progress = 0;
                    if (edge_progress > 1) edge_progress = 1;

                    active_stars[c][constellation->edges[i].v1].connect_progress = edge_progress;
                }

                if (active_timers[c] >= phase_duration) {
                    active_phases[c] = PHASE_HOLD;
                    active_timers[c] = 0;
                }

            } else if (active_phases[c] == PHASE_HOLD) {
                // Hold the formed constellation
                if (active_timers[c] >= phase_duration * 4) { // hold longer
                    active_phases[c] = PHASE_DISSOLVE;
                    active_timers[c] = 0;
                }

            } else if (active_phases[c] == PHASE_DISSOLVE) {
                // Gradually disconnect stars
                float dissolve_progress = active_timers[c] / phase_duration;
                for (int i = 0; i < constellation->num_edges; i++) {
                    float edge_progress = 1.0f - dissolve_progress * constellation->num_edges + i;
                    if (edge_progress < 0) edge_progress = 0;
                    if (edge_progress > 1) edge_progress = 1;

                    active_stars[c][constellation->edges[i].v1].connect_progress = edge_progress;
                }

                if (dissolve_progress >= constellation->num_edges * 0.1f) {
                    // Scatter stars when nearly dissolved
                    for (int i = 0; i < active_num_stars[c]; i++) {
                        active_stars[c][i].pos.x += (rand() % 150 - 75) * dissolve_progress;
                        active_stars[c][i].pos.y += (rand() % 150 - 75) * dissolve_progress;
                    }
                }

                // Wait for group advancement
            }
        }

        // Rendering - black background
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Draw galaxy background stars - high resolution with frequent twinkling
        for (int i = 0; i < GALAXY_STARS; i++) {
            int brightness = galaxy_stars[i].brightness;
            if (galaxy_stars[i].is_twinkle) {
                // Twinkle effect: modulate brightness with sine wave
                galaxy_stars[i].twinkle_phase += 0.2f; // faster twinkling
                float twinkle = sin(galaxy_stars[i].twinkle_phase) * 0.6f + 0.5f; // 0.2-0.8 range
                brightness = galaxy_stars[i].brightness * (0.4f + twinkle * 0.6f); // vary from 40% to 100%
                if (brightness > 255) brightness = 255;
                if (brightness < 0) brightness = 0;
            }
            SDL_SetRenderDrawColor(renderer, brightness, brightness, brightness, 255);
            SDL_RenderDrawPoint(renderer, galaxy_stars[i].x, galaxy_stars[i].y);
        }

        // Draw each constellation
        for (int c = 0; c < 3; c++) {
            const Constellation *constellation = &constellations[active_indices[c]];
            int offset_x = x_offsets[c];
            int offset_y = y_offsets[c];

            // Draw connections
            SDL_SetRenderDrawColor(renderer, constellation->line_color.r, constellation->line_color.g, constellation->line_color.b, constellation->line_color.a);
            for (int i = 0; i < constellation->num_edges; i++) {
                const Edge *edge = &constellation->edges[i];
                if (edge->v1 >= active_num_stars[c] || edge->v2 >= active_num_stars[c]) continue;

                float line_progress = active_stars[c][edge->v1].connect_progress;
                if (line_progress > 0) {
                    Star *s1 = &active_stars[c][edge->v1];
                    Star *s2 = &active_stars[c][edge->v2];

                    int x1 = W/2 + offset_x + (int)s1->pos.x;
                    int y1 = H/2 + offset_y + (int)s1->pos.y;
                    int x2 = W/2 + offset_x + (int)(s1->pos.x + (s2->pos.x - s1->pos.x) * line_progress);
                    int y2 = H/2 + offset_y + (int)(s1->pos.y + (s2->pos.y - s1->pos.y) * line_progress);

                    // Thicker for DNA
                    if (active_indices[c] == 4 || active_indices[c] == 29) { // DNA indices
                        for (int thickness = -1; thickness <= 1; thickness++) {
                            SDL_RenderDrawLine(renderer, x1 + thickness, y1, x2 + thickness, y2);
                        }
                    } else {
                        SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
                        SDL_RenderDrawLine(renderer, x1 + 1, y1, x2 + 1, y2);
                    }
                }
            }

            // Draw stars - single point to match galaxy background size
            SDL_SetRenderDrawColor(renderer, constellation->star_color.r, constellation->star_color.g, constellation->star_color.b, constellation->star_color.a);
            for (int i = 0; i < active_num_stars[c]; i++) {
                if (active_stars[c][i].is_active) {
                    int x = W/2 + offset_x + (int)active_stars[c][i].pos.x;
                    int y = H/2 + offset_y + (int)active_stars[c][i].pos.y;
                    SDL_RenderDrawPoint(renderer, x, y);
                }
            }
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    // Cleanup
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
