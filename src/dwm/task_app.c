#include "task_app.h"
#include "dwm.h"
#include "../graphics/graphics.h"
#include "../clib/clib.h"

// A small presentable task list / system monitor app.
// Shows registered app names and heap usage live.

void task_app_draw(void) {
    graphics_clear_screen(RGB(20,20,30));

    graphics_draw_string(8, 8, "Task Monitor", RGB(200,200,255), 2);

    // List registered apps
    int count = dwm_get_registered_app_count();
    char buf[64];
    int y = 40;
    for (int i = 0; i < count && y < 220; ++i) {
        const char *t = dwm_get_registered_app_title(i);
        if (!t) break;
        // prefix with index
        int n = 0;
        // format "[i] TITLE"
        buf[0] = '[';
        buf[1] = '0' + (i % 10);
        buf[2] = ']';
        buf[3] = ' ';
        // copy title
        for (n = 0; n < 50 && t[n]; ++n) buf[4+n] = t[n];
        buf[4+n] = '\0';
        graphics_draw_string(12, y, buf, RGB(220,220,220), 1);
        y += 12;
    }

    // Show extended heap statistics
    size_t total=0, used=0, freeb=0, num_blocks=0, num_free_blocks=0, largest_free=0, largest_used=0;
    clib_heap_inspect(&total, &used, &freeb, &num_blocks, &num_free_blocks, &largest_free, &largest_used);

    char line[128];
    int ry = 40;
    int rx = 300; // right column

    // Summary
    int used_k = (int)(used/1024);
    int free_k = (int)(freeb/1024);
    int total_k = (int)(total/1024);
    int lf_k = (int)(largest_free/1024);
    int lu_k = (int)(largest_used/1024);

    // Draw headings
    graphics_draw_string(rx, ry, "Heap Summary:", RGB(200,200,255), 1);
    ry += 14;
    // formatted lines
    // used/total
    int p = 0;
    // "Used: XXXXK / YYYYK"
    const char *lab = "Used: ";
    p = 0; while (lab[p]) { line[p] = lab[p]; p++; }
    // append used_k
    int v = used_k; char tmp[16]; int ti=0; if (v==0) tmp[ti++]='0'; while (v>0){ tmp[ti++]= '0'+(v%10); v/=10;} for (int j=0;j<ti;j++) line[p++]=tmp[ti-1-j];
    line[p++]='K'; line[p++]=' '; line[p++]='/'; line[p++]=' ';
    v = total_k; ti=0; if (v==0) tmp[ti++]='0'; while (v>0){ tmp[ti++]= '0'+(v%10); v/=10;} for (int j=0;j<ti;j++) line[p++]=tmp[ti-1-j]; line[p++]='K'; line[p]=0;
    graphics_draw_string(rx, ry, line, RGB(180,255,180), 1);
    ry += 12;

    // free
    p = 0; const char *lab2 = "Free: "; while (lab2[p]) { line[p] = lab2[p]; p++; } v = free_k; ti=0; if (v==0) tmp[ti++]='0'; while (v>0){ tmp[ti++]= '0'+(v%10); v/=10;} for (int j=0;j<ti;j++) line[p++]=tmp[ti-1-j]; line[p++]='K'; line[p]=0;
    graphics_draw_string(rx, ry, line, RGB(180,255,180), 1);
    ry += 12;

    // blocks
    p = 0; const char *lab3 = "Blocks: "; while (lab3[p]) { line[p] = lab3[p]; p++; } v = (int)num_blocks; ti=0; if (v==0) tmp[ti++]='0'; while (v>0){ tmp[ti++]= '0'+(v%10); v/=10;} for (int j=0;j<ti;j++) line[p++]=tmp[ti-1-j]; line[p]=0;
    graphics_draw_string(rx, ry, line, RGB(200,200,200), 1);
    ry += 12;

    p = 0; const char *lab4 = "Free blocks: "; while (lab4[p]) { line[p] = lab4[p]; p++; } v = (int)num_free_blocks; ti=0; if (v==0) tmp[ti++]='0'; while (v>0){ tmp[ti++]= '0'+(v%10); v/=10;} for (int j=0;j<ti;j++) line[p++]=tmp[ti-1-j]; line[p]=0;
    graphics_draw_string(rx, ry, line, RGB(200,200,200), 1);
    ry += 12;

    // largest free/used
    p = 0; const char *lab5 = "Largest free: "; while (lab5[p]) { line[p] = lab5[p]; p++; } v = lf_k; ti=0; if (v==0) tmp[ti++]='0'; while (v>0){ tmp[ti++]= '0'+(v%10); v/=10;} for (int j=0;j<ti;j++) line[p++]=tmp[ti-1-j]; line[p++]='K'; line[p]=0;
    graphics_draw_string(rx, ry, line, RGB(200,200,200), 1);
    ry += 12;

    p = 0; const char *lab6 = "Largest used: "; while (lab6[p]) { line[p] = lab6[p]; p++; } v = lu_k; ti=0; if (v==0) tmp[ti++]='0'; while (v>0){ tmp[ti++]= '0'+(v%10); v/=10;} for (int j=0;j<ti;j++) line[p++]=tmp[ti-1-j]; line[p++]='K'; line[p]=0;
    graphics_draw_string(rx, ry, line, RGB(200,200,200), 1);
    ry += 12;

    // footer note
    // Show per-owner memory usage below the heap summary
    ry += 8;
    graphics_draw_string(rx, ry, "Per-process memory:", RGB(200,200,255), 1);
    ry += 14;
    int owner_count = clib_owner_count();
    for (int oi = 0; oi < owner_count; ++oi) {
        const char *on = clib_owner_name(oi);
        size_t ob = clib_owner_bytes(oi);
        // format: NAME - NNNK  M%  C%
        char l2[96];
        int lp = 0;
        int k = (int)(ob / 1024);

        // copy name
        for (int j = 0; j < 40 && on && on[j]; ++j) l2[lp++] = on[j];
        l2[lp++] = ' '; l2[lp++] = '-'; l2[lp++] = ' ';

        // append memory in K
        int vv = k; char tmp2[16]; int t2=0; if (vv==0) tmp2[t2++]='0'; while (vv>0){ tmp2[t2++]= '0'+(vv%10); vv/=10;} for (int j=0;j<t2;j++) l2[lp++]=tmp2[t2-1-j];
        l2[lp++]='K';

        // memory percent (use heap total)
        int mem_pct = 0;
        if (total > 0) mem_pct = (int)((ob * 100ULL) / total);

        // append mem percent
        l2[lp++] = ' '; l2[lp++] = '(';
        int mtmp = mem_pct; char mpbuf[8]; int mbi = 0; if (mtmp==0) mpbuf[mbi++]='0'; while (mtmp>0){ mpbuf[mbi++]= '0'+(mtmp%10); mtmp/=10;} for (int j=0;j<mbi;j++) l2[lp++]=mpbuf[mbi-1-j];
        l2[lp++] = '%'; l2[lp++] = ')'; l2[lp++] = ' ';

        // CPU percent
        unsigned long long total_ticks = clib_total_cpu_ticks();
        unsigned long long my_ticks = clib_owner_cpu_ticks(oi);
        int cpu_pct = 0;
        if (total_ticks > 0) cpu_pct = (int)((my_ticks * 100ULL) / total_ticks);

        // append CPU percent
        int ptmp = cpu_pct; char pctbuf[8]; int pbi = 0; if (ptmp==0) pctbuf[pbi++]='0'; while (ptmp>0){ pctbuf[pbi++]= '0'+(ptmp%10); ptmp/=10;} for (int j=0;j<pbi;j++) l2[lp++]=pctbuf[pbi-1-j];
        l2[lp++] = '%'; l2[lp] = 0;

        graphics_draw_string(rx, ry, l2, RGB(220,220,180), 1);

        // Draw simple memory bar
        int bar_x = rx;
        int bar_y = ry + 10;
        int bar_w = 120;
        int bar_h = 6;
        graphics_draw_rectangle(bar_x, bar_y, bar_w, bar_h, RGB(60,60,60)); // background
        int filled = (mem_pct * bar_w) / 100;
        if (filled > 0) graphics_draw_rectangle(bar_x, bar_y, filled, bar_h, RGB(80,200,120));

        // Draw CPU bar under it
        int cbar_y = bar_y + bar_h + 2;
        graphics_draw_rectangle(bar_x, cbar_y, bar_w, bar_h, RGB(40,40,40));
        int cfilled = (cpu_pct * bar_w) / 100;
        if (cfilled > 0) graphics_draw_rectangle(bar_x, cbar_y, cfilled, bar_h, RGB(220,200,80));

        ry = cbar_y + bar_h + 8;
    }

    graphics_draw_string(12, 260-30, "Task monitoring software", RGB(150,150,150), 1);
}
