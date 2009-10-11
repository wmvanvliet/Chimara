/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * main.c
 * Copyright (C) Philip en Marijn 2008 <>
 * 
 * main.c is free software copyrighted by Philip en Marijn.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name ``Philip en Marijn'' nor the name of any other
 *    contributor may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * 
 * main.c IS PROVIDED BY Philip en Marijn ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Philip en Marijn OR ANY OTHER CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "callbacks.h"
#include "error.h"
#include <libchimara/chimara-glk.h>

/* Global pointers to widgets */
GtkBuilder *builder = NULL;
GtkUIManager *uimanager = NULL;
GtkWidget *window = NULL;
GtkWidget *glk = NULL;

static void
on_started(ChimaraGlk *glk)
{
    g_printerr("Started!\n");
}

static void
on_stopped(ChimaraGlk *glk)
{
    g_printerr("Stopped!\n");
}

static void
on_char_input(ChimaraGlk *glk, guint32 window_rock, guint keysym)
{
	g_printerr("Character input in window %d: key %d\n", window_rock, keysym);
}

static void
on_line_input(ChimaraGlk *glk, guint32 window_rock, gchar *text)
{
	g_printerr("Line input in window %d: '%s'\n", window_rock, text);
}

static void
on_text_buffer_output(ChimaraGlk *glk, guint32 window_rock, gchar *text)
{
	g_printerr("Text buffer output in window %d: '%s'\n", window_rock, text);
}

static GObject *
load_object(const gchar *name)
{
	GObject *retval;
	if( (retval = gtk_builder_get_object(builder, name)) == NULL) {
		error_dialog(NULL, NULL, "Error while getting object '%s'", name);
		g_error("Error while getting object '%s'", name);
	}
	return retval;
}

static void
create_window(void)
{
	GError *error = NULL;

   	builder = gtk_builder_new();
	if( !gtk_builder_add_from_file(builder, "chimara.ui", &error) ) {
		error_dialog(NULL, error, "Error while building interface: ");	
		return;
	}

	window = GTK_WIDGET(load_object("chimara"));
	GtkActionGroup *actiongroup = GTK_ACTION_GROUP(load_object("actiongroup"));

	/* Add all the actions to the action group. This for-loop is a temporary fix
	and can be removed once Glade supports adding actions and accelerators to an
	action group. */
	const gchar *actions[] = { 
		"game", "",
		"open", "<ctrl>F7", 
		"save", NULL, /* NULL means use stock accelerator */
		"quit", NULL,
		NULL
	};
	const gchar **ptr;
	for(ptr = actions; *ptr; ptr += 2)
		gtk_action_group_add_action_with_accel(actiongroup, GTK_ACTION(load_object(ptr[0])), ptr[1]);

	uimanager = gtk_ui_manager_new();
	if( !gtk_ui_manager_add_ui_from_file(uimanager, "chimara.menus", &error) ) {
		error_dialog(NULL, error, "Error while building interface: ");
		return;
	}
	
	gtk_builder_connect_signals(builder, NULL);
	
	glk = chimara_glk_new();
	g_object_set(glk, "border-width", 6, "spacing", 6, NULL);
	chimara_glk_set_default_font_string(CHIMARA_GLK(glk), "Serif 12");
	chimara_glk_set_monospace_font_string(CHIMARA_GLK(glk), "Monospace 12");
	g_signal_connect(glk, "started", G_CALLBACK(on_started), NULL);
	g_signal_connect(glk, "stopped", G_CALLBACK(on_stopped), NULL);
	g_signal_connect(glk, "char-input", G_CALLBACK(on_char_input), NULL);
	g_signal_connect(glk, "line-input", G_CALLBACK(on_line_input), NULL);
	g_signal_connect(glk, "text-buffer-output", G_CALLBACK(on_text_buffer_output), NULL);
	
	GtkBox *vbox = GTK_BOX( gtk_builder_get_object(builder, "vbox") );			
	if(vbox == NULL)
	{
		error_dialog(NULL, NULL, "Could not find vbox");
		return;
	}

	gtk_ui_manager_insert_action_group(uimanager, actiongroup, 0);
	GtkWidget *menubar = gtk_ui_manager_get_widget(uimanager, "/menubar");
	GtkWidget *toolbar = gtk_ui_manager_get_widget(uimanager, "/toolbar");

	gtk_box_pack_end(vbox, glk, TRUE, TRUE, 0);
	gtk_box_pack_start(vbox, menubar, FALSE, FALSE, 0);
	gtk_box_pack_start(vbox, toolbar, FALSE, FALSE, 0);
}

int
main(int argc, char *argv[])
{
	GError *error = NULL;

#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);
#endif

	if( !g_thread_supported() )
		g_thread_init(NULL);
	gdk_threads_init();
	gtk_init(&argc, &argv);

	create_window();
	gtk_widget_show_all(window);

	g_object_unref( G_OBJECT(builder) );
	g_object_unref( G_OBJECT(uimanager) );

    if( !chimara_glk_run(CHIMARA_GLK(glk), "../interpreters/frotz/.libs/frotz.so", argc, argv, &error) ) {
   		error_dialog(GTK_WINDOW(window), error, "Error starting Glk library: ");
		return 1;
	}

    gdk_threads_enter();
	gtk_main();
	gdk_threads_leave();

	chimara_glk_stop(CHIMARA_GLK(glk));
	chimara_glk_wait(CHIMARA_GLK(glk));

	return 0;
}
