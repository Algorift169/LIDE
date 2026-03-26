#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

/**
 * Callback function triggered when a list box row is activated.
 * Executes the associated command using system().
 *
 * @param button The button that triggered the signal (unused, matches GTK signature)
 * @param data   The command string to execute, stored as user data on the row
 *
 * @sideeffect Executes an external process via system(). Blocks UI until process completes.
 * @security   Executes arbitrary command from user data without sanitization.
 */
static void launch_app(GtkButton *button, gpointer data) 
{
    system((char*)data);
}

/**
 * Application activation callback.
 * Constructs the main window UI with a search entry and scrollable list of applications.
 *
 * @param app        The GtkApplication instance
 * @param user_data  User data passed during signal connection (unused)
 *
 * @sideeffect Creates and displays all UI widgets. Registers signal handlers.
 */
static void activate(GtkApplication *app, gpointer user_data) 
{
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "BlackLine Launcher");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 300);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), box);

    GtkWidget *entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(box), entry, FALSE, FALSE, 0);

    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_box_pack_start(GTK_BOX(box), scrolled, TRUE, TRUE, 0);

    GtkWidget *listbox = gtk_list_box_new();
    gtk_container_add(GTK_CONTAINER(scrolled), listbox);
    
    //  apps
    const char *apps[] = {
        "Terminal", "xterm",
        "File Manager", "nautilus",
        "Text Editor", "gedit",
        "Calculator", "gnome-calculator",
        NULL
    };

    /* Populate list box with application entries.
     * Store the executable command string as row user data for launch_app callback. */
    for (int i = 0; apps[i]; i += 2) 
    
    {
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *label = gtk_label_new(apps[i]);
        gtk_container_add(GTK_CONTAINER(row), label);
        /* Attach command string to row without a destroy notification since the string
         * is static data from the apps array. No memory management required. */
        g_object_set_data_full(G_OBJECT(row), "cmd", (gpointer)apps[i+1], NULL);
        gtk_list_box_insert(GTK_LIST_BOX(listbox), row, -1);
    }

    /* Connect row activation signal. The NULL user_data parameter means the callback
     * will retrieve the command from the row's user data. */
    g_signal_connect(listbox, "row-activated", G_CALLBACK(launch_app), NULL);

    gtk_widget_show_all(window);
}

/**
 * Application entry point.
 * Creates and runs a GtkApplication instance.
 *
 * @param argc  Argument count from command line
 * @param argv  Argument vector from command line
 * @return      Exit status from g_application_run()
 */
int main(int argc, char **argv) 
{
    GtkApplication *app = gtk_application_new("org.blackline.launcher", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}