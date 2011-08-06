#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>

typedef struct tm InternalDate;

InternalDate *crk_create_date(){
   return (InternalDate *)calloc(1, sizeof(InternalDate));
}

InternalDate *crk_localtime(InternalDate *d, int64_t t){
   const time_t lt = (const time_t)t;
   return localtime_r(&lt, d);
}


InternalDate *crk_localtime_now(InternalDate *now){
   struct timeval tv;
   gettimeofday(&tv, NULL);
   return localtime_r(&(tv.tv_sec), now);
}

InternalDate *crk_gmtime(InternalDate *d, int64_t t){
   const time_t gt = (const time_t)t;
   return gmtime_r(&gt, d);
}

InternalDate *crk_gmtime_now(InternalDate *now){
   struct timeval tv;
   gettimeofday(&tv, NULL);
   return gmtime_r(&(tv.tv_sec), now);
}

void crk_epoch(InternalDate *epoch){
   epoch->tm_year = 70;
   epoch->tm_mon = 0;
   epoch->tm_mday = 1;
}

char *crk_ctime_r(int64_t t, char * buf){
   const time_t lt = (const time_t)t;
   return ctime_r(&lt, buf);
}

char **get_environ(){
   return environ;
}


#include "ext/Module.h"
#include "ext/Type.h"
#include "ext/Func.h"

extern "C"
void crack_runtime_time_init(crack::ext::Module *mod) {
    crack::ext::Func *f;
    crack::ext::Type *type_Class = mod->getClassType();
    crack::ext::Type *type_void = mod->getVoidType();
    crack::ext::Type *type_voidptr = mod->getVoidptrType();
    crack::ext::Type *type_bool = mod->getBoolType();
    crack::ext::Type *type_byteptr = mod->getByteptrType();
    crack::ext::Type *type_byte = mod->getByteType();
    crack::ext::Type *type_int32 = mod->getInt32Type();
    crack::ext::Type *type_int64 = mod->getInt64Type();
    crack::ext::Type *type_uint32 = mod->getUint32Type();
    crack::ext::Type *type_uint64 = mod->getUint64Type();
    crack::ext::Type *type_int = mod->getIntType();
    crack::ext::Type *type_uint = mod->getUintType();
    crack::ext::Type *type_float32 = mod->getFloat32Type();
    crack::ext::Type *type_float64 = mod->getFloat64Type();
    crack::ext::Type *type_float = mod->getFloatType();

    crack::ext::Type *type_InternalDate = mod->addType("InternalDate");
        type_InternalDate->addInstVar(type_int, "tm_sec");
        type_InternalDate->addInstVar(type_int, "tm_min");
        type_InternalDate->addInstVar(type_int, "tm_hour");
        type_InternalDate->addInstVar(type_int, "tm_mday");
        type_InternalDate->addInstVar(type_int, "tm_mon");
        type_InternalDate->addInstVar(type_int, "tm_year");
        type_InternalDate->addInstVar(type_int, "tm_wday");
        type_InternalDate->addInstVar(type_int, "tm_yday");
        type_InternalDate->addInstVar(type_int, "tm_isdst");
        type_InternalDate->addInstVar(type_int64, "tm_gmtoff");
        type_InternalDate->addInstVar(type_byteptr, "tm_zone");
        f = type_InternalDate->addConstructor("init",
                    (void *)crk_create_date
            );

        f = type_InternalDate->addMethod(type_int64, "getSeconds",
                    (void *)mktime
            );

        f = type_InternalDate->addMethod(type_void, "setLocalSeconds",
                    (void *)crk_localtime
            );
            f->addArg(type_int64, "t");

        f = type_InternalDate->addMethod(type_void, "setLocalNow",
                    (void *)crk_localtime_now
            );

        f = type_InternalDate->addMethod(type_void, "setUTCSeconds",
                    (void *)crk_gmtime
            );
            f->addArg(type_int64, "t");

        f = type_InternalDate->addMethod(type_void, "setUTCNow",
                    (void *)crk_gmtime_now
            );

        f = type_InternalDate->addMethod(type_void, "setEpoch",
                    (void *)crk_epoch
            );

        f = type_InternalDate->addMethod(type_void, "_toBufferRaw",
                    (void *)asctime_r
            );
            f->addArg(type_byteptr, "buf");

    type_InternalDate->finish();


    crack::ext::Type *array = mod->getType("array");

    crack::ext::Type *array_pbyteptr_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_byteptr;
        array_pbyteptr_q = array->getSpecialization(params);
    }
    f = mod->addFunc(type_int64, "mktime",
            (void *)mktime
        );
       f->addArg(type_InternalDate, "d");

    f = mod->addFunc(type_InternalDate, "localtime",
            (void *)crk_localtime
        );
       f->addArg(type_InternalDate, "d");
       f->addArg(type_int64, "t");

    f = mod->addFunc(type_InternalDate, "localtime_now",
            (void *)crk_localtime_now
        );
       f->addArg(type_InternalDate, "now");

    f = mod->addFunc(type_InternalDate, "gmtime_now",
            (void *)crk_gmtime_now
        );
       f->addArg(type_InternalDate, "now");

    f = mod->addFunc(type_InternalDate, "gmtime",
            (void *)crk_gmtime
        );
       f->addArg(type_InternalDate, "now");
       f->addArg(type_int64, "t");

    f = mod->addFunc(type_void, "epoch",
            (void *)crk_epoch
        );
       f->addArg(type_InternalDate, "epoch");

    f = mod->addFunc(type_byteptr, "asctime",
            (void *)asctime_r
        );
       f->addArg(type_InternalDate, "d");
       f->addArg(type_byteptr, "buf");

    f = mod->addFunc(type_byteptr, "ctime",
            (void *)crk_ctime_r
        );
       f->addArg(type_int64, "seconds");
       f->addArg(type_byteptr, "buf");

    f = mod->addFunc(type_uint64, "strftime",
            (void *)strftime
        );
       f->addArg(type_byteptr, "s");
       f->addArg(type_uint64, "max");
       f->addArg(type_byteptr, "format");
       f->addArg(type_InternalDate, "d");

    f = mod->addFunc(array_pbyteptr_q, "get_environ",
            (void *)get_environ
        );

    f = mod->addFunc(type_int, "putenv",
            (void *)putenv
        );
       f->addArg(type_byteptr, "keyvalue");

    f = mod->addFunc(type_byteptr, "getenv",
            (void *)getenv
        );
       f->addArg(type_byteptr, "name");

    f = mod->addFunc(type_int, "setenv",
            (void *)setenv
        );
       f->addArg(type_byteptr, "name");
       f->addArg(type_byteptr, "value");
       f->addArg(type_int, "overwrite");

    f = mod->addFunc(type_int, "unsetenv",
            (void *)unsetenv
        );
       f->addArg(type_byteptr, "name");

}
