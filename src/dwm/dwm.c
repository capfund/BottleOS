#include "dwm.h"
#include "window.h"
#include "blit.h"
#include "../graphics/graphics.h"
#include "../mouse/ps2.h"
#include "event.h"
#include "../keyboard/keyboard.h"
#include <stddef.h>
#include <stdint.h>
#include "../clib/clib.h"

extern graphics_buffer_t screen_buffer;

#define TBH 20 // titlebar height
#define MAX_WINDOWS 8

static Window windows[MAX_WINDOWS];
static int win_count = 0;

// Desktop app registry (modular apps show as icons on the desktop)
#define MAX_APPS 16
typedef struct {
    const char *title;
    int x, y, w, h;
    void (*draw)(void);
    int used;
    int win_w;
    int win_h;
} DesktopApp;

static DesktopApp apps[MAX_APPS];

// Drag state
static int drag_win = -1;
static int drag_off_x = 0;
static int drag_off_y = 0;

static int mouse_x = 300;
static int mouse_y = 200;
static int prev_left = 0;

// Focus state: index of focused window, and index being currently rendered
static int focused_window = -1;
static int current_window = -1;

// Default icon sizing/spacing when modules don't provide sizes/positions
#define ICON_DEFAULT_W 64
#define ICON_DEFAULT_H 48
#define ICON_SPACING_X 16
#define ICON_SPACING_Y 24
#define ICON_START_X 16
#define ICON_START_Y 24

int dwm_is_current_window_focused(void) {
    return (current_window != -1 && current_window == focused_window) ? 1 : 0;
}

int dwm_get_current_window_id(void) {
    if (current_window == -1) return -1;
    if (current_window < 0 || current_window >= win_count) return -1;
    return (int)windows[current_window].id;
}

static void bring_to_top(int idx) {
    if (idx < 0 || idx >= win_count) return;
    Window tmp = windows[idx];
    for (int i = idx; i < win_count - 1; ++i) windows[i] = windows[i+1];
    windows[win_count-1] = tmp;
}

static void create_window_at(int x, int y, int w, int h, const char *title, void (*draw)(void)) {
    if (win_count >= MAX_WINDOWS) return;
    window_init(&windows[win_count], x, y, w, h, title, draw);
    win_count++;
    // Newly created window becomes focused
    focused_window = win_count - 1;
}

void dwm_register_desktop_app(const char *title, int icon_x, int icon_y, int icon_w, int icon_h, void (*draw)(void)) {
    for (int i = 0; i < MAX_APPS; ++i) {
        if (!apps[i].used) {
            apps[i].used = 1;
            apps[i].title = title;
            // If caller didn't specify sizes/positions, auto-place
            int w = icon_w <= 0 ? ICON_DEFAULT_W : icon_w;
            int h = icon_h <= 0 ? ICON_DEFAULT_H : icon_h;
            apps[i].w = w;
            apps[i].h = h;
            apps[i].win_w = 320;
            apps[i].win_h = 200;
            if (icon_x < 0 || icon_y < 0 || icon_w <= 0 || icon_h <= 0) {
                // compute grid placement
                int sw = (int)screen_buffer.width;
                int cols = (sw - ICON_START_X) / (w + ICON_SPACING_X);
                if (cols < 1) cols = 1;
                int idx = i; // place by slot index
                int col = idx % cols;
                int row = idx / cols;
                apps[i].x = ICON_START_X + col * (w + ICON_SPACING_X);
                apps[i].y = ICON_START_Y + row * (h + ICON_SPACING_Y);
            } else {
                apps[i].x = icon_x;
                apps[i].y = icon_y;
            }
            apps[i].draw = draw;
            return;
        }
    }
}

