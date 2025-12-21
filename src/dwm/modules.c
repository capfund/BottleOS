#include "dwm.h"
#include "shell_app.h"
#include "../power/power.h"
#include "../graphics/graphics.h"

// forward-declare the shutdown draw function we add below
void shutdown_app_draw(void);

void modules_init(void) {
    // Register the shell app at a sensible desktop position
    // Let DWM auto-place the app icon by passing zero/negative for size/pos
    dwm_register_desktop_app("Shell", -1, -1, 0, 0, shell_app_draw);

    // Register a Shutdown app which triggers power_off when its window is created/drawn
    dwm_register_desktop_app("Shutdown", -1, -1, 0, 0, shutdown_app_draw);
    // Register Task Monitor app
    extern void task_app_draw(void);
    dwm_register_desktop_app("Tasks", -1, -1, 0, 0, task_app_draw);
    // Request a larger window for the task monitor
    dwm_set_app_window_size("Tasks", 560, 260);
}

// A tiny draw callback that triggers power_off once when rendered.
// It will display a short message in the window buffer and call power_off().
void shutdown_app_draw(void) {
    // clear window buffer
    graphics_clear_screen(RGB(30,30,30));
    graphics_draw_string(8, 20, "Shutting down...", RGB(220,220,220), 1);
    // attempt power off
    power_off();
}
