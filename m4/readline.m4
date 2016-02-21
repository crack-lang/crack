# Copyright 2016 Google Inc.

AC_DEFUN([AM_PATH_READLINE], [dnl
    AC_ARG_ENABLE(readline,
        [  --disable-readline         Disable the Readline library],
        , enable_readline=yes)
    READLINE_LIBS=
    got_readline=no
    if test x$enable_readline = xyes; then
        AC_MSG_CHECKING(for readline)
        READLINE_LIBS=-lreadline
        ac_save_LIBS="$LIBS"
        LIBS="$LIBS $READLINE_LIBS"
        AC_TRY_RUN([
            #include <readline/readline.h>

            int main() {
                readline("");
                return 0;
            }
        ], [
            got_readline=yes
            AC_MSG_RESULT(yes)
        ], [
            AC_MSG_RESULT(no)
            READLINE_LIBS=
        ])
        LIBS="$ac_save_LIBS"
    else
        AC_MSG_WARN(ssl disabled)
    fi
    AC_SUBST(READLINE_LIBS)
])
