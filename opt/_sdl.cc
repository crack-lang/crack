#include <SDL.h>
SDL_Event *SDL_EventNew() { return new SDL_Event; }
int SDL_Event_GetType(SDL_Event *evt) { return evt->type; }
SDL_KeyboardEvent *SDL_Event_GetKey(SDL_Event *evt) { return &evt->key; }
SDL_keysym *SDL_KeyboardEvent_GetKeysym(SDL_KeyboardEvent *evt) { return &evt->keysym; }
unsigned SDL_keysym_GetScancode(SDL_keysym *s) { return s->scancode; }
unsigned SDL_keysym_GetSym(SDL_keysym *s) { return s->sym; }
unsigned SDL_keysym_GetMod(SDL_keysym *s) { return s->mod; }
unsigned SDL_keysym_GetUnicode(SDL_keysym *s) { return s->unicode; }
uint8_t SDL_KeyboardEvent_GetType(SDL_KeyboardEvent *evt) { return evt->type; }
uint8_t SDL_KeyboardEvent_GetWhich(SDL_KeyboardEvent *evt) { return evt->which; }
uint8_t SDL_KeyboardEvent_GetState(SDL_KeyboardEvent *evt) { return evt->state; }
SDL_MouseMotionEvent *SDL_Event_GetMotion(SDL_Event *evt) { return &evt->motion; }
uint SDL_MouseMotionEvent_GetX(SDL_MouseMotionEvent *evt) { return evt->x; }
uint SDL_MouseMotionEvent_GetY(SDL_MouseMotionEvent *evt) { return evt->y; }


#include "ext/Module.h"
#include "ext/Type.h"
#include "ext/Func.h"

extern "C"
void crack_ext__sdl_init(crack::ext::Module *mod) {
    crack::ext::Func *f;
    crack::ext::Type *type_Class = mod->getClassType();

    crack::ext::Type *type_SDL_Event = mod->addType("SDL_Event");
    type_SDL_Event->finish();

    crack::ext::Type *type_SDL_KeyboardEvent = mod->addType("SDL_KeyboardEvent");
    type_SDL_KeyboardEvent->finish();

    crack::ext::Type *type_SDL_MouseMotionEvent = mod->addType("SDL_MouseMotionEvent");
    type_SDL_MouseMotionEvent->finish();

    crack::ext::Type *type_SDL_Surface = mod->addType("SDL_Surface");
    type_SDL_Surface->finish();

    crack::ext::Type *type_SDL_keysym = mod->addType("SDL_keysym");
    type_SDL_keysym->finish();

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

    f = mod->addFunc(type_SDL_KeyboardEvent, "SDL_Event_GetKey",
                     (void *)SDL_Event_GetKey
                     );
    f->addArg(type_SDL_Event, "evt");

    f = mod->addFunc(type_SDL_keysym, "SDL_KeyboardEvent_GetKeysym",
                     (void *)SDL_KeyboardEvent_GetKeysym
                     );
    f->addArg(type_SDL_KeyboardEvent, "evt");

    f = mod->addFunc(type_uint, "SDL_keysym_GetScancode",
                     (void *)SDL_keysym_GetScancode
                     );
    f->addArg(type_SDL_keysym, "keysym");

    f = mod->addFunc(type_uint, "SDL_keysym_GetSym",
                     (void *)SDL_keysym_GetSym
                     );
    f->addArg(type_SDL_keysym, "keysym");

    f = mod->addFunc(type_uint, "SDL_keysym_GetMod",
                     (void *)SDL_keysym_GetMod
                     );
    f->addArg(type_SDL_keysym, "keysym");

    f = mod->addFunc(type_uint, "SDL_keysym_GetUnicode",
                     (void *)SDL_keysym_GetUnicode
                     );
    f->addArg(type_SDL_keysym, "keysym");

    f = mod->addFunc(type_byte, "SDL_KeyboardEvent_GetType",
                     (void *)SDL_KeyboardEvent_GetType
                     );
    f->addArg(type_SDL_KeyboardEvent, "evt");

    f = mod->addFunc(type_byte, "SDL_KeyboardEvent_GetWhich",
                     (void *)SDL_KeyboardEvent_GetWhich
                     );
    f->addArg(type_SDL_KeyboardEvent, "evt");

    f = mod->addFunc(type_byte, "SDL_KeyboardEvent_GetState",
                     (void *)SDL_KeyboardEvent_GetState
                     );
    f->addArg(type_SDL_KeyboardEvent, "evt");

    f = mod->addFunc(type_SDL_MouseMotionEvent, "SDL_Event_GetMotion",
                     (void *)SDL_Event_GetMotion
                     );
    f->addArg(type_SDL_Event, "evt");

    f = mod->addFunc(type_uint, "SDL_MouseMotionEvent_GetX",
                     (void *)SDL_MouseMotionEvent_GetX
                     );
    f->addArg(type_SDL_MouseMotionEvent, "evt");

    f = mod->addFunc(type_uint, "SDL_MouseMotionEvent_GetY",
                     (void *)SDL_MouseMotionEvent_GetY
                     );
    f->addArg(type_SDL_MouseMotionEvent, "evt");

    f = mod->addFunc(type_void, "SDL_WarpMouse",
                     (void *)SDL_WarpMouse
                     );
    f->addArg(type_uint, "x");
    f->addArg(type_uint, "y");

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

    mod->addConstant(type_uint32, "SDL_NOEVENT",
                     static_cast<int>(SDL_NOEVENT)
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

    mod->addConstant(type_uint32, "SDL_MOUSEBUTTONDOWN",
                     static_cast<int>(SDL_MOUSEBUTTONDOWN)
                     );

    mod->addConstant(type_uint32, "SDL_MOUSEBUTTONUP",
                     static_cast<int>(SDL_MOUSEBUTTONUP)
                     );

    mod->addConstant(type_uint32, "SDL_JOYAXISMOTION",
                     static_cast<int>(SDL_JOYAXISMOTION)
                     );

    mod->addConstant(type_uint32, "SDL_JOYBALLMOTION",
                     static_cast<int>(SDL_JOYBALLMOTION)
                     );

    mod->addConstant(type_uint32, "SDL_JOYHATMOTION",
                     static_cast<int>(SDL_JOYHATMOTION)
                     );

    mod->addConstant(type_uint32, "SDL_JOYBUTTONDOWN",
                     static_cast<int>(SDL_JOYBUTTONDOWN)
                     );

    mod->addConstant(type_uint32, "SDL_JOYBUTTONUP",
                     static_cast<int>(SDL_JOYBUTTONUP)
                     );

    mod->addConstant(type_uint32, "SDL_QUIT",
                     static_cast<int>(SDL_QUIT)
                     );

    mod->addConstant(type_uint32, "SDL_SYSWMEVENT",
                     static_cast<int>(SDL_SYSWMEVENT)
                     );

    mod->addConstant(type_uint32, "SDL_EVENT_RESERVED2",
                     static_cast<int>(SDL_EVENT_RESERVED2)
                     );

    mod->addConstant(type_uint32, "SDL_EVENT_RESERVED3",
                     static_cast<int>(SDL_EVENT_RESERVED3)
                     );

    mod->addConstant(type_uint32, "SDL_USEREVENT",
                     static_cast<int>(SDL_USEREVENT)
                     );

    mod->addConstant(type_uint32, "SDL_NUMEVENTS",
                     static_cast<int>(SDL_NUMEVENTS)
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
