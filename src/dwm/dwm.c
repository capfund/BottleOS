#include "dwm.h"
#include "window.h"
#include "blit.h"
#include "../graphics/graphics.h"
#include "../mouse/ps2.h"

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
} DesktopApp;

static DesktopApp apps[MAX_APPS];

// Drag state
static int drag_win = -1;
static int drag_off_x = 0;
static int drag_off_y = 0;

static int mouse_x = 300;
static int mouse_y = 200;
static int prev_left = 0;

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
}

void dwm_register_desktop_app(const char *title, int icon_x, int icon_y, int icon_w, int icon_h, void (*draw)(void)) {
    for (int i = 0; i < MAX_APPS; ++i) {
        if (!apps[i].used) {
            apps[i].used = 1;
            apps[i].title = title;
            apps[i].x = icon_x;
            apps[i].y = icon_y;
            apps[i].w = icon_w;
            apps[i].h = icon_h;
            apps[i].draw = draw;
            return;
        }
    }
}

void dwm_init(void) {
    // initialize mouse (ignore failures for now)
    mouse_init();
    win_count = 0;
    // let compiled-in modules register their apps
    modules_init();
}

void dwm_frame(void) {
    MousePacket pkt;

    // Poll all available mouse packets and update state
    while (mouse_poll(&pkt) == 1) {
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
                        win_count--;
                        handled = 1;
                        break;
                    }

                    // start dragging this window and bring to top
                    bring_to_top(i);
                    drag_win = win_count - 1;
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
                        // Launch app window overlapping the icon
                        create_window_at(apps[a].x, apps[a].y, 320, 200, apps[a].title, apps[a].draw);
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
        windows[i].draw();
    }

    // Now draw desktop (background + icons) onto screen
    graphics_set_target(&screen_buffer);
    graphics_clear_screen(RGB(22,172,199));

    // Draw registered app icons on the desktop
    for (int a = 0; a < MAX_APPS; ++a) {
        if (!apps[a].used) continue;
        graphics_draw_rectangle(apps[a].x, apps[a].y, apps[a].w, apps[a].h, RGB(60,60,140));
        graphics_draw_string(apps[a].x + 6, apps[a].y + apps[a].h + 4, apps[a].title, RGB(255,255,255), 1);
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
}
