#include "settings.h"
#include "history.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <glib.h>
#include <webkit2/webkit2.h>

#define SETTINGS_FILE "voidfox_settings.txt"
#define CONFIG_DIR ".config/lide/voidfox"

Settings settings;

// Default values
static const char *default_home_page = "about:blank";
static const char *default_custom_search = "https://www.google.com/search?q=%s";
static const char *default_download_dir = "Downloads";
static const char *default_user_agent = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36";

// Ensure config directory exists
static void ensure_config_dir(void)
{
    char *path = g_build_filename(g_get_home_dir(), CONFIG_DIR, NULL);
    g_mkdir_with_parents(path, 0700);
    g_free(path);
}

// Get full path to settings file
static char* get_settings_path(void)
{
    return g_build_filename(g_get_home_dir(), CONFIG_DIR, SETTINGS_FILE, NULL);
}

// Initialize default settings
static void init_default_settings(void)
{
    settings.home_page = g_strdup(default_home_page);
    settings.search_engine = SEARCH_GOOGLE;
    settings.custom_search_url = g_strdup(default_custom_search);
    settings.download_dir = g_strdup(default_download_dir);
    settings.enable_javascript = TRUE;
    settings.enable_cookies = TRUE;
    settings.enable_images = TRUE;
    settings.enable_popups = FALSE;
    settings.enable_plugins = TRUE;
    settings.hardware_acceleration = FALSE; // Off by default for stability
    settings.user_agent = g_strdup(default_user_agent);
    settings.dark_mode = TRUE;
    settings.remember_history = TRUE;
    settings.remember_passwords = TRUE;
    settings.do_not_track = TRUE;
}

// Load settings from file
void load_settings(void)
{
    ensure_config_dir();
    char *path = get_settings_path();
    FILE *f = fopen(path, "r");
    if (!f) {
        printf("Settings: no settings file found, using defaults\n");
        init_default_settings();
        save_settings(); // save defaults
        g_free(path);
        return;
    }

    // Start with defaults, then override with file
    init_default_settings();

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        char *newline = strchr(line, '\n');
        if (newline) *newline = '\0';
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        char *key = line;
        char *value = eq + 1;

        if (strcmp(key, "home_page") == 0) {
            g_free(settings.home_page);
            settings.home_page = g_strdup(value);
        }
        else if (strcmp(key, "search_engine") == 0) {
            int se = atoi(value);
            if (se >= SEARCH_GOOGLE && se <= SEARCH_CUSTOM)
                settings.search_engine = se;
        }
        else if (strcmp(key, "custom_search_url") == 0) {
            g_free(settings.custom_search_url);
            settings.custom_search_url = g_strdup(value);
        }
        else if (strcmp(key, "download_dir") == 0) {
            g_free(settings.download_dir);
            settings.download_dir = g_strdup(value);
        }
        else if (strcmp(key, "enable_javascript") == 0) {
            settings.enable_javascript = atoi(value) != 0;
        }
        else if (strcmp(key, "enable_cookies") == 0) {
            settings.enable_cookies = atoi(value) != 0;
        }
        else if (strcmp(key, "enable_images") == 0) {
            settings.enable_images = atoi(value) != 0;
        }
        else if (strcmp(key, "enable_popups") == 0) {
            settings.enable_popups = atoi(value) != 0;
        }
        else if (strcmp(key, "enable_plugins") == 0) {
            settings.enable_plugins = atoi(value) != 0;
        }
        else if (strcmp(key, "hardware_acceleration") == 0) {
            settings.hardware_acceleration = atoi(value) != 0;
        }
        else if (strcmp(key, "user_agent") == 0) {
            g_free(settings.user_agent);
            settings.user_agent = g_strdup(value);
        }
        else if (strcmp(key, "dark_mode") == 0) {
            settings.dark_mode = atoi(value) != 0;
        }
        else if (strcmp(key, "remember_history") == 0) {
            settings.remember_history = atoi(value) != 0;
        }
        else if (strcmp(key, "remember_passwords") == 0) {
            settings.remember_passwords = atoi(value) != 0;
        }
        else if (strcmp(key, "do_not_track") == 0) {
            settings.do_not_track = atoi(value) != 0;
        }
    }
    fclose(f);
    g_free(path);
    printf("Settings: loaded\n");
}

