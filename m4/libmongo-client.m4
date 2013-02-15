// Copyright 2011 Google Inc.
// Copyright 2013 Conrad Steenberg <conrad.steenberg@gmail.com>
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 


AC_DEFUN([AM_PATH_MONGO_CLIENT], [dnl
    AC_ARG_ENABLE(mongoclient,
        [  --disable-mongoclient         Disable the mongo client library],
        , enable_mongoclient=yes)
    MONGO_CLIENT_LIBS=
    MONGO_CLIENT_CFLAGS=
    if test x$enable_mongoclient = xyes; then
        AC_PATH_PROG(PKG_CONFIG, pkg-config, no)

        if test x$PKG_CONFIG != xno ; then
            if pkg-config --atleast-pkgconfig-version 0.7 ; then
              :
            else
              echo "*** pkg-config too old; version 0.7 or better required."
              PKG_CONFIG=no
            fi
        else
            enable_mongoclient=yes
        fi

        AC_MSG_CHECKING(for libmongo-client version >= 1.5.0)
        MONGO_CLIENT_CFLAGS=`$PKG_CONFIG libmongo-client --cflags`
        MONGO_CLIENT_LIBS=`$PKG_CONFIG libmongo-client --libs`
        ac_save_LIBS="$LIBS"
        ac_save_CFLAGS="$CFLAGS"
        LIBS="$LIBS $MONGO_CLIENT_LIBS"
        CFLAGS="$CFLAGS $MONGO_CLIENT_CFLAGS"
    else
        AC_MSG_WARN(mongo-client disabled)
    fi
    AC_SUBST(MONGO_CLIENT_LIBS)
    AC_SUBST(MONGO_CLIENT_CFLAGS)
])
