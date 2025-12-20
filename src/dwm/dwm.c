#include "dwm.h"
#include "window.h"
#include "blit.h"
#include "shell_app.h"
#include "../graphics/graphics.h"

extern graphics_buffer_t screen_buffer;

static Window win;
#define TBH 20 // titlebar height

void dwm_init(void) {
    window_init(&win, 50, 50, 200, 150, "Shelly", shell_app_draw);
}

void dwm_frame(void) {
    graphics_set_target(&win.buffer);
    win.draw();

    graphics_set_target(&screen_buffer);
    graphics_clear_screen(RGB(0,0,0));
    graphics_draw_rectangle(
        win.x,
        (win.y - TBH),
        win.width,
        TBH,
        RGB(80,80,80)
    );
    graphics_draw_string(
        win.x + 4,
        (win.y - TBH + 4),
        win.title,
        RGB(255,255,255),
        1
    );

    blit_buffer(&win.buffer, &screen_buffer, win.x, win.y);

    graphics_present();
}
