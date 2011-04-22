# Try to find the CWD librairies
#  CWD_FOUND - system has CWD lib
#  CWD_INCLUDE_DIR - the CWD include directory
#  CWD_LIBRARIES - Libraries needed to use CWD

# based on FindGMP by:
# Copyright (c) 2006, Laurent Montel, <montel@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.

if (CWD_INCLUDE_DIR AND CWD_LIBRARIES)
  # Already in cache, be silent
  set(CWD_FIND_QUIETLY TRUE)
endif (CWD_INCLUDE_DIR AND CWD_LIBRARIES)

find_path(CWD_INCLUDE_DIR NAMES "libcwd/sys.h" HINTS "$ENV{CWD_DIR}/include")
find_library(CWD_LIBRARIES NAMES cwd HINTS $ENV{CWD_DIR}/lib )
MESSAGE(STATUS "CWD lib: " ${CWD_LIBRARIES} )

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(CWD DEFAULT_MSG CWD_INCLUDE_DIR CWD_LIBRARIES)

mark_as_advanced(CWD_INCLUDE_DIR CWD_LIBRARIES)
