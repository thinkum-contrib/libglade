#ifndef GLADE_WIDGET_TREE_H
#define GLADE_WIDGET_TREE_H

#include <gtk/gtk.h>

typedef struct _GladeAttribute GladeAttribute;
struct _GladeAttribute {
    gchar *name;
    gchar *value;
};

typedef struct _GladeAcceleratorInfo GladeAcceleratorInfo;
struct _GladeAcceleratorInfo {
    guint key;
    GdkModifierType modifiers;
    gchar *signal;
};

typedef struct _GladeSignalInfo GladeSignalInfo;
struct _GladeSignalInfo {
    gchar *name;
    gchar *handler;
    gchar *data;
    gchar *object; /* NULL if this isn't a connect_object signal */
    gboolean after : 1;
};

typedef struct _GladeStyleInfo GladeStyleInfo;
struct _GladeStyleInfo {
    gchar *name;
    gchar *rc_name;
    gboolean local : 1;
};

typedef struct _GladeWidgetInfo GladeWidgetInfo;
struct _GladeWidgetInfo {
    GladeWidgetInfo *parent;

    gchar *class;
    gchar *name;
    gchar *tooltip;

    gint width, height;
    gint border_width;

    /* bit field */
    gboolean visible : 1;
    gboolean sensitive : 1;
    gboolean can_default : 1;
    gboolean can_focus : 1;

    GladeStyleInfo *style;

    /* lists of GladeAttribute's */
    GList *attributes;
    GList *child_attributes; /* for the <child></child> section */

    GList *signals;
    GList *accelerators;

    GList *children;
};

typedef struct _GladeWidgetTree GladeWidgetTree;
struct _GladeWidgetTree {
    GList *styles;
    GList *widgets;
    GHashTable *names;
};

/* parse a file and create a GladeWidgetTree structure */
GladeWidgetTree *glade_widget_tree_parse_file(const char *file);
/* free a GladeWidgetTree structure*/
void glade_widget_tree_free(GladeWidgetTree *tree);
/* print the info stored in a GladeWidgetTree structure */
void glade_widget_tree_print(GladeWidgetTree *tree);

#endif
