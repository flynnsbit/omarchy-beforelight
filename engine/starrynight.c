/**
 * Starry Night Screensaver
 * Hyprland-compatible night sky screensaver with drifting stars and meteors
 * Night-sky starfield with meteors for Hyprland/Wayland.
 *
 * Controls:
 * - ESC or mouse/keyboard input to exit
 * - -s F: speed multiplier (default 1.0)
 * - -d N: star density (0=sparse, 1=dense, default 0.5)
 * - -m F: meteor frequency multiplier (default 1.0, higher = more meteors)
 *
 * Requires: SDL2
 * Build: gcc -o starrynight starrynight.c `sdl2-config --cflags --libs` -lm
 * Run: SDL_VIDEODRIVER=wayland ./starrynight
 */

#include <SDL2/SDL.h>
#include "beforelight_input.h"
#include "starry_gl_sdl.h"
#include <math.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#define PI 3.14159265359f
#define STAR_COUNT 500  // Space for drifting sky stars only
#define GAP_STAR_COUNT 10000  // Stars specifically in gaps between buildings
#define METEOR_COUNT 100
#define METEOR_PARTICLES 20
#define CITY_BUILDINGS 13     // Number of solid buildings with windows

// ADVANCED URBAN SYSTEM DEFINES - Dynamic Building System for Dense Cityscape
#define MAX_URBAN_BUILDINGS 100  // Increased capacity for dense city skyline coverage
#define LIGHTING_SYSTEM_LIMIT 300
#define ROOF_FEATURE_ARRAYS 15

// ROOF FEATURE DEFINITIONS - Bitmask Constants
#define ROOF_RESERVOIR_TOWER   0
#define ROOF_TRANSMISSION_TOWER 1
#define ROOF_AERIAL_PLATFORM   2
#define ROOF_MAINTENANCE_CRANE 3
#define ROOF_VENTILATIONS      4

// CHUNK 6: ADDITIONAL ARCHITECTURAL ROOF FEATURES
#define ROOF_HELIPAD_PLATFORM  5
#define ROOF_SOLAR_PANEL_ARRAY 6
#define ROOF_HVAC_UNITS       7
#define ROOF_RELIGIOUS_SYMBOLS 8
#define ROOF_SURVEILLANCE_BLIMP 9

// AIRCRAFT WARNING BEACON SPECIFICATIONS (CHUNK 2 Implementation)
#define AIRCRAFT_BEACON_DIAMETER     7.0f    // 5-10 pixel diameter, using 7
#define AIRCRAFT_BEACON_BLINK_PERIOD 1.5f    // 1-2 second intervals
#define AIRCRAFT_BEACON_ACTIVE_TIME  1.0f    // 1 second illumination duration

// WATER TOWER FACILITY SPECIFICATIONS (CHUNK 4 Implementation)
#define WATER_TOWER_CYLINDER_RADIUS  8.0f    // Base cylinder radius in pixels
#define WATER_TOWER_CYLINDER_HEIGHT 15.0f   // Main cylinder height
#define WATER_TOWER_DOME_HEIGHT     6.0f    // Dome cap height
#define CAUTION_LIGHT_PULSE_FREQ    0.75f   // 0.5-1.0 Hz pulse frequency (1.33s cycle)

// ILLUMINATED WINDOW GRID SPECIFICATIONS (CHUNK 5 Implementation)
#define MAX_WINDOW_GRID_WIDTH      20       // Maximum windows per floor
#define MAX_WINDOW_GRID_HEIGHT     32       // Maximum floors per building
#define WINDOW_GRID_SAFE_MARGIN    3.0f     // Pixel margin from building edges
#define WINDOW_ILLUMINATION_UPDATE_RATE 0.1f // Update frequency for dynamic illumination

typedef struct {
    float x, y;           // Position in pixels
    float vx, vy;         // Velocity for drift effect
    float brightness;     // Current brightness 0-1 (for twinkling)
    float base_brightness; // Base brightness 0-1
    float twinkle_phase;  // Sin phase offset for unique twinkling
    float twinkle_speed;  // How fast it twinkles
    float size;           // Star size in pixels
    bool is_bright;       // Extra-bright star status for glow effect
    int building_gap;     // Which building gap this star belongs to (-1 for sky)
} Star;

/**
 * MASTER URBAN BUILDING STRUCTURE - COMPLEX ARCHITECTURAL FRAMEWORK
 * Foundation for 11 Building Archetypes with Advanced Urban Features
 */
typedef struct {
    // GENERAL GEOMETRY - Core Structural Elements
    float x, y;                          // Ground floor base position
    float width, height;                 // Primary building envelope
    float right_edge;                    // Calculated boundary coordinates

    // ARCHITECTURAL HIERARCHY - Urban Scale Classification
    int building_type;                   // 0-10 Archetype enumeration
    int floor_quantity;                  // Story count based on archetype
    int window_count_horizontal;         // Windows per level specification

    // ILLUMINATION DYNAMICS - Building-Specific Lighting
    float illumination_percentage;       // 0.0-1.0 occupancy coefficient
    float brightness_coefficient;        // Buildings scale visibility factor
    int illumination_pattern_type;       // Residential/commercial differentiation

    // ROOFTOP INFRASTRUCTURE - Advanced Architectural Accessories
    unsigned int roof_feature_mask;      // Bitmask: antennas, water towers, helipads
    int antenna_element_array;          // Broadcast transmission components
    int water_storage_capacity;         // Municipal utility reservoir status
    int aircraft_warning_beacon_present; // Aviation safety compliance beacons

    // COMMUNICATION TOWER PROPERTIES - Fixed dimensions for stable rendering
    int tower_height_pixels;            // Tower height in pixels (30-60)
    int antenna_system_layout;          // Antenna positioning configuration

    // ILLUMINATED WINDOW GRID SYSTEM (CHUNK 5 Implementation)
    int window_grid[MAX_WINDOW_GRID_HEIGHT][MAX_WINDOW_GRID_WIDTH]; // 2D grid of illuminated windows
    float current_illumination_level;   // Real-time lighting percentage (animated)

    // OPERATIONAL STATS - Dynamic Performance Indicators
    float pulse_synchronization_timer;  // Lighting animation coordination
    int beacon_activation_cycle;        // Current flicker sequence phase
    int specialty_feature_indicator;    // Building-classified functionality

    // ADVANCED METRICS - Urban Planning Indicators
    float roof_level_elevation;         // Total height including features
    float architectural_significance;   // Cultural/historical weighting factor

} UrbanBuilding;

// URBAN LIGHTING MASTER CONTROLLER - Dynamic Illumination System
typedef struct {
    float positional_coordinates_x;   // Pixel coordinate precision x
    float positional_coordinates_y;   // Pixel coordinate precision y
    float illumination_intensity;      // Brightness modulation (0-1)
    float temporal_animation_cycle;   // Effects timing synchronization
    int structural_attachment_index;  // Parent building relationship
    int illumination_classification;  // Beacon/sign/etc. enumeration
    int operational_status_flag;      // Active/inactive state management

    // ADVANCED LIGHTING PROPERTIES (Chunk 1 Prepared for Chunks 2-10)
    float spectral_composition_r;     // RGB composition stack
    float spectral_composition_g;
    float spectral_composition_b;
    float operational_duration_ms;   // Duty cycle specifications
    float spatial_influence_radius;  // Lighting effect propagation
    int regulatory_compliance_code;   // Safety/zoning requirement flags

} DynamicLightingElement;

// ROOF ACCESSORY CATALOG DEFINITIONS - Architectural Feature Taxonomy
typedef struct {
    const char* architectural_description;     // ANSI text specification
    int vertical_dimension_requirements;       // Z-axis space allocation
    int illumination_theme_color_primary;      // Dominant spectral allocation
    int illumination_theme_color_secondary;    // Auxiliary lighting scheme
    int collision_detection_volume;           // Physical interference parameters
    int operational_power_consumption;        // Energy demand calculations
    int maintenance_service_interval;         // Scheduled upkeep specifications

} RoofArchitecturalAccessory;

// MASTER URBAN ARRAYS - Foundation for Sophisticated Cityscape
UrbanBuilding urban_complex[MAX_URBAN_BUILDINGS];              // Advanced building repository
DynamicLightingElement illumination_array[LIGHTING_SYSTEM_LIMIT]; // Urban lighting matrix
RoofArchitecturalAccessory architectural_catalog[ROOF_FEATURE_ARRAYS]; // Feature ontologies

// Legacy building array for backwards compatibility (remove after full integration)
typedef struct {
    float x, y;        // Bottom-left position
    float width, height; // Building dimensions
    float right_edge;  // Right edge x coordinate
} Building;

Building buildings[CITY_BUILDINGS]; // Static building array - temporary

// Gap stars - fill spaces between buildings
Star *gap_stars; // Stars that specifically fill gaps between buildings

typedef struct {
    float x, y;           // Current position
    float vx, vy;         // Velocity
    float life;           // 0-1 lifetime remaining
    float tail_particlesx[METEOR_PARTICLES]; // Positions for tail effect
    float tail_particlesy[METEOR_PARTICLES];
    float tail_alphas[METEOR_PARTICLES];
    int active;           // Currently animating
} Meteor;

// FUNCTION PROTOTYPES - Extended for Urban System Complexity
void init_stars(Star *stars, int count, int screen_width, int screen_height);
void update_stars(Star *stars, int count, float dt, int screen_width, int screen_height);
void render_stars(Star *stars, int count, int screen_width, int screen_height);
void render_gradient_background(int screen_width, int screen_height);
void init_meteor(Meteor *meteor, int screen_width, int screen_height);
void render_meteor(Meteor *meteor, int screen_width, int screen_height);
void update_meteor(Meteor *meteor, float dt, int screen_width, int screen_height);
void init_opengl(int width, int height);
void usage(const char *prog);

/**
 * URBAN BUILDING INITIALIZATION FUNCTION (Chunk 1 Core Implementation)
 * Generates sophisticated architectural configurations with building archetypes
 */
void initialize_urban_complex_generation(int screen_width, int screen_height);

/**
 * URBAN LIGHTING INFRASTRUCTURE INITIALIZATION (Chunk 1 Lighting Framework)
 * Establishes dynamic illumination management system for future chunks
 */
void establish_urban_lighting_infrastructure();

/**
 * AIRCRAFT WARNING BEACON RENDERING (Chunk 2 Implementation)
 * Renders FAA-compliant red flashing beacons on tall buildings for aviation safety
 */
void render_aircraft_warning_beacons(int screen_width __attribute__((unused)), int screen_height __attribute__((unused)));

/**
 * COMMUNICATION TOWER SYSTEMS (Chunk 3 Implementation)
 * Renders cellular transmission infrastructure with rotating strobe beacons
 */
