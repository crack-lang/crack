# Detect Cairo graphics library
#


if (CAIRO_INCLUDE_DIR)
  set(CAIRO_FOUND TRUE)
else (CAIRO_INCLUDE_DIR)

  find_program(CAIRO_CONFIG_EXECUTABLE
      NAMES pkg-config
      PATHS /opt/local/bin /usr/bin /usr/local/bin
  )
  
  exec_program(${CAIRO_CONFIG_EXECUTABLE} ARGS cairo --cflags OUTPUT_VARIABLE CAIRO_CFLAGS )
  exec_program(${CAIRO_CONFIG_EXECUTABLE} ARGS cairo --libs   OUTPUT_VARIABLE CAIRO_LDFLAGS )

  if(CAIRO_CFLAGS)
    set(CAIRO_FOUND TRUE)
  endif(CAIRO_CFLAGS)
  
  if(CAIRO_FOUND)
    message(STATUS "Found cairo")
  else(CAIRO_FOUND)
    if(CAIRO_FIND_REQUIRED)
      message(FATAL_ERROR "Could NOT find cairo")
    endif(CAIRO_FIND_REQUIRED)
  endif(CAIRO_FOUND)

endif (CAIRO_INCLUDE_DIR)
