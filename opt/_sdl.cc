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
SDL_ResizeEvent *SDL_Event_GetResize(SDL_Event *evt) { return &evt->resize; }
int16_t SDL_ResizeEvent_GetW(SDL_ResizeEvent *evt) { return evt->w; }
int16_t SDL_ResizeEvent_GetH(SDL_ResizeEvent *evt) { return evt->h; }

void crk_SDL_Rect_init(SDL_Rect *rect, int16_t x, int16_t y, uint16_t w, uint16_t h) {
    rect->x = x;
    rect->y = y;
    rect->w = w;
    rect->h = h;
}

SDL_JoyAxisEvent *SDL_Event_GetJoyAxisEvent(SDL_Event *evt) {
    return &evt->jaxis;
}

uint8_t SDL_JoyAxisEvent_GetType(SDL_JoyAxisEvent *evt) {
    return evt->type;
}

uint8_t SDL_JoyAxisEvent_GetWhich(SDL_JoyAxisEvent *evt) {
    return evt->which;
}

uint8_t SDL_JoyAxisEvent_GetAxis(SDL_JoyAxisEvent *evt) {
    return evt->axis;
}

int16_t SDL_JoyAxisEvent_GetValue(SDL_JoyAxisEvent *evt) {
    return evt->value;
}

SDL_JoyButtonEvent *SDL_Event_GetJoyButtonEvent(SDL_Event *evt) {
    return &evt->jbutton;
}

uint8_t SDL_JoyButtonEvent_GetType(SDL_JoyButtonEvent *evt) {
    return evt->type;
}

uint8_t SDL_JoyButtonEvent_GetWhich(SDL_JoyButtonEvent *evt) {
    return evt->which;
}

uint8_t SDL_JoyButtonEvent_GetState(SDL_JoyButtonEvent *evt) {
    return evt->state;
}

SDL_MouseButtonEvent *SDL_Event_GetMouseButtonEvent(SDL_Event *evt) {
    return &evt->button;
}

struct Undef {};


#include "ext/Module.h"
#include "ext/Type.h"
#include "ext/Func.h"

extern "C"
void crack_ext__sdl_rinit() {
    return;
}

