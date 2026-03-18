#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static GtkCssProvider *internet_settings_provider = NULL;

// Apply CSS to a specific widget only
static void apply_widget_css(GtkWidget *widget)
{
    if (internet_settings_provider == NULL) {
        internet_settings_provider = gtk_css_provider_new();
        gtk_css_provider_load_from_data(internet_settings_provider,
            "window { background-color: #0b0f14; color: #ffffff; }"
            "button { background-color: #1a1f26; color: #00ff88; border: 1px solid #00ff88; padding: 8px 15px; margin: 2px; font-weight: bold; border-radius: 4px; }"
            "button:hover { background-color: #252d36; color: #00ff00; border: 1px solid #00ff00; }"
            "label { color: #00ff88; font-size: 11px; font-weight: 500; }"
            "entry { background-color: #1a1f26; color: #00ff88; border: 1px solid #00ff88; padding: 6px; border-radius: 4px; }"
            "combobox { background-color: #1a1f26; color: #00ff88; border: 1px solid #00ff88; }"
            "frame { border: 1px solid #00ff88; border-radius: 4px; }",
            -1, NULL);
    }
    
    gtk_style_context_add_provider(
        gtk_widget_get_style_context(widget),
        GTK_STYLE_PROVIDER(internet_settings_provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

// Get current network interfaces
static char** get_network_interfaces(int *count)
{
    FILE *fp = popen("ls /sys/class/net/ 2>/dev/null", "r");
    if (!fp) return NULL;
    
    char **interfaces = NULL;
    char line[256];
    int num = 0;
    
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = 0;
        if (strcmp(line, "lo") == 0) continue; // Skip loopback
        
        interfaces = realloc(interfaces, (num + 1) * sizeof(char*));
        interfaces[num] = strdup(line);
        num++;
    }
    pclose(fp);
    
    *count = num;
    return interfaces;
}

// Get current connection method
static const char* get_connection_method(const char *iface)
{
    static char method[32] = "dhcp";
    char cmd[256];
    char result[256] = {0};
    
    snprintf(cmd, sizeof(cmd), "nmcli -t -f ipv4.method connection show '%s' 2>/dev/null | head -1", iface);
    FILE *fp = popen(cmd, "r");
    if (fp) {
        if (fgets(result, sizeof(result), fp)) {
            result[strcspn(result, "\n")] = 0;
            strcpy(method, result);
        }
        pclose(fp);
    }
    return method;
}

// Show internet settings dialog
void show_internet_settings(GtkWidget *parent_window)
{
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Internet Settings",
        GTK_WINDOW(parent_window),
        GTK_DIALOG_MODAL,
        "Apply",
        GTK_RESPONSE_APPLY,
        "Close",
        GTK_RESPONSE_CLOSE,
        NULL
    );
    
    gtk_window_set_default_size(GTK_WINDOW(dialog), 500, 400);
    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
    
    // Apply CSS to dialog
    apply_widget_css(dialog);
    
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 15);
    gtk_container_add(GTK_CONTAINER(content), vbox);
    
    // Title
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<b>🔧 Internet Connection Settings</b>");
    gtk_label_set_xalign(GTK_LABEL(title), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), title, FALSE, FALSE, 0);
    apply_widget_css(title);
    
    // Interface selection
    GtkWidget *iface_frame = gtk_frame_new("Network Interface");
    gtk_box_pack_start(GTK_BOX(vbox), iface_frame, FALSE, FALSE, 0);
    apply_widget_css(iface_frame);
    
    GtkWidget *iface_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(iface_box), 10);
    gtk_container_add(GTK_CONTAINER(iface_frame), iface_box);
    
    int iface_count = 0;
    char **interfaces = get_network_interfaces(&iface_count);
    
    GtkWidget *iface_combo = gtk_combo_box_text_new();
    for (int i = 0; i < iface_count; i++) {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(iface_combo), interfaces[i]);
        free(interfaces[i]);
    }
    free(interfaces);
    
    if (iface_count > 0) {
        gtk_combo_box_set_active(GTK_COMBO_BOX(iface_combo), 0);
    }
    gtk_box_pack_start(GTK_BOX(iface_box), iface_combo, FALSE, FALSE, 0);
    apply_widget_css(iface_combo);
    
    // Connection method frame
    GtkWidget *method_frame = gtk_frame_new("Connection Method");
    gtk_box_pack_start(GTK_BOX(vbox), method_frame, FALSE, FALSE, 0);
    apply_widget_css(method_frame);
    
    GtkWidget *method_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(method_box), 10);
    gtk_container_add(GTK_CONTAINER(method_frame), method_box);
    
    GSList *method_group = NULL;
    GtkWidget *dhcp_radio = gtk_radio_button_new_with_label(method_group, "Automatic (DHCP)");
    method_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(dhcp_radio));
    gtk_box_pack_start(GTK_BOX(method_box), dhcp_radio, FALSE, FALSE, 0);
    apply_widget_css(dhcp_radio);
    
    GtkWidget *static_radio = gtk_radio_button_new_with_label(method_group, "Manual (Static IP)");
    method_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(static_radio));
    gtk_box_pack_start(GTK_BOX(method_box), static_radio, FALSE, FALSE, 0);
    apply_widget_css(static_radio);
    
    // Static IP settings frame
    GtkWidget *static_frame = gtk_frame_new("Static IP Configuration");
    gtk_box_pack_start(GTK_BOX(vbox), static_frame, FALSE, FALSE, 0);
    apply_widget_css(static_frame);
    
    GtkWidget *static_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(static_grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(static_grid), 10);
    gtk_container_set_border_width(GTK_CONTAINER(static_grid), 10);
    gtk_container_add(GTK_CONTAINER(static_frame), static_grid);
    
    // IP Address
    GtkWidget *ip_label = gtk_label_new("IP Address:");
    gtk_grid_attach(GTK_GRID(static_grid), ip_label, 0, 0, 1, 1);
    apply_widget_css(ip_label);
    
    GtkWidget *ip_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(ip_entry), "192.168.1.100");
    gtk_grid_attach(GTK_GRID(static_grid), ip_entry, 1, 0, 1, 1);
    apply_widget_css(ip_entry);
    
    // Netmask
    GtkWidget *netmask_label = gtk_label_new("Netmask:");
    gtk_grid_attach(GTK_GRID(static_grid), netmask_label, 0, 1, 1, 1);
    apply_widget_css(netmask_label);
    
    GtkWidget *netmask_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(netmask_entry), "255.255.255.0");
    gtk_grid_attach(GTK_GRID(static_grid), netmask_entry, 1, 1, 1, 1);
    apply_widget_css(netmask_entry);
    
    // Gateway
    GtkWidget *gateway_label = gtk_label_new("Gateway:");
    gtk_grid_attach(GTK_GRID(static_grid), gateway_label, 0, 2, 1, 1);
    apply_widget_css(gateway_label);
    
    GtkWidget *gateway_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(gateway_entry), "192.168.1.1");
    gtk_grid_attach(GTK_GRID(static_grid), gateway_entry, 1, 2, 1, 1);
    apply_widget_css(gateway_entry);
    
    // DNS
    GtkWidget *dns_label = gtk_label_new("DNS Server:");
    gtk_grid_attach(GTK_GRID(static_grid), dns_label, 0, 3, 1, 1);
    apply_widget_css(dns_label);
    
    GtkWidget *dns_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(dns_entry), "8.8.8.8");
    gtk_grid_attach(GTK_GRID(static_grid), dns_entry, 1, 3, 1, 1);
    apply_widget_css(dns_entry);
    
    gtk_widget_show_all(dialog);
    
    int response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response == GTK_RESPONSE_APPLY) {
        const char *iface = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(iface_combo));
        gboolean use_dhcp = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dhcp_radio));
        
        if (use_dhcp) {
            // Apply DHCP
            gchar *cmd = g_strdup_printf("nmcli connection modify '%s' ipv4.method auto && nmcli connection up '%s' 2>/dev/null &", 
                                         iface, iface);
            system(cmd);
            g_free(cmd);
        } else {
            // Apply static IP
            const char *ip = gtk_entry_get_text(GTK_ENTRY(ip_entry));
            const char *netmask = gtk_entry_get_text(GTK_ENTRY(netmask_entry));
            const char *gateway = gtk_entry_get_text(GTK_ENTRY(gateway_entry));
            const char *dns = gtk_entry_get_text(GTK_ENTRY(dns_entry));
            
            if (strlen(ip) > 0 && strlen(netmask) > 0) {
                gchar *cmd = g_strdup_printf(
                    "nmcli connection modify '%s' ipv4.method manual ipv4.addresses %s/%s "
                    "ipv4.gateway %s ipv4.dns '%s' && nmcli connection up '%s' 2>/dev/null &",
                    iface, ip, netmask, gateway ? gateway : "", dns ? dns : "8.8.8.8", iface);
                system(cmd);
                g_free(cmd);
            }
        }
        
        // Show success message
        GtkWidget *success = gtk_message_dialog_new(
            GTK_WINDOW(parent_window),
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_INFO,
            GTK_BUTTONS_OK,
            "Settings Applied"
        );
        gtk_message_dialog_format_secondary_text(
            GTK_MESSAGE_DIALOG(success),
            "Network settings have been applied to %s",
            iface
        );
        apply_widget_css(success);
        gtk_dialog_run(GTK_DIALOG(success));
        gtk_widget_destroy(success);
    }
    
    gtk_widget_destroy(dialog);
}

// Clean up provider
void internet_settings_cleanup(void)
{
    if (internet_settings_provider) {
        g_object_unref(internet_settings_provider);
        internet_settings_provider = NULL;
    }
}