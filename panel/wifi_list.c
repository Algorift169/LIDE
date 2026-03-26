#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <glib.h>

/* Forward declaration for callback */
static void on_network_button_clicked(GtkButton *btn, gpointer data);
static void on_wifi_toggle_clicked(GtkButton *btn, gpointer data);
static void refresh_network_list(GtkWidget *dialog, GtkWidget *scroll, GtkWidget *parent_window);

static GtkCssProvider *wifi_list_provider = NULL;
static GtkWidget *current_dialog = NULL;
static GtkWidget *current_scroll = NULL;
static GtkWidget *current_parent = NULL;

/* Structure to hold WiFi network information */
typedef struct {
    char ssid[256];
    int signal_strength;
    char security[128];
    gboolean is_connected;
} WiFiNetwork;

/**
 * Comparison function for sorting networks by signal strength.
 * Used with g_array_sort for descending order.
 *
 * @param a First WiFiNetwork to compare.
 * @param b Second WiFiNetwork to compare.
 * @return Negative if b has higher strength, positive if a has higher strength.
 */
static int compare_networks(const void *a, const void *b)
{
    const WiFiNetwork *na = (const WiFiNetwork*)a;
    const WiFiNetwork *nb = (const WiFiNetwork*)b;
    return nb->signal_strength - na->signal_strength; /* Descending order */
}

/**
 * Applies custom CSS styling to a widget.
 * Loads CSS provider lazily on first call.
 *
 * @param widget The widget to apply CSS to.
 *
 * @sideeffect Creates and populates global CSS provider if not already created.
 */
