
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
    gchar *data; /* the stuff in the rc file between "style ... {" and "}" */
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
