#include <fluidsynth.h>
typedef int Undef;


#include "ext/Module.h"
#include "ext/Type.h"
#include "ext/Func.h"

extern "C"
void crack_ext__fluidsynth_rinit() {
    return;
}

extern "C"
void crack_ext__fluidsynth_cinit(crack::ext::Module *mod) {
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

    crack::ext::Type *type_fluid_settings_t = mod->addType("fluid_settings_t", sizeof(Undef));
    type_fluid_settings_t->finish();


    crack::ext::Type *type_fluid_synth_t = mod->addType("fluid_synth_t", sizeof(Undef));
    type_fluid_synth_t->finish();


    crack::ext::Type *type_fluid_audio_driver_t = mod->addType("fluid_audio_driver_t", sizeof(Undef));
    type_fluid_audio_driver_t->finish();

    f = mod->addFunc(type_fluid_settings_t, "new_fluid_settings",
                     (void *)new_fluid_settings
                     );

    f = mod->addFunc(type_fluid_synth_t, "new_fluid_synth",
                     (void *)new_fluid_synth
                     );
       f->addArg(type_fluid_settings_t, "settings");

    f = mod->addFunc(type_void, "delete_fluid_synth",
                     (void *)delete_fluid_synth
                     );
       f->addArg(type_fluid_synth_t, "synth");

    f = mod->addFunc(type_void, "delete_fluid_settings",
                     (void *)delete_fluid_settings
                     );
       f->addArg(type_fluid_settings_t, "settings");

    f = mod->addFunc(type_int, "fluid_synth_noteon",
                     (void *)fluid_synth_noteon
                     );
       f->addArg(type_fluid_synth_t, "synth");
       f->addArg(type_int, "channel");
       f->addArg(type_int, "key");
       f->addArg(type_int, "velocity");

    f = mod->addFunc(type_int, "fluid_synth_noteoff",
                     (void *)fluid_synth_noteoff
                     );
       f->addArg(type_fluid_synth_t, "synth");
       f->addArg(type_int, "channel");
       f->addArg(type_int, "key");

    f = mod->addFunc(type_int, "fluid_synth_pitch_bend",
                     (void *)fluid_synth_pitch_bend
                     );
       f->addArg(type_fluid_synth_t, "synth");
       f->addArg(type_int, "channel");
       f->addArg(type_int, "val");

    f = mod->addFunc(type_int, "fluid_synth_program_change",
                     (void *)fluid_synth_program_change
                     );
       f->addArg(type_fluid_synth_t, "synth");
       f->addArg(type_int, "channel");
       f->addArg(type_int, "program");

    f = mod->addFunc(type_int, "fluid_synth_all_notes_off",
                     (void *)fluid_synth_all_notes_off
                     );
       f->addArg(type_fluid_synth_t, "synth");
       f->addArg(type_int, "channel");

    f = mod->addFunc(type_int, "fluid_synth_all_sounds_off",
                     (void *)fluid_synth_all_sounds_off
                     );
       f->addArg(type_fluid_synth_t, "synth");
       f->addArg(type_int, "channel");

    f = mod->addFunc(type_fluid_audio_driver_t, "new_fluid_audio_driver",
                     (void *)new_fluid_audio_driver
                     );
       f->addArg(type_fluid_settings_t, "settings");
       f->addArg(type_fluid_synth_t, "synth");

    f = mod->addFunc(type_int, "fluid_synth_sfload",
                     (void *)fluid_synth_sfload
                     );
       f->addArg(type_fluid_synth_t, "synth");
       f->addArg(type_byteptr, "filename");
       f->addArg(type_int, "reset_presets");

}
