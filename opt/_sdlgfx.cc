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

    crack::ext::Type *array = mod->getType("array");

    crack::ext::Type *array_pint16_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_int16;
        array_pint16_q = array->getSpecialization(params);
    }

    crack::ext::Type *array_pint_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_int;
        array_pint_q = array->getSpecialization(params);
    }

    crack::ext::Type *array_parray_pint_q_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = array_pint_q;
        array_parray_pint_q_q = array->getSpecialization(params);
    }
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

    f = mod->addFunc(type_int, "lineColor",
                     (void *)lineColor
                     );
       f->addArg(type_SDL_Surface, "dst");
       f->addArg(type_int16, "x1");
       f->addArg(type_int16, "y1");
       f->addArg(type_int16, "x2");
       f->addArg(type_int16, "y2");
       f->addArg(type_uint32, "color");

    f = mod->addFunc(type_int, "lineRGBA",
                     (void *)lineRGBA
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

    f = mod->addFunc(type_int, "aalineColor",
                     (void *)aalineColor
                     );
       f->addArg(type_SDL_Surface, "dst");
       f->addArg(type_int16, "x1");
       f->addArg(type_int16, "y1");
       f->addArg(type_int16, "x2");
       f->addArg(type_int16, "y2");
       f->addArg(type_uint32, "color");

    f = mod->addFunc(type_int, "aalineRGBA",
                     (void *)aalineRGBA
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

    f = mod->addFunc(type_int, "aacircleColor",
                     (void *)aacircleColor
                     );
       f->addArg(type_SDL_Surface, "dst");
       f->addArg(type_int16, "x");
       f->addArg(type_int16, "y");
       f->addArg(type_int16, "r");
       f->addArg(type_uint32, "color");

    f = mod->addFunc(type_int, "aacircleRGBA",
                     (void *)aacircleRGBA
                     );
       f->addArg(type_SDL_Surface, "dst");
       f->addArg(type_int16, "x");
       f->addArg(type_int16, "y");
       f->addArg(type_int16, "rad");
       f->addArg(type_byte, "r");
       f->addArg(type_byte, "g");
       f->addArg(type_byte, "b");
       f->addArg(type_byte, "a");

    f = mod->addFunc(type_int, "filledCircleColor",
                     (void *)filledCircleColor
                     );
       f->addArg(type_SDL_Surface, "dst");
       f->addArg(type_int16, "x");
       f->addArg(type_int16, "y");
       f->addArg(type_int16, "r");
       f->addArg(type_uint32, "color");

    f = mod->addFunc(type_int, "filledCircleRGBA",
                     (void *)filledCircleRGBA
                     );
       f->addArg(type_SDL_Surface, "dst");
       f->addArg(type_int16, "x");
       f->addArg(type_int16, "y");
       f->addArg(type_int16, "rad");
       f->addArg(type_byte, "r");
       f->addArg(type_byte, "g");
       f->addArg(type_byte, "b");
       f->addArg(type_byte, "a");

    f = mod->addFunc(type_int, "ellipseColor",
                     (void *)ellipseColor
                     );
       f->addArg(type_SDL_Surface, "dst");
       f->addArg(type_int16, "x");
       f->addArg(type_int16, "y");
       f->addArg(type_int16, "rx");
       f->addArg(type_int16, "ry");
       f->addArg(type_uint32, "color");

    f = mod->addFunc(type_int, "ellipseRGBA",
                     (void *)ellipseRGBA
                     );
       f->addArg(type_SDL_Surface, "dst");
       f->addArg(type_int16, "x");
       f->addArg(type_int16, "y");
       f->addArg(type_int16, "rx");
       f->addArg(type_int16, "ry");
       f->addArg(type_byte, "r");
       f->addArg(type_byte, "g");
       f->addArg(type_byte, "b");
       f->addArg(type_byte, "a");

    f = mod->addFunc(type_int, "aaellipseColor",
                     (void *)aaellipseColor
                     );
       f->addArg(type_SDL_Surface, "dst");
       f->addArg(type_int16, "xc");
       f->addArg(type_int16, "yc");
       f->addArg(type_int16, "rx");
       f->addArg(type_int16, "ry");
       f->addArg(type_uint32, "color");

    f = mod->addFunc(type_int, "aaellipseRGBA",
                     (void *)aaellipseRGBA
                     );
       f->addArg(type_SDL_Surface, "dst");
       f->addArg(type_int16, "x");
       f->addArg(type_int16, "y");
       f->addArg(type_int16, "rx");
       f->addArg(type_int16, "ry");
       f->addArg(type_byte, "r");
       f->addArg(type_byte, "g");
       f->addArg(type_byte, "b");
       f->addArg(type_byte, "a");

    f = mod->addFunc(type_int, "filledEllipseColor",
                     (void *)filledEllipseColor
                     );
       f->addArg(type_SDL_Surface, "dst");
       f->addArg(type_int16, "x");
       f->addArg(type_int16, "y");
       f->addArg(type_int16, "rx");
       f->addArg(type_int16, "ry");
       f->addArg(type_uint32, "color");

    f = mod->addFunc(type_int, "filledEllipseRGBA",
                     (void *)filledEllipseRGBA
                     );
       f->addArg(type_SDL_Surface, "dst");
       f->addArg(type_int16, "x");
       f->addArg(type_int16, "y");
       f->addArg(type_int16, "rx");
       f->addArg(type_int16, "ry");
       f->addArg(type_byte, "r");
       f->addArg(type_byte, "g");
       f->addArg(type_byte, "b");
       f->addArg(type_byte, "a");

    f = mod->addFunc(type_int, "pieColor",
                     (void *)pieColor
                     );
       f->addArg(type_SDL_Surface, "dst");
       f->addArg(type_int16, "x");
       f->addArg(type_int16, "y");
       f->addArg(type_int16, "rad");
       f->addArg(type_int16, "start");
       f->addArg(type_int16, "end");
       f->addArg(type_uint32, "color");

    f = mod->addFunc(type_int, "pieRGBA",
                     (void *)pieRGBA
                     );
       f->addArg(type_SDL_Surface, "dst");
       f->addArg(type_int16, "x");
       f->addArg(type_int16, "y");
       f->addArg(type_int16, "rad");
       f->addArg(type_int16, "start");
       f->addArg(type_int16, "end");
       f->addArg(type_byte, "r");
       f->addArg(type_byte, "g");
       f->addArg(type_byte, "b");
       f->addArg(type_byte, "a");

    f = mod->addFunc(type_int, "filledPieColor",
                     (void *)filledPieColor
                     );
       f->addArg(type_SDL_Surface, "dst");
       f->addArg(type_int16, "x");
       f->addArg(type_int16, "y");
       f->addArg(type_int16, "rad");
       f->addArg(type_int16, "start");
       f->addArg(type_int16, "end");
       f->addArg(type_uint32, "color");

    f = mod->addFunc(type_int, "filledPieRGBA",
                     (void *)filledPieRGBA
                     );
       f->addArg(type_SDL_Surface, "dst");
       f->addArg(type_int16, "x");
       f->addArg(type_int16, "y");
       f->addArg(type_int16, "rad");
       f->addArg(type_int16, "start");
       f->addArg(type_int16, "end");
       f->addArg(type_byte, "r");
       f->addArg(type_byte, "g");
       f->addArg(type_byte, "b");
       f->addArg(type_byte, "a");

    f = mod->addFunc(type_int, "trigonColor",
                     (void *)trigonColor
                     );
       f->addArg(type_SDL_Surface, "dst");
       f->addArg(type_int16, "x1");
       f->addArg(type_int16, "y1");
       f->addArg(type_int16, "x2");
       f->addArg(type_int16, "y2");
       f->addArg(type_int16, "x3");
       f->addArg(type_int16, "y3");
       f->addArg(type_uint32, "color");

    f = mod->addFunc(type_int, "trigonRGBA",
                     (void *)trigonRGBA
                     );
       f->addArg(type_SDL_Surface, "dst");
       f->addArg(type_int16, "x1");
       f->addArg(type_int16, "y1");
       f->addArg(type_int16, "x2");
       f->addArg(type_int16, "y2");
       f->addArg(type_int16, "x3");
       f->addArg(type_int16, "y3");
       f->addArg(type_byte, "r");
       f->addArg(type_byte, "g");
       f->addArg(type_byte, "b");
       f->addArg(type_byte, "a");

    f = mod->addFunc(type_int, "aatrigonColor",
                     (void *)aatrigonColor
                     );
       f->addArg(type_SDL_Surface, "dst");
       f->addArg(type_int16, "x1");
       f->addArg(type_int16, "y1");
       f->addArg(type_int16, "x2");
       f->addArg(type_int16, "y2");
       f->addArg(type_int16, "x3");
       f->addArg(type_int16, "y3");
       f->addArg(type_uint32, "color");

    f = mod->addFunc(type_int, "aatrigonRGBA",
                     (void *)aatrigonRGBA
                     );
       f->addArg(type_SDL_Surface, "dst");
       f->addArg(type_int16, "x1");
       f->addArg(type_int16, "y1");
       f->addArg(type_int16, "x2");
       f->addArg(type_int16, "y2");
       f->addArg(type_int16, "x3");
       f->addArg(type_int16, "y3");
       f->addArg(type_byte, "r");
       f->addArg(type_byte, "g");
       f->addArg(type_byte, "b");
       f->addArg(type_byte, "a");

    f = mod->addFunc(type_int, "filledTrigonColor",
                     (void *)filledTrigonColor
                     );
       f->addArg(type_SDL_Surface, "dst");
       f->addArg(type_int16, "x1");
       f->addArg(type_int16, "y1");
       f->addArg(type_int16, "x2");
       f->addArg(type_int16, "y2");
       f->addArg(type_int16, "x3");
       f->addArg(type_int16, "y3");
       f->addArg(type_uint32, "color");

    f = mod->addFunc(type_int, "filledTrigonRGBA",
                     (void *)filledTrigonRGBA
                     );
       f->addArg(type_SDL_Surface, "dst");
       f->addArg(type_int16, "x1");
       f->addArg(type_int16, "y1");
       f->addArg(type_int16, "x2");
       f->addArg(type_int16, "y2");
       f->addArg(type_int16, "x3");
       f->addArg(type_int16, "y3");
       f->addArg(type_byte, "r");
       f->addArg(type_byte, "g");
       f->addArg(type_byte, "b");
       f->addArg(type_byte, "a");

    f = mod->addFunc(type_int, "polygonColor",
                     (void *)polygonColor
                     );
       f->addArg(type_SDL_Surface, "dst");
       f->addArg(array_pint16_q, "vx");
       f->addArg(array_pint16_q, "vy");
       f->addArg(type_int, "n");
       f->addArg(type_uint32, "color");

    f = mod->addFunc(type_int, "polygonRGBA",
                     (void *)polygonRGBA
                     );
       f->addArg(type_SDL_Surface, "dst");
       f->addArg(array_pint16_q, "vx");
       f->addArg(array_pint16_q, "vy");
       f->addArg(type_int, "n");
       f->addArg(type_byte, "r");
       f->addArg(type_byte, "g");
       f->addArg(type_byte, "b");
       f->addArg(type_byte, "a");

    f = mod->addFunc(type_int, "aapolygonColor",
                     (void *)aapolygonColor
                     );
       f->addArg(type_SDL_Surface, "dst");
       f->addArg(array_pint16_q, "vx");
       f->addArg(array_pint16_q, "vy");
       f->addArg(type_int, "n");
       f->addArg(type_uint32, "color");

    f = mod->addFunc(type_int, "aapolygonRGBA",
                     (void *)aapolygonRGBA
                     );
       f->addArg(type_SDL_Surface, "dst");
       f->addArg(array_pint16_q, "vx");
       f->addArg(array_pint16_q, "vy");
       f->addArg(type_int, "n");
       f->addArg(type_byte, "r");
       f->addArg(type_byte, "g");
       f->addArg(type_byte, "b");
       f->addArg(type_byte, "a");

    f = mod->addFunc(type_int, "filledPolygonColor",
                     (void *)filledPolygonColor
                     );
       f->addArg(type_SDL_Surface, "dst");
       f->addArg(array_pint16_q, "vx");
       f->addArg(array_pint16_q, "vy");
       f->addArg(type_int, "n");
       f->addArg(type_uint32, "color");

    f = mod->addFunc(type_int, "filledPolygonRGBA",
                     (void *)filledPolygonRGBA
                     );
       f->addArg(type_SDL_Surface, "dst");
       f->addArg(array_pint16_q, "vx");
       f->addArg(array_pint16_q, "vy");
       f->addArg(type_int, "n");
       f->addArg(type_byte, "r");
       f->addArg(type_byte, "g");
       f->addArg(type_byte, "b");
       f->addArg(type_byte, "a");

    f = mod->addFunc(type_int, "texturedPolygon",
                     (void *)texturedPolygon
                     );
       f->addArg(type_SDL_Surface, "dst");
       f->addArg(array_pint16_q, "vx");
       f->addArg(array_pint16_q, "vy");
       f->addArg(type_int, "n");
       f->addArg(type_SDL_Surface, "texture");
       f->addArg(type_int, "texture_dx");
       f->addArg(type_int, "texture_dy");

    f = mod->addFunc(type_int, "filledPolygonColorMT",
                     (void *)filledPolygonColorMT
                     );
       f->addArg(type_SDL_Surface, "dst");
       f->addArg(array_pint16_q, "vx");
       f->addArg(array_pint16_q, "vy");
       f->addArg(type_int, "n");
       f->addArg(type_uint32, "color");
       f->addArg(array_parray_pint_q_q, "polyInts");
       f->addArg(array_pint_q, "polyAllocated");

    f = mod->addFunc(type_int, "filledPolygonRGBAMT",
                     (void *)filledPolygonRGBAMT
                     );
       f->addArg(type_SDL_Surface, "dst");
       f->addArg(array_pint16_q, "vx");
       f->addArg(array_pint16_q, "vy");
       f->addArg(type_int, "n");
       f->addArg(type_byte, "r");
       f->addArg(type_byte, "g");
       f->addArg(type_byte, "b");
       f->addArg(type_byte, "a");
       f->addArg(array_parray_pint_q_q, "polyInts");
       f->addArg(array_pint_q, "polyAllocated");

    f = mod->addFunc(type_int, "texturedPolygonMT",
                     (void *)texturedPolygonMT
                     );
       f->addArg(type_SDL_Surface, "dst");
       f->addArg(array_pint16_q, "vx");
       f->addArg(array_pint16_q, "vy");
       f->addArg(type_int, "n");
       f->addArg(type_SDL_Surface, "texture");
       f->addArg(type_int, "texture_dx");
       f->addArg(type_int, "texture_dy");
       f->addArg(array_parray_pint_q_q, "polyInts");
       f->addArg(array_pint_q, "polyAllocated");

    f = mod->addFunc(type_int, "bezierColor",
                     (void *)bezierColor
                     );
       f->addArg(type_SDL_Surface, "dst");
       f->addArg(array_pint16_q, "vx");
       f->addArg(array_pint16_q, "vy");
       f->addArg(type_int, "n");
       f->addArg(type_int, "s");
       f->addArg(type_uint32, "color");

    f = mod->addFunc(type_int, "bezierRGBA",
                     (void *)bezierRGBA
                     );
       f->addArg(type_SDL_Surface, "dst");
       f->addArg(array_pint16_q, "vx");
       f->addArg(array_pint16_q, "vy");
       f->addArg(type_int, "n");
       f->addArg(type_int, "s");
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
       f->addArg(type_uint32, "color");

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
