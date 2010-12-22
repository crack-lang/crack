#include <SDL.h>
SDL_Event *SDL_EventNew() { return new SDL_Event; }
int SDL_Event_GetType(SDL_Event *evt) { return evt->type; }


#include "ext/Module.h"
#include "ext/Type.h"
#include "ext/Func.h"

extern "C"
void crack_ext__sdl_init(crack::ext::Module *mod) {
    crack::ext::Func *f;
    crack::ext::Type *type_Class = mod->getClassType();

    crack::ext::Type *type_SDL_Event = mod->addType("SDL_Event");
    type_SDL_Event->finish();

    crack::ext::Type *type_SDL_Surface = mod->addType("SDL_Surface");
    type_SDL_Surface->finish();

    crack::ext::Type *array = mod->getType("array");
    crack::ext::Type *type_int = mod->getIntType();

    crack::ext::Type *array_pint_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_int;
        array_pint_q = array->getSpecialization(params);
    }
    crack::ext::Type *type_bool = mod->getBoolType();
    crack::ext::Type *type_byte = mod->getByteType();
    crack::ext::Type *type_byteptr = mod->getByteptrType();
    crack::ext::Type *type_float = mod->getFloatType();
    crack::ext::Type *type_float32 = mod->getFloat32Type();
    crack::ext::Type *type_float64 = mod->getFloat64Type();
    crack::ext::Type *type_int32 = mod->getInt32Type();
    crack::ext::Type *type_int64 = mod->getInt64Type();
    crack::ext::Type *type_uint = mod->getUintType();
    crack::ext::Type *type_uint32 = mod->getUint32Type();
    crack::ext::Type *type_uint64 = mod->getUint64Type();
    crack::ext::Type *type_void = mod->getVoidType();
    crack::ext::Type *type_voidptr = mod->getVoidptrType();

    f = mod->addFunc(type_int, "SDL_Init",
                     (void *)SDL_Init
                     );
    f->addArg(type_uint32, "flags");

    f = mod->addFunc(type_int, "SDL_Quit",
                     (void *)SDL_Quit
                     );

    f = mod->addFunc(type_SDL_Surface, "SDL_SetVideoMode",
                     (void *)SDL_SetVideoMode
                     );
    f->addArg(type_int, "width");
    f->addArg(type_int, "height");
    f->addArg(type_int, "bpp");
    f->addArg(type_uint32, "flags");

    f = mod->addFunc(type_SDL_Event, "SDL_EventNew",
                     (void *)SDL_EventNew
                     );

    f = mod->addFunc(type_int, "SDL_Event_GetType",
                     (void *)SDL_Event_GetType
                     );
    f->addArg(type_SDL_Event, "event");

    f = mod->addFunc(type_int, "SDL_PollEvent",
                     (void *)SDL_PollEvent
                     );
    f->addArg(type_SDL_Event, "event");

    f = mod->addFunc(type_int, "SDL_GL_SetAttribute",
                     (void *)SDL_GL_SetAttribute
                     );
    f->addArg(type_uint32, "attr");
    f->addArg(type_int, "value");

    f = mod->addFunc(type_int, "SDL_GL_GetAttribute",
                     (void *)SDL_GL_GetAttribute
                     );
    f->addArg(type_uint32, "attr");
    f->addArg(array_pint_q, "value");

    f = mod->addFunc(type_void, "SDL_GL_SwapBuffers",
                     (void *)SDL_GL_SwapBuffers
                     );

    mod->addConstant(type_uint32, "SDL_INIT_TIMER",
                     static_cast<int>(SDL_INIT_TIMER)
                     );

    mod->addConstant(type_uint32, "SDL_INIT_AUDIO",
                     static_cast<int>(SDL_INIT_AUDIO)
                     );

    mod->addConstant(type_uint32, "SDL_INIT_VIDEO",
                     static_cast<int>(SDL_INIT_VIDEO)
                     );

    mod->addConstant(type_uint32, "SDL_INIT_CDROM",
                     static_cast<int>(SDL_INIT_CDROM)
                     );

    mod->addConstant(type_uint32, "SDL_INIT_JOYSTICK",
                     static_cast<int>(SDL_INIT_JOYSTICK)
                     );

    mod->addConstant(type_uint32, "SDL_INIT_EVERYTHING",
                     static_cast<int>(SDL_INIT_EVERYTHING)
                     );

    mod->addConstant(type_uint32, "SDL_INIT_NOPARACHUTE",
                     static_cast<int>(SDL_INIT_NOPARACHUTE)
                     );

    mod->addConstant(type_uint32, "SDL_INIT_EVENTTHREAD",
                     static_cast<int>(SDL_INIT_EVENTTHREAD)
                     );

    mod->addConstant(type_uint32, "SDL_SWSURFACE",
                     static_cast<int>(SDL_SWSURFACE)
                     );

    mod->addConstant(type_uint32, "SDL_HWSURFACE",
                     static_cast<int>(SDL_HWSURFACE)
                     );

    mod->addConstant(type_uint32, "SDL_ASYNCBLIT",
                     static_cast<int>(SDL_ASYNCBLIT)
                     );

    mod->addConstant(type_uint32, "SDL_ANYFORMAT",
                     static_cast<int>(SDL_ANYFORMAT)
                     );

    mod->addConstant(type_uint32, "SDL_HWPALETTE",
                     static_cast<int>(SDL_HWPALETTE)
                     );

    mod->addConstant(type_uint32, "SDL_DOUBLEBUF",
                     static_cast<int>(SDL_DOUBLEBUF)
                     );

    mod->addConstant(type_uint32, "SDL_FULLSCREEN",
                     static_cast<int>(SDL_FULLSCREEN)
                     );

    mod->addConstant(type_uint32, "SDL_OPENGL",
                     static_cast<int>(SDL_OPENGL)
                     );

    mod->addConstant(type_uint32, "SDL_OPENGLBLIT",
                     static_cast<int>(SDL_OPENGLBLIT)
                     );

    mod->addConstant(type_uint32, "SDL_RESIZABLE",
                     static_cast<int>(SDL_RESIZABLE)
                     );

    mod->addConstant(type_uint32, "SDL_NOFRAME",
                     static_cast<int>(SDL_NOFRAME)
                     );

    mod->addConstant(type_uint32, "SDL_KEYDOWN",
                     static_cast<int>(SDL_KEYDOWN)
                     );

    mod->addConstant(type_uint32, "SDL_KEYUP",
                     static_cast<int>(SDL_KEYUP)
                     );

    mod->addConstant(type_uint32, "SDL_MOUSEMOTION",
                     static_cast<int>(SDL_MOUSEMOTION)
                     );

    mod->addConstant(type_uint32, "SDL_GL_RED_SIZE",
                     static_cast<int>(SDL_GL_RED_SIZE)
                     );

    mod->addConstant(type_uint32, "SDL_GL_GREEN_SIZE",
                     static_cast<int>(SDL_GL_GREEN_SIZE)
                     );

    mod->addConstant(type_uint32, "SDL_GL_BLUE_SIZE",
                     static_cast<int>(SDL_GL_BLUE_SIZE)
                     );

    mod->addConstant(type_uint32, "SDL_GL_DEPTH_SIZE",
                     static_cast<int>(SDL_GL_DEPTH_SIZE)
                     );

    mod->addConstant(type_uint32, "SDL_GL_DOUBLEBUFFER",
                     static_cast<int>(SDL_GL_DOUBLEBUFFER)
                     );
}
