#include <gtk/gtk.h>
#include <vte/vte.h>
#include <gdk/gdkkeysyms.h>
#include <stdlib.h>
#include <string.h>

// Function prototypes
static void new_terminal_tab(const char *initial_directory);
static void spawn_callback(VteTerminal *terminal, GPid pid, GError *error, gpointer user_data);
static void close_tab_callback(GtkButton *button, gpointer notebook);
static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer window);
static gboolean on_button_release(GtkWidget *widget, GdkEventButton *event, gpointer window);
static gboolean on_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer window);
static void on_minimize_clicked(GtkButton *button, gpointer window);
static void on_maximize_clicked(GtkButton *button, gpointer window);
static void on_close_clicked(GtkButton *button, gpointer window);
static gboolean on_window_state_changed(GtkWidget *window, GdkEventWindowState *event, gpointer data);

// Global variables
static GtkWidget *main_window = NULL;
static GtkWidget *notebook = NULL;
static int is_dragging = 0;
static int drag_start_x, drag_start_y;

// Dragging handlers
static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer window)

{
    if (event->button == 1)
    {
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

// Track window state changes to update maximize button
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

// CSS
static void apply_css(void) {
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "window { background-color: #0b0f14; color: #ffffff; }\n"
        "notebook { background-color: #0b0f14; }\n"
        "notebook tab { background-color: #1e2429; color: #ffffff; }\n"
        "notebook tab:checked { background-color: #00ff88; color: #0b0f14; }\n"
        "notebook tab button { padding: 0; min-width: 20px; min-height: 20px; }\n"
        "#title-bar { background-color: #0b0f14; border-bottom: 2px solid #00ff88; }\n"
        "button { background-color: #1e2429; color: #00ff88; border: none; }\n"
        "button:hover { background-color: #2a323a; }\n",
        -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

// Spawn callback
static void spawn_callback(VteTerminal *terminal, GPid pid, GError *error, gpointer user_data) 

{
    if (error) {
        g_warning("Failed to spawn shell: %s", error->message);
        g_error_free(error);
    }
}

// Close tab callback
static void close_tab_callback(GtkButton *button, gpointer notebook) 

{
    GtkWidget *tab = g_object_get_data(G_OBJECT(button), "tab");
    if (tab) {
        gtk_notebook_remove_page(GTK_NOTEBOOK(notebook), 
            gtk_notebook_page_num(GTK_NOTEBOOK(notebook), tab));
    }
}

// Create a new terminal tab
static void new_terminal_tab(const char *initial_directory) 

{
    GtkWidget *vte = vte_terminal_new();

    // Set basic options
    vte_terminal_set_scrollback_lines(VTE_TERMINAL(vte), 10000);

    // Spawn the shell
    const char *shell = vte_get_user_shell();
    if (!shell || *shell == '\0') shell = "/bin/bash";

    char *argv[] = { (char*)shell, NULL };

    vte_terminal_spawn_async(VTE_TERMINAL(vte),
        VTE_PTY_DEFAULT,
        initial_directory,
        argv,
        NULL,
        0,
        NULL, NULL, NULL,
        -1,
        NULL,
        spawn_callback,
        NULL);

    // Create tab with close button
    GtkWidget *tab_label_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *tab_label = gtk_label_new("Terminal");
    GtkWidget *close_btn = gtk_button_new_from_icon_name("window-close", GTK_ICON_SIZE_MENU);
    gtk_button_set_relief(GTK_BUTTON(close_btn), GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(close_btn, "Close tab");

    gtk_box_pack_start(GTK_BOX(tab_label_box), tab_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(tab_label_box), close_btn, FALSE, FALSE, 0);
    gtk_widget_show_all(tab_label_box);

    // Add to notebook
    int page_num = gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vte, tab_label_box);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), page_num);
    gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(notebook), vte, TRUE);

    // Connect close button
    g_object_set_data(G_OBJECT(close_btn), "tab", vte);
    g_signal_connect(close_btn, "clicked", G_CALLBACK(close_tab_callback), notebook);
}

// Window activate callback
static void activate(GtkApplication *app, gpointer user_data) 

{
    GtkWidget *vbox;
    GtkWidget *title_bar;
    GtkWidget *title_label;
    GtkWidget *window_buttons;
    GtkWidget *min_btn;
    GtkWidget *max_btn;
    GtkWidget *close_btn;

    // Create main window
    main_window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(main_window), "Terminal");
    gtk_window_set_default_size(GTK_WINDOW(main_window), 800, 600);
    gtk_window_set_position(GTK_WINDOW(main_window), GTK_WIN_POS_CENTER);
    gtk_window_set_decorated(GTK_WINDOW(main_window), FALSE);

    // Enable dragging
    gtk_widget_add_events(main_window, GDK_BUTTON_PRESS_MASK |
                                       GDK_BUTTON_RELEASE_MASK |
                                       GDK_POINTER_MOTION_MASK);
    g_signal_connect(main_window, "button-press-event", G_CALLBACK(on_button_press), main_window);
    g_signal_connect(main_window, "button-release-event", G_CALLBACK(on_button_release), main_window);
    g_signal_connect(main_window, "motion-notify-event", G_CALLBACK(on_motion_notify), main_window);

    apply_css();

    // Main vertical box
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(main_window), vbox);

    // Custom title bar
    title_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_name(title_bar, "title-bar");
    gtk_widget_set_size_request(title_bar, -1, 30);
    gtk_box_pack_start(GTK_BOX(vbox), title_bar, FALSE, FALSE, 0);

    title_label = gtk_label_new("Terminal");
    gtk_label_set_xalign(GTK_LABEL(title_label), 0.0);
    gtk_box_pack_start(GTK_BOX(title_bar), title_label, TRUE, TRUE, 10);

    window_buttons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_end(GTK_BOX(title_bar), window_buttons, FALSE, FALSE, 5);

    // Minimize button
    min_btn = gtk_button_new_with_label("─");
    gtk_widget_set_size_request(min_btn, 30, 25);
    g_signal_connect(min_btn, "clicked", G_CALLBACK(on_minimize_clicked), main_window);
    gtk_box_pack_start(GTK_BOX(window_buttons), min_btn, FALSE, FALSE, 0);

    // Maximize button
    max_btn = gtk_button_new_with_label("□");
    gtk_widget_set_size_request(max_btn, 30, 25);
    g_signal_connect(max_btn, "clicked", G_CALLBACK(on_maximize_clicked), main_window);
    g_signal_connect(main_window, "window-state-event", G_CALLBACK(on_window_state_changed), max_btn);
    gtk_box_pack_start(GTK_BOX(window_buttons), max_btn, FALSE, FALSE, 0);

    // Close button
    close_btn = gtk_button_new_with_label("✕");
    gtk_widget_set_size_request(close_btn, 30, 25);
    g_signal_connect(close_btn, "clicked", G_CALLBACK(on_close_clicked), main_window);
    gtk_box_pack_start(GTK_BOX(window_buttons), close_btn, FALSE, FALSE, 0);

    // Notebook for tabs
    notebook = gtk_notebook_new();
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), TRUE);
    gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook), FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

    // Create first tab
    new_terminal_tab(NULL);

    gtk_widget_show_all(main_window);
}

int main(int argc, char **argv) 

{
    GtkApplication *app = gtk_application_new("org.blackline.terminal", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}