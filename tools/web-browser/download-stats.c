#include "download-stats.h"
#include "downloads.h"
#include <stdio.h>
#include <string.h>

// Dragging variables
static int is_dragging = 0;
static int drag_start_x, drag_start_y;

// Window reference
static GtkWidget *stats_window = NULL;
static BrowserWindow *main_browser = NULL;
static guint refresh_timeout = 0;

// Function prototypes
static void update_stats_list(GtkListBox *listbox);
static gboolean refresh_stats(gpointer user_data);
static void on_close_clicked(GtkButton *button, gpointer window);
static void on_minimize_clicked(GtkButton *button, gpointer window);
static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer window);
static gboolean on_button_release(GtkWidget *widget, GdkEventButton *event, gpointer window);
static gboolean on_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer window);

// Dragging handlers
static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer window)

{
    if (event->button == 1) {
        is_dragging = 1;
        drag_start_x = event->x_root;
        drag_start_y = event->y_root;
        gtk_window_present(GTK_WINDOW(window));
        return TRUE;
    }
    return FALSE;
}

static gboolean on_button_release(GtkWidget *widget, GdkEventButton *event, gpointer window)

{
    (void)widget;
    (void)event;
    (void)window;
    is_dragging = 0;
    return FALSE;
}

static gboolean on_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer window)

{
    GtkWidget *win = GTK_WIDGET(window);

    if (is_dragging) {
        int dx = event->x_root - drag_start_x;
        int dy = event->y_root - drag_start_y;

        int x, y;
        gtk_window_get_position(GTK_WINDOW(win), &x, &y);
        gtk_window_move(GTK_WINDOW(win), x + dx, y + dy);

        drag_start_x = event->x_root;
        drag_start_y = event->y_root;
        return TRUE;
    }
    return FALSE;
}

static void on_minimize_clicked(GtkButton *button, gpointer window)

{
    (void)button;
    gtk_window_iconify(GTK_WINDOW(window));
}

static void on_close_clicked(GtkButton *button, gpointer window)

{
    (void)button;
    if (refresh_timeout) {
        g_source_remove(refresh_timeout);
        refresh_timeout = 0;
    }
    gtk_widget_destroy(GTK_WIDGET(window));
    stats_window = NULL;
}

static void update_stats_list(GtkListBox *listbox)

{
    // Clear existing rows
    GList *children = gtk_container_get_children(GTK_CONTAINER(listbox));
    for (GList *c = children; c; c = c->next) {
        gtk_widget_destroy(GTK_WIDGET(c->data));
    }
    g_list_free(children);

    if (!downloads) {
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *label = gtk_label_new("No downloads");
        gtk_container_add(GTK_CONTAINER(row), label);
        gtk_list_box_insert(listbox, row, -1);
        return;
    }

    for (GList *l = downloads; l; l = l->next) {
        DownloadItem *item = l->data;

        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        gtk_container_add(GTK_CONTAINER(row), vbox);
        gtk_widget_set_margin_top(row, 5);
        gtk_widget_set_margin_bottom(row, 5);
        gtk_widget_set_margin_start(row, 5);
        gtk_widget_set_margin_end(row, 5);

        // Filename
        GtkWidget *filename_label = gtk_label_new(NULL);
        char *filename_markup = g_strdup_printf("<span weight='bold'>%s</span>", item->filename);
        gtk_label_set_markup(GTK_LABEL(filename_label), filename_markup);
        g_free(filename_markup);
        gtk_label_set_xalign(GTK_LABEL(filename_label), 0.0);
        gtk_box_pack_start(GTK_BOX(vbox), filename_label, FALSE, FALSE, 0);

        // URL
        GtkWidget *url_label = gtk_label_new(item->url);
        gtk_label_set_xalign(GTK_LABEL(url_label), 0.0);
        gtk_widget_set_opacity(url_label, 0.7);
        gtk_box_pack_start(GTK_BOX(vbox), url_label, FALSE, FALSE, 0);

        // Progress bar and status
        if (item->status == 1) { // downloading
            GtkWidget *progress_bar = gtk_progress_bar_new();
            gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), item->progress / 100.0);
            char progress_text[256];
            if (item->total > 0) {
                snprintf(progress_text, sizeof(progress_text), "%.1f%% - %llu / %llu bytes", 
                         item->progress, 
                         (unsigned long long)item->received, 
                         (unsigned long long)item->total);
            } else {
                snprintf(progress_text, sizeof(progress_text), "%.1f%% - %llu bytes", 
                         item->progress, 
                         (unsigned long long)item->received);
            }
            gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress_bar), progress_text);
            gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(progress_bar), TRUE);
            gtk_box_pack_start(GTK_BOX(vbox), progress_bar, FALSE, FALSE, 0);
        } else {
            char status_text[256];
            if (item->status == 0) snprintf(status_text, sizeof(status_text), "Pending");
            else if (item->status == 2) snprintf(status_text, sizeof(status_text), "Complete");
            else if (item->status == 3) snprintf(status_text, sizeof(status_text), "Failed: %s", item->error_message ?: "Unknown");
            else snprintf(status_text, sizeof(status_text), "Unknown");

            GtkWidget *status_label = gtk_label_new(status_text);
            gtk_label_set_xalign(GTK_LABEL(status_label), 0.0);
            gtk_box_pack_start(GTK_BOX(vbox), status_label, FALSE, FALSE, 0);
        }

        gtk_list_box_insert(listbox, row, -1);
    }
}

