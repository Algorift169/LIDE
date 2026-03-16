#include "fm.h"
#include "window_resize.h"
#include <gio/gio.h>
#include <string.h>
#include <gdk/gdkkeysyms.h>
#include <unistd.h>

// Clipboard for cut/copy
static gchar *clipboard_path = NULL;
static gboolean clipboard_is_cut = FALSE;

// Function prototypes for new features
static void fm_show_context_menu(FileManager *fm, GdkEventButton *event, const gchar *selected_path);
static void fm_open_file(const gchar *path);
static void fm_cut_file(FileManager *fm, const gchar *path);
static void fm_copy_file(const gchar *path);
static void fm_paste_file(FileManager *fm, const gchar *dest_dir);
static void fm_move_to_trash(const gchar *path);
static void fm_delete_permanently(const gchar *path);
static void fm_open_in_terminal(const gchar *path);
static void fm_show_properties(const gchar *path);
static void fm_move_to(FileManager *fm, const gchar *source);
static void fm_copy_to(FileManager *fm, const gchar *source);

// Dragging functions
static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    FileManager *fm = (FileManager *)data;

    if (event->button == 1)
    {
        fm->resize_edge = detect_resize_edge_absolute(GTK_WINDOW(fm->window), event->x_root, event->y_root);

        if (fm->resize_edge != RESIZE_NONE) {
            fm->is_resizing = 1;
        } else {
            fm->is_dragging = 1;
        }

        fm->drag_start_x = event->x_root;
        fm->drag_start_y = event->y_root;
        gtk_window_present(GTK_WINDOW(fm->window));
        return TRUE;
    }
    return FALSE;
}

static gboolean on_button_release(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    FileManager *fm = (FileManager *)data;

    if (event->button == 1) {
        fm->is_dragging = 0;
        fm->is_resizing = 0;
        fm->resize_edge = RESIZE_NONE;
        return TRUE;
    }
    return FALSE;
}

static gboolean on_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer data)
{
    FileManager *fm = (FileManager *)data;

    if (!fm->is_dragging && !fm->is_resizing) {
        int resize_edge = detect_resize_edge_absolute(GTK_WINDOW(fm->window), event->x_root, event->y_root);
        update_resize_cursor(widget, resize_edge);
    }

    if (fm->is_resizing) {
        int delta_x = event->x_root - fm->drag_start_x;
        int delta_y = event->y_root - fm->drag_start_y;

        int window_width, window_height;
        gtk_window_get_size(GTK_WINDOW(fm->window), &window_width, &window_height);

        apply_window_resize(GTK_WINDOW(fm->window), fm->resize_edge,
                           delta_x, delta_y, window_width, window_height);

        fm->drag_start_x = event->x_root;
        fm->drag_start_y = event->y_root;
        return TRUE;
    } else if (fm->is_dragging) {
        int dx = event->x_root - fm->drag_start_x;
        int dy = event->y_root - fm->drag_start_y;

        int x, y;
        gtk_window_get_position(GTK_WINDOW(fm->window), &x, &y);
        gtk_window_move(GTK_WINDOW(fm->window), x + dx, y + dy);

        fm->drag_start_x = event->x_root;
        fm->drag_start_y = event->y_root;
        return TRUE;
    }
    return FALSE;
}

// Window control callbacks
static void on_minimize_clicked(GtkButton *button, gpointer window)
{
    (void)button;
    gtk_window_iconify(GTK_WINDOW(window));
}

static void on_maximize_clicked(GtkButton *button, gpointer window)
{
    (void)button;
    GtkWindow *win = GTK_WINDOW(window);
    
    if (gtk_window_is_maximized(win)) {
        gtk_window_unmaximize(win);
    } else {
        gtk_window_maximize(win);
    }
}

static gboolean on_window_state_changed(GtkWidget *window, GdkEventWindowState *event, gpointer data)
{
    GtkButton *max_btn = GTK_BUTTON(data);
    
    if (event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED) {
        gtk_button_set_label(max_btn, "❐");
        gtk_widget_set_tooltip_text(GTK_WIDGET(max_btn), "Restore");
    } else {
        gtk_button_set_label(max_btn, "□");
        gtk_widget_set_tooltip_text(GTK_WIDGET(max_btn), "Maximize");
    }
    
    return FALSE;
}