void render_communication_tower_systems(int screen_width __attribute__((unused)), int screen_height __attribute__((unused)));

/**
 * ILLUMINATED WINDOW GRID ALGORITHMS (Chunk 5 Implementation)
 * Renders intelligent building occupancy visualization with time-sensitive patterns
 */
void render_illuminated_window_grids(int screen_width __attribute__((unused)), int screen_height __attribute__((unused)));

void initialize_window_illumination_patterns(void);

/**
 * ROOF ARCHITECTURAL ACCESSORY COMPLEXITY (Chunk 6 Implementation)
 * Renders sophisticated rooftop architectural features like helipads, solar panels, and cranes
 */
void render_roof_architectural_accessory_complexity(int screen_width __attribute__((unused)), int screen_height __attribute__((unused))) {
    static float global_hvac_timer = 0.0f; // Smooth accumulated timing for HVAC fan rotation
    global_hvac_timer += 0.016f; // ~60 FPS increment for consistent timing

    for (int building_index = 0; building_index < MAX_URBAN_BUILDINGS; building_index++) {
        UrbanBuilding* structure = &urban_complex[building_index];

        // HELIPAD PLATFORMS - Circular helicopter landing platforms
        if (structure->roof_feature_mask & (1 << ROOF_HELIPAD_PLATFORM)) {
            glColor4f(0.8f, 0.8f, 0.8f, 0.9f); // Light gray landing surface

            // HELIPAD CIRCLE - 15-pixel radius circular landing area
            float helipad_center_x = structure->x + structure->width / 2.0f;
            float helipad_center_y = structure->roof_level_elevation + 8.0f;
            float helipad_radius = 15.0f;

            // Filled circle using triangle fan approximation
            glBegin(GL_TRIANGLE_FAN);
            glVertex2f(helipad_center_x, helipad_center_y); // Center point
            for (int i = 0; i <= 16; i++) {
                float angle = (float)i * PI / 8.0f; // 16 segments for smooth circle
                glVertex2f(helipad_center_x + cosf(angle) * helipad_radius,
                          helipad_center_y + sinf(angle) * helipad_radius);
            }
            glEnd();

            // HELIPAD CENTER H MARKING - FAA-required H symbol
            glColor4f(1.0f, 1.0f, 1.0f, 0.8f); // White H marking
            glLineWidth(2.0f);

            glBegin(GL_LINES);
            // Horizontal H spine
            glVertex2f(helipad_center_x - 8.0f, helipad_center_y);
            glVertex2f(helipad_center_x + 8.0f, helipad_center_y);

            // Left H leg
            glVertex2f(helipad_center_x - 8.0f, helipad_center_y - 6.0f);
            glVertex2f(helipad_center_x - 8.0f, helipad_center_y + 6.0f);

            // Right H leg
            glVertex2f(helipad_center_x + 8.0f, helipad_center_y - 6.0f);
            glVertex2f(helipad_center_x + 8.0f, helipad_center_y + 6.0f);
            glEnd();
        }

        // SOLAR PANEL ARRAYS - Photovoltaic energy collection systems
        if (structure->roof_feature_mask & (1 << ROOF_SOLAR_PANEL_ARRAY)) {
            glColor4f(0.2f, 0.2f, 0.4f, 0.9f); // Dark blue solar panels

            // SOLAR PANEL GRID - Arranged in 2x3 formation near building edge
            float panel_start_x = structure->x + 5.0f;
            float panel_y = structure->roof_level_elevation + 1.0f;
            float panel_width = 8.0f;
            float panel_height = 12.0f;
            float panel_spacing_x = 2.0f;
            float panel_spacing_y = 3.0f;

            for (int row = 0; row < 2; row++) {
                for (int col = 0; col < 3; col++) {
                    float panel_x = panel_start_x + col * (panel_width + panel_spacing_x);
                    float current_panel_y = panel_y + row * (panel_height + panel_spacing_y);

                    // Individual solar panel rectangle
                    glBegin(GL_QUADS);
                    glVertex2f(panel_x, current_panel_y);
                    glVertex2f(panel_x + panel_width, current_panel_y);
                    glVertex2f(panel_x + panel_width, current_panel_y + panel_height);
                    glVertex2f(panel_x, current_panel_y + panel_height);
                    glEnd();
                }
            }

            // REFLECTIVE GLAZE HIGHLIGHTS - Shiny panel appearance
            glColor4f(0.6f, 0.7f, 0.9f, 0.6f); // Light blue reflection
            glLineWidth(1.0f);

            glBegin(GL_LINES);
            // Diagonal reflection lines for realism
            for (int row = 0; row < 2; row++) {
                for (int col = 0; col < 3; col++) {
                    float panel_x = panel_start_x + col * (panel_width + panel_spacing_x);
                    float current_panel_y = panel_y + row * (panel_height + panel_spacing_y);
                    glVertex2f(panel_x + 1.0f, current_panel_y + panel_height - 2.0f);
                    glVertex2f(panel_x + panel_width - 3.0f, current_panel_y + 2.0f);
                }
            }
            glEnd();
        }

        // HVAC VENTILATION UNITS - Cooling and ventilation equipment
        if (structure->roof_feature_mask & (1 << ROOF_HVAC_UNITS)) {
            // HVAC UNIT BODY - Rectangular mechanical equipment
            glColor4f(0.3f, 0.3f, 0.4f, 0.95f); // Dark metallic blue-gray
            float hvac_x = structure->x + structure->width - 15.0f;
            float hvac_y = structure->roof_level_elevation;
            float hvac_width = 10.0f;
            float hvac_height = 6.0f;

            glBegin(GL_QUADS);
            glVertex2f(hvac_x, hvac_y);
            glVertex2f(hvac_x + hvac_width, hvac_y);
            glVertex2f(hvac_x + hvac_width, hvac_y + hvac_height);
            glVertex2f(hvac_x, hvac_y + hvac_height);
            glEnd();

            // HVAC FAN GRILLE - Circular ventilation openings
            glColor4f(0.1f, 0.1f, 0.1f, 1.0f); // Black grille
            float grille_center_x = hvac_x + hvac_width/2.0f;
            float grille_center_y = hvac_y + hvac_height - 3.0f;
            float grille_radius = 2.0f;

            glBegin(GL_POINTS);
            for (int i = 0; i < 8; i++) {
                float grille_angle = (float)i * PI / 4.0f;
                glVertex2f(grille_center_x + cosf(grille_angle) * grille_radius,
                          grille_center_y + sinf(grille_angle) * grille_radius);
            }
            glEnd();

            // ROTATING FAN BLADES - Animated mechanical movement
            glColor4f(0.8f, 0.8f, 0.8f, 0.9f); // Light gray blades
            float fan_rotation = global_hvac_timer * 360.0f * 0.5f; // 180°/second rotation

            glBegin(GL_LINES);
            for (int blade = 0; blade < 4; blade++) {
                float blade_angle = (PI / 2.0f) * blade + (fan_rotation * PI / 180.0f);
                glVertex2f(grille_center_x, grille_center_y);
                glVertex2f(grille_center_x + cosf(blade_angle) * (grille_radius * 0.8f),
                          grille_center_y + sinf(blade_angle) * (grille_radius * 0.8f));
            }
            glEnd();
        }

        // RELIGIOUS ARCHITECTURAL SYMBOLS - Crosses and symbolic forms
        if (structure->roof_feature_mask & (1 << ROOF_RELIGIOUS_SYMBOLS)) {
            glColor4f(0.9f, 0.85f, 0.5f, 1.0f); // Gold-colored cross

            // CROSS SYMBOL - Vertical and horizontal beams
            float cross_center_x = structure->x + structure->width - 8.0f;
            float cross_center_y = structure->roof_level_elevation + 12.0f;
            float cross_beam_length = 6.0f;
            float cross_beam_thickness = 1.5f;

            // Vertical cross beam
            glBegin(GL_QUADS);
            glVertex2f(cross_center_x - cross_beam_thickness/2.0f,
                      cross_center_y - cross_beam_length * 0.6f);
            glVertex2f(cross_center_x + cross_beam_thickness/2.0f,
                      cross_center_y - cross_beam_length * 0.6f);
            glVertex2f(cross_center_x + cross_beam_thickness/2.0f,
                      cross_center_y + cross_beam_length * 0.6f);
            glVertex2f(cross_center_x - cross_beam_thickness/2.0f,
                      cross_center_y + cross_beam_length * 0.6f);
            glEnd();

            // Horizontal cross beam
            glBegin(GL_QUADS);
            glVertex2f(cross_center_x - cross_beam_length/2.0f,
                      cross_center_y - cross_beam_thickness/2.0f);
            glVertex2f(cross_center_x + cross_beam_length/2.0f,
                      cross_center_y - cross_beam_thickness/2.0f);
            glVertex2f(cross_center_x + cross_beam_length/2.0f,
                      cross_center_y + cross_beam_thickness/2.0f);
            glVertex2f(cross_center_x - cross_beam_length/2.0f,
                      cross_center_y + cross_beam_thickness/2.0f);
            glEnd();

            // GLOW AURA - Sacred illumination effect
            glColor4f(0.95f, 0.9f, 0.7f, 0.4f); // Soft golden glow
            glPointSize(4.0f);
            glBegin(GL_POINTS);
            glVertex2f(cross_center_x, cross_center_y);
            glEnd();
        }

        // SURVEILLANCE BLIMPS - Inflatable camera platforms with tether systems
        if (structure->roof_feature_mask & (1 << ROOF_SURVEILLANCE_BLIMP)) {
            // BLIMP TETHER LINE - Thin cable from building to blimp
            glColor4f(0.3f, 0.3f, 0.3f, 0.8f); // Dark gray tether
            glLineWidth(1.0f);

            float tether_x = structure->x + structure->width / 2.0f;
            float tether_roof_y = structure->roof_level_elevation;
            float tether_blimp_y = structure->roof_level_elevation + 25.0f;

            glBegin(GL_LINES);
            glVertex2f(tether_x, tether_roof_y);
            glVertex2f(tether_x, tether_blimp_y);
            glEnd();

            // BLIMP ENVELOPE - Elliptical surveillance platform
            glColor4f(0.6f, 0.6f, 0.8f, 0.85f); // Light gray blimp envelope
            float blimp_width = 15.0f;
            float blimp_height = 8.0f;
            float blimp_x = tether_x - blimp_width/2.0f;
            float blimp_y = tether_blimp_y - blimp_height/2.0f;

            glBegin(GL_QUADS);
            glVertex2f(blimp_x, blimp_y);
            glVertex2f(blimp_x + blimp_width, blimp_y);
            glVertex2f(blimp_x + blimp_width, blimp_y + blimp_height);
            glVertex2f(blimp_x, blimp_y + blimp_height);
            glEnd();

            // SURVEILLANCE CAMERA - Equipment mounted on blimp
            glColor4f(0.2f, 0.2f, 0.2f, 0.95f); // Black camera housing
            glPointSize(2.0f);
            glBegin(GL_POINTS);
            glVertex2f(tether_x, tether_blimp_y);
            glEnd();

            // CAMERA LENS REFLECTION
            glColor4f(1.0f, 1.0f, 0.8f, 0.8f); // Bright lens flare
            glPointSize(1.0f);
            glBegin(GL_POINTS);
            glVertex2f(tether_x - 1.0f, tether_blimp_y + 0.5f);
            glEnd();
        }

        // CONSTRUCTION CRANES - Boom arms with counterweights (already implemented in earlier chunks)
        // CRANES ARE ALREADY RENDERED IN render_aircraft_warning_beacons() function
    }

    // RESET RENDER STATE - Restore defaults for subsequent rendering layers
    glLineWidth(1.0f); // Reset line width
    glPointSize(1.0f); // Reset point size
}

