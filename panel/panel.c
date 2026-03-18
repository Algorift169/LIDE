#include <gtk/gtk.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "tools/minimized_container.h"
#include "network_stats.h"
#include "network_manager.h"

// Define HISTORY_SIZE before using it
#define HISTORY_SIZE 60

// Define the global variables expected by the system monitor code
double cpu_history[HISTORY_SIZE] = {0};
int cpu_history_index = 0;
double mem_history[HISTORY_SIZE] = {0};
int mem_history_index = 0;

// Include system monitor files after defining the globals and HISTORY_SIZE
#include "../tools/system-monitor/cpu.c"
#include "../tools/system-monitor/memory.c"

static pid_t tools_pid = 0;  // Store the PID of the launched tools container

// Global stat variables - renamed to avoid conflicts with monitor.h
static double panel_cpu_percent = 0.0;
static guint64 panel_mem_total = 0;
static guint64 panel_mem_available = 0;
static guint64 panel_mem_used = 0;
static int panel_mem_percent = 0;

// CPU history for smooth display - renamed to avoid conflicts
static float panel_cpu_history[5] = {0};
static int panel_cpu_history_index = 0;

// Function prototypes
static int get_battery_percentage(void);
static const char* get_internet_status(void);
static void format_speed(double speed, char *buffer, size_t size);

// Wrapper functions to avoid parameter issues
static void panel_update_cpu_usage(void)
{
    CpuData cpu;
    update_cpu_usage(&cpu);
    panel_cpu_percent = cpu.usage;
}

static void panel_update_mem_usage(void)
{
    MemData mem;
    update_mem_usage(&mem);
    panel_mem_total = mem.total;
    panel_mem_available = mem.available;
    panel_mem_used = panel_mem_total - panel_mem_available;
    if (panel_mem_total > 0) {
        panel_mem_percent = (panel_mem_used * 100) / panel_mem_total;
    } else {
        panel_mem_percent = 0;
    }
}

// Format speed for display
static void format_speed(double speed, char *buffer, size_t size)
{
    if (speed < 1024) {
        snprintf(buffer, size, "%.0f B/s", speed);
    } else if (speed < 1024 * 1024) {
        snprintf(buffer, size, "%.1f KB/s", speed / 1024);
    } else {
        snprintf(buffer, size, "%.1f MB/s", speed / (1024 * 1024));
    }
}

// Find a window with a given PID using _NET_WM_PID
static Window find_window_by_pid(Display *display, pid_t pid)
{
    Atom net_client_list = XInternAtom(display, "_NET_CLIENT_LIST", True);
    Atom net_wm_pid = XInternAtom(display, "_NET_WM_PID", True);
    Atom actual_type;
    int actual_format;
    unsigned long num_items;
    unsigned long bytes_after;
    unsigned char *data = NULL;
    
    if (net_client_list == None || net_wm_pid == None)
        return None;
    
    Window root = DefaultRootWindow(display);
    
    if (XGetWindowProperty(display, root, net_client_list, 0, ~0L, False, XA_WINDOW,
                          &actual_type, &actual_format, &num_items, &bytes_after,
                          &data) != Success)
        return None;
    
    if (actual_type != XA_WINDOW || actual_format != 32) {
        XFree(data);
        return None;
    }
    
    Window *windows = (Window*)data;
    Window result = None;
    
    for (unsigned long i = 0; i < num_items; i++) {
        Window win = windows[i];
        
        unsigned char *pid_data = NULL;
        unsigned long pid_items;
        if (XGetWindowProperty(display, win, net_wm_pid, 0, 1, False, XA_CARDINAL,
                               &actual_type, &actual_format, &pid_items, &bytes_after,
                               &pid_data) == Success) {
            if (actual_type == XA_CARDINAL && actual_format == 32 && pid_items > 0) {
                unsigned long win_pid = *(unsigned long*)pid_data;
                if (win_pid == (unsigned long)pid) {
                    result = win;
                    XFree(pid_data);
                    break;
                }
            }
            if (pid_data)
                XFree(pid_data);
        }
    }
    
    XFree(data);
    return result;
}

