#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Forward declaration for callback */
static void on_show_password_toggled(GtkToggleButton *btn, gpointer entry);
static GtkCssProvider *wifi_connect_provider = NULL;

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
    if (wifi_connect_provider == NULL) {
        wifi_connect_provider = gtk_css_provider_new();
        gtk_css_provider_load_from_data(wifi_connect_provider,
            "window { background-color: #0b0f14; color: #ffffff; }"
            "button { background-color: #1a1f26; color: #a92d5a; border: 1px solid #a92d5a; padding: 10px 20px; margin: 5px; font-weight: bold; border-radius: 4px; }"
            "button:hover { background-color: #252d36; color: #472e57; border: 1px solid #472e57; }"
            "label { color: #a92d5a; font-size: 11px; font-weight: 500; }"
            "entry { background-color: #1a1f26; color: #a92d5a; border: 1px solid #a92d5a; padding: 8px; border-radius: 4px; }"
            "checkbutton { color: #a92d5a; }"
            "checkbutton label { color: #a92d5a; }",
            -1, NULL);
    }
    
    gtk_style_context_add_provider(
        gtk_widget_get_style_context(widget),
        GTK_STYLE_PROVIDER(wifi_connect_provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

/**
 * Checks whether a WiFi network requires a password.
 *
 * @param ssid Network SSID to check.
 * @return TRUE if network is secured, FALSE if open or on error.
 *
 * @sideeffect Executes nmcli to query network security status.
 */
static gboolean is_network_secured(const char *ssid)
{
    FILE *fp = popen("nmcli -t -f SSID,SECURITY device wifi list 2>/dev/null", "r");
    if (!fp) return TRUE; /* Assume secured if can't check */
    
    char line[512];
    gboolean secured = TRUE; /* Default to secured */
    
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = 0;
        char *colon = strchr(line, ':');
        if (colon) {
            *colon = '\0';
            char *net_ssid = line;
            char *security = colon + 1;
            
            if (strcmp(net_ssid, ssid) == 0) {
                secured = (strlen(security) > 0 && strcmp(security, "--") != 0);
                break;
            }
        }
    }
    pclose(fp);
    return secured;
}

/**
 * Displays a modal dialog for connecting to a WiFi network.
 * For open networks, connects directly without password prompt.
 * For secured networks, shows password entry dialog.
 *
 * @param parent_window Parent window for the dialog.
 * @param ssid          Network SSID to connect to.
 *
 * @sideeffect Executes nmcli commands to connect to the network.
 * @sideeffect Shows success/error message dialogs.
 */
void show_wifi_connect_dialog(GtkWidget *parent_window, const char *ssid)
{
    /* First check if network is secured */
    gboolean secured = is_network_secured(ssid);
    
    if (!secured) {
        /* Open network - connect directly */
        gchar *cmd = g_strdup_printf("nmcli device wifi connect '%s' 2>/dev/null", ssid);
        system(cmd);
        g_free(cmd);
        
        /* Show success message */
        GtkWidget *success = gtk_message_dialog_new(
            GTK_WINDOW(parent_window),
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_INFO,
            GTK_BUTTONS_OK,
            "Connected to %s",
            ssid
        );
        gtk_message_dialog_format_secondary_text(
            GTK_MESSAGE_DIALOG(success),
            "Successfully connected to the open network."
        );
        apply_widget_css(success);
        gtk_dialog_run(GTK_DIALOG(success));
        gtk_widget_destroy(success);
        return;
    }
    
    /* Secured network - show password dialog */
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
    
    /* Apply CSS to dialog */
    apply_widget_css(dialog);
    
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 20);
    gtk_container_add(GTK_CONTAINER(content), vbox);
    
    /* Title with SSID */
    GtkWidget *title = gtk_label_new(NULL);
    gchar *title_text = g_strdup_printf("<b>📡 Connect to WiFi Network</b>\n\n<big><b>%s</b></big>\n<small>This network is secured</small>", ssid);
    gtk_label_set_markup(GTK_LABEL(title), title_text);
    gtk_label_set_xalign(GTK_LABEL(title), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), title, FALSE, FALSE, 0);
    g_free(title_text);
    apply_widget_css(title);
    
    /* Password label */
    GtkWidget *pwd_label = gtk_label_new("Password:");
    gtk_label_set_xalign(GTK_LABEL(pwd_label), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), pwd_label, FALSE, FALSE, 0);
    apply_widget_css(pwd_label);
    
    /* Password entry */
    GtkWidget *pwd_entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(pwd_entry), FALSE);
    gtk_entry_set_placeholder_text(GTK_ENTRY(pwd_entry), "Enter WiFi password");
    gtk_box_pack_start(GTK_BOX(vbox), pwd_entry, FALSE, FALSE, 0);
    apply_widget_css(pwd_entry);
    
    /* Show password checkbox */
    GtkWidget *show_pwd = gtk_check_button_new_with_label("Show password");
    g_signal_connect(show_pwd, "toggled", G_CALLBACK(on_show_password_toggled), pwd_entry);
    gtk_box_pack_start(GTK_BOX(vbox), show_pwd, FALSE, FALSE, 0);
    apply_widget_css(show_pwd);
    
    /* Connect automatically checkbox */
    GtkWidget *auto_connect = gtk_check_button_new_with_label("Connect automatically");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(auto_connect), TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), auto_connect, FALSE, FALSE, 0);
    apply_widget_css(auto_connect);
    
    gtk_widget_show_all(dialog);
    
    int response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response == GTK_RESPONSE_OK) {
        const gchar *password = gtk_entry_get_text(GTK_ENTRY(pwd_entry));
        gboolean auto_conn = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(auto_connect));
        
        /* Execute connection command */
        if (password && strlen(password) > 0) {
            gchar *cmd = g_strdup_printf("nmcli device wifi connect '%s' password '%s' %s 2>/dev/null", 
                                         ssid, password, 
                                         auto_conn ? "" : "temporary");
            int result = system(cmd);
            g_free(cmd);
            
            /* Show result */
            GtkWidget *status_dialog = gtk_message_dialog_new(
                GTK_WINDOW(parent_window),
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
                    "Failed to connect to %s.\nPlease check the password and try again.", ssid
                );
            }
            
            apply_widget_css(status_dialog);
            gtk_dialog_run(GTK_DIALOG(status_dialog));
            gtk_widget_destroy(status_dialog);
        }
    }
    
    gtk_widget_destroy(dialog);
}