/**
 * WATER TOWER FACILITY INSTALLATIONS (Chunk 4 Implementation)
 * Renders elevated water reservoir towers with pulsing caution lighting
 */
void render_water_tower_facility_installations(int screen_width __attribute__((unused)), int screen_height __attribute__((unused))) {
    static float global_caution_timer = 0.0f; // Smooth accumulated timing for caution lighting
    global_caution_timer += 0.016f; // ~60 FPS increment for consistent timing

    for (int building_index = 0; building_index < MAX_URBAN_BUILDINGS; building_index++) {
        UrbanBuilding* structure = &urban_complex[building_index];

        // WATER TOWER PRESENCE VERIFICATION
        if (!(structure->roof_feature_mask & (1 << ROOF_RESERVOIR_TOWER))) {
            continue; // No water tower on this building
        }

        // WATER TOWER POSITIONING - Elevated structure above building roof level
        float tower_base_x = structure->x + structure->width / 2.0f; // Center of building
        float tower_base_y = structure->roof_level_elevation + 3.0f; // Above roof level

        // WATER TOWER GEOMETRY - Aged metallic cylindrical silhouette with dome
        float cylinder_bottom_y = tower_base_y;
        float cylinder_top_y = tower_base_y + WATER_TOWER_CYLINDER_HEIGHT;
        float dome_top_y = cylinder_top_y + WATER_TOWER_DOME_HEIGHT;

        // STRUCTURAL SUPPORT LEGS - Four independent steel support pillars
        glColor4f(0.2f, 0.2f, 0.2f, 0.9f); // Dark metallic support color
        glLineWidth(3.0f);

        glBegin(GL_LINES);
        // Four corner support pillars
        float leg_offset = WATER_TOWER_CYLINDER_RADIUS;
        glVertex2f(tower_base_x - leg_offset, tower_base_y);
        glVertex2f(tower_base_x - leg_offset, cylinder_bottom_y);

        glVertex2f(tower_base_x + leg_offset, tower_base_y);
        glVertex2f(tower_base_x + leg_offset, cylinder_bottom_y);

        glVertex2f(tower_base_x - leg_offset/2.0f, cylinder_top_y);
        glVertex2f(tower_base_x - leg_offset/2.0f, dome_top_y);

        glVertex2f(tower_base_x + leg_offset/2.0f, cylinder_top_y);
        glVertex2f(tower_base_x + leg_offset/2.0f, dome_top_y);
        glEnd();

        // CYLINDRICAL RESERVOIR BODY - Aged metallic water container
        glColor4f(0.4f, 0.4f, 0.45f, 0.95f); // Aged metal blue-gray
        glLineWidth(1.0f);

        // Cylinder outline using quad strips for filled appearance
        glBegin(GL_QUADS);
        // Front face
        glVertex2f(tower_base_x - WATER_TOWER_CYLINDER_RADIUS, cylinder_bottom_y);
        glVertex2f(tower_base_x + WATER_TOWER_CYLINDER_RADIUS, cylinder_bottom_y);
        glVertex2f(tower_base_x + WATER_TOWER_CYLINDER_RADIUS, cylinder_top_y);
        glVertex2f(tower_base_x - WATER_TOWER_CYLINDER_RADIUS, cylinder_top_y);

        // Back face (same as front for 2D appearance)
        glVertex2f(tower_base_x - WATER_TOWER_CYLINDER_RADIUS, cylinder_bottom_y);
        glVertex2f(tower_base_x + WATER_TOWER_CYLINDER_RADIUS, cylinder_bottom_y);
        glVertex2f(tower_base_x + WATER_TOWER_CYLINDER_RADIUS, cylinder_top_y);
        glVertex2f(tower_base_x - WATER_TOWER_CYLINDER_RADIUS, cylinder_top_y);
        glEnd();

        // DOME CAP - Spherical dome silhouette above cylinder
        glColor4f(0.5f, 0.5f, 0.55f, 0.9f); // Lighter dome color
        glBegin(GL_QUADS);
        // Dome approximated with rectangular sections
        glVertex2f(tower_base_x - WATER_TOWER_CYLINDER_RADIUS * 0.7f, cylinder_top_y);
        glVertex2f(tower_base_x + WATER_TOWER_CYLINDER_RADIUS * 0.7f, cylinder_top_y);
        glVertex2f(tower_base_x + WATER_TOWER_CYLINDER_RADIUS * 0.5f, dome_top_y);
        glVertex2f(tower_base_x - WATER_TOWER_CYLINDER_RADIUS * 0.5f, dome_top_y);
        glEnd();

        // MAINTENANCE CATWALK - Platform around cylinder for access
        glColor4f(0.25f, 0.25f, 0.25f, 0.8f); // Dark platform color
        glBegin(GL_QUADS);
        // Catwalk platform rectangle
        float catwalk_width = WATER_TOWER_CYLINDER_RADIUS * 2.5f;
        float catwalk_height = 2.0f;
        float catwalk_y = cylinder_bottom_y + WATER_TOWER_CYLINDER_HEIGHT * 0.6f;

        glVertex2f(tower_base_x - catwalk_width/2.0f, catwalk_y);
        glVertex2f(tower_base_x + catwalk_width/2.0f, catwalk_y);
        glVertex2f(tower_base_x + catwalk_width/2.0f, catwalk_y + catwalk_height);
        glVertex2f(tower_base_x - catwalk_width/2.0f, catwalk_y + catwalk_height);
        glEnd();

        // HANDRAIL DETAILS - Safety railings on catwalk
        glColor4f(0.15f, 0.15f, 0.15f, 0.7f); // Darker handrail color
        glLineWidth(2.0f);
        glBegin(GL_LINES);
        glVertex2f(tower_base_x - catwalk_width/2.0f, catwalk_y + catwalk_height);
        glVertex2f(tower_base_x + catwalk_width/2.0f, catwalk_y + catwalk_height);
        glEnd();

        // SEVER DUTY CAUTION LIGHTING - Red/yellow pulsing warning beacons
        float caution_light_y = cylinder_top_y + WATER_TOWER_DOME_HEIGHT + 5.0f;

        // TIME-BASED CAUTION PULSE - 0.75 Hz frequency (1.33 second cycle)
        float pulse_phase = fmodf(global_caution_timer * CAUTION_LIGHT_PULSE_FREQ, 1.0f);

        // COLOR-CYCLING WARNING SYSTEM - Red to yellow pulsing pattern
        float caution_intensity = 0.8f + 0.2f * sinf(pulse_phase * 2.0f * PI); // Pulsing intensity

        if (pulse_phase < 0.5f) {
            // First half cycle - red warning color
            glColor4f(1.0f, 0.0f, 0.0f, caution_intensity); // Bright red
        } else {
            // Second half cycle - yellow caution color
            glColor4f(1.0f, 1.0f, 0.0f, caution_intensity); // Bright yellow
        }

        // CAUTION BEACON RENDERING - Large diameter warning light
        glPointSize(6.0f);
        glBegin(GL_POINTS);
        glVertex2f(tower_base_x, caution_light_y);
        glEnd();

        // AURA GLOW EFFECT - Enhanced visibility for safety warnings
        glPointSize(10.0f);
        glColor4f(1.0f, 0.5f, 0.0f, caution_intensity * 0.4f); // Orange-red glow

        glBegin(GL_POINTS);
        glVertex2f(tower_base_x, caution_light_y);
        glEnd();
    }

    // RESET RENDER STATE - Restore defaults for subsequent rendering
    glLineWidth(1.0f); // Reset line width
    glPointSize(1.0f); // Reset point size
}

/**
 * ILLUMINATED WINDOW GRID ALGORITHMS - Initialize window illumination patterns
 * Sets up realistic occupancy patterns based on building types and floor structures
 */
void initialize_window_illumination_patterns(void) {
    for (int build_index = 0; build_index < MAX_URBAN_BUILDINGS; build_index++) {
        UrbanBuilding* structure = &urban_complex[build_index];

        // CALCULATE WINDOW GRID DIMENSIONS based on building properties
        int floors = structure->floor_quantity;
        int windows_per_floor = (structure->window_count_horizontal > 0) ?
            structure->window_count_horizontal : 3; // Default fallback

        // Ensure grid doesn't exceed maximum dimensions
        floors = (floors > MAX_WINDOW_GRID_HEIGHT) ? MAX_WINDOW_GRID_HEIGHT : floors;
        windows_per_floor = (windows_per_floor > MAX_WINDOW_GRID_WIDTH) ?
            MAX_WINDOW_GRID_WIDTH : windows_per_floor;

        // INITIALIZE ILLUMINATION GRID - Nighttime occupancy defaults
        float base_occupancy = structure->illumination_percentage;
        float illumination_level = 0.0f; // Start at 0 for daytime/low-activity simulation

        for (int floor = 0; floor < floors; floor++) {
            for (int window_x = 0; window_x < windows_per_floor; window_x++) {
                // CLEAR GRID - Start with all windows dark
                structure->window_grid[floor][window_x] = 0;

                // BUILDING-TYPE SPECIFIC ILLUMINATION PATTERNS
                float illumination_probability = base_occupancy;

                // FLOOR-BASED VARIATION - Lower floors more occupied in residential
                if (structure->illumination_pattern_type <= 1) { // Residential/Apartment
                    float floor_factor = 1.0f - (float)floor / (float)floors * 0.3f; // Bottom floors busier
                    illumination_probability *= floor_factor;
                } else if (structure->illumination_pattern_type >= 5) { // Commercial/Late-night
                    float floor_factor = 1.0f - (float)floor / (float)floors * 0.1f; // More uniform
                    illumination_probability *= floor_factor;
                }

                // RANDOM OCCUPANCY DETERMINATION
                float occupancy_roll = (float)rand() / RAND_MAX;
                if (occupancy_roll < illumination_probability) {
                    structure->window_grid[floor][window_x] = 1; // Illuminated
                    illumination_level += 1.0f / (floors * windows_per_floor); // Track total illumination
                }
            }
        }

        // STORE REAL-TIME ILLUMINATION LEVEL for animation system
        structure->current_illumination_level = illumination_level;
    }
}