// Allow modules to set a preferred initial window size for their app
void dwm_set_app_window_size(const char *title, int win_w, int win_h) {
    for (int i = 0; i < MAX_APPS; ++i) {
        if (!apps[i].used) continue;
        if (!apps[i].title) continue;
        // simple string compare
        int j = 0;
        const char *a = apps[i].title;
        const char *b = title;
        while (a[j] && b[j] && a[j] == b[j]) j++;
        if (a[j] == '\0' && b[j] == '\0') {
            apps[i].win_w = win_w > 0 ? win_w : apps[i].win_w;
            apps[i].win_h = win_h > 0 ? win_h : apps[i].win_h;
            return;
        }
    }
}

int dwm_get_registered_app_count(void) {
    int c = 0;
    for (int i = 0; i < MAX_APPS; ++i) if (apps[i].used) c++;
    return c;
}

const char *dwm_get_registered_app_title(int idx) {
    int c = 0;
    for (int i = 0; i < MAX_APPS; ++i) {
        if (!apps[i].used) continue;
        if (c == idx) return apps[i].title;
        c++;
    }
    return NULL;
}

void dwm_init(void) {
    // initialize mouse (ignore failures for now)
    mouse_init();
    event_init();
    win_count = 0;
    // let compiled-in modules register their apps
    modules_init();
}

