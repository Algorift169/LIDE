#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/**
 * Main entry point for the wallpaper setter utility.
 *
 * Attempts to set a wallpaper image using feh. If the image file is not
 * found, falls back to a solid color background via xsetroot.
 *
 * @return 0 on success, non-zero on system command failure (not checked).
 *
 * Workflow:
 * - Checks for existence of the wallpaper file at ./LIDE/images/wal1.png.
 * - If present, executes feh with --bg-scale to set the wallpaper.
 * - Regardless of feh success, also sets a solid color via xsetroot as a
 *   fallback or to ensure consistent color scheme.
 * - If the wallpaper file is missing, sets only the solid color.
 *
 * Side effects: Executes external commands (feh, xsetroot) via system().
 * No error handling for subprocess failures.
 */
int main() 

{
    const char *wallpaper = "./LIDE/images/wal1.png";
    
    // Check if file exists
    if (access(wallpaper, F_OK) == 0) 
    {
        // Try feh
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "feh --bg-scale '%s'", wallpaper);
        system(cmd);
        
        // Also set with xsetroot as fallback
        system("xsetroot -solid '#0b0f14'");
        
        printf("Wallpaper set\n");
    } else {
        // Fallback to solid color
        system("xsetroot -solid '#0b0f14'");
        printf("Wallpaper not found, using solid color\n");
    }
    
    return 0;
}