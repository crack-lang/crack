#include <png.h>
typedef int Undef;


#include "ext/Module.h"
#include "ext/Type.h"
#include "ext/Func.h"

extern "C"
void crack_ext__png_rinit() {
    return;
}

extern "C"
void crack_ext__png_cinit(crack::ext::Module *mod) {
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

    crack::ext::Type *type_png_structp = mod->addType("png_structp", sizeof(png_structp));
    type_png_structp->finish();


    crack::ext::Type *type_png_struct = mod->addType("png_struct", sizeof(Undef));
    type_png_struct->finish();


    crack::ext::Type *type_png_info = mod->addType("png_info", sizeof(Undef));
    type_png_info->finish();


    crack::ext::Type *array = mod->getType("array");

    crack::ext::Type *array_puint32_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_uint32;
        array_puint32_q = array->getSpecialization(params);
    }

    crack::ext::Type *array_pint_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_int;
        array_pint_q = array->getSpecialization(params);
    }

    crack::ext::Type *array_pbyteptr_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_byteptr;
        array_pbyteptr_q = array->getSpecialization(params);
    }

    crack::ext::Type *array_ppng__info_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_png_info;
        array_ppng__info_q = array->getSpecialization(params);
    }
    f = mod->addFunc(type_png_struct, "png_create_read_struct",
                     (void *)png_create_read_struct
                     );
       f->addArg(type_byteptr, "user_png_ver");
       f->addArg(type_voidptr, "error_ptr");
       f->addArg(type_voidptr, "error_fn");
       f->addArg(type_voidptr, "warn_fn");

    f = mod->addFunc(type_png_info, "png_create_info_struct",
                     (void *)png_create_info_struct
                     );
       f->addArg(type_png_struct, "png_ptr");

    f = mod->addFunc(type_void, "png_set_read_fn",
                     (void *)png_set_read_fn
                     );
       f->addArg(type_png_struct, "png_ptr");
       f->addArg(type_voidptr, "io_ptr");
       f->addArg(type_voidptr, "read_data_fn");

    f = mod->addFunc(type_void, "png_read_png",
                     (void *)png_read_png
                     );
       f->addArg(type_png_struct, "png_ptr");
       f->addArg(type_png_info, "info_ptr");
       f->addArg(type_int, "transforms");
       f->addArg(type_voidptr, "params");

    f = mod->addFunc(type_uint32, "png_get_IHDR",
                     (void *)png_get_IHDR
                     );
       f->addArg(type_png_struct, "png_ptr");
       f->addArg(type_png_info, "info_ptr");
       f->addArg(array_puint32_q, "width");
       f->addArg(array_puint32_q, "height");
       f->addArg(array_pint_q, "bit_depth");
       f->addArg(array_pint_q, "color_type");
       f->addArg(array_pint_q, "interlace_method");
       f->addArg(array_pint_q, "compression_method");
       f->addArg(array_pint_q, "filter_method");

    f = mod->addFunc(array_pbyteptr_q, "png_get_rows",
                     (void *)png_get_rows
                     );
       f->addArg(type_png_struct, "png_ptr");
       f->addArg(type_png_info, "info_ptr");

    f = mod->addFunc(type_void, "png_destroy_read_struct",
                     (void *)png_destroy_read_struct
                     );
       f->addArg(type_png_struct, "png_ptr");
       f->addArg(array_ppng__info_q, "info_ptr_ptr");
       f->addArg(array_ppng__info_q, "end_info_ptr_ptr");

    f = mod->addFunc(type_voidptr, "png_get_io_ptr",
                     (void *)png_get_io_ptr
                     );
       f->addArg(type_png_struct, "png_ptr");


    mod->addConstant(type_uint, "PNG_COLOR_TYPE_GRAY",
                     static_cast<int>(PNG_COLOR_TYPE_GRAY)
                     );

    mod->addConstant(type_uint, "PNG_COLOR_TYPE_PALETTE",
                     static_cast<int>(PNG_COLOR_TYPE_PALETTE)
                     );

    mod->addConstant(type_uint, "PNG_COLOR_TYPE_RGB",
                     static_cast<int>(PNG_COLOR_TYPE_RGB)
                     );

    mod->addConstant(type_uint, "PNG_COLOR_TYPE_RGB_ALPHA",
                     static_cast<int>(PNG_COLOR_TYPE_RGB_ALPHA)
                     );

    mod->addConstant(type_uint, "PNG_COLOR_TYPE_GRAY_ALPHA",
                     static_cast<int>(PNG_COLOR_TYPE_GRAY_ALPHA)
                     );

    mod->addConstant(type_uint, "PNG_COLOR_TYPE_RGBA",
                     static_cast<int>(PNG_COLOR_TYPE_RGBA)
                     );

    mod->addConstant(type_uint, "PNG_COLOR_TYPE_GA",
                     static_cast<int>(PNG_COLOR_TYPE_GA)
                     );
}
