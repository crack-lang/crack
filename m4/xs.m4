// Copyright 2011 Google Inc.
// Copyright 2012 Conrad Steenberg <conrad.steenberg@gmail.com>
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

AC_DEFUN([AM_PATH_XS], [dnl
    AC_ARG_ENABLE(xs,
        [  --disable-xs         Disable the Crossroads library],
        , enable_xs=yes)
    XS_LIBS=
    XS_CFLAGS=
    got_xs=no
    if test x$enable_xs = xyes; then
        AC_MSG_CHECKING(for xs version >= 1.2.0)
        XS_LIBS=-lxs
        ac_save_LIBS="$LIBS"
        ac_save_CFLAGS="$CFLAGS"
        LIBS="$LIBS $XS_LIBS"
        CFLAGS="$CFLAGS $XS_CFLAGS"
        AC_TRY_RUN([
            #include <xs/xs.h>
            #include <stdio.h>
            
            int main() {
                return XS_VERSION < 10200 ? 1 : 0;
            }
        ], [
            got_xs=yes
            AC_MSG_RESULT(yes)
        ], [
            AC_MSG_RESULT(no)
            XS_LIBS=
            XS_CFLAGS=
        ])
        LIBS="$ac_save_LIBS"
    else
        AC_MSG_WARN(xs disabled)
    fi
    AC_SUBST(XS_LIBS)
    AC_SUBST(XS_CFLAGS)
])
