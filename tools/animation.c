#include "animation.h"
#include <stdio.h>
#include <math.h>

static GHashTable *active_animations = NULL;

// Easing functions
static double ease_linear(double t) { return t; }
static double ease_in_quad(double t) { return t * t; }
static double ease_out_quad(double t) { return t * (2 - t); }
static double ease_in_out_quad(double t) { return t < 0.5 ? 2 * t * t : -1 + (4 - 2 * t) * t; }
static double ease_in_cubic(double t) { return t * t * t; }
static double ease_out_cubic(double t) { return 1 - pow(1 - t, 3); }
static double ease_in_out_cubic(double t) { return t < 0.5 ? 4 * t * t * t : 1 - pow(-2 * t + 2, 3) / 2; }
static double ease_out_bounce(double t) {
    if (t < (1/2.75)) {
        return 7.5625 * t * t;
    } else if (t < (2/2.75)) {
        t -= 1.5/2.75;
        return 7.5625 * t * t + 0.75;
    } else if (t < (2.5/2.75)) {
        t -= 2.25/2.75;
        return 7.5625 * t * t + 0.9375;
    } else {
        t -= 2.625/2.75;
        return 7.5625 * t * t + 0.984375;
    }
}
static double ease_out_elastic(double t) {
    if (t == 0 || t == 1) return t;
    double p = 0.3;
    double s = p / 4;
    return pow(2, -10 * t) * sin((t - s) * (2 * M_PI) / p) + 1;
}
static double ease_in_back(double t) {
    double c1 = 1.70158;
    double c3 = c1 + 1;
    return c3 * t * t * t - c1 * t * t;
}
static double ease_out_back(double t) {
    double c1 = 1.70158;
    double c3 = c1 + 1;
    return 1 + c3 * pow(t - 1, 3) + c1 * pow(t - 1, 2);
}
static double ease_in_out_back(double t) {
    double c1 = 1.70158;
    double c2 = c1 * 1.525;
    return t < 0.5
      ? (pow(2 * t, 2) * ((c2 + 1) * 2 * t - c2)) / 2
      : (pow(2 * t - 2, 2) * ((c2 + 1) * (t * 2 - 2) + c2) + 2) / 2;
}

double easing_calculate(double t, AnimationEasing easing) {
    switch (easing) {
        case ANIM_EASE_LINEAR: return ease_linear(t);
        case ANIM_EASE_IN_QUAD: return ease_in_quad(t);
        case ANIM_EASE_OUT_QUAD: return ease_out_quad(t);
        case ANIM_EASE_IN_OUT_QUAD: return ease_in_out_quad(t);
        case ANIM_EASE_IN_CUBIC: return ease_in_cubic(t);
        case ANIM_EASE_OUT_CUBIC: return ease_out_cubic(t);
        case ANIM_EASE_IN_OUT_CUBIC: return ease_in_out_cubic(t);
        case ANIM_EASE_OUT_BOUNCE: return ease_out_bounce(t);
        case ANIM_EASE_OUT_ELASTIC: return ease_out_elastic(t);
        case ANIM_EASE_IN_BACK: return ease_in_back(t);
        case ANIM_EASE_OUT_BACK: return ease_out_back(t);
        case ANIM_EASE_IN_OUT_BACK: return ease_in_out_back(t);
        default: return t;
    }
}

void animation_init(void) {
    if (!active_animations) {
        active_animations = g_hash_table_new(g_direct_hash, g_direct_equal);
    }
}