static void launch_tools(GtkButton *button, gpointer data)
{
    (void)button;
    (void)data;
    
    if (tools_pid > 0) {
        Display *display = XOpenDisplay(NULL);
        if (display) {
            Window win = find_window_by_pid(display, tools_pid);
            if (win != None) {
                XRaiseWindow(display, win);
                XMapRaised(display, win);
                XFlush(display);
                XCloseDisplay(display);
                return;
            }
            XCloseDisplay(display);
        }
        tools_pid = 0;
    }
    
    pid_t pid = fork();
    if (pid == 0) {
        execl("./blackline-tools", "blackline-tools", NULL);
        exit(0);
    } else if (pid > 0) {
        tools_pid = pid;
    }
}

static void do_nothing(GtkButton *button, gpointer data)
{
    (void)button;
    (void)data;
}

// Update all system stats
static gboolean update_system_stats(gpointer user_data)
{
    GtkWidget *cpu_label = GTK_WIDGET(user_data);
    GtkWidget *mem_label = GTK_WIDGET(g_object_get_data(G_OBJECT(cpu_label), "mem-label"));
    GtkWidget *upload_label = GTK_WIDGET(g_object_get_data(G_OBJECT(cpu_label), "upload-label"));
    GtkWidget *download_label = GTK_WIDGET(g_object_get_data(G_OBJECT(cpu_label), "download-label"));
    GtkWidget *battery_label = GTK_WIDGET(g_object_get_data(G_OBJECT(cpu_label), "battery-label"));
    
    if (!cpu_label || !mem_label || !upload_label || !download_label) {
        return G_SOURCE_CONTINUE;
    }
    
    // Update CPU using wrapper
    panel_update_cpu_usage();
    
    // Smooth CPU display with moving average
    panel_cpu_history[panel_cpu_history_index] = panel_cpu_percent;
    panel_cpu_history_index = (panel_cpu_history_index + 1) % 5;
    
    float avg_cpu = 0;
    for (int i = 0; i < 5; i++) {
        avg_cpu += panel_cpu_history[i];
    }
    avg_cpu /= 5;
    
    char cpu_text[64];
    if (avg_cpu < 10) {
        snprintf(cpu_text, sizeof(cpu_text), "CPU:  %.1f%%", avg_cpu);
    } else {
        snprintf(cpu_text, sizeof(cpu_text), "CPU: %.1f%%", avg_cpu);
    }
    gtk_label_set_text(GTK_LABEL(cpu_label), cpu_text);
    
    // Update Memory using wrapper
    panel_update_mem_usage();
    
    char mem_text[64];
    snprintf(mem_text, sizeof(mem_text), "RAM: %d%%", panel_mem_percent);
    gtk_label_set_text(GTK_LABEL(mem_label), mem_text);
    
    // Update Network stats
    network_stats_update();
    double upload_speed = network_stats_get_upload();
    double download_speed = network_stats_get_download();
    
    char upload_text[32];
    char download_text[32];
    format_speed(upload_speed, upload_text, sizeof(upload_text));
    format_speed(download_speed, download_text, sizeof(download_text));
    
    gtk_label_set_text(GTK_LABEL(upload_label), upload_text);
    gtk_label_set_text(GTK_LABEL(download_label), download_text);
    
    // Update Battery percentage
    if (battery_label) {
        int battery = get_battery_percentage();
        char battery_text[32];
        snprintf(battery_text, sizeof(battery_text), "🔋 %d%%", battery);
        gtk_label_set_text(GTK_LABEL(battery_label), battery_text);
    }
    
    return G_SOURCE_CONTINUE;
}

static gboolean update_clock(gpointer label)
{
    time_t rawtime;
    struct tm *timeinfo;
    char buffer[64];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    
    strftime(buffer, sizeof(buffer), "%a %d %b %H:%M:%S", timeinfo);
    gtk_label_set_text(GTK_LABEL(label), buffer);
    return G_SOURCE_CONTINUE;
}

// Get battery percentage
static int get_battery_percentage(void)
{
    FILE *fp;
    int capacity = 0;
    
    fp = fopen("/sys/class/power_supply/BAT0/capacity", "r");
    if (!fp) {
        fp = fopen("/sys/class/power_supply/BAT1/capacity", "r");
    }
    
    if (fp) {
        fscanf(fp, "%d", &capacity);
        fclose(fp);
    }
    
    return capacity;
}

// Lock screen callback
static void on_lock_screen_clicked(GtkButton *button, gpointer data)
{
    (void)button;
    (void)data;
    system("xlock -mode blank &");
}

