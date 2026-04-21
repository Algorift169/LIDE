#include "Otab.h"
#include "FileChooser.h"
#include <X11/Xatom.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

/*
 * O_tab.c
 * 
 * Right-click context menu for the window manager desktop.
 * Pure X11 implementation with modern styling.
 */

static Window menu_window = None;
static int menu_active = 0;
static int menu_item_height = 28;
static int menu_width = 220;
static int menu_item_count = 7;
static int menu_x = 0, menu_y = 0;

/* Menu item rectangles for hit testing */
typedef struct {
    int x, y, width, height;
    const char *label;
    void (*callback)(void);
} MenuItem;

static MenuItem menu_items[7];

/* Function pointers for external actions */
static void (*external_new_folder)(const char *path, const char *name) = NULL;
static void (*external_new_file)(const char *path, const char *name) = NULL;
static void (*external_open_terminal)(void) = NULL;
static void (*external_change_background)(void) = NULL;
static void (*external_display_settings)(void) = NULL;
static void (*external_show_files)(void) = NULL;
static void (*external_open_file_manager)(void) = NULL;
static char *(*external_get_desktop_path)(void) = NULL;

/**
 * Registers external callback functions for menu actions.
 */
void register_context_menu_callbacks(
    void (*new_folder_cb)(const char *, const char *),
    void (*new_file_cb)(const char *, const char *),
    void (*terminal_cb)(void),
    void (*change_bg_cb)(void),
    void (*display_settings_cb)(void),
    void (*show_files_cb)(void),
    char *(*get_desktop_path_cb)(void))
{
    external_new_folder = new_folder_cb;
    external_new_file = new_file_cb;
    external_open_terminal = terminal_cb;
    external_change_background = change_bg_cb;
    external_display_settings = display_settings_cb;
    external_show_files = show_files_cb;
    external_get_desktop_path = get_desktop_path_cb;
}

/**
 * Runs file chooser dialog in a separate process.
 * Spawns the dialog without blocking the window manager.
 */
static void run_file_chooser(const char *title, const char *default_name)
{
    pid_t pid = fork();
    if (pid == 0) {
        // Child process - run custom file chooser dialog

        // Set up display before gtk_init
        Display *d = XOpenDisplay(NULL);
        if (!d) {
            exit(1);
        }

        int argc = 0;
        char **argv = NULL;
        gtk_init(&argc, &argv);

        FileChooserMode mode = CHOOSER_FILE;
        if (strcmp(title, "Create New Folder") == 0) {
            mode = CHOOSER_FOLDER;
        }

        FileChooser *chooser = file_chooser_new(mode, NULL);
        if (!chooser) {
            XCloseDisplay(d);
            exit(1);
        }

        /* Set default filename */
        gtk_entry_set_text(GTK_ENTRY(chooser->filename_entry), default_name);

        char *selected_path = file_chooser_show(chooser);

        if (selected_path) {
            char *dir_path = g_path_get_dirname(selected_path);
            char *base_name = g_path_get_basename(selected_path);

            if (strcmp(title, "Create New Folder") == 0) {
                if (external_new_folder) {
                    external_new_folder(dir_path, base_name);
                }
            } else if (strcmp(title, "Create New File") == 0) {
                if (external_new_file) {
                    external_new_file(dir_path, base_name);
                }
            }

            g_free(dir_path);
            g_free(base_name);
            g_free(selected_path);

            // Refresh desktop
            if (external_show_files) {
                external_show_files();
            }
        }

        file_chooser_destroy(chooser);
        XCloseDisplay(d);
        exit(0);
    }
    // Parent continues immediately without waiting - child runs independently
}

/* Menu action implementations */
static void action_new_folder(void)
{
    run_file_chooser("Create New Folder", "New Folder");
}

static void action_new_file(void)
{
    run_file_chooser("Create New File", "New File.txt");
}

static void action_terminal(void)
{
    if (external_open_terminal) external_open_terminal();
}

static void action_change_bg(void)
{
    if (external_change_background) external_change_background();
}

static void action_display_settings(void)
{
    if (external_display_settings) external_display_settings();
}

static void action_show_files(void)
{
    if (external_show_files) external_show_files();
}

static void action_open_file_manager(void)
{
    if (external_open_file_manager) {
        external_open_file_manager();
    } else {
        pid_t pid = fork();
        if (pid == 0) {
            execl("./blackline-fm", "blackline-fm", NULL);
            exit(0);
        }
    }
}

/**
 * Creates the menu window with modern styling.
 */