/**
 * Callback for show password checkbox toggle.
 * Toggles visibility of the password entry field.
 *
 * @param btn   The checkbox that was toggled.
 * @param entry The GtkEntry widget to modify.
 */
static void on_show_password_toggled(GtkToggleButton *btn, gpointer entry)
{
    gtk_entry_set_visibility(GTK_ENTRY(entry), gtk_toggle_button_get_active(btn));
}

/**
 * Quick connect to WiFi network without showing password dialog.
 * Assumes network is open or already has stored credentials.
 *
 * @param ssid Network SSID to connect to.
 *
 * @sideeffect Executes nmcli connection command.
 */
void wifi_quick_connect(const char *ssid)
{
    gchar *cmd = g_strdup_printf("nmcli device wifi connect '%s' 2>/dev/null", ssid);
    system(cmd);
    g_free(cmd);
}

/**
 * Removes a WiFi network from stored connections.
 *
 * @param ssid Network SSID to forget.
 *
 * @sideeffect Executes nmcli to delete the connection profile.
 */
void wifi_forget_network(const char *ssid)
{
    gchar *cmd = g_strdup_printf("nmcli connection delete '%s' 2>/dev/null", ssid);
    system(cmd);
    g_free(cmd);
}

/**
 * Cleans up resources used by the WiFi connect module.
 * Unrefs the global CSS provider if it exists.
 */
void wifi_connect_cleanup(void)
{
    if (wifi_connect_provider) {
        g_object_unref(wifi_connect_provider);
        wifi_connect_provider = NULL;
    }
}