static void apply_widget_css(GtkWidget *widget)
{
    if (wifi_list_provider == NULL) {
        wifi_list_provider = gtk_css_provider_new();
        gtk_css_provider_load_from_data(wifi_list_provider,
            "window { background-color: #0b0f14; color: #ffffff; }"
            "button { background-color: #1a1f26; color: #a92d5a; border: 1px solid #a92d5a; padding: 12px 15px; margin: 2px; font-weight: bold; border-radius: 4px; background-image: none; }"
            "button:hover { background-color: #252d36; color: #a92d5a; border: 1px solid #a92d5a; }"
            "button:active { background-color: #a92d5a; color: #ffffff; }"
            "button.toggle-on { background-color: #a92d5a; color: #ffffff; }"
            "label { color: #a92d5a; font-size: 11px; font-weight: 500; }"
            "scrolledwindow { border: 1px solid #a92d5a; border-radius: 4px; background-color: #0b0f14; }"
            "frame { border: 1px solid #a92d5a; border-radius: 4px; }"
            "frame > label { color: #a92d5a; }",
            -1, NULL);
    }
    
    gtk_style_context_add_provider(
        gtk_widget_get_style_context(widget),
        GTK_STYLE_PROVIDER(wifi_list_provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

/**
 * Checks whether WiFi radio is enabled via nmcli.
 *
 * @return TRUE if WiFi is enabled, FALSE otherwise.
 */
static gboolean is_wifi_enabled(void)
{
    FILE *fp = popen("nmcli radio wifi 2>/dev/null | tr -d '\\n'", "r");
    if (!fp) return FALSE;
    
    char status[16] = {0};
    if (fgets(status, sizeof(status), fp)) {
        pclose(fp);
        return (strcmp(status, "enabled") == 0);
    }
    pclose(fp);
    return FALSE;
}

/**
 * Enables or disables WiFi radio via nmcli.
 *
 * @param enabled TRUE to enable WiFi, FALSE to disable.
 *
 * @sideeffect Executes nmcli commands to control WiFi radio.
 */
static void set_wifi_enabled(gboolean enabled)
{
    if (enabled) {
        system("nmcli radio wifi on 2>/dev/null");
        sleep(1); /* Give hardware time to initialize */
        system("nmcli device wifi rescan 2>/dev/null");
    } else {
        system("nmcli radio wifi off 2>/dev/null");
    }
}

/**
 * Retrieves the SSID of the currently connected WiFi network.
 *
 * @return Newly allocated string containing SSID, or NULL if not connected.
 *         Caller must free with g_free().
 *
 * @sideeffect Executes nmcli to query active connections.
 */
static char* get_connected_ssid(void)
{
    FILE *fp = popen("nmcli -t -f NAME,TYPE connection show --active 2>/dev/null | grep :wifi | cut -d: -f1 | head -1", "r");
    if (!fp) return NULL;
    
    char ssid[256] = {0};
    if (fgets(ssid, sizeof(ssid), fp)) {
        ssid[strcspn(ssid, "\n")] = 0;
        pclose(fp);
        return (strlen(ssid) > 0) ? g_strdup(ssid) : NULL;
    }
    pclose(fp);
    return NULL;
}

/**
 * Builds the WiFi network list UI.
 * Scans for available networks and creates clickable buttons.
 *
 * @return GtkWidget containing the complete network list container.
 *
 * @sideeffect Executes nmcli to scan for WiFi networks.
 */
GtkWidget* build_wifi_network_list(void)
{
    GtkWidget *net_list = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_container_set_border_width(GTK_CONTAINER(net_list), 10);
    
    /* Apply CSS to the list */
    apply_widget_css(net_list);
    
    /* Check if WiFi is enabled */
    if (!is_wifi_enabled()) {
        GtkWidget *wifi_off = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(wifi_off), 
            "<span size='large'>📡 WiFi Disabled</span>\n\n"
            "<span size='small'>Click the 'Turn On WiFi' button above to enable</span>");
        gtk_label_set_justify(GTK_LABEL(wifi_off), GTK_JUSTIFY_CENTER);
        gtk_box_pack_start(GTK_BOX(net_list), wifi_off, TRUE, TRUE, 20);
        apply_widget_css(wifi_off);
        return net_list;
    }
    
    /* Get currently connected network */
    char *connected_ssid = get_connected_ssid();
    
    /* Show currently connected network first if any */
    if (connected_ssid) {
        GtkWidget *connected_frame = gtk_frame_new("Currently Connected");
        gtk_box_pack_start(GTK_BOX(net_list), connected_frame, FALSE, FALSE, 5);
        apply_widget_css(connected_frame);
        
        GtkWidget *connected_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        gtk_container_set_border_width(GTK_CONTAINER(connected_box), 10);
        gtk_container_add(GTK_CONTAINER(connected_frame), connected_box);
        
        GtkWidget *check = gtk_label_new("✓");
        gtk_widget_set_name(check, "connected-check");
        gtk_box_pack_start(GTK_BOX(connected_box), check, FALSE, FALSE, 0);
        
        GtkWidget *ssid_label = gtk_label_new(connected_ssid);
        gtk_label_set_xalign(GTK_LABEL(ssid_label), 0);
        gtk_box_pack_start(GTK_BOX(connected_box), ssid_label, TRUE, TRUE, 0);
        
        GtkWidget *signal_label = gtk_label_new("🟢 Connected");
        gtk_box_pack_start(GTK_BOX(connected_box), signal_label, FALSE, FALSE, 0);
        
        g_free(connected_ssid);
    }
    
    /* Separator */
    if (connected_ssid) {
        GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
        gtk_box_pack_start(GTK_BOX(net_list), sep, FALSE, FALSE, 5);
    }
    
    /* Get WiFi networks via nmcli - using CSV format for reliability */
    FILE *fp = popen("nmcli -t -f SSID,SIGNAL,SECURITY device wifi list --rescan yes 2>/dev/null", "r");
    if (!fp) {
        GtkWidget *error = gtk_label_new("Failed to scan networks");
        gtk_box_pack_start(GTK_BOX(net_list), error, FALSE, FALSE, 0);
        return net_list;
    }
    
    char line[1024];
    int count = 0;
    GArray *networks = g_array_new(FALSE, TRUE, sizeof(WiFiNetwork));
    
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) < 2) continue;
        
        WiFiNetwork net;
        memset(&net, 0, sizeof(WiFiNetwork));
        
        /* Parse colon-separated fields */
        char *ssid = strtok(line, ":");
        char *signal_str = strtok(NULL, ":");
        char *security = strtok(NULL, ":");
        
        /* Skip header and invalid entries */
        if (!ssid || strcmp(ssid, "SSID") == 0 || strlen(ssid) == 0 || strcmp(ssid, "--") == 0) {
            continue;
        }
        
        strncpy(net.ssid, ssid, sizeof(net.ssid) - 1);
        net.signal_strength = signal_str ? atoi(signal_str) : 0;
        strncpy(net.security, security ? security : "--", sizeof(net.security) - 1);
        
        g_array_append_val(networks, net);
        count++;
    }
    pclose(fp);
    
    /* Sort networks by signal strength (strongest first) */
    if (count > 0) {
        g_array_sort(networks, compare_networks);
        
        /* Add each network as a button */
        for (int i = 0; i < networks->len; i++) {
            WiFiNetwork *net = &g_array_index(networks, WiFiNetwork, i);
            
            /* Determine signal icon based on strength */
            const char *signal_icon;
            if (net->signal_strength >= 80) signal_icon = "🟢"; /* Excellent */
            else if (net->signal_strength >= 60) signal_icon = "🟢"; /* Good */
            else if (net->signal_strength >= 40) signal_icon = "🟡"; /* Fair */
            else if (net->signal_strength >= 20) signal_icon = "🟠"; /* Weak */
            else signal_icon = "🔴"; /* Very weak */
            
            /* Security icon */
            const char *lock_icon = "";
            if (strlen(net->security) > 0 && strcmp(net->security, "--") != 0) {
                lock_icon = "🔒 ";
            }
            
            /* Create button with proper formatting */
            gchar *btn_label;
            if (net->signal_strength > 0) {
                btn_label = g_strdup_printf("%s %s%s   %d%%", 
                                           signal_icon, lock_icon, net->ssid, net->signal_strength);
            } else {
                btn_label = g_strdup_printf("%s %s%s", signal_icon, lock_icon, net->ssid);
            }
            
            GtkWidget *net_btn = gtk_button_new_with_label(btn_label);
            g_free(btn_label);
            
            /* Make the button text left-aligned */
            GtkWidget *btn_child = gtk_bin_get_child(GTK_BIN(net_btn));
            if (btn_child && GTK_IS_LABEL(btn_child)) {
                gtk_label_set_xalign(GTK_LABEL(btn_child), 0);
            }
            
            /* Store network info */
            g_object_set_data_full(G_OBJECT(net_btn), "ssid", g_strdup(net->ssid), g_free);
            g_object_set_data_full(G_OBJECT(net_btn), "security", g_strdup(net->security), g_free);
            g_object_set_data(G_OBJECT(net_btn), "signal", GINT_TO_POINTER(net->signal_strength));
            
            gtk_widget_set_margin_top(net_btn, 2);
            gtk_widget_set_margin_bottom(net_btn, 2);
            gtk_box_pack_start(GTK_BOX(net_list), net_btn, FALSE, FALSE, 0);
            
            /* Apply CSS */
            apply_widget_css(net_btn);
        }
    }
    
    g_array_free(networks, TRUE);
    
    /* Add "No networks found" message if empty */
    if (count == 0) {
        GtkWidget *empty = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(empty),
            "<span size='large'>📡 No Networks Found</span>\n\n"
            "<span size='small'>Try clicking 'Rescan Networks' above</span>");
        gtk_label_set_justify(GTK_LABEL(empty), GTK_JUSTIFY_CENTER);
        gtk_box_pack_start(GTK_BOX(net_list), empty, TRUE, TRUE, 20);
        apply_widget_css(empty);
    }
    
    return net_list;
}

