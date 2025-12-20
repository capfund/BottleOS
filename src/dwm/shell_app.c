#include "shell_app.h"
#include "../graphics/graphics.h"

static Button btn = {
    .x = 20,
    .y = 20,
    .width = 120,
    .height = 40,
    .label = "HELLO",
    .bg_color = RGB(40,120,200),
    .text_color = RGB(255,255,255),
    .text_scale = 1,
    .border_rad = 6
};

void shell_app_draw(void) {
    graphics_clear_screen(RGB(30,30,30));
    draw_button(&btn);
    graphics_draw_string(20, 80, "shell app", RGB(200,200,200), 1);
}
