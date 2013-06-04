# - Try to find the NetCDF C client library
# Once done this will define
#
#  NETCDF_FOUND - system has the MONGO library
#  NETCDF_INCLUDE_DIR - the MONGO include directory
#  NETCDF_LIBRARIES - The libraries needed to use NetCDF

# Copyright (c) 2006, Alexander Neundorf, <neundorf@kde.org>
# Copyright (c) 2013, Conrad Steenberg, <conrad.steenberg@gmail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.


if (NETCDF_INCLUDE_DIR AND NETCDF_LIBRARY)
  # Already in cache, be silent
  set(NETCDF_FIND_QUIETLY TRUE)
endif (NETCDF_INCLUDE_DIR AND NETCDF_LIBRARY)


if (NOT WIN32)
  # use pkg-config to get the directories and then use these values
  # in the FIND_PATH() and FIND_LIBRARY() calls
  find_package(PkgConfig)

  pkg_search_module(NETCDF netcdf>=4.0)

endif (NOT WIN32)

if (NETCDF_FOUND)

    find_path(NETCDF_INCLUDE_DIR netcdf.h
              HINTS ${NETCDF_INCLUDE_DIRS})

    find_library(NETCDF_LIBRARY NAMES netcdf HINTS ${PC_NETCDF_LIBDIR}
                               ${PC_NETCDF_LIBRARY_DIRS}
                )

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(MONGO DEFAULT_MSG NETCDF_INCLUDE_DIR
                                            NETCDF_LIBRARY
                                     )

    set(NETCDF_LIBRARIES ${NETCDF_LIBRARY})

    mark_as_advanced(NETCDF_INCLUDE_DIR NETCDF_LIBRARIES NETCDF_LIBRARY)
endif (NETCDF_FOUND)
