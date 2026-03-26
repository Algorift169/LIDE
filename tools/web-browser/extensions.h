#ifndef VOIDFOX_EXTENSIONS_H
#define VOIDFOX_EXTENSIONS_H

#include <gtk/gtk.h>
#include "voidfox.h"

/**
 * Browser theme structure.
 * Defines the color scheme for all UI components of the browser.
 */
typedef struct {
    const char *name;           /* Display name of the theme */
    const char *description;    /* Short description of the theme */
    const char *bg_color;       /* Main window background color */
    const char *text_color;     /* Default text color */
    const char *entry_bg;       /* Entry widget background color */
    const char *entry_text;     /* Entry widget text color */
    const char *entry_border;   /* Entry widget border color */
    const char *button_bg;      /* Button background color */
    const char *button_text;    /* Button text color */
    const char *button_hover;   /* Button hover background color */
    const char *title_bar_bg;   /* Custom title bar background color */
    const char *title_bar_border; /* Title bar bottom border color */
    const char *bookmarks_bar_bg; /* Bookmarks bar background color */
} BrowserTheme;

/* Function prototypes */

/**
 * Displays the extensions tab.
 * Currently shows a placeholder message as extension support is under development.
 *
 * @param browser BrowserWindow instance.
 *
 * @sideeffect Adds a new tab to the notebook with extensions information.
 */
void show_extensions_tab(BrowserWindow *browser);

/**
 * Displays the themes tab with all available browser themes.
 * Provides a list of themes with color previews and apply buttons.
 *
 * @param browser BrowserWindow instance.
 *
 * @sideeffect Adds a new tab to the notebook with theme selection UI.
 */
void show_themes_tab(BrowserWindow *browser);

/**
 * Applies a theme to the browser by generating and loading CSS.
 *
 * @param browser BrowserWindow instance.
 * @param theme   BrowserTheme to apply.
 *
 * @sideeffect Creates and applies CSS provider for the theme.
 */
void apply_theme(BrowserWindow *browser, const BrowserTheme *theme);

#endif /* VOIDFOX_EXTENSIONS_H */