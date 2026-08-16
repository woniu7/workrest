// SDL3 view implementation: fullscreen window showing the countdown + keyboard input.
//
// Unlike gui.c this file is shared by every platform (that is the point of SDL). The core
// runs on the platform's own main loop, not on SDL's, so nobody would ever drain the SDL
// event queue: a repeating platform timer does it instead.
//
// Text is drawn with SDL's built-in debug font, so there is no SDL_ttf dependency and no
// font file to ship; digits are just scaled up to fill the screen.
#include "view.h"
#include "rest.h"
#include "platform.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PUMP_MS_RESTING 16   // Window visible: ~60 fps, keys feel instant
#define PUMP_MS_WORKING 500  // Window hidden: just keep the event queue drained

static RestCore     *g_core     = NULL;
static SDL_Window   *g_window   = NULL;
static SDL_Renderer *g_renderer = NULL;
static int           g_seconds  = 0;     // Currently displayed seconds
static bool          g_visible  = false; // Resting (window shown) or working (hidden)
static PlatTimer     g_pump     = PLAT_TIMER_NONE;

// Draw the large countdown number, centered on a red background
static void draw(void) {
    int   w = 0, h = 0;
    float scale;
    char  buf[16];

    if (!g_renderer) return;
    SDL_GetRenderOutputSize(g_renderer, &w, &h);

    SDL_SetRenderScale(g_renderer, 1.0f, 1.0f);
    SDL_SetRenderDrawColor(g_renderer, 200, 0, 0, 255); // Red, like the GTK/Win32 views
    SDL_RenderClear(g_renderer);

    snprintf(buf, sizeof(buf), "%d", g_seconds);

    // The debug font is a fixed 8x8 px per glyph, so scale it up to ~40% of the window height
    scale = (float)h * 0.4f / SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE;
    if (scale < 1.0f) scale = 1.0f;
    SDL_SetRenderScale(g_renderer, scale, scale);
    SDL_SetRenderDrawColor(g_renderer, 255, 255, 255, 255); // White
    SDL_RenderDebugText(g_renderer,
                        ((float)w / scale - SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * (float)strlen(buf)) / 2.0f,
                        ((float)h / scale - SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE) / 2.0f,
                        buf);

    SDL_RenderPresent(g_renderer);
}

static void on_pump(void *user);

// Restart the pump at a new rate (fast while the window is up, slow while it is hidden).
// Safe to call from inside on_pump: plat_timer_del tolerates deleting the firing timer.
static void set_pump(unsigned ms) {
    if (g_pump != PLAT_TIMER_NONE) plat_timer_del(g_pump);
    g_pump = plat_timer_add(ms, 1, on_pump, NULL);
}

// Called from the platform main loop: drain SDL events, then repaint if the window is up
static void on_pump(void *user) {
    SDL_Event ev;

    (void)user;

    while (SDL_PollEvent(&ev)) {
        switch (ev.type) {
            case SDL_EVENT_KEY_DOWN:
                // Printable keycodes are plain ASCII; the core does the case-insensitive matching
                if (ev.key.key > 0 && ev.key.key < 128) {
                    rest_core_send_key(g_core, (char)ev.key.key);
                }
                break;
            case SDL_EVENT_QUIT:
                rest_core_send_key(g_core, 'q');
                break;
            default:
                break;
        }
    }

    // A key may have switched us to work mode (window hidden) just above
    if (g_visible) draw();
}

// --- view interface implementation ---
void view_init(RestCore *core) {
    g_core = core;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        exit(1);
    }

    // Created hidden: the core shows it when a rest starts
    if (!SDL_CreateWindowAndRenderer("Have a break", 800, 600,
                                     SDL_WINDOW_FULLSCREEN | SDL_WINDOW_HIDDEN |
                                     SDL_WINDOW_BORDERLESS | SDL_WINDOW_ALWAYS_ON_TOP,
                                     &g_window, &g_renderer)) {
        fprintf(stderr, "SDL_CreateWindowAndRenderer failed: %s\n", SDL_GetError());
        SDL_Quit();
        exit(1);
    }

    set_pump(PUMP_MS_WORKING);
}

void view_rest_begin(int seconds) {
    g_seconds = seconds;
    g_visible = true;
    SDL_ShowWindow(g_window);
    SDL_RaiseWindow(g_window);
    draw();
    set_pump(PUMP_MS_RESTING);
}

void view_tick(int seconds) {
    g_seconds = seconds;
    if (g_visible) draw();
}

void view_work_begin(void) {
    g_visible = false;
    SDL_HideWindow(g_window);
    set_pump(PUMP_MS_WORKING);
}

void view_destroy(void) {
    if (g_pump != PLAT_TIMER_NONE) { plat_timer_del(g_pump); g_pump = PLAT_TIMER_NONE; }
    if (g_renderer) { SDL_DestroyRenderer(g_renderer); g_renderer = NULL; }
    if (g_window)   { SDL_DestroyWindow(g_window);     g_window   = NULL; }
    SDL_Quit();
}
