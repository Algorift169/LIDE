#include "wm.h"

/**
 * Create a synthetic titlebar window for a client window.
 *
 * @param client The client window ID (unused, kept for API consistency).
 * @param width  Initial width of the titlebar in pixels.
 * @return X11 window ID of the created titlebar.
 *
 * Creates a window with background color #0b0f14 and the current focused
 * border color. The titlebar receives button events for window dragging.
 * Initially mapped but not yet reparented; the caller must reparent both
 * the titlebar and client window appropriately.
 */
Window create_titlebar(Window client, int width) 

{
    (void)client;
    XSetWindowAttributes attr;
    attr.background_pixel = 0x0b0f14;
    attr.border_pixel = state.focused_pixel;
    attr.event_mask = ButtonPressMask | ButtonReleaseMask | ExposureMask;
    
    Window titlebar = XCreateWindow(state.display, state.root,
                                    0, 0, width, TITLEBAR_HEIGHT, 1,
                                    CopyFromParent, InputOutput, CopyFromParent,
                                    CWBackPixel | CWBorderPixel | CWEventMask, &attr);
    
    XMapWindow(state.display, titlebar);
    return titlebar;
}

/**
 * Add a new window to the window manager's managed list.
 *
 * @param w X11 window ID to manage.
 *
 * Steps:
 * - Retrieves current window attributes.
 * - Creates a WindowNode structure.
 * - Creates a titlebar window.
 * - Reparents the client window into the titlebar frame.
 * - Reparents the titlebar to the root window at the client's original position.
 * - Adds the node to the global window list.
 * - Sets the initial border color (unfocused).
 */
void add_window(Window w) 

{
    XWindowAttributes attrs;
    XGetWindowAttributes(state.display, w, &attrs);
    
    WindowNode *node = malloc(sizeof(WindowNode));
    node->window = w;
    node->x = attrs.x;
    node->y = attrs.y;
    node->width = attrs.width;
    node->height = attrs.height;
    node->is_moving = 0;
    
    node->titlebar = create_titlebar(w, attrs.width);
    XReparentWindow(state.display, w, node->titlebar, 0, TITLEBAR_HEIGHT);
    XReparentWindow(state.display, node->titlebar, state.root, attrs.x, attrs.y);
    
    node->next = state.windows;
    state.windows = node;

    set_border(w, 0);
}

/**
 * Remove a window from the window manager's managed list.
 *
 * @param w X11 window ID to unmanage.
 *
 * Destroys the titlebar, frees the node, and updates focus if the
 * removed window was focused. Does not destroy the client window itself.
 */
void remove_window(Window w) 

{
    WindowNode **p = &state.windows;
    while (*p) {
        if ((*p)->window == w) 
        {
            WindowNode *tmp = *p;
            *p = (*p)->next;
            XDestroyWindow(state.display, tmp->titlebar);
            free(tmp);
            break;
        }
        p = &(*p)->next;
    }

    if (state.focused == w) 
    {
        if (state.windows) {
            focus_window(state.windows->window);
        } else {
            state.focused = 0;
        }
    }
}

/**
 * Set input focus to a specific window.
 *
 * @param w X11 window ID to focus.
 *
 * Updates border colors for both the client and its titlebar,
 * raises the window and its titlebar in the stacking order,
 * and sets the X input focus. Does nothing if the window is already focused.
 */
void focus_window(Window w) 

{
    if (state.focused == w) return;

    if (state.focused) {
        set_border(state.focused, 0);
    }

    state.focused = w;
    if (w) {
        set_border(w, 1);
        XSetInputFocus(state.display, w, RevertToPointerRoot, CurrentTime);
        XRaiseWindow(state.display, w);
        
        WindowNode *node = state.windows;
        while (node) {
            if (node->window == w) {
                XRaiseWindow(state.display, node->titlebar);
                break;
            }
            node = node->next;
        }
    }
}

/**
 * Set the border color and width for a client window and its titlebar.
 *
 * @param w       X11 window ID of the client.
 * @param focused Non-zero for focused color, zero for unfocused.
 *
 * Updates both the client window border and the associated titlebar border.
 * Border width is always BORDER_WIDTH.
 */
