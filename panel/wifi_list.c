#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declaration for callback
static void on_network_button_clicked(GtkButton *btn, gpointer data);

static GtkCssProvider *wifi_list_provider = NULL;

// Apply CSS to a specific widget only
static void apply_widget_css(GtkWidget *widget)
{
    if (wifi_list_provider == NULL) {
        wifi_list_provider = gtk_css_provider_new();
        gtk_css_provider_load_from_data(wifi_list_provider,
            "window { background-color: #0b0f14; color: #ffffff; }"
            "button { background-color: #1a1f26; color: #00ff88; border: 1px solid #00ff88; padding: 10px 12px; margin: 2px; font-weight: bold; border-radius: 4px; }"
            "button:hover { background-color: #252d36; color: #00ff00; border: 1px solid #00ff00; }"
            "label { color: #00ff88; font-size: 11px; font-weight: 500; }"
            "scrolledwindow { border: 1px solid #00ff88; border-radius: 4px; }"
            "frame { border: 1px solid #00ff88; border-radius: 4px; }",
            -1, NULL);
    }
    
    gtk_style_context_add_provider(
        gtk_widget_get_style_context(widget),
        GTK_STYLE_PROVIDER(wifi_list_provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

// Refresh WiFi network list
GtkWidget* build_wifi_network_list(void)
{
    GtkWidget *net_list = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(net_list), 10);
    
    // Apply CSS to the list
    apply_widget_css(net_list);
    
    // Get WiFi networks via nmcli with rescan
    FILE *fp = popen("nmcli device wifi list --rescan yes 2>/dev/null | tail -n +2", "r");
    if (!fp) {
        GtkWidget *error = gtk_label_new("Failed to scan networks");
        gtk_box_pack_start(GTK_BOX(net_list), error, FALSE, FALSE, 0);
        return net_list;
    }
    
    char line[512];
    int count = 0;
    
    while (fgets(line, sizeof(line), fp) && count < 15) {
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) < 5) continue;
        
        // Skip header lines
        if (strstr(line, "IN-USE") || strstr(line, "SSID")) continue;
        
        // Parse SSID and signal strength
        char ssid[128] = {0};
        int signal = 0;
        char security[64] = {0};
        
        // Simple parsing - extract SSID and signal
        char *p = line;
        // Skip leading spaces
        while (*p == ' ') p++;
        
        // Check for IN-USE marker
        if (*p == '*') {
            p += 2; // Skip "* " 
        }
        
        // Copy SSID (may contain spaces)
        char ssid_part[128] = {0};
        int i = 0;
        while (*p && *p != ' ' && i < 127) {
            ssid_part[i++] = *p++;
        }
        strcpy(ssid, ssid_part);
        
        // Skip to signal strength
        while (*p && !(*p >= '0' && *p <= '9')) p++;
        if (*p) {
            signal = atoi(p);
        }
        
        if (strlen(ssid) > 0 && strcmp(ssid, "--") != 0) {
            // Determine signal icon
            const char *signal_icon = "🔴";
            if (signal >= 80) signal_icon = "🟢";
            else if (signal >= 60) signal_icon = "🟢";
            else if (signal >= 40) signal_icon = "🟡";
            else if (signal >= 20) signal_icon = "🟠";
            
            // Create button for this network
            gchar *btn_label = g_strdup_printf("%s %s [%d%%]", signal_icon, ssid, signal);
            GtkWidget *net_btn = gtk_button_new_with_label(btn_label);
            g_free(btn_label);
            
            // Store SSID as data for the callback
            gchar *ssid_copy = g_strdup(ssid);
            g_object_set_data_full(G_OBJECT(net_btn), "ssid", ssid_copy, g_free);
            
            gtk_widget_set_margin_top(net_btn, 3);
            gtk_widget_set_margin_bottom(net_btn, 3);
            gtk_box_pack_start(GTK_BOX(net_list), net_btn, FALSE, FALSE, 0);
            
            // Apply CSS to button
            apply_widget_css(net_btn);
            
            count++;
        }
    }
    pclose(fp);
    
    // Add "No networks found" if empty
    if (count == 0) {
        GtkWidget *empty = gtk_label_new("No WiFi networks found\nTry refreshing");
        gtk_label_set_xalign(GTK_LABEL(empty), 0.5);
        gtk_box_pack_start(GTK_BOX(net_list), empty, TRUE, TRUE, 0);
        apply_widget_css(empty);
    }
    
    return net_list;
}