static void on_close_clicked(GtkButton *button, gpointer window)
{
    (void)button;
    gtk_window_close(GTK_WINDOW(window));
}

// Populate sidebar with places including Recent, Starred, Trash
void fm_populate_sidebar(FileManager *fm)
{
    gtk_list_store_clear(fm->sidebar_store);

    GtkTreeIter iter;

    // Home
    const gchar *home = g_get_home_dir();
    gtk_list_store_append(fm->sidebar_store, &iter);
    gtk_list_store_set(fm->sidebar_store, &iter, 0, "Home", -1);
    g_object_set_data_full(G_OBJECT(fm->sidebar_tree), "path_Home", g_strdup(home), g_free);

    // Recent
    gtk_list_store_append(fm->sidebar_store, &iter);
    gtk_list_store_set(fm->sidebar_store, &iter, 0, "Recent", -1);

    // Starred
    gtk_list_store_append(fm->sidebar_store, &iter);
    gtk_list_store_set(fm->sidebar_store, &iter, 0, "Starred", -1);

    // Trash
    gchar *trash_path = g_build_filename(g_get_home_dir(), ".local/share/Trash/files", NULL);
    gtk_list_store_append(fm->sidebar_store, &iter);
    gtk_list_store_set(fm->sidebar_store, &iter, 0, "Trash", -1);
    g_object_set_data_full(G_OBJECT(fm->sidebar_tree), "path_Trash", trash_path, g_free);
}

// Handle sidebar row activation
void fm_on_sidebar_row_activated(GtkTreeView *tree, GtkTreePath *path, GtkTreeViewColumn *col, FileManager *fm)
{
    GtkTreeIter iter;
    if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(fm->sidebar_store), &iter, path))
        return;

    gchar *text;
    gtk_tree_model_get(GTK_TREE_MODEL(fm->sidebar_store), &iter, 0, &text, -1);

    if (g_strcmp0(text, "Home") == 0) {
        fm_go_home(fm);
    } else if (g_strcmp0(text, "Recent") == 0) {
        gtk_label_set_text(GTK_LABEL(fm->status_label), "Recent files");
    } else if (g_strcmp0(text, "Starred") == 0) {
        gtk_label_set_text(GTK_LABEL(fm->status_label), "Starred items");
    } else if (g_strcmp0(text, "Trash") == 0) {
        gchar *trash_path = g_object_get_data(G_OBJECT(tree), "path_Trash");
        if (trash_path)
            fm_open_location(fm, trash_path);
    }

    g_free(text);
}

// Context menu for main view
static void fm_show_context_menu(FileManager *fm, GdkEventButton *event, const gchar *selected_path)
{
    GtkWidget *menu = gtk_menu_new();
    GtkWidget *item;

    // Open
    item = gtk_menu_item_new_with_label("Open");
    g_signal_connect_swapped(item, "activate", G_CALLBACK(fm_open_file), (gpointer)selected_path);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    // Cut
    item = gtk_menu_item_new_with_label("Cut");
    g_signal_connect_swapped(item, "activate", G_CALLBACK(fm_cut_file), fm);
    g_object_set_data_full(G_OBJECT(item), "path", g_strdup(selected_path), g_free);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    // Copy
    item = gtk_menu_item_new_with_label("Copy");
    g_signal_connect_swapped(item, "activate", G_CALLBACK(fm_copy_file), (gpointer)selected_path);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    // Paste
    if (clipboard_path) {
        item = gtk_menu_item_new_with_label("Paste");
        g_signal_connect_swapped(item, "activate", G_CALLBACK(fm_paste_file), fm);
        gchar *current_dir_path = g_file_get_path(fm->current_dir);
        g_object_set_data_full(G_OBJECT(item), "dest", current_dir_path, g_free);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    }

    // Move to
    item = gtk_menu_item_new_with_label("Move to...");
    g_signal_connect_swapped(item, "activate", G_CALLBACK(fm_move_to), fm);
    g_object_set_data_full(G_OBJECT(item), "source", g_strdup(selected_path), g_free);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    // Copy to
    item = gtk_menu_item_new_with_label("Copy to...");
    g_signal_connect_swapped(item, "activate", G_CALLBACK(fm_copy_to), fm);
    g_object_set_data_full(G_OBJECT(item), "source", g_strdup(selected_path), g_free);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

    // Move to Trash
    item = gtk_menu_item_new_with_label("Move to Trash");
    g_signal_connect_swapped(item, "activate", G_CALLBACK(fm_move_to_trash), (gpointer)selected_path);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    // Delete Permanently
    item = gtk_menu_item_new_with_label("Delete Permanently");
    g_signal_connect_swapped(item, "activate", G_CALLBACK(fm_delete_permanently), (gpointer)selected_path);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

    // Open in Terminal
    item = gtk_menu_item_new_with_label("Open in Terminal");
    g_signal_connect_swapped(item, "activate", G_CALLBACK(fm_open_in_terminal), (gpointer)selected_path);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    // Properties
    item = gtk_menu_item_new_with_label("Properties");
    g_signal_connect_swapped(item, "activate", G_CALLBACK(fm_show_properties), (gpointer)selected_path);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    gtk_widget_show_all(menu);
    gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent*)event);
}