gboolean on_animation_tick(GtkWidget *widget, GdkFrameClock *frame_clock, gpointer user_data) {
    Animation *anim = (Animation *)user_data;
    if (!anim || !anim->active) return G_SOURCE_REMOVE;
    
    guint current_time = gdk_frame_clock_get_frame_time(frame_clock) / 1000; // Convert to ms
    guint elapsed = current_time - anim->start_time;
    
    if (elapsed >= anim->duration) {
        // Animation complete
        if (anim->target_width > 0 && anim->target_height > 0) {
            gtk_window_resize(GTK_WINDOW(anim->widget), anim->target_width, anim->target_height);
        }
        if (anim->target_x != -1 && anim->target_y != -1) {
            gtk_window_move(GTK_WINDOW(anim->widget), anim->target_x, anim->target_y);
        }
        if (anim->target_opacity >= 0) {
            gtk_widget_set_opacity(anim->widget, anim->target_opacity);
        }
        if (anim->target_scale > 0) {
            GtkCssProvider *provider = gtk_css_provider_new();
            char css[256];
            snprintf(css, sizeof(css), 
                    "widget { transform: scale(%f); transition: all 200ms ease; }", 
                    anim->target_scale);
            gtk_css_provider_load_from_data(provider, css, -1, NULL);
            gtk_style_context_add_provider(
                gtk_widget_get_style_context(anim->widget),
                GTK_STYLE_PROVIDER(provider),
                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
            g_object_unref(provider);
        }
        
        anim->active = FALSE;
        if (anim->on_complete) {
            anim->on_complete(anim->complete_data);
        }
        return G_SOURCE_REMOVE;
    }
    
    double progress = (double)elapsed / anim->duration;
    double eased = easing_calculate(progress, anim->easing);
    
    // Handle resize
    if (anim->target_width > 0 && anim->target_height > 0) {
        int current_width = anim->start_width + (anim->target_width - anim->start_width) * eased;
        int current_height = anim->start_height + (anim->target_height - anim->start_height) * eased;
        gtk_window_resize(GTK_WINDOW(anim->widget), current_width, current_height);
    }
    
    // Handle movement
    if (anim->target_x != -1 && anim->target_y != -1) {
        int current_x = anim->start_x + (anim->target_x - anim->start_x) * eased;
        int current_y = anim->start_y + (anim->target_y - anim->start_y) * eased;
        gtk_window_move(GTK_WINDOW(anim->widget), current_x, current_y);
    }
    
    // Handle opacity
    if (anim->target_opacity >= 0) {
        double current_opacity = anim->start_opacity + (anim->target_opacity - anim->start_opacity) * eased;
        gtk_widget_set_opacity(anim->widget, current_opacity);
    }
    
    // Handle scale with CSS
    if (anim->target_scale > 0) {
        double current_scale = anim->start_scale + (anim->target_scale - anim->start_scale) * eased;
        GtkCssProvider *provider = gtk_css_provider_new();
        char css[256];
        snprintf(css, sizeof(css), 
                "widget { transform: scale(%f); transition: transform %dms ease; }", 
                current_scale, anim->duration);
        gtk_css_provider_load_from_data(provider, css, -1, NULL);
        gtk_style_context_add_provider(
            gtk_widget_get_style_context(anim->widget),
            GTK_STYLE_PROVIDER(provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        g_object_unref(provider);
    }
    
    return G_SOURCE_CONTINUE;
}

gboolean on_morph_tick(GtkWidget *widget, GdkFrameClock *frame_clock, gpointer user_data) {
    Animation *anim = (Animation *)user_data;
    if (!anim || !anim->active) return G_SOURCE_REMOVE;
    
    guint current_time = gdk_frame_clock_get_frame_time(frame_clock) / 1000;
    guint elapsed = current_time - anim->start_time;
    
    if (elapsed >= anim->duration) {
        anim->active = FALSE;
        if (anim->snapshot) {
            cairo_surface_destroy(anim->snapshot);
        }
        return G_SOURCE_REMOVE;
    }
    
    double progress = (double)elapsed / anim->duration;
    double eased = easing_calculate(progress, anim->easing);
    
    // Handle resize with morph effect
    if (anim->target_width > 0 && anim->target_height > 0) {
        int current_width = anim->start_width + (anim->target_width - anim->start_width) * eased;
        int current_height = anim->start_height + (anim->target_height - anim->start_height) * eased;
        gtk_widget_set_size_request(widget, current_width, current_height);
    }
    
    // Handle opacity
    if (anim->target_opacity >= 0) {
        double current_opacity = anim->start_opacity + (anim->target_opacity - anim->start_opacity) * eased;
        gtk_widget_set_opacity(widget, current_opacity);
    }
    
    // Queue redraw for morph effect
    gtk_widget_queue_draw(widget);
    
    return G_SOURCE_CONTINUE;
}

void animate_window_open(GtkWindow *window, int x, int y, int width, int height) {
    Animation *anim = g_new0(Animation, 1);
    
    // Start from center with tiny size
    int start_width = 1;
    int start_height = 1;
    int start_x = x + width/2;
    int start_y = y + height/2;
    
    gtk_window_move(window, start_x, start_y);
    gtk_window_resize(window, start_width, start_height);
    gtk_widget_set_opacity(GTK_WIDGET(window), 0.0);
    gtk_widget_show(GTK_WIDGET(window));
    
    anim->widget = GTK_WIDGET(window);
    anim->start_x = start_x;
    anim->start_y = start_y;
    anim->target_x = x;
    anim->target_y = y;
    anim->start_width = start_width;
    anim->start_height = start_height;
    anim->target_width = width;
    anim->target_height = height;
    anim->start_opacity = 0.0;
    anim->target_opacity = 1.0;
    anim->duration = 500;
    anim->start_time = g_get_monotonic_time() / 1000;
    anim->easing = ANIM_EASE_OUT_ELASTIC;
    anim->active = TRUE;
    
    gtk_widget_add_tick_callback(GTK_WIDGET(window), on_animation_tick, anim, g_free);
}

void animate_window_morph_open(GtkWindow *window, int target_x, int target_y, 
                               int target_width, int target_height,
                               int morph_x, int morph_y, int morph_width, int morph_height) {
    Animation *anim = g_new0(Animation, 1);
    
    gtk_window_move(window, morph_x, morph_y);
    gtk_window_resize(window, morph_width, morph_height);
    gtk_widget_set_opacity(GTK_WIDGET(window), 0.3);
    gtk_widget_show(GTK_WIDGET(window));
    
    anim->widget = GTK_WIDGET(window);
    anim->start_x = morph_x;
    anim->start_y = morph_y;
    anim->target_x = target_x;
    anim->target_y = target_y;
    anim->start_width = morph_width;
    anim->start_height = morph_height;
    anim->target_width = target_width;
    anim->target_height = target_height;
    anim->start_opacity = 0.3;
    anim->target_opacity = 1.0;
    anim->duration = 600;
    anim->start_time = g_get_monotonic_time() / 1000;
    anim->easing = ANIM_EASE_OUT_BACK;
    anim->active = TRUE;
    
    gtk_widget_add_tick_callback(GTK_WIDGET(window), on_animation_tick, anim, g_free);
}

void animate_window_morph_close(GtkWindow *window, int morph_x, int morph_y, 
                                int morph_width, int morph_height) {
    Animation *anim = g_new0(Animation, 1);
    
    int x, y, width, height;
    gtk_window_get_position(window, &x, &y);
    gtk_window_get_size(window, &width, &height);
    
    anim->widget = GTK_WIDGET(window);
    anim->start_x = x;
    anim->start_y = y;
    anim->target_x = morph_x + morph_width/2;
    anim->target_y = morph_y + morph_height/2;
    anim->start_width = width;
    anim->start_height = height;
    anim->target_width = morph_width;
    anim->target_height = morph_height;
    anim->start_opacity = 1.0;
    anim->target_opacity = 0.0;
    anim->duration = 400;
    anim->start_time = g_get_monotonic_time() / 1000;
    anim->easing = ANIM_EASE_IN_OUT_QUAD;
    anim->active = TRUE;
    anim->on_complete = (void (*)(gpointer))gtk_widget_destroy;
    anim->complete_data = window;
    
    gtk_widget_add_tick_callback(GTK_WIDGET(window), on_animation_tick, anim, NULL);
}

void animate_window_close(GtkWindow *window) {
    Animation *anim = g_new0(Animation, 1);
    
    int x, y, width, height;
    gtk_window_get_position(window, &x, &y);
    gtk_window_get_size(window, &width, &height);
    
    anim->widget = GTK_WIDGET(window);
    anim->start_x = x;
    anim->start_y = y;
    anim->target_x = x + width/2;
    anim->target_y = y + height/2;
    anim->start_width = width;
    anim->start_height = height;
    anim->target_width = 1;
    anim->target_height = 1;
    anim->start_opacity = 1.0;
    anim->target_opacity = 0.0;
    anim->duration = 300;
    anim->start_time = g_get_monotonic_time() / 1000;
    anim->easing = ANIM_EASE_IN_QUAD;
    anim->active = TRUE;
    anim->on_complete = (void (*)(gpointer))gtk_widget_destroy;
    anim->complete_data = window;
    
    gtk_widget_add_tick_callback(GTK_WIDGET(window), on_animation_tick, anim, NULL);
}

void animate_window_resize(GtkWindow *window, int target_width, int target_height, guint duration) {
    Animation *anim = g_new0(Animation, 1);
    
    int current_width, current_height;
    gtk_window_get_size(window, &current_width, &current_height);
    
    anim->widget = GTK_WIDGET(window);
    anim->start_width = current_width;
    anim->start_height = current_height;
    anim->target_width = target_width;
    anim->target_height = target_height;
    anim->duration = duration;
    anim->start_time = g_get_monotonic_time() / 1000;
    anim->easing = ANIM_EASE_OUT_CUBIC;
    anim->active = TRUE;
    
    gtk_widget_add_tick_callback(GTK_WIDGET(window), on_animation_tick, anim, g_free);
}

void animate_window_move(GtkWindow *window, int target_x, int target_y, guint duration) {
    Animation *anim = g_new0(Animation, 1);
    
    int current_x, current_y;
    gtk_window_get_position(window, &current_x, &current_y);
    
    anim->widget = GTK_WIDGET(window);
    anim->start_x = current_x;
    anim->start_y = current_y;
    anim->target_x = target_x;
    anim->target_y = target_y;
    anim->duration = duration;
    anim->start_time = g_get_monotonic_time() / 1000;
    anim->easing = ANIM_EASE_OUT_QUAD;
    anim->active = TRUE;
    
    gtk_widget_add_tick_callback(GTK_WIDGET(window), on_animation_tick, anim, g_free);
}

void animate_fade_in(GtkWidget *widget, guint duration) {
    Animation *anim = g_new0(Animation, 1);
    
    gtk_widget_show(widget);
    gtk_widget_set_opacity(widget, 0.0);
    
    anim->widget = widget;
    anim->start_opacity = 0.0;
    anim->target_opacity = 1.0;
    anim->duration = duration;
    anim->start_time = g_get_monotonic_time() / 1000;
    anim->easing = ANIM_EASE_OUT_QUAD;
    anim->active = TRUE;
    
    gtk_widget_add_tick_callback(widget, on_animation_tick, anim, g_free);
}

void animate_fade_out(GtkWidget *widget, guint duration) {
    Animation *anim = g_new0(Animation, 1);
    
    anim->widget = widget;
    anim->start_opacity = gtk_widget_get_opacity(widget);
    anim->target_opacity = 0.0;
    anim->duration = duration;
    anim->start_time = g_get_monotonic_time() / 1000;
    anim->easing = ANIM_EASE_IN_QUAD;
    anim->active = TRUE;
    anim->on_complete = (void (*)(gpointer))gtk_widget_hide;
    anim->complete_data = widget;
    
    gtk_widget_add_tick_callback(widget, on_animation_tick, anim, NULL);
}

void animate_fade_in_with_scale(GtkWidget *widget, guint duration) {
    Animation *anim = g_new0(Animation, 1);
    
    gtk_widget_show(widget);
    gtk_widget_set_opacity(widget, 0.0);
    
    // Apply initial scale via CSS
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, "widget { transform: scale(0.5); }", -1, NULL);
    gtk_style_context_add_provider(
        gtk_widget_get_style_context(widget),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
    
    anim->widget = widget;
    anim->start_opacity = 0.0;
    anim->target_opacity = 1.0;
    anim->start_scale = 0.5;
    anim->target_scale = 1.0;
    anim->duration = duration;
    anim->start_time = g_get_monotonic_time() / 1000;
    anim->easing = ANIM_EASE_OUT_ELASTIC;
    anim->active = TRUE;
    
    gtk_widget_add_tick_callback(widget, on_animation_tick, anim, g_free);
}

void animate_fade_out_with_scale(GtkWidget *widget, guint duration) {
    Animation *anim = g_new0(Animation, 1);
    
    anim->widget = widget;
    anim->start_opacity = gtk_widget_get_opacity(widget);
    anim->target_opacity = 0.0;
    anim->start_scale = 1.0;
    anim->target_scale = 0.5;
    anim->duration = duration;
    anim->start_time = g_get_monotonic_time() / 1000;
    anim->easing = ANIM_EASE_IN_BACK;
    anim->active = TRUE;
    anim->on_complete = (void (*)(gpointer))gtk_widget_hide;
    anim->complete_data = widget;
    
    gtk_widget_add_tick_callback(widget, on_animation_tick, anim, NULL);
}