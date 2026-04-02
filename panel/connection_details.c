#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * connection_details.c
 * 
 * Connection details display. Gathers network configuration data from
 * system commands and presents in a modal dialog.
 */

/**
 * Displays a modal dialog showing network configuration information.
 * Retrieves hostname, IP address, gateway, and DNS server via shell commands.
 *
 * @param parent_window The parent GtkWidget to attach the dialog to.
 *                      The dialog will be modal relative to this window.
 *
 * @sideeffect Creates and runs a modal dialog. Blocks execution until dialog is closed.
 * @sideeffect Executes multiple external commands (hostname, hostname -I, ip route, cat)
 *             via popen() to gather network information.
 * @sideeffect Applies custom CSS styling to the dialog window and labels.
 * @memory     Allocated strings are freed; dialog is destroyed after use.
 * @security   Executes system commands without sanitization. Assumes standard Linux
 *             network utilities are available in PATH.
 */
void show_connection_details(GtkWidget *parent_window)
{
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Connection Details",
        GTK_WINDOW(parent_window),
        GTK_DIALOG_MODAL,
        "Close",
        GTK_RESPONSE_CLOSE,
        NULL
    );
    
    gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 300);
    
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 15);
    gtk_container_add(GTK_CONTAINER(content), vbox);
    
    /* Collect network information via shell commands.
     * Commands are piped through popen() and output is trimmed. */
    char hostname[256] = "unknown";
    char ip_addr[256] = "unknown";
    char netmask[256] = "unknown";
    char gateway[256] = "unknown";
    char dns[256] = "unknown";
    
    /* Retrieve system hostname. */
    FILE *fp = popen("hostname 2>/dev/null", "r");
    if (fp) {
        if (fgets(hostname, sizeof(hostname)-1, fp)) {
            hostname[strcspn(hostname, "\n")] = 0;
        }
        pclose(fp);
    }
    
    /* Retrieve first non-loopback IP address. */
    fp = popen("hostname -I 2>/dev/null | awk '{print $1}'", "r");
    if (fp) {
        if (fgets(ip_addr, sizeof(ip_addr)-1, fp)) {
            ip_addr[strcspn(ip_addr, "\n")] = 0;
        }
        pclose(fp);
    }
    
    /* Retrieve default gateway from routing table. */
    fp = popen("ip route | grep default | awk '{print $3}'", "r");
    if (fp) {
        if (fgets(gateway, sizeof(gateway)-1, fp)) {
            gateway[strcspn(gateway, "\n")] = 0;
        }
        pclose(fp);
    }
    
    /* Retrieve primary nameserver from resolv.conf. */
    fp = popen("cat /etc/resolv.conf 2>/dev/null | grep nameserver | head -1 | awk '{print $2}'", "r");
    if (fp) {
        if (fgets(dns, sizeof(dns)-1, fp)) {
            dns[strcspn(dns, "\n")] = 0;
        }
        pclose(fp);
    }
    
    /* Build dialog UI with collected network information. */
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<b>🖥️  Network Configuration</b>");
    gtk_label_set_xalign(GTK_LABEL(title), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), title, FALSE, FALSE, 0);
    
    /* Hostname row */
    GtkWidget *hostname_label = gtk_label_new(NULL);
    gchar *hostname_text = g_strdup_printf("<b>Hostname:</b> %s", hostname);
    gtk_label_set_markup(GTK_LABEL(hostname_label), hostname_text);
    gtk_label_set_xalign(GTK_LABEL(hostname_label), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), hostname_label, FALSE, FALSE, 0);
    g_free(hostname_text);
    
    /* IP address row */
    GtkWidget *ip_label = gtk_label_new(NULL);
    gchar *ip_text = g_strdup_printf("<b>IP Address:</b> %s", ip_addr);
    gtk_label_set_markup(GTK_LABEL(ip_label), ip_text);
    gtk_label_set_xalign(GTK_LABEL(ip_label), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), ip_label, FALSE, FALSE, 0);
    g_free(ip_text);
    
    /* Gateway row - fallback to "N/A" if not configured */
    GtkWidget *gw_label = gtk_label_new(NULL);
    gchar *gw_text = g_strdup_printf("<b>Gateway:</b> %s", strlen(gateway) > 0 ? gateway : "N/A");
    gtk_label_set_markup(GTK_LABEL(gw_label), gw_text);
    gtk_label_set_xalign(GTK_LABEL(gw_label), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), gw_label, FALSE, FALSE, 0);
    g_free(gw_text);
    
    /* DNS row - fallback to "Auto" if not configured */
    GtkWidget *dns_label = gtk_label_new(NULL);
    gchar *dns_text = g_strdup_printf("<b>DNS Server:</b> %s", strlen(dns) > 0 ? dns : "Auto");
    gtk_label_set_markup(GTK_LABEL(dns_label), dns_text);
    gtk_label_set_xalign(GTK_LABEL(dns_label), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), dns_label, FALSE, FALSE, 0);
    g_free(dns_text);
    
    /* Expander to push content upward */
    GtkWidget *spacer = gtk_label_new(NULL);
    gtk_box_pack_start(GTK_BOX(vbox), spacer, TRUE, TRUE, 0);
    
    /* Apply custom dark theme styling to dialog */
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "window { background-color: #0b0f14; color: #ffffff; }"
        "label { color: #00ff88; }",
        -1, NULL);
    gtk_style_context_add_provider_for_screen(
        gtk_widget_get_screen(dialog),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
    
    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}