// Context menu trigger on right-click
static gboolean on_main_tree_button_press(GtkWidget *widget, GdkEventButton *event, FileManager *fm)
{
    if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
        GtkTreePath *path;
        if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget), event->x, event->y, &path, NULL, NULL, NULL)) {
            gtk_tree_view_set_cursor(GTK_TREE_VIEW(widget), path, NULL, FALSE);
            GtkTreeIter iter;
            if (gtk_tree_model_get_iter(GTK_TREE_MODEL(fm->main_store), &iter, path)) {
                gchar *name;
                gtk_tree_model_get(GTK_TREE_MODEL(fm->main_store), &iter, 0, &name, -1);
                gchar *current_dir_path = g_file_get_path(fm->current_dir);
                gchar *full_path = g_build_filename(current_dir_path, name, NULL);
                fm_show_context_menu(fm, event, full_path);
                g_free(name);
                g_free(full_path);
                g_free(current_dir_path);
            }
            gtk_tree_path_free(path);
            return TRUE;
        }
    }
    return FALSE;
}

// Action implementations
static void fm_open_file(const gchar *path)
{
    GFile *file = g_file_new_for_path(path);
    g_app_info_launch_default_for_uri(g_file_get_uri(file), NULL, NULL);
    g_object_unref(file);
}

static void fm_cut_file(FileManager *fm, const gchar *path)
{
    if (clipboard_path)
        g_free(clipboard_path);
    clipboard_path = g_strdup(path);
    clipboard_is_cut = TRUE;
    gtk_label_set_text(GTK_LABEL(fm->status_label), "Cut: ready to paste");
}

static void fm_copy_file(const gchar *path)
{
    if (clipboard_path)
        g_free(clipboard_path);
    clipboard_path = g_strdup(path);
    clipboard_is_cut = FALSE;
}

static void fm_paste_file(FileManager *fm, const gchar *dest_dir)
{
    if (!clipboard_path) return;

    GFile *src = g_file_new_for_path(clipboard_path);
    GFile *dest = g_file_new_for_path(dest_dir);
    GFile *dest_file = g_file_get_child(dest, g_path_get_basename(clipboard_path));

    GError *error = NULL;
    if (clipboard_is_cut) {
        if (!g_file_move(src, dest_file, G_FILE_COPY_OVERWRITE, NULL, NULL, NULL, &error)) {
            g_printerr("Move failed: %s\n", error->message);
            g_error_free(error);
        } else {
            gtk_label_set_text(GTK_LABEL(fm->status_label), "Moved successfully");
        }
    } else {
        if (!g_file_copy(src, dest_file, G_FILE_COPY_OVERWRITE, NULL, NULL, NULL, &error)) {
            g_printerr("Copy failed: %s\n", error->message);
            g_error_free(error);
        } else {
            gtk_label_set_text(GTK_LABEL(fm->status_label), "Copied successfully");
        }
    }

    g_object_unref(src);
    g_object_unref(dest);
    g_object_unref(dest_file);

    g_free(clipboard_path);
    clipboard_path = NULL;
    clipboard_is_cut = FALSE;

    fm_refresh(fm);
}

static void fm_move_to_trash(const gchar *path)
{
    GFile *file = g_file_new_for_path(path);
    GError *error = NULL;
    if (!g_file_trash(file, NULL, &error)) {
        g_printerr("Trash failed: %s\n", error->message);
        g_error_free(error);
    }
    g_object_unref(file);
}

