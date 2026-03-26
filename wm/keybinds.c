#include "wm.h"
#include <X11/XKBlib.h>

/**
 * Grab keyboard shortcuts on the root window.
 *
 * Registers the following key bindings:
 * - Mod4+Return: Launch xterm terminal
 * - Mod4+q: Close the currently focused window
 * - Mod4+Space: Launch the BlackLine launcher
 *
 * Grabs are performed with GrabModeAsync to avoid blocking other clients.
 */
void grab_keys(void) 

{
    KeyCode mod4 = XKeysymToKeycode(state.display, XK_Super_L);
    if (!mod4) {
        fprintf(stderr, "Warning: Could not get Super key keycode\n");
        return;
    }
    
    KeyCode return_key = XKeysymToKeycode(state.display, XK_Return);
    
    if (return_key) 
    {
        XGrabKey(state.display, return_key, Mod4Mask, state.root, 
                True, GrabModeAsync, GrabModeAsync);
    }

    KeyCode q_key = XKeysymToKeycode(state.display, XK_q);
    
    if (q_key) {
        XGrabKey(state.display, q_key, Mod4Mask, state.root, 
                True, GrabModeAsync, GrabModeAsync);
    }

    KeyCode space_key = XKeysymToKeycode(state.display, XK_space);
    
    if (space_key) {
        XGrabKey(state.display, space_key, Mod4Mask, state.root, 
                True, GrabModeAsync, GrabModeAsync);
    }
}

/**
 * Spawn a child process with a new session.
 *
 * @param argv NULL-terminated argument vector for execvp.
 *
 * Forks a child, calls setsid() to create a new session, then executes
 * the specified program. The parent process returns immediately.
 */
static void spawn(char *const argv[]) 

{
    if (fork() == 0) {
        setsid();
        execvp(argv[0], argv);
        perror("execvp failed");
        exit(1);
    }
}

/**
 * Send a WM_DELETE_WINDOW protocol message to a client.
 *
 * @param w X11 window ID to close.
 *
 * Sends a ClientMessage event requesting the window to close gracefully.
 * This allows applications to perform cleanup or show save dialogs.
 */
static void close_window(Window w)

{
    XEvent ev;
    ev.xclient.type = ClientMessage;
    ev.xclient.window = w;
    ev.xclient.message_type = state.wm_protocols;
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = state.wm_delete_window;
    ev.xclient.data.l[1] = CurrentTime;
    XSendEvent(state.display, w, False, NoEventMask, &ev);
}

/**
 * Handle key press events for global shortcuts.
 *
 * @param ev X11 key event.
 *
 * Only processes events with Mod4Mask (Super key) modifier.
 * - Super+Return: Launch xterm
 * - Super+q: Close focused window
 * - Super+Space: Launch BlackLine launcher
 */
void handle_keypress(XKeyEvent *ev) 

{
    KeySym keysym = XkbKeycodeToKeysym(state.display, ev->keycode, 0, 0);
    unsigned int mods = ev->state;

    if (!(mods & Mod4Mask)) return;

    if (keysym == XK_Return) {
        char *const argv[] = { "xterm", NULL };
        spawn(argv);
    } else if (keysym == XK_q) {
        if (state.focused) {
            close_window(state.focused);
        }
    } else if (keysym == XK_space) {
        char *const argv[] = { "./blackline-launcher", NULL };
        spawn(argv);
    }
}