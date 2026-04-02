#include "wm.h"
#include <stdio.h>
#include <time.h>


/*
 * wm_debug.c
 * 
 * Debugging and diagnostics for the window manager
Provides inspection of X11 event queue, window property dump,
focus state tracking, and event logging utilities.
 *
 * This module is part of the LIDE desktop environment system.
 * See the main window manager (wm/) and session management (session/)
 * for system architecture overview.
 */

static FILE *log_file = NULL;

/**
 * Write a timestamped message to the debug log file.
 *
 * @param msg Message string to log.
 *
 * Writes to /tmp/blackline-wm-debug.log. Does nothing if log file is not open.
 */
static void log_message(const char *msg) 

{
    if (!log_file) return;
    time_t now = time(NULL);
    fprintf(log_file, "[%ld] %s\n", now, msg);
    fflush(log_file);
}

/**
 * X11 error handler for debugging.
 *
 * @param d X11 display.
 * @param e X11 error event.
 * @return 0 (XSetErrorHandler expects 0 return).
 *
 * Logs error messages to the debug file without terminating the WM.
 * Overrides the default error handler to prevent crashes from non-fatal X errors.
 */
static int debug_error_handler(Display *d, XErrorEvent *e) 

{
    char buf[1024];
    XGetErrorText(d, e->error_code, buf, sizeof(buf));
    log_message(buf);
    return 0;
}

/**
 * Main entry point for the BlackLine Window Manager.
 *
 * Opens X display, sets up error handling, initializes the WM state,
 * grabs keyboard shortcuts, and enters the main event loop.
 *
 * Side effects:
 * - Creates debug log file at /tmp/blackline-wm-debug.log
 * - Grabs keys on the root window (Mod4+Return, Mod4+Shift+c, etc.)
 * - Reparents and manages client windows
 *
 * The WM runs indefinitely until terminated by a signal or X server shutdown.
 *
 * @return 0 on normal exit (unreachable in typical operation), 1 on fatal error.
 */
int main(void) 

{
    log_file = fopen("/tmp/blackline-wm-debug.log", "w");
    log_message("Window manager starting");
    
    state.display = XOpenDisplay(NULL);
    if (!state.display) 
    {
        log_message("Cannot open display");
        if (log_file) fclose(log_file);
        return 1;
    }
    log_message("Display opened");

    XSetErrorHandler(debug_error_handler);

    state.screen = DefaultScreen(state.display);
    state.root = RootWindow(state.display, state.screen);
    log_message("Root window obtained");

    state.cursor = XCreateFontCursor(state.display, XC_left_ptr);
    XDefineCursor(state.display, state.root, state.cursor);
    log_message("Cursor set");

    state.wm_delete_window = XInternAtom(state.display, "WM_DELETE_WINDOW", False);
    state.wm_protocols = XInternAtom(state.display, "WM_PROTOCOLS", False);

    Colormap cmap = DefaultColormap(state.display, state.screen);
    XColor focused_color, unused;
    XAllocNamedColor(state.display, cmap, FOCUSED_COLOR, &focused_color, &unused);
    state.focused_pixel = focused_color.pixel;
    XAllocNamedColor(state.display, cmap, UNFOCUSED_COLOR, &focused_color, &unused);
    state.unfocused_pixel = focused_color.pixel;

    XSelectInput(state.display, state.root, 
                 SubstructureRedirectMask | 
                 SubstructureNotifyMask | 
                 KeyPressMask | 
                 EnterWindowMask |
                 ButtonPressMask);

    grab_keys();
    XSync(state.display, False);
    log_message("Keys grabbed, entering main loop");

    XEvent ev;

    while (1) {
        XNextEvent(state.display, &ev);
        // Log every 100 events to avoid log spam
        static int event_count = 0;
        if (++event_count % 100 == 0)
        {
            char buf[256];
            snprintf(buf, sizeof(buf), "Processing event type %d", ev.type);
            log_message(buf);
        }

        switch (ev.type)
        {
            case MapRequest:
                map_request(&ev.xmaprequest);
                break;
            case ConfigureRequest:
                handle_configure_request(&ev.xconfigurerequest);
                break;
            case DestroyNotify:
                destroy_notify(&ev.xdestroywindow);
                break;
            case EnterNotify:
                enter_notify(&ev.xcrossing);
                break;
            case KeyPress:
                handle_keypress(&ev.xkey);
                break;
            case ButtonPress:
                handle_button_press(&ev.xbutton);
                break;
            case ButtonRelease:
                handle_button_release(&ev.xbutton);
                break;
            case MotionNotify:
                handle_motion_notify(&ev.xmotion);
                break;
        }
    }

    log_message("Window manager exiting");
    XCloseDisplay(state.display);
    if (log_file) fclose(log_file);
    return 0;
}