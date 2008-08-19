#ifndef INPUT_H
#define INPUT_H

#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>

#include "window.h"
#include "event.h"

gboolean on_window_key_press_event(GtkWidget *widget, GdkEventKey *event, winid_t window);
void on_window_insert_text(GtkTextBuffer *textbuffer, GtkTextIter *location, gchar *text, gint len, gpointer user_data);
void after_window_insert_text(GtkTextBuffer *textbuffer, GtkTextIter *location, gchar *text, gint len, winid_t window);
#endif
