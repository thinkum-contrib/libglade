/* -*- Mode: C; c-basic-offset: 4 -*- */
#include <stdio.h>
#include <stdlib.h>

#include <glib.h>

#include <tree.h>
#include <parser.h>
#include <parserInternals.h>

#include "widget-tree.h"

guint glade_key_get(const char *str);

GladeWidgetTree *glade_widget_tree_new(void) {
    GladeWidgetTree *self = g_new0(GladeWidgetTree, 1);

    self->names = g_hash_table_new(g_str_hash, g_str_equal);

    return self;
}

GladeWidgetInfo *glade_widget_info_new(void) {
    GladeWidgetInfo *info = g_new0(GladeWidgetInfo, 1);

    info->width = -2;
    info->height = -2;
    info->border_width = -1;
    info->visible = TRUE;
    info->sensitive = TRUE;
    info->can_default = FALSE;
    info->can_focus = TRUE;

    return info;
}

static gchar *glade_style_make_name(void) {
    static guint num = 0;
    return g_strdup_printf("_local_style_%u", num++);
}

typedef enum {
    PARSER_START,
    PARSER_FINISH,
    PARSER_GTK_INTERFACE,
    PARSER_WIDGET,
    PARSER_WIDGET_ATTRIBUTE,
    PARSER_WIDGET_CHILD,
    PARSER_WIDGET_CHILD_ATTRIBUTE,
    PARSER_SIGNAL,
    PARSER_SIGNAL_ATTRIBUTE,
    PARSER_ACCELERATOR,
    PARSER_ACCELERATOR_ATTRIBUTE,
    PARSER_STYLE,
    PARSER_STYLE_ATTRIBUTE,
    PARSER_UNKNOWN
} ParserState;

static const char *states[] = {
    "START",
    "FINISH",
    "GTK_INTERFACE",
    "WIDGET",
    "WIDGET_ATTRIBUTE",
    "WIDGET_CHILD",
    "WIDGET_CHILD_ATTRIBUTE",
    "SIGNAL",
    "SIGNAL_ATTRIBUTE",
    "ACCELERATOR",
    "ACCELERATOR_ATTRIBUTE",
    "STYLE",
    "STYLE_ATTRIBUTE",
    "UNKNOWN"
};

typedef struct _GladeParseState GladeParseState;
struct _GladeParseState {
    ParserState state;

    gint unknown_depth; /* handle recursive unknown tags */
    ParserState prev_state; /* the previous state */

    gint widget_depth;
    GString *content;

    GladeWidgetTree *tree;
    GladeWidgetInfo *widget;

    GladeAttribute *cur_attr;
    GladeSignalInfo *cur_sig;
    GladeAcceleratorInfo *cur_accel;
    GladeStyleInfo *cur_style;

    GString *style_data;
};

static void gladeStartDocument(GladeParseState *state) {
    state->state = PARSER_START;
    state->unknown_depth = 0;
    state->prev_state = PARSER_UNKNOWN;

    state->widget_depth = 0;
    state->content = g_string_sized_new(128);

    state->tree = glade_widget_tree_new();
    state->widget = NULL;

    state->cur_attr = NULL;
    state->cur_sig = NULL;
    state->cur_accel = NULL;
    state->cur_style = NULL;

    state->style_data = NULL;
}

static void gladeEndDocument(GladeParseState *state) {
    if (state->cur_attr)
	g_free(state->cur_attr);
    state->cur_attr = NULL;
    g_message("Ending state == %s", states[state->state]);
    g_string_free(state->content, TRUE);
    if (state->unknown_depth != 0)
	g_warning("unknown_depth != 0 (%d)", state->unknown_depth);
    if (state->widget_depth != 0)
	g_warning("widget_depth != 0 (%d)", state->widget_depth);
}

