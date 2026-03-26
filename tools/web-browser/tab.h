#ifndef VOIDFOX_TAB_H
#define VOIDFOX_TAB_H

#include "voidfox.h"

/* Tab-specific functionality and utilities */

/**
 * Tab structure.
 * Represents a single browser tab with its WebView and UI elements.
 */
typedef struct {
    WebKitWebView *web_view;   /* WebKit rendering engine instance */
    GtkWidget *tab_label;      /* Label widget in tab bar */
    GtkWidget *close_button;   /* Close button for the tab */
    gboolean is_pinned;        /* TRUE if tab is pinned to the left */
    gboolean is_muted;         /* TRUE if audio is muted */
    char *url;                 /* Current URL of the tab */
    char *title;               /* Current page title */
} BrowserTab;

/* Function prototypes */

/**
 * Creates a new browser tab.
 *
 * @param browser BrowserWindow instance.
 * @param url     URL to load in the new tab (NULL for home page).
 *
 * @sideeffect Creates a new tab with WebView and adds to notebook.
 */
void new_tab(BrowserWindow *browser, const char *url);

/**
 * Closes a browser tab.
 *
 * @param browser   BrowserWindow instance.
 * @param tab_child The tab widget to close.
 *
 * @sideeffect Removes tab from notebook and cleans up resources.
 * @sideeffect Creates a new tab if closing the last tab.
 */
void close_tab(BrowserWindow *browser, GtkWidget *tab_child);

/**
 * Duplicates the current tab.
 *
 * @param browser BrowserWindow instance.
 *
 * @sideeffect Creates a new tab with the same URL and history as current tab.
 */
void duplicate_tab(BrowserWindow *browser);

/**
 * Pins a tab to the left side of the tab bar.
 * Pinned tabs are smaller and persist across sessions.
 *
 * @param browser BrowserWindow instance.
 * @param tab     The tab to pin.
 */
void pin_tab(BrowserWindow *browser, BrowserTab *tab);

/**
 * Unpins a previously pinned tab.
 *
 * @param browser BrowserWindow instance.
 * @param tab     The tab to unpin.
 */
void unpin_tab(BrowserWindow *browser, BrowserTab *tab);

/**
 * Mutes or unmutes audio in the current tab.
 *
 * @param browser BrowserWindow instance.
 * @param mute    TRUE to mute, FALSE to unmute.
 */
void mute_tab(BrowserWindow *browser, gboolean mute);

/**
 * Reloads all tabs.
 *
 * @param browser BrowserWindow instance.
 *
 * @sideeffect Refreshes every open tab.
 */
void reload_all_tabs(BrowserWindow *browser);

/**
 * Closes all tabs except the current one.
 *
 * @param browser BrowserWindow instance.
 */
void close_other_tabs(BrowserWindow *browser);

/**
 * Closes tabs to the right of the current tab.
 *
 * @param browser BrowserWindow instance.
 */
void close_tabs_to_right(BrowserWindow *browser);

/**
 * Restores the last closed tab.
 *
 * @param browser BrowserWindow instance.
 *
 * @sideeffect Reopens the most recently closed tab with its history.
 */
void restore_last_closed_tab(BrowserWindow *browser);

/**
 * Gets the currently active browser tab.
 *
 * @param browser BrowserWindow instance.
 * @return BrowserTab pointer for the active tab, or NULL if none.
 */
BrowserTab* get_current_tab(BrowserWindow *browser);

#endif /* VOIDFOX_TAB_H */