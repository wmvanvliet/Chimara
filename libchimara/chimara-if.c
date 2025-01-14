#include <glib-object.h>

#include "chimara-if.h"
#include "chimara-glk.h"
#include "chimara-glk-private.h"
#include "chimara-marshallers.h"
#include "init.h"

#ifndef PLUGINDIR
#define PLUGINDIR "."
#endif

/**
 * SECTION:chimara-if
 * @short_description: Widget which plays an interactive fiction game
 * @stability: Unstable
 *
 * The #ChimaraIF widget, given an interactive fiction game file to run, selects
 * an appropriate interpreter plugin and runs it. Interpreter options are set by
 * setting properties on the widget.
 *
 * Using it in a GTK program is similar to using #ChimaraGlk (which see).
 * Use chimara_if_run_game() to start playing an interactive fiction game.
 */

static gboolean supported_formats[CHIMARA_IF_NUM_FORMATS][CHIMARA_IF_NUM_INTERPRETERS] = {
	/* Frotz Nitfol Glulxe Git    Bocfel */
	{ TRUE,  TRUE,  FALSE, FALSE, TRUE  }, /* Z5 */
	{ TRUE,  TRUE,  FALSE, FALSE, TRUE  }, /* Z6 */
	{ TRUE,  TRUE,  FALSE, FALSE, TRUE  }, /* Z8 */
	{ TRUE,  TRUE,  FALSE, FALSE, TRUE  }, /* Zblorb */
	{ FALSE, FALSE, TRUE,  TRUE,  FALSE }, /* Glulx */
	{ FALSE, FALSE, TRUE,  TRUE,  FALSE }  /* Gblorb */
};
static gchar *format_names[CHIMARA_IF_NUM_FORMATS] = {
	"Z-code version 5",
	"Z-code version 6",
	"Z-code version 8",
	"Blorbed Z-code",
	"Glulx",
	"Blorbed Glulx"
};
static gchar *interpreter_names[CHIMARA_IF_NUM_INTERPRETERS] = {
	"Frotz", "Nitfol", "Glulxe", "Git", "Bocfel"
};
static gchar *plugin_names[CHIMARA_IF_NUM_INTERPRETERS] = {
	"frotz", "nitfol", "glulxe", "git", "bocfel"
};

typedef enum _ChimaraIFFlags {
	CHIMARA_IF_PIRACY_MODE = 1 << 0,
	CHIMARA_IF_TANDY_BIT = 1 << 1,
	CHIMARA_IF_EXPAND_ABBREVIATIONS = 1 << 2,
	CHIMARA_IF_IGNORE_ERRORS = 1 << 3,
	CHIMARA_IF_TYPO_CORRECTION = 1 << 4
} ChimaraIFFlags;

typedef struct _ChimaraIFPrivate {
	ChimaraIFInterpreter preferred_interpreter[CHIMARA_IF_NUM_FORMATS];
	ChimaraIFFormat format;
	ChimaraIFInterpreter interpreter;
	ChimaraIFFlags flags;
	ChimaraIFZmachineVersion interpreter_number;
	gint random_seed;
	gboolean random_seed_set;
	gchar *graphics_file;
	/* Holding buffers for inputs and responses */
	GHashTable *active_inputs;
	GSList *window_librock_list;
} ChimaraIFPrivate;

typedef struct {
	char *input;
	GString *response;
	gboolean active;
} InputResponse;

enum {
	PROP_0,
	PROP_PIRACY_MODE,
	PROP_TANDY_BIT,
	PROP_EXPAND_ABBREVIATIONS,
	PROP_IGNORE_ERRORS,
	PROP_TYPO_CORRECTION,
	PROP_INTERPRETER_NUMBER,
	PROP_RANDOM_SEED,
	PROP_RANDOM_SEED_SET,
	PROP_GRAPHICS_FILE
};

enum {
	COMMAND,
	LAST_SIGNAL
};

static guint chimara_if_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE(ChimaraIF, chimara_if, CHIMARA_TYPE_GLK);

static InputResponse *
input_response_new(void)
{
	InputResponse *retval = g_slice_new0(InputResponse);
	retval->response = g_string_new("");
	return retval;
}

static void
input_response_destroy(InputResponse *inp)
{
	g_free(inp->input);
	g_string_free(inp->response, TRUE);
	g_slice_free(InputResponse, inp);
}

