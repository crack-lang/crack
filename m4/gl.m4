// Copyright 2010 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 
# check for OpenGL

AC_DEFUN([AM_PATH_GL], [dnl
    AC_MSG_CHECKING(for OpenGL)
    GL_LIBS=-lGL
    ac_save_LIBS="$LIBS"
    LIBS="$LIBS $GL_LIBS"
    AC_TRY_RUN([
        #include <GL/gl.h>
        
        int main(int argc, const char **argv) {
            glMatrixMode(GL_PROJECTION);
            return 0;
        }
    ], [
        got_gl=yes
        AC_MSG_RESULT(yes)
        AC_SUBST(GL_LIBS)
    ], [
        AC_MSG_RESULT(no)
    ])
    LIBS="$ac_save_LIBS"
])