static gboolean refresh_stats(gpointer user_data)

{
    GtkListBox *listbox = GTK_LIST_BOX(user_data);
    update_stats_list(listbox);
    return G_SOURCE_CONTINUE;
}

void show_download_stats_window(BrowserWindow *browser)

{
    if (stats_window) {
        gtk_window_present(GTK_WINDOW(stats_window));
        return;
    }

    main_browser = browser;

    stats_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(stats_window), "Download Manager");
    gtk_window_set_default_size(GTK_WINDOW(stats_window), 600, 400);
    gtk_window_set_position(GTK_WINDOW(stats_window), GTK_WIN_POS_CENTER);
    gtk_window_set_decorated(GTK_WINDOW(stats_window), FALSE); // Remove default title bar
    gtk_window_set_keep_above(GTK_WINDOW(stats_window), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(stats_window), GTK_WINDOW(browser->window));

    // Enable dragging
    gtk_widget_add_events(stats_window, GDK_BUTTON_PRESS_MASK |
                                         GDK_BUTTON_RELEASE_MASK |
                                         GDK_POINTER_MOTION_MASK);
    g_signal_connect(stats_window, "button-press-event", G_CALLBACK(on_button_press), stats_window);
    g_signal_connect(stats_window, "button-release-event", G_CALLBACK(on_button_release), stats_window);
    g_signal_connect(stats_window, "motion-notify-event", G_CALLBACK(on_motion_notify), stats_window);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(stats_window), vbox);

    // Custom title bar
    GtkWidget *title_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_name(title_bar, "title-bar");
    gtk_widget_set_size_request(title_bar, -1, 30);
    gtk_box_pack_start(GTK_BOX(vbox), title_bar, FALSE, FALSE, 0);

    GtkWidget *title_label = gtk_label_new("Download Manager");
    gtk_label_set_xalign(GTK_LABEL(title_label), 0.0);
    gtk_box_pack_start(GTK_BOX(title_bar), title_label, TRUE, TRUE, 10);

    GtkWidget *window_buttons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_end(GTK_BOX(title_bar), window_buttons, FALSE, FALSE, 5);

    // Minimize button
    GtkWidget *min_btn = gtk_button_new_with_label("─");
    gtk_widget_set_size_request(min_btn, 30, 25);
    g_signal_connect(min_btn, "clicked", G_CALLBACK(on_minimize_clicked), stats_window);
    gtk_box_pack_start(GTK_BOX(window_buttons), min_btn, FALSE, FALSE, 0);

    // Close button
    GtkWidget *close_btn = gtk_button_new_with_label("✕");
    gtk_widget_set_size_request(close_btn, 30, 25);
    g_signal_connect(close_btn, "clicked", G_CALLBACK(on_close_clicked), stats_window);
    gtk_box_pack_start(GTK_BOX(window_buttons), close_btn, FALSE, FALSE, 0);

    // Separator
    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), sep, FALSE, FALSE, 0);

    // Scrolled window with listbox for downloads
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);

    GtkWidget *listbox = gtk_list_box_new();
    gtk_container_add(GTK_CONTAINER(scrolled), listbox);

    // CSS
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "window { background-color: #0b0f14; color: #ffffff; }\n"
        "button { background-color: #1e2429; color: #00ff88; border: none; }\n"
        "button:hover { background-color: #2a323a; }\n"
        "#title-bar { background-color: #0b0f14; border-bottom: 2px solid #00ff88; }\n"
        "listboxrow { background-color: #1a1a1a; color: #ffffff; }\n"
        "progressbar { background-color: #00ff88; }\n",
        -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);

    update_stats_list(GTK_LIST_BOX(listbox));

    // Set up a timeout to refresh every 500 ms
    refresh_timeout = g_timeout_add(500, refresh_stats, listbox);

    gtk_widget_show_all(stats_window);
}