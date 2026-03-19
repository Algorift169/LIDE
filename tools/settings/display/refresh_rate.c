#include "refresh_rate.h"

GtkWidget *refresh_rate_widget_new(void)
{
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *label = gtk_label_new("Rate:");
    GtkWidget *combo = gtk_combo_box_text_new();

    // Add common refresh rates
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "60 Hz");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "75 Hz");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "120 Hz");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "144 Hz");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "240 Hz");

    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0); // default 60 Hz

    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), combo, TRUE, TRUE, 0);

    return hbox;
}