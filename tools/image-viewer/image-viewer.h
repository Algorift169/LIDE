#ifndef IMAGE_VIEWER_H
#define IMAGE_VIEWER_H

#include <gtk/gtk.h>

/**
 * Launches the BlackLine Image Viewer as a separate process.
 * Spawns a new instance of the image viewer application using g_spawn_async.
 *
 * @param filename Path to the image file to open.
 *                 If NULL or empty, launches the viewer without an image.
 *
 * @sideeffect Forks a child process and executes blackline-image-viewer.
 * @sideeffect Any spawn errors are silently ignored (error freed internally).
 * @note The function returns immediately; does not wait for the viewer to exit.
 */
void launch_image_viewer(const char *filename);

#endif /* IMAGE_VIEWER_H */