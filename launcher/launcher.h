#ifndef LIDE_LAUNCHER_H
#define LIDE_LAUNCHER_H

#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <string.h>
#include <dirent.h>

typedef struct {
    char *name;
    char *exec;
} AppEntry;

void search_changed(GtkSearchEntry *entry, GtkListBox *listbox);
void launch_selected(GtkListBox *listbox, GtkListBoxRow *row, gpointer data);
gint sort_list(GtkListBoxRow *row1, GtkListBoxRow *row2, gpointer data);

#endif