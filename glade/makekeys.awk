BEGIN {
  printf "/* libglade - a library for building interfaces from XML files at runtime\n";
  printf " * Copyright (C) 1998  James Henstridge <james@daa.com.au>\n";
  printf " *\n";
  printf " * This library is free software; you can redistribute it and/or\n";
  printf " * modify it under the terms of the GNU Library General Public\n";
  printf " * License as published by the Free Software Foundation; either\n";
  printf " * version 2 of the License, or (at your option) any later version.\n";
  printf " *\n";
  printf " * This library is distributed in the hope that it will be useful,\n";
  printf " * but WITHOUT ANY WARRANTY; without even the implied warranty of\n";
  printf " * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU\n";
  printf " * Library General Public License for more details.\n";
  printf " *\n";
  printf " * You should have received a copy of the GNU Library General Public\n";
  printf " * License along with this library; if not, write to the \n";
  printf " * Free Software Foundation, Inc., 59 Temple Place - Suite 330,\n";
  printf " * Boston, MA  02111-1307, USA.\n";
  printf " */\n\n";
  printf "#include <glib.h>\n";
  printf "#include <gdk/gdkkeysyms.h>\n\n";
  printf "/* This file was automatically produced by makekeys.awk */\n\n";
  printf "typedef struct _GladeKeyDef GladeKeyDef;\n";
  printf "struct _GladeKeyDef {\n";
  printf "  char *str;\n";
  printf "  guint key;\n";
  printf "};\n\n";
  printf "static const GladeKeyDef keys[] = {\n";
}

/^\#define/ {
  printf "  { \"%s\", %s },\n", $2, $2;
}

END {
  printf "  { NULL, 0 }\n";
  printf "};\n\n";

  printf "static GHashTable *key_hash() {\n";
  printf "  static GHashTable *hash = NULL;\n";
  printf "  if (!hash) {\n";
  printf "    int i = 0;\n";
  printf "    hash = g_hash_table_new(g_str_hash, g_str_equal);\n";
  printf "    g_hash_table_freeze(hash);\n";
  printf "    while (keys[i].str != NULL) {\n";
  printf "      g_hash_table_insert(hash, keys[i].str, GUINT_TO_POINTER(keys[i].key));\n";
  printf "      i++;\n";
  printf "    }\n";
  printf "    g_hash_table_thaw(hash);\n";
  printf "  }\n";
  printf "  return hash;\n";
  printf "}\n\n";

  printf "guint glade_key_get(char *str) {\n";
  printf "  guint ret = GPOINTER_TO_UINT(g_hash_table_lookup(key_hash(), str));\n";
  printf "  g_return_val_if_fail(ret != 0, 0);\n";
  printf "  return ret;\n";
  printf "}\n";
}

