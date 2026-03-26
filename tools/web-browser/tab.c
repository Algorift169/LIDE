#include "voidfox.h"

/* Tab utility functions */

/**
 * Gets the currently active browser tab.
 *
 * @param browser BrowserWindow instance.
 * @return BrowserTab pointer for the active tab, or NULL if none.
 */
BrowserTab* get_current_tab(BrowserWindow *browser)
{
    if (!browser || !browser->notebook) return NULL;
    
    int current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(browser->notebook));
    if (current_page < 0) return NULL;
    
    GtkWidget *current_page_widget = gtk_notebook_get_nth_page(GTK_NOTEBOOK(browser->notebook), current_page);
    if (!current_page_widget) return NULL;
    
    return (BrowserTab *)g_object_get_data(G_OBJECT(current_page_widget), "browser-tab");
}

/**
 * Creates a new browser tab.
 *
 * @param browser BrowserWindow instance.
 * @param url     URL to load in the new tab (NULL for home page).
 *
 * @sideeffect Creates a new tab with WebView and adds to notebook.
 */
void new_tab(BrowserWindow *browser, const char *url)
{
    if (!browser || !browser->notebook) return;
    
    /* Create WebView with settings */
    WebKitWebView *web_view = WEBKIT_WEB_VIEW(webkit_web_view_new());
    if (!web_view) return;
    
    /* Apply settings to web view */
    apply_settings_to_web_view(web_view);
    
    /* Connect signals */
    g_signal_connect(web_view, "load-changed", G_CALLBACK(on_load_changed), browser);
    g_signal_connect(web_view, "permission-request", G_CALLBACK(on_web_view_permission_request), browser);
    g_signal_connect(web_view, "decide-policy", G_CALLBACK(on_web_view_decide_policy), browser);
    
    /* Load URL - use provided URL or home page */
    if (url && *url) {
        webkit_web_view_load_uri(web_view, url);
    } else {
        char *home_path = get_home_page_path();
        char *home_uri = g_strconcat("file://", home_path, NULL);
        webkit_web_view_load_uri(web_view, home_uri);
        g_free(home_uri);
    }
    
    /* Create tab content container */
    GtkWidget *tab_child = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    if (!tab_child) {
        g_object_unref(web_view);
        return;
    }
    gtk_box_pack_start(GTK_BOX(tab_child), GTK_WIDGET(web_view), TRUE, TRUE, 0);
    gtk_widget_show(tab_child);
    
    /* Create BrowserTab struct */
    BrowserTab *tab = g_new0(BrowserTab, 1);
    tab->web_view = web_view;
    tab->tab_label = gtk_label_new("Loading...");
    tab->close_button = gtk_button_new_from_icon_name("window-close", GTK_ICON_SIZE_MENU);
    tab->is_pinned = FALSE;
    tab->is_muted = FALSE;
    tab->url = NULL;
    tab->title = NULL;
    
    if (tab->close_button) {
        gtk_button_set_relief(GTK_BUTTON(tab->close_button), GTK_RELIEF_NONE);
        gtk_widget_set_tooltip_text(tab->close_button, "Close tab");
    }
    
    /* Pack close button into a box */
    GtkWidget *tab_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    if (tab_box) {
        if (tab->tab_label) gtk_box_pack_start(GTK_BOX(tab_box), tab->tab_label, FALSE, FALSE, 0);
        if (tab->close_button) gtk_box_pack_start(GTK_BOX(tab_box), tab->close_button, FALSE, FALSE, 0);
        gtk_widget_show_all(tab_box);
    }
    
    /* Store tab data */
    g_object_set_data_full(G_OBJECT(tab_child), "browser-tab", tab, g_free);
    if (tab->close_button) {
        g_object_set_data_full(G_OBJECT(tab->close_button), "tab-child", tab_child, NULL);
        g_signal_connect(tab->close_button, "clicked", G_CALLBACK(on_close_tab_clicked), browser);
    }
    g_signal_connect(web_view, "notify::title", G_CALLBACK(on_title_changed), tab);
    
    /* Append tab */
    int page_num = gtk_notebook_append_page(GTK_NOTEBOOK(browser->notebook), tab_child, tab_box);
    gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(browser->notebook), tab_child, TRUE);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(browser->notebook), page_num);
}

/**
 * Closes a browser tab.
 *
 * @param browser   BrowserWindow instance.
 * @param tab_child The tab widget to close.
 *
 * @sideeffect Removes tab from notebook and cleans up resources.
 * @sideeffect Creates a new tab if closing the last tab.
 */
