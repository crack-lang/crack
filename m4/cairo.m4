// Copyright 2011 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

AC_DEFUN([AM_PATH_CAIRO], [dnl
    AC_ARG_ENABLE(cairo,
        [  --disable-cairo         Disable the Cairo library],
        , enable_cairo=yes)
    CAIRO_LIBS=
    CAIRO_CPPFLAGS=
    got_cairo=no
    if test x$enable_cairo = xyes; then
        AC_MSG_CHECKING(for cairo version >= 1.10)
        CAIRO_LIBS=-lcairo
        ac_save_LIBS="$LIBS"
        ac_save_CPPFLAGS="$CPPFLAGS"
        LIBS="$LIBS $CAIRO_LIBS"
        CPPFLAGS="$CPPFLAGS $CAIRO_CPPFLAGS"
        AC_TRY_RUN([
            #include <cairo/cairo.h>
            
            int main() {
                cairo_version_string();
                return CAIRO_VERSION < 11000 ? 1 : 0;
            }
        ], [
            got_cairo=yes
            AC_MSG_RESULT(yes)
        ], [
            AC_MSG_RESULT(no)
            CAIRO_LIBS=
            CAIRO_CPPFLAGS=
        ])
        LIBS="$ac_save_LIBS"
    else
        AC_MSG_WARN(cairo disabled)
    fi
    AC_SUBST(CAIRO_LIBS)
    AC_SUBST(CAIRO_CPPFLAGS)
])