// Save settings to file
void save_settings(void)
{
    ensure_config_dir();
    char *path = get_settings_path();
    FILE *f = fopen(path, "w");
    if (!f) {
        printf("Settings: failed to save\n");
        g_free(path);
        return;
    }

    fprintf(f, "home_page=%s\n", settings.home_page);
    fprintf(f, "search_engine=%d\n", settings.search_engine);
    fprintf(f, "custom_search_url=%s\n", settings.custom_search_url);
    fprintf(f, "download_dir=%s\n", settings.download_dir);
    fprintf(f, "enable_javascript=%d\n", settings.enable_javascript);
    fprintf(f, "enable_cookies=%d\n", settings.enable_cookies);
    fprintf(f, "enable_images=%d\n", settings.enable_images);
    fprintf(f, "enable_popups=%d\n", settings.enable_popups);
    fprintf(f, "enable_plugins=%d\n", settings.enable_plugins);
    fprintf(f, "hardware_acceleration=%d\n", settings.hardware_acceleration);
    fprintf(f, "user_agent=%s\n", settings.user_agent);
    fprintf(f, "dark_mode=%d\n", settings.dark_mode);
    fprintf(f, "remember_history=%d\n", settings.remember_history);
    fprintf(f, "remember_passwords=%d\n", settings.remember_passwords);
    fprintf(f, "do_not_track=%d\n", settings.do_not_track);

    fclose(f);
    g_free(path);
    printf("Settings: saved\n");
}

// Apply settings to a WebKitWebView (to be called when creating a new tab)
void apply_settings_to_web_view(WebKitWebView *web_view)
{
    WebKitSettings *wk_settings = webkit_web_view_get_settings(web_view);
    if (!wk_settings) return;

    webkit_settings_set_enable_javascript(wk_settings, settings.enable_javascript);
    
    // enable_plugins is deprecated - use enable_media instead
    webkit_settings_set_enable_media(wk_settings, settings.enable_plugins);
    
    webkit_settings_set_enable_webgl(wk_settings, settings.hardware_acceleration);
    webkit_settings_set_enable_webrtc(wk_settings, settings.hardware_acceleration);
    webkit_settings_set_auto_load_images(wk_settings, settings.enable_images);
    webkit_settings_set_enable_page_cache(wk_settings, TRUE);
    webkit_settings_set_enable_smooth_scrolling(wk_settings, TRUE);

    // Cookies
    WebKitCookieManager *cookie_manager = webkit_web_context_get_cookie_manager(webkit_web_view_get_context(web_view));
    if (cookie_manager) {
        if (settings.enable_cookies) {
            char *cookies_path = g_build_filename(g_get_home_dir(), CONFIG_DIR, "cookies.txt", NULL);
            webkit_cookie_manager_set_persistent_storage(cookie_manager,
                cookies_path,
                WEBKIT_COOKIE_PERSISTENT_STORAGE_TEXT);
            g_free(cookies_path);
        } else {
            webkit_cookie_manager_set_persistent_storage(cookie_manager, NULL, WEBKIT_COOKIE_PERSISTENT_STORAGE_TEXT);
        }
    }

    // User agent
    if (settings.user_agent && *settings.user_agent) {
        webkit_settings_set_user_agent(wk_settings, settings.user_agent);
    }

    // Do Not Track - not supported in older WebKitGTK versions
    // We'll skip it for compatibility
    // If you have a newer version, you can add:
    // webkit_settings_set_enable_do_not_track(wk_settings, settings.do_not_track);
}

