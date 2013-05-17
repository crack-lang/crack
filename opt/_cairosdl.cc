#include "opt/cairosdl.h"
void *cairosdl_surface_create_void (void *sdl_surface){
return cairosdl_surface_create((SDL_Surface *)sdl_surface);
}


#include "ext/Module.h"
#include "ext/Type.h"
#include "ext/Func.h"

extern "C"
void crack_ext__cairosdl_rinit() {
    return;
}

extern "C"
void crack_ext__cairosdl_cinit(crack::ext::Module *mod) {
    crack::ext::Func *f;
    mod->inject(std::string("import crack.ext._cairo cairo_t, cairo_surface_t; import crack.ext._sdl SDL_Surface, SDL_Rect;"));
    crack::ext::Type *type_Class = mod->getClassType();
    crack::ext::Type *type_void = mod->getVoidType();
    crack::ext::Type *type_voidptr = mod->getVoidptrType();
    crack::ext::Type *type_bool = mod->getBoolType();
    crack::ext::Type *type_byteptr = mod->getByteptrType();
    crack::ext::Type *type_byte = mod->getByteType();
    crack::ext::Type *type_int16 = mod->getInt16Type();
    crack::ext::Type *type_int32 = mod->getInt32Type();
    crack::ext::Type *type_int64 = mod->getInt64Type();
    crack::ext::Type *type_uint16 = mod->getUint16Type();
    crack::ext::Type *type_uint32 = mod->getUint32Type();
    crack::ext::Type *type_uint64 = mod->getUint64Type();
    crack::ext::Type *type_int = mod->getIntType();
    crack::ext::Type *type_uint = mod->getUintType();
    crack::ext::Type *type_intz = mod->getIntzType();
    crack::ext::Type *type_uintz = mod->getUintzType();
    crack::ext::Type *type_float32 = mod->getFloat32Type();
    crack::ext::Type *type_float64 = mod->getFloat64Type();
    crack::ext::Type *type_float = mod->getFloatType();
    crack::ext::Type *type_cairo_t = mod->getType("cairo_t");
    crack::ext::Type *type_cairo_surface_t = mod->getType("cairo_surface_t");
    crack::ext::Type *type_SDL_Surface = mod->getType("SDL_Surface");
    crack::ext::Type *type_SDL_Rect = mod->getType("SDL_Rect");

    crack::ext::Type *array = mod->getType("array");

    crack::ext::Type *array_pSDL__Rect_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_SDL_Rect;
        array_pSDL__Rect_q = array->getSpecialization(params);
    }
    f = mod->addFunc(type_cairo_surface_t, "cairosdl_surface_create",
                     (void *)cairosdl_surface_create
                     );
       f->addArg(type_SDL_Surface, "sdl_surface");

    f = mod->addFunc(type_SDL_Surface, "cairosdl_surface_get_target",
                     (void *)cairosdl_surface_get_target
                     );
       f->addArg(type_cairo_surface_t, "surface");

    f = mod->addFunc(type_void, "cairosdl_surface_flush_rects",
                     (void *)cairosdl_surface_flush_rects
                     );
       f->addArg(type_cairo_surface_t, "surface");
       f->addArg(type_int, "num_rects");
       f->addArg(array_pSDL__Rect_q, "rects");

    f = mod->addFunc(type_void, "cairosdl_surface_flush_rect",
                     (void *)cairosdl_surface_flush_rect
                     );
       f->addArg(type_cairo_surface_t, "surface");
       f->addArg(type_int, "x");
       f->addArg(type_int, "y");
       f->addArg(type_int, "width");
       f->addArg(type_int, "height");

    f = mod->addFunc(type_void, "cairosdl_surface_flush",
                     (void *)cairosdl_surface_flush
                     );
       f->addArg(type_cairo_surface_t, "surface");

    f = mod->addFunc(type_void, "cairosdl_surface_mark_dirty_rects",
                     (void *)cairosdl_surface_mark_dirty_rects
                     );
       f->addArg(type_cairo_surface_t, "surface");
       f->addArg(type_int, "num_rects");
       f->addArg(array_pSDL__Rect_q, "rects");

    f = mod->addFunc(type_void, "cairosdl_surface_mark_dirty_rect",
                     (void *)cairosdl_surface_mark_dirty_rect
                     );
       f->addArg(type_cairo_surface_t, "surface");
       f->addArg(type_int, "x");
       f->addArg(type_int, "y");
       f->addArg(type_int, "width");
       f->addArg(type_int, "height");

    f = mod->addFunc(type_void, "cairosdl_surface_mark_dirty",
                     (void *)cairosdl_surface_mark_dirty
                     );
       f->addArg(type_cairo_surface_t, "surface");

    f = mod->addFunc(type_cairo_t, "cairosdl_create",
                     (void *)cairosdl_create
                     );
       f->addArg(type_SDL_Surface, "sdl_surface");

    f = mod->addFunc(type_SDL_Surface, "cairosdl_get_target",
                     (void *)cairosdl_get_target
                     );
       f->addArg(type_cairo_t, "cr");

    f = mod->addFunc(type_void, "cairosdl_destroy",
                     (void *)cairosdl_destroy
                     );
       f->addArg(type_cairo_t, "cr");


    mod->addConstant(type_uint32, "CAIROSDL_ASHIFT",
                     static_cast<int>(24)
                     );

    mod->addConstant(type_uint32, "CAIROSDL_RSHIFT",
                     static_cast<int>(16)
                     );

    mod->addConstant(type_uint32, "CAIROSDL_GSHIFT",
                     static_cast<int>(8)
                     );

    mod->addConstant(type_uint32, "CAIROSDL_BSHIFT",
                     static_cast<int>(0)
                     );

    mod->addConstant(type_uint32, "CAIROSDL_AMASK",
                     static_cast<int>(4278190080)
                     );

    mod->addConstant(type_uint32, "CAIROSDL_RMASK",
                     static_cast<int>(16711680)
                     );

    mod->addConstant(type_uint32, "CAIROSDL_GMASK",
                     static_cast<int>(65280)
                     );

    mod->addConstant(type_uint32, "CAIROSDL_BMASK",
                     static_cast<int>(0)
                     );
}
