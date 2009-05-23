#ifndef WINDOW_H
#define WINDOW_H

#include <gtk/gtk.h>
#include "glk.h"

#include "stream.h"
#include "input.h"
#include "style.h"


enum InputRequestType
{
	INPUT_REQUEST_NONE,
	INPUT_REQUEST_CHARACTER,
	INPUT_REQUEST_CHARACTER_UNICODE,
	INPUT_REQUEST_LINE,
	INPUT_REQUEST_LINE_UNICODE
};

/**
 * glk_window_struct:
 *
 * This is an opaque structure (see <link linkend="chimara-Opaque-Structures">
 * Opaque Structures</link> and should not be accessed directly.
 */
struct glk_window_struct
{
	/*< private >*/
	glui32 magic, rock;
	/* Pointer to the node in the global tree that contains this window */
	GNode *window_node;
	/* Window parameters */
	glui32 type;
	/* "widget" is the actual widget with the window's functionality */
	GtkWidget *widget;
	/* "frame" is the widget that is the child of the ChimaraGlk container, such 
	as a scroll window. It may be the same as "widget". */
	GtkWidget *frame;
	/* Width and height of the window's size units, in pixels */
	int unit_width;
	int unit_height;
	/* Streams associated with the window */
	strid_t window_stream;
	strid_t echo_stream;
	/* Width and height of the window, in characters (text grids only) */
	glui32 width;
	glui32 height;
	/* Window split data (pair windows only) */
	winid_t key_window;
	glui32 split_method;
	glui32 constraint_size;
	/* Input request stuff */
	enum InputRequestType input_request_type;
	gchar *line_input_buffer;
	glui32 *line_input_buffer_unicode;
	glui32 line_input_buffer_max_len;
	gboolean mouse_input_requested;
	/* Line input field (text grids only) */
	glui32 input_length;
	GtkTextChildAnchor *input_anchor;
	GtkWidget *input_entry;
	/* Signal handlers */
	gulong keypress_handler;
	gulong insert_text_handler;
};

#endif
