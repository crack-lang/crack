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