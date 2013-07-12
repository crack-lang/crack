# Detect LLVM and set various variable to link against the different component of LLVM
#
# NOTE: This is a modified version of the module originally found in the OpenGTL project
# at www.opengtl.org
#
# LLVM_BIN_DIR : directory with LLVM binaries
# LLVM_LIB_DIR : directory with LLVM library
# LLVM_INCLUDE_DIR : directory with LLVM include
#
# LLVM_COMPILE_FLAGS : compile flags needed to build a program using LLVM headers
# LLVM_LDFLAGS : ldflags needed to link
# LLVM_LIBS_CORE : ldflags needed to link against a LLVM core library
# LLVM_LIBS_JIT : ldflags needed to link against a LLVM JIT
# LLVM_LIBS_JIT_OBJECTS : objects you need to add to your source when using LLVM JIT

if (LLVM_INCLUDE_DIR)
  set(LLVM_FOUND TRUE)
else (LLVM_INCLUDE_DIR)

  find_program(LLVM_CONFIG_EXECUTABLE
      NAMES llvm-config-${LLVM_MIN_VERSION_TEXT} llvm-config llvm-config-${LLVM_MIN_VERSION_TEXT}
      PATHS /opt/local/bin
      HINTS "$ENV{LLVM_DIR}/bin"
  )
  
#  find_program(LLVM_GXX_EXECUTABLE
#      NAMES llvm-g++ llvmg++
#      PATHS
#      /opt/local/bin
#  )

#  if (LLVM_GXX_EXECUTABLE)
#      MESSAGE(STATUS "LLVM llvm-g++ found at: ${LLVM_GXX_EXECUTABLE}")
#  else(LLVM_GXX_EXECUTABLE)
#      MESSAGE(FATAL_ERROR "LLVM llvm-g++ is required, but not found!")
#  endif(LLVM_GXX_EXECUTABLE)
  
  MACRO(FIND_LLVM_LIBS LLVM_CONFIG_EXECUTABLE _libname_ LIB_VAR OBJECT_VAR)
    exec_program( ${LLVM_CONFIG_EXECUTABLE} ARGS --libs ${_libname_}  OUTPUT_VARIABLE ${LIB_VAR} )
    STRING(REGEX MATCHALL "[^ ]*[.]o[ $]"  ${OBJECT_VAR} ${${LIB_VAR}})
    SEPARATE_ARGUMENTS(${OBJECT_VAR})
    STRING(REGEX REPLACE "[^ ]*[.]o[ $]" ""  ${LIB_VAR} ${${LIB_VAR}})
  ENDMACRO(FIND_LLVM_LIBS)
  
  INCLUDE(TransformVersion)    
  
  exec_program(${LLVM_CONFIG_EXECUTABLE} ARGS --version OUTPUT_VARIABLE LLVM_STRING_VERSION )
  MESSAGE(STATUS "LLVM version: " ${LLVM_STRING_VERSION})
  transform_version(LLVM_VERSION ${LLVM_STRING_VERSION})
  
  exec_program(${LLVM_CONFIG_EXECUTABLE} ARGS --bindir OUTPUT_VARIABLE LLVM_BIN_DIR )
  exec_program(${LLVM_CONFIG_EXECUTABLE} ARGS --libdir OUTPUT_VARIABLE LLVM_LIB_DIR )
  #MESSAGE(STATUS "LLVM lib dir: " ${LLVM_LIB_DIR})
  exec_program(${LLVM_CONFIG_EXECUTABLE} ARGS --includedir OUTPUT_VARIABLE LLVM_INCLUDE_DIR )
      
  exec_program(${LLVM_CONFIG_EXECUTABLE} ARGS --cppflags  OUTPUT_VARIABLE LLVM_COMPILE_FLAGS )
  # strip this from llvm's version, we should add this ourselves in production mode to main CFLAGS
  STRING(REPLACE "-DNDEBUG" "-frtti" LLVM_COMPILE_FLAGS ${LLVM_COMPILE_FLAGS})

  exec_program(${LLVM_CONFIG_EXECUTABLE} ARGS --ldflags   OUTPUT_VARIABLE LLVM_LDFLAGS )

  # strip -ldl this is due to order of link with libcwd
  STRING(REPLACE "-ldl" "" LLVM_LDFLAGS ${LLVM_LDFLAGS})
  MESSAGE(STATUS "LLVM LD flags: " ${LLVM_LDFLAGS})
  
  exec_program(${LLVM_CONFIG_EXECUTABLE} ARGS --libs core jit native bitreader bitwriter instrumentation ipo linker OUTPUT_VARIABLE LLVM_LIBS )
  MESSAGE(STATUS "LLVM libs: " ${LLVM_LIBS})

  if(LLVM_INCLUDE_DIR)
    set(LLVM_FOUND TRUE)
  endif(LLVM_INCLUDE_DIR)
  
  if(LLVM_FOUND)
    message(STATUS "Found LLVM: ${LLVM_INCLUDE_DIR}")
  else(LLVM_FOUND)
    if(LLVM_FIND_REQUIRED)
      message(FATAL_ERROR "Could NOT find LLVM")
    endif(LLVM_FIND_REQUIRED)
  endif(LLVM_FOUND)

endif (LLVM_INCLUDE_DIR)