/**
 * ILLUMINATED WINDOW GRID ALGORITHMS (Chunk 5 Implementation)
 * Renders intelligent building occupancy visualization with time-sensitive patterns
 */
void render_illuminated_window_grids(int screen_width __attribute__((unused)), int screen_height __attribute__((unused))) {
    for (int build_index = 0; build_index < MAX_URBAN_BUILDINGS; build_index++) {
        UrbanBuilding* structure = &urban_complex[build_index];

        // BUILDING DIMENSIONS - Use established building parameters
        float building_x = structure->x;
        float building_y = structure->y;
        float building_width = structure->width;
        float building_height = structure->height;

        // WINDOW GRID CALCULATION
        int floors = structure->floor_quantity;
        int windows_per_floor = (structure->window_count_horizontal > 0) ?
            structure->window_count_horizontal : 3;

        // CONSTRAIN TO SAFE BOUNDARIES
        floors = (floors > MAX_WINDOW_GRID_HEIGHT) ? MAX_WINDOW_GRID_HEIGHT : floors;
        windows_per_floor = (windows_per_floor > MAX_WINDOW_GRID_WIDTH) ?
            MAX_WINDOW_GRID_WIDTH : windows_per_floor;

        // WINDOW DIMENSIONS AND SPACING - Smaller windows for more realistic appearance
        float available_width = building_width - (WINDOW_GRID_SAFE_MARGIN * 2.0f);
        float window_width = available_width / windows_per_floor;
        float floor_height = building_height / floors;
        float window_height = floor_height * 0.5f; // Windows occupy 50% of floor height (smaller)
        float window_spacing_y = floor_height * 0.25f; // Vertical spacing between floors

        // ILLUMINATED WINDOW RENDERING WITH GRADIENT LIGHTING (Phase 3 Enhancement)
        glBegin(GL_QUADS);
        for (int floor = 0; floor < floors; floor++) {
            for (int window_x = 0; window_x < windows_per_floor; window_x++) {
                // CHECK WINDOW ILLUMINATION STATUS
                if (structure->window_grid[floor][window_x] == 0) {
                    continue; // Window is dark
                }

                // WINDOW POSITION CALCULATION
                float window_left = building_x + WINDOW_GRID_SAFE_MARGIN + window_x * window_width;
                float window_bottom = building_y + WINDOW_GRID_SAFE_MARGIN + floor * floor_height + window_spacing_y;
                float window_right = window_left + window_width * 0.8f; // 80% of allocated width
                float window_top = window_bottom + window_height;
                float window_center_x = (window_left + window_right) / 2.0f;
                float window_center_y = (window_bottom + window_top) / 2.0f;

                // RENDER DARK WINDOW BORDER FRAME FIRST
                glColor4f(0.1f, 0.1f, 0.1f, 0.9f); // Dark gray frame
                const float FRAME_WIDTH = 1.5f; // 1.5 pixel frame
                glVertex2f(window_left - FRAME_WIDTH, window_bottom - FRAME_WIDTH);
                glVertex2f(window_right + FRAME_WIDTH, window_bottom - FRAME_WIDTH);
                glVertex2f(window_right + FRAME_WIDTH, window_top + FRAME_WIDTH);
                glVertex2f(window_left - FRAME_WIDTH, window_top + FRAME_WIDTH);

                // PHASE 3: MULTI-LAYER WINDOW GRADIENT LIGHTING for 3D depth
                // Bright center, darkening toward edges for realistic interior depth

                // LAYER 1: BRIGHT CENTER (Core high-intensity area)
                glColor4f(0.85f, 0.65f, 0.4f, 0.98f); // Bright gold center
                float center_size_x = (window_right - window_left) * 0.6f; // 60% width
                float center_size_y = (window_top - window_bottom) * 0.65f; // 65% height
                glVertex2f(window_center_x - center_size_x/2.0f, window_center_y - center_size_y/2.0f);
                glVertex2f(window_center_x + center_size_x/2.0f, window_center_y - center_size_y/2.0f);
                glVertex2f(window_center_x + center_size_x/2.0f, window_center_y + center_size_y/2.0f);
                glVertex2f(window_center_x - center_size_x/2.0f, window_center_y + center_size_y/2.0f);

                // LAYER 2: MEDIUM BRIGHTNESS MIDDLE LAYER
                glColor4f(0.75f, 0.55f, 0.3f, 0.9f); // Medium gold
                float middle_size_x = (window_right - window_left) * 0.85f; // 85% width
                float middle_size_y = (window_top - window_bottom) * 0.85f; // 85% height
                glVertex2f(window_center_x - middle_size_x/2.0f, window_center_y - middle_size_y/2.0f);
                glVertex2f(window_center_x + middle_size_x/2.0f, window_center_y - middle_size_y/2.0f);
                glVertex2f(window_center_x + middle_size_x/2.0f, window_center_y + middle_size_y/2.0f);
                glVertex2f(window_center_x - middle_size_x/2.0f, window_center_y + middle_size_y/2.0f);

                // LAYER 3: DIM OUTER EDGE (Full coverage with lowest brightness)
                glColor4f(0.65f, 0.45f, 0.2f, 0.8f); // Dim gold edges
                glVertex2f(window_left, window_bottom);
                glVertex2f(window_right, window_bottom);
                glVertex2f(window_right, window_top);
                glVertex2f(window_left, window_top);

                // GLASS REFLECTION HIGHLIGHTS - Enhanced for Phase 3 depth
                glEnd(); // End current quads
                glColor4f(1.0f, 1.0f, 0.95f, 0.85f); // Brighter highlights
                glPointSize(2.5f); // Even more visible
                glBegin(GL_POINTS);
                // Enhanced reflection pattern for glass depth
                glVertex2f(window_left + 3.0f, window_top - 3.0f);  // Close top-left
                glVertex2f(window_left + (window_right - window_left) * 0.65f, window_top - 4.0f); // Mid-upper
                glVertex2f(window_left + (window_right - window_left) * 0.35f, window_top - 2.0f); // Left-upper
                glVertex2f(window_left + 2.0f, window_bottom + (window_top - window_bottom) * 0.7f); // Left-mid
                glEnd();
                glPointSize(1.0f); // Reset point size for stars
                glBegin(GL_QUADS); // Resume quads for next window
            }
        }
        glEnd();
    }
}

