#ifndef BLACKLINE_EDIT_H
#define BLACKLINE_EDIT_H

#include <gtk/gtk.h>

/**
 * edit.h
 * 
 * Document editing interface definitions.
 *
 * This module is part of the LIDE desktop environment system.
 * See the main window manager (wm/) and session management (session/)
 * for system architecture overview.
 */

/**
 * Edit operation types enumeration.
 * Identifies the type of edit operation for potential future extensions.
 */
typedef enum {
    EDIT_UNDO,           /* Undo last operation */
    EDIT_REDO,           /* Redo last undone operation */
    EDIT_CUT,            /* Cut selected text to clipboard */
    EDIT_COPY,           /* Copy selected text to clipboard */
    EDIT_PASTE,          /* Paste text from clipboard */
    EDIT_DELETE,         /* Delete selected text */
    EDIT_SELECT_ALL,     /* Select all text in document */
    EDIT_FIND,           /* Open find dialog */
    EDIT_REPLACE,        /* Open replace dialog */
    EDIT_GOTO_LINE,      /* Jump to specific line number */
    EDIT_COMMENT_TOGGLE, /* Toggle line comment (//) */
    EDIT_INDENT,         /* Indent line or selection */
    EDIT_UNINDENT,       /* Unindent line or selection */
    EDIT_UPPERCASE,      /* Convert selected text to uppercase */
    EDIT_LOWERCASE,      /* Convert selected text to lowercase */
    EDIT_CAPITALIZE,     /* Capitalize first letter of each word */
    EDIT_DUPLICATE_LINE, /* Duplicate current line */
    EDIT_DELETE_LINE,    /* Delete current line */
    EDIT_JOIN_LINES,     /* Join multiple lines into one */
    EDIT_SORT_LINES      /* Sort selected lines alphabetically */
} EditOperation;

/**
 * Find/Replace dialog data structure.
 * Holds all UI elements and state for find/replace operations.
 */
typedef struct {
    GtkWidget *dialog;           /* Find/Replace dialog widget */
    GtkWidget *find_entry;       /* Entry for search text */
    GtkWidget *replace_entry;    /* Entry for replacement text (replace dialog only) */
    GtkWidget *match_case_check; /* Checkbox for case-sensitive matching */
    GtkWidget *whole_word_check; /* Checkbox for whole-word matching (find dialog only) */
    GtkTextBuffer *buffer;       /* Text buffer being searched */
    GtkWidget *window;           /* Parent window for dialog */
} FindReplaceData;

/**
 * Undo/Redo node structure.
 * Represents a single state in the undo/redo history stack.
 */
typedef struct EditNode {
    gchar *text;                 /* Full buffer text at this state */
    GtkTextMark *start_mark;     /* Mark for start of selection/position */
    GtkTextMark *end_mark;       /* Mark for end of selection/position */
    struct EditNode *next;       /* Next state in history (forward direction) */
    struct EditNode *prev;       /* Previous state in history (backward direction) */
} EditNode;

/**
 * Edit history manager structure.
 * Manages undo/redo stack with size limits.
 */
typedef struct {
    EditNode *current;           /* Current state in history */
    EditNode *head;              /* Oldest state in history */
    EditNode *tail;              /* Newest state in history */
    gint max_stack_size;         /* Maximum number of states to retain */
    gint current_size;           /* Current number of states in stack */
} EditHistory;

/* Function prototypes - EDITING OPERATIONS */

/**
 * Initializes edit features for a text buffer.
 * Sets up undo history and connects change signal.
 *
 * @param buffer The GtkTextBuffer to initialize.
 */
void edit_init(GtkTextBuffer *buffer);

/**
 * Undoes the last text edit operation.
 *
 * @param widget Unused (callback compatibility).
 * @param data   GtkTextView to operate on.
 */
void edit_undo(GtkWidget *widget, gpointer data);

/**
 * Redoes the last undone text edit operation.
 *
 * @param widget Unused (callback compatibility).
 * @param data   GtkTextView to operate on.
 */
void edit_redo(GtkWidget *widget, gpointer data);

/**
 * Cuts the selected text to clipboard.
 *
 * @param widget Unused (callback compatibility).
 * @param data   GtkTextView to operate on.
 */
