#include "task_app.h"
#include "dwm.h"
#include "../graphics/graphics.h"
#include "../clib/clib.h"

// A small task/system monitor app: shows apps and heap usage
void task_app_draw(void) {
    graphics_clear_screen(RGB(20,20,30));

    // Header
    graphics_draw_string(8, 8, "Task Monitor", RGB(200,200,255), 2);

    // List registered apps
    int count = dwm_get_registered_app_count();
    char buf[64];
    int y = 40;
    for (int i = 0; i < count && y < 220; ++i) {
        const char *title = dwm_get_registered_app_title(i);
        if (!title) break;

        // Format "[i] TITLE"
        int n = 0;
        buf[n++] = '[';
        buf[n++] = '0' + (i % 10);
        buf[n++] = ']';
        buf[n++] = ' ';

        for (int j = 0; j < 50 && title[j]; ++j)
            buf[n++] = title[j];

        buf[n] = 0;
        graphics_draw_string(12, y, buf, RGB(220,220,220), 1);
        y += 12;
    }

    // Heap statistics
    size_t total, used, freeb, num_blocks, num_free_blocks, largest_free, largest_used;
    clib_heap_inspect(&total, &used, &freeb, &num_blocks, &num_free_blocks, &largest_free, &largest_used);

    int rx = 300;
    int ry = 40;
    char line[128];

    // Helper to print "<label>: <val>K"
    #define DRAW_LINE(label, value, color) do { \
        int len = 0; \
        for (const char *p = label; *p; ++p) line[len++] = *p; \
        char tmp[16]; utoa_dec(value, tmp); \
        for (int i = 0; tmp[i]; ++i) line[len++] = tmp[i]; \
        line[len++] = 'K'; line[len] = 0; \
        graphics_draw_string(rx, ry, line, color, 1); \
        ry += 12; \
    } while(0)

    graphics_draw_string(rx, ry, "Heap Summary:", RGB(200,200,255), 1);
    ry += 14;

    DRAW_LINE("Used: ", (int)(used/1024), RGB(180,255,180));
    DRAW_LINE("Free: ", (int)(freeb/1024), RGB(180,255,180));

    // Blocks
    {
        int len = 0;
        const char *lbls[] = { "Blocks: ", "Free blocks: " };
        size_t vals[] = { num_blocks, num_free_blocks };
        uint32_t colors[] = { RGB(200,200,200), RGB(200,200,200) };
        for (int i = 0; i < 2; ++i) {
            len = 0;
            for (const char *p = lbls[i]; *p; ++p) line[len++] = *p;
            char tmp[16]; utoa_dec((unsigned int)vals[i], tmp);
            for (int j = 0; tmp[j]; ++j) line[len++] = tmp[j];
            line[len] = 0;
            graphics_draw_string(rx, ry, line, colors[i], 1);
            ry += 12;
        }
    }

    DRAW_LINE("Largest free: ", (int)(largest_free/1024), RGB(200,200,200));
    DRAW_LINE("Largest used: ", (int)(largest_used/1024), RGB(200,200,200));

    // Footer note
    ry += 8;
    graphics_draw_string(rx, ry, "Per-process memory:", RGB(200,200,255), 1);
    ry += 14;

    // Per-owner memory and CPU usage
    int owner_count = clib_owner_count();
    unsigned long long total_ticks = clib_total_cpu_ticks();

    for (int oi = 0; oi < owner_count; ++oi) {
        const char *on = clib_owner_name(oi);
        size_t ob = clib_owner_bytes(oi);
        int kbytes = (int)(ob / 1024);
        int mem_pct = total ? (int)((ob * 100ULL) / total) : 0;

        int cpu_pct = total_ticks ? (int)((clib_owner_cpu_ticks(oi) * 100ULL) / total_ticks) : 0;

        // Format line: NAME - NNNK (M%) C%
        int lp = 0;
        for (int j = 0; j < 40 && on && on[j]; ++j) line[lp++] = on[j];
        line[lp++] = ' '; line[lp++] = '-'; line[lp++] = ' ';
        char tmp[16]; utoa_dec(kbytes, tmp); for (int j=0; tmp[j]; ++j) line[lp++] = tmp[j];
        line[lp++]='K'; line[lp++]=' '; line[lp++]='('; utoa_dec(mem_pct, tmp); for(int j=0; tmp[j]; ++j) line[lp++] = tmp[j]; line[lp++]='%'; line[lp++] = ')'; line[lp++]=' '; 
        utoa_dec(cpu_pct, tmp); for(int j=0; tmp[j]; ++j) line[lp++] = tmp[j]; line[lp++]='%'; line[lp++] = ' '; line[lp++] = 'C'; line[lp++] = 'P'; line[lp++] = 'U'; line[lp]=0;

        graphics_draw_string(rx, ry, line, RGB(220,220,180), 1);

        // Draw memory bar
        int bar_w = 120, bar_h = 6;
        graphics_draw_rectangle(rx, ry+10, bar_w, bar_h, RGB(60,60,60));
        int filled = (mem_pct*bar_w)/100;
        if (filled) graphics_draw_rectangle(rx, ry+10, filled, bar_h, RGB(80,200,120));

        // Draw CPU bar
        graphics_draw_rectangle(rx, ry+18, bar_w, bar_h, RGB(40,40,40));
        int cfilled = (cpu_pct*bar_w)/100;
        if (cfilled) graphics_draw_rectangle(rx, ry+18, cfilled, bar_h, RGB(220,200,80));

        ry += 12 + 6 + 2 + 6 + 8; // Advance to next owner
    }

    // Footer
    graphics_draw_string(12, 230, "Task monitoring software", RGB(150,150,150), 1);
}