// Main function
int main(int argc, char *argv[]) {
    // Parse command line arguments
    float speed_mult = 1.0f;
    float star_density = 0.5f;
    float meteor_freq = 1.0f;
    int ch;

    while ((ch = getopt(argc, argv, "s:d:m:f:h")) != -1) {
        switch (ch) {
            case 's':
                speed_mult = atof(optarg);
                break;
            case 'd':
                star_density = atof(optarg);
                if (star_density < 0) star_density = 0;
                if (star_density > 1) star_density = 1;
                break;
            case 'f':
                break;
            case 'm':
                meteor_freq = atof(optarg);
                if (meteor_freq < 0) meteor_freq = 0;
                if (meteor_freq > 5) meteor_freq = 5;
                break;
            case 'h':
            default:
                usage(argv[0]);
                return 1;
        }
    }

    SDL_SetHint(SDL_HINT_VIDEODRIVER, "wayland");

    srand((unsigned int)time(NULL));

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Starry Night",
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          800, 600,
                                          SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS | SDL_WINDOW_RESIZABLE);
    if (!window) {
        fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    beforelight_grab_cursor();
    g_r = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!g_r)
        g_r = SDL_CreateRenderer(window, -1, 0);
    if (!g_r) {
        fprintf(stderr, "Renderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_SetRenderDrawBlendMode(g_r, SDL_BLENDMODE_BLEND);

    int screen_width = 800, screen_height = 600;
    SDL_GetRendererOutputSize(g_r, &screen_width, &screen_height);
    init_opengl(screen_width, screen_height);

    // CHUNK 1: ESTABLISH URBAN SYSTEM FOUNDATION
    // Initialize sophisticated urban building data architecture
    initialize_urban_complex_generation(screen_width, screen_height);
    establish_urban_lighting_infrastructure();

    // DYNAMIC WINDOW ILLUMINATION UPDATE TIMER - For random on/off transitions
    float window_update_timer = 0.0f;

// LEGACY STATIC BUILDING PRECALCULATION REMOVED
// All buildings now dynamically generated in urban_complex with proper urban planning

    // CALCULATE CONTINUOUS STAR FIELD ACROSS ENTIRE SCREEN WIDTH
    // Stars will be positioned across full horizontal range with vertical density control
    gap_stars = malloc(GAP_STAR_COUNT * sizeof(Star));

    // Stars are now distributed across full screen width (no gaps - everything is "open area")
    // Vertical distribution controls where stars appear relative to buildings

    // CREATE CONTINUOUS STAR FIELD WITH GRADUAL HEIGHT-BASED DENSITY ACROSS ENTIRE WIDTH
    int star_idx = 0;

    // Find the maximum building height to determine blend zones
    // Use the dynamic urban_complex instead of static buildings array
    float max_building_height = 0.0f;
    for (int i = 0; i < MAX_URBAN_BUILDINGS; i++) {
        if (urban_complex[i].height > max_building_height) {
            max_building_height = urban_complex[i].height;
        }
    }

    // Define density zones for continuous coverage
    // Zone 1: Building area (dense - near ground level)
    // Zone 2: Above building tops (gradually decreasing)
    // Zone 3: Upper atmosphere (matching sky density)

    float building_top_level = urban_complex[0].y + max_building_height; // Top of tallest building
    float zone2_end = building_top_level + (max_building_height * 0.5f); // 50% above buildings
    float zone3_start = screen_height / 4; // Where sky stars begin

    // Create stars across entire screen width with height-based density
    for (int j = 0; j < GAP_STAR_COUNT; j++) {
        Star *star = &gap_stars[star_idx];

        // Position across full screen width (not just gaps)
        star->x = (float)rand() / RAND_MAX * screen_width;

        // Position with gradual density decrease from building level to full height
        // Map star to different vertical zones with varying density
        float rand_val = (float)rand() / RAND_MAX;

        if (rand_val < 0.6f) {
            // 60% of stars in dense building level zone (near ground, between buildings)
            star->y = urban_complex[0].y + rand_val / 0.6f * max_building_height;
        } else if (rand_val < 0.9f) {
            // 30% of stars in medium density zone (above building tops)
            float zone_progress = (rand_val - 0.6f) / 0.3f; // 0-1 in this zone
            star->y = building_top_level + zone_progress * (zone2_end - building_top_level);
        } else {
            // 10% of stars in light upper zone (towards where sky stars begin)
            float zone_progress = (rand_val - 0.9f) / 0.1f; // 0-1 in this zone
            star->y = zone3_start + zone_progress * (screen_height - zone3_start);
        }

        // Define motion
        star->vx = (float)(rand() % 20 - 10) / 500.0f; // Very slow drift
        star->vy = (float)(rand() % 20 - 10) / 500.0f;

        // Standard star properties - slightly dimmer than sky stars for layering
        star->base_brightness = 0.3f + (float)(rand() % 50) / 100.0f; // 0.3-0.8 range
        star->brightness = star->base_brightness;
        star->twinkle_phase = (float)(rand() % 628) / 100.0f;
        star->twinkle_speed = 0.6f + (float)(rand() % 80) / 100.0f; // Varied twinkling
        star->size = 0.8f + (float)(rand() % 15) / 10.0f; // Smaller 0.8-2.3 pixels
        star->is_bright = (rand() % 100) < 15; // 15% bright gap stars (fewer than sky)
        star->building_gap = -1; // No longer tracking specific gaps

        star_idx++;
    }

    SDL_GetRendererOutputSize(g_r, &screen_width, &screen_height);
    init_opengl(screen_width, screen_height);

    // BUILDING TEMPLATES AND COMPLEX LIGHTING LOGIC REMOVED - CLEANING UP VISUAL ARTIFACTS

    // Initialize star system - adjust count based on density setting
    int actual_star_count = (int)(STAR_COUNT * (0.3f + star_density * 0.7f)); // 225-750 stars
    Star *stars = (Star *)malloc(actual_star_count * sizeof(Star));
    init_stars(stars, actual_star_count, screen_width, screen_height);

    // Initialize meteor system
    Meteor meteors[METEOR_COUNT];
    for (int i = 0; i < METEOR_COUNT; i++) {
        meteors[i].life = 0;  // Start inactive
    }

    // OLD BACKGROUND STAR SYSTEM REMOVED - Replaced with gap stars that fill spaces between buildings

    Uint64 last_time = SDL_GetTicks64();
    Uint32 start_time = SDL_GetTicks();
    BeforelightInput input = {0};
    float meteor_timer = 0;

    SDL_Event event;
    bool running = true;

    while (running) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    running = false;
                    break;
                case SDL_KEYDOWN:
                case SDL_MOUSEBUTTONDOWN:
                    running = false;
                    break;
                case SDL_MOUSEMOTION:
                    if (beforelight_should_quit_motion(&input, start_time, &event))
                        running = false;
                    break;
                case SDL_WINDOWEVENT:
                    if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                        SDL_GetRendererOutputSize(g_r, &screen_width, &screen_height);
                        init_opengl(screen_width, screen_height);
                    }
                    break;
            }
        }

        Uint64 current_time = SDL_GetTicks64();
        float dt = (float)(current_time - last_time) / 1000.0f;
        last_time = current_time;

        // Update sky stars and gap stars
        update_stars(stars, actual_star_count, dt * speed_mult, screen_width, screen_height);
        update_stars(gap_stars, GAP_STAR_COUNT, dt * speed_mult, screen_width, screen_height);

        // Update and handle meteors - much more frequent for visibility
        meteor_timer += dt * speed_mult;
        float meteor_interval = 1.0f / meteor_freq; // Base interval of 1 second, adjusted by frequency

        if (meteor_timer >= meteor_interval) {
            meteor_timer -= meteor_interval;

            // Find inactive meteor to activate
            for (int i = 0; i < METEOR_COUNT; i++) {
                if (meteors[i].life <= 0) {
                    init_meteor(&meteors[i], screen_width, screen_height);
                    break;
                }
            }
        }

        // Update active meteors
        for (int i = 0; i < METEOR_COUNT; i++) {
            if (meteors[i].life > 0) {
                update_meteor(&meteors[i], dt * speed_mult, screen_width, screen_height);
            }
        }

        // WINDOW RANDOM ILLUMINATION UPDATE - Every 0.75 seconds toggle some windows randomly for dynamic lighting effect
        window_update_timer += dt;
        if (window_update_timer >= 0.75f) { // 0.75 second interval - faster, more visible activity
            window_update_timer = 0.0f; // Reset timer

            // Randomly select and toggle several windows across buildings (slightly more than before)
            for (int toggle_count = 0; toggle_count < 25; toggle_count++) { // Toggle 25 windows each time
                int random_building = rand() % MAX_URBAN_BUILDINGS;
                UrbanBuilding* structure = &urban_complex[random_building];

                // Only toggle windows for buildings that have been initialized
                if (structure->floor_quantity > 0 && structure->window_count_horizontal > 0) {
                    int max_floors = (structure->floor_quantity < MAX_WINDOW_GRID_HEIGHT) ?
                        structure->floor_quantity : MAX_WINDOW_GRID_HEIGHT;
                    int max_windows = (structure->window_count_horizontal < MAX_WINDOW_GRID_WIDTH) ?
                        structure->window_count_horizontal : MAX_WINDOW_GRID_WIDTH;

                    int random_floor = rand() % max_floors;
                    int random_window = rand() % max_windows;

                    // Toggle the window state (0 to 1 or 1 to 0)
                    structure->window_grid[random_floor][random_window] =
                        !structure->window_grid[random_floor][random_window];
                }
            }
        }

        SDL_SetRenderDrawColor(g_r, 0, 0, 0, 255);
        SDL_RenderClear(g_r);

        glPointSize(1.0f);
        render_stars(stars, actual_star_count, screen_width, screen_height);

        render_stars(gap_stars, GAP_STAR_COUNT, screen_width, screen_height);

        glColor3f(0.0f, 0.0f, 0.0f);
        for (int build_idx = 0; build_idx < MAX_URBAN_BUILDINGS; build_idx++) {
            UrbanBuilding *structure = &urban_complex[build_idx];
            if (structure->floor_quantity <= 0) continue;
            glBegin(GL_QUADS);
            glVertex2f(structure->x, structure->y);
            glVertex2f(structure->x + structure->width, structure->y);
            glVertex2f(structure->x + structure->width, structure->y + structure->height);
            glVertex2f(structure->x, structure->y + structure->height);
            glEnd();
        }

        // ADD ARCHITECTURAL 3D OUTLINES - Subtle depth suggestion for mass and form
        glLineWidth(2.5f); // Slightly thicker for architectural presence

        for (int build_idx = 0; build_idx < MAX_URBAN_BUILDINGS; build_idx++) {
            UrbanBuilding *structure = &urban_complex[build_idx];
            if (structure->floor_quantity <= 0) continue;

            float build_left = structure->x;
            float build_bottom = structure->y;
            float build_right = structure->x + structure->width;
            float build_top = structure->y + structure->height;

            // SHADOW EDGES - Dark outlines suggesting overhead top-right lighting
            glColor4f(0.1f, 0.1f, 0.1f, 0.7f); // Dark shadow color
            glBegin(GL_LINES);
            // Bottom shadow edge
            glVertex2f(build_left + 1.0f, build_bottom);
            glVertex2f(build_right - 1.0f, build_bottom);
            // Left shadow edge
            glVertex2f(build_left, build_bottom + 1.0f);
            glVertex2f(build_left, build_top - 1.0f);
            glEnd();

            // HIGHLIGHT EDGES - Subtle light outlines for architectural definition
            glColor4f(0.9f, 0.9f, 0.95f, 0.6f); // Subtle light highlight color
            glBegin(GL_LINES);
            // Top highlight edge
            glVertex2f(build_left + 1.0f, build_top);
            glVertex2f(build_right - 1.0f, build_top);
            // Right highlight edge
            glVertex2f(build_right, build_bottom + 1.0f);
            glVertex2f(build_right, build_top - 1.0f);
            glEnd();
        }

        glLineWidth(1.0f); // Reset line width for subsequent rendering

        // Render meteors
        for (int i = 0; i < METEOR_COUNT; i++) {
            if (meteors[i].life > 0) {
                render_meteor(&meteors[i], screen_width, screen_height);
            }
        }

        // CHUNK 2: AIRCRAFT WARNING BEACON SYSTEM - Aviation Safety Lighting
        // Render FAA-compliant red beacons on tall buildings for aircraft safety
        render_aircraft_warning_beacons(screen_width, screen_height);

        // CHUNK 3: COMMUNICATION TOWER SYSTEMS - Cellular Transmission Infrastructure
        // Render steel lattice towers with rotating strobe beacons and antenna arrays
        render_communication_tower_systems(screen_width, screen_height);

        // CHUNK 4: WATER TOWER FACILITY INSTALLATIONS - Hydraulic Infrastructure
        // Render elevated water reservoir towers with pulsing caution lighting
        render_water_tower_facility_installations(screen_width, screen_height);

        // CHUNK 6: ROOF ARCHITECTURAL ACCESSORY COMPLEXITY - Advanced rooftop features
        // Render sophisticated rooftop architectural features like helipads, solar panels, and HVAC
        render_roof_architectural_accessory_complexity(screen_width, screen_height);

        // CHUNK 5: ILLUMINATED WINDOW GRID ALGORITHMS - Building Occupancy Visualization
        // Render intelligent building occupancy visualization with time-sensitive patterns
        render_illuminated_window_grids(screen_width, screen_height);

        SDL_RenderPresent(g_r);
        SDL_Delay(16);
    }

    free(stars);
    free(gap_stars);
    SDL_DestroyRenderer(g_r);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

void usage(const char *prog) {
    fprintf(stderr, "Usage: %s [OPTIONS]\n", prog);
    fprintf(stderr, "Starry Night Screensaver for Hyprland/Wayland\n\n");
    fprintf(stderr, "OPTIONS:\n");
    fprintf(stderr, "  -s F    Speed multiplier (default 1.0)\n");
    fprintf(stderr, "  -d F    Star density 0.0-1.0 (default 0.5)\n");
    fprintf(stderr, "  -m F    Meteor frequency multiplier (default 1.0)\n");
    fprintf(stderr, "  -h      Show this help\n\n");
    fprintf(stderr, "Run with: SDL_VIDEODRIVER=wayland ./starrynight\n");
    fprintf(stderr, "Exit with ESC or mouse/keyboard input after 5s delay\n");
}

void init_opengl(int width, int height) {
    g_w = width;
    g_h = height;

    // Point smoothing for star glow
    glEnable(GL_POINT_SMOOTH);
    glPointSize(1.0f);

    // Enable stencil buffer for building masks
    glEnable(GL_STENCIL_TEST);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    // Disable depth test for 2D rendering
    glDisable(GL_DEPTH_TEST);
}