void dwm_frame(void) {
    MousePacket pkt;
    InputEvent ev;
    #include <stdint.h>

    // Poll drivers to generate events (non-blocking)
    // poll mouse hardware (ps2 mouse) to push any pending packets
    while (mouse_poll(&pkt) == 1) {
        (void)0; // ps2.c already pushes events when packets are assembled
    }

    // poll keyboard non-blocking to push scancodes
    unsigned char sc;
    while ((sc = keyboard_poll_scancode()) != 0) {
        (void)sc; // keyboard_poll_scancode pushes events
    }

    // Drain central input event queue but only consume mouse events.
    // Leave key events in the queue so focused apps can consume them during their draw().
    while (event_pop(&ev)) {
        if (ev.type != EVENT_MOUSE_MOVE) {
            // put non-mouse event back and stop draining
            event_push(&ev);
            break;
        }

        pkt.dx = ev.u.mouse.dx;
        pkt.dy = ev.u.mouse.dy;
        pkt.left = ev.u.mouse.left;
        pkt.right = ev.u.mouse.right;
        pkt.middle = ev.u.mouse.middle;

        mouse_x += pkt.dx;
        mouse_y -= pkt.dy;

        // Clamp
        if (mouse_x < 0) mouse_x = 0;
        if (mouse_x > ((int)screen_buffer.width - 1)) mouse_x = ((int)screen_buffer.width - 1);
        if (mouse_y < 0) mouse_y = 0;
        if (mouse_y > ((int)screen_buffer.height - 1)) mouse_y = ((int)screen_buffer.height - 1);

        // Left button pressed
        if (pkt.left && !prev_left) {
            int handled = 0;
            // Check topmost window titlebar hit (iterate top-down)
            for (int i = win_count - 1; i >= 0; --i) {
                Window *w = &windows[i];
                int tb_top = w->y - TBH;
                if (mouse_x >= w->x && mouse_x < (w->x + w->width) &&
                    mouse_y >= tb_top && mouse_y < w->y) {

                    // Close button region (square inside titlebar, right side)
                    int close_w = TBH - 8;
                    int close_h = TBH - 8;
                    int close_x = w->x + w->width - 4 - close_w;
                    int close_y = tb_top + 4;

                    if (mouse_x >= close_x && mouse_x < (close_x + close_w) &&
                        mouse_y >= close_y && mouse_y < (close_y + close_h)) {
                        // close this window
                        window_destroy(w);
                        // remove from list
                        for (int j = i; j < win_count - 1; ++j) windows[j] = windows[j+1];
                        // adjust focused_window if necessary
                        if (focused_window == i) focused_window = -1;
                        else if (focused_window > i) focused_window--;
                        win_count--;
                        handled = 1;
                        break;
                    }

                    // start dragging this window and bring to top
                    bring_to_top(i);
                    drag_win = win_count - 1;
                    // give focus to this window
                    focused_window = drag_win;
                    drag_off_x = mouse_x - windows[drag_win].x;
                    drag_off_y = mouse_y - windows[drag_win].y;
                    handled = 1;
                    break;
                }
            }

            // If no window titlebar hit, check registered app icons
            if (!handled) {
                for (int a = 0; a < MAX_APPS; ++a) {
                    if (!apps[a].used) continue;
                    if (mouse_x >= apps[a].x && mouse_x < (apps[a].x + apps[a].w) &&
                        mouse_y >= apps[a].y && mouse_y < (apps[a].y + apps[a].h)) {
                        // Launch app window overlapping the icon — use app's preferred window size
                        create_window_at(apps[a].x, apps[a].y, apps[a].win_w, apps[a].win_h, apps[a].title, apps[a].draw);
                        handled = 1;
                        break;
                    }
                }
            }
        }

        // While left is held, drag active window
        if (pkt.left && drag_win != -1) {
            windows[drag_win].x = mouse_x - drag_off_x;
            windows[drag_win].y = mouse_y - drag_off_y;
        }

        // On release, stop dragging
        if (!pkt.left && prev_left) {
            drag_win = -1;
        }

        prev_left = pkt.left;
    }

    // First render window contents into their buffers
    for (int i = 0; i < win_count; ++i) {
        graphics_set_target(&windows[i].buffer);
        // Mark which window is currently being rendered so apps can check focus
        current_window = i;
        // measure CPU time spent in the app draw callback
        unsigned int lo1, hi1, lo2, hi2;
        unsigned long long t0, t1;
        __asm__ volatile ("rdtsc" : "=a" (lo1), "=d" (hi1));
        windows[i].draw();
        __asm__ volatile ("rdtsc" : "=a" (lo2), "=d" (hi2));
        t0 = ((unsigned long long)hi1 << 32) | lo1;
        t1 = ((unsigned long long)hi2 << 32) | lo2;
        if (t1 > t0) {
            unsigned long long delta = t1 - t0;
            clib_account_cpu(windows[i].owner, delta);
        }
        current_window = -1;
    }

    // Now draw desktop (background + icons) onto screen
    graphics_set_target(&screen_buffer);
    graphics_clear_screen(RGB(22,172,199));

    // Draw registered app icons on the desktop
    for (int a = 0; a < MAX_APPS; ++a) {
        if (!apps[a].used) continue;
        graphics_draw_rectangle(apps[a].x, apps[a].y, apps[a].w, apps[a].h, RGB(60,60,140));
        // center title text under icon
        int len = 0;
        const char *s = apps[a].title;
        while (s && s[len]) ++len;
        int text_w = len * 8; // approx 8px per char at scale 1
        int text_x = apps[a].x + (apps[a].w - text_w) / 2;
        graphics_draw_string(text_x, apps[a].y + apps[a].h + 4, apps[a].title, RGB(255,255,255), 1);
    }

    // Blit windows in order (0 = bottom, last = top)
    for (int i = 0; i < win_count; ++i) {
        Window *w = &windows[i];

        // Titlebar
        graphics_draw_rectangle(
            w->x,
            (w->y - TBH),
            w->width,
            TBH,
            RGB(80,80,80)
        );

        // Close button
        int close_w = TBH - 8;
        int close_h = TBH - 8;
        int close_x = w->x + w->width - 4 - close_w;
        int close_y = (w->y - TBH) + 4;
        graphics_draw_rectangle(close_x, close_y, close_w, close_h, RGB(180,50,50));
        graphics_draw_string(close_x + 3, close_y, "X", RGB(255,255,255), 1);

        graphics_draw_string(
            w->x + 4,
            (w->y - TBH + 4),
            w->title,
            RGB(255,255,255),
            1
        );

        blit_buffer(&w->buffer, &screen_buffer, w->x, w->y);
    }

    // Draw cursor last
    graphics_draw_cursor(mouse_x, mouse_y, RGB(255,255,255));

    graphics_present();
    // decay per-owner CPU counters slightly each frame to weight recent activity
    clib_cpu_frame_decay();
}