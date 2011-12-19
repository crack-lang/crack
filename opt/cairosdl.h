#ifndef CAIROSDL_H
#define CAIROSDL_H
/*
 * Copyright (c) 2009  M Joonas Pihlaja
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */
#include <cairo/cairo.h>
#include <SDL/SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

/* All cairo and cairosdl functions expect the underlying SDL_Surface
 * to be locked or not need locking.  The underlying condition is that
 * the ->pixels member of the SDL_Surface should be valid and not
 * change during the lifetime of the cairo_surface_t representing it,
 * and that malloc and whatever other OS facilities are allowed to be
 * called. */

/* Create a cairo image surface and bind the SDL_Surface to it.  If
 * the pixel format of the SDL_Surface isn't supported by cairo,
 * returns a surface in CAIRO_STATUS_INVALID_FORMAT error state. */
cairo_surface_t *
cairosdl_surface_create (SDL_Surface *sdl_surface);

/* Returns the SDL_Surface bound to the image surface. */
SDL_Surface *
cairosdl_surface_get_target (cairo_surface_t *surface);


/* These functions are noops for Amask=0 surfaces.  For
 * Amask=0xFF000000 surfaces they write the indicated area(s) of the
 * SDL_Surface bound to the surface from a backing buffer. */
void
cairosdl_surface_flush_rects (cairo_surface_t *surface,
                              int              num_rects,
                              SDL_Rect const  *rects);

void
cairosdl_surface_flush_rect (cairo_surface_t *surface,
                             int              x,
                             int              y,
                             int              width,
                             int              height);

void
cairosdl_surface_flush (cairo_surface_t *surface);


/* These functions are noops for Amask=0 surfaces.  For
 * Amask=0xFF000000 surfaces they read the indicated area(s) from the
 * SDL_Surface bound to the surface into a backing buffer. */
void
cairosdl_surface_mark_dirty_rects (cairo_surface_t *surface,
                                   int              num_rects,
                                   SDL_Rect const  *rects);

void
cairosdl_surface_mark_dirty_rect (cairo_surface_t *surface,
                                  int              x,
                                  int              y,
                                  int              width,
                                  int              height);

void
cairosdl_surface_mark_dirty (cairo_surface_t *surface);


/* Context convenience functions. */

/* Equivalent to cairo_create(cairosdl_surface_create(sdl_surface)); */
cairo_t *
cairosdl_create (SDL_Surface *sdl_surface);

/* Equivalent to cairosdl_surface_get_target(cairo_get_target(cr));  */
SDL_Surface *
cairosdl_get_target (cairo_t *cr);

/* Calls cairosdl_surface_flush() on the target of the cairo context
 * and then destroys the context. */
void
cairosdl_destroy (cairo_t *cr);


/* Cairo pixel configuration.  This isn't tweakable, it just is. */
#define CAIROSDL_ASHIFT 24
#define CAIROSDL_RSHIFT 16
#define CAIROSDL_GSHIFT  8
#define CAIROSDL_BSHIFT  0
#define CAIROSDL_AMASK (255U << CAIROSDL_ASHIFT)
#define CAIROSDL_RMASK (255U << CAIROSDL_RSHIFT)
#define CAIROSDL_GMASK (255U << CAIROSDL_GSHIFT)
#define CAIROSDL_BMASK (255U << CAIROSDL_BSHIFT)

#ifdef __cplusplus
}
#endif
#endif /* CAIROSDL_H */