void init_stars(Star *stars, int count, int screen_width, int screen_height) {
    for (int i = 0; i < count; i++) {
        stars[i].x = (float)(rand() % screen_width);
        stars[i].y = (float)(screen_height / 4 + (rand() % (screen_height * 3 / 4))); // Avoid bottom quarter

        // Gentle drifting motion
        stars[i].vx = -0.1f - (float)(rand() % 4) / 10.0f; // Subtle leftward drift
        stars[i].vy = (float)(rand() % 10 - 5) / 20.0f;    // Very slight vertical drift

        stars[i].base_brightness = 0.5f + (float)(rand() % 5) / 10.0f; // 0.5-1.0
        stars[i].brightness = stars[i].base_brightness;
        stars[i].twinkle_phase = (float)(rand() % 628) / 100.0f; // 0-~6.28 for sin waves
        stars[i].twinkle_speed = 0.5f + (float)(rand() % 150) / 100.0f; // 0.5-2.0 range

        // Size variation
        stars[i].size = 1.0f + (float)(rand() % 20) / 10.0f; // 1.0-3.0 pixels

    // Some stars are extra bright
    stars[i].is_bright = (rand() % 100) < 15; // 15% are bright
    }
}

void update_stars(Star *stars, int count, float dt, int screen_width, int screen_height) {
    static float time = 0;
    time += dt;

    for (int i = 0; i < count; i++) {
        Star *s = &stars[i];

        // Update position with gentle drift (no settling behavior anymore)
        s->x += s->vx * dt;
        s->y += s->vy * dt;

        // Wrap horizontally only
        if (s->x < 0) s->x = screen_width;
        if (s->x > screen_width) s->x = 0;

        // All stars stay within bounds vertically
        if (s->y < 20) s->y = screen_height - 20; // Avoid bottom area for floating stars
        if (s->y > screen_height - 20) s->y = 20;

        // Update twinkling brightness with sine wave
        float twinkle_offset = sinf(time * s->twinkle_speed + s->twinkle_phase) * 0.4f;
        s->brightness = s->base_brightness + twinkle_offset;

        // Clamp brightness
        if (s->brightness < 0.2f) s->brightness = 0.2f;
        if (s->brightness > 1.0f) s->brightness = 1.0f;
    }
}

void render_stars(Star *stars, int count, int screen_width __attribute__((unused)), int screen_height __attribute__((unused))) {
    glBegin(GL_POINTS);

    for (int i = 0; i < count; i++) {
        Star *s = &stars[i];

        // Color: slight yellow tint for warmer stars
        float r = 1.0f, g = 1.0f, b = 0.9f;

        // Extra bright stars get more yellow
        if (s->is_bright) {
            g = 0.95f;
            b = 0.85f;
        }

        // Set star color with brightness
        glColor4f(r, g, b, s->brightness);

        // Draw the star point
        glVertex2f(s->x, s->y);

        // Bright stars get a glow effect (multiple translucent points)
        if (s->is_bright && s->brightness > 0.8f) {
            glColor4f(r, g, b, s->brightness * 0.3f);
            glVertex2f(s->x - 1, s->y);
            glVertex2f(s->x + 1, s->y);
            glVertex2f(s->x, s->y - 1);
            glVertex2f(s->x, s->y + 1);
        }
    }

    glEnd();
}

void init_meteor(Meteor *meteor, int screen_width, int screen_height) {
    // Random start position within visible sky area, well above all buildings
    // Maximum building height ~20% of screen + 50px base = ensure 30% safe margin
    int min_sky_y = screen_height * 0.3f; // 30% down screen from top (well above tallest buildings)

    meteor->x = rand() % screen_width; // Anywhere across full screen width (0 to screen_width)
    meteor->y = min_sky_y + rand() % (screen_height - min_sky_y); // From 30% height to top

    // Random direction and speed for natural-looking movement across sky
    // Movement should generally be downward and horizontal for realistic meteor trajectories
    float angle = (rand() % 314) / 100.0f; // Random angle 0-3.14 radians
    float speed = 150 + rand() % 200; // 150-350 pixels/second

    // Convert angle to velocity components, weighted toward diagonal movement
    meteor->vx = cosf(angle) * speed;
    meteor->vy = sinf(angle) * speed * 0.7f; // Slightly less vertical for more horizontal movement
    meteor->vy += 50; // Add steady downward component for visibility

    meteor->life = 1.0f; // Full life
    meteor->active = true;

    // Initialize particle trail positions
    for (int i = 0; i < METEOR_PARTICLES; i++) {
        meteor->tail_particlesx[i] = meteor->x;
        meteor->tail_particlesy[i] = meteor->y;
        meteor->tail_alphas[i] = 0.0f;
    }
}

void update_meteor(Meteor *meteor, float dt, int screen_width,
                  __attribute__((unused)) int screen_height) {
    if (meteor->life <= 0) return;

    meteor->x += meteor->vx * dt;
    meteor->y -= meteor->vy * dt; // Downward
    meteor->life -= dt * 1.2f;    // Fade over ~1.2-2.0 seconds

    // Update particle trail
    for (int i = METEOR_PARTICLES - 1; i > 0; i--) {
        meteor->tail_particlesx[i] = meteor->tail_particlesx[i-1];
        meteor->tail_particlesy[i] = meteor->tail_particlesy[i-1];
        meteor->tail_alphas[i] = meteor->tail_alphas[i-1];
    }

    meteor->tail_particlesx[0] = meteor->x;
    meteor->tail_particlesy[0] = meteor->y;
    meteor->tail_alphas[0] = meteor->life;

    // Deactivate when off screen or faded
    if (meteor->life <= 0 || meteor->x > screen_width + 100 || meteor->y < -100) {
        meteor->active = false;
        meteor->life = 0;
    }
}

void render_meteor(Meteor *meteor, int screen_width __attribute__((unused)), int screen_height __attribute__((unused))) {
    if (meteor->life <= 0 || !meteor->active) return;

    // Render particle trail
    glBegin(GL_POINTS);
    glColor3f(0.8f, 0.9f, 1.0f); // Bright blue-white

    for (int i = 0; i < METEOR_PARTICLES; i++) {
        if (meteor->tail_alphas[i] > 0.1f) {
            glColor4f(0.8f, 0.9f, 1.0f, meteor->tail_alphas[i]);
            glVertex2f(meteor->tail_particlesx[i], meteor->tail_particlesy[i]);
        }
    }
    glEnd();

    // Render EXTRA-BRIGHT head of meteor with enhanced glow
    glPointSize(4.0f); // Larger head for extra brightness

    // Main bright center core
    glBegin(GL_POINTS);
    glColor4f(1.0f, 1.0f, 1.0f, meteor->life * 1.2f); // Slightly brighter than max
    glVertex2f(meteor->x, meteor->y);
    glEnd();

    // GLOWING AURA - Extra brightness halo
    glPointSize(8.0f); // Wide glow halo
    glBegin(GL_POINTS);
    glColor4f(1.0f, 1.0f, 1.0f, meteor->life * 0.6f); // Dimmer halo for glow effect
    glVertex2f(meteor->x, meteor->y);
    glEnd();
}

/**
 * URBAN BUILDING INITIALIZATION FUNCTION (Chunk 1 Core Implementation)
 * Generates sophisticated architectural configurations with building archetypes
 */
