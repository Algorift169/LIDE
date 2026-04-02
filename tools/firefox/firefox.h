#ifndef LIDE_FIREFOX_H
#define LIDE_FIREFOX_H

#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

/**

/*
 * firefox.h
 * 
 * Firefox integration interface definitions
 *
 * This module is part of the LIDE desktop environment system.
 * See the main window manager (wm/) and session management (session/)
 * for system architecture overview.
 */

 * Firefox browser window structure.
 * Encapsulates all UI components and state for a browser tab/window.
 */
typedef struct {
    GtkWidget *window;           /* Main application window */
    GtkWidget *notebook;         /* Tab container for multiple pages */
    GtkWidget *url_entry;        /* Entry widget for URL/location bar */
    GtkWidget *back_button;      /* Button to navigate backward in history */
    GtkWidget *forward_button;   /* Button to navigate forward in history */
    GtkWidget *reload_button;    /* Button to reload current page */
    GtkWidget *home_button;      /* Button to navigate to home page */
    WebKitWebView *web_view;     /* WebKit view for rendering web content */
} FirefoxWindow;

/**
 * Application activation callback.
 * Creates and displays the main browser window.
 *
 * @param app        The GtkApplication instance.
 * @param user_data  User data passed during signal connection.
 *
 * @sideeffect Creates browser UI and initializes WebKit view.
 */
void firefox_activate(GtkApplication *app, gpointer user_data);

/**
 * Loads a URL in the current browser tab.
 *
 * @param firefox FirefoxWindow instance.
 * @param url     URL to load (e.g., "https://example.com").
 *
 * @sideeffect Navigates the web view to the specified URL.
 * @sideeffect Updates URL entry text to reflect loaded URL.
 */
void load_url(FirefoxWindow *firefox, const char *url);

/**
 * Navigates backward in browser history.
 *
 * @param firefox FirefoxWindow instance.
 *
 * @sideeffect Loads previous page in history if available.
 * @sideeffect Updates navigation button states.
 */
void go_back(FirefoxWindow *firefox);

/**
 * Navigates forward in browser history.
 *
 * @param firefox FirefoxWindow instance.
 *
 * @sideeffect Loads next page in history if available.
 * @sideeffect Updates navigation button states.
 */
void go_forward(FirefoxWindow *firefox);

/**
 * Reloads the current page.
 *
 * @param firefox FirefoxWindow instance.
 *
 * @sideeffect Refreshes the current web view content.
 */
void reload_page(FirefoxWindow *firefox);

/**
 * Updates the state of navigation buttons based on WebKit history.
 *
 * @param firefox FirefoxWindow instance.
 *
 * @sideeffect Enables/disables back/forward buttons based on history availability.
 */
void update_navigation_buttons(FirefoxWindow *firefox);

#endif /* LIDE_FIREFOX_H */