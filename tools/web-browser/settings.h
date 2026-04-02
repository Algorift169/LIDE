/* settings.h */
#ifndef SETTINGS_H
#define SETTINGS_H

#include <glib.h>
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include "voidfox.h"

/** 
 * settings.h
 * 
 * Settings UI interface definitions and preferences
 *
 * This module is part of the LIDE desktop environment system.
 * See the main window manager (wm/) and session management (session/)
 * for system architecture overview.
 */
/**
 * Search engine enumeration.
 * Defines available default search providers.
 */
typedef enum {
    SEARCH_GOOGLE,       /* Google search engine */
    SEARCH_DUCKDUCKGO,   /* DuckDuckGo privacy-focused search */
    SEARCH_BING,         /* Microsoft Bing search */
    SEARCH_YAHOO,        /* Yahoo search */
    SEARCH_CUSTOM        /* User-defined custom search URL */
} SearchEngine;

/**
 * Startup behavior enumeration.
 * Defines what to show when the browser starts.
 */
typedef enum {
    STARTUP_NEW_TAB_PAGE,   /* Open a blank new tab */
    STARTUP_CONTINUE,       /* Restore previous session */
    STARTUP_HOME_PAGE,      /* Open the configured home page */
    STARTUP_SPECIFIC_PAGES  /* Open user-specified pages */
} StartupBehavior;

/**
 * Tab behavior enumeration.
 * Defines how tabs are managed.
 */
typedef enum {
    TAB_BEHAVIOR_LAST,      /* Reopen last closed tab before closing browser */
    TAB_BEHAVIOR_MAINTAIN,  /* Maintain multiple tabs */
    TAB_BEHAVIOR_SINGLE     /* Always single tab mode */
} TabBehavior;

/**
 * HTTPS only mode enumeration.
 * Defines how to handle HTTP connections.
 */
typedef enum {
    HTTPS_ONLY_OFF,              /* Allow HTTP connections */
    HTTPS_ONLY_ALL_SITES,       /* Upgrade all connections to HTTPS */
    HTTPS_ONLY_STANDARD_PORTS   /* Upgrade only standard HTTP ports */
} HTTPSOnlyMode;

/**
 * Cache size enumeration.
 * Defines maximum cache storage limits.
 */
typedef enum {
    CACHE_UNLIMITED,    /* No size limit */
    CACHE_100MB,        /* 100 MB limit */
    CACHE_500MB,        /* 500 MB limit */
    CACHE_1GB,          /* 1 GB limit */
    CACHE_DISABLE       /* Cache disabled */
} CacheSize;

/**
 * Main settings structure.
 * Contains all configurable browser preferences.
 */