// Construct search URL based on settings and query
const char* get_search_url(const char *query)
{
    static GString *url = NULL;
    if (url) g_string_free(url, TRUE);
    url = g_string_new("");

    char *encoded = g_uri_escape_string(query, NULL, FALSE);
    switch (settings.search_engine) {
        case SEARCH_GOOGLE:
            g_string_printf(url, "https://www.google.com/search?q=%s", encoded);
            break;
        case SEARCH_DUCKDUCKGO:
            g_string_printf(url, "https://duckduckgo.com/?q=%s", encoded);
            break;
        case SEARCH_BING:
            g_string_printf(url, "https://www.bing.com/search?q=%s", encoded);
            break;
        case SEARCH_YAHOO:
            g_string_printf(url, "https://search.yahoo.com/search?p=%s", encoded);
            break;
        case SEARCH_CUSTOM:
            if (settings.custom_search_url && *settings.custom_search_url) {
                g_string_printf(url, settings.custom_search_url, encoded);
            } else {
                g_string_printf(url, "https://www.google.com/search?q=%s", encoded);
            }
            break;
        default:
            g_string_printf(url, "https://www.google.com/search?q=%s", encoded);
            break;
    }
    g_free(encoded);
    return url->str;
}

// Settings dialog
typedef struct {
    GtkWidget *dialog;
    GtkWidget *home_entry;
    GtkWidget *search_combo;
    GtkWidget *custom_search_entry;
    GtkWidget *download_dir_entry;
    GtkWidget *download_dir_button;
    GtkWidget *js_check;
    GtkWidget *cookies_check;
    GtkWidget *images_check;
    GtkWidget *popups_check;
    GtkWidget *plugins_check;
    GtkWidget *hw_accel_check;
    GtkWidget *user_agent_entry;
    GtkWidget *dark_mode_check;
    GtkWidget *history_check;
    GtkWidget *passwords_check;
    GtkWidget *dnt_check;
    BrowserWindow *browser;
} SettingsDialogData;

static void on_search_engine_changed(GtkComboBox *combo, SettingsDialogData *data)
{
    int active = gtk_combo_box_get_active(combo);
    gtk_widget_set_sensitive(data->custom_search_entry, (active == SEARCH_CUSTOM));
}

static void on_download_dir_clicked(GtkButton *button, SettingsDialogData *data)
{
    (void)button;
    GtkWidget *chooser = gtk_file_chooser_dialog_new("Select Download Directory",
                                                      GTK_WINDOW(data->dialog),
                                                      GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                                      "_Cancel", GTK_RESPONSE_CANCEL,
                                                      "_Select", GTK_RESPONSE_ACCEPT,
                                                      NULL);
    if (gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT) {
        char *folder = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
        gtk_entry_set_text(GTK_ENTRY(data->download_dir_entry), folder);
        g_free(folder);
    }
    gtk_widget_destroy(chooser);
}