static void gladeStartElement(GladeParseState *state, const CHAR *name,
			      const CHAR **attrs) {
    switch (state->state) {
    case PARSER_START:
	if (strcmp(name, "GTK-Interface"))
	    g_warning("Expecting GTK-Interface.  Got %s", name);
	state->state = PARSER_GTK_INTERFACE;
	break;
    case PARSER_GTK_INTERFACE:
	/* a widge tag ... */
	if (!strcmp(name, "widget")) {
	    GladeWidgetInfo *info = glade_widget_info_new();
	    state->widget = info;
	    state->tree->widgets = g_list_append(state->tree->widgets, info);

	    state->state = PARSER_WIDGET;
	    state->widget_depth++;
	} else if (!strcmp(name, "style")) {
	    state->cur_style = g_new0(GladeStyleInfo, 1);
	    state->tree->styles = g_list_prepend(state->tree->styles,
						 state->cur_style);
	    state->state = PARSER_STYLE;
	    state->style_data = g_string_sized_new(128);
	} else {
	    state->prev_state = state->state;
	    state->state = PARSER_UNKNOWN;
	    state->unknown_depth++;
	}
	break;
    case PARSER_WIDGET:
	if (!strcmp(name, "style")) {
	    state->cur_style = g_new0(GladeStyleInfo, 1);
	    state->cur_style->local = TRUE;
	    state->tree->styles = g_list_prepend(state->tree->styles,
						 state->cur_style);
	    state->state = PARSER_STYLE;
	    state->style_data = g_string_sized_new(128);
	} else if (!strcmp(name,"accelerator")||!strcmp(name,"Accelerator")) {
	    state->cur_accel = g_new0(GladeAcceleratorInfo, 1);
	    state->widget->accelerators = g_list_prepend(
			state->widget->accelerators, state->cur_accel);
	    state->state = PARSER_ACCELERATOR;
	} else if (!strcmp(name, "signal") || !strcmp(name, "Signal")) {
	    state->cur_sig = g_new0(GladeSignalInfo, 1);
	    state->widget->signals = g_list_prepend(state->widget->signals,
						    state->cur_sig);
	    state->state = PARSER_SIGNAL;
	} else if (!strcmp(name, "child")) {
	    /* the child section */
	    state->state = PARSER_WIDGET_CHILD;
	} else if (!strcmp(name, "widget")) {
	    /* child widget */
	    GladeWidgetInfo *info = glade_widget_info_new();
	    info->parent = state->widget;
	    state->widget->children = g_list_append(state->widget->children,
						    info);
	    state->widget = info;
	    state->widget_depth++;
	} else {
	    /* maybe an old GladeAttribute hase been left behind */
	    if (!state->cur_attr)
		state->cur_attr = g_new(GladeAttribute, 1);
	    state->widget->attributes=g_list_append(state->widget->attributes,
						    state->cur_attr);
	    state->cur_attr->name = g_strdup(name);
	    state->state = PARSER_WIDGET_ATTRIBUTE;
	    g_string_truncate(state->content, 0);
	}
	break;
    case PARSER_WIDGET_CHILD:
	/* maybe an old GladeAttribute hase been left behind */
	if (!state->cur_attr)
	    state->cur_attr = g_new(GladeAttribute, 1);
	state->widget->child_attributes =
	    g_list_append(state->widget->child_attributes, state->cur_attr);
	state->cur_attr->name = g_strdup(name);
	state->state = PARSER_WIDGET_CHILD_ATTRIBUTE;
	g_string_truncate(state->content, 0);
	break;
    case PARSER_WIDGET_ATTRIBUTE:
    case PARSER_WIDGET_CHILD_ATTRIBUTE:
    case PARSER_SIGNAL_ATTRIBUTE:
    case PARSER_ACCELERATOR_ATTRIBUTE:
    case PARSER_STYLE_ATTRIBUTE:
	/* there should be no tags inside these types of tags */
	g_message("*** '%s' tag found while in state %s", name,
		  states[state->state]);
	state->prev_state = state->state;
	state->state = PARSER_UNKNOWN;
	state->unknown_depth++;
	break;
    case PARSER_SIGNAL:
	state->state = PARSER_SIGNAL_ATTRIBUTE;
	g_string_truncate(state->content, 0);
	break;
    case PARSER_ACCELERATOR:
	state->state = PARSER_ACCELERATOR_ATTRIBUTE;
	g_string_truncate(state->content, 0);
	break;
    case PARSER_STYLE:
	state->state = PARSER_STYLE_ATTRIBUTE;
	g_string_truncate(state->content, 0);
	break;
    case PARSER_UNKNOWN:
	state->unknown_depth++;
	break;
    case PARSER_FINISH:
	/* should not start new elements in this state */
	g_assert_not_reached();
	break;
    }
    /*g_message("Start element %s (state %s)", name, states[state->state]);*/
}
static void gladeEndElement(GladeParseState *state, const CHAR *name) {
    switch (state->state) {
    case PARSER_UNKNOWN:
	state->unknown_depth--;
	if (state->unknown_depth == 0)
	    state->state = state->prev_state;
	break;
    case PARSER_WIDGET_ATTRIBUTE:
	state->state = PARSER_WIDGET;
	/* for common attributes, we store them in the GladeWidgetInfo
	 * structure directly.  This reduces memory usage and processing
	 * time later. */
	if (!strcmp(name, "class"))
	    state->widget->class = g_strdup(state->content->str);
	else if (!strcmp(name, "name")) {
	    state->widget->name = g_strdup(state->content->str);
	    g_hash_table_insert(state->tree->names, state->widget->name,
				state->widget->name);
	    /* insert name -> widget mapping into hash table */
	} else if (!strcmp(name, "tooltip"))
	    state->widget->tooltip = g_strdup(state->content->str);
	else if (!strcmp(name, "border_width"))
	    state->widget->width = strtol(state->content->str, NULL, 0);
	else if (!strcmp(name, "width"))
	    state->widget->width = strtol(state->content->str, NULL, 0);
	else if (!strcmp(name, "height"))
	    state->widget->height = strtol(state->content->str, NULL, 0);
	else if (!strcmp(name, "visible"))
	    state->widget->visible = (state->content->str[0] == 'T');
	else if (!strcmp(name, "sensitive"))
	    state->widget->sensitive = (state->content->str[0] == 'T');
	else if (!strcmp(name, "can_default"))
	    state->widget->can_default = (state->content->str[0] == 'T');
	else if (!strcmp(name, "can_focus"))
	    state->widget->can_focus = (state->content->str[0] == 'T');
	else if (!strcmp(name, "style_name")) {
	    GList *tmp;
	    GladeStyleInfo *si = NULL;
	    for (tmp = state->tree->styles; tmp; tmp = tmp->next) {
		si = tmp->data;
		if (!si->local && !strcmp(si->name, state->content->str))
		    break;
	    }
	    if (tmp)
		state->widget->style = si;
	} else {
	    /* some other attribute */
	    state->cur_attr->value = g_strdup(state->content->str);
	    state->cur_attr = NULL;
	    break;
	}
	/* leave the GladeAttribute structure available for another attr */
	state->widget->attributes = g_list_remove(state->widget->attributes,
						  state->cur_attr);
	g_free(state->cur_attr->name);
	break;
    case PARSER_WIDGET_CHILD_ATTRIBUTE:
	state->cur_attr->value = g_strdup(state->content->str);
	state->cur_attr = NULL;
	state->state = PARSER_WIDGET_CHILD;
	break;
    case PARSER_WIDGET_CHILD:
	state->state = PARSER_WIDGET;
	break;
    case PARSER_WIDGET:
	/* close the widget tag */
	state->widget = state->widget->parent;
	state->widget_depth--;
	if (!state->widget)
	    state->state = PARSER_GTK_INTERFACE;
	break;
    case PARSER_SIGNAL_ATTRIBUTE:
	state->state = PARSER_SIGNAL;
	if (!strcmp(name, "name"))
	    state->cur_sig->name = g_strdup(state->content->str);
	else if (!strcmp(name, "handler"))
	    state->cur_sig->handler = g_strdup(state->content->str);
	else if (!strcmp(name, "data"))
	    state->cur_sig->data = g_strdup(state->content->str);
	else if (!strcmp(name, "object"))
	    state->cur_sig->object = g_strdup(state->content->str);
	else if (!strcmp(name, "after"))
	    state->cur_sig->after = (state->content->str[0] == 'T');
	break;
    case PARSER_SIGNAL:
	state->state = PARSER_WIDGET;
	state->cur_sig = NULL;
	break;
    case PARSER_ACCELERATOR_ATTRIBUTE:
	state->state = PARSER_ACCELERATOR;
	if (!strcmp(name, "key"))
	    state->cur_accel->key = glade_key_get(state->content->str);
	else if (!strcmp(name, "modifiers")) {
	    /* decode the modifiers string */
	    char *pos = state->content->str;

	    state->cur_accel->modifiers = 0;
	    while (pos[0])
		if (!strncmp(pos, "GDK_", 4)) {
		    pos += 4;
		    if (!strncmp(pos, "SHIFT_MASK", 10)) {
			state->cur_accel->modifiers |= GDK_SHIFT_MASK;
			pos += 10;
		    } else if (!strncmp(pos, "LOCK_MASK", 9)) {
			state->cur_accel->modifiers |= GDK_LOCK_MASK;
			pos += 9;
		    } else if (!strncmp(pos, "CONTROL_MASK", 12)) {
			state->cur_accel->modifiers |= GDK_CONTROL_MASK;
			pos += 12;
		    } else if (!strncmp(pos, "MOD", 3) &&
			       !strncmp(pos+4, "_MASK", 5)) {
			switch (pos[3]) {
			case '1':
			    state->cur_accel->modifiers |= GDK_MOD1_MASK;break;
			case '2':
			    state->cur_accel->modifiers |= GDK_MOD2_MASK;break;
			case '3':
			    state->cur_accel->modifiers |= GDK_MOD3_MASK;break;
			case '4':
			    state->cur_accel->modifiers |= GDK_MOD4_MASK;break;
			case '5':
			    state->cur_accel->modifiers |= GDK_MOD5_MASK;break;
			}
			pos += 9;
		    } else if (!strncmp(pos, "BUTTON", 6) &&
			       !strncmp(pos+7, "_MASK", 5)) {
			switch (pos[6]) {
			case '1':
			   state->cur_accel->modifiers|=GDK_BUTTON1_MASK;break;
			case '2':
			   state->cur_accel->modifiers|=GDK_BUTTON2_MASK;break;
			case '3':
			   state->cur_accel->modifiers|=GDK_BUTTON3_MASK;break;
			case '4':
			   state->cur_accel->modifiers|=GDK_BUTTON4_MASK;break;
			case '5':
			   state->cur_accel->modifiers|=GDK_BUTTON5_MASK;break;
			}
			pos += 12;
		    } else if (!strncmp(pos, "RELEASE_MASK", 12)) {
			state->cur_accel->modifiers |= GDK_RELEASE_MASK;
			pos += 12;
		    } else
			pos++;
		}
	} else if (!strcmp(name, "signal"))
	    state->cur_accel->signal = g_strdup(state->content->str);
	break;
    case PARSER_ACCELERATOR:
	state->state = PARSER_WIDGET;
	state->cur_accel = NULL;
	break;
    case PARSER_STYLE_ATTRIBUTE:
	state->state = PARSER_STYLE;
	/* append a bit of data for the style, based on the attribute */
	if (!strcmp(name, "style_name"))
	    state->cur_style->name = g_strdup(state->content->str);
	else if (!strcmp(name, "style_font"))
	    g_string_sprintfa(state->style_data, "  font = \"%s\"\n",
			      state->content->str);
	else if (!strncmp(name, "fg-", 3)) {
	    gint r, g, b;
	    sscanf(state->content->str, "%d,%d,%d", &r, &g, &b);
	    g_string_sprintfa(state->style_data,
			      "  fg[%s] = { %.3f, %.3f, %.3f }\n",
			      &name[3],
			      CLAMP(r, 0, 255) / 255.0,
			      CLAMP(g, 0, 255) / 255.0,
			      CLAMP(b, 0, 255) / 255.0);
	} else if (!strncmp(name, "bg-", 3)) {
	    gint r, g, b;
	    sscanf(state->content->str, "%d,%d,%d", &r, &g, &b);
	    g_string_sprintfa(state->style_data,
			      "  bg[%s] = { %.3f, %.3f, %.3f }\n",
			      &name[3],
			      CLAMP(r, 0, 255) / 255.0,
			      CLAMP(g, 0, 255) / 255.0,
			      CLAMP(b, 0, 255) / 255.0);
	} else if (!strncmp(name, "text-", 5)) {
	    gint r, g, b;
	    sscanf(state->content->str, "%d,%d,%d", &r, &g, &b);
	    g_string_sprintfa(state->style_data,
			      "  text[%s] = { %.3f, %.3f, %.3f }\n",
			      &name[5],
			      CLAMP(r, 0, 255) / 255.0,
			      CLAMP(g, 0, 255) / 255.0,
			      CLAMP(b, 0, 255) / 255.0);
	} else if (!strncmp(name, "base-", 5)) {
	    gint r, g, b;
	    sscanf(state->content->str, "%d,%d,%d", &r, &g, &b);
	    g_string_sprintfa(state->style_data,
			      "  base[%s] = { %.3f, %.3f, %.3f }\n",
			      &name[5],
			      CLAMP(r, 0, 255) / 255.0,
			      CLAMP(g, 0, 255) / 255.0,
			      CLAMP(b, 0, 255) / 255.0);
	} else if (!strncmp(name, "bg_pixmap-", 10))
	    g_string_sprintfa(state->style_data, "  bg_pixmap[%s] = \"%s\"\n",
			      &name[10], state->content->str);
	break;
    case PARSER_STYLE:
	if (state->cur_style->local)
	    state->state = PARSER_WIDGET;
	else
	    state->state = PARSER_GTK_INTERFACE;
	state->cur_style->data = state->style_data->str;
	if (!state->cur_style->name)
	    state->cur_style->name = glade_style_make_name();
	g_string_free(state->style_data, FALSE);
	state->cur_style = NULL;
	state->style_data = NULL;
	break;
    case PARSER_GTK_INTERFACE:
	/* the end of the file ... */
	state->state = PARSER_FINISH;
	break;
    case PARSER_START:
	/* we should not have a closing tag in this state */
	g_assert_not_reached();
	break;
    }
    /*g_message("End element %s (state %s)", name, states[state->state]);*/
}
static void gladeCharacters(GladeParseState *state, const CHAR *chars,
			    int len) {
    int i;

    if (state->state == PARSER_WIDGET_ATTRIBUTE ||
	state->state == PARSER_WIDGET_CHILD_ATTRIBUTE ||
	state->state == PARSER_SIGNAL_ATTRIBUTE ||
	state->state == PARSER_ACCELERATOR_ATTRIBUTE ||
	state->state == PARSER_STYLE_ATTRIBUTE)
	for (i = 0; i < len; i++)
	    g_string_append_c(state->content, chars[i]);
}

