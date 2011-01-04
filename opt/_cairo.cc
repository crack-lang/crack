#include <cairo.h>


#include "ext/Module.h"
#include "ext/Type.h"
#include "ext/Func.h"

extern "C"
void crack_ext__cairo_init(crack::ext::Module *mod) {
    crack::ext::Func *f;
    crack::ext::Type *type_Class = mod->getClassType();

    crack::ext::Type *array = mod->getType("array");
    crack::ext::Type *type_float64 = mod->getFloat64Type();

    crack::ext::Type *array_pfloat64_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_float64;
        array_pfloat64_q = array->getSpecialization(params);
    }
    crack::ext::Type *type_bool = mod->getBoolType();
    crack::ext::Type *type_byte = mod->getByteType();
    crack::ext::Type *type_byteptr = mod->getByteptrType();

    crack::ext::Type *type_cairo_device_t = mod->addType("cairo_device_t");
    type_cairo_device_t->finish();

    crack::ext::Type *type_cairo_matrix_t = mod->addType("cairo_matrix_t");
    type_cairo_matrix_t->finish();

    crack::ext::Type *type_cairo_pattern_t = mod->addType("cairo_pattern_t");
    type_cairo_pattern_t->finish();

    crack::ext::Type *type_cairo_surface_t = mod->addType("cairo_surface_t");
    type_cairo_surface_t->finish();

    crack::ext::Type *type_cairo_t = mod->addType("cairo_t");
    type_cairo_t->finish();
    crack::ext::Type *type_float = mod->getFloatType();
    crack::ext::Type *type_float32 = mod->getFloat32Type();
    crack::ext::Type *type_int = mod->getIntType();
    crack::ext::Type *type_int32 = mod->getInt32Type();
    crack::ext::Type *type_int64 = mod->getInt64Type();
    crack::ext::Type *type_uint = mod->getUintType();
    crack::ext::Type *type_uint32 = mod->getUint32Type();
    crack::ext::Type *type_uint64 = mod->getUint64Type();
    crack::ext::Type *type_void = mod->getVoidType();
    crack::ext::Type *type_voidptr = mod->getVoidptrType();

    f = mod->addFunc(type_cairo_t, "cairo_create",
                     (void *)cairo_create
                     );
    f->addArg(type_cairo_surface_t, "target");

    f = mod->addFunc(type_cairo_t, "cairo_reference",
                     (void *)cairo_reference
                     );
    f->addArg(type_cairo_t, "cr");

    f = mod->addFunc(type_void, "cairo_destroy",
                     (void *)cairo_destroy
                     );
    f->addArg(type_cairo_t, "cr");

    f = mod->addFunc(type_uint, "cairo_get_reference_count",
                     (void *)cairo_get_reference_count
                     );
    f->addArg(type_cairo_t, "cr");

    f = mod->addFunc(type_void, "cairo_save",
                     (void *)cairo_save
                     );
    f->addArg(type_cairo_t, "cr");

    f = mod->addFunc(type_void, "cairo_restore",
                     (void *)cairo_restore
                     );
    f->addArg(type_cairo_t, "cr");

    f = mod->addFunc(type_void, "cairo_push_group",
                     (void *)cairo_push_group
                     );
    f->addArg(type_cairo_t, "cr");

    f = mod->addFunc(type_cairo_pattern_t, "cairo_pop_group",
                     (void *)cairo_pop_group
                     );
    f->addArg(type_cairo_t, "cr");

    f = mod->addFunc(type_void, "cairo_pop_group_to_source",
                     (void *)cairo_pop_group_to_source
                     );
    f->addArg(type_cairo_t, "cr");

    f = mod->addFunc(type_void, "cairo_set_operator",
                     (void *)cairo_set_operator
                     );
    f->addArg(type_cairo_t, "cr");
    f->addArg(type_uint32, "op");

    f = mod->addFunc(type_void, "cairo_set_source",
                     (void *)cairo_set_source
                     );
    f->addArg(type_cairo_t, "cr");
    f->addArg(type_cairo_pattern_t, "source");

    f = mod->addFunc(type_void, "cairo_set_source_rgb",
                     (void *)cairo_set_source_rgb
                     );
    f->addArg(type_cairo_t, "cr");
    f->addArg(type_float64, "red");
    f->addArg(type_float64, "green");
    f->addArg(type_float64, "blue");

    f = mod->addFunc(type_void, "cairo_set_source_rgba",
                     (void *)cairo_set_source_rgba
                     );
    f->addArg(type_cairo_t, "cr");
    f->addArg(type_float64, "red");
    f->addArg(type_float64, "green");
    f->addArg(type_float64, "blue");
    f->addArg(type_float64, "alpha");

    f = mod->addFunc(type_void, "cairo_set_source_surface",
                     (void *)cairo_set_source_surface
                     );
    f->addArg(type_cairo_t, "cr");
    f->addArg(type_cairo_surface_t, "surface");
    f->addArg(type_float64, "x");
    f->addArg(type_float64, "y");

    f = mod->addFunc(type_void, "cairo_set_tolerance",
                     (void *)cairo_set_tolerance
                     );
    f->addArg(type_cairo_t, "cr");
    f->addArg(type_float64, "tolerance");

    f = mod->addFunc(type_void, "cairo_set_antialias",
                     (void *)cairo_set_antialias
                     );
    f->addArg(type_cairo_t, "cr");
    f->addArg(type_uint32, "antialias");

    f = mod->addFunc(type_void, "cairo_set_fill_rule",
                     (void *)cairo_set_fill_rule
                     );
    f->addArg(type_cairo_t, "cr");
    f->addArg(type_uint32, "fill_rule");

    f = mod->addFunc(type_void, "cairo_set_line_width",
                     (void *)cairo_set_line_width
                     );
    f->addArg(type_cairo_t, "cr");
    f->addArg(type_float64, "width");

    f = mod->addFunc(type_void, "cairo_set_line_cap",
                     (void *)cairo_set_line_cap
                     );
    f->addArg(type_cairo_t, "cr");
    f->addArg(type_uint32, "line_cap");

    f = mod->addFunc(type_void, "cairo_set_line_join",
                     (void *)cairo_set_line_join
                     );
    f->addArg(type_cairo_t, "cr");
    f->addArg(type_uint32, "line_join");

    f = mod->addFunc(type_void, "cairo_set_dash",
                     (void *)cairo_set_dash
                     );
    f->addArg(type_cairo_t, "cr");
    f->addArg(array_pfloat64_q, "dashes");
    f->addArg(type_int, "num_dashes");
    f->addArg(type_float64, "offset");

    f = mod->addFunc(type_void, "cairo_set_miter_limit",
                     (void *)cairo_set_miter_limit
                     );
    f->addArg(type_cairo_t, "cr");
    f->addArg(type_float64, "limit");

    f = mod->addFunc(type_void, "cairo_translate",
                     (void *)cairo_translate
                     );
    f->addArg(type_cairo_t, "cr");
    f->addArg(type_float64, "tx");
    f->addArg(type_float64, "ty");

    f = mod->addFunc(type_void, "cairo_scale",
                     (void *)cairo_scale
                     );
    f->addArg(type_cairo_t, "cr");
    f->addArg(type_float64, "sx");
    f->addArg(type_float64, "sy");

    f = mod->addFunc(type_void, "cairo_rotate",
                     (void *)cairo_rotate
                     );
    f->addArg(type_cairo_t, "cr");
    f->addArg(type_float64, "angle");

    f = mod->addFunc(type_void, "cairo_transform",
                     (void *)cairo_transform
                     );
    f->addArg(type_cairo_t, "cr");
    f->addArg(type_cairo_matrix_t, "matrix");

    f = mod->addFunc(type_void, "cairo_set_matrix",
                     (void *)cairo_set_matrix
                     );
    f->addArg(type_cairo_t, "cr");
    f->addArg(type_cairo_matrix_t, "matrix");

    f = mod->addFunc(type_void, "cairo_identity_matrix",
                     (void *)cairo_identity_matrix
                     );
    f->addArg(type_cairo_t, "cr");

    f = mod->addFunc(type_void, "cairo_user_to_device",
                     (void *)cairo_user_to_device
                     );
    f->addArg(type_cairo_t, "cr");
    f->addArg(array_pfloat64_q, "x");
    f->addArg(array_pfloat64_q, "y");

    f = mod->addFunc(type_void, "cairo_user_to_device_distance",
                     (void *)cairo_user_to_device_distance
                     );
    f->addArg(type_cairo_t, "cr");
    f->addArg(array_pfloat64_q, "dx");
    f->addArg(array_pfloat64_q, "dy");

    f = mod->addFunc(type_void, "cairo_device_to_user",
                     (void *)cairo_device_to_user
                     );
    f->addArg(type_cairo_t, "cr");
    f->addArg(array_pfloat64_q, "x");
    f->addArg(array_pfloat64_q, "y");

    f = mod->addFunc(type_void, "cairo_device_to_user_distance",
                     (void *)cairo_device_to_user_distance
                     );
    f->addArg(type_cairo_t, "cr");
    f->addArg(array_pfloat64_q, "dx");
    f->addArg(array_pfloat64_q, "dy");

    f = mod->addFunc(type_void, "cairo_new_path",
                     (void *)cairo_new_path
                     );
    f->addArg(type_cairo_t, "cr");

    f = mod->addFunc(type_void, "cairo_move_to",
                     (void *)cairo_move_to
                     );
    f->addArg(type_cairo_t, "cr");
    f->addArg(type_float64, "x");
    f->addArg(type_float64, "y");

    f = mod->addFunc(type_void, "cairo_new_sub_path",
                     (void *)cairo_new_sub_path
                     );
    f->addArg(type_cairo_t, "cr");

    f = mod->addFunc(type_void, "cairo_line_to",
                     (void *)cairo_line_to
                     );
    f->addArg(type_cairo_t, "cr");
    f->addArg(type_float64, "x");
    f->addArg(type_float64, "y");

    f = mod->addFunc(type_void, "cairo_curve_to",
                     (void *)cairo_curve_to
                     );
    f->addArg(type_cairo_t, "cr");
    f->addArg(type_float64, "x1");
    f->addArg(type_float64, "y1");
    f->addArg(type_float64, "x2");
    f->addArg(type_float64, "y2");
    f->addArg(type_float64, "x3");
    f->addArg(type_float64, "y3");

    f = mod->addFunc(type_void, "cairo_arc",
                     (void *)cairo_arc
                     );
    f->addArg(type_cairo_t, "cr");
    f->addArg(type_float64, "xc");
    f->addArg(type_float64, "yc");
    f->addArg(type_float64, "radius");
    f->addArg(type_float64, "angle1");
    f->addArg(type_float64, "angle2");

    f = mod->addFunc(type_void, "cairo_arc_negative",
                     (void *)cairo_arc_negative
                     );
    f->addArg(type_cairo_t, "cr");
    f->addArg(type_float64, "xc");
    f->addArg(type_float64, "yc");
    f->addArg(type_float64, "radius");
    f->addArg(type_float64, "angle1");
    f->addArg(type_float64, "angle2");

    f = mod->addFunc(type_void, "cairo_rel_move_to",
                     (void *)cairo_rel_move_to
                     );
    f->addArg(type_cairo_t, "cr");
    f->addArg(type_float64, "dx");
    f->addArg(type_float64, "dy");

    f = mod->addFunc(type_void, "cairo_rel_line_to",
                     (void *)cairo_rel_line_to
                     );
    f->addArg(type_cairo_t, "cr");
    f->addArg(type_float64, "dx");
    f->addArg(type_float64, "dy");

    f = mod->addFunc(type_void, "cairo_rel_curve_to",
                     (void *)cairo_rel_curve_to
                     );
    f->addArg(type_cairo_t, "cr");
    f->addArg(type_float64, "dx1");
    f->addArg(type_float64, "dy1");
    f->addArg(type_float64, "dx2");
    f->addArg(type_float64, "dy2");
    f->addArg(type_float64, "dx3");
    f->addArg(type_float64, "dy3");

    f = mod->addFunc(type_void, "cairo_rectangle",
                     (void *)cairo_rectangle
                     );
    f->addArg(type_cairo_t, "cr");
    f->addArg(type_float64, "x");
    f->addArg(type_float64, "y");
    f->addArg(type_float64, "width");
    f->addArg(type_float64, "height");

    f = mod->addFunc(type_void, "cairo_close_path",
                     (void *)cairo_close_path
                     );
    f->addArg(type_cairo_t, "cr");

    f = mod->addFunc(type_void, "cairo_path_extents",
                     (void *)cairo_path_extents
                     );
    f->addArg(type_cairo_t, "cr");
    f->addArg(array_pfloat64_q, "x1");
    f->addArg(array_pfloat64_q, "y1");
    f->addArg(array_pfloat64_q, "x2");
    f->addArg(array_pfloat64_q, "y2");

    f = mod->addFunc(type_void, "cairo_paint",
                     (void *)cairo_paint
                     );
    f->addArg(type_cairo_t, "cr");

    f = mod->addFunc(type_void, "cairo_paint_with_alpha",
                     (void *)cairo_paint_with_alpha
                     );
    f->addArg(type_cairo_t, "cr");
    f->addArg(type_float64, "alpha");

    f = mod->addFunc(type_void, "cairo_mask",
                     (void *)cairo_mask
                     );
    f->addArg(type_cairo_t, "cr");
    f->addArg(type_cairo_pattern_t, "pattern");

    f = mod->addFunc(type_void, "cairo_mask_surface",
                     (void *)cairo_mask_surface
                     );
    f->addArg(type_cairo_t, "cr");
    f->addArg(type_cairo_surface_t, "surface");
    f->addArg(type_float64, "surface_x");
    f->addArg(type_float64, "surface_y");

    f = mod->addFunc(type_void, "cairo_stroke",
                     (void *)cairo_stroke
                     );
    f->addArg(type_cairo_t, "cr");

    f = mod->addFunc(type_void, "cairo_stroke_preserve",
                     (void *)cairo_stroke_preserve
                     );
    f->addArg(type_cairo_t, "cr");

    f = mod->addFunc(type_void, "cairo_fill",
                     (void *)cairo_fill
                     );
    f->addArg(type_cairo_t, "cr");

    f = mod->addFunc(type_void, "cairo_fill_preserve",
                     (void *)cairo_fill_preserve
                     );
    f->addArg(type_cairo_t, "cr");

    f = mod->addFunc(type_void, "cairo_copy_page",
                     (void *)cairo_copy_page
                     );
    f->addArg(type_cairo_t, "cr");

    f = mod->addFunc(type_void, "cairo_show_page",
                     (void *)cairo_show_page
                     );
    f->addArg(type_cairo_t, "cr");

    f = mod->addFunc(type_bool, "cairo_in_stroke",
                     (void *)cairo_in_stroke
                     );
    f->addArg(type_cairo_t, "cr");
    f->addArg(type_float64, "x");
    f->addArg(type_float64, "y");

    f = mod->addFunc(type_bool, "cairo_in_fill",
                     (void *)cairo_in_fill
                     );
    f->addArg(type_cairo_t, "cr");
    f->addArg(type_float64, "x");
    f->addArg(type_float64, "y");

    f = mod->addFunc(type_bool, "cairo_in_clip",
                     (void *)cairo_in_clip
                     );
    f->addArg(type_cairo_t, "cr");
    f->addArg(type_float64, "x");
    f->addArg(type_float64, "y");

    f = mod->addFunc(type_void, "cairo_stroke_extents",
                     (void *)cairo_stroke_extents
                     );
    f->addArg(type_cairo_t, "cr");
    f->addArg(array_pfloat64_q, "x1");
    f->addArg(array_pfloat64_q, "y1");
    f->addArg(array_pfloat64_q, "x2");
    f->addArg(array_pfloat64_q, "y2");

    f = mod->addFunc(type_void, "cairo_fill_extents",
                     (void *)cairo_fill_extents
                     );
    f->addArg(type_cairo_t, "cr");
    f->addArg(array_pfloat64_q, "x1");
    f->addArg(array_pfloat64_q, "y1");
    f->addArg(array_pfloat64_q, "x2");
    f->addArg(array_pfloat64_q, "y2");

    f = mod->addFunc(type_void, "cairo_reset_clip",
                     (void *)cairo_reset_clip
                     );
    f->addArg(type_cairo_t, "cr");

    f = mod->addFunc(type_void, "cairo_clip",
                     (void *)cairo_clip
                     );
    f->addArg(type_cairo_t, "cr");

    f = mod->addFunc(type_void, "cairo_clip_preserve",
                     (void *)cairo_clip_preserve
                     );
    f->addArg(type_cairo_t, "cr");

    f = mod->addFunc(type_void, "cairo_clip_extents",
                     (void *)cairo_clip_extents
                     );
    f->addArg(type_cairo_t, "cr");
    f->addArg(array_pfloat64_q, "x1");
    f->addArg(array_pfloat64_q, "y1");
    f->addArg(array_pfloat64_q, "x2");
    f->addArg(array_pfloat64_q, "y2");

    mod->addConstant(type_uint32, "CAIRO_STATUS_SUCCESS",
                     static_cast<int>(CAIRO_STATUS_SUCCESS)
                     );

    mod->addConstant(type_uint32, "CAIRO_STATUS_NO_MEMORY",
                     static_cast<int>(CAIRO_STATUS_NO_MEMORY)
                     );

    mod->addConstant(type_uint32, "CAIRO_STATUS_INVALID_RESTORE",
                     static_cast<int>(CAIRO_STATUS_INVALID_RESTORE)
                     );

    mod->addConstant(type_uint32, "CAIRO_STATUS_INVALID_POP_GROUP",
                     static_cast<int>(CAIRO_STATUS_INVALID_POP_GROUP)
                     );

    mod->addConstant(type_uint32, "CAIRO_STATUS_NO_CURRENT_POINT",
                     static_cast<int>(CAIRO_STATUS_NO_CURRENT_POINT)
                     );

    mod->addConstant(type_uint32, "CAIRO_STATUS_INVALID_MATRIX",
                     static_cast<int>(CAIRO_STATUS_INVALID_MATRIX)
                     );

    mod->addConstant(type_uint32, "CAIRO_STATUS_INVALID_STATUS",
                     static_cast<int>(CAIRO_STATUS_INVALID_STATUS)
                     );

    mod->addConstant(type_uint32, "CAIRO_STATUS_NULL_POINTER",
                     static_cast<int>(CAIRO_STATUS_NULL_POINTER)
                     );

    mod->addConstant(type_uint32, "CAIRO_STATUS_INVALID_STRING",
                     static_cast<int>(CAIRO_STATUS_INVALID_STRING)
                     );

    mod->addConstant(type_uint32, "CAIRO_STATUS_INVALID_PATH_DATA",
                     static_cast<int>(CAIRO_STATUS_INVALID_PATH_DATA)
                     );

    mod->addConstant(type_uint32, "CAIRO_STATUS_READ_ERROR",
                     static_cast<int>(CAIRO_STATUS_READ_ERROR)
                     );

    mod->addConstant(type_uint32, "CAIRO_STATUS_WRITE_ERROR",
                     static_cast<int>(CAIRO_STATUS_WRITE_ERROR)
                     );

    mod->addConstant(type_uint32, "CAIRO_STATUS_SURFACE_FINISHED",
                     static_cast<int>(CAIRO_STATUS_SURFACE_FINISHED)
                     );

    mod->addConstant(type_uint32, "CAIRO_STATUS_SURFACE_TYPE_MISMATCH",
                     static_cast<int>(CAIRO_STATUS_SURFACE_TYPE_MISMATCH)
                     );

    mod->addConstant(type_uint32, "CAIRO_STATUS_PATTERN_TYPE_MISMATCH",
                     static_cast<int>(CAIRO_STATUS_PATTERN_TYPE_MISMATCH)
                     );

    mod->addConstant(type_uint32, "CAIRO_STATUS_INVALID_CONTENT",
                     static_cast<int>(CAIRO_STATUS_INVALID_CONTENT)
                     );

    mod->addConstant(type_uint32, "CAIRO_STATUS_INVALID_FORMAT",
                     static_cast<int>(CAIRO_STATUS_INVALID_FORMAT)
                     );

    mod->addConstant(type_uint32, "CAIRO_STATUS_INVALID_VISUAL",
                     static_cast<int>(CAIRO_STATUS_INVALID_VISUAL)
                     );

    mod->addConstant(type_uint32, "CAIRO_STATUS_FILE_NOT_FOUND",
                     static_cast<int>(CAIRO_STATUS_FILE_NOT_FOUND)
                     );

    mod->addConstant(type_uint32, "CAIRO_STATUS_INVALID_DASH",
                     static_cast<int>(CAIRO_STATUS_INVALID_DASH)
                     );

    mod->addConstant(type_uint32, "CAIRO_STATUS_INVALID_DSC_COMMENT",
                     static_cast<int>(CAIRO_STATUS_INVALID_DSC_COMMENT)
                     );

    mod->addConstant(type_uint32, "CAIRO_STATUS_INVALID_INDEX",
                     static_cast<int>(CAIRO_STATUS_INVALID_INDEX)
                     );

    mod->addConstant(type_uint32, "CAIRO_STATUS_CLIP_NOT_REPRESENTABLE",
                     static_cast<int>(CAIRO_STATUS_CLIP_NOT_REPRESENTABLE)
                     );

    mod->addConstant(type_uint32, "CAIRO_STATUS_TEMP_FILE_ERROR",
                     static_cast<int>(CAIRO_STATUS_TEMP_FILE_ERROR)
                     );

    mod->addConstant(type_uint32, "CAIRO_STATUS_INVALID_STRIDE",
                     static_cast<int>(CAIRO_STATUS_INVALID_STRIDE)
                     );

    mod->addConstant(type_uint32, "CAIRO_STATUS_FONT_TYPE_MISMATCH",
                     static_cast<int>(CAIRO_STATUS_FONT_TYPE_MISMATCH)
                     );

    mod->addConstant(type_uint32, "CAIRO_STATUS_USER_FONT_IMMUTABLE",
                     static_cast<int>(CAIRO_STATUS_USER_FONT_IMMUTABLE)
                     );

    mod->addConstant(type_uint32, "CAIRO_STATUS_USER_FONT_ERROR",
                     static_cast<int>(CAIRO_STATUS_USER_FONT_ERROR)
                     );

    mod->addConstant(type_uint32, "CAIRO_STATUS_NEGATIVE_COUNT",
                     static_cast<int>(CAIRO_STATUS_NEGATIVE_COUNT)
                     );

    mod->addConstant(type_uint32, "CAIRO_STATUS_INVALID_CLUSTERS",
                     static_cast<int>(CAIRO_STATUS_INVALID_CLUSTERS)
                     );

    mod->addConstant(type_uint32, "CAIRO_STATUS_INVALID_SLANT",
                     static_cast<int>(CAIRO_STATUS_INVALID_SLANT)
                     );

    mod->addConstant(type_uint32, "CAIRO_STATUS_INVALID_WEIGHT",
                     static_cast<int>(CAIRO_STATUS_INVALID_WEIGHT)
                     );

    mod->addConstant(type_uint32, "CAIRO_STATUS_INVALID_SIZE",
                     static_cast<int>(CAIRO_STATUS_INVALID_SIZE)
                     );

    mod->addConstant(type_uint32, "CAIRO_STATUS_USER_FONT_NOT_IMPLEMENTED",
                     static_cast<int>(CAIRO_STATUS_USER_FONT_NOT_IMPLEMENTED)
                     );

    mod->addConstant(type_uint32, "CAIRO_STATUS_DEVICE_TYPE_MISMATCH",
                     static_cast<int>(CAIRO_STATUS_DEVICE_TYPE_MISMATCH)
                     );

    mod->addConstant(type_uint32, "CAIRO_STATUS_DEVICE_ERROR",
                     static_cast<int>(CAIRO_STATUS_DEVICE_ERROR)
                     );

    mod->addConstant(type_uint32, "CAIRO_STATUS_LAST_STATUS",
                     static_cast<int>(CAIRO_STATUS_LAST_STATUS)
                     );

    mod->addConstant(type_uint32, "CAIRO_CONTENT_COLOR",
                     static_cast<int>(CAIRO_CONTENT_COLOR)
                     );

    mod->addConstant(type_uint32, "CAIRO_CONTENT_ALPHA",
                     static_cast<int>(CAIRO_CONTENT_ALPHA)
                     );

    mod->addConstant(type_uint32, "CAIRO_CONTENT_COLOR_ALPHA",
                     static_cast<int>(CAIRO_CONTENT_COLOR_ALPHA)
                     );

    mod->addConstant(type_uint32, "CAIRO_OPERATOR_CLEAR",
                     static_cast<int>(CAIRO_OPERATOR_CLEAR)
                     );

    mod->addConstant(type_uint32, "CAIRO_OPERATOR_SOURCE",
                     static_cast<int>(CAIRO_OPERATOR_SOURCE)
                     );

    mod->addConstant(type_uint32, "CAIRO_OPERATOR_OVER",
                     static_cast<int>(CAIRO_OPERATOR_OVER)
                     );

    mod->addConstant(type_uint32, "CAIRO_OPERATOR_IN",
                     static_cast<int>(CAIRO_OPERATOR_IN)
                     );

    mod->addConstant(type_uint32, "CAIRO_OPERATOR_OUT",
                     static_cast<int>(CAIRO_OPERATOR_OUT)
                     );

    mod->addConstant(type_uint32, "CAIRO_OPERATOR_ATOP",
                     static_cast<int>(CAIRO_OPERATOR_ATOP)
                     );

    mod->addConstant(type_uint32, "CAIRO_OPERATOR_DEST",
                     static_cast<int>(CAIRO_OPERATOR_DEST)
                     );

    mod->addConstant(type_uint32, "CAIRO_OPERATOR_DEST_OVER",
                     static_cast<int>(CAIRO_OPERATOR_DEST_OVER)
                     );

    mod->addConstant(type_uint32, "CAIRO_OPERATOR_DEST_IN",
                     static_cast<int>(CAIRO_OPERATOR_DEST_IN)
                     );

    mod->addConstant(type_uint32, "CAIRO_OPERATOR_DEST_OUT",
                     static_cast<int>(CAIRO_OPERATOR_DEST_OUT)
                     );

    mod->addConstant(type_uint32, "CAIRO_OPERATOR_DEST_ATOP",
                     static_cast<int>(CAIRO_OPERATOR_DEST_ATOP)
                     );

    mod->addConstant(type_uint32, "CAIRO_OPERATOR_XOR",
                     static_cast<int>(CAIRO_OPERATOR_XOR)
                     );

    mod->addConstant(type_uint32, "CAIRO_OPERATOR_ADD",
                     static_cast<int>(CAIRO_OPERATOR_ADD)
                     );

    mod->addConstant(type_uint32, "CAIRO_OPERATOR_SATURATE",
                     static_cast<int>(CAIRO_OPERATOR_SATURATE)
                     );

    mod->addConstant(type_uint32, "CAIRO_OPERATOR_MULTIPLY",
                     static_cast<int>(CAIRO_OPERATOR_MULTIPLY)
                     );

    mod->addConstant(type_uint32, "CAIRO_OPERATOR_SCREEN",
                     static_cast<int>(CAIRO_OPERATOR_SCREEN)
                     );

    mod->addConstant(type_uint32, "CAIRO_OPERATOR_OVERLAY",
                     static_cast<int>(CAIRO_OPERATOR_OVERLAY)
                     );

    mod->addConstant(type_uint32, "CAIRO_OPERATOR_DARKEN",
                     static_cast<int>(CAIRO_OPERATOR_DARKEN)
                     );

    mod->addConstant(type_uint32, "CAIRO_OPERATOR_LIGHTEN",
                     static_cast<int>(CAIRO_OPERATOR_LIGHTEN)
                     );

    mod->addConstant(type_uint32, "CAIRO_OPERATOR_COLOR_DODGE",
                     static_cast<int>(CAIRO_OPERATOR_COLOR_DODGE)
                     );

    mod->addConstant(type_uint32, "CAIRO_OPERATOR_COLOR_BURN",
                     static_cast<int>(CAIRO_OPERATOR_COLOR_BURN)
                     );

    mod->addConstant(type_uint32, "CAIRO_OPERATOR_HARD_LIGHT",
                     static_cast<int>(CAIRO_OPERATOR_HARD_LIGHT)
                     );

    mod->addConstant(type_uint32, "CAIRO_OPERATOR_SOFT_LIGHT",
                     static_cast<int>(CAIRO_OPERATOR_SOFT_LIGHT)
                     );

    mod->addConstant(type_uint32, "CAIRO_OPERATOR_DIFFERENCE",
                     static_cast<int>(CAIRO_OPERATOR_DIFFERENCE)
                     );

    mod->addConstant(type_uint32, "CAIRO_OPERATOR_EXCLUSION",
                     static_cast<int>(CAIRO_OPERATOR_EXCLUSION)
                     );

    mod->addConstant(type_uint32, "CAIRO_OPERATOR_HSL_HUE",
                     static_cast<int>(CAIRO_OPERATOR_HSL_HUE)
                     );

    mod->addConstant(type_uint32, "CAIRO_OPERATOR_HSL_SATURATION",
                     static_cast<int>(CAIRO_OPERATOR_HSL_SATURATION)
                     );

    mod->addConstant(type_uint32, "CAIRO_OPERATOR_HSL_COLOR",
                     static_cast<int>(CAIRO_OPERATOR_HSL_COLOR)
                     );

    mod->addConstant(type_uint32, "CAIRO_OPERATOR_HSL_LUMINOSITY",
                     static_cast<int>(CAIRO_OPERATOR_HSL_LUMINOSITY)
                     );

    mod->addConstant(type_uint32, "CAIRO_ANTIALIAS_DEFAULT",
                     static_cast<int>(CAIRO_ANTIALIAS_DEFAULT)
                     );

    mod->addConstant(type_uint32, "CAIRO_ANTIALIAS_NONE",
                     static_cast<int>(CAIRO_ANTIALIAS_NONE)
                     );

    mod->addConstant(type_uint32, "CAIRO_ANTIALIAS_GRAY",
                     static_cast<int>(CAIRO_ANTIALIAS_GRAY)
                     );

    mod->addConstant(type_uint32, "CAIRO_ANTIALIAS_SUBPIXEL",
                     static_cast<int>(CAIRO_ANTIALIAS_SUBPIXEL)
                     );

    mod->addConstant(type_uint32, "CAIRO_FILL_RULE_WINDING",
                     static_cast<int>(CAIRO_FILL_RULE_WINDING)
                     );

    mod->addConstant(type_uint32, "CAIRO_FILL_RULE_EVEN_ODD",
                     static_cast<int>(CAIRO_FILL_RULE_EVEN_ODD)
                     );

    mod->addConstant(type_uint32, "CAIRO_LINE_CAP_BUTT",
                     static_cast<int>(CAIRO_LINE_CAP_BUTT)
                     );

    mod->addConstant(type_uint32, "CAIRO_LINE_CAP_ROUND",
                     static_cast<int>(CAIRO_LINE_CAP_ROUND)
                     );

    mod->addConstant(type_uint32, "CAIRO_LINE_CAP_SQUARE",
                     static_cast<int>(CAIRO_LINE_CAP_SQUARE)
                     );

    mod->addConstant(type_uint32, "CAIRO_LINE_JOIN_MITER",
                     static_cast<int>(CAIRO_LINE_JOIN_MITER)
                     );

    mod->addConstant(type_uint32, "CAIRO_LINE_JOIN_ROUND",
                     static_cast<int>(CAIRO_LINE_JOIN_ROUND)
                     );

    mod->addConstant(type_uint32, "CAIRO_LINE_JOIN_BEVEL",
                     static_cast<int>(CAIRO_LINE_JOIN_BEVEL)
                     );

    mod->addConstant(type_uint32, "CAIRO_TEXT_CLUSTER_FLAG_BACKWARD",
                     static_cast<int>(CAIRO_TEXT_CLUSTER_FLAG_BACKWARD)
                     );

    mod->addConstant(type_uint32, "CAIRO_FONT_SLANT_NORMAL",
                     static_cast<int>(CAIRO_FONT_SLANT_NORMAL)
                     );

    mod->addConstant(type_uint32, "CAIRO_FONT_SLANT_ITALIC",
                     static_cast<int>(CAIRO_FONT_SLANT_ITALIC)
                     );

    mod->addConstant(type_uint32, "CAIRO_FONT_SLANT_OBLIQUE",
                     static_cast<int>(CAIRO_FONT_SLANT_OBLIQUE)
                     );

    mod->addConstant(type_uint32, "CAIRO_FONT_WEIGHT_NORMAL",
                     static_cast<int>(CAIRO_FONT_WEIGHT_NORMAL)
                     );

    mod->addConstant(type_uint32, "CAIRO_FONT_WEIGHT_BOLD",
                     static_cast<int>(CAIRO_FONT_WEIGHT_BOLD)
                     );

    mod->addConstant(type_uint32, "CAIRO_SUBPIXEL_ORDER_DEFAULT",
                     static_cast<int>(CAIRO_SUBPIXEL_ORDER_DEFAULT)
                     );

    mod->addConstant(type_uint32, "CAIRO_SUBPIXEL_ORDER_RGB",
                     static_cast<int>(CAIRO_SUBPIXEL_ORDER_RGB)
                     );

    mod->addConstant(type_uint32, "CAIRO_SUBPIXEL_ORDER_BGR",
                     static_cast<int>(CAIRO_SUBPIXEL_ORDER_BGR)
                     );

    mod->addConstant(type_uint32, "CAIRO_SUBPIXEL_ORDER_VRGB",
                     static_cast<int>(CAIRO_SUBPIXEL_ORDER_VRGB)
                     );

    mod->addConstant(type_uint32, "CAIRO_SUBPIXEL_ORDER_VBGR",
                     static_cast<int>(CAIRO_SUBPIXEL_ORDER_VBGR)
                     );

    mod->addConstant(type_uint32, "CAIRO_HINT_STYLE_DEFAULT",
                     static_cast<int>(CAIRO_HINT_STYLE_DEFAULT)
                     );

    mod->addConstant(type_uint32, "CAIRO_HINT_STYLE_NONE",
                     static_cast<int>(CAIRO_HINT_STYLE_NONE)
                     );

    mod->addConstant(type_uint32, "CAIRO_HINT_STYLE_SLIGHT",
                     static_cast<int>(CAIRO_HINT_STYLE_SLIGHT)
                     );

    mod->addConstant(type_uint32, "CAIRO_HINT_STYLE_MEDIUM",
                     static_cast<int>(CAIRO_HINT_STYLE_MEDIUM)
                     );

    mod->addConstant(type_uint32, "CAIRO_HINT_STYLE_FULL",
                     static_cast<int>(CAIRO_HINT_STYLE_FULL)
                     );

    mod->addConstant(type_uint32, "CAIRO_HINT_METRICS_DEFAULT",
                     static_cast<int>(CAIRO_HINT_METRICS_DEFAULT)
                     );

    mod->addConstant(type_uint32, "CAIRO_HINT_METRICS_OFF",
                     static_cast<int>(CAIRO_HINT_METRICS_OFF)
                     );

    mod->addConstant(type_uint32, "CAIRO_HINT_METRICS_ON",
                     static_cast<int>(CAIRO_HINT_METRICS_ON)
                     );

    mod->addConstant(type_uint32, "CAIRO_FONT_TYPE_TOY",
                     static_cast<int>(CAIRO_FONT_TYPE_TOY)
                     );

    mod->addConstant(type_uint32, "CAIRO_FONT_TYPE_FT",
                     static_cast<int>(CAIRO_FONT_TYPE_FT)
                     );

    mod->addConstant(type_uint32, "CAIRO_FONT_TYPE_WIN32",
                     static_cast<int>(CAIRO_FONT_TYPE_WIN32)
                     );

    mod->addConstant(type_uint32, "CAIRO_FONT_TYPE_QUARTZ",
                     static_cast<int>(CAIRO_FONT_TYPE_QUARTZ)
                     );

    mod->addConstant(type_uint32, "CAIRO_FONT_TYPE_USER",
                     static_cast<int>(CAIRO_FONT_TYPE_USER)
                     );

    mod->addConstant(type_uint32, "CAIRO_PATH_MOVE_TO",
                     static_cast<int>(CAIRO_PATH_MOVE_TO)
                     );

    mod->addConstant(type_uint32, "CAIRO_PATH_LINE_TO",
                     static_cast<int>(CAIRO_PATH_LINE_TO)
                     );

    mod->addConstant(type_uint32, "CAIRO_PATH_CURVE_TO",
                     static_cast<int>(CAIRO_PATH_CURVE_TO)
                     );

    mod->addConstant(type_uint32, "CAIRO_PATH_CLOSE_PATH",
                     static_cast<int>(CAIRO_PATH_CLOSE_PATH)
                     );

    mod->addConstant(type_uint32, "CAIRO_DEVICE_TYPE_DRM",
                     static_cast<int>(CAIRO_DEVICE_TYPE_DRM)
                     );

    mod->addConstant(type_uint32, "CAIRO_DEVICE_TYPE_GL",
                     static_cast<int>(CAIRO_DEVICE_TYPE_GL)
                     );

    mod->addConstant(type_uint32, "CAIRO_DEVICE_TYPE_SCRIPT",
                     static_cast<int>(CAIRO_DEVICE_TYPE_SCRIPT)
                     );

    mod->addConstant(type_uint32, "CAIRO_DEVICE_TYPE_XCB",
                     static_cast<int>(CAIRO_DEVICE_TYPE_XCB)
                     );

    mod->addConstant(type_uint32, "CAIRO_DEVICE_TYPE_XLIB",
                     static_cast<int>(CAIRO_DEVICE_TYPE_XLIB)
                     );

    mod->addConstant(type_uint32, "CAIRO_DEVICE_TYPE_XML",
                     static_cast<int>(CAIRO_DEVICE_TYPE_XML)
                     );

    mod->addConstant(type_uint32, "CAIRO_SURFACE_TYPE_IMAGE",
                     static_cast<int>(CAIRO_SURFACE_TYPE_IMAGE)
                     );

    mod->addConstant(type_uint32, "CAIRO_SURFACE_TYPE_PDF",
                     static_cast<int>(CAIRO_SURFACE_TYPE_PDF)
                     );

    mod->addConstant(type_uint32, "CAIRO_SURFACE_TYPE_PS",
                     static_cast<int>(CAIRO_SURFACE_TYPE_PS)
                     );

    mod->addConstant(type_uint32, "CAIRO_SURFACE_TYPE_XLIB",
                     static_cast<int>(CAIRO_SURFACE_TYPE_XLIB)
                     );

    mod->addConstant(type_uint32, "CAIRO_SURFACE_TYPE_XCB",
                     static_cast<int>(CAIRO_SURFACE_TYPE_XCB)
                     );

    mod->addConstant(type_uint32, "CAIRO_SURFACE_TYPE_GLITZ",
                     static_cast<int>(CAIRO_SURFACE_TYPE_GLITZ)
                     );

    mod->addConstant(type_uint32, "CAIRO_SURFACE_TYPE_QUARTZ",
                     static_cast<int>(CAIRO_SURFACE_TYPE_QUARTZ)
                     );

    mod->addConstant(type_uint32, "CAIRO_SURFACE_TYPE_WIN32",
                     static_cast<int>(CAIRO_SURFACE_TYPE_WIN32)
                     );

    mod->addConstant(type_uint32, "CAIRO_SURFACE_TYPE_BEOS",
                     static_cast<int>(CAIRO_SURFACE_TYPE_BEOS)
                     );

    mod->addConstant(type_uint32, "CAIRO_SURFACE_TYPE_DIRECTFB",
                     static_cast<int>(CAIRO_SURFACE_TYPE_DIRECTFB)
                     );

    mod->addConstant(type_uint32, "CAIRO_SURFACE_TYPE_SVG",
                     static_cast<int>(CAIRO_SURFACE_TYPE_SVG)
                     );

    mod->addConstant(type_uint32, "CAIRO_SURFACE_TYPE_OS2",
                     static_cast<int>(CAIRO_SURFACE_TYPE_OS2)
                     );

    mod->addConstant(type_uint32, "CAIRO_SURFACE_TYPE_WIN32_PRINTING",
                     static_cast<int>(CAIRO_SURFACE_TYPE_WIN32_PRINTING)
                     );

    mod->addConstant(type_uint32, "CAIRO_SURFACE_TYPE_QUARTZ_IMAGE",
                     static_cast<int>(CAIRO_SURFACE_TYPE_QUARTZ_IMAGE)
                     );

    mod->addConstant(type_uint32, "CAIRO_SURFACE_TYPE_SCRIPT",
                     static_cast<int>(CAIRO_SURFACE_TYPE_SCRIPT)
                     );

    mod->addConstant(type_uint32, "CAIRO_SURFACE_TYPE_QT",
                     static_cast<int>(CAIRO_SURFACE_TYPE_QT)
                     );

    mod->addConstant(type_uint32, "CAIRO_SURFACE_TYPE_RECORDING",
                     static_cast<int>(CAIRO_SURFACE_TYPE_RECORDING)
                     );

    mod->addConstant(type_uint32, "CAIRO_SURFACE_TYPE_VG",
                     static_cast<int>(CAIRO_SURFACE_TYPE_VG)
                     );

    mod->addConstant(type_uint32, "CAIRO_SURFACE_TYPE_GL",
                     static_cast<int>(CAIRO_SURFACE_TYPE_GL)
                     );

    mod->addConstant(type_uint32, "CAIRO_SURFACE_TYPE_DRM",
                     static_cast<int>(CAIRO_SURFACE_TYPE_DRM)
                     );

    mod->addConstant(type_uint32, "CAIRO_SURFACE_TYPE_TEE",
                     static_cast<int>(CAIRO_SURFACE_TYPE_TEE)
                     );

    mod->addConstant(type_uint32, "CAIRO_SURFACE_TYPE_XML",
                     static_cast<int>(CAIRO_SURFACE_TYPE_XML)
                     );

    mod->addConstant(type_uint32, "CAIRO_SURFACE_TYPE_SKIA",
                     static_cast<int>(CAIRO_SURFACE_TYPE_SKIA)
                     );

    mod->addConstant(type_uint32, "CAIRO_SURFACE_TYPE_SUBSURFACE",
                     static_cast<int>(CAIRO_SURFACE_TYPE_SUBSURFACE)
                     );

    mod->addConstant(type_uint32, "CAIRO_FORMAT_INVALID",
                     static_cast<int>(CAIRO_FORMAT_INVALID)
                     );

    mod->addConstant(type_uint32, "CAIRO_FORMAT_ARGB32",
                     static_cast<int>(CAIRO_FORMAT_ARGB32)
                     );

    mod->addConstant(type_uint32, "CAIRO_FORMAT_RGB24",
                     static_cast<int>(CAIRO_FORMAT_RGB24)
                     );

    mod->addConstant(type_uint32, "CAIRO_FORMAT_A8",
                     static_cast<int>(CAIRO_FORMAT_A8)
                     );

    mod->addConstant(type_uint32, "CAIRO_FORMAT_A1",
                     static_cast<int>(CAIRO_FORMAT_A1)
                     );

    mod->addConstant(type_uint32, "CAIRO_FORMAT_RGB16_565",
                     static_cast<int>(CAIRO_FORMAT_RGB16_565)
                     );

    mod->addConstant(type_uint32, "CAIRO_PATTERN_TYPE_SOLID",
                     static_cast<int>(CAIRO_PATTERN_TYPE_SOLID)
                     );

    mod->addConstant(type_uint32, "CAIRO_PATTERN_TYPE_SURFACE",
                     static_cast<int>(CAIRO_PATTERN_TYPE_SURFACE)
                     );

    mod->addConstant(type_uint32, "CAIRO_PATTERN_TYPE_LINEAR",
                     static_cast<int>(CAIRO_PATTERN_TYPE_LINEAR)
                     );

    mod->addConstant(type_uint32, "CAIRO_PATTERN_TYPE_RADIAL",
                     static_cast<int>(CAIRO_PATTERN_TYPE_RADIAL)
                     );

    mod->addConstant(type_uint32, "CAIRO_EXTEND_NONE",
                     static_cast<int>(CAIRO_EXTEND_NONE)
                     );

    mod->addConstant(type_uint32, "CAIRO_EXTEND_REPEAT",
                     static_cast<int>(CAIRO_EXTEND_REPEAT)
                     );

    mod->addConstant(type_uint32, "CAIRO_EXTEND_REFLECT",
                     static_cast<int>(CAIRO_EXTEND_REFLECT)
                     );

    mod->addConstant(type_uint32, "CAIRO_EXTEND_PAD",
                     static_cast<int>(CAIRO_EXTEND_PAD)
                     );

    mod->addConstant(type_uint32, "CAIRO_FILTER_FAST",
                     static_cast<int>(CAIRO_FILTER_FAST)
                     );

    mod->addConstant(type_uint32, "CAIRO_FILTER_GOOD",
                     static_cast<int>(CAIRO_FILTER_GOOD)
                     );

    mod->addConstant(type_uint32, "CAIRO_FILTER_BEST",
                     static_cast<int>(CAIRO_FILTER_BEST)
                     );

    mod->addConstant(type_uint32, "CAIRO_FILTER_NEAREST",
                     static_cast<int>(CAIRO_FILTER_NEAREST)
                     );

    mod->addConstant(type_uint32, "CAIRO_FILTER_BILINEAR",
                     static_cast<int>(CAIRO_FILTER_BILINEAR)
                     );

    mod->addConstant(type_uint32, "CAIRO_FILTER_GAUSSIAN",
                     static_cast<int>(CAIRO_FILTER_GAUSSIAN)
                     );

    mod->addConstant(type_uint32, "CAIRO_REGION_OVERLAP_IN",
                     static_cast<int>(CAIRO_REGION_OVERLAP_IN)
                     );

    mod->addConstant(type_uint32, "CAIRO_REGION_OVERLAP_OUT",
                     static_cast<int>(CAIRO_REGION_OVERLAP_OUT)
                     );

    mod->addConstant(type_uint32, "CAIRO_REGION_OVERLAP_PART",
                     static_cast<int>(CAIRO_REGION_OVERLAP_PART)
                     );
}
