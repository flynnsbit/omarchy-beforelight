/* Immediate-mode GL subset on SDL_Renderer. GL y=0 is the bottom. */
#pragma once

#include <SDL2/SDL.h>
#include <math.h>
#include <string.h>

#define GL_POINTS 0
#define GL_LINES 1
#define GL_QUADS 2
#define GL_TRIANGLE_FAN 3
#define GL_TRIANGLES 4
#define GL_POINT_SMOOTH 0
#define GL_STENCIL_TEST 0
#define GL_DEPTH_TEST 0
#define GL_TRUE 1
#define GL_FALSE 0

static SDL_Renderer *g_r;
static int g_w, g_h;
static float g_cr = 1, g_cg = 1, g_cb = 1, g_ca = 1;
static float g_point_size = 1, g_line_width = 1;
static int g_prim;
#define SN_MAX_V 64
static float g_vx[SN_MAX_V], g_vy[SN_MAX_V];
static int g_vn;

static int sn_sy(float y) {
    int yy = (int)lroundf((float)g_h - y);
    return yy;
}

static void sn_apply_color(void) {
    SDL_SetRenderDrawBlendMode(g_r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(g_r,
        (Uint8)(g_cr * 255.0f + 0.5f),
        (Uint8)(g_cg * 255.0f + 0.5f),
        (Uint8)(g_cb * 255.0f + 0.5f),
        (Uint8)(g_ca * 255.0f + 0.5f));
}

static void sn_fill_circle(float cx, float cy, float radius) {
    int r = (int)lroundf(radius);
    int x = (int)lroundf(cx);
    int y = sn_sy(cy);
    if (r < 1) {
        SDL_RenderDrawPoint(g_r, x, y);
        return;
    }
    for (int dy = -r; dy <= r; dy++) {
        float span2 = (float)(r * r - dy * dy);
        if (span2 < 0)
            continue;
        int dx = (int)lroundf(sqrtf(span2));
        SDL_RenderDrawLine(g_r, x - dx, y + dy, x + dx, y + dy);
    }
}

static void sn_draw_point(float x, float y) {
    sn_apply_color();
    int s = (int)lroundf(g_point_size);
    if (s <= 1)
        SDL_RenderDrawPoint(g_r, (int)lroundf(x), sn_sy(y));
    else
        sn_fill_circle(x, y, s * 0.5f);
}

static void glColor4f(float r, float g, float b, float a) {
    g_cr = r;
    g_cg = g;
    g_cb = b;
    g_ca = a;
}

static void glColor3f(float r, float g, float b) {
    glColor4f(r, g, b, 1.0f);
}

static void glPointSize(float s) { g_point_size = s; }
static void glLineWidth(float w) { g_line_width = w; }

static void glBegin(int mode) {
    g_prim = mode;
    g_vn = 0;
}

static void glVertex2f(float x, float y) {
    if (g_prim == GL_POINTS) {
        sn_draw_point(x, y);
        return;
    }
    if (g_vn < SN_MAX_V) {
        g_vx[g_vn] = x;
        g_vy[g_vn] = y;
        g_vn++;
    }
}

static void glEnd(void) {
    sn_apply_color();
    if (g_prim == GL_LINES) {
        int w = (int)lroundf(g_line_width);
        if (w < 1)
            w = 1;
        for (int i = 0; i + 1 < g_vn; i += 2) {
            int x1 = (int)lroundf(g_vx[i]);
            int y1 = sn_sy(g_vy[i]);
            int x2 = (int)lroundf(g_vx[i + 1]);
            int y2 = sn_sy(g_vy[i + 1]);
            for (int o = -(w / 2); o <= w / 2; o++)
                SDL_RenderDrawLine(g_r, x1, y1 + o, x2, y2 + o);
        }
    } else if (g_prim == GL_QUADS) {
        for (int i = 0; i + 3 < g_vn; i += 4) {
            float minx = fminf(fminf(g_vx[i], g_vx[i + 1]), fminf(g_vx[i + 2], g_vx[i + 3]));
            float maxx = fmaxf(fmaxf(g_vx[i], g_vx[i + 1]), fmaxf(g_vx[i + 2], g_vx[i + 3]));
            float miny = fminf(fminf(g_vy[i], g_vy[i + 1]), fminf(g_vy[i + 2], g_vy[i + 3]));
            float maxy = fmaxf(fmaxf(g_vy[i], g_vy[i + 1]), fmaxf(g_vy[i + 2], g_vy[i + 3]));
            SDL_Rect r = {
                (int)lroundf(minx),
                sn_sy(maxy),
                (int)lroundf(maxx - minx),
                (int)lroundf(maxy - miny)
            };
            if (r.w < 1)
                r.w = 1;
            if (r.h < 1)
                r.h = 1;
            SDL_RenderFillRect(g_r, &r);
        }
    } else if (g_prim == GL_TRIANGLE_FAN && g_vn >= 3) {
        float cx = g_vx[0], cy = g_vy[0];
        float rad = hypotf(g_vx[1] - cx, g_vy[1] - cy);
        sn_fill_circle(cx, cy, rad);
    } else if (g_prim == GL_TRIANGLES) {
        for (int i = 0; i + 2 < g_vn; i += 3) {
            float x0 = g_vx[i], y0 = g_vy[i];
            float x1 = g_vx[i + 1], y1 = g_vy[i + 1];
            float x2 = g_vx[i + 2], y2 = g_vy[i + 2];
            int minx = (int)floorf(fminf(fminf(x0, x1), x2));
            int maxx = (int)ceilf(fmaxf(fmaxf(x0, x1), x2));
            int miny = (int)floorf(fminf(fminf(y0, y1), y2));
            int maxy = (int)ceilf(fmaxf(fmaxf(y0, y1), y2));
            float den = (y1 - y2) * (x0 - x2) + (x2 - x1) * (y0 - y2);
            if (fabsf(den) < 1e-6f)
                continue;
            for (int y = miny; y <= maxy; y++) {
                for (int x = minx; x <= maxx; x++) {
                    float w0 = ((y1 - y2) * (x - x2) + (x2 - x1) * (y - y2)) / den;
                    float w1 = ((y2 - y0) * (x - x2) + (x0 - x2) * (y - y2)) / den;
                    float w2 = 1.0f - w0 - w1;
                    if (w0 >= 0 && w1 >= 0 && w2 >= 0)
                        SDL_RenderDrawPoint(g_r, x, sn_sy((float)y));
                }
            }
        }
    }
    g_vn = 0;
}

static void glClear(int mask) { (void)mask; }
static void glClearStencil(int v) { (void)v; }
static void glStencilMask(unsigned m) { (void)m; }
static void glColorMask(int r, int g, int b, int a) { (void)r; (void)g; (void)b; (void)a; }
static void glStencilFunc(int f, int ref, unsigned m) { (void)f; (void)ref; (void)m; }
static void glStencilOp(int a, int b, int c) { (void)a; (void)b; (void)c; }
static void glDisable(int cap) { (void)cap; }
static void glEnable(int cap) { (void)cap; }
static void glBlendFunc(int a, int b) { (void)a; (void)b; }
static void glViewport(int x, int y, int w, int h) { (void)x; (void)y; (void)w; (void)h; }
static void glMatrixMode(int m) { (void)m; }
static void glLoadIdentity(void) {}
static void gluOrtho2D(double a, double b, double c, double d) { (void)a; (void)b; (void)c; (void)d; }
static void glClearColor(float r, float g, float b, float a) { (void)r; (void)g; (void)b; (void)a; }

#define GL_COLOR_BUFFER_BIT 0
#define GL_STENCIL_BUFFER_BIT 0
#define GL_ALWAYS 0
#define GL_EQUAL 0
#define GL_KEEP 0
#define GL_REPLACE 0
#define GL_SCISSOR_TEST 0
#define GL_BLEND 0
#define GL_SRC_ALPHA 0
#define GL_ONE_MINUS_SRC_ALPHA 0
#define GL_PROJECTION 0
#define GL_MODELVIEW 0
