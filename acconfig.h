#ifndef CONFIG_H
#define CONFIG_H

@TOP@
/* The package name */
#undef PACKAGE

/* The package version */
#undef VERSION

/* extra debugging output */
#undef DEBUG

/* If true, include GNOME support */
#undef ENABLE_GNOME

@BOTTOM@

#ifdef DEBUG
#  define debug(stmnt) stmnt
#else
#  define debug(stmnt) /* nothing */
#endif

#endif
