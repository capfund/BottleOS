#ifndef __DWM_H
#define __DWM_H

void dwm_init(void);
void dwm_frame(void);

// Register a desktop application for the modular DWM desktop.
// The `draw` callback will be used to render the app inside its window.
void dwm_register_desktop_app(const char *title, int icon_x, int icon_y, int icon_w, int icon_h, void (*draw)(void));

// Called by the DWM to let compiled-in modules register themselves.
void modules_init(void);

// Focus helpers for apps to check whether the currently-rendered window is focused.
int dwm_is_current_window_focused(void);
// Return the stable id of the window currently being rendered, or -1 if none.
int dwm_get_current_window_id(void);

#endif
