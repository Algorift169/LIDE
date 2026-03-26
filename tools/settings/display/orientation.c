#include "orientation.h"

/**
 * Creates the orientation selection widget.
 * Provides radio buttons for selecting screen orientation modes:
 * - Landscape (normal)
 * - Portrait (90 degrees rotation)
 * - Landscape flipped (180 degrees rotation)
 * - Portrait flipped (270 degrees rotation)
 *
 * @return GtkWidget containing horizontally packed radio buttons.
 *         Default selection is Landscape.
 */
GtkWidget *orientation_widget_new(void)
{
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    
    /* Create radio buttons with normal landscape as the group leader */
    GtkWidget *radio_landscape = gtk_radio_button_new_with_label(NULL, "Landscape");
    GtkWidget *radio_portrait = gtk_radio_button_new_with_label_from_widget(
        GTK_RADIO_BUTTON(radio_landscape), "Portrait");
    GtkWidget *radio_landscape_flipped = gtk_radio_button_new_with_label_from_widget(
        GTK_RADIO_BUTTON(radio_landscape), "Landscape (flipped)");
    GtkWidget *radio_portrait_flipped = gtk_radio_button_new_with_label_from_widget(
        GTK_RADIO_BUTTON(radio_landscape), "Portrait (flipped)");

    gtk_box_pack_start(GTK_BOX(box), radio_landscape, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), radio_portrait, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), radio_landscape_flipped, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), radio_portrait_flipped, FALSE, FALSE, 0);

    /* Set landscape as the default selection */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_landscape), TRUE);

    return box;
}