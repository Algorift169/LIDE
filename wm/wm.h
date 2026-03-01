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

typedef struct WindowNode {
    Window window;
    Window titlebar;
    int x, y, width, height;
    int is_moving;
    int move_start_x, move_start_y;
    struct WindowNode *next;
} WindowNode;

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

void grab_keys(void);
void handle_keypress(XKeyEvent *ev);
void add_window(Window w);
void remove_window(Window w);
void focus_window(Window w);
void set_border(Window w, int focused);
void update_borders(void);
Window create_titlebar(Window client, int width);
void handle_button_press(XButtonEvent *ev);
void handle_button_release(XButtonEvent *ev);
void handle_motion_notify(XMotionEvent *ev);
void handle_configure_request(XConfigureRequestEvent *ev);

#endif