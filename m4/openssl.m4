# Copyright 2012 Google Inc.

AC_DEFUN([AM_PATH_OPENSSL], [dnl
    AC_ARG_ENABLE(ssl,
        [  --disable-ssl         Disable the OpenSSL library],
        , enable_ssl=yes)
    SSL_LIBS=
    SSL_CPPFLAGS=
    got_ssl=no
    if test x$enable_ssl = xyes; then
        AC_MSG_CHECKING(for openssl)
        SSL_LIBS=-lssl
        ac_save_LIBS="$LIBS"
        ac_save_CPPFLAGS="$CPPFLAGS"
        LIBS="$LIBS $SSL_LIBS"
        CPPFLAGS="$CPPFLAGS $SSL_CPPFLAGS"
        AC_TRY_RUN([
            #include <openssl/ssl.h>

            int main() {
                SSL_library_init();
                return 0;
            }
        ], [
            got_ssl=yes
            AC_MSG_RESULT(yes)
        ], [
            AC_MSG_RESULT(no)
            SSL_LIBS=
            SSL_CPPFLAGS=
        ])
        LIBS="$ac_save_LIBS"
    else
        AC_MSG_WARN(ssl disabled)
    fi
    AC_SUBST(SSL_LIBS)
    AC_SUBST(SSL_CPPFLAGS)
])
