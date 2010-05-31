# Try to find the SPUG librairies
#  SPUG_FOUND - system has SPUG lib
#  SPUG_INCLUDE_DIR - the SPUG include directory
#  SPUG_LIBRARIES - Libraries needed to use SPUG

# based on FindGMP by:
# Copyright (c) 2006, Laurent Montel, <montel@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.

if (SPUG_INCLUDE_DIR AND SPUG_LIBRARIES)
  # Already in cache, be silent
  set(SPUG_FIND_QUIETLY TRUE)
endif (SPUG_INCLUDE_DIR AND SPUG_LIBRARIES)

find_path(SPUG_INCLUDE_DIR NAMES "spug/RCBase.h" HINTS "$ENV{SPUG_DIR}/include")
find_library(SPUG_LIBRARIES NAMES spug++ HINTS $ENV{SPUG_DIR}/lib )
MESSAGE(STATUS "spug lib: " ${SPUG_LIBRARIES} )

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(SPUG DEFAULT_MSG SPUG_INCLUDE_DIR SPUG_LIBRARIES)

mark_as_advanced(SPUG_INCLUDE_DIR SPUG_LIBRARIES)
