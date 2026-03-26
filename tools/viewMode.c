#include "viewMode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static ViewMode current_mode = VIEW_MODE_LIST;
static char *config_path = NULL;
static int is_loaded = 0;

/**
 * Get the configuration directory path.
 *
 * Constructs the path "$HOME/.config/blackline" and creates the directory
 * if it does not exist. Falls back to "/tmp" if HOME environment variable
 * is not set.
 *
 * @return Static pointer to the config directory path.
 */
static const char* get_config_dir(void)

{
    static char *config_dir = NULL;
    
    if (config_dir == NULL) {
        const char *home = getenv("HOME");
        if (home) {
            config_dir = g_build_filename(home, ".config", "blackline", NULL);
            
            // Create directory if it doesn't exist
            if (access(config_dir, F_OK) != 0) {
                g_mkdir_with_parents(config_dir, 0755);
                g_print("Created config directory: %s\n", config_dir);
            }
        } else {
            config_dir = g_strdup("/tmp");
        }
    }
    
    return config_dir;
}

/**
 * Get the configuration file path.
 *
 * Constructs the full path to "tools_view_mode.conf" inside the config directory.
 * The path is cached for subsequent calls.
 *
 * @return Static pointer to the config file path.
 */
static const char* get_config_path(void)

{
    if (config_path == NULL) {
        config_path = g_build_filename(get_config_dir(), "tools_view_mode.conf", NULL);
        g_print("Config file path: %s\n", config_path);
    }
    
    return config_path;
}

/**
 * Save the current view mode to the configuration file.
 *
 * Writes the integer value of current_mode to the config file.
 * Called after mode changes to persist user preference.
 */
void view_mode_save(void)

{
    const char *path = get_config_path();
    FILE *fp = fopen(path, "w");
    
    if (fp) {
        fprintf(fp, "%d\n", current_mode);
        fclose(fp);
        g_print("Saved view mode %d to %s\n", current_mode, path);
    } else {
        g_print("Failed to save view mode to %s\n", path);
    }
}

/**
 * Load the view mode from the configuration file.
 *
 * Reads the integer value from the config file and validates it against
 * VIEW_MODE_LIST and VIEW_MODE_GRID. Falls back to LIST mode if the file
 * does not exist or contains an invalid value.
 */
void view_mode_load(void)

{
    const char *path = get_config_path();
    FILE *fp = fopen(path, "r");
    
    if (fp) {
        int mode;
        if (fscanf(fp, "%d", &mode) == 1) {
            if (mode == VIEW_MODE_LIST || mode == VIEW_MODE_GRID) {
                current_mode = (ViewMode)mode;
                g_print("Loaded view mode %d from %s\n", current_mode, path);
            }
        }
        fclose(fp);
    } else {
        g_print("No config file found, using default LIST mode\n");
    }
    
    is_loaded = 1;
}

/**
 * Get the current view mode.
 *
 * Loads the configuration file on first call if not already loaded.
 *
 * @return The current ViewMode (LIST or GRID).
 */
ViewMode view_mode_get_current(void)

{
    if (!is_loaded) {
        view_mode_load();
    }
    return current_mode;
}

/**
 * Set the current view mode programmatically.
 *
 * @param mode The new view mode (must be VIEW_MODE_LIST or VIEW_MODE_GRID).
 *
 * Validates input and logs a warning if an invalid mode is provided.
 */
void view_mode_set_current(ViewMode mode)
{
    if (mode == VIEW_MODE_LIST || mode == VIEW_MODE_GRID) {
        current_mode = mode;
        g_print("View mode set to %d\n", mode);
    } else {
        g_warning("Invalid view mode: %d", mode);
    }
}

/**
 * Populate a container with tools in list view layout.
 *
 * @param container GtkBox to pack buttons into.
 * @param tools     Array of ToolItem structures.
 * @param num_tools Number of tools in the array.
 * @param window    Parent window pointer passed to button callbacks.
 *
 * Creates a vertical list of buttons, each displaying icon and label.
 */
static void populate_list_view(GtkWidget *container, const ToolItem *tools, int num_tools, gpointer window)

{
    for (int i = 0; i < num_tools; i++) {
        char *label_text = g_strdup_printf("%s %s", tools[i].icon, tools[i].label);
        GtkWidget *button = gtk_button_new_with_label(label_text);
        g_free(label_text);
        
        gtk_widget_set_size_request(button, -1, 35);
        g_object_set_data(G_OBJECT(button), "window", window);
        g_signal_connect(button, "clicked", G_CALLBACK(tools[i].callback), window);
        gtk_box_pack_start(GTK_BOX(container), button, FALSE, FALSE, 2);
    }
}