static xmlEntityPtr gladeGetEntity(GladeParseState *state, const CHAR *name) {
    return xmlGetPredefinedEntity(name);
}

static void gladeComment(GladeParseState *state, const char *msg) {
    g_log("XML", G_LOG_LEVEL_MESSAGE, "%s", msg);
}

static void gladeWarning(GladeParseState *state, const char *msg, ...) {
    va_list args;

    va_start(args, msg);
    g_logv("XML", G_LOG_LEVEL_WARNING, msg, args);
    va_end(args);
}

static void gladeError(GladeParseState *state, const char *msg, ...) {
    va_list args;

    va_start(args, msg);
    g_logv("XML", G_LOG_LEVEL_CRITICAL, msg, args);
    va_end(args);
}

static void gladeFatalError(GladeParseState *state, const char *msg, ...) {
    va_list args;

    va_start(args, msg);
    g_logv("XML", G_LOG_LEVEL_ERROR, msg, args);
    va_end(args);
}

static xmlSAXHandler gladeSAXParser = {
    0, /* internalSubset */
    0, /* isStandalone */
    0, /* hasInternalSubset */
    0, /* hasExternalSubset */
    0, /* resolveEntity */
    (getEntitySAXFunc)gladeGetEntity, /* getEntity */
    0, /* entityDecl */
    0, /* notationDecl */
    0, /* attributeDecl */
    0, /* elementDecl */
    0, /* unparsedEntityDecl */
    0, /* setDocumentLocator */
    (startDocumentSAXFunc)gladeStartDocument, /* startDocument */
    (endDocumentSAXFunc)gladeEndDocument, /* endDocument */
    (startElementSAXFunc)gladeStartElement, /* startElement */
    (endElementSAXFunc)gladeEndElement, /* endElement */
    0, /* reference */
    (charactersSAXFunc)gladeCharacters, /* characters */
    0, /* ignorableWhitespace */
    0, /* processingInstruction */
    (commentSAXFunc)gladeComment, /* comment */
    (warningSAXFunc)gladeWarning, /* warning */
    (errorSAXFunc)gladeError, /* error */
    (fatalErrorSAXFunc)gladeFatalError, /* fatalError */
};

