# root

cmake_minimum_required(VERSION 2.6)

# this policy warning is due to lib command lines arg output from llvm-config
if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
    cmake_policy(SET CMP0011 NEW)
endif(COMMAND cmake_policy)

project (crack)

set(CRACK_VERSION "0.3")
set(API_VERSION   "0.0.0")

set(LLVM_MIN_VERSION       "2008000")
set(LLVM_MIN_VERSION_TEXT  "2.8")

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules )

#set(CMAKE_CXX_FLAGS_DEBUG "-fexceptions -g -Wall -Winline -W -Wwrite-strings -Wno-unused")

MESSAGE( STATUS "PROJECT_SOURCE_DIR: " ${PROJECT_SOURCE_DIR} )
MESSAGE( STATUS "CMAKE_MODULE_PATH: " ${CMAKE_MODULE_PATH} )

MESSAGE( STATUS "CMAKE_SYSTEM_PROCESSOR: " ${CMAKE_SYSTEM_PROCESSOR} )
INCLUDE(FindPkgConfig)

Find_Package(LLVM REQUIRED)

include_directories(${PROJECT_SOURCE_DIR} 
                    ${LLVM_INCLUDE_DIR})

IF( LLVM_VERSION LESS ${LLVM_MIN_VERSION} )
  MESSAGE(FATAL_ERROR "LLVM version ${LLVM_STRING_VERSION} is too old, 
                       please install ${LLVM_MIN_VERSION_TEXT} of greater!")
ENDIF( LLVM_VERSION LESS ${LLVM_MIN_VERSION} )

add_definitions(-DLLVM_VERSION=${LLVM_VERSION})
add_definitions("-DCRACKLIB=\"${CMAKE_INSTALL_PREFIX}/lib/crack-${CRACK_VERSION}\"")

# share this is autoconf
file(STRINGS sourceModules.txt CRACK_SRC_FILES)
#MESSAGE( STATUS "modules: " ${CRACK_SRC_FILES})
file(STRINGS runtimeModules.txt RUNTIME_SRC_FILES)

# these are llvm specific compile flags, needed only for source files that
# include llvm headers
set_source_files_properties( ${CRACK_SRC_FILES}
                             PROPERTIES COMPILE_FLAGS ${LLVM_COMPILE_FLAGS}
                            )
set_source_files_properties( crack.cc
                             PROPERTIES COMPILE_FLAGS ${LLVM_COMPILE_FLAGS}
                            )

add_library(libcrack SHARED ${CRACK_SRC_FILES})
set_target_properties(libcrack
                      PROPERTIES
                      OUTPUT_NAME CrackLang
                      SOVERSION ${API_VERSION}
                      LINK_FLAGS ${LLVM_LDFLAGS}
                      )
target_link_libraries( libcrack
                       ${LLVM_LIBS}
                     )

# extension config, per autoconf
set(SIZEOF_VOID_P ${CMAKE_SIZEOF_VOID_P})
configure_file("${PROJECT_SOURCE_DIR}/ext/crack_config.h.in"
               "${PROJECT_SOURCE_DIR}/ext/crack_config.h")

# test extension. we set this up to mirror the autoconf
# build
add_library(testext SHARED test/testext.cc)
set_target_properties(testext
                      PROPERTIES
                      OUTPUT_NAME testext
                      PREFIX ""
                      LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/test/.libs/"
                      )

# runtime extension
add_library( crack-runtime SHARED ${RUNTIME_SRC_FILES} )
set_target_properties(crack-runtime
                      PROPERTIES
                      OUTPUT_NAME runtime
                      PREFIX ""
                      LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/.libs/"
                      )


add_executable(crack crack.cc)
set_target_properties( crack
                       PROPERTIES LINK_FLAGS ${LLVM_LDFLAGS}
                       # test suite currently expects crack binary in project 
                       # source
                       RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}
                       VERSION ${CRACK_VERSION}
                     )

target_link_libraries( crack
                       libcrack
                     )

# install targets
install(TARGETS crack libcrack
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
       )

# runtime
install(TARGETS crack-runtime
         LIBRARY DESTINATION "lib/crack-${CRACK_VERSION}/crack"
        )

# manual
install(FILES doc/Manual.html DESTINATION share/doc/crack)