/**
 * Populate a container with tools in grid view layout.
 *
 * @param container GtkGrid to pack items into.
 * @param tools     Array of ToolItem structures.
 * @param num_tools Number of tools in the array.
 * @param window    Parent window pointer passed to event handlers.
 *
 * Creates a 2-column grid where each tool is displayed as a vertical box
 * with a large icon and label. Click events are handled via button-press-event
 * on GtkEventBox.
 */
static void populate_grid_view(GtkWidget *container, const ToolItem *tools, int num_tools, gpointer window)

{
    int cols = 2;
    
    for (int i = 0; i < num_tools; i++) {
        GtkWidget *grid_item = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        gtk_widget_set_size_request(grid_item, 120, 100);
        
        char *icon_markup = g_strdup_printf("<span font='32' foreground='#00ff88'>%s</span>", tools[i].icon);
        GtkWidget *icon_label = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(icon_label), icon_markup);
        g_free(icon_markup);
        
        GtkWidget *name_label = gtk_label_new(tools[i].label);
        
        gtk_box_pack_start(GTK_BOX(grid_item), icon_label, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(grid_item), name_label, FALSE, FALSE, 0);
        
        GtkWidget *event_box = gtk_event_box_new();
        gtk_container_add(GTK_CONTAINER(event_box), grid_item);
        
        g_object_set_data(G_OBJECT(event_box), "window", window);
        g_signal_connect(event_box, "button-press-event", G_CALLBACK(tools[i].callback_event), window);
        
        int row = i / cols;
        int col = i % cols;
        gtk_grid_attach(GTK_GRID(container), event_box, col, row, 1, 1);
    }
    
    gtk_grid_set_column_homogeneous(GTK_GRID(container), TRUE);
    gtk_grid_set_row_homogeneous(GTK_GRID(container), TRUE);
}

/**
 * Create a container widget populated with tools in the specified view mode.
 *
 * @param tools     Array of ToolItem structures.
 * @param num_tools Number of tools in the array.
 * @param mode      The view mode to use (LIST or GRID).
 * @param window    Parent window pointer passed to callbacks.
 * @return Newly created GtkWidget (either GtkBox or GtkGrid) containing
 *         all tools in the specified layout.
 */
GtkWidget* view_mode_create_container(const ToolItem *tools, int num_tools, ViewMode mode, gpointer window)

{
    GtkWidget *container;
    
    if (mode == VIEW_MODE_LIST) {
        container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
        populate_list_view(container, tools, num_tools, window);
    } else {
        container = gtk_grid_new();
        gtk_grid_set_row_spacing(GTK_GRID(container), 10);
        gtk_grid_set_column_spacing(GTK_GRID(container), 10);
        populate_grid_view(container, tools, num_tools, window);
    }
    
    current_mode = mode;
    gtk_widget_show_all(container);
    
    return container;
}

/**
 * Toggle the view mode and replace the container.
 *
 * @param old_container The current container to destroy.
 * @param tools         Array of ToolItem structures.
 * @param num_tools     Number of tools in the array.
 * @param parent_box    Parent GtkBox where the container is packed.
 * @param window        Parent window pointer passed to callbacks.
 * @return Newly created container with the toggled view mode.
 *
 * Destroys the old container, creates a new one with the opposite mode,
 * packs it into the parent box at position 2, and saves the new mode.
 * Intended for use with a toggle button that swaps the display.
 */
GtkWidget* view_mode_toggle(GtkWidget *old_container, const ToolItem *tools, int num_tools, GtkWidget *parent_box, gpointer window)

{
    ViewMode new_mode = (current_mode == VIEW_MODE_LIST) ? VIEW_MODE_GRID : VIEW_MODE_LIST;
    
    if (old_container) {
        gtk_widget_destroy(old_container);
    }
    
    GtkWidget *new_container = view_mode_create_container(tools, num_tools, new_mode, window);
    
    gtk_box_pack_start(GTK_BOX(parent_box), new_container, TRUE, TRUE, 0);
    gtk_box_reorder_child(GTK_BOX(parent_box), new_container, 2);
    
    view_mode_save();
    
    return new_container;
}