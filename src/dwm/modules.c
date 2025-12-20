#include "dwm.h"
#include "shell_app.h"

void modules_init(void) {
    // Register the shell app at a sensible desktop position
    dwm_register_desktop_app("SHELL", 24, 24, 64, 48, shell_app_draw);
}
