# Copyright 2012 Google Inc.
# 
#   This Source Code Form is subject to the terms of the Mozilla Public
#   License, v. 2.0. If a copy of the MPL was not distributed with this
#   file, You can obtain one at http://mozilla.org/MPL/2.0/.
# 
# check for OpenGL

AC_DEFUN([AM_PATH_PNG], [dnl
    AC_MSG_CHECKING(for PNG)
    PNG_LIBS=-lpng
    ac_save_LIBS="$LIBS"
    LIBS="$LIBS $PNG_LIBS"
    AC_TRY_RUN([
        #include <png.h>
        
        int main(int argc, const char **argv) {
            char header[8];
            png_sig_cmp(header, 0, 8);
            return 0;
        }
    ], [
        got_png=yes
        AC_MSG_RESULT(yes)
        AC_SUBST(PNG_LIBS)
    ], [
        AC_MSG_RESULT(no)
    ])
    LIBS="$ac_save_LIBS"
])