static void fm_delete_permanently(const gchar *path)
{
    GFile *file = g_file_new_for_path(path);
    GError *error = NULL;
    if (!g_file_delete(file, NULL, &error)) {
        g_printerr("Delete failed: %s\n", error->message);
        g_error_free(error);
    }
    g_object_unref(file);
}

// Fixed: Now launches the LIDE terminal application
static void fm_open_in_terminal(const gchar *path)
{
    gchar *dir = g_path_get_dirname(path);
    
    // Launch the LIDE terminal application
    pid_t pid = fork();
    if (pid == 0) {
        // Child process - change to the directory and launch terminal
        execl("./blackline-terminal", "blackline-terminal", NULL);
        exit(0);
    }
    
    g_free(dir);
}

static void fm_show_properties(const gchar *path)
{
    GFile *file = g_file_new_for_path(path);
    GFileInfo *info = g_file_query_info(file, "standard::*,time::*", G_FILE_QUERY_INFO_NONE, NULL, NULL);
    if (!info) {
        g_object_unref(file);
        return;
    }

    const gchar *name = g_file_info_get_display_name(info);
    guint64 size = g_file_info_get_size(info);
    const gchar *type = g_file_info_get_content_type(info);
    GDateTime *modified = g_file_info_get_modification_date_time(info);

    gchar *size_str = g_format_size(size);
    gchar *modified_str = modified ? g_date_time_format(modified, "%Y-%m-%d %H:%M:%S") : g_strdup("Unknown");

    GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                                               "Properties of %s\n\nSize: %s\nType: %s\nModified: %s",
                                               name, size_str, type ? type : "unknown", modified_str);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    g_free(size_str);
    g_free(modified_str);
    g_object_unref(info);
    g_object_unref(file);
}

static void fm_move_to(FileManager *fm, const gchar *source)
{
    GtkWidget *chooser = gtk_file_chooser_dialog_new("Select Destination",
                                                      GTK_WINDOW(fm->window),
                                                      GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                                      "_Cancel", GTK_RESPONSE_CANCEL,
                                                      "_Select", GTK_RESPONSE_ACCEPT,
                                                      NULL);
    if (gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT) {
        gchar *dest_dir = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
        GFile *src = g_file_new_for_path(source);
        GFile *dest = g_file_new_for_path(dest_dir);
        GFile *dest_file = g_file_get_child(dest, g_path_get_basename(source));

        GError *error = NULL;
        if (!g_file_move(src, dest_file, G_FILE_COPY_OVERWRITE, NULL, NULL, NULL, &error)) {
            g_printerr("Move failed: %s\n", error->message);
            g_error_free(error);
        } else {
            gtk_label_set_text(GTK_LABEL(fm->status_label), "Moved successfully");
        }

        g_object_unref(src);
        g_object_unref(dest);
        g_object_unref(dest_file);
        g_free(dest_dir);

        fm_refresh(fm);
    }
    gtk_widget_destroy(chooser);
}

static void fm_copy_to(FileManager *fm, const gchar *source)
{
    GtkWidget *chooser = gtk_file_chooser_dialog_new("Select Destination",
                                                      GTK_WINDOW(fm->window),
                                                      GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                                      "_Cancel", GTK_RESPONSE_CANCEL,
                                                      "_Select", GTK_RESPONSE_ACCEPT,
                                                      NULL);
    if (gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT) {
        gchar *dest_dir = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
        GFile *src = g_file_new_for_path(source);
        GFile *dest = g_file_new_for_path(dest_dir);
        GFile *dest_file = g_file_get_child(dest, g_path_get_basename(source));

        GError *error = NULL;
        if (!g_file_copy(src, dest_file, G_FILE_COPY_OVERWRITE, NULL, NULL, NULL, &error)) {
            g_printerr("Copy failed: %s\n", error->message);
            g_error_free(error);
        } else {
            gtk_label_set_text(GTK_LABEL(fm->status_label), "Copied successfully");
        }

        g_object_unref(src);
        g_object_unref(dest);
        g_object_unref(dest_file);
        g_free(dest_dir);

        fm_refresh(fm);
    }
    gtk_widget_destroy(chooser);
}

