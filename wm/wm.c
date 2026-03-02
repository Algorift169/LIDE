#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>  
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) 
{
    Display *d = XOpenDisplay(NULL);
    if (!d) 
    {
        fprintf(stderr, "Cannot open display\n");
        return 1;
    }
    
    int screen = DefaultScreen(d);
    Window root = RootWindow(d, screen);
    
    // Create and set a visible cursor
    Cursor cursor = XCreateFontCursor(d, XC_left_ptr);  
    XDefineCursor(d, root, cursor);
    
    // Select events but DON'T override redirect for the root window
    XSelectInput(d, root, SubstructureNotifyMask | ButtonPressMask | KeyPressMask);
    
    // Set event mask for root to allow wallpaper to show
    XWindowAttributes attr;
    XGetWindowAttributes(d, root, &attr);
    XSelectInput(d, root, attr.your_event_mask | SubstructureRedirectMask);
    
    XEvent ev;
    while (1)
     {
        XNextEvent(d, &ev);
        
        switch(ev.type) {
            case MapRequest: {
                // Map the window but don't change its background
                XMapWindow(d, ev.xmaprequest.window);
                
                // Raise the window but don't cover the whole screen
                XRaiseWindow(d, ev.xmaprequest.window);
                break;
            }
            
            case ConfigureRequest: {
                XConfigureRequestEvent *e = &ev.xconfigurerequest;
                XWindowChanges changes;
                changes.x = e->x;
                changes.y = e->y;
                changes.width = e->width;
                changes.height = e->height;
                changes.border_width = e->border_width;
                changes.sibling = e->above;
                changes.stack_mode = e->detail;
                XConfigureWindow(d, e->window, e->value_mask, &changes);
                break;
            }
            
            case DestroyNotify:
                // Window destroyed, nothing to do
                break;
        }
    }
    
    return 0;
}