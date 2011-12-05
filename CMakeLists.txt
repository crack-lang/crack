# root

# 2.8.3 includes a patch for detecting gdk-pixbuf
cmake_minimum_required(VERSION 2.8.3)

# this policy warning is due to lib command lines arg output from llvm-config
if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
    cmake_policy(SET CMP0011 NEW)
endif(COMMAND cmake_policy)

project (crack)

set(CRACK_VERSION "0.6")
set(CRACKLANG_API_VERSION "3.0.0")
set(NATRUN_API_VERSION "2.0.0")

set(LLVM_MIN_VERSION       "3000000")
set(LLVM_MIN_VERSION_TEXT  "3.0")

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules )

set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

include(MacroOptionalFindPackage)
include(MacroLogFeature)

MESSAGE( STATUS "PROJECT_SOURCE_DIR: " ${PROJECT_SOURCE_DIR} )
MESSAGE( STATUS "CMAKE_MODULE_PATH: " ${CMAKE_MODULE_PATH} )

MESSAGE( STATUS "CMAKE_SYSTEM_PROCESSOR: " ${CMAKE_SYSTEM_PROCESSOR} )

Find_Package(LLVM REQUIRED)
IF(NOT LLVM_CONFIG_EXECUTABLE)
  MESSAGE(FATAL_ERROR "LLVM not found. Please install ${LLVM_MIN_VERSION_TEXT} or"
                      " greater, or set LLVM_DIR environment variable")
ENDIF (NOT LLVM_CONFIG_EXECUTABLE)

IF( LLVM_VERSION LESS ${LLVM_MIN_VERSION} )
  MESSAGE(FATAL_ERROR "LLVM version ${LLVM_STRING_VERSION} is too old,"
                      " please install ${LLVM_MIN_VERSION_TEXT} or greater!")
ENDIF( LLVM_VERSION LESS ${LLVM_MIN_VERSION} )

include_directories(${PROJECT_SOURCE_DIR}
                    ${PROJECT_BINARY_DIR}
                    ${LLVM_INCLUDE_DIR})

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
set_source_files_properties( crack_main.cc
                             PROPERTIES COMPILE_FLAGS ${LLVM_COMPILE_FLAGS}
                            )

# libCrackLang
add_library(libcrack SHARED ${CRACK_SRC_FILES})
set_target_properties(libcrack
                      PROPERTIES
                      OUTPUT_NAME CrackLang
                      SOVERSION ${CRACKLANG_API_VERSION}
                      LINK_FLAGS ${LLVM_LDFLAGS}
                      COMPILE_DEFINITIONS_DEBUG _DEBUG
                      )

target_link_libraries( libcrack
                       ${LLVM_LIBS} dl
                     )

# libCrackNativeRuntime
add_library(libcrackextstub SHARED ext/Stub.cc)
set_target_properties(libcrackextstub
                      PROPERTIES
                      OUTPUT_NAME CrackNativeRuntime
                      SOVERSION ${NATRUN_API_VERSION}
                      LINK_FLAGS ${LLVM_LDFLAGS}
                      COMPILE_DEFINITIONS_DEBUG _DEBUG
                      )
target_link_libraries( libcrackextstub
                       ${LLVM_LIBS}
                     )

# extension config, per autoconf
set(SIZEOF_VOID_P ${CMAKE_SIZEOF_VOID_P})
configure_file("${PROJECT_SOURCE_DIR}/ext/crack_config.h.in"
               "${PROJECT_BINARY_DIR}/crack_config.h")

# test extension. we set this up to mirror the autoconf
# build
add_library(testext SHARED test/testext.cc)
set_target_properties(testext
                      PROPERTIES
                      OUTPUT_NAME testext
                      PREFIX ""
                      )

# runtime extension
add_library( crack-runtime SHARED ${RUNTIME_SRC_FILES} )
set_target_properties(crack-runtime
                      PROPERTIES
                      OUTPUT_NAME runtime
                      LIBRARY_OUTPUT_DIRECTORY lib/crack
                      PREFIX ""
                      )
target_link_libraries(crack-runtime
                      pthread)


add_executable(crack crack_main.cc)
set_target_properties( crack
                       PROPERTIES LINK_FLAGS ${LLVM_LDFLAGS}
                       VERSION ${CRACK_VERSION}
                     )

target_link_libraries( crack
                       libcrack pthread
                     )

add_custom_command(TARGET crack POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E create_symlink crack crackc)

# install targets
install(TARGETS crack libcrack libcrackextstub
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
       )

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/crackc DESTINATION bin)

# runtime
install(TARGETS crack-runtime
         LIBRARY DESTINATION "lib/crack-${CRACK_VERSION}/crack"
        )

# manual
if (EXISTS doc/Manual.html)
    install(FILES doc/Manual.html DESTINATION share/doc/crack)
endif (EXISTS doc/Manual.html)

# header files
install(DIRECTORY compiler/ DESTINATION "include/crack-${CRACK_VERSION}/crack/compiler"
        FILES_MATCHING PATTERN "*.h")
install(DIRECTORY ext/ DESTINATION "include/crack-${CRACK_VERSION}/crack/ext"
        FILES_MATCHING PATTERN "*.h")
install(FILES "${PROJECT_BINARY_DIR}/crack_config.h" DESTINATION "include/crack-${CRACK_VERSION}/crack/ext")

# install library modules
install(DIRECTORY lib/crack DESTINATION "lib/crack-${CRACK_VERSION}"
        FILES_MATCHING PATTERN "*.crk")

add_custom_target(check env CRACK_LIB_PATH=${CMAKE_CURRENT_BUILD_DIR}/lib BUILD_ROOT=${CMAKE_CURRENT_BUILD_DIR}
                            SOURCES_ROOT=${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/test/screen)

add_custom_target(checkc env CRACK_LIB_PATH=${CMAKE_CURRENT_BUILD_DIR}/lib BUILD_ROOT=${CMAKE_CURRENT_BUILD_DIR}
                            SOURCES_ROOT=${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/test/screen -n)

add_subdirectory(opt)

macro_display_feature_log()
