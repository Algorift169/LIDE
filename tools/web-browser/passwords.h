#ifndef VOIDFOX_PASSWORDS_H
#define VOIDFOX_PASSWORDS_H

#include <gtk/gtk.h>
#include "voidfox.h"

/**
 * Password entry structure.
 * Represents a saved credential for a website.
 */
typedef struct {
    char *site;      /* Website domain (e.g., example.com) */
    char *username;  /* Username or email for the site */
    char *password;  /* Password for the site (stored in plaintext) */
} PasswordEntry;

/* Function prototypes */

/**
 * Creates and displays the saved passwords tab.
 * Shows all stored credentials with options to show/hide and copy passwords.
 *
 * @param browser BrowserWindow instance.
 *
 * @sideeffect Adds a new tab to the notebook with password list.
 */
void show_passwords_tab(BrowserWindow *browser);

/**
 * Adds a password entry to the list.
 *
 * @param site     Website domain.
 * @param username Username or email for the site.
 * @param password Password for the site.
 *
 * @sideeffect Appends to global passwords list and saves to file.
 */
void add_password(const char *site, const char *username, const char *password);

/**
 * Saves all passwords to disk.
 * Format: site|username|password per line.
 *
 * @sideeffect Writes to PASSWORDS_FILE in the current directory.
 */
void save_passwords(void);

/**
 * Loads passwords from disk.
 * Reads PASSWORDS_FILE and populates the passwords list.
 *
 * @sideeffect Appends loaded passwords to global passwords list.
 * @note Call during application initialization to restore saved credentials.
 */
void load_passwords(void);

#endif /* VOIDFOX_PASSWORDS_H */