void edit_cut(GtkWidget *widget, gpointer data);

/**
 * Copies the selected text to clipboard.
 *
 * @param widget Unused (callback compatibility).
 * @param data   GtkTextView to operate on.
 */
void edit_copy(GtkWidget *widget, gpointer data);

/**
 * Pastes text from clipboard at cursor position.
 *
 * @param widget Unused (callback compatibility).
 * @param data   GtkTextView to operate on.
 */
void edit_paste(GtkWidget *widget, gpointer data);

/**
 * Deletes the selected text.
 *
 * @param widget Unused (callback compatibility).
 * @param data   GtkTextView to operate on.
 */
void edit_delete(GtkWidget *widget, gpointer data);

/**
 * Selects all text in the document.
 *
 * @param widget Unused (callback compatibility).
 * @param data   GtkTextView to operate on.
 */
void edit_select_all(GtkWidget *widget, gpointer data);

/* Search operations */

/**
 * Opens the find dialog.
 *
 * @param widget Unused (callback compatibility).
 * @param data   GtkTextView to operate on.
 */
void edit_find(GtkWidget *widget, gpointer data);

/**
 * Opens the replace dialog.
 *
 * @param widget Unused (callback compatibility).
 * @param data   GtkTextView to operate on.
 */
void edit_replace(GtkWidget *widget, gpointer data);

/**
 * Opens the Go to Line dialog.
 *
 * @param widget Unused (callback compatibility).
 * @param data   GtkTextView to operate on.
 */
void edit_goto_line(GtkWidget *widget, gpointer data);

/* Text transformation */

/**
 * Converts selected text to uppercase.
 *
 * @param widget Unused (callback compatibility).
 * @param data   GtkTextView to operate on.
 */
void edit_to_uppercase(GtkWidget *widget, gpointer data);

/**
 * Converts selected text to lowercase.
 *
 * @param widget Unused (callback compatibility).
 * @param data   GtkTextView to operate on.
 */
void edit_to_lowercase(GtkWidget *widget, gpointer data);

/**
 * Capitalizes the first letter of each word in the selected text.
 *
 * @param widget Unused (callback compatibility).
 * @param data   GtkTextView to operate on.
 */
void edit_capitalize(GtkWidget *widget, gpointer data);

/* Line operations */

/**
 * Toggles C-style line comments (//) on the current line or selection.
 *
 * @param widget Unused (callback compatibility).
 * @param data   GtkTextView to operate on.
 */
void edit_toggle_comment(GtkWidget *widget, gpointer data);

/**
 * Indents the current line or selection by inserting 4 spaces.
 *
 * @param widget Unused (callback compatibility).
 * @param data   GtkTextView to operate on.
 */
void edit_indent(GtkWidget *widget, gpointer data);

/**
 * Unindents the current line or selection by removing leading spaces/tabs.
 *
 * @param widget Unused (callback compatibility).
 * @param data   GtkTextView to operate on.
 */
void edit_unindent(GtkWidget *widget, gpointer data);

/**
 * Duplicates the current line.
 *
 * @param widget Unused (callback compatibility).
 * @param data   GtkTextView to operate on.
 */
void edit_duplicate_line(GtkWidget *widget, gpointer data);

/**
 * Deletes the current line.
 *
 * @param widget Unused (callback compatibility).
 * @param data   GtkTextView to operate on.
 */
void edit_delete_line(GtkWidget *widget, gpointer data);

/**
 * Joins multiple lines into a single line.
 *
 * @param widget Unused (callback compatibility).
 * @param data   GtkTextView to operate on.
 */
void edit_join_lines(GtkWidget *widget, gpointer data);

/**
 * Sorts the selected lines alphabetically.
 *
 * @param widget Unused (callback compatibility).
 * @param data   GtkTextView to operate on.
 */
void edit_sort_lines(GtkWidget *widget, gpointer data);

/* Print */

/**
 * Opens the print dialog and prints the document.
 *
 * @param widget Unused (callback compatibility).
 * @param data   GtkTextView to print.
 */
void edit_print(GtkWidget *widget, gpointer data);

#endif /* BLACKLINE_EDIT_H */