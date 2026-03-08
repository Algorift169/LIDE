#ifndef VOIDFOX_SETTINGS_H
#define VOIDFOX_SETTINGS_H

#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include "voidfox.h"

// Search engine enum
typedef enum {
    SEARCH_GOOGLE,
    SEARCH_DUCKDUCKGO,
    SEARCH_BING,
    SEARCH_YAHOO,
    SEARCH_CUSTOM
} SearchEngine;

// Settings structure
typedef struct {
    char *home_page;
    SearchEngine search_engine;
    char *custom_search_url;
    char *download_dir;
    gboolean enable_javascript;
    gboolean enable_cookies;
    gboolean enable_images;
    gboolean enable_popups;
    gboolean enable_plugins;
    gboolean hardware_acceleration;
    char *user_agent;
    gboolean dark_mode;
    gboolean remember_history;
    gboolean remember_passwords;
    gboolean do_not_track;
} Settings;

// Global settings variable (declared in settings.c)
extern Settings settings;

// Function prototypes
void load_settings(void);
void save_settings(void);
void show_settings_dialog(BrowserWindow *browser);
void apply_settings_to_web_view(WebKitWebView *web_view);
const char* get_search_url(const char *query);

#endif