GladeWidgetTree *glade_parse_tree(const char *file) {
    xmlParserCtxtPtr ctxt;
    GladeParseState state;

    ctxt = xmlCreateFileParserCtxt(file);
    if (!ctxt) return;
    ctxt->sax = &gladeSAXParser;
    ctxt->userData = &state;

    xmlParseDocument(ctxt);

    if (!ctxt->wellFormed)
	g_warning("document not well formed!");
    ctxt->sax = NULL;
    xmlFreeParserCtxt(ctxt);

    return state.tree;
}

static void free_sig_info(GladeSignalInfo *info) {
    
}

static void glade_print_widget_info(GladeWidgetInfo *info, gchar *indent) {
    GList *tmp;

    g_print("\n");
    g_print("%sWidget name : %s\n", indent, info->name);
    g_print("%sWidget class: %s\n", indent, info->class);
    if (info->style)
	g_print("%sStyle name  : %s\n", indent, info->style->name);
    if (info->attributes) {
	g_print("%sAttributes:\n", indent);
	for (tmp = info->attributes; tmp; tmp = tmp->next) {
	    GladeAttribute *attr = tmp->data;
	    g_print("%s  %s = %s\n", indent, attr->name, attr->value);
	}
    }
    if (info->child_attributes) {
	g_print("%sChild Attributes:\n", indent);
	for (tmp = info->child_attributes; tmp; tmp = tmp->next) {
	    GladeAttribute *attr = tmp->data;
	    g_print("%s  %s = %s\n", indent, attr->name, attr->value);
	}
    }
    if (info->signals) {
	g_print("%sSignals:\n", indent);
	for (tmp = info->signals; tmp; tmp = tmp->next) {
	    GladeSignalInfo *sig = tmp->data;
	    g_print("%s %s <-> %s\n", indent, sig->name, sig->handler);
	}
    }
    if (info->children) {
	gchar *new_indent = g_strconcat(indent, "  ", NULL);
	g_print("%sChildren:\n", indent);
	for (tmp = info->children; tmp; tmp = tmp->next)
	    glade_print_widget_info(tmp->data, new_indent);
	g_free(new_indent);
    }
}

void glade_print_widget_tree(GladeWidgetTree *tree) {
    GList *tmp;

    for (tmp = tree->widgets; tmp; tmp = tmp->next)
	glade_print_widget_info(tmp->data, "");
}

void main(int argc, char *argv[]) {
    if (argc > 1)
	    glade_print_widget_tree(glade_parse_tree(argv[1]));
    else
	g_message("need filename");
}

