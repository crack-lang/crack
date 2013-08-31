# optional extensions

set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib/crack/ext)

# GTK+2
SET(GTK2_ADDITIONAL_SUFFIXES x86_64-linux-gnu/glib-2.0 gdk-pixbuf-2.0)
macro_optional_find_package(GTK2 2.6 COMPONENTS gtk)
macro_log_feature(GTK2_FOUND "gtk2+" "Bindings for gtk2+" "http://www.gtk.org/")

if (GTK2_FOUND)
    include_directories(${GTK2_INCLUDE_DIRS})
    add_library(gtk SHARED _gtk.cc)
    set_target_properties(gtk
                          PROPERTIES
                          OUTPUT_NAME _gtk
                          PREFIX ""
                          )
    target_link_libraries(gtk
                          ${GTK2_LIBRARIES}
                         )
    install(TARGETS gtk DESTINATION "lib/crack-${CRACK_VERSION}/crack/ext")
endif(GTK2_FOUND)

# PCRE
macro_optional_find_package(PCRE)
macro_log_feature(PCRE_FOUND "PCRE" "Bindings for PCRE" "http://www.pcre.org/")

if (PCRE_FOUND)
    include_directories(${PCRE_INCLUDE_DIR})
    add_library(pcre SHARED _pcre.cc)
    set_target_properties(pcre
                          PROPERTIES
                          OUTPUT_NAME _pcre
                          PREFIX ""
                          LINK_FLAGS ${LLVM_LDFLAGS}
                         )
    target_link_libraries(pcre
                          libcrack ${PCRE_LIBRARIES}
                         )
    install(TARGETS pcre DESTINATION "lib/crack-${CRACK_VERSION}/crack/ext")
endif(PCRE_FOUND)

# libCurl
macro_optional_find_package(CURL)
macro_log_feature(CURL_FOUND "CURL" "Bindings for libcurl"
                             "http://curl.haxx.se/libcurl/c/"
                 )
if (CURL_FOUND)
    include_directories(${CURL_INCLUDE_DIR})
    add_library(curl SHARED _curl.cc)
    set_target_properties(curl
                          PROPERTIES
                          OUTPUT_NAME _curl
                          PREFIX ""
                         )
    target_link_libraries(curl
                          ${CURL_LIBRARY}
                         )
    install(TARGETS curl DESTINATION "lib/crack-${CRACK_VERSION}/crack/ext")
endif(CURL_FOUND)

# Mongo client
macro_optional_find_package(Mongo)
macro_log_feature(MONGO_FOUND "mongo" "Bindings for libmongo-client"
                              "http://algernon.github.com/libmongo-client/"
                 )
if (MONGO_FOUND)
    include_directories(${GLIB_CONFIG_DIR} ${GLIB_INCLUDE_DIR}
                        ${MONGO_INCLUDE_DIR}
                        )
    add_library(mongo SHARED _mongo.cc)
    set_target_properties(mongo
                          PROPERTIES
                          OUTPUT_NAME _mongo
                          PREFIX ""
                         )
    target_link_libraries(mongo
                          ${MONGO_LIBRARIES}
                         )
    install(TARGETS mongo DESTINATION "lib/crack-${CRACK_VERSION}/crack/ext")
endif(MONGO_FOUND)


# SDL
macro_optional_find_package(SDL)
macro_log_feature(SDL_FOUND "SDL" "Bindings for SDL" "http://www.libsdl.org/")

if (SDL_FOUND)
    include_directories(${SDL_INCLUDE_DIR})
    add_library(sdl SHARED _sdl.cc)
    set_target_properties(sdl
                          PROPERTIES
                          OUTPUT_NAME _sdl
                          PREFIX ""
                         )
    target_link_libraries( sdl
                            ${SDL_LIBRARY}
                        )
    install(TARGETS sdl DESTINATION "lib/crack-${CRACK_VERSION}/crack/ext")
endif (SDL_FOUND)

# OpenGL
macro_optional_find_package(OpenGL)
macro_log_feature(OPENGL_FOUND "OpenGL" "Bindings for OpenGL"
                               "http://www.opengl.org/"
                 )

if (OPENGL_FOUND)
    include_directories(${OPENGL_INCLUDE_DIR})
    add_library(gl SHARED _gl.cc)
    set_target_properties(gl
                          PROPERTIES
                          OUTPUT_NAME _gl
                          PREFIX ""
                         )
    target_link_libraries(gl
                          ${OPENGL_LIBRARIES}
                         )
    install(TARGETS gl DESTINATION "lib/crack-${CRACK_VERSION}/crack/ext")
