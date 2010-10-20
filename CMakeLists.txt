# root

cmake_minimum_required(VERSION 2.6)

# this policy warning is due to lib command lines arg output from llvm-config
if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

project (crack)

set(CRACK_VERSION "0.2.1")
set(API_VERSION   "0.0.0")

set(LLVM_MIN_VERSION       "2008000")
set(LLVM_MIN_VERSION_TEXT  "2.8")

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules )

#set(CMAKE_CXX_FLAGS_DEBUG "-fexceptions -g -Wall -Winline -W -Wwrite-strings -Wno-unused")

MESSAGE( STATUS "PROJECT_SOURCE_DIR: " ${PROJECT_SOURCE_DIR} )
MESSAGE( STATUS "CMAKE_MODULE_PATH: " ${CMAKE_MODULE_PATH} )

MESSAGE( STATUS "CMAKE_SYSTEM_PROCESSOR: " ${CMAKE_SYSTEM_PROCESSOR} )

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
                      OUTPUT_NAME crack
                      VERSION ${CRACK_VERSION}
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
install(TARGETS crack libcrack crack-runtime
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
       )
install(FILES doc/Manual.html DESTINATION share/doc/crack)
# todo: a better scheme that saves directory structure of lib directory
FILE(GLOB lib_files_root lib/crack/*.crk)
install(FILES ${lib_files_root} DESTINATION "lib/crack-${CRACK_VERSION}/crack")
FILE(GLOB lib_files_exp lib/crack/exp/*.crk)
install(FILES ${lib_files_exp} DESTINATION "lib/crack-${CRACK_VERSION}/crack/exp")

# test suite
add_custom_target(check 
                  ${PROJECT_SOURCE_DIR}/test/run_all_tests
                  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
		  )

add_custom_target(screen
                  ${PROJECT_SOURCE_DIR}/crack ${PROJECT_SOURCE_DIR}/screen/screen.crk ${PROJECT_SOURCE_DIR}/screen/tests/
                  )
