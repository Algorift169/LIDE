#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declaration for callback
static void on_show_password_toggled(GtkToggleButton *btn, gpointer entry);
static GtkCssProvider *wifi_connect_provider = NULL;

// Apply CSS to a specific widget only
static void apply_widget_css(GtkWidget *widget)
{
    if (wifi_connect_provider == NULL) {
        wifi_connect_provider = gtk_css_provider_new();
        gtk_css_provider_load_from_data(wifi_connect_provider,
            "window { background-color: #0b0f14; color: #ffffff; }"
            "button { background-color: #1a1f26; color: #00ff88; border: 1px solid #00ff88; padding: 10px 20px; margin: 5px; font-weight: bold; border-radius: 4px; }"
            "button:hover { background-color: #252d36; color: #00ff00; border: 1px solid #00ff00; }"
            "label { color: #00ff88; font-size: 11px; font-weight: 500; }"
            "entry { background-color: #1a1f26; color: #00ff88; border: 1px solid #00ff88; padding: 8px; border-radius: 4px; }"
            "checkbutton { color: #00ff88; }"
            "checkbutton label { color: #00ff88; }",
            -1, NULL);
    }
    
    gtk_style_context_add_provider(
        gtk_widget_get_style_context(widget),
        GTK_STYLE_PROVIDER(wifi_connect_provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

// WiFi connection dialog
void show_wifi_connect_dialog(GtkWidget *parent_window, const char *ssid)
{
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Connect to WiFi",
        GTK_WINDOW(parent_window),
        GTK_DIALOG_MODAL,
        "Cancel",
        GTK_RESPONSE_CANCEL,
        "Connect",
        GTK_RESPONSE_OK,
        NULL
    );
    
    gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 250);
    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
    
    // Apply CSS to dialog
    apply_widget_css(dialog);
    
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 20);
    gtk_container_add(GTK_CONTAINER(content), vbox);
    
    // Title with SSID
    GtkWidget *title = gtk_label_new(NULL);
    gchar *title_text = g_strdup_printf("<b>📡 Connect to WiFi Network</b>\n\n<big><b>%s</b></big>", ssid);
    gtk_label_set_markup(GTK_LABEL(title), title_text);
    gtk_label_set_xalign(GTK_LABEL(title), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), title, FALSE, FALSE, 0);
    g_free(title_text);
    apply_widget_css(title);
    
    // Password label
    GtkWidget *pwd_label = gtk_label_new("Password:");
    gtk_label_set_xalign(GTK_LABEL(pwd_label), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), pwd_label, FALSE, FALSE, 0);
    apply_widget_css(pwd_label);
    
    // Password entry
    GtkWidget *pwd_entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(pwd_entry), FALSE);
    gtk_entry_set_placeholder_text(GTK_ENTRY(pwd_entry), "Enter WiFi password");
    gtk_box_pack_start(GTK_BOX(vbox), pwd_entry, FALSE, FALSE, 0);
    apply_widget_css(pwd_entry);
    
    // Show password checkbox
    GtkWidget *show_pwd = gtk_check_button_new_with_label("Show password");
    g_signal_connect(show_pwd, "toggled", G_CALLBACK(on_show_password_toggled), pwd_entry);
    gtk_box_pack_start(GTK_BOX(vbox), show_pwd, FALSE, FALSE, 0);
    apply_widget_css(show_pwd);
    
    // Connect automatically checkbox
    GtkWidget *auto_connect = gtk_check_button_new_with_label("Connect automatically");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(auto_connect), TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), auto_connect, FALSE, FALSE, 0);
    apply_widget_css(auto_connect);
    
    gtk_widget_show_all(dialog);
    
    int response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response == GTK_RESPONSE_OK) {
        const gchar *password = gtk_entry_get_text(GTK_ENTRY(pwd_entry));
        gboolean auto_conn = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(auto_connect));
        
        // Execute connection command
        if (password && strlen(password) > 0) {
            gchar *cmd = g_strdup_printf("nmcli device wifi connect '%s' password '%s' %s 2>/dev/null &", 
                                         ssid, password, 
                                         auto_conn ? "" : "temporary");
            system(cmd);
            g_free(cmd);
            
            // Show connection status
            GtkWidget *status_dialog = gtk_message_dialog_new(
                GTK_WINDOW(parent_window),
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_INFO,
                GTK_BUTTONS_OK,
                "Connecting to %s",
                ssid
            );
            gtk_message_dialog_format_secondary_text(
                GTK_MESSAGE_DIALOG(status_dialog),
                "Attempting to connect to the WiFi network...\nThis may take a few seconds."
            );
            apply_widget_css(status_dialog);
            gtk_dialog_run(GTK_DIALOG(status_dialog));
            gtk_widget_destroy(status_dialog);
        }
    }
    
    gtk_widget_destroy(dialog);
}

// Callback for show password checkbox
static void on_show_password_toggled(GtkToggleButton *btn, gpointer entry)
{
    gtk_entry_set_visibility(GTK_ENTRY(entry), gtk_toggle_button_get_active(btn));
}

// Quick connect to WiFi (without password dialog)
void wifi_quick_connect(const char *ssid)
{
    gchar *cmd = g_strdup_printf("nmcli device wifi connect '%s' 2>/dev/null &", ssid);
    system(cmd);
    g_free(cmd);
}

// Forget WiFi network
void wifi_forget_network(const char *ssid)
{
    gchar *cmd = g_strdup_printf("nmcli connection delete '%s' 2>/dev/null &", ssid);
    system(cmd);
    g_free(cmd);
}

// Clean up provider
void wifi_connect_cleanup(void)
{
    if (wifi_connect_provider) {
        g_object_unref(wifi_connect_provider);
        wifi_connect_provider = NULL;
    }
}