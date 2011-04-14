# Check for libcwd

AC_DEFUN([AM_PATH_CWD], [dnl
    AC_MSG_CHECKING(for libcwd)
    CWD_LIBS=-lcwd
    CWD_CPPFLAGS=-DGOT_CWD
    ac_save_LIBS="$LIBS"
    LIBS="$LIBS $CWD_LIBS"
    AC_LANG_PUSH(C++)
    AC_TRY_RUN([
        #include <libcwd/sys.h>
        #define CWDEBUG
        #include <libcwd/debug.h>
        
        void func() {}
        int main(int argc, char **argv) {
            libcwd::location_ct loc(reinterpret_cast<char *>(func));
        }
    ], [
        got_cwd=yes
        AC_MSG_RESULT(yes)
        AC_SUBST(CWD_LIBS)
        AC_SUBST(CWD_CPPFLAGS)
    ], [
        AC_MSG_RESULT(no)
        CWD_LIBS=
        CWD_CPPFLAGS=
    ])
    AC_LANG_POP
    LIBS="$ac_save_LIBS"
])
