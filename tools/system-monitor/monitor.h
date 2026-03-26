#ifndef LIDE_MONITOR_H
#define LIDE_MONITOR_H

#include <gtk/gtk.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <fcntl.h>

#define _GNU_SOURCE  /* For DT_DIR */
#define HISTORY_SIZE 60  /* Number of data points for history graphs */

/**
 * CPU statistics structure.
 * Stores CPU times from /proc/stat and calculated usage percentage.
 */
typedef struct {
    guint64 user, nice, system, idle, iowait, irq, softirq, steal;
    guint64 total_prev;  /* Previous total CPU time for delta calculation */
    guint64 idle_prev;   /* Previous idle time for delta calculation */
    double usage;        /* Current CPU usage percentage (0-100) */
} CpuData;

/**
 * Memory statistics structure.
 * Stores memory values from /proc/meminfo and calculated usage percentage.
 */
typedef struct {
    guint64 total;       /* Total physical memory in kB */
    guint64 free;        /* Free memory in kB */
    guint64 available;   /* Available memory in kB (includes cache) */
    guint64 cached;      /* Cached memory in kB */
    double percent;      /* Current memory usage percentage (0-100) */
} MemData;

/**
 * Process entry structure.
 * Represents a single process for the process list.
 */
typedef struct {
    pid_t pid;           /* Process ID */
    char name[256];      /* Process name (from /proc/[pid]/comm) */
    double cpu_percent;  /* CPU usage percentage for this process */
    double mem_percent;  /* Memory usage percentage for this process */
    guint64 vm_rss;      /* Resident Set Size in pages */
} ProcessEntry;

/**
 * Monitor application state structure.
 * Encapsulates all UI components and window state.
 */
typedef struct {
    GtkWidget *window;   /* Main application window */
    GtkWidget *cpu_da;   /* Drawing area for CPU usage graph */
    GtkWidget *mem_da;   /* Drawing area for memory usage bar */

    /* Window manipulation state */
    int is_dragging;     /* TRUE while user is dragging the window */
    int is_resizing;     /* TRUE while user is resizing the window */
    int resize_edge;     /* Which edge/corner is being resized (from window_resize.h) */
    int drag_start_x;    /* Initial mouse X position for drag operation */
    int drag_start_y;    /* Initial mouse Y position for drag operation */
} Monitor;

/* Global data for graphs */
extern double cpu_history[HISTORY_SIZE];  /* Circular buffer of CPU usage history */
extern int cpu_history_index;              /* Current write index for CPU history */
extern double mem_history[HISTORY_SIZE];  /* Circular buffer of memory usage history */
extern int mem_history_index;              /* Current write index for memory history */

/* Function prototypes */

/**
 * Updates CPU usage percentage based on delta between successive reads.
 *
 * @param cpu CpuData structure containing previous and current values.
 *
 * @sideeffect Updates cpu->usage with calculated percentage.
 * @sideeffect Stores current values as previous for next calculation.
 */
void update_cpu_usage(CpuData *cpu);

/**
 * Updates memory usage statistics from /proc/meminfo.
 *
 * @param mem MemData structure to populate with current values.
 *
 * @sideeffect Opens and reads /proc/meminfo.
 * @sideeffect Calculates memory usage percentage.
 */
void update_mem_usage(MemData *mem);

/**
 * Retrieves a list of running processes with CPU and memory usage.
 *
 * @return GList containing ProcessEntry structures.
 *         Caller must free with free_process_list().
 *
 * @sideeffect Reads /proc directory and parses process information.
 */
GList* get_process_list(void);

/**
 * Frees a GList of ProcessEntry structures.
 *
 * @param list GList of ProcessEntry pointers to free.
 */
void free_process_list(GList *list);

/* Drawing helpers */

/**
 * Drawing function for CPU usage graph.
 * Renders a line graph of historical CPU usage data.
 *
 * @param widget The drawing area widget.
 * @param cr     Cairo context for drawing.
 * @param data   User data (unused).
 * @return       FALSE to allow further drawing.
 *
 * @requires cpu_history array must be populated with HISTORY_SIZE values.
 */
gboolean draw_cpu_graph(GtkWidget *widget, cairo_t *cr, gpointer data);

/**
 * Drawing function for memory usage bar graph.
 * Renders a horizontal progress bar showing memory utilization.
 *
 * @param widget The drawing area widget.
 * @param cr     Cairo context for drawing.
 * @param data   User data (unused).
 * @return       FALSE to allow further drawing.
 *
 * @requires mem_history array must be populated with current memory percentage.
 */
gboolean draw_mem_bar(GtkWidget *widget, cairo_t *cr, gpointer data);

#endif /* LIDE_MONITOR_H */