void initialize_urban_complex_generation(int screen_width __attribute__((unused)), int screen_height __attribute__((unused))) {
    for (int build_index = 0; build_index < MAX_URBAN_BUILDINGS; build_index++) {
        UrbanBuilding* urban_structure = &urban_complex[build_index];

        // DENSE CITYSCAPE POSITIONING - Pack buildings tightly without gaps
        // Calculate building position based on index - all buildings touch each other
        if (build_index == 0) {
            urban_structure->x = 5.0f; // Start 5 pixels from left edge
        } else {
            // Each building starts right after the previous one's end
            UrbanBuilding* prev_structure = &urban_complex[build_index - 1];
            urban_structure->x = prev_structure->x + prev_structure->width;
        }

        urban_structure->y = 50.0f; // Ground floor datum consistency

        // ARCHITECTURAL PROFILE DETERMINATION - 11 Building Archetype System
        int architectural_classification = rand() % 11; // Sophisticated typology selection
        urban_structure->building_type = architectural_classification;

        // BUILDING SPECIFICATION ENGINE - Feature-Driven Architectural Synthesis
        // Enhanced for cityscape appearance with 20% screen height limits (±10% variability) and wider buildings
        // Maximum height = 20% of available screen space (screen_height-50) with 10% variability
        float base_height_cap = (screen_height - 50) * 0.2f;
        float height_variability = base_height_cap * 0.1f; // ±10% range
        float height_random_factor = (rand() % 201 - 100) / 100.0f; // -1.0 to +1.0
        float max_building_height = base_height_cap + height_random_factor * height_variability;

        switch (architectural_classification) {
            case -1: // FORCE HEIGHT CAP ENFORCEMENT
                // This case should never be reached - ensures compiler doesn't complain about unused variable
                if (max_building_height < 50) max_building_height = 50; // Minimum safe height
                break;
            case 0: // RESIDENTIAL APARTMENT COMPLEX (2-10 Stories, Balconies/Balconies Design)
                urban_structure->floor_quantity = 2 + (rand() % 9);
                urban_structure->height = urban_structure->floor_quantity * 20.0f * 1.2f; // 20% height increase
                urban_structure->height = (urban_structure->height > max_building_height) ? max_building_height : urban_structure->height; // Cap at 20%
                urban_structure->width = 20.0f + (float)(rand() % 30); // Wider range: 20-50 pixels
                urban_structure->illumination_percentage = 0.7f;
                urban_structure->illumination_pattern_type = 0; // Residential nighttime energy profile
                break;

            case 1: // OFFICE FINANCIAL CENTER (15-35 Stories, Glass Curtain Architecture)
                urban_structure->floor_quantity = 15 + (rand() % 21);
                urban_structure->height = urban_structure->floor_quantity * 18.0f * 1.2f; // 20% height increase
                urban_structure->height = (urban_structure->height > max_building_height) ? max_building_height : urban_structure->height; // Cap at 20%
                urban_structure->width = 28.0f + (float)(rand() % 35); // Wider range: 28-63 pixels
                urban_structure->illumination_percentage = 0.9f;
                urban_structure->illumination_pattern_type = 1; // Business hour diurnal cycle
                // Prepare for aircraft beacon installation (Chunk 2)
                urban_structure->aircraft_warning_beacon_present = (urban_structure->floor_quantity >= 30);
                break;

            case 2: // MEGATOWER CONSTRUCTION (40-80 Stories, Supertall Architectural Monument)
                urban_structure->floor_quantity = 40 + (rand() % 41);
                urban_structure->height = urban_structure->floor_quantity * 16.5f * 1.2f; // 20% height increase
                urban_structure->height = (urban_structure->height > max_building_height) ? max_building_height : urban_structure->height; // Cap at 20%
                urban_structure->width = 30.0f + (float)(rand() % 40); // Wider range: 30-70 pixels
                urban_structure->illumination_percentage = 1.0f;
                urban_structure->illumination_pattern_type = 2; // 24/7 operational criticality
                urban_structure->aircraft_warning_beacon_present = 1; // Mandatory aviation safety
                // REMOVED: urban_structure->antenna_element_array = 1 + (rand() % 3); - antennas controlled by global system
                break;

            case 3: // HOSPITAL MEDICAL FACILITY (8-20 Stories, Emergency Illumination)
                urban_structure->floor_quantity = 8 + (rand() % 13);
                urban_structure->height = urban_structure->floor_quantity * 22.0f * 1.2f; // 20% height increase
                urban_structure->height = (urban_structure->height > max_building_height) ? max_building_height : urban_structure->height; // Cap at 20%
                urban_structure->width = 22.0f + (float)(rand() % 32); // Wider range: 22-54 pixels
                urban_structure->illumination_percentage = 1.0f;
                urban_structure->illumination_pattern_type = 3; // 24-hour medical operation
                urban_structure->aircraft_warning_beacon_present = (urban_structure->floor_quantity >= 15);
                break;

            case 4: // EDUCATIONAL ACADEMIC INSTITUTE (6-15 Stories, Classroom Configuration)
                urban_structure->floor_quantity = 6 + (rand() % 10);
                urban_structure->height = urban_structure->floor_quantity * 19.0f * 1.2f; // 20% height increase
                urban_structure->height = (urban_structure->height > max_building_height) ? max_building_height : urban_structure->height; // Cap at 20%
                urban_structure->width = 26.0f + (float)(rand() % 28); // Wider range: 26-54 pixels
                urban_structure->illumination_percentage = 0.6f;
                urban_structure->illumination_pattern_type = 4; // Academic scheduling cycle
                urban_structure->architectural_significance = 1.2f; // Cultural weighting factor
                break;

            case 5: // COMMERCIAL BUSINESS DISTRICT (10-25 Stories, Neon Advertising)
                urban_structure->floor_quantity = 10 + (rand() % 16);
                urban_structure->height = urban_structure->floor_quantity * 17.5f * 1.2f; // 20% height increase
                urban_structure->height = (urban_structure->height > max_building_height) ? max_building_height : urban_structure->height; // Cap at 20%
                urban_structure->width = 25.0f + (float)(rand() % 31); // Wider range: 25-56 pixels
                urban_structure->illumination_percentage = 0.85f;
                urban_structure->illumination_pattern_type = 5; // Late-night commercial economy
                break;

            case 6: // INDUSTRIAL MANUFACTURING COMPLEX (3-8 Stories, Ventilation Systems)
                urban_structure->floor_quantity = 3 + (rand() % 6);
                urban_structure->height = urban_structure->floor_quantity * 25.0f * 1.2f; // 20% height increase
                urban_structure->height = (urban_structure->height > max_building_height) ? max_building_height : urban_structure->height; // Cap at 20%
                urban_structure->width = 18.0f + (float)(rand() % 28); // Wider range: 18-46 pixels
                urban_structure->illumination_percentage = 0.8f;
                urban_structure->illumination_pattern_type = 6; // First/third shift operational cycles
                urban_structure->roof_feature_mask |= (1 << ROOF_VENTILATIONS); // Industrial specific
                break;

            case 7: // CULTURAL INSTITUTION VENUE (12-25 Stories, Performance Hall Architecture)
                urban_structure->floor_quantity = 12 + (rand() % 14);
                urban_structure->height = urban_structure->floor_quantity * 20.0f * 1.2f; // 20% height increase
                urban_structure->height = (urban_structure->height > max_building_height) ? max_building_height : urban_structure->height; // Cap at 20%
                urban_structure->width = 27.0f + (float)(rand() % 29); // Wider range: 27-56 pixels
                urban_structure->illumination_percentage = 0.4f;
                urban_structure->illumination_pattern_type = 7; // Event-based illumination patterns
                urban_structure->architectural_significance = 1.5f; // Artistic importance multiplier
                break;

            case 8: // RESEARCH LABORATORY COMPLEX (8-18 Stories, Specialized Ventilation)
                urban_structure->floor_quantity = 8 + (rand() % 11);
                urban_structure->height = urban_structure->floor_quantity * 21.0f * 1.2f; // 20% height increase
                urban_structure->height = (urban_structure->height > max_building_height) ? max_building_height : urban_structure->height; // Cap at 20%
                urban_structure->width = 24.0f + (float)(rand() % 30); // Wider range: 24-54 pixels
                urban_structure->illumination_percentage = 1.0f;
                urban_structure->illumination_pattern_type = 8; // Continuous operational criticality
                break;

            case 9: // RETAIL SHOPPING COMPLEX (2-6 Stories, Aluminum Framing)
                urban_structure->floor_quantity = 2 + (rand() % 5);
                urban_structure->height = urban_structure->floor_quantity * 28.0f * 1.2f; // 20% height increase
                urban_structure->height = (urban_structure->height > max_building_height) ? max_building_height : urban_structure->height; // Cap at 20%
                urban_structure->width = 20.0f + (float)(rand() % 36); // Wider range: 20-56 pixels
                urban_structure->illumination_percentage = 0.75f;
                urban_structure->illumination_pattern_type = 9; // Retail business hour cycle
                break;

            case 10: // CONVENTION EVENT FACILITY (4-12 Stories, Exhibition Architecture)
                urban_structure->floor_quantity = 4 + (rand() % 9);
                urban_structure->height = urban_structure->floor_quantity * 23.0f * 1.2f; // 20% height increase
                urban_structure->height = (urban_structure->height > max_building_height) ? max_building_height : urban_structure->height; // Cap at 20%
                urban_structure->width = 29.0f + (float)(rand() % 30); // Wider range: 29-59 pixels
                urban_structure->illumination_percentage = 0.3f;
                urban_structure->illumination_pattern_type = 10; // Convention schedule coordination
                urban_structure->architectural_significance = 1.3f; // Convention center importance
                break;
        }

        // OLD PROBABILITY SYSTEM REMOVED - Replaced with ultra-sparse controlled infrastructure assignment
        // All rooftop features now explicitly managed via global infrastructure placement system above

        // Specialist building features for high-rise structures ( maintenace crane as backup only)
        if (urban_structure->floor_quantity >= 40 && !(urban_structure->roof_feature_mask)) {
            // Only buildings without other infrastructure get the maintenance crane as backup
            urban_structure->roof_feature_mask |= (1 << ROOF_MAINTENANCE_CRANE);
        }

        // ULTRA-SPARSE ROOFTOP INFRASTRUCTURE PLACEMENT - Ultimate Realism
        // Exactly 2 infrastructure items per type = 10 total objects across entire screen
        // No building gets more than one item, distributed like real-world city planning

        // Global infrastructure placement array - 10 slots for individual items
        static int infrastructure_placements[10]; // Index 0-1: antennas, 2-3: towers, etc.
        static int placements_initialized = 0;

        // Initialize the 10 infrastructure placements on first building generation
        if (placements_initialized == 0) {
            // Track which buildings are already occupied
            static int occupied_buildings[MAX_URBAN_BUILDINGS];
            for (int i = 0; i < MAX_URBAN_BUILDINGS; i++) {
                occupied_buildings[i] = 0; // 0 = available
            }

            // Infrastructure placement logic (feature_types no longer needed in new system)

            for (int type_idx = 0; type_idx < 5; type_idx++) {
                for (int instance = 0; instance < 2; instance++) { // 2 per type
                    // Array index: type 0 slots 0-1, type 1 slots 2-3, etc.
                    int placement_slot = type_idx * 2 + instance;

                    // Find an unassigned building
                    int building_attempts = 0;
                    while (building_attempts < MAX_URBAN_BUILDINGS) {
                        int candidate_building = rand() % MAX_URBAN_BUILDINGS;
                        if (occupied_buildings[candidate_building] == 0) {
                            occupied_buildings[candidate_building] = 1; // Mark as occupied
                            infrastructure_placements[placement_slot] = candidate_building;
                            break;
                        }
                        building_attempts++;
                    }
                }
            }

            placements_initialized = 1;
        }

        // Check if this building is assigned to host exactly 2 antennas and 2 towers across entire screen
        // Only these specific infrastructure items will be visible on rooftops
        for (int placement_idx = 0; placement_idx < 10; placement_idx++) {
            if (infrastructure_placements[placement_idx] == build_index) {
                // This building hosts this infrastructure item
                int feature_type = -1;

                // EXPANDED ULTRA-SPARSE ASSIGNMENT: All infrastructure types with 2 per type total
                // slots 0-1 = antennas, 2-3 = towers, 4 = helipad, 5 = solar, 6 = HVAC, 7 = religious, 8 = blimp, 9 = water tower
                if (placement_idx == 0 || placement_idx == 1) {
                    feature_type = ROOF_TRANSMISSION_TOWER; // Antennas on towers
                } else if (placement_idx == 2 || placement_idx == 3) {
                    feature_type = ROOF_TRANSMISSION_TOWER; // Separate tower structures
                } else if (placement_idx == 4) {
                    feature_type = ROOF_HELIPAD_PLATFORM; // Helipad
                } else if (placement_idx == 5) {
                    feature_type = ROOF_SOLAR_PANEL_ARRAY; // Solar panels
                } else if (placement_idx == 6) {
                    feature_type = ROOF_HVAC_UNITS; // HVAC equipment
                } else if (placement_idx == 7) {
                    feature_type = ROOF_RELIGIOUS_SYMBOLS; // Religious crosses
                } else if (placement_idx == 8) {
                    feature_type = ROOF_SURVEILLANCE_BLIMP; // Surveillance blimp
                } else if (placement_idx == 9) {
                    feature_type = ROOF_RESERVOIR_TOWER; // Water tower
                }

                if (feature_type != -1) {
                    urban_structure->roof_feature_mask |= (1 << feature_type);
                    // Ensure single antenna per tower for exact 2-antennas-visible requirement
                    urban_structure->antenna_element_array = 1;
                }
                // Each building gets at most one item, so we can break after finding
                break;
            }
        }

        // Add a backup tower placement for tall buildings (this is not part of the strict 2-per-type system)
        if (urban_structure->floor_quantity >= 40 && !(urban_structure->roof_feature_mask)) {
            urban_structure->roof_feature_mask |= (1 << ROOF_MAINTENANCE_CRANE);
        }

        // Boundary calculation and spatial validation
        urban_structure->right_edge = urban_structure->x + urban_structure->width;
        urban_structure->window_count_horizontal = 2 + (rand() % 6); // Windows per floor variation

        // Roof level elevation calculation for future three-dimensional features
        urban_structure->roof_level_elevation = urban_structure->y + urban_structure->height;

        // Advanced lighting preparation (coordination system for Chunks 2-10)
        urban_structure->pulse_synchronization_timer = (float)rand() / RAND_MAX * 2.0f * PI; // Randomized phase

        // CHUNK 5: WINDOW GRID INITIALIZATION - Setup illuminated window patterns
        initialize_window_illumination_patterns();
    }
}

