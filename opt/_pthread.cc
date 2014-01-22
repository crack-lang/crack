#include <pthread.h>

    int crk_pthread_join(pthread_t *thread, void **retval) {
        return pthread_join(*thread, retval);
    }


#include "ext/Module.h"
#include "ext/Type.h"
#include "ext/Func.h"

extern "C"
void crack_ext__pthread_rinit() {
    return;
}

extern "C"
void crack_ext__pthread_cinit(crack::ext::Module *mod) {
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

    crack::ext::Type *type_pthread_t = mod->addType("pthread_t", sizeof(pthread_t));
    type_pthread_t->finish();


    crack::ext::Type *type_pthread_attr_t = mod->addType("pthread_attr_t", sizeof(pthread_attr_t));
    type_pthread_attr_t->finish();


    crack::ext::Type *function = mod->getType("function");

    crack::ext::Type *function_pvoid_c_svoidptr_q;
    {
        std::vector<crack::ext::Type *> params(2);
        params[0] = type_void;
        params[1] = type_voidptr;
        function_pvoid_c_svoidptr_q = function->getSpecialization(params);
    }

    crack::ext::Type *array = mod->getType("array");

    crack::ext::Type *array_pvoidptr_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_voidptr;
        array_pvoidptr_q = array->getSpecialization(params);
    }

    crack::ext::Type *type_pthread_mutexattr_t = mod->addType("pthread_mutexattr_t", sizeof(pthread_mutexattr_t));
    type_pthread_mutexattr_t->finish();


    crack::ext::Type *type_pthread_mutex_t = mod->addType("pthread_mutex_t", sizeof(pthread_mutex_t));
    type_pthread_mutex_t->finish();


    crack::ext::Type *type_pthread_cond_t = mod->addType("pthread_cond_t", sizeof(pthread_cond_t));
    type_pthread_cond_t->finish();


    crack::ext::Type *type_pthread_condattr_t = mod->addType("pthread_condattr_t", sizeof(pthread_condattr_t));
    type_pthread_condattr_t->finish();

    f = mod->addFunc(type_int, "pthread_create",
                     (void *)pthread_create
                     );
       f->addArg(type_pthread_t, "thread");
       f->addArg(type_pthread_attr_t, "attr");
       f->addArg(function_pvoid_c_svoidptr_q, "func");
       f->addArg(type_voidptr, "arg");

    f = mod->addFunc(type_int, "pthread_join",
                     (void *)crk_pthread_join
                     );
       f->addArg(type_pthread_t, "thread");
       f->addArg(array_pvoidptr_q, "retval");

    f = mod->addFunc(type_int, "pthread_mutex_init",
                     (void *)pthread_mutex_init
                     );
       f->addArg(type_pthread_mutex_t, "mutex");
       f->addArg(type_pthread_mutexattr_t, "attrs");

    f = mod->addFunc(type_int, "pthread_mutex_destroy",
                     (void *)pthread_mutex_destroy
                     );
       f->addArg(type_pthread_mutex_t, "mutext");

    f = mod->addFunc(type_int, "pthread_mutex_lock",
                     (void *)pthread_mutex_lock
                     );
       f->addArg(type_pthread_mutex_t, "mutex");

    f = mod->addFunc(type_int, "pthread_mutex_trylock",
                     (void *)pthread_mutex_trylock
                     );
       f->addArg(type_pthread_mutex_t, "mutex");

    f = mod->addFunc(type_int, "pthread_mutex_unlock",
                     (void *)pthread_mutex_unlock
                     );
       f->addArg(type_pthread_mutex_t, "mutex");

    f = mod->addFunc(type_int, "pthread_cond_init",
                     (void *)pthread_cond_init
                     );
       f->addArg(type_pthread_cond_t, "cond");
       f->addArg(type_pthread_condattr_t, "attrs");

    f = mod->addFunc(type_int, "pthread_cond_destroy",
                     (void *)pthread_cond_destroy
                     );
       f->addArg(type_pthread_cond_t, "cond");

    f = mod->addFunc(type_int, "pthread_cond_signal",
                     (void *)pthread_cond_signal
                     );
       f->addArg(type_pthread_cond_t, "cond");

    f = mod->addFunc(type_int, "pthread_cond_broadcast",
                     (void *)pthread_cond_broadcast
                     );
       f->addArg(type_pthread_cond_t, "cond");

    f = mod->addFunc(type_int, "pthread_cond_wait",
                     (void *)pthread_cond_wait
                     );
       f->addArg(type_pthread_cond_t, "cond");
       f->addArg(type_pthread_mutex_t, "mutex");

}
