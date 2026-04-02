#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <glib.h>

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

/**
 * Read the current wallpaper path from config file.
 *
 * @param wallpaper_path Buffer to store the wallpaper path (must be at least 512 bytes)
 * @return 1 if config file was read successfully, 0 otherwise
 *
 * Config file location: ~/.config/blackline/current_wallpaper.conf
 * If config file doesn't exist or can't be read, uses default wallpaper.
 */
static int read_wallpaper_config(char *wallpaper_path)
{
    const char *home = g_getenv("HOME");
    if (!home) home = "/root";
    
    char config_path[512];
    snprintf(config_path, sizeof(config_path), "%s/.config/blackline/current_wallpaper.conf", home);
    
    FILE *f = fopen(config_path, "r");
    if (!f) {
        /* Use default wallpaper if config doesn't exist */
        snprintf(wallpaper_path, 512, "%s/Desktop/LIDE/images/walpapers/wal1.png", home);
        printf("[DEBUG] Config not found, using default: %s\n", wallpaper_path);
        return 0;
    }
    
    /* Read wallpaper path from config file */
    if (fgets(wallpaper_path, 512, f) != NULL) {
        /* Remove trailing newline */
        size_t len = strlen(wallpaper_path);
        if (len > 0 && wallpaper_path[len - 1] == '\n') {
            wallpaper_path[len - 1] = '\0';
        }
        printf("[DEBUG] Read from config: %s\n", wallpaper_path);
        fclose(f);
        fflush(stdout);
        return 1;
    }
    
    fclose(f);
    snprintf(wallpaper_path, 512, "%s/Desktop/LIDE/images/walpapers/wal1.png", home);
    printf("[DEBUG] Failed to read config, using default: %s\n", wallpaper_path);
    return 0;
}

int main() 
{
    /* Register signal handlers for graceful shutdown */
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    
    printf("Background service started\n");
    
    char last_wallpaper[512] = "";
    FILE *debug_log = fopen("/tmp/background_debug.log", "w");
    if (debug_log) fprintf(debug_log, "Background service started\n");
    
    while (running) {
        /* Read current wallpaper from config file */
        char wallpaper_path[512];
        read_wallpaper_config(wallpaper_path);
        
        /* Only apply if wallpaper changed to avoid excessive system() calls */
        if (strcmp(wallpaper_path, last_wallpaper) != 0) {
            printf("[DEBUG] Wallpaper changed, applying: %s\n", wallpaper_path);
            if (debug_log) {
                fprintf(debug_log, "[BG] Wallpaper changed: %s\n", wallpaper_path);
                fflush(debug_log);
            }
            strncpy(last_wallpaper, wallpaper_path, sizeof(last_wallpaper) - 1);
            
            /* Set wallpaper */
            if (access(wallpaper_path, F_OK) == 0) {
                char cmd[1024];
                /* Construct feh command to set wallpaper with scaling */
                snprintf(cmd, sizeof(cmd), "feh --bg-scale '%s' 2>&1", wallpaper_path);
                printf("[DEBUG] Executing: %s\n", cmd);
                if (debug_log) {
                    fprintf(debug_log, "[BG] Executing: %s\n", cmd);
                    fflush(debug_log);
                }
                int ret = system(cmd);
                printf("[DEBUG] feh returned: %d\n", ret);
                if (debug_log) {
                    fprintf(debug_log, "[BG] feh returned: %d\n", ret);
                    fflush(debug_log);
                }
            } else {
                printf("[DEBUG] Wallpaper file not found: %s\n", wallpaper_path);
                if (debug_log) {
                    fprintf(debug_log, "[BG] Wallpaper not found: %s\n", wallpaper_path);
                    fflush(debug_log);
                }
                /* Fallback to solid color if wallpaper file not found */
                system("xsetroot -solid '#0b0f14'");
            }
        }
        
        /* Polling interval to override window manager wallpaper changes */
        sleep(2);
    }
    
    if (debug_log) fclose(debug_log);
    return 0;
}