# a macro to get the libs/cflags for libglade
# serial 1

dnl AM_PATH_LIBGLADE([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl Test to see if libglade is installed, and define LIBGLADE_CFLAGS, LIBS
dnl
AC_DEFUN(AM_PATH_LIBGLADE,
[dnl
dnl Get the cflags and libraries from the libglade-config script
dnl
AC_ARG_WITH(libglade-config,
[  --with-libglade-config=LIBGLADE_CONFIG  Location of libglade-config],
LIBGLADE_CONFIG="$withval")

AC_PATH_PROG(LIBGLADE_CONFIG, libglade-config, no)
AC_MSG_CHECKING(for libglade)
if test "$LIBGLADE_CONFIG" = "no"; then
  AC_MSG_RESULT(no)
  ifelse([$2], , :, [$2])
else
  LIBGLADE_CFLAGS=`$LIBGLADE_CONFIG --cflags`
  LIBGLADE_LIBS=`$LIBGLADE_CONFIG --libs`
  AC_MSG_RESULT(yes)
  ifelse([$1], , :, [$1])
fi
AC_SUBST(LIBGLADE_CFLAGS)
AC_SUBST(LIBGLADE_LIBS)
])