static void on_settings_response(GtkDialog *dialog, gint response_id, SettingsDialogData *data)
{
    if (response_id == GTK_RESPONSE_ACCEPT) {
        // Save settings from dialog
        g_free(settings.home_page);
        settings.home_page = g_strdup(gtk_entry_get_text(GTK_ENTRY(data->home_entry)));

        settings.search_engine = gtk_combo_box_get_active(GTK_COMBO_BOX(data->search_combo));

        g_free(settings.custom_search_url);
        settings.custom_search_url = g_strdup(gtk_entry_get_text(GTK_ENTRY(data->custom_search_entry)));

        g_free(settings.download_dir);
        settings.download_dir = g_strdup(gtk_entry_get_text(GTK_ENTRY(data->download_dir_entry)));

        settings.enable_javascript = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->js_check));
        settings.enable_cookies = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->cookies_check));
        settings.enable_images = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->images_check));
        settings.enable_popups = !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->popups_check)); // Invert because checkbox says "Block"
        settings.enable_plugins = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->plugins_check));
        settings.hardware_acceleration = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->hw_accel_check));

        g_free(settings.user_agent);
        settings.user_agent = g_strdup(gtk_entry_get_text(GTK_ENTRY(data->user_agent_entry)));

        settings.dark_mode = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->dark_mode_check));
        settings.remember_history = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->history_check));
        settings.remember_passwords = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->passwords_check));
        settings.do_not_track = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->dnt_check));

        save_settings();

        // Optionally, apply to existing tabs? For now just save.
    }

    gtk_widget_destroy(GTK_WIDGET(dialog));
    g_free(data);
}
void show_settings_dialog(BrowserWindow *browser)
{
    SettingsDialogData *data = g_new0(SettingsDialogData, 1);
    data->browser = browser;

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Settings",
                                                    GTK_WINDOW(browser->window),
                                                    GTK_DIALOG_MODAL,
                                                    "_Cancel", GTK_RESPONSE_CANCEL,
                                                    "_Save", GTK_RESPONSE_ACCEPT,
                                                    NULL);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 600, 500);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *notebook = gtk_notebook_new();
    gtk_container_add(GTK_CONTAINER(content), notebook);

    // --- General tab ---
    GtkWidget *general = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(general), 10);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), general, gtk_label_new("General"));

    // Home page
    GtkWidget *home_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(general), home_hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(home_hbox), gtk_label_new("Home page:"), FALSE, FALSE, 0);
    data->home_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(data->home_entry), settings.home_page);
    gtk_box_pack_start(GTK_BOX(home_hbox), data->home_entry, TRUE, TRUE, 0);

    // Search engine
    GtkWidget *search_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(general), search_hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(search_hbox), gtk_label_new("Search engine:"), FALSE, FALSE, 0);
    data->search_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(data->search_combo), "Google");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(data->search_combo), "DuckDuckGo");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(data->search_combo), "Bing");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(data->search_combo), "Yahoo");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(data->search_combo), "Custom");
    gtk_combo_box_set_active(GTK_COMBO_BOX(data->search_combo), settings.search_engine);
    gtk_box_pack_start(GTK_BOX(search_hbox), data->search_combo, FALSE, FALSE, 0);

    // Custom search URL
    GtkWidget *custom_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(general), custom_hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(custom_hbox), gtk_label_new("Custom URL (use %s for query):"), FALSE, FALSE, 0);
    data->custom_search_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(data->custom_search_entry), settings.custom_search_url);
    gtk_box_pack_start(GTK_BOX(custom_hbox), data->custom_search_entry, TRUE, TRUE, 0);

    // Download directory
    GtkWidget *download_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(general), download_hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(download_hbox), gtk_label_new("Download folder:"), FALSE, FALSE, 0);
    data->download_dir_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(data->download_dir_entry), settings.download_dir);
    gtk_box_pack_start(GTK_BOX(download_hbox), data->download_dir_entry, TRUE, TRUE, 0);
    data->download_dir_button = gtk_button_new_with_label("Browse...");
    g_signal_connect(data->download_dir_button, "clicked", G_CALLBACK(on_download_dir_clicked), data);
    gtk_box_pack_start(GTK_BOX(download_hbox), data->download_dir_button, FALSE, FALSE, 0);

    // User agent
    GtkWidget *ua_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(general), ua_hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(ua_hbox), gtk_label_new("User Agent:"), FALSE, FALSE, 0);
    data->user_agent_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(data->user_agent_entry), settings.user_agent);
    gtk_box_pack_start(GTK_BOX(ua_hbox), data->user_agent_entry, TRUE, TRUE, 0);

    // --- Privacy tab ---
    GtkWidget *privacy = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(privacy), 10);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), privacy, gtk_label_new("Privacy"));

    data->js_check = gtk_check_button_new_with_label("Enable JavaScript");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->js_check), settings.enable_javascript);
    gtk_box_pack_start(GTK_BOX(privacy), data->js_check, FALSE, FALSE, 0);

    data->cookies_check = gtk_check_button_new_with_label("Enable Cookies");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->cookies_check), settings.enable_cookies);
    gtk_box_pack_start(GTK_BOX(privacy), data->cookies_check, FALSE, FALSE, 0);

    data->images_check = gtk_check_button_new_with_label("Load Images");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->images_check), settings.enable_images);
    gtk_box_pack_start(GTK_BOX(privacy), data->images_check, FALSE, FALSE, 0);

    data->popups_check = gtk_check_button_new_with_label("Block Pop-up Windows");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->popups_check), !settings.enable_popups);
    gtk_box_pack_start(GTK_BOX(privacy), data->popups_check, FALSE, FALSE, 0);

    data->plugins_check = gtk_check_button_new_with_label("Enable Plugins/Media");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->plugins_check), settings.enable_plugins);
    gtk_box_pack_start(GTK_BOX(privacy), data->plugins_check, FALSE, FALSE, 0);

    data->hw_accel_check = gtk_check_button_new_with_label("Hardware Acceleration (may cause crashes)");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->hw_accel_check), settings.hardware_acceleration);
    gtk_box_pack_start(GTK_BOX(privacy), data->hw_accel_check, FALSE, FALSE, 0);

    data->dnt_check = gtk_check_button_new_with_label("Send 'Do Not Track' request");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->dnt_check), settings.do_not_track);
    gtk_box_pack_start(GTK_BOX(privacy), data->dnt_check, FALSE, FALSE, 0);

    // --- History & Data tab ---
    GtkWidget *history_data = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(history_data), 10);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), history_data, gtk_label_new("History & Data"));

    data->history_check = gtk_check_button_new_with_label("Remember browsing history");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->history_check), settings.remember_history);
    gtk_box_pack_start(GTK_BOX(history_data), data->history_check, FALSE, FALSE, 0);

    data->passwords_check = gtk_check_button_new_with_label("Remember passwords");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->passwords_check), settings.remember_passwords);
    gtk_box_pack_start(GTK_BOX(history_data), data->passwords_check, FALSE, FALSE, 0);

    GtkWidget *clear_history_btn = gtk_button_new_with_label("Clear History");
    g_signal_connect(clear_history_btn, "clicked", G_CALLBACK(clear_history), browser);
    gtk_box_pack_start(GTK_BOX(history_data), clear_history_btn, FALSE, FALSE, 0);

    // --- Appearance tab ---
    GtkWidget *appearance = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(appearance), 10);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), appearance, gtk_label_new("Appearance"));

    data->dark_mode_check = gtk_check_button_new_with_label("Dark mode");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->dark_mode_check), settings.dark_mode);
    gtk_box_pack_start(GTK_BOX(appearance), data->dark_mode_check, FALSE, FALSE, 0);

    // Connect signals for dynamic sensitivity
    g_signal_connect(data->search_combo, "changed", G_CALLBACK(on_search_engine_changed), data);
    on_search_engine_changed(GTK_COMBO_BOX(data->search_combo), data); // initial sensitivity

    gtk_widget_show_all(dialog);

    // Run the dialog modally
    gint response = gtk_dialog_run(GTK_DIALOG(dialog));

    if (response == GTK_RESPONSE_ACCEPT) {
        // Save settings from dialog
        g_free(settings.home_page);
        settings.home_page = g_strdup(gtk_entry_get_text(GTK_ENTRY(data->home_entry)));

        settings.search_engine = gtk_combo_box_get_active(GTK_COMBO_BOX(data->search_combo));

        g_free(settings.custom_search_url);
        settings.custom_search_url = g_strdup(gtk_entry_get_text(GTK_ENTRY(data->custom_search_entry)));

        g_free(settings.download_dir);
        settings.download_dir = g_strdup(gtk_entry_get_text(GTK_ENTRY(data->download_dir_entry)));

        settings.enable_javascript = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->js_check));
        settings.enable_cookies = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->cookies_check));
        settings.enable_images = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->images_check));
        settings.enable_popups = !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->popups_check)); // Invert
        settings.enable_plugins = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->plugins_check));
        settings.hardware_acceleration = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->hw_accel_check));

        g_free(settings.user_agent);
        settings.user_agent = g_strdup(gtk_entry_get_text(GTK_ENTRY(data->user_agent_entry)));

        settings.dark_mode = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->dark_mode_check));
        settings.remember_history = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->history_check));
        settings.remember_passwords = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->passwords_check));
        settings.do_not_track = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->dnt_check));

        save_settings();
    }

    gtk_widget_destroy(dialog);
    g_free(data);
}