/**
 * Callback for network button clicks.
 * Initiates connection to the selected WiFi network.
 *
 * @param btn  The button that was clicked.
 * @param data Parent window for dialogs.
 *
 * @sideeffect Executes nmcli commands to connect to the network.
 * @sideeffect Shows password dialog for secured networks.
 */
static void on_network_button_clicked(GtkButton *btn, gpointer data)
{
    GtkWidget *parent = GTK_WIDGET(data);
    const char *ssid = g_object_get_data(G_OBJECT(btn), "ssid");
    const char *security = g_object_get_data(G_OBJECT(btn), "security");
    
    if (!ssid) return;
    
    /* Check if already connected to this network */
    char *connected = get_connected_ssid();
    if (connected && strcmp(connected, ssid) == 0) {
        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(parent),
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_INFO,
            GTK_BUTTONS_OK,
            "Already connected to %s", ssid
        );
        apply_widget_css(dialog);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        g_free(connected);
        return;
    }
    if (connected) g_free(connected);
    
    /* Check if network is secured */
    gboolean secured = (security && strlen(security) > 0 && strcmp(security, "--") != 0);
    
    if (secured) {
        /* Show password dialog for secured networks */
        extern void show_wifi_connect_dialog(GtkWidget *, const char *);
        show_wifi_connect_dialog(parent, ssid);
    } else {
        /* Connect directly to open networks */
        gchar *cmd = g_strdup_printf("nmcli device wifi connect '%s' 2>/dev/null", ssid);
        int result = system(cmd);
        g_free(cmd);
        
        /* Show result */
        GtkWidget *status_dialog = gtk_message_dialog_new(
            GTK_WINDOW(parent),
            GTK_DIALOG_MODAL,
            result == 0 ? GTK_MESSAGE_INFO : GTK_MESSAGE_ERROR,
            GTK_BUTTONS_OK,
            result == 0 ? "Connection Successful" : "Connection Failed"
        );
        
        if (result == 0) {
            gtk_message_dialog_format_secondary_text(
                GTK_MESSAGE_DIALOG(status_dialog),
                "Successfully connected to %s", ssid
            );
        } else {
            gtk_message_dialog_format_secondary_text(
                GTK_MESSAGE_DIALOG(status_dialog),
                "Failed to connect to %s.\nPlease try again.", ssid
            );
        }
        
        apply_widget_css(status_dialog);
        gtk_dialog_run(GTK_DIALOG(status_dialog));
        gtk_widget_destroy(status_dialog);
    }
    
    /* Refresh the network list to show updated connection status */
    if (current_dialog && current_scroll) {
        refresh_network_list(current_dialog, current_scroll, parent);
    }
}