// Callback for network button clicks
static void on_network_button_clicked(GtkButton *btn, gpointer data)
{
    GtkWidget *parent = GTK_WIDGET(data);
    const char *ssid = g_object_get_data(G_OBJECT(btn), "ssid");
    
    if (ssid) {
        // Show password dialog (will handle both secured and open networks)
        extern void show_wifi_connect_dialog(GtkWidget *, const char *);
        show_wifi_connect_dialog(parent, ssid);
    }
}

// Refresh the network list
static void refresh_network_list(GtkWidget *dialog, GtkWidget *scroll, GtkWidget *parent_window)
{
    // Remove old content
    GtkWidget *child = gtk_bin_get_child(GTK_BIN(scroll));
    if (child) {
        gtk_widget_destroy(child);
    }
    
    // Create new list
    GtkWidget *new_list = build_wifi_network_list();
    gtk_container_add(GTK_CONTAINER(scroll), new_list);
    
    // Connect signals to new buttons
    GList *children = gtk_container_get_children(GTK_CONTAINER(new_list));
    for (GList *l = children; l; l = l->next) {
        GtkWidget *btn = GTK_WIDGET(l->data);
        if (GTK_IS_BUTTON(btn)) {
            g_signal_connect(btn, "clicked", G_CALLBACK(on_network_button_clicked), parent_window);
        }
    }
    g_list_free(children);
    
    gtk_widget_show_all(dialog);
}

// Show WiFi selection dialog
void show_wifi_list(GtkWidget *parent_window)
{
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Available WiFi Networks",
        GTK_WINDOW(parent_window),
        GTK_DIALOG_MODAL,
        "Rescan Networks",
        1,
        "Close",
        GTK_RESPONSE_CLOSE,
        NULL
    );
    
    gtk_window_set_default_size(GTK_WINDOW(dialog), 450, 500);
    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
    
    // Apply CSS to dialog
    apply_widget_css(dialog);
    
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 15);
    gtk_container_add(GTK_CONTAINER(content), vbox);
    
    // Title
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<b>📡 Available WiFi Networks</b>\n<small>Click on a network to connect</small>");
    gtk_label_set_xalign(GTK_LABEL(title), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), title, FALSE, FALSE, 0);
    apply_widget_css(title);
    
    // Scrollable window for network list
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scroll), 300);
    gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 0);
    apply_widget_css(scroll);
    
    // Build initial network list
    GtkWidget *net_list = build_wifi_network_list();
    gtk_container_add(GTK_CONTAINER(scroll), net_list);
    
    // Connect signals to network buttons
    GList *children = gtk_container_get_children(GTK_CONTAINER(net_list));
    for (GList *l = children; l; l = l->next) {
        GtkWidget *btn = GTK_WIDGET(l->data);
        if (GTK_IS_BUTTON(btn)) {
            g_signal_connect(btn, "clicked", G_CALLBACK(on_network_button_clicked), parent_window);
        }
    }
    g_list_free(children);
    
    gtk_widget_show_all(dialog);
    
    // Handle dialog response
    gint response;
    while ((response = gtk_dialog_run(GTK_DIALOG(dialog))) == 1) {
        // Refresh button clicked
        refresh_network_list(dialog, scroll, parent_window);
    }
    
    gtk_widget_destroy(dialog);
}

// Clean up provider when program exits
void wifi_list_cleanup(void)
{
    if (wifi_list_provider) {
        g_object_unref(wifi_list_provider);
        wifi_list_provider = NULL;
    }
}