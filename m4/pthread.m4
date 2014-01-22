# Copyright 2012 Google Inc.
#
#   This Source Code Form is subject to the terms of the Mozilla Public
#   License, v. 2.0. If a copy of the MPL was not distributed with this
#   file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# check for pthread

AC_DEFUN([AM_PATH_PTHREAD], [dnl
    AC_MSG_CHECKING(for pthread)
    PTHREAD_LIBS=
    PTHREAD_CPPFLAGS=-pthread
    PTHREAD_LDFLAGS=-pthread
    ac_save_LIBS="$LIBS"
    LIBS="$LIBS $PTHREAD_LIBS"
    AC_TRY_RUN([
        #include <pthread.h>

        int main(int argc, const char **argv) {
            pthread_self();
            return 0;
        }
    ], [
        got_pthread=yes
        AC_MSG_RESULT(yes)
        AC_SUBST(PTHREAD_LIBS)
        AC_SUBST(PTHREAD_CPPFLAGS)
        AC_SUBST(PTHREAD_LDFLAGS)
    ], [
        AC_MSG_RESULT(no)
        PTHREAD_CPPFLAGS=
        PTHREAD_LDFLAGS=
        AC_SUBST(PTHREAD_CPPFLAGS)
        AC_SUBST(PTHREAD_LDFLAGS)
    ])
    LIBS="$ac_save_LIBS"
])