void set_border(Window w, int focused)

{
    unsigned long pixel = focused ? state.focused_pixel : state.unfocused_pixel;
    XSetWindowBorder(state.display, w, pixel);
    XSetWindowBorderWidth(state.display, w, BORDER_WIDTH);
    
    WindowNode *node = state.windows;
    while (node) {
        if (node->window == w) {
            XSetWindowBorder(state.display, node->titlebar, pixel);
            break;
        }
        node = node->next;
    }
}

/**
 * Handle button press events on windows.
 *
 * @param ev X11 button event.
 *
 * If the button press occurs on a titlebar, initiates window moving.
 * If on a client window, sets focus and replays the event to the client.
 * For other windows, passes the event through.
 */
void handle_button_press(XButtonEvent *ev)

{
    // Find which window was clicked
    WindowNode *node = state.windows;
    while (node) {
        if (ev->window == node->titlebar) {
            // Clicked on titlebar - start moving
            node->is_moving = 1;
            node->move_start_x = ev->x_root;
            node->move_start_y = ev->y_root;
            focus_window(node->window);
            return;
        } else if (ev->window == node->window) {
            // Clicked on client window - pass through
            focus_window(node->window);
            XAllowEvents(state.display, ReplayPointer, CurrentTime);
            return;
        }
        node = node->next;
    }
    
    // If window not found, pass the event through
    XAllowEvents(state.display, ReplayPointer, CurrentTime);
}

/**
 * Handle button release events to end window moving.
 *
 * @param ev X11 button event.
 *
 * Clears the is_moving flag on the titlebar if the button was released
 * over a titlebar that was being dragged.
 */
void handle_button_release(XButtonEvent *ev)

{
    WindowNode *node = state.windows;
    while (node) {
        if (ev->window == node->titlebar) 
        {
            node->is_moving = 0;
            break;
        }
        node = node->next;
    }
}

/**
 * Handle motion notify events for window dragging.
 *
 * @param ev X11 motion event.
 *
 * Updates window position when a move operation is in progress.
 * Calculates the delta since the last motion event and moves the
 * titlebar accordingly.
 */
void handle_motion_notify(XMotionEvent *ev) 

{
    WindowNode *node = state.windows;
    while (node) {
        if (node->is_moving)
         {
            int dx = ev->x_root - node->move_start_x;
            int dy = ev->y_root - node->move_start_y;
            
            node->x += dx;
            node->y += dy;
            
            XMoveWindow(state.display, node->titlebar, node->x, node->y);
            
            node->move_start_x = ev->x_root;
            node->move_start_y = ev->y_root;
            break;
        }
        node = node->next;
    }
}

/**
 * Handle configure request events from clients.
 *
 * @param ev X11 configure request event.
 *
 * Updates the internal window geometry and resizes the titlebar
 * to match the client's new width. Passes the configure request
 * through to the client window.
 */
void handle_configure_request(XConfigureRequestEvent *ev) 

{
    XWindowChanges changes;
    changes.x = ev->x;
    changes.y = ev->y;
    changes.width = ev->width;
    changes.height = ev->height;
    changes.border_width = ev->border_width;
    changes.sibling = ev->above;
    changes.stack_mode = ev->detail;
    
    WindowNode *node = state.windows;
    while (node) {
        if (node->window == ev->window)
        {
            node->x = ev->x;
            node->y = ev->y;
            node->width = ev->width;
            node->height = ev->height;
            XMoveResizeWindow(state.display, node->titlebar, ev->x, ev->y, 
                             ev->width, TITLEBAR_HEIGHT);
            break;
        }
        node = node->next;
    }
    
    XConfigureWindow(state.display, ev->window, ev->value_mask, &changes);
}

/**
 * Update border colors for all managed windows.
 *
 * Iterates through the window list and applies the correct border color
 * based on whether each window is currently focused.
 */
void update_borders(void) 

{
    WindowNode *n = state.windows;
    while (n) {
        set_border(n->window, n->window == state.focused);
        n = n->next;
    }
}