# header files
install(DIRECTORY compiler/ DESTINATION "include/crack-${CRACK_VERSION}/crack/compiler"
        FILES_MATCHING PATTERN "*.h")
install(DIRECTORY ext/ DESTINATION "include/crack-${CRACK_VERSION}/crack/ext"
        FILES_MATCHING PATTERN "*.h")

# install library modules and extensions
install(DIRECTORY lib/crack DESTINATION "lib/crack-${CRACK_VERSION}"
        FILES_MATCHING PATTERN "*.crk")
install(DIRECTORY .libs/ DESTINATION "lib/crack-${CRACK_VERSION}/crack/ext"
        FILES_MATCHING PATTERN "_*.so")


# test suite
add_custom_target(check 
                  ${PROJECT_SOURCE_DIR}/test/run_all_tests
                  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                  )
add_custom_target(checkc
                  ${PROJECT_SOURCE_DIR}/test/run_all_tests crackc
                  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                  )

add_custom_target(screen
                  ${PROJECT_SOURCE_DIR}/crack ${PROJECT_SOURCE_DIR}/screen/screen.crk ${PROJECT_SOURCE_DIR}/screen/tests/
                  )

# optional extensions

# GTK
find_package(GTK2 2.6 COMPONENTS gtk)
find_package(GTK2_GDKPIXBUF)
if(GTK2_FOUND)
   message(STATUS "Found GTK, will build extension")
   include_directories(${GTK2_INCLUDE_DIRS})
   add_library(gtk SHARED opt/_gtk.cc)
   set_target_properties(gtk
                      PROPERTIES
                      OUTPUT_NAME _gtk
                      COMPILE_FLAGS
                      ${GTK2_GDKPIXBUF_INCLUDE_DIRS}
                      PREFIX ""
                      LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/.libs/"
                      )
   target_link_libraries( gtk
                          ${GTK2_LIBRARIES} ${GTK2_GDKPIXBUF_LDFLAGS}
                         )
endif()

# pcre
find_library(PCRE_LIBRARY NAMES pcre)
find_path(PCRE_INCLUDE_DIR NAMES pcre.h)
if(PCRE_LIBRARY AND PCRE_INCLUDE_DIR)
  SET(PCRE_FOUND TRUE)
endif(PCRE_LIBRARY AND PCRE_INCLUDE_DIR)
if (PCRE_FOUND)
   message(STATUS "Found pcre, will build extension")
   include_directories(${PCRE_INCLUDE_DIR})
   add_library(pcre SHARED opt/_pcre.cc)
   set_target_properties(pcre
                      PROPERTIES
                      OUTPUT_NAME _pcre
                      PREFIX ""
                      LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/.libs/"
                      )
   target_link_libraries( pcre
                          ${PCRE_LIBRARY}
                         )

endif()

# SDL
find_package(SDL)
if(SDL_FOUND)
   message(STATUS "Found SDL, will build extension")
   include_directories(${SDL_INCLUDE_DIR})
   add_library(sdl SHARED opt/_sdl.cc)
   set_target_properties(sdl
                      PROPERTIES
                      OUTPUT_NAME _sdl
                      PREFIX ""
                      LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/.libs/"
                      )
   target_link_libraries( sdl
                          ${SDL_LIBRARY}
                         )
endif()

# OpenGL
find_package(OpenGL)
if(OPENGL_FOUND)
   message(STATUS "Found OpenGL, will build extension")
   include_directories(${OPENGL_INCLUDE_DIR})
   add_library(gl SHARED opt/_gl.cc)
   set_target_properties(gl
                      PROPERTIES
                      OUTPUT_NAME _gl
                      PREFIX ""
                      LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/.libs/"
                      )
   target_link_libraries( gl
                          ${OPENGL_LIBRARIES}
                         )
endif()

# Cairo
IF (PKG_CONFIG_FOUND)
   pkg_check_modules(CAIRO cairo>=1.8)
   if(CAIRO_FOUND)
      message(STATUS "Found Cairo, will build extension")
      add_library(cairo SHARED opt/_cairo.cc)
      set_target_properties(cairo
                         PROPERTIES
                         OUTPUT_NAME _cairo
                         PREFIX ""
                         LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/.libs/"
                         )
      target_link_libraries( cairo
                             ${CAIRO_LDFLAGS} 
                            )
   endif()
endif()
