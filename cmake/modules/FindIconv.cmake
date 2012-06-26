# Find iconv library
#
# Author: Eddy Xu <eddyxu at gmail.com>
#
# taken from http://www.cmake.org/pipermail/cmake/2007-October/017032.html
#
# Released under BSD license
#
#  ICONV_INCLUDE_DIRS   - where to find iconv.h, etc
#  ICONV_LIBRARIES      - Lists of libraries when using iconv
#  ICONV_FOUND          - True if iconv found

IF (ICONV_INCLUDE_DIRS)
    SET(ICONV_FOUND 1)
    RETURN()
ENDIF (ICONV_INCLUDE_DIRS)

# Look for the header file
FIND_PATH( ICONV_INCLUDE_DIR NAMES iconv.h )
MARK_AS_ADVANCED( ICONV_INCLUDE_DIR )

# Look for the library
FIND_LIBRARY( ICONV_LIBRARY NAMES iconv )
MARK_AS_ADVANCED( ICONV_LIBRARY )

# Copy the result to output variables
# ICONV_LIBRARY not required - it might be defined in libc
IF(ICONV_INCLUDE_DIR)
  SET(ICONV_FOUND 1)
  IF(NOT ICONV_LIBRARY)
    SET(ICONV_LIBRARY "")
  ENDIF(NOT ICONV_LIBRARY)
ELSE(ICONV_INCLUDE_DIR)
  SET(ICONV_FOUND 0)
ENDIF(ICONV_INCLUDE_DIR)

SET(ICONV_LIBRARIES ${ICONV_LIBRARY} CACHE FILEPATH "the iconv library")
SET(ICONV_INCLUDE_DIRS ${ICONV_INCLUDE_DIR} CACHE PATH "iconv include directory")

# Report results
IF(NOT ICONV_FOUND)
  SET(ICONV_DIR_MESSAGE "Iconv was not found. Make sure ICONV_INCLUDE_DIR is set.")

  IF(NOT Iconv_FIND_QUIETLY AND NOT Iconv_FIND_REQUIRED)
    MESSAGE(STATUS ${ICONV_DIR_MESSAGE})
  ENDIF(NOT Iconv_FIND_QUIETLY AND NOT Iconv_FIND_REQUIRED)

  IF(Iconv_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR ${ICONV_DIR_MESSAGE})
  ENDIF(Iconv_FIND_REQUIRED)
ELSE(NOT ICONV_FOUND)
  IF(NOT Iconv_FIND_QUIETLY)
    MESSAGE(STATUS "Found Iconv: ${ICONV_INCLUDE_DIR}")
  ENDIF(NOT Iconv_FIND_QUIETLY)
ENDIF(NOT ICONV_FOUND)