extern "C"
void crack_ext__sdl_cinit(crack::ext::Module *mod) {
    crack::ext::Func *f;
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

    crack::ext::Type *type_SDL_Surface = mod->addType("SDL_Surface", sizeof(SDL_Surface));
    type_SDL_Surface->finish();


    crack::ext::Type *type_SDL_Event = mod->addType("SDL_Event", sizeof(SDL_Event));
    type_SDL_Event->finish();


    crack::ext::Type *type_SDL_Rect = mod->addType("SDL_Rect", sizeof(SDL_Rect));
        type_SDL_Rect->addInstVar(type_int16, "x",
                                CRACK_OFFSET(SDL_Rect, x));
        type_SDL_Rect->addInstVar(type_int16, "y",
                                CRACK_OFFSET(SDL_Rect, y));
        type_SDL_Rect->addInstVar(type_uint16, "w",
                                CRACK_OFFSET(SDL_Rect, w));
        type_SDL_Rect->addInstVar(type_uint16, "h",
                                CRACK_OFFSET(SDL_Rect, h));
        f = type_SDL_Rect->addConstructor("init",
                            (void *)crk_SDL_Rect_init
                        );
    f->addArg(type_int16, 
              "x"
              );
    f->addArg(type_int16, 
              "y"
              );
    f->addArg(type_uint16, 
              "w"
              );
    f->addArg(type_uint16, 
              "h"
              );

    type_SDL_Rect->finish();


    crack::ext::Type *type_SDL_KeyboardEvent = mod->addType("SDL_KeyboardEvent", sizeof(SDL_KeyboardEvent));
    type_SDL_KeyboardEvent->finish();


    crack::ext::Type *type_SDL_keysym = mod->addType("SDL_keysym", sizeof(SDL_keysym));
    type_SDL_keysym->finish();


    crack::ext::Type *type_SDL_MouseMotionEvent = mod->addType("SDL_MouseMotionEvent", sizeof(SDL_MouseMotionEvent));
    type_SDL_MouseMotionEvent->finish();


    crack::ext::Type *type_SDL_ResizeEvent = mod->addType("SDL_ResizeEvent", sizeof(SDL_ResizeEvent));
    type_SDL_ResizeEvent->finish();


    crack::ext::Type *type_SDL_JoyAxisEvent = mod->addType("SDL_JoyAxisEvent", sizeof(SDL_JoyAxisEvent));
        type_SDL_JoyAxisEvent->addInstVar(type_byte, "type",
                                CRACK_OFFSET(SDL_JoyAxisEvent, type));
        type_SDL_JoyAxisEvent->addInstVar(type_byte, "which",
                                CRACK_OFFSET(SDL_JoyAxisEvent, which));
        type_SDL_JoyAxisEvent->addInstVar(type_byte, "axis",
                                CRACK_OFFSET(SDL_JoyAxisEvent, axis));
        type_SDL_JoyAxisEvent->addInstVar(type_int16, "value",
                                CRACK_OFFSET(SDL_JoyAxisEvent, value));
    type_SDL_JoyAxisEvent->finish();


    crack::ext::Type *type_SDL_JoyButtonEvent = mod->addType("SDL_JoyButtonEvent", sizeof(SDL_JoyButtonEvent));
        type_SDL_JoyButtonEvent->addInstVar(type_byte, "type",
                                CRACK_OFFSET(SDL_JoyButtonEvent, type));
        type_SDL_JoyButtonEvent->addInstVar(type_byte, "which",
                                CRACK_OFFSET(SDL_JoyButtonEvent, which));
        type_SDL_JoyButtonEvent->addInstVar(type_byte, "button",
                                CRACK_OFFSET(SDL_JoyButtonEvent, button));
        type_SDL_JoyButtonEvent->addInstVar(type_byte, "state",
                                CRACK_OFFSET(SDL_JoyButtonEvent, state));
    type_SDL_JoyButtonEvent->finish();


    crack::ext::Type *type_SDL_Joystick = mod->addType("SDL_Joystick", sizeof(Undef));
    type_SDL_Joystick->finish();


    crack::ext::Type *type_SDL_MouseButtonEvent = mod->addType("SDL_MouseButtonEvent", sizeof(SDL_MouseButtonEvent));
        type_SDL_MouseButtonEvent->addInstVar(type_byte, "type",
                                CRACK_OFFSET(SDL_MouseButtonEvent, type));
        type_SDL_MouseButtonEvent->addInstVar(type_byte, "which",
                                CRACK_OFFSET(SDL_MouseButtonEvent, which));
        type_SDL_MouseButtonEvent->addInstVar(type_byte, "button",
                                CRACK_OFFSET(SDL_MouseButtonEvent, button));
        type_SDL_MouseButtonEvent->addInstVar(type_byte, "state",
                                CRACK_OFFSET(SDL_MouseButtonEvent, state));
        type_SDL_MouseButtonEvent->addInstVar(type_uint16, "x",
                                CRACK_OFFSET(SDL_MouseButtonEvent, x));
        type_SDL_MouseButtonEvent->addInstVar(type_uint16, "y",
                                CRACK_OFFSET(SDL_MouseButtonEvent, y));
    type_SDL_MouseButtonEvent->finish();


    crack::ext::Type *array = mod->getType("array");

    crack::ext::Type *array_pint_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_int;
        array_pint_q = array->getSpecialization(params);
    }
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

    f = mod->addFunc(type_int, "SDL_Flip",
                     (void *)SDL_Flip
                     );
       f->addArg(type_SDL_Surface, "screen");

    f = mod->addFunc(type_int, "SDL_FillRect",
                     (void *)SDL_FillRect
                     );
       f->addArg(type_SDL_Surface, "surface");
       f->addArg(type_SDL_Rect, "rect");
       f->addArg(type_uint32, "color");

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

    f = mod->addFunc(type_SDL_ResizeEvent, "SDL_Event_GetResize",
                     (void *)SDL_Event_GetResize
                     );
       f->addArg(type_SDL_Event, "evt");

    f = mod->addFunc(type_int16, "SDL_ResizeEvent_GetW",
                     (void *)SDL_ResizeEvent_GetW
                     );
       f->addArg(type_SDL_ResizeEvent, "evt");

    f = mod->addFunc(type_int16, "SDL_ResizeEvent_GetH",
                     (void *)SDL_ResizeEvent_GetH
                     );
       f->addArg(type_SDL_ResizeEvent, "evt");

    f = mod->addFunc(type_SDL_JoyAxisEvent, "SDL_Event_GetJoyAxisEvent",
                     (void *)SDL_Event_GetJoyAxisEvent
                     );
       f->addArg(type_SDL_Event, "evt");

    f = mod->addFunc(type_byte, "SDL_JoyAxisEvent_GetType",
                     (void *)SDL_JoyAxisEvent_GetType
                     );
       f->addArg(type_SDL_JoyAxisEvent, "evt");

    f = mod->addFunc(type_byte, "SDL_JoyAxisEvent_GetWhich",
                     (void *)SDL_JoyAxisEvent_GetWhich
                     );
       f->addArg(type_SDL_JoyAxisEvent, "evt");

    f = mod->addFunc(type_byte, "SDL_JoyAxisEvent_GetAxis",
                     (void *)SDL_JoyAxisEvent_GetAxis
                     );
       f->addArg(type_SDL_JoyAxisEvent, "evt");

    f = mod->addFunc(type_int16, "SDL_JoyAxisEvent_GetValue",
                     (void *)SDL_JoyAxisEvent_GetValue
                     );
       f->addArg(type_SDL_JoyAxisEvent, "evt");

    f = mod->addFunc(type_SDL_JoyButtonEvent, "SDL_Event_GetJoyButtonEvent",
                     (void *)SDL_Event_GetJoyButtonEvent
                     );
       f->addArg(type_SDL_Event, "evt");

    f = mod->addFunc(type_byte, "SDL_JoyButtonEvent_GetType",
                     (void *)SDL_JoyButtonEvent_GetType
                     );
       f->addArg(type_SDL_JoyButtonEvent, "evt");

    f = mod->addFunc(type_byte, "SDL_JoyButtonEvent_GetWhich",
                     (void *)SDL_JoyButtonEvent_GetWhich
                     );
       f->addArg(type_SDL_JoyButtonEvent, "evt");

    f = mod->addFunc(type_byte, "SDL_JoyButtonEvent_GetState",
                     (void *)SDL_JoyButtonEvent_GetState
                     );
       f->addArg(type_SDL_JoyButtonEvent, "evt");

    f = mod->addFunc(type_SDL_Joystick, "SDL_JoystickOpen",
                     (void *)SDL_JoystickOpen
                     );
       f->addArg(type_int, "deviceIndex");

    f = mod->addFunc(type_int, "SDL_JoystickOpened",
                     (void *)SDL_JoystickOpened
                     );
       f->addArg(type_int, "deviceIndex");

    f = mod->addFunc(type_void, "SDL_JoystickClose",
                     (void *)SDL_JoystickClose
                     );
       f->addArg(type_SDL_Joystick, "joystick");

    f = mod->addFunc(type_SDL_MouseButtonEvent, "SDL_Event_GetMouseButtonEvent",
                     (void *)SDL_Event_GetMouseButtonEvent
                     );
       f->addArg(type_SDL_Event, "evt");

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

    mod->addConstant(type_uint32, "SDL_VIDEORESIZE",
                     static_cast<int>(SDL_VIDEORESIZE)
                     );

    mod->addConstant(type_uint32, "SDL_NUMEVENTS",
                     static_cast<int>(SDL_NUMEVENTS)
                     );

    mod->addConstant(type_uint32, "SDL_PRESSED",
                     static_cast<int>(SDL_PRESSED)
                     );

    mod->addConstant(type_uint32, "SDL_RELEASED",
                     static_cast<int>(SDL_RELEASED)
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
