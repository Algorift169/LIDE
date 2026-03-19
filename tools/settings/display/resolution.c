#include "resolution.h"

GtkWidget *resolution_widget_new(void)
{
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *label = gtk_label_new("Resolution:");
    GtkWidget *combo = gtk_combo_box_text_new();

    // Add common resolutions
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "1920x1080");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "1366x768");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "1280x720");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "2560x1440");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "3840x2160");

    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);

    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), combo, TRUE, TRUE, 0);

    return hbox;
}