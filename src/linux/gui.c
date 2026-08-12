// GTK view implementation: fullscreen window showing the countdown + keyboard input.
#include "../view.h"
#include "../rest.h"
#include <gtk/gtk.h>

static RestCore  *g_core   = NULL;
static GtkWidget *g_window = NULL;
static GtkWidget *g_label  = NULL;

// Update the large countdown number
static void update_label(int seconds) {
    char buffer[16];
    char *markup;
    snprintf(buffer, sizeof(buffer), "%d", seconds);
    markup = g_strdup_printf("<span font='220' weight='bold' foreground='white'>%s</span>", buffer);
    gtk_label_set_markup(GTK_LABEL(g_label), markup);
    g_free(markup);
}

// GTK key press: convert to a character and feed the core
static gboolean on_key_pressed(GtkEventControllerKey *controller,
                               guint keyval, guint keycode,
                               GdkModifierType state,
                               gpointer user_data) {
    guint32 uni = gdk_keyval_to_unicode(keyval);
    if (uni != 0 && uni < 128) {
        rest_core_send_key(g_core, (char)uni);
        return TRUE;
    }
    return FALSE;
}

// --- view interface implementation ---
void view_init(RestCore *core) {
    GtkCssProvider *provider;
    GtkEventController *controller;

    g_core = core;
    gtk_init();

    g_window = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(g_window), "Have a break");
    gtk_window_set_default_size(GTK_WINDOW(g_window), 800, 600);

    provider = gtk_css_provider_new();
    gtk_css_provider_load_from_string(provider, "window { background-color: red; }");
    gtk_style_context_add_provider_for_display(gdk_display_get_default(),
                                               GTK_STYLE_PROVIDER(provider),
                                               GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);

    g_label = gtk_label_new(NULL);
    gtk_window_set_child(GTK_WINDOW(g_window), g_label);

    controller = gtk_event_controller_key_new();
    g_signal_connect(controller, "key-pressed", G_CALLBACK(on_key_pressed), NULL);
    gtk_widget_add_controller(g_window, controller);
}

void view_rest_begin(int seconds) {
    gtk_widget_set_visible(g_window, TRUE);
    gtk_window_fullscreen(GTK_WINDOW(g_window));
    gtk_window_present(GTK_WINDOW(g_window));
    update_label(seconds);
}

void view_tick(int seconds) {
    update_label(seconds);
}

void view_work_begin(void) {
    gtk_widget_set_visible(g_window, FALSE);
}

void view_destroy(void) {
    if (g_window) {
        gtk_window_destroy(GTK_WINDOW(g_window));
        g_window = NULL;
    }
}