/**
 * URBAN LIGHTING INFRASTRUCTURE INITIALIZATION (Chunk 1 Lighting Framework)
 * Establishes dynamic illumination management system for future chunks
 */
void establish_urban_lighting_infrastructure() {
    // RESET ILLUMINATION ARRAY - Clean slate for sophisticated lighting (Chunks 2-10)
    for (int illumination_index = 0; illumination_index < LIGHTING_SYSTEM_LIMIT; illumination_index++) {
        DynamicLightingElement* light_unit = &illumination_array[illumination_index];

        // Initialize with zero values for system integrity
        light_unit->positional_coordinates_x = light_unit->positional_coordinates_y = 0.0f;
        light_unit->illumination_intensity = 0.0f;
        light_unit->temporal_animation_cycle = 0.0f;
        light_unit->structural_attachment_index = -1;
        light_unit->illumination_classification = 0;
        light_unit->operational_status_flag = 0;
    }

    // ARCHITECTURAL CATALOG POPULATION - Define available roof feature components
    architectural_catalog[0].architectural_description = "Water Reservoir Tower";
    architectural_catalog[1].architectural_description = "Transmission Antenna Array";
    architectural_catalog[2].architectural_description = "Helicopter Landing Platform";
    architectural_catalog[3].architectural_description = "Maintenance Construction Crane";
    architectural_catalog[4].architectural_description = "Industrial Ventilation Systems";

    // Initialize other architectural catalog properties as needed
    for (int i = 0; i < ROOF_FEATURE_ARRAYS; i++) {
        architectural_catalog[i].vertical_dimension_requirements = 0;
        architectural_catalog[i].illumination_theme_color_primary = 0;
        architectural_catalog[i].illumination_theme_color_secondary = 0;
        architectural_catalog[i].collision_detection_volume = 0;
        architectural_catalog[i].operational_power_consumption = 0;
        architectural_catalog[i].maintenance_service_interval = 0;
    }
}

/**
 * AIRCRAFT WARNING BEACON RENDERING (Chunk 2 Implementation)
 * Renders FAA-compliant red flashing beacons on tall buildings for aviation safety
 */
void render_aircraft_warning_beacons(int screen_width __attribute__((unused)), int screen_height __attribute__((unused))) {
    // static float global_beacon_timer = 0.0f; // Reserved for future timing coordination

    for (int building_index = 0; building_index < MAX_URBAN_BUILDINGS; building_index++) {
        UrbanBuilding* structure = &urban_complex[building_index];

        // HEIGHT-BASED COMPLIANCE TRIGGERING - FAA Aviation Safety Specifications
        if (!structure->aircraft_warning_beacon_present) {
            continue; // Building doesn't qualify for aviation lighting
        }

        // BEACON POSITIONING CALCULATION - Rooftop placement with regulatory compliance
        float beacon_x = structure->x + structure->width / 2.0f; // Center of building
        float beacon_y_offset = 5.0f; // Small elevation above roof for visibility
        float beacon_y = structure->roof_level_elevation + beacon_y_offset;

        // REGULATORY BEACON QUANTITY DETERMINATION
        int beacon_count = (structure->floor_quantity >= 50) ? 2 : 1; // Dual for megastructures

        for (int beacon_instance = 0; beacon_instance < beacon_count; beacon_instance++) {
            // DUAL BEACON POSITIONING - Offset for second beacon on megastructures
            float x_offset = (beacon_count == 2 && beacon_instance == 1) ? structure->width * 0.25f : 0.0f;
            float current_beacon_x = beacon_x + x_offset;
            float current_beacon_y = beacon_y;

            // BEACON ANIMATED ILLUMINATION CYCLE - 1.5s period with 1.0s active time
            structure->pulse_synchronization_timer += 0.016f; // ~60 FPS increment
            float cycle_position = fmodf(structure->pulse_synchronization_timer, AIRCRAFT_BEACON_BLINK_PERIOD);
            int beacon_lit = (cycle_position < AIRCRAFT_BEACON_ACTIVE_TIME);

            if (beacon_lit) {
                // FAA-CERTIFIED RED AVIATION COLOR - RGB(1.0, 0.0, 0.0)
                glColor4f(1.0f, 0.0f, 0.0f, 1.0f); // Pure red, fully opaque

                // EMERGENCY ILLUMINATION RENDERING - Point geometry with safety visibility
                glPointSize(AIRCRAFT_BEACON_DIAMETER);

                glBegin(GL_POINTS);
                glVertex2f(current_beacon_x, current_beacon_y);
                glEnd();

                // AURA GLOW EFFECT - Enhanced visibility for aviation safety
                glPointSize(AIRCRAFT_BEACON_DIAMETER * 1.5f);
                glColor4f(1.0f, 0.0f, 0.0f, 0.4f); // Semi-transparent red glow

                glBegin(GL_POINTS);
                glVertex2f(current_beacon_x, current_beacon_y);
                glEnd();
            }
        }

        // REGULATORY FLASH SYNCHRONIZATION - Prevent simultaneous activation hazards
        // Different phase offsets ensure aviation safety spacing between adjacent structures
    }

    // RESET RENDER STATE - Restore point size for subsequent rendering layers
    glPointSize(1.0f); // Reset to default for stars
}

/**
 * COMMUNICATION TOWER SYSTEMS (Chunk 3 Implementation)
 * Renders cellular transmission infrastructure with rotating strobe beacons
 */
void render_communication_tower_systems(int screen_width __attribute__((unused)), int screen_height __attribute__((unused))) {
    static float global_rotation_timer = 0.0f; // Smooth accumulated timing for beacon rotation
    global_rotation_timer += 0.016f; // ~60 FPS increment for consistent timing

    for (int building_index = 0; building_index < MAX_URBAN_BUILDINGS; building_index++) {
        UrbanBuilding* structure = &urban_complex[building_index];

        // TRANSMISSION INFRASTRUCTURE PRESENCE VERIFICATION
        if (!(structure->roof_feature_mask & (1 << ROOF_TRANSMISSION_TOWER))) {
            continue; // No communication tower on this building
        }

        // TOWER POSITIONING CALCULATION - Centered on building rooftop
        float tower_base_x = structure->x + structure->width / 2.0f; // Center of building
        float tower_base_y = structure->y + structure->height - 10.0f; // Near roof level

        // TOWER DIMENSIONS - Use fixed values from structure initialization
        float tower_height = (float)structure->tower_height_pixels; // Fixed 35-60 pixel range

        // STRUCTURAL LATTICE RENDERING - Thin line geometry for steel appearance
        glColor4f(0.3f, 0.3f, 0.3f, 0.8f); // Dark gray steel color
        glLineWidth(2.0f);

        glBegin(GL_LINES);
        // Main vertical tower shaft
        glVertex2f(tower_base_x, tower_base_y);
        glVertex2f(tower_base_x, tower_base_y + tower_height);

        // Lateral bracing elements
        float bracing_levels = 4.0f;
        for (int level = 1; level <= (int)bracing_levels; level++) {
            float brace_y = tower_base_y + (tower_height / bracing_levels) * (float)level;
            float brace_width = 8.0f - (float)level; // Decreasing width upward

            // Cross bracing - lattice appearance
            glVertex2f(tower_base_x - brace_width/2.0f, brace_y);
            glVertex2f(tower_base_x + brace_width/2.0f, brace_y);
        }
        glEnd();

        // ANTENNA ARRAY RENDITION - Multi-element microwave transceiver systems
        float antenna_top_y = tower_base_y + tower_height;
        int antenna_count = structure->antenna_element_array;

        glColor4f(0.8f, 0.8f, 0.8f, 0.9f); // Light gray antenna color
        glBegin(GL_LINES);
        for (int antenna_idx = 0; antenna_idx < antenna_count; antenna_idx++) {
            float antenna_x = tower_base_x + (float)(antenna_idx - antenna_count/2) * 4.0f;
            glVertex2f(antenna_x, antenna_top_y);
            glVertex2f(antenna_x, antenna_top_y + 8.0f);
        }
        glEnd();

        // ROTARY STROBE BEACON SYSTEM - 360° sweeping white/high-vis patterns
        float beacon_center_y = antenna_top_y + 10.0f;

        // TIME-DEPENDENT ROTATION - Continuous 360° sweeping animation
        float rotation_speed = 120.0f; // Degrees per second
        float rotation_angle = fmodf(global_rotation_timer * rotation_speed, 360.0f);

        // BEACON ILLUMINATION PATTERN - High-visibility white strobe
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f); // Pure white

        // Rotating beacon beam representation
        float beam_length = 25.0f;
        float beam_angle_width = 15.0f; // 15-degree beam width

        // Calculate beam endpoints
        float beam_start_angle = rotation_angle - beam_angle_width/2.0f;
        float beam_end_angle = rotation_angle + beam_angle_width/2.0f;

        // Convert to radians for OpenGL
        float start_rad = beam_start_angle * PI / 180.0f;
        float end_rad = beam_end_angle * PI / 180.0f;

        glBegin(GL_TRIANGLES);
        // Sweeping beam - triangle from beacon center to endpoints
        glVertex2f(tower_base_x, beacon_center_y);
        glVertex2f(tower_base_x + cosf(start_rad) * beam_length, beacon_center_y + sinf(start_rad) * beam_length);
        glVertex2f(tower_base_x + cosf(end_rad) * beam_length, beacon_center_y + sinf(end_rad) * beam_length);
        glEnd();

        // BEACON CENTER INDICATOR - Pulsing white light source
        glPointSize(3.0f);
        glBegin(GL_POINTS);
        glVertex2f(tower_base_x, beacon_center_y);
        glEnd();

        // AURA GLOW EFFECT - Enhanced visibility for communication systems
        glPointSize(5.0f);
        glColor4f(1.0f, 1.0f, 1.0f, 0.6f); // Semi-transparent white glow

        glBegin(GL_POINTS);
        glVertex2f(tower_base_x, beacon_center_y);
        glEnd();
    }

    // RESET RENDER STATE - Restore defaults for subsequent rendering
    glLineWidth(1.0f); // Reset line width
    glPointSize(1.0f); // Reset point size
}