/**
 * Callback for WiFi toggle button click.
 * Enables or disables WiFi radio and updates button text.
 *
 * @param btn  The toggle button that was clicked.
 * @param data User data (unused).
 *
 * @sideeffect Executes nmcli commands to control WiFi.
 */
static void on_wifi_toggle_clicked(GtkButton *btn, gpointer data)
{
    (void)data;
    
    /* Get current state */
    FILE *fp = popen("nmcli radio wifi 2>/dev/null | tr -d '\\n'", "r");
    gboolean currently_enabled = TRUE;
    if (fp) {
        char status[16] = {0};
        if (fgets(status, sizeof(status), fp)) {
            currently_enabled = (strcmp(status, "enabled") == 0);
        }
        pclose(fp);
    }
    
    /* Toggle WiFi */
    if (currently_enabled) {
        system("nmcli radio wifi off 2>/dev/null");
        gtk_button_set_label(btn, "🟢 Turn On WiFi");
    } else {
        system("nmcli radio wifi on 2>/dev/null");
        sleep(1);
        system("nmcli device wifi rescan 2>/dev/null");
        gtk_button_set_label(btn, "🔴 Turn Off WiFi");
    }
    
    /* Refresh the network list if dialog exists */
    if (current_dialog != NULL && current_scroll != NULL) {
        refresh_network_list(current_dialog, current_scroll, current_parent);
    }
}

/**
 * Refreshes the WiFi network list display.
 * Performs a new scan and rebuilds the network list.
 *
 * @param dialog        The parent dialog containing the scroll container.
 * @param scroll        The GtkScrolledWindow containing the network list.
 * @param parent_window Parent window for connection dialogs.
 *
 * @sideeffect Executes nmcli to rescan networks.
 * @sideeffect Destroys and recreates the network list.
 */
