#ifndef ANIMATION_H
#define ANIMATION_H

#include <gtk/gtk.h>
#include <math.h>
#include <cairo.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef enum {
    ANIM_EASE_LINEAR,
    ANIM_EASE_IN_QUAD,
    ANIM_EASE_OUT_QUAD,
    ANIM_EASE_IN_OUT_QUAD,
    ANIM_EASE_OUT_BOUNCE,
    ANIM_EASE_OUT_ELASTIC,
    ANIM_EASE_IN_BACK,
    ANIM_EASE_OUT_BACK,
    ANIM_EASE_IN_OUT_BACK,
    ANIM_EASE_IN_CUBIC,
    ANIM_EASE_OUT_CUBIC,
    ANIM_EASE_IN_OUT_CUBIC
} AnimationEasing;

typedef struct {
    GtkWidget *widget;
    int start_x, start_y;
    int target_x, target_y;
    int start_width, start_height;
    int target_width, target_height;
    double start_opacity;
    double target_opacity;
    double start_scale;
    double target_scale;
    guint duration;
    guint start_time;
    AnimationEasing easing;
    gboolean active;
    void (*on_complete)(gpointer data);
    gpointer complete_data;
    cairo_surface_t *snapshot;
    GtkCssProvider *scale_provider;
} Animation;

// Initialize animation system
void animation_init(void);

// Window animations
void animate_window_open(GtkWindow *window, int x, int y, int width, int height);
void animate_window_close(GtkWindow *window);
void animate_window_morph_open(GtkWindow *window, int target_x, int target_y, 
                               int target_width, int target_height,
                               int morph_x, int morph_y, int morph_width, int morph_height);
void animate_window_morph_close(GtkWindow *window, int morph_x, int morph_y, 
                                int morph_width, int morph_height);
void animate_window_resize(GtkWindow *window, int target_width, int target_height, guint duration);
void animate_window_move(GtkWindow *window, int target_x, int target_y, guint duration);

// Fade animations
void animate_fade_in(GtkWidget *widget, guint duration);
void animate_fade_out(GtkWidget *widget, guint duration);
void animate_fade_in_with_scale(GtkWidget *widget, guint duration);
void animate_fade_out_with_scale(GtkWidget *widget, guint duration);

// Morph animations
gboolean on_morph_tick(GtkWidget *widget, GdkFrameClock *frame_clock, gpointer user_data);

// Helper functions
double easing_calculate(double t, AnimationEasing easing);
gboolean on_animation_tick(GtkWidget *widget, GdkFrameClock *frame_clock, gpointer user_data);

#endif