static InputResponse *
ensure_input_response_object(ChimaraIFPrivate *priv, char *string_id)
{
	InputResponse *retval = g_hash_table_lookup(priv->active_inputs, string_id);
	if(retval == NULL)
	{
		char *new_key = g_strdup(string_id);
		priv->window_librock_list = g_slist_prepend(priv->window_librock_list, new_key);
		retval = input_response_new();
		g_hash_table_insert(priv->active_inputs, new_key, retval);
	}
	return retval;
}

static void
emit_command_signal_on_active_inputs(char *window_librock, ChimaraGlk *glk)
{
	ChimaraIFPrivate *priv = chimara_if_get_instance_private(CHIMARA_IF(glk));
	InputResponse *inp = g_hash_table_lookup(priv->active_inputs, window_librock);
	if(!inp->active)
		return;

	char *response = g_strdup(inp->response->str);
	g_string_truncate(inp->response, 0);

	g_signal_emit_by_name(glk, "command", inp->input, response);

	g_free(inp->input);
	g_free(response);
	inp->input = NULL;
	inp->active = FALSE;
}

static void
chimara_if_waiting(ChimaraGlk *glk)
{
	ChimaraIFPrivate *priv = chimara_if_get_instance_private(CHIMARA_IF(glk));
	g_slist_foreach(priv->window_librock_list, (GFunc)emit_command_signal_on_active_inputs, glk);
}

static void
chimara_if_stopped(ChimaraGlk *glk)
{
	ChimaraIFPrivate *priv = chimara_if_get_instance_private(CHIMARA_IF(glk));

	/* Send one last command signal for any active inputs */
	g_slist_foreach(priv->window_librock_list, (GFunc)emit_command_signal_on_active_inputs, glk);

	priv->format = CHIMARA_IF_FORMAT_NONE;
	priv->interpreter = CHIMARA_IF_INTERPRETER_NONE;
}

static void
chimara_if_char_input(ChimaraGlk *glk, guint32 win_rock, char *string_id, unsigned keysym)
{
	ChimaraIFPrivate *priv = chimara_if_get_instance_private(CHIMARA_IF(glk));
	InputResponse *inp = ensure_input_response_object(priv, string_id);
	g_assert(!inp->active);

	gchar outbuf[6];
	gint outbuflen = g_unichar_to_utf8(gdk_keyval_to_unicode(keysym), outbuf);
	inp->input = g_strndup(outbuf, outbuflen);
	inp->active = TRUE;
}

static void
chimara_if_line_input(ChimaraGlk *glk, guint32 win_rock, char *string_id, char *input)
{
	ChimaraIFPrivate *priv = chimara_if_get_instance_private(CHIMARA_IF(glk));
	InputResponse *inp = ensure_input_response_object(priv, string_id);
	g_assert(!inp->active);
	inp->input = g_strdup(input);
	inp->active = TRUE;
}

static void
chimara_if_text_buffer_output(ChimaraGlk *glk, guint32 win_rock, char *string_id, char *output)
{
	ChimaraIFPrivate *priv = chimara_if_get_instance_private(CHIMARA_IF(glk));
	InputResponse *inp = ensure_input_response_object(priv, string_id);
	g_string_append(inp->response, output);
	inp->active = TRUE;
}

static void
chimara_if_init(ChimaraIF *self)
{
	chimara_init(); /* This is a library entry point */

	ChimaraIFPrivate *priv = chimara_if_get_instance_private(self);
	priv->preferred_interpreter[CHIMARA_IF_FORMAT_Z5] = CHIMARA_IF_INTERPRETER_BOCFEL;
	priv->preferred_interpreter[CHIMARA_IF_FORMAT_Z6] = CHIMARA_IF_INTERPRETER_BOCFEL;
	priv->preferred_interpreter[CHIMARA_IF_FORMAT_Z8] = CHIMARA_IF_INTERPRETER_BOCFEL;
	priv->preferred_interpreter[CHIMARA_IF_FORMAT_Z_BLORB] = CHIMARA_IF_INTERPRETER_BOCFEL;
	priv->preferred_interpreter[CHIMARA_IF_FORMAT_GLULX] = CHIMARA_IF_INTERPRETER_GLULXE;
	priv->preferred_interpreter[CHIMARA_IF_FORMAT_GLULX_BLORB] = CHIMARA_IF_INTERPRETER_GLULXE;
	priv->format = CHIMARA_IF_FORMAT_NONE;
	priv->interpreter = CHIMARA_IF_INTERPRETER_NONE;
	priv->flags = CHIMARA_IF_TYPO_CORRECTION;
	priv->interpreter_number = CHIMARA_IF_ZMACHINE_DEFAULT;
	priv->active_inputs = g_hash_table_new_full(g_str_hash, g_str_equal,
		(GDestroyNotify)g_free, (GDestroyNotify)input_response_destroy);

	/* Connect to signals of ChimaraGlk parent */
	g_signal_connect(self, "stopped", G_CALLBACK(chimara_if_stopped), NULL);
	g_signal_connect(self, "waiting", G_CALLBACK(chimara_if_waiting), NULL);
	g_signal_connect(self, "char-input", G_CALLBACK(chimara_if_char_input), NULL);
	g_signal_connect(self, "line-input", G_CALLBACK(chimara_if_line_input), NULL);
	g_signal_connect(self, "text-buffer-output", G_CALLBACK(chimara_if_text_buffer_output), NULL);
}

