#include "monitor.h"

/**
 * Updates memory usage statistics from /proc/meminfo.
 * Reads total, free, available, and cached memory values.
 *
 * @param mem MemData structure to populate with current values.
 *
 * @sideeffect Opens and reads /proc/meminfo.
 * @sideeffect Calculates memory usage percentage based on available memory.
 */
void update_mem_usage(MemData *mem) 

{
    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp) return;

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "MemTotal: %llu kB", &mem->total) == 1) continue;
        if (sscanf(line, "MemFree: %llu kB", &mem->free) == 1) continue;
        if (sscanf(line, "MemAvailable: %llu kB", &mem->available) == 1) continue;
        if (sscanf(line, "Cached: %llu kB", &mem->cached) == 1) continue;
    }
    fclose(fp);

    if (mem->total > 0) {
        mem->percent = 100.0 * (mem->total - mem->available) / mem->total;
    } else {
        mem->percent = 0;
    }
}

/**
 * Drawing function for memory usage bar graph.
 * Renders a horizontal progress bar showing memory utilization.
 *
 * @param widget The drawing area widget.
 * @param cr     Cairo context for drawing.
 * @param data   User data (unused).
 * @return       FALSE to allow further drawing.
 *
 * @sideeffect Draws background bar, filled portion, and percentage text.
 * @requires    mem_history array must be populated with current memory percentage.
 */
gboolean draw_mem_bar(GtkWidget *widget, cairo_t *cr, gpointer data) 

{
    GtkAllocation alloc;
    gtk_widget_get_allocation(widget, &alloc);
    int width = alloc.width;
    int height = alloc.height;

    /* Background - dark gray #1e2429 */
    cairo_set_source_rgb(cr, 0x1e/255.0, 0x24/255.0, 0x29/255.0);
    cairo_paint(cr);

    /* Get latest memory percentage from circular history buffer */
    double percent = mem_history[(mem_history_index - 1 + HISTORY_SIZE) % HISTORY_SIZE];

    /* Draw used portion of the bar */
    int used_width = (int)(width * percent / 100.0);
    cairo_set_source_rgb(cr, 0x00/255.0, 0xff/255.0, 0x88/255.0); /* #00ff88 - accent color */
    cairo_rectangle(cr, 0, 0, used_width, height);
    cairo_fill(cr);

    /* Draw percentage text overlay */
    char text[32];
    snprintf(text, sizeof(text), "%.1f%% used", percent);
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_select_font_face(cr, "Monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 14);
    cairo_move_to(cr, 10, height - 8);
    cairo_show_text(cr, text);

    return FALSE;
}