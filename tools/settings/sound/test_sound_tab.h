#ifndef LIDE_TEST_SOUND_TAB_H
#define LIDE_TEST_SOUND_TAB_H

#include <gtk/gtk.h>



/*
 * test_sound_tab.h
 * 
 * Sound test interface definitions
 *
 * This module is part of the LIDE desktop environment system.
 * See the main window manager (wm/) and session management (session/)
 * for system architecture overview.
 */
/**
 * Creates the sound test visualization tab.
 * Provides visual feedback for sound testing with animated waveforms.
 *
 * @return GtkWidget containing sound test visualization UI
 */
GtkWidget *test_sound_tab_new(void);

#endif