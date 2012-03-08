#include <SDL_gfxPrimitives.h>


#include "ext/Module.h"
#include "ext/Type.h"
#include "ext/Func.h"

extern "C"
void crack_ext__sdlgfx_rinit() {
    return;
}

extern "C"
void crack_ext__sdlgfx_cinit(crack::ext::Module *mod) {
    crack::ext::Func *f;
    mod->inject(std::string("import crack.ext._sdl SDL_Surface;"));
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
    crack::ext::Type *type_SDL_Surface = mod->getType("SDL_Surface");
    f = mod->addFunc(type_int, "pixelColor",
                     (void *)pixelColor
                     );
       f->addArg(type_SDL_Surface, "dst");
       f->addArg(type_int16, "x");
       f->addArg(type_int16, "y");
       f->addArg(type_uint32, "color");

    f = mod->addFunc(type_int, "pixelRGBA",
                     (void *)pixelRGBA
                     );
       f->addArg(type_SDL_Surface, "dst");
       f->addArg(type_int16, "x");
       f->addArg(type_int16, "y");
       f->addArg(type_byte, "r");
       f->addArg(type_byte, "g");
       f->addArg(type_byte, "b");
       f->addArg(type_byte, "a");

    f = mod->addFunc(type_int, "hlineColor",
                     (void *)hlineColor
                     );
       f->addArg(type_SDL_Surface, "dst");
       f->addArg(type_int16, "x1");
       f->addArg(type_int16, "x2");
       f->addArg(type_int16, "y");
       f->addArg(type_uint32, "color");

    f = mod->addFunc(type_int, "hlineRGBA",
                     (void *)hlineRGBA
                     );
       f->addArg(type_SDL_Surface, "dst");
       f->addArg(type_int16, "x1");
       f->addArg(type_int16, "x2");
       f->addArg(type_int16, "y");
       f->addArg(type_byte, "r");
       f->addArg(type_byte, "g");
       f->addArg(type_byte, "b");
       f->addArg(type_byte, "a");

    f = mod->addFunc(type_int, "vlineColor",
                     (void *)vlineColor
                     );
       f->addArg(type_SDL_Surface, "dst");
       f->addArg(type_int16, "x");
       f->addArg(type_int16, "y1");
       f->addArg(type_int16, "y2");
       f->addArg(type_uint32, "color");

    f = mod->addFunc(type_int, "vlineRGBA",
                     (void *)vlineRGBA
                     );
       f->addArg(type_SDL_Surface, "dst");
       f->addArg(type_int16, "x");
       f->addArg(type_int16, "y1");
       f->addArg(type_int16, "y2");
       f->addArg(type_byte, "r");
       f->addArg(type_byte, "g");
       f->addArg(type_byte, "b");
       f->addArg(type_byte, "a");

    f = mod->addFunc(type_int, "rectangleColor",
                     (void *)rectangleColor
                     );
       f->addArg(type_SDL_Surface, "dst");
       f->addArg(type_int16, "x1");
       f->addArg(type_int16, "y1");
       f->addArg(type_int16, "x2");
       f->addArg(type_int16, "y2");
       f->addArg(type_uint32, "color");

    f = mod->addFunc(type_int, "rectangleRGBA",
                     (void *)rectangleRGBA
                     );
       f->addArg(type_SDL_Surface, "dst");
       f->addArg(type_int16, "x1");
       f->addArg(type_int16, "y1");
       f->addArg(type_int16, "x2");
       f->addArg(type_int16, "y2");
       f->addArg(type_byte, "r");
       f->addArg(type_byte, "g");
       f->addArg(type_byte, "b");
       f->addArg(type_byte, "a");

    f = mod->addFunc(type_int, "boxColor",
                     (void *)boxColor
                     );
       f->addArg(type_SDL_Surface, "dst");
       f->addArg(type_int16, "x1");
       f->addArg(type_int16, "y1");
       f->addArg(type_int16, "x2");
       f->addArg(type_int16, "y2");
       f->addArg(type_uint32, "color");

    f = mod->addFunc(type_int, "boxRGBA",
                     (void *)boxRGBA
                     );
       f->addArg(type_SDL_Surface, "dst");
       f->addArg(type_int16, "x1");
       f->addArg(type_int16, "y1");
       f->addArg(type_int16, "x2");
       f->addArg(type_int16, "y2");
       f->addArg(type_byte, "r");
       f->addArg(type_byte, "g");
       f->addArg(type_byte, "b");
       f->addArg(type_byte, "a");

    f = mod->addFunc(type_int, "circleColor",
                     (void *)circleColor
                     );
       f->addArg(type_SDL_Surface, "dst");
       f->addArg(type_int16, "x");
       f->addArg(type_int16, "y");
       f->addArg(type_int16, "r");
       f->addArg(type_uint32, "color");

    f = mod->addFunc(type_int, "circleRGBA",
                     (void *)circleRGBA
                     );
       f->addArg(type_SDL_Surface, "dst");
       f->addArg(type_int16, "x");
       f->addArg(type_int16, "y");
       f->addArg(type_int16, "rad");
       f->addArg(type_byte, "r");
       f->addArg(type_byte, "g");
       f->addArg(type_byte, "b");
       f->addArg(type_byte, "a");

    f = mod->addFunc(type_int, "characterColor",
                     (void *)characterColor
                     );
       f->addArg(type_SDL_Surface, "dst");
       f->addArg(type_int16, "x");
       f->addArg(type_int16, "y");
       f->addArg(type_byte, "c");
       f->addArg(type_int32, "color");

    f = mod->addFunc(type_int, "characterRGBA",
                     (void *)characterRGBA
                     );
       f->addArg(type_SDL_Surface, "dst");
       f->addArg(type_int16, "x");
       f->addArg(type_int16, "y");
       f->addArg(type_byte, "c");
       f->addArg(type_byte, "r");
       f->addArg(type_byte, "g");
       f->addArg(type_byte, "b");
       f->addArg(type_byte, "a");

    f = mod->addFunc(type_int, "stringColor",
                     (void *)stringColor
                     );
       f->addArg(type_SDL_Surface, "dst");
       f->addArg(type_int16, "x");
       f->addArg(type_int16, "y");
       f->addArg(type_byteptr, "c");
       f->addArg(type_int32, "color");

    f = mod->addFunc(type_int, "stringRGBA",
                     (void *)stringRGBA
                     );
       f->addArg(type_SDL_Surface, "dst");
       f->addArg(type_int16, "x");
       f->addArg(type_int16, "y");
       f->addArg(type_byteptr, "c");
       f->addArg(type_byte, "r");
       f->addArg(type_byte, "g");
       f->addArg(type_byte, "b");
       f->addArg(type_byte, "a");

    f = mod->addFunc(type_void, "gfxPrimitivesSetFont",
                     (void *)gfxPrimitivesSetFont
                     );
       f->addArg(type_byteptr, "fontdata");
       f->addArg(type_int, "cw");
       f->addArg(type_int, "ch");

}