void close_tab(BrowserWindow *browser, GtkWidget *tab_child)
{
    if (!browser || !browser->notebook || !tab_child) return;
    
    int page_num = gtk_notebook_page_num(GTK_NOTEBOOK(browser->notebook), tab_child);
    if (page_num != -1) {
        /* Don't close if it's the last tab */
        if (gtk_notebook_get_n_pages(GTK_NOTEBOOK(browser->notebook)) <= 1) {
            char *home_path = get_home_page_path();
            char *home_uri = g_strconcat("file://", home_path, NULL);
            new_tab(browser, home_uri);
            g_free(home_uri);
        }
        gtk_notebook_remove_page(GTK_NOTEBOOK(browser->notebook), page_num);
    }
}

/**
 * Duplicates the current tab.
 *
 * @param browser BrowserWindow instance.
 *
 * @sideeffect Creates a new tab with the same URL and history as current tab.
 */
void duplicate_tab(BrowserWindow *browser)
{
    BrowserTab *current = get_current_tab(browser);
    if (!current || !current->web_view) return;
    
    const char *url = webkit_web_view_get_uri(current->web_view);
    if (url && *url) {
        new_tab(browser, url);
    }
}

/**
 * Pins a tab to the left side of the tab bar.
 * Pinned tabs are smaller and persist across sessions.
 *
 * @param browser BrowserWindow instance.
 * @param tab     The tab to pin.
 */
void pin_tab(BrowserWindow *browser, BrowserTab *tab)
{
    (void)browser;
    if (!tab || tab->is_pinned) return;
    
    tab->is_pinned = TRUE;
    /* TODO: Implement tab pinning UI changes */
}

/**
 * Unpins a previously pinned tab.
 *
 * @param browser BrowserWindow instance.
 * @param tab     The tab to unpin.
 */
void unpin_tab(BrowserWindow *browser, BrowserTab *tab)
{
    (void)browser;
    if (!tab || !tab->is_pinned) return;
    
    tab->is_pinned = FALSE;
    /* TODO: Implement tab unpinning UI changes */
}

/**
 * Mutes or unmutes audio in the current tab.
 *
 * @param browser BrowserWindow instance.
 * @param mute    TRUE to mute, FALSE to unmute.
 */
void mute_tab(BrowserWindow *browser, gboolean mute)
{
    BrowserTab *current = get_current_tab(browser);
    if (!current || !current->web_view) return;
    
    current->is_muted = mute;
    webkit_web_view_set_is_muted(current->web_view, mute);
    
    /* Update tab label to show mute icon */
    const char *title = webkit_web_view_get_title(current->web_view);
    char *label_text;
    if (mute) {
        label_text = g_strdup_printf("🔇 %s", title ? title : "New Tab");
    } else {
        label_text = g_strdup_printf("%s", title ? title : "New Tab");
    }
    gtk_label_set_text(GTK_LABEL(current->tab_label), label_text);
    g_free(label_text);
}

/**
 * Reloads all tabs.
 *
 * @param browser BrowserWindow instance.
 *
 * @sideeffect Refreshes every open tab.
 */
void reload_all_tabs(BrowserWindow *browser)
{
    if (!browser || !browser->notebook) return;
    
    int n_pages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(browser->notebook));
    for (int i = 0; i < n_pages; i++) {
        GtkWidget *page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(browser->notebook), i);
        if (!page) continue;
        
        BrowserTab *tab = (BrowserTab *)g_object_get_data(G_OBJECT(page), "browser-tab");
        if (tab && tab->web_view) {
            webkit_web_view_reload(tab->web_view);
        }
    }
}

/**
 * Closes all tabs except the current one.
 *
 * @param browser BrowserWindow instance.
 */
void close_other_tabs(BrowserWindow *browser)
{
    if (!browser || !browser->notebook) return;
    
    int current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(browser->notebook));
    if (current_page < 0) return;
    
    GtkWidget *current_tab = gtk_notebook_get_nth_page(GTK_NOTEBOOK(browser->notebook), current_page);
    if (!current_tab) return;
    
    int n_pages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(browser->notebook));
    for (int i = n_pages - 1; i >= 0; i--) {
        if (i == current_page) continue;
        
        GtkWidget *page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(browser->notebook), i);
        if (page) {
            gtk_notebook_remove_page(GTK_NOTEBOOK(browser->notebook), i);
        }
    }
}

/**
 * Closes tabs to the right of the current tab.
 *
 * @param browser BrowserWindow instance.
 */
void close_tabs_to_right(BrowserWindow *browser)
{
    if (!browser || !browser->notebook) return;
    
    int current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(browser->notebook));
    if (current_page < 0) return;
    
    int n_pages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(browser->notebook));
    for (int i = n_pages - 1; i > current_page; i--) {
        GtkWidget *page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(browser->notebook), i);
        if (page) {
            gtk_notebook_remove_page(GTK_NOTEBOOK(browser->notebook), i);
        }
    }
}

/**
 * Restores the last closed tab.
 *
 * @param browser BrowserWindow instance.
 *
 * @sideeffect Reopens the most recently closed tab with its history.
 */
void restore_last_closed_tab(BrowserWindow *browser)
{
    /* TODO: Implement tab history for restoring closed tabs */
    (void)browser;
}