#define PROCESS_FLAG(flags, flag, val) (flags) = (val)? (flags) | (flag) : (flags) & ~(flag)

static void
chimara_if_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	ChimaraIFPrivate *priv = chimara_if_get_instance_private(CHIMARA_IF(object));
    switch(prop_id)
    {
    	case PROP_PIRACY_MODE:
    		PROCESS_FLAG(priv->flags, CHIMARA_IF_PIRACY_MODE, g_value_get_boolean(value));
    		g_object_notify(object, "piracy-mode");
    		break;
    	case PROP_TANDY_BIT:
    		PROCESS_FLAG(priv->flags, CHIMARA_IF_TANDY_BIT, g_value_get_boolean(value));
    		g_object_notify(object, "tandy-bit");
    		break;
    	case PROP_EXPAND_ABBREVIATIONS:
    		PROCESS_FLAG(priv->flags, CHIMARA_IF_EXPAND_ABBREVIATIONS, g_value_get_boolean(value));
    		g_object_notify(object, "expand-abbreviations");
    		break;
    	case PROP_IGNORE_ERRORS:
    		PROCESS_FLAG(priv->flags, CHIMARA_IF_IGNORE_ERRORS, g_value_get_boolean(value));
    		g_object_notify(object, "ignore-errors");
    		break;
    	case PROP_TYPO_CORRECTION:
    		PROCESS_FLAG(priv->flags, CHIMARA_IF_TYPO_CORRECTION, g_value_get_boolean(value));
    		g_object_notify(object, "typo-correction");
    		break;
    	case PROP_INTERPRETER_NUMBER:
    		priv->interpreter_number = g_value_get_uint(value);
    		g_object_notify(object, "interpreter-number");
    		break;
    	case PROP_RANDOM_SEED:
    		priv->random_seed = g_value_get_int(value);
    		g_object_notify(object, "random-seed");
    		break;
    	case PROP_RANDOM_SEED_SET:
    		priv->random_seed_set = g_value_get_boolean(value);
    		g_object_notify(object, "random-seed-set");
    		break;
		case PROP_GRAPHICS_FILE:
			priv->graphics_file = g_strdup(g_value_get_string(value));
			g_object_notify(object, "graphics-file");
			break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void
chimara_if_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	ChimaraIFPrivate *priv = chimara_if_get_instance_private(CHIMARA_IF(object));
    switch(prop_id)
    {
    	case PROP_PIRACY_MODE:
    		g_value_set_boolean(value, priv->flags & CHIMARA_IF_PIRACY_MODE);
    		break;
    	case PROP_TANDY_BIT:
    		g_value_set_boolean(value, priv->flags & CHIMARA_IF_TANDY_BIT);
    		break;
    	case PROP_EXPAND_ABBREVIATIONS:
    		g_value_set_boolean(value, priv->flags & CHIMARA_IF_EXPAND_ABBREVIATIONS);
    		break;
    	case PROP_IGNORE_ERRORS:
    		g_value_set_boolean(value, priv->flags & CHIMARA_IF_IGNORE_ERRORS);
    		break;
    	case PROP_TYPO_CORRECTION:
    		g_value_set_boolean(value, priv->flags & CHIMARA_IF_TYPO_CORRECTION);
    		break;
    	case PROP_INTERPRETER_NUMBER:
    		g_value_set_uint(value, priv->interpreter_number);
    		break;
    	case PROP_RANDOM_SEED:
    		g_value_set_int(value, priv->random_seed);
    		break;
    	case PROP_RANDOM_SEED_SET:
    		g_value_set_boolean(value, priv->random_seed_set);
    		break;
		case PROP_GRAPHICS_FILE:
			g_value_set_string(value, priv->graphics_file);
			break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void
chimara_if_finalize(GObject *object)
{
	ChimaraIFPrivate *priv = chimara_if_get_instance_private(CHIMARA_IF(object));
	g_free(priv->graphics_file);
	g_hash_table_destroy(priv->active_inputs);
	g_slist_free(priv->window_librock_list); /* values are already freed in hashtable */
    G_OBJECT_CLASS(chimara_if_parent_class)->finalize(object);
}

static void
chimara_if_class_init(ChimaraIFClass *klass)
{
	/* Override methods of parent classes */
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	object_class->set_property = chimara_if_set_property;
	object_class->get_property = chimara_if_get_property;
	object_class->finalize = chimara_if_finalize;

	/* Signals */
	/**
	 * ChimaraIF::command:
	 * @self: The widget that received the signal
	 * @input: The command typed into the game, or %NULL
	 * @response: The game's response to the command
	 *
	 * Emitted once for each input-response cycle of an interactive fiction
	 * game. Note that games with nontraditional input systems (i.e. not all
	 * taking place in the same text buffer window) may confuse this signal.
	 *
	 * It may happen that @input is %NULL, in which case @response is not due to
	 * a user command, but contains the text printed at the beginning of the
	 * game, up until the first prompt.
	 */
	chimara_if_signals[COMMAND] = g_signal_new("command",
		G_OBJECT_CLASS_TYPE(klass), 0,
		G_STRUCT_OFFSET(ChimaraIFClass, command), NULL, NULL,
		_chimara_marshal_VOID__STRING_STRING,
		G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_STRING);

	/* Properties */
	/**
	 * ChimaraIF:piracy-mode:
	 *
	 * The Z-machine specification defines a facility for games to ask the
	 * interpreter they are running on whether this copy of the game is pirated.
	 * How the interpreter is supposed to magically determine that it is running
	 * pirate software is unclear, and so the majority of games and interpreters
	 * ignore this feature. Set this property to %TRUE if you want the
	 * interpreter to pretend it has detected a pirated game.
	 *
	 * Only affects Z-machine interpreters.
	 */
	g_object_class_install_property(object_class, PROP_PIRACY_MODE,
		g_param_spec_boolean("piracy-mode", "Piracy mode",
		"Pretend the game is pirated", FALSE,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_LAX_VALIDATION | G_PARAM_STATIC_STRINGS));
	/**
	 * ChimaraIF:tandy-bit:
	 *
	 * Some early Infocom games were sold by the Tandy Corporation. Setting this
	 * property to %TRUE changes the wording of some Version 3 Infocom games
	 * slightly, so as to be less offensive.
	 * See <http://www.ifarchive.org/if-archive/infocom/info/tandy_bits.html>.
	 *
	 * Only affects Z-machine interpreters.
	 */
	g_object_class_install_property(object_class, PROP_TANDY_BIT,
		g_param_spec_boolean("tandy-bit", "Tandy bit",
		"Censor certain Infocom games", FALSE,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_LAX_VALIDATION | G_PARAM_STATIC_STRINGS));
	/**
	 * ChimaraIF:expand-abbreviations:
	 *
	 * Most Z-machine games, in particular ones compiled with the Inform
	 * library, support the following one-letter abbreviations:
	 * <simplelist>
	 * <member>D &mdash; Down</member>
	 * <member>E &mdash; East</member>
	 * <member>G &mdash; aGain</member>
	 * <member>I &mdash; Inventory</member>
	 * <member>L &mdash; Look</member>
	 * <member>N &mdash; North</member>
	 * <member>O &mdash; Oops</member>
	 * <member>Q &mdash; Quit</member>
	 * <member>S &mdash; South</member>
	 * <member>U &mdash; Up</member>
	 * <member>W &mdash; West</member>
	 * <member>X &mdash; eXamine</member>
	 * <member>Y &mdash; Yes</member>
	 * <member>Z &mdash; wait (ZZZZ...)</member>
	 * </simplelist>
	 * Some early Infocom games might not recognize these abbreviations. Setting
	 * this property to %TRUE will cause the interpreter to expand the
	 * abbreviations to the full words before passing the commands on to the
	 * game. Frotz only expands G, X, and Z; Nitfol expands all of the above
	 * plus the following nonstandard ones:
	 * <simplelist>
	 * <member>C &mdash; Close</member>
	 * <member>K &mdash; attacK</member>
	 * <member>P &mdash; oPen</member>
	 * <member>R &mdash; dRop</member>
	 * <member>T &mdash; Take</member>
	 * </simplelist>
	 *
	 * Only affects Z-machine interpreters. Behaves differently on Frotz and
	 * Nitfol.
	 */
	g_object_class_install_property(object_class, PROP_EXPAND_ABBREVIATIONS,
		g_param_spec_boolean("expand-abbreviations", "Expand abbreviations",
		"Expand abbreviations such as X for EXAMINE", FALSE,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_LAX_VALIDATION | G_PARAM_STATIC_STRINGS));
	/**
	 * ChimaraIF:ignore-errors:
	 *
	 * Setting this property to %TRUE will cause the interpreter to ignore
	 * certain Z-machine runtime errors.
   * If this is turned on, Frotz will ignore any fatal errors.
	 * Nitfol, on the other hand, will warn about any shady behavior at all if
	 * this is turned off.
	 *
	 * Only affects Z-machine interpreters. Behaves differently on Frotz and
	 * Nitfol.
	 */
	g_object_class_install_property(object_class, PROP_IGNORE_ERRORS,
		g_param_spec_boolean("ignore-errors", "Ignore errors",
		"Do not warn the user about Z-machine errors", FALSE,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_LAX_VALIDATION | G_PARAM_STATIC_STRINGS));
	/**
	 * ChimaraIF:typo-correction:
	 *
	 * Nitfol has an automatic typo-correction facility, where it searches the
	 * game dictionary for words which differ by one letter from any unknown
	 * input words. Set this property to %FALSE to turn this feature off.
	 *
	 * Only affects Nitfol.
	 */
	g_object_class_install_property(object_class, PROP_TYPO_CORRECTION,
		g_param_spec_boolean("typo-correction", "Typo correction",
		"Try to remedy typos if the interpreter supports it", TRUE,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_LAX_VALIDATION | G_PARAM_STATIC_STRINGS));
	/**
	 * ChimaraIF:interpreter-number:
	 *
	 * Infocom gave each port of their interpreter a different number. The Frotz
	 * and Nitfol plugins can emulate any of these platforms. Some games behave
	 * slightly differently depending on what platform they are on. Set this
	 * property to a #ChimaraIFZmachineVersion value to emulate a certain
	 * platform.
	 *
	 * Note that Nitfol pretends to be an Apple IIe by default.
	 *
	 * Only affects Z-machine interpreters.
	 */
	g_object_class_install_property(object_class, PROP_INTERPRETER_NUMBER,
		g_param_spec_uint("interpreter-number", "Interpreter number",
		"Platform the Z-machine should pretend it is running on",
		CHIMARA_IF_ZMACHINE_DEFAULT, CHIMARA_IF_ZMACHINE_MAXVAL, CHIMARA_IF_ZMACHINE_DEFAULT,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_LAX_VALIDATION | G_PARAM_STATIC_STRINGS));
	/**
	 * ChimaraIF:random-seed:
	 *
	 * If the #ChimaraIF:random-seed-set property is %TRUE, then the interpreter
	 * will use the value of this property as a seed for the random number
	 * generator. Use this feature to duplicate sequences of random numbers
	 * for testing games.
	 *
	 * Note that the value -1 is a valid random number seed for
	 * Nitfol, whereas it will cause Frotz to pick an arbitrary seed even when
	 * #ChimaraIF:random-seed-set is %TRUE.
	 *
	 * Only affects Z-machine interpreters. Behaves slightly differently on
	 * Frotz and Nitfol.
	 */
	g_object_class_install_property(object_class, PROP_RANDOM_SEED,
		g_param_spec_int("random-seed", "Random seed",
		"Seed for the random number generator", G_MININT, G_MAXINT, 0,
		G_PARAM_READWRITE | G_PARAM_LAX_VALIDATION | G_PARAM_STATIC_STRINGS));
	/**
	 * ChimaraIF:random-seed-set:
	 *
	 * Whether to use or ignore the #ChimaraIF:random-seed property.
	 *
	 * Only affects Z-machine interpreters.
	 */
	g_object_class_install_property(object_class, PROP_RANDOM_SEED_SET,
		g_param_spec_boolean("random-seed-set", "Random seed set",
		"Whether the seed for the random number generator should be set manually", FALSE,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_LAX_VALIDATION | G_PARAM_STATIC_STRINGS));
	/**
	 * ChimaraIF:graphics-file:
	 *
	 * Some Z-machine interpreters accept an extra argument that indicates a
	 * separate Blorb file containing graphics and sound resources. The
	 * interpreter will check if the file specified in this property really
	 * exists, and if so, use it as a resource file. If this property is set to 
	 * %NULL, the interpreter will not look for an extra file.
	 *
	 * Only affects Frotz and Nitfol.
	 */
	g_object_class_install_property(object_class, PROP_GRAPHICS_FILE,
	    g_param_spec_string("graphics-file", "Graphics file",
	    "Location in which to look for a separate graphics Blorb file", NULL,
	    G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_LAX_VALIDATION | G_PARAM_STATIC_STRINGS));
}

/* PUBLIC FUNCTIONS */

/**
 * chimara_if_new:
 *
 * Creates and initializes a new #ChimaraIF widget.
 *
 * Return value: a #ChimaraIF widget, with a floating reference.
 */
GtkWidget *
chimara_if_new(void)
{
	/* This is a library entry point; initialize the library */
	chimara_init();
    return GTK_WIDGET(g_object_new(CHIMARA_TYPE_IF, NULL));
}

/**
 * chimara_if_set_preferred_interpreter:
 * @self: A #ChimaraIF widget.
 * @format: The game format to set the preferred interpreter plugin for.
 * @interpreter: The preferred interpreter plugin for @format.
 *
 * The function chimara_if_run_game() picks an appropriate interpreter for the
 * type of game file it is given. This function sets which interpreter is picked
 * for a certain game file format. Most formats, notably the Z-machine, have
 * had many different interpreters written for them over the years, all with
 * slightly different quirks and capabilities, so there is plenty of choice.
 */
void
chimara_if_set_preferred_interpreter(ChimaraIF *self, ChimaraIFFormat format, ChimaraIFInterpreter interpreter)
{
	g_return_if_fail(self && CHIMARA_IS_IF(self));
	g_return_if_fail(format < CHIMARA_IF_NUM_FORMATS);
	g_return_if_fail(interpreter < CHIMARA_IF_NUM_INTERPRETERS);

	ChimaraIFPrivate *priv = chimara_if_get_instance_private(self);

	if(supported_formats[format][interpreter])
		priv->preferred_interpreter[format] = interpreter;
	else
		g_warning("Format '%s' is not supported by interpreter '%s'", format_names[format], interpreter_names[interpreter]);
}

/**
 * chimara_if_get_preferred_interpreter:
 * @self: A #ChimaraIF widget.
 * @format: The game format to query the preferred interpreter plugin for.
 *
 * Looks up the preferred interpreter for the game file format @format. See
 * chimara_if_set_preferred_interpreter().
 *
 * Returns: a #ChimaraIFInterpreter value representing the preferred interpreter
 * plugin for @format.
 */
ChimaraIFInterpreter
chimara_if_get_preferred_interpreter(ChimaraIF *self, ChimaraIFFormat format)
{
	g_return_val_if_fail(self && CHIMARA_IS_IF(self), -1);
	g_return_val_if_fail(format < CHIMARA_IF_NUM_FORMATS, -1);
	ChimaraIFPrivate *priv = chimara_if_get_instance_private(self);
	return priv->preferred_interpreter[format];
}

/**
 * chimara_if_run_game:
 * @self: A #ChimaraIF widget.
 * @game_path: Path to an interactive fiction game file.
 * @error: Return location for an error, or %NULL.
 *
 * Autodetects the type of a game file and runs it using an appropriate
 * interpreter plugin. If there is more than one interpreter that supports the
 * file format, the preferred one will be picked, according to
 * chimara_if_set_preferred_interpreter().
 *
 * Returns: %TRUE if the game was started successfully, %FALSE if not, in which
 * case @error is set.
 */
gboolean
chimara_if_run_game(ChimaraIF *self, const char *game_path, GError **error)
{
	g_return_val_if_fail(self && CHIMARA_IS_IF(self), FALSE);
	g_return_val_if_fail(game_path, FALSE);
	g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

	ChimaraIFPrivate *priv = chimara_if_get_instance_private(self);

	/* Find out what format the game is */
	/* TODO: Look inside the file instead of just looking at the extension */
	ChimaraIFFormat format = CHIMARA_IF_FORMAT_Z5;
	if(g_str_has_suffix(game_path, ".z5"))
		format = CHIMARA_IF_FORMAT_Z5;
	else if(g_str_has_suffix(game_path, ".z6"))
		format = CHIMARA_IF_FORMAT_Z6;
	else if(g_str_has_suffix(game_path, ".z8"))
		format = CHIMARA_IF_FORMAT_Z8;
	else if(g_str_has_suffix(game_path, ".zlb") || g_str_has_suffix(game_path, ".zblorb"))
		format = CHIMARA_IF_FORMAT_Z_BLORB;
	else if(g_str_has_suffix(game_path, ".ulx"))
		format = CHIMARA_IF_FORMAT_GLULX;
	else if(g_str_has_suffix(game_path, ".blb") || g_str_has_suffix(game_path, ".blorb") || g_str_has_suffix(game_path, ".glb") || g_str_has_suffix(game_path, ".gblorb"))
		format = CHIMARA_IF_FORMAT_GLULX_BLORB;

	/* Now decide what interpreter to use */
	ChimaraIFInterpreter interpreter = priv->preferred_interpreter[format];
	gchar *pluginfile = g_strconcat(plugin_names[interpreter], "." G_MODULE_SUFFIX, NULL);

	gchar *pluginpath;
#ifdef DEBUG
	/* If there is a plugin in the source tree, use that */
	pluginpath = g_build_filename(PLUGINSOURCEDIR, plugin_names[interpreter], pluginfile, NULL);
	if( !g_file_test(pluginpath, G_FILE_TEST_EXISTS) )
	{
		g_free(pluginpath);
#endif /* DEBUG */
		pluginpath = g_build_filename(PLUGINDIR, pluginfile, NULL);
		if( !g_file_test(pluginpath, G_FILE_TEST_EXISTS) )
		{
			g_free(pluginpath);
			g_free(pluginfile);
			g_set_error(error, CHIMARA_ERROR, CHIMARA_PLUGIN_NOT_FOUND,
				"No appropriate %s interpreter plugin was found", interpreter_names[interpreter]);
			return FALSE;
		}
#ifdef DEBUG
	}
#endif
	g_free(pluginfile);

	/* Decide what arguments to pass to the interpreters; currently only the
	Z-machine interpreters accept command line arguments other than the game */
	GSList *args = NULL;
	gchar *terpnumstr = NULL, *randomstr = NULL;
	args = g_slist_prepend(args, pluginpath);
	switch(interpreter)
	{
		case CHIMARA_IF_INTERPRETER_FROTZ:
			if(priv->flags & CHIMARA_IF_PIRACY_MODE)
				args = g_slist_prepend(args, "-P");
			if(priv->flags & CHIMARA_IF_TANDY_BIT)
				args = g_slist_prepend(args, "-t");
			if(priv->flags & CHIMARA_IF_EXPAND_ABBREVIATIONS)
				args = g_slist_prepend(args, "-x");
			if(priv->flags & CHIMARA_IF_IGNORE_ERRORS)
				args = g_slist_prepend(args, "-i");
			if(priv->interpreter_number != CHIMARA_IF_ZMACHINE_DEFAULT)
			{
				terpnumstr = g_strdup_printf("-I%u", priv->interpreter_number);
				args = g_slist_prepend(args, terpnumstr);
			}
			if(priv->random_seed_set)
			{
				randomstr = g_strdup_printf("-s%d", priv->random_seed);
				args = g_slist_prepend(args, randomstr);
			}
			break;
		case CHIMARA_IF_INTERPRETER_NITFOL:
			if(priv->flags & CHIMARA_IF_PIRACY_MODE)
				args = g_slist_prepend(args, "-pirate");
			if(priv->flags & CHIMARA_IF_TANDY_BIT)
				args = g_slist_prepend(args, "-tandy");
			if(!(priv->flags & CHIMARA_IF_EXPAND_ABBREVIATIONS))
				args = g_slist_prepend(args, "-no-expand");
			if(priv->flags & CHIMARA_IF_IGNORE_ERRORS)
				args = g_slist_prepend(args, "-ignore");
			if(!(priv->flags & CHIMARA_IF_TYPO_CORRECTION))
				args = g_slist_prepend(args, "-no-spell");
			if(priv->interpreter_number != CHIMARA_IF_ZMACHINE_DEFAULT)
			{
				terpnumstr = g_strdup_printf("-terpnum%u", priv->interpreter_number);
				args = g_slist_prepend(args, terpnumstr);
			}
			if(priv->random_seed_set)
			{
				randomstr = g_strdup_printf("-random%d", priv->random_seed);
				args = g_slist_prepend(args, randomstr);
			}
			break;
		default:
			;
	}

	/* Game file and external blorb file */
	args = g_slist_prepend(args, (gpointer)game_path);
	if(priv->graphics_file
		&& (interpreter == CHIMARA_IF_INTERPRETER_FROTZ || interpreter == CHIMARA_IF_INTERPRETER_NITFOL)
	    && g_file_test(priv->graphics_file, G_FILE_TEST_EXISTS)) {
		args = g_slist_prepend(args, priv->graphics_file);
	}

	/* Allocate argv to hold the arguments */
	int argc = g_slist_length(args);
	args = g_slist_prepend(args, NULL);
	char **argv = g_new0(char *, argc + 1);

	/* Fill argv */
	args = g_slist_reverse(args);
	int count;
	GSList *ptr;
	for(count = 0, ptr = args; ptr; count++, ptr = g_slist_next(ptr))
		argv[count] = g_strdup(ptr->data);
	g_slist_free(args);

	/* Set the story name */
	g_autofree char *story_name = g_path_get_basename(game_path);
	chimara_glk_set_story_name(CHIMARA_GLK(self), story_name);

	gboolean retval = chimara_glk_run(CHIMARA_GLK(self), pluginpath, argc, argv, error);
	g_strfreev(argv);
	if(terpnumstr)
		g_free(terpnumstr);
	if(randomstr)
		g_free(randomstr);
	g_free(pluginpath);

	/* Set current format and interpreter if plugin was started successfully */
	if(retval)
	{
		priv->format = format;
		priv->interpreter = interpreter;
	}
	return retval;
}

/**
 * chimara_if_run_game_file:
 * @self: A #ChimaraIF widget.
 * @game_file: a #GFile pointing to an interactive fiction game file.
 * @error: Return location for an error, or %NULL.
 *
 * Autodetects the type of a game file and runs it using an appropriate
 * interpreter plugin. See chimara_if_run_game() for more information.
 *
 * Returns: %TRUE if the game was started successfully, %FALSE if not, in which
 * case @error is set.
 */
gboolean
chimara_if_run_game_file(ChimaraIF *self, GFile *game_file, GError **error)
{
	g_return_val_if_fail(self || CHIMARA_IS_IF(self), FALSE);
	g_return_val_if_fail(game_file || G_IS_FILE(game_file), FALSE);
	g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

	char *path = g_file_get_path(game_file);
	gboolean retval = chimara_if_run_game(self, path, error);
	g_free(path);
	return retval;
}

/**
 * chimara_if_get_format:
 * @self: A #ChimaraIF widget.
 *
 * Returns the file format of the currently running game.
 *
 * Returns: a #ChimaraIFFormat constant.
 */
ChimaraIFFormat
chimara_if_get_format(ChimaraIF *self)
{
	g_return_val_if_fail(self && CHIMARA_IS_IF(self), CHIMARA_IF_FORMAT_NONE);
	ChimaraIFPrivate *priv = chimara_if_get_instance_private(self);
	return priv->format;
}

/**
 * chimara_if_get_interpreter:
 * @self: A #ChimaraIF widget.
 *
 * Returns the interpreter plugin currently running.
 *
 * Returns: a #ChimaraIFInterpreter constant.
 */
ChimaraIFInterpreter
chimara_if_get_interpreter(ChimaraIF *self)
{
	g_return_val_if_fail(self && CHIMARA_IS_IF(self), CHIMARA_IF_INTERPRETER_NONE);
	ChimaraIFPrivate *priv = chimara_if_get_instance_private(self);
	return priv->interpreter;
}
