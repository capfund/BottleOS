#include "dwm.h"
#include "window.h"
#include "blit.h"
#include "shell_app.h"
#include "../graphics/graphics.h"

extern graphics_buffer_t screen_buffer;

static Window win;

void dwm_init(void) {
    window_init(&win, 50, 50, 200, 150, shell_app_draw);
}

void dwm_frame(void) {
    graphics_set_target(&win.buffer);
    win.draw();

    graphics_set_target(&screen_buffer);
    graphics_clear_screen(RGB(0,0,0));
    blit_buffer(&win.buffer, &screen_buffer, win.x, win.y);

    graphics_present();
}
