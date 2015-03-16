# Copyright 2003 Michael A. Muller <mmuller@enduden.com>
# Copyright 2009-2012 Google Inc.
# Copyright 2012 Arno Rehn <arno@arnorehn.de>
# 
#   This Source Code Form is subject to the terms of the Mozilla Public
#   License, v. 2.0. If a copy of the MPL was not distributed with this
#   file, You can obtain one at http://mozilla.org/MPL/2.0/.
# 

AC_INIT(crack, 0.10)
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([parser/Token.h])
AC_CONFIG_MACRO_DIR([m4])

# include cppunit
#AM_PATH_CPPUNIT(1.12.0)

# required for C++ programs...
AC_PROG_CXX

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST

AC_CHECK_HEADERS([malloc.h])
AC_CHECK_FUNCS(ppoll)

# automake advised me to add this...
AC_CONFIG_FILES([Makefile])
AC_CONFIG_HEADERS([config.h])

# ... and also this (when I switched to a library)
AC_PROG_RANLIB

# for my shared libs
AC_PROG_LIBTOOL
LT_INIT(dlopen)

# external packages that we need
AM_PATH_LLVM
AM_ICONV

# determine if we're 64 bit.
AC_CHECK_SIZEOF([void *])

# optional libraries

AM_PATH_GTK_2_0(2.0.0, [got_gtk=yes])
AM_CONDITIONAL(HAVE_GTK, [test "x$got_gtk" = xyes])

AX_PATH_LIB_PCRE(got_pcre=yes)
AM_CONDITIONAL(HAVE_PCRE, [test "x$got_pcre" = xyes])

AM_PATH_SDL(1.2.14, [got_sdl=yes])
AM_CONDITIONAL(HAVE_SDL, [test "x$got_sdl" = xyes])

AM_PATH_SDL_MIXER(1.2.14, [got_sdl_mixer=yes])
AM_CONDITIONAL(HAVE_SDL_MIXER, [test "x$got_sdl_mixer" = xyes])

AM_PATH_SDLGFX
AM_CONDITIONAL(HAVE_SDLGFX, [test "x$got_sdlgfx" = xyes])

AM_PATH_GL
AM_CONDITIONAL(HAVE_GL, [test "x$got_gl" = xyes])

AM_PATH_CAIRO
AM_CONDITIONAL(HAVE_CAIRO, [test "x$got_cairo" = xyes])

AM_PATH_XS
AM_CONDITIONAL(HAVE_XS, [test "x$got_xs" = xyes])

AM_PATH_ALSA(1.0.22, [got_alsa=yes], true)
AM_CONDITIONAL(HAVE_ALSA, [test "x$got_alsa" = xyes])

AM_PATH_OPENSSL
AM_CONDITIONAL(HAVE_SSL, [test "x$got_ssl" = xyes])

AM_PATH_FLUIDSYNTH
AM_CONDITIONAL(HAVE_FLUIDSYNTH, [test "x$got_fluidsynth" = xyes])

AM_PATH_PNG
AM_CONDITIONAL(HAVE_PNG, [test "x$got_png" = xyes])

LIBCURL_CHECK_CONFIG
AM_CONDITIONAL(HAVE_LIBCURL, [test "x$got_curl" = xyes])

AM_PATH_MONGO_CLIENT
AM_CONDITIONAL(HAVE_MONGO_CLIENT, [test "x$got_mongoclient" = xyes])

AM_PATH_JACK
AM_CONDITIONAL(HAVE_JACK, [test "x$got_jack" = xyes])

AM_PATH_PTHREAD
AM_CONDITIONAL(HAVE_PTHREAD, [test "x$got_pthread" = xyes])

AC_OUTPUT
