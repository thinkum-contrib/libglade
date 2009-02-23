#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <tree.h>
#include <parser.h>

/* from glade-tree.c */
typedef struct _GladeTreeData GladeTreeData;
struct _GladeTreeData {
	xmlDocPtr xml;
	GNode *tree;      /* a tree of the <widget> tags */
	GHashTable *hash; /* a hash of the GNodes */
};

static void
recurse_tree (xmlNodePtr xmlnode, GNode *parent, GHashTable *hash)
{
	xmlNodePtr tmp;
	GNode *self = g_node_new(xmlnode);

	g_node_append(parent, self);
	for (tmp = xmlnode->childs; tmp != NULL; tmp = tmp->next) {
		char *content = xmlNodeGetContent(tmp);
		if (tmp->name && !strcmp(tmp->name, "name"))
			g_hash_table_insert(hash, g_strdup(content), self);
		else if (tmp->name && !strcmp(tmp->name, "widget"))
			recurse_tree(tmp, self, hash);
		if (content) free(content);
	}
}

static GladeTreeData *
new_func(gchar *file)
{
	GladeTreeData *tree;
	xmlNodePtr tmp;

	tree = g_new(GladeTreeData, 1);
	tree->xml =  xmlParseFile(file);

	if (!tree->xml || strcmp(tree->xml->root->name, "GTK-Interface")) {
		g_warning("%s doesn't appear to be a GLADE XML file", file);
		xmlFreeDoc(tree->xml);
		g_free(tree);
		return NULL;
	}
	/*glade_style_parse(tree->xml);*/
	tree->tree = g_node_new(NULL); /* root node */
	tree->hash = g_hash_table_new(g_str_hash, g_str_equal);

	for (tmp = tree->xml->root->childs; tmp != NULL; tmp = tmp->next)
		if (tmp->name && !strcmp(tmp->name, "widget"))
			recurse_tree(tmp, tree->tree, tree->hash);
	return tree;
}

void main(int argc, char *argv[]) {
    int i;

    if (argc > 1)
	for (i = 0; i < 100; i++)
	    new_func(argv[1]);
    else
	g_message("need filename");
}

