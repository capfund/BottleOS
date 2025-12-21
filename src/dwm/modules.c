#include "dwm.h"
#include "shell_app.h"

void modules_init(void) {
    // Register the shell app at a sensible desktop position
    // Let DWM auto-place the app icon by passing zero/negative for size/pos
    dwm_register_desktop_app("Shell", -1, -1, 0, 0, shell_app_draw);
}
