# Copyright 2015 Google Inc.
#
#   This Source Code Form is subject to the terms of the Mozilla Public
#   License, v. 2.0. If a copy of the MPL was not distributed with this
#   file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# check for fuse

AC_DEFUN([AM_PATH_FUSE], [dnl
    AC_MSG_CHECKING(for fuse)
    FUSE_LIBS=`pkg-config fuse --libs`
    FUSE_CPPFLAGS=`pkg-config fuse --cflags`
    ac_save_LIBS="$LIBS"
    LIBS="$LIBS $FUSE_LIBS"
    if pkg-config fuse --atleast-version 2.9; then
        got_fuse=yes
        AC_MSG_RESULT(yes)
        AC_SUBST(FUSE_LIBS)
        AC_SUBST(FUSE_CPPFLAGS)
    else
        AC_MSG_RESULT(no)
        FUSE_CPPFLAGS=
        FUSE_LDFLAGS=
        AC_SUBST(FUSE_CPPFLAGS)
        AC_SUBST(FUSE_LDFLAGS)
    fi
    LIBS="$ac_save_LIBS"
])