endif (OPENGL_FOUND)

# Crossroads I/O
macro_optional_find_package(XS)
macro_log_feature(XS_FOUND "xs" "Bindings for Crossroads I/O"
                           "http://crossroads.io"
                  )

if (XS_FOUND)
    add_library(xs SHARED _xs.cc)
    set_target_properties(xs
                          PROPERTIES
                          OUTPUT_NAME _xs
                          PREFIX ""
                         )
    target_link_libraries(xs
                          -lxs
                         )
    install(TARGETS xs DESTINATION "lib/crack-${CRACK_VERSION}/crack/ext")
endif (XS_FOUND)

# LMDB key/value store
macro_optional_find_package(LMDB)
macro_log_feature(LMDB_FOUND "lmdb" "Bindings for LMDB"
                           "http://symas.com/mdb/"
                  )

if (LMDB_FOUND)
    add_library(lmdb SHARED _lmdb.cc)
    set_target_properties(lmdb
                          PROPERTIES
                          OUTPUT_NAME _lmdb
                          PREFIX ""
                         )
    target_link_libraries(lmdb
                          -llmdb
                         )
    install(TARGETS lmdb DESTINATION "lib/crack-${CRACK_VERSION}/crack/ext")
endif (LMDB_FOUND)


# Packages that depend on pkg-config
find_package(PkgConfig)

if (PKG_CONFIG_FOUND)
    # Cairi
    # TODO: write a *proper* FindCairo.cmake file or copy one over from another
    # project
    option(WITH_Cairo "Search for cairo package" ON)

    if (WITH_Cairo)
        pkg_search_module(CAIRO cairo >= 1.10)
    else (WITH_Cairo)
        set(CAIRO_FOUND FALSE)
    endif (WITH_Cairo)

    macro_log_feature(CAIRO_FOUND "Cairo" "Bindings for Cairo"
                                  "http://www.cairographics.org/"
                     )

    if (CAIRO_FOUND)
        add_library(cairo SHARED _cairo.cc)
        set_target_properties(cairo
                              PROPERTIES
                              OUTPUT_NAME _cairo
                              PREFIX ""
                             )
        target_link_libraries(cairo
                              ${CAIRO_LDFLAGS}
                             )
        install(TARGETS cairo DESTINATION
                        "lib/crack-${CRACK_VERSION}/crack/ext"
               )
        macro_log_feature(SDL_FOUND "CairoSDL" "Bindings for Cairo/SDL"
                                    "http://www.cairographics.org/"
                         )
        if (SDL_FOUND)
            add_library(cairosdl SHARED _cairosdl.cc cairosdl.c)
            set_target_properties(cairosdl
                                  PROPERTIES
                                  OUTPUT_NAME _cairosdl
                                  PREFIX ""
                                 )
            target_link_libraries(cairosdl
                                  ${CAIRO_LDFLAGS}
                                  ${SDL_LIBRARY}
                                 )
            install(TARGETS cairosdl DESTINATION
                            "lib/crack-${CRACK_VERSION}/crack/ext"
                    )
        endif (SDL_FOUND)

    endif (CAIRO_FOUND)
endif (PKG_CONFIG_FOUND)

# libPNG
find_package(PNG)
macro_log_feature(PNG_FOUND "png" "Bindings for libpng" "http://www.libpng.org")
if (PNG_FOUND)
    add_library(png SHARED _png.cc)
    set_target_properties(png
                          PROPERTIES
                          OUTPUT_NAME _png
                          PREFIX ""
                          LINK_FLAGS ${LLVM_LDFLAGS}
                         )
    target_link_libraries( png
                           libcrack ${PNG_LIBRARY} 
                         )
    install(TARGETS png DESTINATION "lib/crack-${CRACK_VERSION}/crack/ext")

endif (PNG_FOUND)

# libPNG
find_package(NETCDF)
macro_log_feature(NETCDF_FOUND "netcdf" "Bindings for NetCDF 4.x" "http://www.unidata.ucar.edu/software/netcdf/")
if (NETCDF_FOUND)
    add_library(netcdf SHARED _netcdf.cc)
    set_target_properties(netcdf
                          PROPERTIES
                          OUTPUT_NAME _netcdf
                          PREFIX ""
                          LINK_FLAGS ${LLVM_LDFLAGS}
                         )
    target_link_libraries( netcdf
                           libcrack ${NETCDF_LIBRARY} 
                         )
    install(TARGETS netcdf DESTINATION "lib/crack-${CRACK_VERSION}/crack/ext")

endif (NETCDF_FOUND)
