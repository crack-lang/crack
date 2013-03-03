# - Try to find the CURL library
# Once done this will define
#
#  CURL_FOUND - system has the CURL library
#  CURL_INCLUDE_DIR - the CURL include directory
#  CURL_LIBRARIES - The libraries needed to use CURL

# Copyright (c) 2006, Alexander Neundorf, <neundorf@kde.org>
# Copyright (c) 2012, Conrad Steenberg, <conrad.steenberg@gmail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.


if (CURL_INCLUDE_DIR AND CURL_LIBRARY)
  # Already in cache, be silent
  set(CURL_FIND_QUIETLY TRUE)
endif (CURL_INCLUDE_DIR AND CURL_LIBRARY)


if (NOT WIN32)
  # use pkg-config to get the directories and then use these values
  # in the FIND_PATH() and FIND_LIBRARY() calls
  find_package(PkgConfig)

  pkg_search_module(CURL libcurl>=7)

endif (NOT WIN32)

if (CURL_FOUND)
    find_path(CURL_INCLUDE_DIR curl.h HINTS ${PC_CURL_INCLUDEDIR}
                               ${PC_CURL_INCLUDE_DIRS} PATH_SUFFIXES curl
             )

    find_library(CURL_LIBRARY NAMES curl HINTS ${PC_CURL_LIBDIR}
                             ${PC_CURL_LIBRARY_DIRS}
                )

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(CURL DEFAULT_MSG CURL_INCLUDE_DIR
                                           CURL_LIBRARY
                                     )

    set(CURL_LIBRARIES ${CURL_LIBRARY})

    mark_as_advanced(CURL_INCLUDE_DIR CURL_LIBRARIES CURL_LIBRARY)
endif (CURL_FOUND)
