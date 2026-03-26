#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

int running = 1;

/**
 * Signal handler for termination signals.
 *
 * @param sig Signal number received (SIGINT or SIGTERM).
 *
 * Sets the global running flag to 0 to break the main loop.
 * No cleanup performed; process exits naturally after loop termination.
 */
void handle_signal(int sig) 
{
    running = 0;
}

int main() 
{
    /* Register signal handlers for graceful shutdown */
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    
    const char *wallpaper = "./images/wal1.png";
    
    printf("Background service started\n");
    
    while (running) {
        /* Set wallpaper */
        if (access(wallpaper, F_OK) == 0) {
            char cmd[256];
            /* Construct feh command to set wallpaper with scaling */
            snprintf(cmd, sizeof(cmd), "feh --bg-scale '%s' 2>/dev/null", wallpaper);
            system(cmd);
        } else {
            /* Fallback to solid color if wallpaper file not found */
            system("xsetroot -solid '#0b0f14' 2>/dev/null");
        }
        
        /* Polling interval to override window manager wallpaper changes */
        sleep(2);
    }
    
    return 0;
}