#ifndef LIDE_WM_H
#define LIDE_WM_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#define LIDE_WM_CLASS "BlackLineWM"
#define BORDER_WIDTH 2
#define FOCUSED_COLOR "#00ff88"
#define UNFOCUSED_COLOR "#888888"
#define TITLEBAR_HEIGHT 25

/**

/*
 * wm.h
 * 
 * Window manager core definitions and X11 client management interface
Defines linked list structures for managed windows, titlebar frames, and 
focus/border state. Declares public interface for keyboard event handling,
window lifecycle operations, and border color management.
 *
 * This module is part of the LIDE desktop environment system.
 * See the main window manager (wm/) and session management (session/)
 * for system architecture overview.
 */

 * Linked list node representing a managed client window.
 *
 * @window      X11 window ID of the client window.
 * @titlebar    X11 window ID of the synthetic titlebar frame.
 * @x,y         Current position of the window (including titlebar).
 * @width,height Current dimensions of the client window (excluding titlebar).
 * @is_moving   Flag indicating an ongoing move/resize operation.
 * @move_start_x,move_start_start Mouse coordinates when move began.
 * @next        Pointer to next node in the list.
 */
typedef struct WindowNode {
    Window window;
    Window titlebar;
    int x, y, width, height;
    int is_moving;
    int move_start_x, move_start_y;
    struct WindowNode *next;
} WindowNode;

/**
 * Global window manager state.
 *
 * @display         X11 display connection.
 * @screen          Default screen number.
 * @root            Root window ID.
 * @focused         Currently focused client window (None if none).
 * @windows         Linked list of managed windows.
 * @wm_delete_window Atom for WM_DELETE_WINDOW protocol.
 * @wm_protocols    Atom for WM_PROTOCOLS.
 * @focused_pixel   X11 color pixel for focused border.
 * @unfocused_pixel X11 color pixel for unfocused border.
 * @cursor          Default cursor for the root window.
 */
typedef struct {
    Display *display;
    int screen;
    Window root;
    Window focused;
    WindowNode *windows;
    Atom wm_delete_window;
    Atom wm_protocols;
    unsigned long focused_pixel;
    unsigned long unfocused_pixel;
    Cursor cursor;
} WmState;

extern WmState state;

/**
 * Grab keyboard shortcuts on the root window.
 *
 * Registers key bindings for window management operations such as
 * launching terminal, closing windows, and cycling focus.
 */
void grab_keys(void);

/**
 * Handle key press events.
 *
 * @param ev Key press event.
 *
 * Executes bound actions: launches terminal on Mod4+Return,
 * closes focused window on Mod4+Shift+c, cycles focus on Mod4+Tab,
 * and exits the WM on Mod4+Shift+q.
 */
void handle_keypress(XKeyEvent *ev);

/**
 * Add a new window to the window manager's managed list.
 *
 * @param w X11 window ID to manage.
 *
 * Creates a titlebar frame, reparents the client window, sets initial
 * dimensions, and adds the window to the linked list.
 */
void add_window(Window w);

/**
 * Remove a window from the window manager's managed list.
 *
 * @param w X11 window ID to unmanage.
 *
 * Reparents the window back to root, destroys the titlebar, and frees
 * the associated node from the linked list.
 */
void remove_window(Window w);

/**
 * Focus a specific window.
 *
 * @param w Window ID to focus.
 *
 * Sets the focus on the client window, updates border colors for all
 * windows, and updates the internal focused pointer.
 */
void focus_window(Window w);

/**
 * Set the border color for a window.
 *
 * @param w       Window ID (either client or titlebar).
 * @param focused Non-zero for focused color, zero for unfocused.
 *
 * Updates the border color using XSetWindowBorder.
 */
void set_border(Window w, int focused);

/**
 * Update border colors for all managed windows.
 *
 * Iterates through the window list and sets border colors based on
 * the current focused window.
 */
void update_borders(void);

/**
 * Create a synthetic titlebar window for a client.
 *
 * @param client The client window ID.
 * @param width  Initial width of the titlebar.
 * @return X11 window ID of the created titlebar.
 *
 * Creates a window that serves as a draggable titlebar for moving
 * the client window. The titlebar is reparented under the client's
 * parent (the frame) and is positioned above the client.
 */
Window create_titlebar(Window client, int width);

/**
 * Handle button press events for window management.
 *
 * @param ev Button press event.
 *
 * Initiates window move operations when clicking on titlebars.
 * Also handles raising windows on focus.
 */
void handle_button_press(XButtonEvent *ev);

/**
 * Handle button release events.
 *
 * @param ev Button release event.
 *
 * Terminates any ongoing move operation.
 */
void handle_button_release(XButtonEvent *ev);

/**
 * Handle motion notify events for window dragging.
 *
 * @param ev Motion notify event.
 *
 * Updates window position when a move operation is in progress.
 */
void handle_motion_notify(XMotionEvent *ev);

/**
 * Handle configure request events from clients.
 *
 * @param ev Configure request event.
 *
 * Responds to client requests to change position or size.
 * The window manager may override or adjust these requests
 * to maintain consistent layout.
 */
void handle_configure_request(XConfigureRequestEvent *ev);

#endif /* LIDE_WM_H */