#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>

static void on_tool_clicked(GtkButton *button, gpointer data) {
    const char *cmd = (const char *)data;
    printf("Launching: %s\n", cmd);
    system(cmd);
}

static void on_close_clicked(GtkButton *button, gpointer window) {
    gtk_window_close(GTK_WINDOW(window));
}

static void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "BlackLine Tools");
    gtk_window_set_default_size(GTK_WINDOW(window), 300, 400);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    
    // Main vertical box
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    
    // Title bar with close button
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<b>Tools</b>");
    gtk_box_pack_start(GTK_BOX(hbox), title, TRUE, TRUE, 0);
    
    GtkWidget *close_btn = gtk_button_new_with_label("X");
    gtk_widget_set_size_request(close_btn, 30, 25);
    g_signal_connect(close_btn, "clicked", G_CALLBACK(on_close_clicked), window);
    gtk_box_pack_end(GTK_BOX(hbox), close_btn, FALSE, FALSE, 0);
    
    // Separator
    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), sep, FALSE, FALSE, 5);
    
    // Tools list
    const char *tools[] = {
        "Terminal", "xterm",
        "File Manager", "nautilus",
        "Text Editor", "gedit",
        "Calculator", "gnome-calculator",
        "System Monitor", "gnome-system-monitor",
        NULL
    };
    
    for (int i = 0; tools[i] != NULL; i += 2) {
        GtkWidget *button = gtk_button_new_with_label(tools[i]);
        g_signal_connect(button, "clicked", G_CALLBACK(on_tool_clicked), (gpointer)tools[i+1]);
        gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 2);
    }
    
    gtk_widget_show_all(window);
}

int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new("org.blackline.tools", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}