static void refresh_network_list(GtkWidget *dialog, GtkWidget *scroll, GtkWidget *parent_window)
{
    /* Show a spinner or "Refreshing..." message */
    GtkWidget *old_child = gtk_bin_get_child(GTK_BIN(scroll));
    if (old_child) {
        gtk_widget_destroy(old_child);
    }
    
    GtkWidget *loading = gtk_label_new("🔄 Scanning for networks...");
    gtk_label_set_xalign(GTK_LABEL(loading), 0.5);
    gtk_container_add(GTK_CONTAINER(scroll), loading);
    gtk_widget_show_all(dialog);
    
    /* Process events to show the loading message */
    while (gtk_events_pending()) gtk_main_iteration();
    
    /* Small delay to make the rescan visible */
    usleep(500000);
    
    /* Remove loading message */
    gtk_widget_destroy(loading);
    
    /* Force a new scan */
    system("nmcli device wifi rescan 2>/dev/null");
    usleep(1000000); /* Wait for scan to complete */
    
    /* Create new list */
    GtkWidget *new_list = build_wifi_network_list();
    gtk_container_add(GTK_CONTAINER(scroll), new_list);
    
    /* Connect signals to new buttons */
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

/**
 * Displays a modal dialog showing available WiFi networks.
 * Provides controls for enabling/disabling WiFi and connecting to networks.
 *
 * @param parent_window Parent window for the dialog.
 *
 * @sideeffect Creates and displays modal dialog. Blocks until dialog is closed.
 * @sideeffect Executes nmcli commands for WiFi scanning and connections.
 */
void show_wifi_list(GtkWidget *parent_window)
{
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Wi-Fi Networks",
        GTK_WINDOW(parent_window),
        GTK_DIALOG_MODAL,
        "_Rescan",
        1,
        "_Close",
        GTK_RESPONSE_CLOSE,
        NULL
    );
    
    current_dialog = dialog;
    current_parent = parent_window;
    
    gtk_window_set_default_size(GTK_WINDOW(dialog), 450, 600);
    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
    gtk_window_set_resizable(GTK_WINDOW(dialog), TRUE);
    
    /* Apply CSS to dialog */
    apply_widget_css(dialog);
    
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 15);
    gtk_container_add(GTK_CONTAINER(content), vbox);
    
    /* Title with icon */
    GtkWidget *header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(vbox), header, FALSE, FALSE, 0);
    
    GtkWidget *wifi_icon = gtk_label_new("📡");
    gtk_widget_set_name(wifi_icon, "wifi-icon");
    gtk_box_pack_start(GTK_BOX(header), wifi_icon, FALSE, FALSE, 0);
    
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<b><big>Wi-Fi Networks</big></b>");
    gtk_label_set_xalign(GTK_LABEL(title), 0);
    gtk_box_pack_start(GTK_BOX(header), title, TRUE, TRUE, 0);
    
    /* WiFi toggle button */
    GtkWidget *wifi_toggle = gtk_button_new_with_label(is_wifi_enabled() ? "🔴 Turn Off" : "🟢 Turn On");
    gtk_widget_set_tooltip_text(wifi_toggle, "Toggle Wi-Fi radio");
    g_signal_connect(wifi_toggle, "clicked", G_CALLBACK(on_wifi_toggle_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(header), wifi_toggle, FALSE, FALSE, 0);
    apply_widget_css(wifi_toggle);
    
    /* Separator */
    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), sep, FALSE, FALSE, 5);
    
    /* Scrollable window for network list */
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scroll), 350);
    gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 0);
    apply_widget_css(scroll);
    
    current_scroll = scroll;
    
    /* Build initial network list */
    GtkWidget *net_list = build_wifi_network_list();
    gtk_container_add(GTK_CONTAINER(scroll), net_list);
    
    /* Connect signals to network buttons */
    GList *children = gtk_container_get_children(GTK_CONTAINER(net_list));
    for (GList *l = children; l; l = l->next) {
        GtkWidget *btn = GTK_WIDGET(l->data);
        if (GTK_IS_BUTTON(btn)) {
            g_signal_connect(btn, "clicked", G_CALLBACK(on_network_button_clicked), parent_window);
        }
    }
    g_list_free(children);
    
    /* Network info label at bottom */
    GtkWidget *info_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(info_label), 
        "<span size='small' foreground='#666666'>Click on a network to connect</span>");
    gtk_box_pack_start(GTK_BOX(vbox), info_label, FALSE, FALSE, 0);
    
    gtk_widget_show_all(dialog);
    
    /* Handle dialog response */
    gint response;
    while ((response = gtk_dialog_run(GTK_DIALOG(dialog))) == 1) {
        /* Rescan button clicked */
        refresh_network_list(dialog, scroll, parent_window);
    }
    
    current_dialog = NULL;
    current_scroll = NULL;
    current_parent = NULL;
    gtk_widget_destroy(dialog);
}

/**
 * Cleans up resources used by the WiFi list module.
 * Unrefs the global CSS provider if it exists.
 */
void wifi_list_cleanup(void)
{
    if (wifi_list_provider) {
        g_object_unref(wifi_list_provider);
        wifi_list_provider = NULL;
    }
}