// Main activation
static void activate(GtkApplication *app, gpointer user_data)
{
    FileManager *fm = g_new(FileManager, 1);
    fm->history = NULL;
    fm->history_pos = NULL;
    fm->current_dir = NULL;
    fm->is_dragging = 0;
    fm->is_resizing = 0;
    fm->resize_edge = RESIZE_NONE;

    // Main window
    fm->window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(fm->window), "Blackline File Manager");
    gtk_window_set_default_size(GTK_WINDOW(fm->window), 900, 600);
    gtk_window_set_position(GTK_WINDOW(fm->window), GTK_WIN_POS_CENTER);
    gtk_container_set_border_width(GTK_CONTAINER(fm->window), 0);
    gtk_window_set_decorated(GTK_WINDOW(fm->window), TRUE);
    gtk_window_set_resizable(GTK_WINDOW(fm->window), TRUE);
    
    gtk_widget_add_events(fm->window, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK);
    g_signal_connect(fm->window, "button-press-event", G_CALLBACK(on_button_press), fm);
    g_signal_connect(fm->window, "button-release-event", G_CALLBACK(on_button_release), fm);
    g_signal_connect(fm->window, "motion-notify-event", G_CALLBACK(on_motion_notify), fm);

    // CSS
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "window { background-color: #1e1e1e; color: #e0e0e0; }\n"
        "treeview { background-color: #252525; color: #e0e0e0; }\n"
        "treeview:selected { background-color: #0d6efd; color: #ffffff; }\n"
        "treeview:selected:focus { background-color: #0b5ed7; }\n"
        "entry { background-color: #2d2d2d; color: #e0e0e0; border: 1px solid #404040; }\n"
        "entry:focus { border-color: #0d6efd; }\n"
        "button { background-color: #2d2d2d; color: #e0e0e0; border: 1px solid #404040; }\n"
        "button:hover { background-color: #3d3d3d; border-color: #0d6efd; }\n"
        "button:active { background-color: #1e1e1e; }\n"
        "statusbar { background-color: #1a1a1a; color: #e0e0e0; border-top: 1px solid #404040; }\n"
        "scrolledwindow { border: 1px solid #404040; }\n",
        -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);

    // Main vertical box
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(fm->window), vbox);

    // Title bar
    GtkWidget *title_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_name(title_bar, "title-bar");
    gtk_widget_set_size_request(title_bar, -1, 30);
    gtk_box_pack_start(GTK_BOX(vbox), title_bar, FALSE, FALSE, 0);

    GtkWidget *title_label = gtk_label_new("Blackline File Manager");
    gtk_label_set_xalign(GTK_LABEL(title_label), 0.0);
    gtk_box_pack_start(GTK_BOX(title_bar), title_label, TRUE, TRUE, 10);

    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_end(GTK_BOX(title_bar), button_box, FALSE, FALSE, 5);

    GtkWidget *min_btn = gtk_button_new_with_label("─");
    gtk_widget_set_size_request(min_btn, 30, 25);
    g_signal_connect(min_btn, "clicked", G_CALLBACK(on_minimize_clicked), fm->window);
    gtk_box_pack_start(GTK_BOX(button_box), min_btn, FALSE, FALSE, 0);

    GtkWidget *max_btn = gtk_button_new_with_label("□");
    gtk_widget_set_size_request(max_btn, 30, 25);
    g_signal_connect(max_btn, "clicked", G_CALLBACK(on_maximize_clicked), fm->window);
    g_signal_connect(fm->window, "window-state-event", G_CALLBACK(on_window_state_changed), max_btn);
    gtk_box_pack_start(GTK_BOX(button_box), max_btn, FALSE, FALSE, 0);

    GtkWidget *close_btn = gtk_button_new_with_label("✕");
    gtk_widget_set_size_request(close_btn, 30, 25);
    g_signal_connect(close_btn, "clicked", G_CALLBACK(on_close_clicked), fm->window);
    gtk_box_pack_start(GTK_BOX(button_box), close_btn, FALSE, FALSE, 0);

    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), sep, FALSE, FALSE, 0);

    // Toolbar
    GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 5);

    fm->back_button = gtk_button_new_with_label("←");
    gtk_box_pack_start(GTK_BOX(toolbar), fm->back_button, FALSE, FALSE, 0);
    g_signal_connect_swapped(fm->back_button, "clicked", G_CALLBACK(fm_go_back), fm);

    fm->forward_button = gtk_button_new_with_label("→");
    gtk_box_pack_start(GTK_BOX(toolbar), fm->forward_button, FALSE, FALSE, 0);
    g_signal_connect_swapped(fm->forward_button, "clicked", G_CALLBACK(fm_go_forward), fm);

    fm->up_button = gtk_button_new_with_label("↑");
    gtk_box_pack_start(GTK_BOX(toolbar), fm->up_button, FALSE, FALSE, 0);
    g_signal_connect_swapped(fm->up_button, "clicked", G_CALLBACK(fm_go_up), fm);

    fm->home_button = gtk_button_new_with_label("🏠");
    gtk_box_pack_start(GTK_BOX(toolbar), fm->home_button, FALSE, FALSE, 0);
    g_signal_connect_swapped(fm->home_button, "clicked", G_CALLBACK(fm_go_home), fm);

    fm->refresh_button = gtk_button_new_with_label("↻");
    gtk_box_pack_start(GTK_BOX(toolbar), fm->refresh_button, FALSE, FALSE, 0);
    g_signal_connect_swapped(fm->refresh_button, "clicked", G_CALLBACK(fm_refresh), fm);

    fm->location_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(toolbar), fm->location_entry, TRUE, TRUE, 0);
    g_signal_connect_swapped(fm->location_entry, "activate", G_CALLBACK(fm_on_location_activate), fm);

    // Horizontal paned
    GtkWidget *hpaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), hpaned, TRUE, TRUE, 5);

    // Sidebar
    GtkWidget *sidebar_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sidebar_scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_paned_pack1(GTK_PANED(hpaned), sidebar_scroll, FALSE, FALSE);

    fm->sidebar_store = gtk_list_store_new(1, G_TYPE_STRING);
    fm->sidebar_tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(fm->sidebar_store));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(fm->sidebar_tree), FALSE);
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(fm->sidebar_tree), -1, "Places", renderer, "text", 0, NULL);
    gtk_container_add(GTK_CONTAINER(sidebar_scroll), fm->sidebar_tree);
    g_signal_connect(fm->sidebar_tree, "row-activated", G_CALLBACK(fm_on_sidebar_row_activated), fm);
    fm_populate_sidebar(fm);

    // Main view
    GtkWidget *main_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(main_scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_paned_pack2(GTK_PANED(hpaned), main_scroll, TRUE, TRUE);

    fm->main_store = gtk_tree_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    fm->main_tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(fm->main_store));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(fm->main_tree), TRUE);

    renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *col = gtk_tree_view_column_new_with_attributes("Name", renderer, "text", 0, NULL);
    gtk_tree_view_column_set_sort_column_id(col, 0);
    gtk_tree_view_append_column(GTK_TREE_VIEW(fm->main_tree), col);

    renderer = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Size", renderer, "text", 1, NULL);
    gtk_tree_view_column_set_sort_column_id(col, 1);
    gtk_tree_view_append_column(GTK_TREE_VIEW(fm->main_tree), col);

    renderer = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Type", renderer, "text", 2, NULL);
    gtk_tree_view_column_set_sort_column_id(col, 2);
    gtk_tree_view_append_column(GTK_TREE_VIEW(fm->main_tree), col);

    renderer = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Modified", renderer, "text", 3, NULL);
    gtk_tree_view_column_set_sort_column_id(col, 3);
    gtk_tree_view_append_column(GTK_TREE_VIEW(fm->main_tree), col);

    gtk_container_add(GTK_CONTAINER(main_scroll), fm->main_tree);
    g_signal_connect(fm->main_tree, "row-activated", G_CALLBACK(fm_on_row_activated), fm);
    g_signal_connect(fm->main_tree, "button-press-event", G_CALLBACK(on_main_tree_button_press), fm);

    // Status bar
    fm->status_label = gtk_label_new("Ready");
    gtk_label_set_xalign(GTK_LABEL(fm->status_label), 0);
    gtk_box_pack_start(GTK_BOX(vbox), fm->status_label, FALSE, FALSE, 2);

    // Start at home
    const gchar *home = g_get_home_dir();
    fm_open_location(fm, home);

    gtk_widget_show_all(fm->window);
    g_object_set_data_full(G_OBJECT(fm->window), "fm", fm, g_free);
}

int main(int argc, char **argv)
{
    GtkApplication *app = gtk_application_new("org.blackline.filemanager", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}