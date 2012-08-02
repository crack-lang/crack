# Copyright 2012 Google Inc.
# 
#   This Source Code Form is subject to the terms of the Mozilla Public
#   License, v. 2.0. If a copy of the MPL was not distributed with this
#   file, You can obtain one at http://mozilla.org/MPL/2.0/.
# 

AC_DEFUN([AM_PATH_SDLGFX], [dnl
    AC_ARG_ENABLE(sdlgfx,
        [  --disable-sdlgfx         Disable the SDL_gfx library],
        , enable_sdlgfx=yes)
    SDLGFX_LIBS=
    SDLGFX_CPPFLAGS=
    got_sdlgfx=no
    if test x$enable_sdlgfx = xyes; then
        AC_MSG_CHECKING(for sdlgfx version >= 1.2)
        SDLGFX_LIBS=-lSDL_gfx
        ac_save_LIBS="$LIBS"
        ac_save_CPPFLAGS="$CPPFLAGS"
        LIBS="$LIBS $SDL_LIBS $SDLGFX_LIBS"
        CPPFLAGS="$CPPFLAGS $SDLGFX_CPPFLAGS"
        AC_TRY_RUN([
            #include <SDL/SDL_gfxPrimitives.h>
            
            int main() {
                exit(SDL_GFXPRIMITIVES_MAJOR < 2);
            }
        ], [
            got_sdlgfx=yes
            AC_MSG_RESULT(yes)
        ], [
            AC_MSG_RESULT(no)
            SDLGFX_LIBS=
            SDLGFX_CPPFLAGS=
        ])
        LIBS="$ac_save_LIBS"
    else
        AC_MSG_WARN(sdlgfx disabled)
    fi
    AC_SUBST(SDLGFX_LIBS)
    AC_SUBST(SDLGFX_CPPFLAGS)
])
