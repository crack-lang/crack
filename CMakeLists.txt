# root

cmake_minimum_required(VERSION 2.6)

# this policy warning is due to lib command lines arg output from llvm-config
if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

project (crack)

set(CRACK_VERSION "0.1")

set(LLVM_MIN_VERSION       "2007000")
set(LLVM_MIN_VERSION_TEXT  "2.7")

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules )

set(CMAKE_CXX_FLAGS_DEBUG "-fexceptions -g -Wall -Winline -W -Wwrite-strings -Wno-unused")

MESSAGE( STATUS "PROJECT_SOURCE_DIR: " ${PROJECT_SOURCE_DIR} )
MESSAGE( STATUS "CMAKE_MODULE_PATH: " ${CMAKE_MODULE_PATH} )

MESSAGE( STATUS "CMAKE_SYSTEM_PROCESSOR: " ${CMAKE_SYSTEM_PROCESSOR} )

Find_Package(LLVM REQUIRED)
Find_Package(Spug REQUIRED)

include_directories(${PROJECT_SOURCE_DIR} ${LLVM_INCLUDE_DIR} ${SPUG_INCLUDE_DIR})
#link_directories(${ICU_LIB_DIRS})

IF( LLVM_VERSION LESS ${LLVM_MIN_VERSION} )
  MESSAGE(FATAL_ERROR "LLVM version ${LLVM_STRING_VERSION} is too old, please install ${LLVM_MIN_VERSION_TEXT} of greater!")
ENDIF( LLVM_VERSION LESS ${LLVM_MIN_VERSION} )

add_definitions(-DLLVM_VERSION=${LLVM_VERSION})
add_definitions("-DCRACKLIB=\"${CMAKE_INSTALL_PREFIX}/lib/crack-${CRACK_VERSION}\"")


set(CRACK_SRC_FILES
  builder/LLVMBuilder.cc
  model/StrConst.cc
  model/AssignExpr.cc
  model/VarRef.cc
  model/FuncCall.cc
  model/Expr.cc
  model/OverloadDef.cc
  model/ResultExpr.cc
  model/Initializers.cc
  model/AllocExpr.cc
  model/IntConst.cc
  model/FloatConst.cc
  model/VarDef.cc
  model/Context.cc
  model/NullConst.cc
  model/CleanupFrame.cc
  model/TypeDef.cc
  model/FuncDef.cc
  model/ModuleDef.cc
  parser/Toker.cc
  parser/Parser.cc
  parser/ParseError.cc
  parser/Token.cc
  #parser/Location.cc
  Crack.cc  
  crack.cc
)

# these are llvm specific compile flags, needed only for source files that
# include llvm headers
set_source_files_properties( ${CRACK_SRC_FILES}
                             PROPERTIES COMPILE_FLAGS ${LLVM_COMPILE_FLAGS}
                            )

add_executable( crack ${CRACK_SRC_FILES} )

set_target_properties( crack
                       PROPERTIES LINK_FLAGS ${LLVM_LDFLAGS}
                     )
                     
target_link_libraries( crack
			${SPUG_LIBRARIES}
            ${LLVM_LIBS}
                      ) 