typedef struct {
    /* General */
    char *home_page;                /* Home page URL */
    StartupBehavior startup_behavior; /* What to show on startup */
    GList *startup_pages;           /* List of URLs for specific pages startup */

    /* Search */
    SearchEngine search_engine;     /* Default search engine */
    char *custom_search_url;        /* Custom search URL with %s placeholder */

    /* Appearance */
    gboolean dark_mode;             /* Enable dark color scheme */
    int font_size;                  /* Default font size in points */
    char *font_family;              /* Default font family name */
    int zoom_level;                 /* Page zoom percentage (100 = 100%) */
    gboolean show_bookmarks_bar;    /* Display bookmarks bar */
    gboolean show_home_button;      /* Show home button in toolbar */
    gboolean show_status_bar;       /* Show status bar at bottom */
    int theme_color;                /* 0=default, 1=light, 2=sepia, 3=dark */

    /* Privacy & Security */
    gboolean enable_javascript;     /* Enable JavaScript execution */
    gboolean enable_cookies;        /* Enable cookie storage */
    gboolean enable_images;         /* Load images on pages */
    gboolean enable_popups;         /* Allow popup windows */
    gboolean enable_plugins;        /* Enable media plugins */
    gboolean do_not_track;          /* Send DNT header */
    gboolean safe_browsing_enabled; /* Block dangerous sites */
    gboolean send_usage_stats;      /* Send anonymous usage data */
    gboolean predictive_search;     /* Preload search results */
    gboolean remember_passwords;    /* Save passwords for sites */
    gboolean autofill_enabled;      /* Auto-fill form data */
    gboolean block_third_party_cookies; /* Block cross-site cookies */
    HTTPSOnlyMode https_only_mode;  /* HTTPS upgrade policy */
    gboolean block_mixed_content;   /* Block HTTP content on HTTPS pages */
    gboolean isolate_site_data;     /* Prevent cross-site tracking */
    gboolean block_tracking_scripts; /* Block known trackers */
    gboolean block_cryptomining;    /* Block cryptocurrency miners */
    gboolean block_fingerprinting;  /* Block browser fingerprinting */
    char *blocked_sites;            /* Comma-separated blocked site list */

    /* Permissions */
    gboolean location_enabled;      /* Allow geolocation access */
    gboolean camera_enabled;        /* Allow camera access */
    gboolean microphone_enabled;    /* Allow microphone access */
    gboolean notifications_enabled; /* Allow push notifications */
    gboolean clipboard_enabled;     /* Allow clipboard access */
    gboolean midi_enabled;          /* Allow MIDI device access */
    gboolean usb_enabled;           /* Allow USB device access */
    gboolean gyroscope_enabled;     /* Allow gyroscope access */

    /* Downloads */
    char *download_dir;             /* Default download directory */
    gboolean ask_download_location; /* Prompt for download location */
    gboolean show_downloads_when_done; /* Show downloads tab on completion */

    /* Tabs & Windows */
    TabBehavior tab_behavior;       /* Tab management behavior */
    gboolean restore_session_on_startup; /* Restore previous tabs */
    gboolean enable_tab_groups;     /* Allow tab grouping */

    /* Content Settings */
    gboolean enable_sound;          /* Enable audio playback */
    gboolean enable_autoplay_video; /* Allow video autoplay */
    gboolean enable_autoplay_audio; /* Allow audio autoplay */
    gboolean enable_pdf_viewer;     /* Use built-in PDF viewer */
    gboolean block_ads;             /* Enable ad blocking */
    gboolean enable_reading_mode;   /* Show reading mode button */

    /* Accessibility */
    gboolean text_scaled;           /* Apply global text scaling */
    int text_scale_percentage;      /* Text scale percentage (100=normal) */
    gboolean high_contrast_mode;    /* Enhanced contrast for readability */
    gboolean minimize_ui;           /* Compact interface */
    gboolean show_page_colors;      /* Display page-specified colors */

    /* Language & Translation */
    char *language;                 /* Browser language (e.g., "en", "en-US") */
    gboolean enable_translation;    /* Offer page translation */
    char *translation_language;     /* Target language for translation */

    /* System */
    gboolean hardware_acceleration; /* Enable GPU acceleration */
    char *user_agent;               /* Custom user agent string */
    gboolean use_system_title_bar;  /* Use OS window decorations */
    gboolean open_links_in_background; /* Open new tabs in background */
    gboolean enable_system_proxy;   /* Use system proxy settings */

    /* Cache & Storage */
    CacheSize cache_size;           /* Cache size limit */
    gboolean cache_enabled;         /* Enable disk cache */
    int cookie_expiration_days;     /* Cookie lifetime (0=session only) */
    gboolean offline_content_enabled; /* Store pages for offline use */

    /* History & Data */
    gboolean remember_history;      /* Record visited pages */
    gboolean remember_form_entries; /* Save form input history */
    int history_days_to_keep;       /* History retention in days (0=forever) */
    gboolean clear_cache_on_exit;   /* Delete cache on browser close */
    gboolean clear_cookies_on_exit; /* Delete cookies on browser close */
    gboolean clear_history_on_exit; /* Clear history on browser close */

    /* Advanced */
    char *startup_command;          /* Custom command to run on startup */
    gboolean enable_developer_tools; /* Show developer tools in context menu */
    gboolean enable_debugging;      /* Enable remote debugging protocol */
} Settings;

/* Global settings instance */
extern Settings settings;

/* Function prototypes */

/**
 * Loads settings from the configuration file.
 * If the file doesn't exist, initializes with defaults and saves them.
 *
 * @sideeffect Populates the global settings structure.
 */
void load_settings(void);

/**
 * Saves current settings to the configuration file.
 *
 * @sideeffect Writes settings to disk.
 */
void save_settings(void);

/**
 * Applies settings to a WebKitWebView.
 * Updates WebKitSettings based on current configuration.
 *
 * @param web_view The WebKitWebView to configure.
 */
void apply_settings_to_web_view(WebKitWebView *web_view);

/**
 * Constructs a search URL based on current search engine settings.
 *
 * @param query The search query string.
 * @return Pointer to static string containing the full search URL.
 *         Valid until next call to this function.
 */
const char* get_search_url(const char *query);

/**
 * Displays the settings dialog as a modal window.
 *
 * @param browser BrowserWindow instance for applying changes.
 */
void show_settings_dialog(BrowserWindow *browser);

/**
 * Opens settings as a tab inside the browser window.
 *
 * @param browser BrowserWindow instance.
 */
void show_settings_tab(BrowserWindow *browser);

#endif /* SETTINGS_H */