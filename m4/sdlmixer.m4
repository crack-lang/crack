# Copyright 2014 Google Inc.
# 
#   This Source Code Form is subject to the terms of the Mozilla Public
#   License, v. 2.0. If a copy of the MPL was not distributed with this
#   file, You can obtain one at http://mozilla.org/MPL/2.0/.
# 
# check for SDL Mixer

AC_DEFUN([AM_PATH_SDL_MIXER], [dnl
    AC_MSG_CHECKING(for SDL Mixer)
    SDL_MIXER_LIBS=`pkg-config --libs SDL_mixer`
    SDL_MIXER_CPPFLAGS=`pkg-config --cflags SDL_mixer` 
    ac_save_LIBS="$LIBS"
    ac_save_CPPFLAGS="$CPPFLAGS"
    LIBS="$LIBS $SDL_MIXER_LIBS"
    CPPFLAGS="$CPPFLAGS $SDL_MIXER_CPPFLAGS"
    AC_TRY_RUN([
        #include <SDL_mixer.h>
        
        int main() {
            if (SDL_MIXER_MAJOR_VERSION != 1 ||
                SDL_MIXER_MINOR_VERSION < 2
                ) {
                printf("Wrong jack version\n");
                return 1;
            } else {
                return 0;
            }
        }
    ], [
        got_sdl_mixer=yes
        AC_MSG_RESULT(yes)
        AC_SUBST(SDL_MIXER_LIBS)
        AC_SUBST(SDL_MIXER_CPPFLAGS)
    ], [
        AC_MSG_RESULT(no)
    ])
    LIBS="$ac_save_LIBS"
    CPPFLAGS="$ac_save_CPPFLAGS"
])