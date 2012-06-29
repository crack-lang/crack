# Try to find the Crossroads I/O librairies
#  XS_FOUND - system has XS lib
#  XS_INCLUDE_DIR - the XS include directory
#  XS_LIBRARIES - Libraries needed to use XS

# FindCWD based on FindGMP by:
# Copyright (c) 2006, Laurent Montel, <montel@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.

# Adapted from FindCWD by:
# Copyright 2012 Conrad Steenberg <conrad.steenberg@gmail.com>
# 6/22/2012

if (XS_INCLUDE_DIR AND XS_LIBRARIES)
  # Already in cache, be silent
  set(XS_FIND_QUIETLY TRUE)
endif (XS_INCLUDE_DIR AND XS_LIBRARIES)

find_path(XS_INCLUDE_DIR NAMES "xs/xs.h" HINTS "$ENV{XS_DIR}/include")
find_library(XS_LIBRARIES NAMES xs HINTS $ENV{XS_DIR}/lib )
MESSAGE(STATUS "XS lib: " ${XS_LIBRARIES} )
MESSAGE(STATUS "XS include: " ${XS_INCLUDE} )

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(XS DEFAULT_MSG XS_INCLUDE_DIR XS_LIBRARIES)

mark_as_advanced(XS_INCLUDE_DIR XS_LIBRARIES)
