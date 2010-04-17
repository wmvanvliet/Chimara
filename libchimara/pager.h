#ifndef PAGER_H
#define PAGER_H

#include <gtk/gtk.h>
#include "glk.h"
#include "window.h"

G_GNUC_INTERNAL gboolean pager_on_expose(GtkTextView *textview, GdkEventExpose *event, winid_t win);
G_GNUC_INTERNAL gboolean pager_on_key_press_event(GtkTextView *textview, GdkEventKey *event, winid_t win);
G_GNUC_INTERNAL void pager_after_insert_text(GtkTextBuffer *buffer, GtkTextIter *location, gchar *text, gint len, winid_t win);
G_GNUC_INTERNAL void pager_after_adjustment_changed(GtkAdjustment *adj, winid_t win);

#endif
