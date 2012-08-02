# Copyright 2012 Google Inc.
# 
#   This Source Code Form is subject to the terms of the Mozilla Public
#   License, v. 2.0. If a copy of the MPL was not distributed with this
#   file, You can obtain one at http://mozilla.org/MPL/2.0/.
# 

AC_DEFUN([AM_PATH_FLUIDSYNTH], [dnl
    AC_ARG_ENABLE(fluidsynth,
        [  --disable-fluidsynth         Disable the fluidsynth library],
        , enable_fluidsynth=yes)
    FLUIDSYNTH_LIBS=
    FLUIDSYNTH_CPPFLAGS=
    got_fluidsynth=no
    if test x$enable_fluidsynth = xyes; then
        AC_MSG_CHECKING(for fluidsynth version >= 1.1)
        FLUIDSYNTH_LIBS=-lfluidsynth
        ac_save_LIBS="$LIBS"
        ac_save_CPPFLAGS="$CPPFLAGS"
        LIBS="$LIBS $SDL_LIBS $FLUIDSYNTH_LIBS"
        CPPFLAGS="$CPPFLAGS $FLUIDSYNTH_CPPFLAGS"
        AC_TRY_RUN([
            #include <fluidsynth.h>

            int main() {
                exit((FLUIDSYNTH_VERSION_MAJOR == 1 &&
                      FLUIDSYNTH_VERSION_MINOR >= 1) ? 0 : 1);
            }
        ], [
            got_fluidsynth=yes
            AC_MSG_RESULT(yes)
        ], [
            AC_MSG_RESULT(no)
            FLUIDSYNTH_LIBS=
            FLUIDSYNTH_CPPFLAGS=
        ])
        LIBS="$ac_save_LIBS"
    else
        AC_MSG_WARN(fluidsynth disabled)
    fi
    AC_SUBST(FLUIDSYNTH_LIBS)
    AC_SUBST(FLUIDSYNTH_CPPFLAGS)
])
