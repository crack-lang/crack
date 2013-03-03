# - Try to find the mongo C client library
# Once done this will define
#
#  MONGO_FOUND - system has the MONGO library
#  MONGO_INCLUDE_DIR - the MONGO include directory
#  MONGO_LIBRARIES - The libraries needed to use MONGO

# Copyright (c) 2006, Alexander Neundorf, <neundorf@kde.org>
# Copyright (c) 2012, Conrad Steenberg, <conrad.steenberg@gmail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.


if (MONGO_INCLUDE_DIR AND MONGO_LIBRARY)
  # Already in cache, be silent
  set(MONGO_FIND_QUIETLY TRUE)
endif (MONGO_INCLUDE_DIR AND MONGO_LIBRARY)


if (NOT WIN32)
  # use pkg-config to get the directories and then use these values
  # in the FIND_PATH() and FIND_LIBRARY() calls
  find_package(PkgConfig)

  pkg_search_module(MONGO libmongo-client>=0.1)

endif (NOT WIN32)

if (MONGO_FOUND)
    find_path(GLIB_INCLUDE_DIR glib.h
              HINTS ${MONGO_INCLUDE_DIRS} 
              PATH_SUFFIXES glib-2.0)

    find_path(GLIB_CONFIG_DIR glibconfig.h
              HINTS ${MONGO_INCLUDE_DIRS} 
              PATH_SUFFIXES glib-2.0)

    find_path(MONGO_INCLUDE_DIR mongo.h
              HINTS ${MONGO_INCLUDE_DIRS} 
              PATH_SUFFIXES mongo-client)

    find_library(MONGO_LIBRARY NAMES mongo-client HINTS ${PC_MONGO_LIBDIR}
                               ${PC_MONGO_LIBRARY_DIRS}
                )

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(MONGO DEFAULT_MSG MONGO_INCLUDE_DIR
                                            MONGO_LIBRARY
                                     )

    set(MONGO_LIBRARIES ${MONGO_LIBRARY})

    mark_as_advanced(MONGO_INCLUDE_DIR MONGO_LIBRARIES MONGO_LIBRARY
                                       GLIB_INCLUDE_DIR
                    )
endif (MONGO_FOUND)
