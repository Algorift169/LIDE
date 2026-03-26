#ifndef BLACKLINE_CALCULATOR_H
#define BLACKLINE_CALCULATOR_H

#include <gtk/gtk.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_E
#define M_E 2.71828182845904523536
#endif

/**
 * Calculator operation modes.
 * Determines which button set is displayed.
 */
typedef enum {
    MODE_BASIC,      /* Standard arithmetic operations only */
    MODE_SCIENTIFIC  /* Extended functions (trig, log, power) */
} CalculatorMode;

/**
 * Calculator state structure.
 * Encapsulates all calculator data and UI components.
 */
typedef struct {
    GtkWidget *window;           /* Main application window */
    GtkWidget *display;          /* Label showing current input/result */
    GtkWidget *history_display;  /* Label showing calculation history */
    GtkWidget *mode_button;      /* Button to toggle calculator mode */
    GtkWidget *button_grid;      /* Container for calculator buttons */

    char current_input[256];     /* Current input or result string */
    char last_result[64];        /* Last computed result */
    char memory[64];             /* Stored memory value */
    double last_value;           /* Previous value for binary operations */
    char last_operator;          /* Last operator for binary operations */
    gboolean new_input;          /* TRUE if next digit should start fresh input */
    gboolean error_state;        /* TRUE if in error state (division by zero, etc.) */
    CalculatorMode mode;         /* Current calculator mode */

    /* Window dragging state */
    int is_dragging;             /* TRUE while user is dragging the window */
    int drag_start_x;            /* Initial mouse X position for drag operation */
    int drag_start_y;            /* Initial mouse Y position for drag operation */

    /* Window resizing state */
    int is_resizing;             /* TRUE while user is resizing the window */
    int resize_edge;             /* Which edge/corner is being resized (from window_resize.h) */
} Calculator;

/* Number input callbacks */
void number_clicked(GtkButton *button, gpointer data);
void operator_clicked(GtkButton *button, gpointer data);
void function_clicked(GtkButton *button, gpointer data);
void equals_clicked(GtkButton *button, gpointer data);
void clear_clicked(GtkButton *button, gpointer data);
void backspace_clicked(GtkButton *button, gpointer data);
void memory_clicked(GtkButton *button, gpointer data);
void toggle_mode(GtkButton *button, gpointer data);

/* Window manipulation callbacks */
gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data);
gboolean on_button_release(GtkWidget *widget, GdkEventButton *event, gpointer data);
gboolean on_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer data);

#endif /* BLACKLINE_CALCULATOR_H */