// Power off callback
static void on_power_off_clicked(GtkButton *button, gpointer data)
{
    (void)button;
    (void)data;
    
    GtkWidget *dialog = gtk_message_dialog_new(NULL,
                                              GTK_DIALOG_MODAL,
                                              GTK_MESSAGE_QUESTION,
                                              GTK_BUTTONS_YES_NO,
                                              "Power off the system?");
    gtk_window_set_title(GTK_WINDOW(dialog), "Shutdown Confirmation");
    
    int response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    
    if (response == GTK_RESPONSE_YES) {
        system("shutdown -h now");
    }
}

// Restart callback
static void on_restart_clicked(GtkButton *button, gpointer data)
{
    (void)button;
    (void)data;
    
    GtkWidget *dialog = gtk_message_dialog_new(NULL,
                                              GTK_DIALOG_MODAL,
                                              GTK_MESSAGE_QUESTION,
                                              GTK_BUTTONS_YES_NO,
                                              "Restart the system?");
    gtk_window_set_title(GTK_WINDOW(dialog), "Restart Confirmation");
    
    int response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    
    if (response == GTK_RESPONSE_YES) {
        system("shutdown -r now");
    }
}

static void activate(GtkApplication *app, gpointer user_data)
{
    (void)app;
    (void)user_data;
    
    // Initialize the minimized container
    minimized_container_initialize();
    
    // Initialize network stats
    network_stats_init();
    
    // Initialize network manager
    NetworkManager *nm = network_manager_new();
    
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "BlackLine Panel");
    gtk_window_set_default_size(GTK_WINDOW(window), -1, 35);
    gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
    gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);
    gtk_window_set_type_hint(GTK_WINDOW(window), GDK_WINDOW_TYPE_HINT_DOCK);
    
    // Full width
    GdkDisplay *gdisplay = gdk_display_get_default();
    GdkMonitor *monitor = gdk_display_get_primary_monitor(gdisplay);
    GdkRectangle geom;
    gdk_monitor_get_geometry(monitor, &geom);
    gtk_window_set_default_size(GTK_WINDOW(window), geom.width, 35);
    gtk_window_move(GTK_WINDOW(window), 0, 0);
    
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_container_add(GTK_CONTAINER(window), box);
    
    // BlackLine button
    GtkWidget *btn1 = gtk_button_new_with_label("BlackLine");
    g_signal_connect(btn1, "clicked", G_CALLBACK(do_nothing), NULL);
    gtk_box_pack_start(GTK_BOX(box), btn1, FALSE, FALSE, 0);
    
    // Tools button
    GtkWidget *btn2 = gtk_button_new_with_label("Tools");
    g_signal_connect(btn2, "clicked", G_CALLBACK(launch_tools), NULL);
    gtk_box_pack_start(GTK_BOX(box), btn2, FALSE, FALSE, 0);
    
    // Minimized Apps button
    GtkWidget *min_btn = minimized_container_get_toggle_button();
    gtk_box_pack_start(GTK_BOX(box), min_btn, FALSE, FALSE, 0);
    
    GtkWidget *spacer = gtk_label_new(NULL);
    gtk_box_pack_start(GTK_BOX(box), spacer, TRUE, TRUE, 0);
    
    // System stats container (right side)
    GtkWidget *stats_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_name(stats_box, "stats-box");
    gtk_box_pack_end(GTK_BOX(box), stats_box, FALSE, FALSE, 5);
    
    // CPU Label
    GtkWidget *cpu_label = gtk_label_new("CPU: 0.0%");
    gtk_box_pack_start(GTK_BOX(stats_box), cpu_label, FALSE, FALSE, 0);
    
    // Memory Label
    GtkWidget *mem_label = gtk_label_new("RAM: 0%");
    gtk_box_pack_start(GTK_BOX(stats_box), mem_label, FALSE, FALSE, 0);
    
    // Separator
    GtkWidget *sep1 = gtk_label_new("|");
    gtk_box_pack_start(GTK_BOX(stats_box), sep1, FALSE, FALSE, 0);
    
    // Upload Label
    GtkWidget *upload_label = gtk_label_new("↑ 0 B/s");
    gtk_box_pack_start(GTK_BOX(stats_box), upload_label, FALSE, FALSE, 0);
    
    // Download Label
    GtkWidget *download_label = gtk_label_new("↓ 0 B/s");
    gtk_box_pack_start(GTK_BOX(stats_box), download_label, FALSE, FALSE, 0);
    
    // Separator
    GtkWidget *sep2 = gtk_label_new("|");
    gtk_box_pack_start(GTK_BOX(stats_box), sep2, FALSE, FALSE, 0);
    
    // Clock with date
    GtkWidget *clock = gtk_label_new(NULL);
    update_clock(clock);
    gtk_box_pack_start(GTK_BOX(stats_box), clock, FALSE, FALSE, 0);
    
    // Separator
    GtkWidget *sep3 = gtk_label_new("|");
    gtk_box_pack_start(GTK_BOX(stats_box), sep3, FALSE, FALSE, 0);
    
    // Battery Label
    GtkWidget *battery_label = gtk_label_new("🔋 0%");
    gtk_box_pack_start(GTK_BOX(stats_box), battery_label, FALSE, FALSE, 0);
    
    // Network Manager Button
    GtkWidget *network_btn = gtk_button_new();
    GtkWidget *network_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    
    GtkWidget *network_icon = gtk_label_new(network_manager_get_status_icon(nm));
    GtkWidget *network_text = gtk_label_new(network_manager_get_status_text(nm));
    
    gtk_box_pack_start(GTK_BOX(network_hbox), network_icon, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(network_hbox), network_text, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(network_btn), network_hbox);
    
    // Connect click to show network popover
    g_signal_connect_swapped(network_btn, "clicked", G_CALLBACK(network_manager_show_popover), nm);
    
    gtk_box_pack_start(GTK_BOX(stats_box), network_btn, FALSE, FALSE, 0);
    
    // Separator
    GtkWidget *sep4 = gtk_label_new("|");
    gtk_box_pack_start(GTK_BOX(stats_box), sep4, FALSE, FALSE, 0);
    
    // System Controls - Lock Screen Button
    GtkWidget *lock_btn = gtk_button_new_with_label("🔒");
    gtk_widget_set_tooltip_text(lock_btn, "Lock screen");
    g_signal_connect(lock_btn, "clicked", G_CALLBACK(on_lock_screen_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(stats_box), lock_btn, FALSE, FALSE, 0);
    
    // Power Off Button
    GtkWidget *poweroff_btn = gtk_button_new_with_label("⏻");
    gtk_widget_set_tooltip_text(poweroff_btn, "Power off");
    g_signal_connect(poweroff_btn, "clicked", G_CALLBACK(on_power_off_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(stats_box), poweroff_btn, FALSE, FALSE, 0);
    
    // Restart Button
    GtkWidget *restart_btn = gtk_button_new_with_label("🔄");
    gtk_widget_set_tooltip_text(restart_btn, "Restart");
    g_signal_connect(restart_btn, "clicked", G_CALLBACK(on_restart_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(stats_box), restart_btn, FALSE, FALSE, 0);
    
    // Store references for stats update
    g_object_set_data(G_OBJECT(cpu_label), "mem-label", mem_label);
    g_object_set_data(G_OBJECT(cpu_label), "upload-label", upload_label);
    g_object_set_data(G_OBJECT(cpu_label), "download-label", download_label);
    g_object_set_data(G_OBJECT(cpu_label), "battery-label", battery_label);
    
    // Store network manager for cleanup
    g_object_set_data_full(G_OBJECT(window), "network-manager", nm, 
                          (GDestroyNotify)network_manager_cleanup);
    
    // Update timers
    g_timeout_add_seconds(1, update_clock, clock);
    g_timeout_add(2000, update_system_stats, cpu_label);
    
    // Update network status periodically
    g_timeout_add(2000, (GSourceFunc)network_manager_refresh, nm);
    
    // Apply CSS styling
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "window { background-color: #0b0f14; color: #ffffff; border-bottom: 1px solid #00ff88; }"
        "button { background-color: #1e2429; color: #00ff88; border: none; padding: 2px 8px; margin: 2px; }"
        "button:hover { background-color: #2a323a; }"
        "label { color: #00ff88; padding: 0 2px; font-size: 11px; }"
        "#stats-box label { font-family: monospace; }"
        "#network-status-icon { font-size: 14px; }"
        "popover { background-color: #1e2429; }"
        "popover label { color: #ffffff; }"
        "popover button { background-color: #2d2d2d; color: #00ff88; }"
        "popover button:hover { background-color: #3d3d3d; }",
        -1, NULL);
    
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
    
    gtk_widget_show_all(window);
}

int main(int argc, char **argv)
{
    GtkApplication *app = gtk_application_new("org.blackline.panel", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    
    network_stats_cleanup();
    
    return status;
}