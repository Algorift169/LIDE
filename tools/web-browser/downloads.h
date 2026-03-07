#ifndef VOIDFOX_DOWNLOADS_H
#define VOIDFOX_DOWNLOADS_H

#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include "voidfox.h"

// Download item structure
typedef struct {
    char *filename;
    char *url;
    char *destination;
    double progress;
    guint64 received;
    guint64 total;
    int status; // 0: pending, 1: downloading, 2: complete, 3: failed
    char *error_message;
    WebKitDownload *download;
} DownloadItem;

// Global download list (shared with download-stats.c)
extern GList *downloads;
extern BrowserWindow *global_browser;

// Function prototypes
void show_downloads_tab(BrowserWindow *browser);
void add_download(WebKitDownload *download, BrowserWindow *browser);
void update_downloads_tab(BrowserWindow *browser);
void save_downloads(void);
void load_downloads(void);

#endif