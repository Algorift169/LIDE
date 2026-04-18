#include "network.h"

/* Forward declarations for panel functions */
extern void show_internet_settings(GtkWidget *parent_window);
extern void show_wifi_list(GtkWidget *parent_window);
extern void show_connection_details(GtkWidget *parent_window);
extern GtkWidget* build_wifi_network_list(void);

/*
 * network.c
 * 
 * Network settings tab - reuses working panel network components.
 */

static void on_wired_clicked(GtkButton *btn, gpointer data)
{
    (void)btn;
    GtkWidget *parent = GTK_WIDGET(data);
    show_internet_settings(parent);
}

static void on_wifi_clicked(GtkButton *btn, gpointer data)
{
    (void)btn;
    GtkWidget *parent = GTK_WIDGET(data);
    show_wifi_list(parent);
}

static void on_details_clicked(GtkButton *btn, gpointer data)
{
    (void)btn;
    GtkWidget *parent = GTK_WIDGET(data);
    show_connection_details(parent);
}

GtkWidget *network_settings_tab_new(void) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 15);
    
    /* Wired (Ethernet) section */
    GtkWidget *wired_frame = gtk_frame_new(NULL);
    gtk_frame_set_label(GTK_FRAME(wired_frame), "Wired (Ethernet)");
    GtkWidget *wired_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(wired_box), 10);
    
    GtkWidget *wired_desc = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(wired_desc), 
        "<span size='small'>Configure Ethernet connection settings</span>");
    gtk_label_set_xalign(GTK_LABEL(wired_desc), 0.0);
    gtk_box_pack_start(GTK_BOX(wired_box), wired_desc, FALSE, FALSE, 0);
    
    GtkWidget *wired_btn = gtk_button_new_with_label("⚙️ Ethernet Settings");
    g_signal_connect(wired_btn, "clicked", G_CALLBACK(on_wired_clicked), vbox);
    gtk_box_pack_start(GTK_BOX(wired_box), wired_btn, FALSE, FALSE, 5);
    
    gtk_container_add(GTK_CONTAINER(wired_frame), wired_box);
    gtk_box_pack_start(GTK_BOX(vbox), wired_frame, FALSE, FALSE, 0);
    
    /* WiFi section */
    GtkWidget *wifi_frame = gtk_frame_new(NULL);
    gtk_frame_set_label(GTK_FRAME(wifi_frame), "Wi-Fi Networks");
    GtkWidget *wifi_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(wifi_box), 10);
    
    GtkWidget *wifi_desc = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(wifi_desc), 
        "<span size='small'>Scan and connect to wireless networks</span>");
    gtk_label_set_xalign(GTK_LABEL(wifi_desc), 0.0);
    gtk_box_pack_start(GTK_BOX(wifi_box), wifi_desc, FALSE, FALSE, 0);
    
    GtkWidget *wifi_btn = gtk_button_new_with_label("📡 Wi-Fi Networks");
    g_signal_connect(wifi_btn, "clicked", G_CALLBACK(on_wifi_clicked), vbox);
    gtk_box_pack_start(GTK_BOX(wifi_box), wifi_btn, FALSE, FALSE, 5);
    
    gtk_container_add(GTK_CONTAINER(wifi_frame), wifi_box);
    gtk_box_pack_start(GTK_BOX(vbox), wifi_frame, FALSE, FALSE, 0);
    
    /* Connection details section */
    GtkWidget *details_frame = gtk_frame_new(NULL);
    gtk_frame_set_label(GTK_FRAME(details_frame), "Connection Details");
    GtkWidget *details_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(details_box), 10);
    
    GtkWidget *details_desc = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(details_desc), 
        "<span size='small'>View current IP address, gateway, and DNS</span>");
    gtk_label_set_xalign(GTK_LABEL(details_desc), 0.0);
    gtk_box_pack_start(GTK_BOX(details_box), details_desc, FALSE, FALSE, 0);
    
    GtkWidget *details_btn = gtk_button_new_with_label("ℹ️ Show Connection Details");
    g_signal_connect(details_btn, "clicked", G_CALLBACK(on_details_clicked), vbox);
    gtk_box_pack_start(GTK_BOX(details_box), details_btn, FALSE, FALSE, 5);
    
    gtk_container_add(GTK_CONTAINER(details_frame), details_box);
    gtk_box_pack_start(GTK_BOX(vbox), details_frame, FALSE, FALSE, 0);
    
    /* Quick WiFi network list preview */
    GtkWidget *preview_frame = gtk_frame_new(NULL);
    gtk_frame_set_label(GTK_FRAME(preview_frame), "Available Networks (Preview)");
    GtkWidget *preview_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(preview_scroll), 
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(preview_scroll, -1, 200);
    
    GtkWidget *preview_list = build_wifi_network_list();
    gtk_container_add(GTK_CONTAINER(preview_scroll), preview_list);
    gtk_container_add(GTK_CONTAINER(preview_frame), preview_scroll);
    gtk_box_pack_start(GTK_BOX(vbox), preview_frame, TRUE, TRUE, 0);
    
    /* Spacer */
    GtkWidget *spacer = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(vbox), spacer, FALSE, FALSE, 0);
    
    return vbox;
}