static void create_menu_window(Display *d, int screen, Window root, int x, int y)
{
    int menu_height = menu_item_count * menu_item_height + 8;
    
    XSetWindowAttributes attr;
    attr.override_redirect = True;
    attr.background_pixel = 0x1e1e1e;
    attr.border_pixel = 0x3c3c3c;
    attr.event_mask = ButtonPressMask | ButtonReleaseMask | ExposureMask | EnterWindowMask | LeaveWindowMask;
    
    menu_window = XCreateWindow(d, root, x, y, menu_width, menu_height, 1,
                                CopyFromParent, InputOutput, CopyFromParent,
                                CWOverrideRedirect | CWBackPixel | CWBorderPixel | CWEventMask,
                                &attr);
    
    Atom net_wm_window_type = XInternAtom(d, "_NET_WM_WINDOW_TYPE", False);
    Atom net_wm_window_type_dock = XInternAtom(d, "_NET_WM_WINDOW_TYPE_DOCK", False);
    Atom xa_atom = XInternAtom(d, "ATOM", False);
    XChangeProperty(d, menu_window, net_wm_window_type, xa_atom, 32,
                   PropModeReplace, (unsigned char*)&net_wm_window_type_dock, 1);
    
    XStoreName(d, menu_window, "Context Menu");
    XMapWindow(d, menu_window);
    XRaiseWindow(d, menu_window);
}

/**
 * Draws the menu items with modern styling.
 */
static void draw_menu(Display *d)
{
    if (menu_window == None) return;
    
    GC gc = XCreateGC(d, menu_window, 0, NULL);
    
    XSetForeground(d, gc, 0x1e1e1e);
    XFillRectangle(d, menu_window, gc, 0, 0, menu_width, menu_item_count * menu_item_height + 8);
    
    const char *labels[] = {
        "New Folder",
        "New File",
        "Open File Manager",
        "Open Terminal",
        "Change Background",
        "Display Settings",
        "Refresh Desktop"
    };
    
    int y_pos = 4;
    
    for (int i = 0; i < menu_item_count; i++) {
        menu_items[i].x = 0;
        menu_items[i].y = y_pos;
        menu_items[i].width = menu_width;
        menu_items[i].height = menu_item_height;
        menu_items[i].label = labels[i];
        
        switch(i) {
            case 0: menu_items[i].callback = action_new_folder; break;
            case 1: menu_items[i].callback = action_new_file; break;
            case 2: menu_items[i].callback = action_open_file_manager; break;
            case 3: menu_items[i].callback = action_terminal; break;
            case 4: menu_items[i].callback = action_change_bg; break;
            case 5: menu_items[i].callback = action_display_settings; break;
            case 6: menu_items[i].callback = action_show_files; break;
            default: menu_items[i].callback = NULL;
        }
        
        XSetForeground(d, gc, 0x2d2d2d);
        XFillRectangle(d, menu_window, gc, 2, y_pos, menu_width - 4, menu_item_height - 2);
        
        XSetForeground(d, gc, 0xe0e0e0);
        XDrawString(d, menu_window, gc, 12, y_pos + 20, labels[i], strlen(labels[i]));
        
        y_pos += menu_item_height;
    }
    
    XSetForeground(d, gc, 0x3c3c3c);
    XDrawRectangle(d, menu_window, gc, 0, 0, menu_width - 1, menu_item_count * menu_item_height + 7);
    
    XFreeGC(d, gc);
}

/**
 * Shows the context menu at the specified coordinates.
 */
void show_context_menu(Display *d, int x, int y)
{
    if (menu_window != None) {
        XDestroyWindow(d, menu_window);
        menu_window = None;
    }
    
    int screen = DefaultScreen(d);
    Window root = RootWindow(d, screen);
    
    int screen_width = DisplayWidth(d, screen);
    int screen_height = DisplayHeight(d, screen);
    int menu_height = menu_item_count * menu_item_height + 8;
    
    if (x + menu_width > screen_width) {
        x = screen_width - menu_width - 10;
    }
    if (y + menu_height > screen_height) {
        y = screen_height - menu_height - 10;
    }
    
    menu_x = x;
    menu_y = y;
    menu_active = 1;
    
    create_menu_window(d, screen, root, x, y);
    draw_menu(d);
    
    XFlush(d);
}

/**
 * Handles button presses on the context menu.
 */
int handle_menu_button(Display *d, XButtonEvent *ev)
{
    if (!menu_active || menu_window == None) return 0;

    if (ev->window == menu_window) {
        if (ev->button == Button1) {
            int y = ev->y - 4;
            int item_index = y / menu_item_height;

            if (item_index >= 0 && item_index < menu_item_count) {
                if (menu_items[item_index].callback) {
                    menu_items[item_index].callback();
                }
            }
        }

        menu_active = 0;
        XDestroyWindow(d, menu_window);
        menu_window = None;
        XFlush(d);  /* Ensure X11 processes the destroy immediately */
        XSync(d, False);  /* Synchronize with X server */
        return 1;
    }

    if (menu_active) {
        menu_active = 0;
        XDestroyWindow(d, menu_window);
        menu_window = None;
        XFlush(d);
        XSync(d, False);
        return 1;
    }

    return 0;
}

/**
 * Cleans up context menu resources.
 */
void context_menu_cleanup(void)
{
    menu_active = 0;
    if (menu_window != None) {
        menu_window = None;
    }
}

/**
 * Checks if the context menu is currently active.
 */
int is_menu_active(void)
{
    return menu_active;
}