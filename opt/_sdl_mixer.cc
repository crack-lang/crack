
#include <SDL_mixer.h>

struct Undef {};


#include "ext/Module.h"
#include "ext/Type.h"
#include "ext/Func.h"

extern "C"
void crack_ext__sdl_mixer_rinit() {
    return;
}

extern "C"
void crack_ext__sdl_mixer_cinit(crack::ext::Module *mod) {
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

    crack::ext::Type *type_SDL_RWops = mod->addType("SDL_RWops", sizeof(SDL_RWops));
    type_SDL_RWops->finish();


    crack::ext::Type *type_Mix_Chunk = mod->addType("Mix_Chunk", sizeof(Undef));
    type_Mix_Chunk->finish();


    crack::ext::Type *function = mod->getType("function");

    crack::ext::Type *function_pvoid_c_sint_q;
    {
        std::vector<crack::ext::Type *> params(2);
        params[0] = type_void;
        params[1] = type_int;
        function_pvoid_c_sint_q = function->getSpecialization(params);
    }
    f = mod->addFunc(type_SDL_RWops, "SDL_RWFromFile",
                     (void *)SDL_RWFromFile
                     );
       f->addArg(type_byteptr, "filename");
       f->addArg(type_byteptr, "mode");

    f = mod->addFunc(type_Mix_Chunk, "Mix_LoadWAV_RW",
                     (void *)Mix_LoadWAV_RW
                     );
       f->addArg(type_SDL_RWops, "ops");
       f->addArg(type_int, "freeSrc");

    f = mod->addFunc(type_void, "Mix_FreeChunk",
                     (void *)Mix_FreeChunk
                     );
       f->addArg(type_Mix_Chunk, "chunk");

    f = mod->addFunc(type_int, "Mix_PlayChannelTimed",
                     (void *)Mix_PlayChannelTimed
                     );
       f->addArg(type_int, "channel");
       f->addArg(type_Mix_Chunk, "chunk");
       f->addArg(type_int, "loops");
       f->addArg(type_int, "ticks");

    f = mod->addFunc(type_int, "Mix_SetPosition",
                     (void *)Mix_SetPosition
                     );
       f->addArg(type_int, "channel");
       f->addArg(type_int16, "angle");
       f->addArg(type_byte, "distance");

    f = mod->addFunc(type_int, "Mix_HaltChannel",
                     (void *)Mix_HaltChannel
                     );
       f->addArg(type_int, "channel");

    f = mod->addFunc(type_int, "Mix_OpenAudio",
                     (void *)Mix_OpenAudio
                     );
       f->addArg(type_int, "rate");
       f->addArg(type_uint16, "format");
       f->addArg(type_int, "channels");
       f->addArg(type_int, "chunksize");

    f = mod->addFunc(type_void, "Mix_ChannelFinished",
                     (void *)Mix_ChannelFinished
                     );
       f->addArg(function_pvoid_c_sint_q, "callback");


    mod->addConstant(type_uint16, "AUDIO_S16",
                     static_cast<int16_t>(AUDIO_S16)
                     );
}
