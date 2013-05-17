#include <cairo/cairo.h>
#include <cairo/cairo-pdf.h>
#include <cairo/cairo-ps.h>
#include <cairo/cairo-svg.h>
#include <cairo/cairo-xlib.h>
#include <stdio.h>
#include <cairo/cairo-features.h>
#include <X11/Xlib.h>
#ifndef CAIRO_HAS_FC_FONT
# define CAIRO_HAS_FC_FONT 0
#endif

#ifndef CAIRO_HAS_FT_FONT
# define CAIRO_HAS_FT_FONT 0
#endif

#ifndef CAIRO_HAS_GOBJECT_FUNCTIONS
# define CAIRO_HAS_GOBJECT_FUNCTIONS 0
#endif

#ifndef CAIRO_HAS_IMAGE_SURFACE
# define CAIRO_HAS_IMAGE_SURFACE 0
#endif

#ifndef CAIRO_HAS_PDF_SURFACE
# define CAIRO_HAS_PDF_SURFACE 0
#endif

#ifndef CAIRO_HAS_PNG_FUNCTIONS
# define CAIRO_HAS_PNG_FUNCTIONS 0
#endif

#ifndef CAIRO_HAS_PS_SURFACE
# define CAIRO_HAS_PS_SURFACE 0
#endif

#ifndef CAIRO_HAS_RECORDING_SURFACE
# define CAIRO_HAS_RECORDING_SURFACE 0
#endif

#ifndef CAIRO_HAS_SVG_SURFACE
# define CAIRO_HAS_SVG_SURFACE 0
#endif

#ifndef CAIRO_HAS_USER_FONT
# define CAIRO_HAS_USER_FONT 0
#endif

#ifndef CAIRO_HAS_XCB_SHM_FUNCTIONS
# define CAIRO_HAS_XCB_SHM_FUNCTIONS 0
#endif

#ifndef CAIRO_HAS_XCB_SURFACE
# define CAIRO_HAS_XCB_SURFACE 0
#endif

#ifndef CAIRO_HAS_XLIB_SURFACE
# define CAIRO_HAS_XLIB_SURFACE 0
#endif

#ifndef CAIRO_HAS_XLIB_XRENDER_SURFACE
# define CAIRO_HAS_XLIB_XRENDER_SURFACE 0
#endif
typedef int Undef;
cairo_matrix_t *cairo_matrix_new() { return new cairo_matrix_t; }
cairo_rectangle_t *cairo_rectangle_new() { return new cairo_rectangle_t; }
cairo_rectangle_list_t *cairo_rectangle_list_new() { return new cairo_rectangle_list_t; }
cairo_text_extents_t *cairo_text_extents_new() { return new cairo_text_extents_t; }
cairo_font_extents_t *cairo_font_extents_new() { return new cairo_font_extents_t; }
cairo_surface_t *cairo_surface_new(cairo_surface_t *existing_surface) { 
      return existing_surface;
   }


#include "ext/Module.h"
#include "ext/Type.h"
#include "ext/Func.h"

extern "C"
void crack_ext__cairo_rinit() {
    return;
}

extern "C"
void crack_ext__cairo_cinit(crack::ext::Module *mod) {
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

    crack::ext::Type *type_cairo_t = mod->addType("cairo_t", sizeof(Undef));
    type_cairo_t->finish();


    crack::ext::Type *type_cairo_surface_t = mod->addType("cairo_surface_t", sizeof(Undef));
    type_cairo_surface_t->finish();


    crack::ext::Type *type_cairo_device_t = mod->addType("cairo_device_t", sizeof(Undef));
    type_cairo_device_t->finish();


    crack::ext::Type *type_cairo_matrix_t = mod->addType("cairo_matrix_t", sizeof(cairo_matrix_t));
        type_cairo_matrix_t->addInstVar(type_float64, "xx",
                                CRACK_OFFSET(cairo_matrix_t, xx));
        type_cairo_matrix_t->addInstVar(type_float64, "yx",
                                CRACK_OFFSET(cairo_matrix_t, yx));
        type_cairo_matrix_t->addInstVar(type_float64, "xy",
                                CRACK_OFFSET(cairo_matrix_t, xy));
        type_cairo_matrix_t->addInstVar(type_float64, "yy",
                                CRACK_OFFSET(cairo_matrix_t, yy));
        type_cairo_matrix_t->addInstVar(type_float64, "x0",
                                CRACK_OFFSET(cairo_matrix_t, x0));
        type_cairo_matrix_t->addInstVar(type_float64, "y0",
                                CRACK_OFFSET(cairo_matrix_t, y0));
    type_cairo_matrix_t->finish();


    crack::ext::Type *type_cairo_user_data_key_t = mod->addType("cairo_user_data_key_t", sizeof(cairo_user_data_key_t));
    type_cairo_user_data_key_t->finish();


    crack::ext::Type *type_cairo_pattern_t = mod->addType("cairo_pattern_t", sizeof(Undef));
    type_cairo_pattern_t->finish();


    crack::ext::Type *type_cairo_rectangle_t = mod->addType("cairo_rectangle_t", sizeof(cairo_rectangle_t));
        type_cairo_rectangle_t->addInstVar(type_float64, "x",
                                CRACK_OFFSET(cairo_rectangle_t, x));
        type_cairo_rectangle_t->addInstVar(type_float64, "y",
                                CRACK_OFFSET(cairo_rectangle_t, y));
        type_cairo_rectangle_t->addInstVar(type_float64, "width",
                                CRACK_OFFSET(cairo_rectangle_t, width));
        type_cairo_rectangle_t->addInstVar(type_float64, "height",
                                CRACK_OFFSET(cairo_rectangle_t, height));
    type_cairo_rectangle_t->finish();


    crack::ext::Type *array = mod->getType("array");

    crack::ext::Type *array_pcairo__rectangle__t_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_cairo_rectangle_t;
        array_pcairo__rectangle__t_q = array->getSpecialization(params);
    }

    crack::ext::Type *type_cairo_rectangle_list_t = mod->addType("cairo_rectangle_list_t", sizeof(cairo_rectangle_list_t));
        type_cairo_rectangle_list_t->addInstVar(type_uint, "status",
                                CRACK_OFFSET(cairo_rectangle_list_t, status));
        type_cairo_rectangle_list_t->addInstVar(array_pcairo__rectangle__t_q, "rectangles",
                                CRACK_OFFSET(cairo_rectangle_list_t, rectangles));
        type_cairo_rectangle_list_t->addInstVar(type_int, "num_rectangles",
                                CRACK_OFFSET(cairo_rectangle_list_t, num_rectangles));
    f->addArg(type_cairo_rectangle_list_t, 
              "rectangle_list"
              );

    type_cairo_rectangle_list_t->finish();


    crack::ext::Type *type_cairo_scaled_font_t = mod->addType("cairo_scaled_font_t", sizeof(Undef));
    type_cairo_scaled_font_t->finish();


    crack::ext::Type *type_cairo_font_face_t = mod->addType("cairo_font_face_t", sizeof(Undef));
    type_cairo_font_face_t->finish();


    crack::ext::Type *type_cairo_operator_t = mod->addType("cairo_operator_t", sizeof(uint));
    type_cairo_operator_t->finish();


    crack::ext::Type *type_cairo_glyph_t = mod->addType("cairo_glyph_t", sizeof(cairo_glyph_t));
    type_cairo_glyph_t->finish();


    crack::ext::Type *type_cairo_text_cluster_t = mod->addType("cairo_text_cluster_t", sizeof(cairo_text_cluster_t));
    type_cairo_text_cluster_t->finish();


    crack::ext::Type *type_cairo_text_extents_t = mod->addType("cairo_text_extents_t", sizeof(cairo_text_extents_t));
        type_cairo_text_extents_t->addInstVar(type_float64, "x_bearing",
                                CRACK_OFFSET(cairo_text_extents_t, x_bearing));
        type_cairo_text_extents_t->addInstVar(type_float64, "y_bearing",
                                CRACK_OFFSET(cairo_text_extents_t, y_bearing));
        type_cairo_text_extents_t->addInstVar(type_float64, "width",
                                CRACK_OFFSET(cairo_text_extents_t, width));
        type_cairo_text_extents_t->addInstVar(type_float64, "height",
                                CRACK_OFFSET(cairo_text_extents_t, height));
        type_cairo_text_extents_t->addInstVar(type_float64, "x_advance",
                                CRACK_OFFSET(cairo_text_extents_t, x_advance));
        type_cairo_text_extents_t->addInstVar(type_float64, "y_advance",
                                CRACK_OFFSET(cairo_text_extents_t, y_advance));
    type_cairo_text_extents_t->finish();


    crack::ext::Type *type_cairo_font_extents_t = mod->addType("cairo_font_extents_t", sizeof(cairo_font_extents_t));
        type_cairo_font_extents_t->addInstVar(type_float64, "ascent",
                                CRACK_OFFSET(cairo_font_extents_t, ascent));
        type_cairo_font_extents_t->addInstVar(type_float64, "descent",
                                CRACK_OFFSET(cairo_font_extents_t, descent));
        type_cairo_font_extents_t->addInstVar(type_float64, "height",
                                CRACK_OFFSET(cairo_font_extents_t, height));
        type_cairo_font_extents_t->addInstVar(type_float64, "max_x_advance",
                                CRACK_OFFSET(cairo_font_extents_t, max_x_advance));
        type_cairo_font_extents_t->addInstVar(type_float64, "max_y_advance",
                                CRACK_OFFSET(cairo_font_extents_t, max_y_advance));
    type_cairo_font_extents_t->finish();


    crack::ext::Type *type_cairo_font_options_t = mod->addType("cairo_font_options_t", sizeof(Undef));
    type_cairo_font_options_t->finish();


    crack::ext::Type *type_cairo_path_data_t = mod->addType("cairo_path_data_t", sizeof(cairo_path_data_t));
    type_cairo_path_data_t->finish();


    crack::ext::Type *type_cairo_path_t = mod->addType("cairo_path_t", sizeof(cairo_path_t));
    type_cairo_path_t->finish();


    crack::ext::Type *type_cairo_rectangle_int_t = mod->addType("cairo_rectangle_int_t", sizeof(cairo_rectangle_int_t));
    type_cairo_rectangle_int_t->finish();


    crack::ext::Type *type_cairo_region_t = mod->addType("cairo_region_t", sizeof(Undef));
    type_cairo_region_t->finish();


    crack::ext::Type *array_pfloat64_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_float64;
        array_pfloat64_q = array->getSpecialization(params);
    }

    crack::ext::Type *array_puint_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_uint;
        array_puint_q = array->getSpecialization(params);
    }

    crack::ext::Type *array_pcairo__glyph__t_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_cairo_glyph_t;
        array_pcairo__glyph__t_q = array->getSpecialization(params);
    }

    crack::ext::Type *function = mod->getType("function");

    crack::ext::Type *function_pvoid_c_svoidptr_q;
    {
        std::vector<crack::ext::Type *> params(2);
        params[0] = type_void;
        params[1] = type_voidptr;
        function_pvoid_c_svoidptr_q = function->getSpecialization(params);
    }

    crack::ext::Type *array_pcairo__text__cluster__t_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_cairo_text_cluster_t;
        array_pcairo__text__cluster__t_q = array->getSpecialization(params);
    }

#ifdef CAIRO_HAS_PNG_FUNCTIONS


    crack::ext::Type *function_puint_c_svoidptr_c_sbyteptr_c_suint_q;
    {
        std::vector<crack::ext::Type *> params(4);
        params[0] = type_uint;
        params[1] = type_voidptr;
        params[2] = type_byteptr;
        params[3] = type_uint;
        function_puint_c_svoidptr_c_sbyteptr_c_suint_q = function->getSpecialization(params);
    }
#endif // CAIRO_HAS_PNG_FUNCTIONS


    crack::ext::Type *array_pbyteptr_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_byteptr;
        array_pbyteptr_q = array->getSpecialization(params);
    }

    crack::ext::Type *array_puint64_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_uint64;
        array_puint64_q = array->getSpecialization(params);
    }

    crack::ext::Type *array_pint_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_int;
        array_pint_q = array->getSpecialization(params);
    }

    crack::ext::Type *type_Display = mod->addType("Display", sizeof(Undef));
    type_Display->finish();


    crack::ext::Type *type_Drawable = mod->addType("Drawable", sizeof(Drawable));
    type_Drawable->finish();


    crack::ext::Type *type_Visual = mod->addType("Visual", sizeof(Visual));
    type_Visual->finish();


    crack::ext::Type *type_Pixmap = mod->addType("Pixmap", sizeof(Pixmap));
    type_Pixmap->finish();


    crack::ext::Type *type_Screen = mod->addType("Screen", sizeof(Screen));
    type_Screen->finish();

    f = mod->addFunc(type_cairo_matrix_t, "cairo_matrix_new",
                     (void *)cairo_matrix_new
                     );

    f = mod->addFunc(type_cairo_rectangle_t, "cairo_rectangle_new",
                     (void *)cairo_rectangle_new
                     );

    f = mod->addFunc(type_cairo_rectangle_list_t, "cairo_rectangle_list_new",
                     (void *)cairo_rectangle_list_new
                     );

    f = mod->addFunc(type_cairo_text_extents_t, "cairo_text_extents_new",
                     (void *)cairo_text_extents_new
                     );

    f = mod->addFunc(type_cairo_font_extents_t, "cairo_font_extents_new",
                     (void *)cairo_font_extents_new
                     );

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
       f->addArg(type_uint, "op");

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
       f->addArg(type_uint, "antialias");

    f = mod->addFunc(type_void, "cairo_set_fill_rule",
                     (void *)cairo_set_fill_rule
                     );
       f->addArg(type_cairo_t, "cr");
       f->addArg(type_uint, "fill_rule");

    f = mod->addFunc(type_void, "cairo_set_line_width",
                     (void *)cairo_set_line_width
                     );
       f->addArg(type_cairo_t, "cr");
       f->addArg(type_float64, "width");

    f = mod->addFunc(type_void, "cairo_set_line_cap",
                     (void *)cairo_set_line_cap
                     );
       f->addArg(type_cairo_t, "cr");
       f->addArg(type_uint, "line_cap");

    f = mod->addFunc(type_void, "cairo_set_line_join",
                     (void *)cairo_set_line_join
                     );
       f->addArg(type_cairo_t, "cr");
       f->addArg(type_uint, "line_join");

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

    f = mod->addFunc(type_cairo_rectangle_list_t, "cairo_copy_clip_rectangle_list",
                     (void *)cairo_copy_clip_rectangle_list
                     );
       f->addArg(type_cairo_t, "cr");

    f = mod->addFunc(type_void, "cairo_rectangle_list_destroy",
                     (void *)cairo_rectangle_list_destroy
                     );
       f->addArg(type_cairo_rectangle_list_t, "rectangle_list");

    f = mod->addFunc(type_cairo_glyph_t, "cairo_glyph_allocate",
                     (void *)cairo_glyph_allocate
                     );
       f->addArg(type_int, "num_glyphs");

    f = mod->addFunc(type_void, "cairo_glyph_free",
                     (void *)cairo_glyph_free
                     );
       f->addArg(type_cairo_glyph_t, "glyphs");

    f = mod->addFunc(type_cairo_text_cluster_t, "cairo_text_cluster_allocate",
                     (void *)cairo_text_cluster_allocate
                     );
       f->addArg(type_int, "num_clusters");

    f = mod->addFunc(type_void, "cairo_text_cluster_free",
                     (void *)cairo_text_cluster_free
                     );
       f->addArg(type_cairo_text_cluster_t, "clusters");

    f = mod->addFunc(type_cairo_font_options_t, "cairo_font_options_create",
                     (void *)cairo_font_options_create
                     );

    f = mod->addFunc(type_cairo_font_options_t, "cairo_font_options_copy",
                     (void *)cairo_font_options_copy
                     );
       f->addArg(type_cairo_font_options_t, "original");

    f = mod->addFunc(type_void, "cairo_font_options_destroy",
                     (void *)cairo_font_options_destroy
                     );
       f->addArg(type_cairo_font_options_t, "options");

    f = mod->addFunc(type_uint, "cairo_font_options_status",
                     (void *)cairo_font_options_status
                     );
       f->addArg(type_cairo_font_options_t, "options");

    f = mod->addFunc(type_void, "cairo_font_options_merge",
                     (void *)cairo_font_options_merge
                     );
       f->addArg(type_cairo_font_options_t, "options");
       f->addArg(type_cairo_font_options_t, "other");

    f = mod->addFunc(type_bool, "cairo_font_options_equal",
                     (void *)cairo_font_options_equal
                     );
       f->addArg(type_cairo_font_options_t, "options");
       f->addArg(type_cairo_font_options_t, "other");

    f = mod->addFunc(type_uint64, "cairo_font_options_hash",
                     (void *)cairo_font_options_hash
                     );
       f->addArg(type_cairo_font_options_t, "options");

    f = mod->addFunc(type_void, "cairo_font_options_set_antialias",
                     (void *)cairo_font_options_set_antialias
                     );
       f->addArg(type_cairo_font_options_t, "options");
       f->addArg(type_uint, "antialias");

    f = mod->addFunc(type_uint, "cairo_font_options_get_antialias",
                     (void *)cairo_font_options_get_antialias
                     );
       f->addArg(type_cairo_font_options_t, "options");

    f = mod->addFunc(type_void, "cairo_font_options_set_subpixel_order",
                     (void *)cairo_font_options_set_subpixel_order
                     );
       f->addArg(type_cairo_font_options_t, "options");
       f->addArg(type_uint, "subpixel_order");

    f = mod->addFunc(type_uint, "cairo_font_options_get_subpixel_order",
                     (void *)cairo_font_options_get_subpixel_order
                     );
       f->addArg(type_cairo_font_options_t, "options");

    f = mod->addFunc(type_void, "cairo_font_options_set_hint_style",
                     (void *)cairo_font_options_set_hint_style
                     );
       f->addArg(type_cairo_font_options_t, "options");
       f->addArg(type_uint, "hint_style");

    f = mod->addFunc(type_uint, "cairo_font_options_get_hint_style",
                     (void *)cairo_font_options_get_hint_style
                     );
       f->addArg(type_cairo_font_options_t, "options");

    f = mod->addFunc(type_void, "cairo_font_options_set_hint_metrics",
                     (void *)cairo_font_options_set_hint_metrics
                     );
       f->addArg(type_cairo_font_options_t, "options");
       f->addArg(type_uint, "hint_metrics");

    f = mod->addFunc(type_uint, "cairo_font_options_get_hint_metrics",
                     (void *)cairo_font_options_get_hint_metrics
                     );
       f->addArg(type_cairo_font_options_t, "options");

    f = mod->addFunc(type_void, "cairo_select_font_face",
                     (void *)cairo_select_font_face
                     );
       f->addArg(type_cairo_t, "cr");
       f->addArg(type_byteptr, "family");
       f->addArg(type_uint, "slant");
       f->addArg(type_uint, "weight");

    f = mod->addFunc(type_void, "cairo_set_font_size",
                     (void *)cairo_set_font_size
                     );
       f->addArg(type_cairo_t, "cr");
       f->addArg(type_float64, "size");

    f = mod->addFunc(type_void, "cairo_set_font_matrix",
                     (void *)cairo_set_font_matrix
                     );
       f->addArg(type_cairo_t, "cr");
       f->addArg(type_cairo_matrix_t, "matrix");

    f = mod->addFunc(type_void, "cairo_get_font_matrix",
                     (void *)cairo_get_font_matrix
                     );
       f->addArg(type_cairo_t, "cr");
       f->addArg(type_cairo_matrix_t, "matrix");

    f = mod->addFunc(type_void, "cairo_set_font_options",
                     (void *)cairo_set_font_options
                     );
       f->addArg(type_cairo_t, "cr");
       f->addArg(type_cairo_font_options_t, "options");

    f = mod->addFunc(type_void, "cairo_get_font_options",
                     (void *)cairo_get_font_options
                     );
       f->addArg(type_cairo_t, "cr");
       f->addArg(type_cairo_font_options_t, "options");

    f = mod->addFunc(type_void, "cairo_set_font_face",
                     (void *)cairo_set_font_face
                     );
       f->addArg(type_cairo_t, "cr");
       f->addArg(type_cairo_font_face_t, "font_face");

    f = mod->addFunc(type_cairo_font_face_t, "cairo_get_font_face",
                     (void *)cairo_get_font_face
                     );
       f->addArg(type_cairo_t, "cr");

    f = mod->addFunc(type_void, "cairo_set_scaled_font",
                     (void *)cairo_set_scaled_font
                     );
       f->addArg(type_cairo_t, "cr");
       f->addArg(type_cairo_scaled_font_t, "scaled_font");

    f = mod->addFunc(type_cairo_scaled_font_t, "cairo_get_scaled_font",
                     (void *)cairo_get_scaled_font
                     );
       f->addArg(type_cairo_t, "cr");

    f = mod->addFunc(type_void, "cairo_show_text",
                     (void *)cairo_show_text
                     );
       f->addArg(type_cairo_t, "cr");
       f->addArg(type_byteptr, "utf8");

    f = mod->addFunc(type_void, "cairo_show_glyphs",
                     (void *)cairo_show_glyphs
                     );
       f->addArg(type_cairo_t, "cr");
       f->addArg(type_cairo_glyph_t, "glyphs");
       f->addArg(type_int, "num_glyphs");

    f = mod->addFunc(type_void, "cairo_show_text_glyphs",
                     (void *)cairo_show_text_glyphs
                     );
       f->addArg(type_cairo_t, "cr");
       f->addArg(type_byteptr, "utf8");
       f->addArg(type_int, "utf8_len");
       f->addArg(type_cairo_glyph_t, "glyphs");
       f->addArg(type_int, "num_glyphs");
       f->addArg(type_cairo_text_cluster_t, "clusters");
       f->addArg(type_int, "num_clusters");
       f->addArg(array_puint_q, "cluster_flags");

    f = mod->addFunc(type_void, "cairo_text_path",
                     (void *)cairo_text_path
                     );
       f->addArg(type_cairo_t, "cr");
       f->addArg(type_byteptr, "utf8");

    f = mod->addFunc(type_void, "cairo_glyph_path",
                     (void *)cairo_glyph_path
                     );
       f->addArg(type_cairo_t, "cr");
       f->addArg(type_cairo_glyph_t, "glyphs");
       f->addArg(type_int, "num_glyphs");

    f = mod->addFunc(type_void, "cairo_text_extents",
                     (void *)cairo_text_extents
                     );
       f->addArg(type_cairo_t, "cr");
       f->addArg(type_byteptr, "utf8");
       f->addArg(type_cairo_text_extents_t, "extents");

    f = mod->addFunc(type_void, "cairo_glyph_extents",
                     (void *)cairo_glyph_extents
                     );
       f->addArg(type_cairo_t, "cr");
       f->addArg(array_pcairo__glyph__t_q, "glyphs");
       f->addArg(type_int, "num_glyphs");
       f->addArg(type_cairo_text_extents_t, "extents");

    f = mod->addFunc(type_void, "cairo_font_extents",
                     (void *)cairo_font_extents
                     );
       f->addArg(type_cairo_t, "cr");
       f->addArg(type_cairo_font_extents_t, "extents");

    f = mod->addFunc(type_cairo_font_face_t, "cairo_font_face_reference",
                     (void *)cairo_font_face_reference
                     );
       f->addArg(type_cairo_font_face_t, "font_face");

    f = mod->addFunc(type_void, "cairo_font_face_destroy",
                     (void *)cairo_font_face_destroy
                     );
       f->addArg(type_cairo_font_face_t, "font_face");

    f = mod->addFunc(type_uint, "cairo_font_face_get_reference_count",
                     (void *)cairo_font_face_get_reference_count
                     );
       f->addArg(type_cairo_font_face_t, "font_face");

    f = mod->addFunc(type_uint, "cairo_font_face_status",
                     (void *)cairo_font_face_status
                     );
       f->addArg(type_cairo_font_face_t, "font_face");

    f = mod->addFunc(type_uint, "cairo_font_face_get_type",
                     (void *)cairo_font_face_get_type
                     );
       f->addArg(type_cairo_font_face_t, "font_face");

    f = mod->addFunc(type_void, "cairo_font_face_get_user_data",
                     (void *)cairo_font_face_get_user_data
                     );
       f->addArg(type_cairo_font_face_t, "font_face");
       f->addArg(type_cairo_user_data_key_t, "key");

    f = mod->addFunc(type_uint, "cairo_font_face_set_user_data",
                     (void *)cairo_font_face_set_user_data
                     );
       f->addArg(type_cairo_font_face_t, "font_face");
       f->addArg(type_cairo_user_data_key_t, "key");
       f->addArg(type_voidptr, "user_data");
       f->addArg(function_pvoid_c_svoidptr_q, "destroy");

    f = mod->addFunc(type_cairo_scaled_font_t, "cairo_scaled_font_create",
                     (void *)cairo_scaled_font_create
                     );
       f->addArg(type_cairo_font_face_t, "font_face");
       f->addArg(type_cairo_matrix_t, "font_matrix");
       f->addArg(type_cairo_matrix_t, "ctm");
       f->addArg(type_cairo_font_options_t, "options");

    f = mod->addFunc(type_cairo_scaled_font_t, "cairo_scaled_font_reference",
                     (void *)cairo_scaled_font_reference
                     );
       f->addArg(type_cairo_scaled_font_t, "scaled_font");

    f = mod->addFunc(type_void, "cairo_scaled_font_destroy",
                     (void *)cairo_scaled_font_destroy
                     );
       f->addArg(type_cairo_scaled_font_t, "scaled_font");

    f = mod->addFunc(type_uint, "cairo_scaled_font_get_reference_count",
                     (void *)cairo_scaled_font_get_reference_count
                     );
       f->addArg(type_cairo_scaled_font_t, "scaled_font");

    f = mod->addFunc(type_uint, "cairo_scaled_font_status",
                     (void *)cairo_scaled_font_status
                     );
       f->addArg(type_cairo_scaled_font_t, "scaled_font");

    f = mod->addFunc(type_uint, "cairo_scaled_font_get_type",
                     (void *)cairo_scaled_font_get_type
                     );
       f->addArg(type_cairo_scaled_font_t, "scaled_font");

    f = mod->addFunc(type_void, "cairo_scaled_font_get_user_data",
                     (void *)cairo_scaled_font_get_user_data
                     );
       f->addArg(type_cairo_scaled_font_t, "scaled_font");
       f->addArg(type_cairo_user_data_key_t, "key");

    f = mod->addFunc(type_uint, "cairo_scaled_font_set_user_data",
                     (void *)cairo_scaled_font_set_user_data
                     );
       f->addArg(type_cairo_scaled_font_t, "scaled_font");
       f->addArg(type_cairo_user_data_key_t, "key");
       f->addArg(type_voidptr, "user_data");
       f->addArg(function_pvoid_c_svoidptr_q, "destroy");

    f = mod->addFunc(type_void, "cairo_scaled_font_extents",
                     (void *)cairo_scaled_font_extents
                     );
       f->addArg(type_cairo_scaled_font_t, "scaled_font");
       f->addArg(type_cairo_font_extents_t, "extents");

    f = mod->addFunc(type_void, "cairo_scaled_font_text_extents",
                     (void *)cairo_scaled_font_text_extents
                     );
       f->addArg(type_cairo_scaled_font_t, "scaled_font");
       f->addArg(type_byteptr, "utf8");
       f->addArg(type_cairo_text_extents_t, "extents");

    f = mod->addFunc(type_void, "cairo_scaled_font_glyph_extents",
                     (void *)cairo_scaled_font_glyph_extents
                     );
       f->addArg(type_cairo_scaled_font_t, "scaled_font");
       f->addArg(array_pcairo__glyph__t_q, "glyphs");
       f->addArg(type_int, "num_glyphs");
       f->addArg(type_cairo_text_extents_t, "extents");

    f = mod->addFunc(type_uint, "cairo_scaled_font_text_to_glyphs",
                     (void *)cairo_scaled_font_text_to_glyphs
                     );
       f->addArg(type_cairo_scaled_font_t, "scaled_font");
       f->addArg(type_float64, "x");
       f->addArg(type_float64, "y");
       f->addArg(type_byteptr, "utf8");
       f->addArg(type_int, "utf8_len");
       f->addArg(array_pcairo__glyph__t_q, "glyphs");
       f->addArg(type_int, "num_glyphs");
       f->addArg(array_pcairo__text__cluster__t_q, "clusters");
       f->addArg(type_int, "num_clusters");
       f->addArg(array_puint_q, "cluster_flags");

    f = mod->addFunc(type_cairo_font_face_t, "cairo_scaled_font_get_font_face",
                     (void *)cairo_scaled_font_get_font_face
                     );
       f->addArg(type_cairo_scaled_font_t, "scaled_font");

    f = mod->addFunc(type_void, "cairo_scaled_font_get_font_matrix",
                     (void *)cairo_scaled_font_get_font_matrix
                     );
       f->addArg(type_cairo_scaled_font_t, "scaled_font");
       f->addArg(type_cairo_matrix_t, "font_matrix");

    f = mod->addFunc(type_void, "cairo_scaled_font_get_ctm",
                     (void *)cairo_scaled_font_get_ctm
                     );
       f->addArg(type_cairo_scaled_font_t, "scaled_font");
       f->addArg(type_cairo_matrix_t, "ctm");

    f = mod->addFunc(type_void, "cairo_scaled_font_get_scale_matrix",
                     (void *)cairo_scaled_font_get_scale_matrix
                     );
       f->addArg(type_cairo_scaled_font_t, "scaled_font");
       f->addArg(type_cairo_matrix_t, "scale_matrix");

    f = mod->addFunc(type_void, "cairo_scaled_font_get_font_options",
                     (void *)cairo_scaled_font_get_font_options
                     );
       f->addArg(type_cairo_scaled_font_t, "scaled_font");
       f->addArg(type_cairo_font_options_t, "options");

    f = mod->addFunc(type_cairo_font_face_t, "cairo_toy_font_face_create",
                     (void *)cairo_toy_font_face_create
                     );
       f->addArg(type_byteptr, "family");
       f->addArg(type_uint, "slant");
       f->addArg(type_uint, "weight");

    f = mod->addFunc(type_byteptr, "cairo_toy_font_face_get_family",
                     (void *)cairo_toy_font_face_get_family
                     );
       f->addArg(type_cairo_font_face_t, "font_face");

    f = mod->addFunc(type_uint, "cairo_toy_font_face_get_slant",
                     (void *)cairo_toy_font_face_get_slant
                     );
       f->addArg(type_cairo_font_face_t, "font_face");

    f = mod->addFunc(type_uint, "cairo_toy_font_face_get_weight",
                     (void *)cairo_toy_font_face_get_weight
                     );
       f->addArg(type_cairo_font_face_t, "font_face");

    f = mod->addFunc(type_cairo_font_face_t, "cairo_user_font_face_create",
                     (void *)cairo_user_font_face_create
                     );

    f = mod->addFunc(type_uint, "cairo_get_operator",
                     (void *)cairo_get_operator
                     );
       f->addArg(type_cairo_t, "cr");

    f = mod->addFunc(type_cairo_pattern_t, "cairo_get_source",
                     (void *)cairo_get_source
                     );
       f->addArg(type_cairo_t, "cr");

    f = mod->addFunc(type_float64, "cairo_get_tolerance",
                     (void *)cairo_get_tolerance
                     );
       f->addArg(type_cairo_t, "cr");

    f = mod->addFunc(type_uint, "cairo_get_antialias",
                     (void *)cairo_get_antialias
                     );
       f->addArg(type_cairo_t, "cr");

    f = mod->addFunc(type_bool, "cairo_has_current_point",
                     (void *)cairo_has_current_point
                     );
       f->addArg(type_cairo_t, "cr");

    f = mod->addFunc(type_void, "cairo_get_current_point",
                     (void *)cairo_get_current_point
                     );
       f->addArg(type_cairo_t, "cr");
       f->addArg(array_pfloat64_q, "x");
       f->addArg(array_pfloat64_q, "y");

    f = mod->addFunc(type_uint, "cairo_get_fill_rule",
                     (void *)cairo_get_fill_rule
                     );
       f->addArg(type_cairo_t, "cr");

    f = mod->addFunc(type_float64, "cairo_get_line_width",
                     (void *)cairo_get_line_width
                     );
       f->addArg(type_cairo_t, "cr");

    f = mod->addFunc(type_uint, "cairo_get_line_cap",
                     (void *)cairo_get_line_cap
                     );
       f->addArg(type_cairo_t, "cr");

    f = mod->addFunc(type_uint, "cairo_get_line_join",
                     (void *)cairo_get_line_join
                     );
       f->addArg(type_cairo_t, "cr");

    f = mod->addFunc(type_float64, "cairo_get_miter_limit",
                     (void *)cairo_get_miter_limit
                     );
       f->addArg(type_cairo_t, "cr");

    f = mod->addFunc(type_int, "cairo_get_dash_count",
                     (void *)cairo_get_dash_count
                     );
       f->addArg(type_cairo_t, "cr");

    f = mod->addFunc(type_void, "cairo_get_dash",
                     (void *)cairo_get_dash
                     );
       f->addArg(type_cairo_t, "cr");
       f->addArg(array_pfloat64_q, "dashes");
       f->addArg(array_pfloat64_q, "offset");

    f = mod->addFunc(type_void, "cairo_get_matrix",
                     (void *)cairo_get_matrix
                     );
       f->addArg(type_cairo_t, "cr");
       f->addArg(type_cairo_matrix_t, "matrix");

    f = mod->addFunc(type_cairo_surface_t, "cairo_get_target",
                     (void *)cairo_get_target
                     );
       f->addArg(type_cairo_t, "cr");

    f = mod->addFunc(type_cairo_surface_t, "cairo_get_group_target",
                     (void *)cairo_get_group_target
                     );
       f->addArg(type_cairo_t, "cr");

    f = mod->addFunc(type_cairo_path_t, "cairo_copy_path",
                     (void *)cairo_copy_path
                     );
       f->addArg(type_cairo_t, "cr");

    f = mod->addFunc(type_cairo_path_t, "cairo_copy_path_flat",
                     (void *)cairo_copy_path_flat
                     );
       f->addArg(type_cairo_t, "cr");

    f = mod->addFunc(type_void, "cairo_append_path",
                     (void *)cairo_append_path
                     );
       f->addArg(type_cairo_t, "cr");
       f->addArg(type_cairo_path_t, "path");

    f = mod->addFunc(type_void, "cairo_path_destroy",
                     (void *)cairo_path_destroy
                     );
       f->addArg(type_cairo_path_t, "path");

    f = mod->addFunc(type_uint, "cairo_status",
                     (void *)cairo_status
                     );
       f->addArg(type_cairo_t, "cr");

    f = mod->addFunc(type_byteptr, "cairo_status_to_string",
                     (void *)cairo_status_to_string
                     );
       f->addArg(type_uint, "status");

    f = mod->addFunc(type_cairo_device_t, "cairo_device_reference",
                     (void *)cairo_device_reference
                     );
       f->addArg(type_cairo_device_t, "device");

    f = mod->addFunc(type_uint, "cairo_device_get_type",
                     (void *)cairo_device_get_type
                     );
       f->addArg(type_cairo_device_t, "device");

    f = mod->addFunc(type_uint, "cairo_device_status",
                     (void *)cairo_device_status
                     );
       f->addArg(type_cairo_device_t, "device");

    f = mod->addFunc(type_uint, "cairo_device_acquire",
                     (void *)cairo_device_acquire
                     );
       f->addArg(type_cairo_device_t, "device");

    f = mod->addFunc(type_void, "cairo_device_release",
                     (void *)cairo_device_release
                     );
       f->addArg(type_cairo_device_t, "device");

    f = mod->addFunc(type_void, "cairo_device_flush",
                     (void *)cairo_device_flush
                     );
       f->addArg(type_cairo_device_t, "device");

    f = mod->addFunc(type_void, "cairo_device_finish",
                     (void *)cairo_device_finish
                     );
       f->addArg(type_cairo_device_t, "device");

    f = mod->addFunc(type_void, "cairo_device_destroy",
                     (void *)cairo_device_destroy
                     );
       f->addArg(type_cairo_device_t, "device");

    f = mod->addFunc(type_uint, "cairo_device_get_reference_count",
                     (void *)cairo_device_get_reference_count
                     );
       f->addArg(type_cairo_device_t, "device");

    f = mod->addFunc(type_voidptr, "cairo_device_get_user_data",
                     (void *)cairo_device_get_user_data
                     );
       f->addArg(type_cairo_device_t, "device");
       f->addArg(type_cairo_user_data_key_t, "key");

    f = mod->addFunc(type_uint, "cairo_device_set_user_data",
                     (void *)cairo_device_set_user_data
                     );
       f->addArg(type_cairo_device_t, "device");
       f->addArg(type_cairo_user_data_key_t, "key");
       f->addArg(type_voidptr, "user_data");
       f->addArg(function_pvoid_c_svoidptr_q, "destroy");

    f = mod->addFunc(type_cairo_surface_t, "cairo_surface_create_similar",
                     (void *)cairo_surface_create_similar
                     );
       f->addArg(type_cairo_surface_t, "other");
       f->addArg(type_uint, "content");
       f->addArg(type_int, "width");
       f->addArg(type_int, "height");

    f = mod->addFunc(type_cairo_surface_t, "cairo_surface_create_for_rectangle",
                     (void *)cairo_surface_create_for_rectangle
                     );
       f->addArg(type_cairo_surface_t, "target");
       f->addArg(type_float64, "x");
       f->addArg(type_float64, "y");
       f->addArg(type_float64, "width");
       f->addArg(type_float64, "height");

    f = mod->addFunc(type_cairo_surface_t, "cairo_surface_reference",
                     (void *)cairo_surface_reference
                     );
       f->addArg(type_cairo_surface_t, "surface");

    f = mod->addFunc(type_void, "cairo_surface_finish",
                     (void *)cairo_surface_finish
                     );
       f->addArg(type_cairo_surface_t, "surface");

    f = mod->addFunc(type_void, "cairo_surface_destroy",
                     (void *)cairo_surface_destroy
                     );
       f->addArg(type_cairo_surface_t, "surface");

    f = mod->addFunc(type_cairo_device_t, "cairo_surface_get_device",
                     (void *)cairo_surface_get_device
                     );
       f->addArg(type_cairo_surface_t, "surface");

    f = mod->addFunc(type_uint, "cairo_surface_get_reference_count",
                     (void *)cairo_surface_get_reference_count
                     );
       f->addArg(type_cairo_surface_t, "surface");

    f = mod->addFunc(type_uint, "cairo_surface_status",
                     (void *)cairo_surface_status
                     );
       f->addArg(type_cairo_surface_t, "surface");

    f = mod->addFunc(type_uint, "cairo_surface_get_type",
                     (void *)cairo_surface_get_type
                     );
       f->addArg(type_cairo_surface_t, "surface");

    f = mod->addFunc(type_uint, "cairo_surface_get_content",
                     (void *)cairo_surface_get_content
                     );
       f->addArg(type_cairo_surface_t, "surface");


#ifdef CAIRO_HAS_PNG_FUNCTIONS

    f = mod->addFunc(type_uint, "cairo_surface_write_to_png",
                     (void *)cairo_surface_write_to_png
                     );
       f->addArg(type_cairo_surface_t, "surface");
       f->addArg(type_byteptr, "filename");

    f = mod->addFunc(type_uint, "cairo_surface_write_to_png_stream",
                     (void *)cairo_surface_write_to_png_stream
                     );
       f->addArg(type_cairo_surface_t, "surface");
       f->addArg(function_puint_c_svoidptr_c_sbyteptr_c_suint_q, "write_func");
       f->addArg(type_voidptr, "closure");

#endif // CAIRO_HAS_PNG_FUNCTIONS

    f = mod->addFunc(type_voidptr, "cairo_surface_get_user_data",
                     (void *)cairo_surface_get_user_data
                     );
       f->addArg(type_cairo_surface_t, "surface");
       f->addArg(type_cairo_user_data_key_t, "key");

    f = mod->addFunc(type_uint, "cairo_surface_set_user_data",
                     (void *)cairo_surface_set_user_data
                     );
       f->addArg(type_cairo_surface_t, "surface");
       f->addArg(type_cairo_user_data_key_t, "key");
       f->addArg(type_voidptr, "user_data");
       f->addArg(function_pvoid_c_svoidptr_q, "destroy");

    f = mod->addFunc(type_void, "cairo_surface_get_mime_data",
                     (void *)cairo_surface_get_mime_data
                     );
       f->addArg(type_cairo_surface_t, "surface");
       f->addArg(type_byteptr, "mime_type");
       f->addArg(array_pbyteptr_q, "data");
       f->addArg(array_puint64_q, "length");

    f = mod->addFunc(type_uint, "cairo_surface_set_mime_data",
                     (void *)cairo_surface_set_mime_data
                     );
       f->addArg(type_cairo_surface_t, "surface");
       f->addArg(type_byteptr, "mime_type");
       f->addArg(type_byteptr, "data");
       f->addArg(type_uint64, "length");
       f->addArg(function_pvoid_c_svoidptr_q, "destroy");
       f->addArg(type_voidptr, "closure");

    f = mod->addFunc(type_void, "cairo_surface_get_font_options",
                     (void *)cairo_surface_get_font_options
                     );
       f->addArg(type_cairo_surface_t, "surface");
       f->addArg(type_cairo_font_options_t, "options");

    f = mod->addFunc(type_void, "cairo_surface_flush",
                     (void *)cairo_surface_flush
                     );
       f->addArg(type_cairo_surface_t, "surface");

    f = mod->addFunc(type_void, "cairo_surface_mark_dirty",
                     (void *)cairo_surface_mark_dirty
                     );
       f->addArg(type_cairo_surface_t, "surface");

    f = mod->addFunc(type_void, "cairo_surface_mark_dirty_rectangle",
                     (void *)cairo_surface_mark_dirty_rectangle
                     );
       f->addArg(type_cairo_surface_t, "surface");
       f->addArg(type_int, "x");
       f->addArg(type_int, "y");
       f->addArg(type_int, "width");
       f->addArg(type_int, "height");

    f = mod->addFunc(type_void, "cairo_surface_set_device_offset",
                     (void *)cairo_surface_set_device_offset
                     );
       f->addArg(type_cairo_surface_t, "surface");
       f->addArg(type_float64, "x_offset");
       f->addArg(type_float64, "y_offset");

    f = mod->addFunc(type_void, "cairo_surface_get_device_offset",
                     (void *)cairo_surface_get_device_offset
                     );
       f->addArg(type_cairo_surface_t, "surface");
       f->addArg(array_pfloat64_q, "x_offset");
       f->addArg(array_pfloat64_q, "y_offset");

    f = mod->addFunc(type_void, "cairo_surface_set_fallback_resolution",
                     (void *)cairo_surface_set_fallback_resolution
                     );
       f->addArg(type_cairo_surface_t, "surface");
       f->addArg(type_float64, "x_pixels_per_inch");
       f->addArg(type_float64, "y_pixels_per_inch");

    f = mod->addFunc(type_void, "cairo_surface_get_fallback_resolution",
                     (void *)cairo_surface_get_fallback_resolution
                     );
       f->addArg(type_cairo_surface_t, "surface");
       f->addArg(array_pfloat64_q, "x_pixels_per_inch");
       f->addArg(array_pfloat64_q, "y_pixels_per_inch");

    f = mod->addFunc(type_void, "cairo_surface_copy_page",
                     (void *)cairo_surface_copy_page
                     );
       f->addArg(type_cairo_surface_t, "surface");

    f = mod->addFunc(type_void, "cairo_surface_show_page",
                     (void *)cairo_surface_show_page
                     );
       f->addArg(type_cairo_surface_t, "surface");

    f = mod->addFunc(type_bool, "cairo_surface_has_show_text_glyphs",
                     (void *)cairo_surface_has_show_text_glyphs
                     );
       f->addArg(type_cairo_surface_t, "surface");

    f = mod->addFunc(type_cairo_surface_t, "cairo_image_surface_create",
                     (void *)cairo_image_surface_create
                     );
       f->addArg(type_uint, "format");
       f->addArg(type_int, "width");
       f->addArg(type_int, "height");

    f = mod->addFunc(type_int, "cairo_format_stride_for_width",
                     (void *)cairo_format_stride_for_width
                     );
       f->addArg(type_uint, "format");
       f->addArg(type_int, "width");

    f = mod->addFunc(type_cairo_surface_t, "cairo_image_surface_create_for_data",
                     (void *)cairo_image_surface_create_for_data
                     );
       f->addArg(type_byteptr, "data");
       f->addArg(type_uint, "format");
       f->addArg(type_int, "width");
       f->addArg(type_int, "height");
       f->addArg(type_int, "stride");

    f = mod->addFunc(type_byteptr, "cairo_image_surface_get_data",
                     (void *)cairo_image_surface_get_data
                     );
       f->addArg(type_cairo_surface_t, "surface");

    f = mod->addFunc(type_uint, "cairo_image_surface_get_format",
                     (void *)cairo_image_surface_get_format
                     );
       f->addArg(type_cairo_surface_t, "surface");

    f = mod->addFunc(type_int, "cairo_image_surface_get_width",
                     (void *)cairo_image_surface_get_width
                     );
       f->addArg(type_cairo_surface_t, "surface");

    f = mod->addFunc(type_int, "cairo_image_surface_get_height",
                     (void *)cairo_image_surface_get_height
                     );
       f->addArg(type_cairo_surface_t, "surface");

    f = mod->addFunc(type_int, "cairo_image_surface_get_stride",
                     (void *)cairo_image_surface_get_stride
                     );
       f->addArg(type_cairo_surface_t, "surface");


#ifdef CAIRO_HAS_PNG_FUNCTIONS

    f = mod->addFunc(type_cairo_surface_t, "cairo_image_surface_create_from_png",
                     (void *)cairo_image_surface_create_from_png
                     );
       f->addArg(type_byteptr, "filename");

#endif // CAIRO_HAS_PNG_FUNCTIONS

    f = mod->addFunc(type_cairo_surface_t, "cairo_recording_surface_create",
                     (void *)cairo_recording_surface_create
                     );
       f->addArg(type_uint, "content");
       f->addArg(type_cairo_rectangle_t, "extents");

    f = mod->addFunc(type_void, "cairo_recording_surface_ink_extents",
                     (void *)cairo_recording_surface_ink_extents
                     );
       f->addArg(type_cairo_surface_t, "surface");
       f->addArg(array_pfloat64_q, "x0");
       f->addArg(array_pfloat64_q, "y0");
       f->addArg(array_pfloat64_q, "width");
       f->addArg(array_pfloat64_q, "height");

    f = mod->addFunc(type_cairo_pattern_t, "cairo_pattern_create_rgb",
                     (void *)cairo_pattern_create_rgb
                     );
       f->addArg(type_float64, "red");
       f->addArg(type_float64, "green");
       f->addArg(type_float64, "blue");

    f = mod->addFunc(type_cairo_pattern_t, "cairo_pattern_create_rgba",
                     (void *)cairo_pattern_create_rgba
                     );
       f->addArg(type_float64, "red");
       f->addArg(type_float64, "green");
       f->addArg(type_float64, "blue");
       f->addArg(type_float64, "alpha");

    f = mod->addFunc(type_cairo_pattern_t, "cairo_pattern_create_for_surface",
                     (void *)cairo_pattern_create_for_surface
                     );
       f->addArg(type_cairo_surface_t, "surface");

    f = mod->addFunc(type_cairo_pattern_t, "cairo_pattern_create_linear",
                     (void *)cairo_pattern_create_linear
                     );
       f->addArg(type_float64, "x0");
       f->addArg(type_float64, "y0");
       f->addArg(type_float64, "x1");
       f->addArg(type_float64, "y1");

    f = mod->addFunc(type_cairo_pattern_t, "cairo_pattern_create_radial",
                     (void *)cairo_pattern_create_radial
                     );
       f->addArg(type_float64, "cx0");
       f->addArg(type_float64, "cy0");
       f->addArg(type_float64, "radius0");
       f->addArg(type_float64, "cx1");
       f->addArg(type_float64, "cy1");
       f->addArg(type_float64, "radius1");

    f = mod->addFunc(type_cairo_pattern_t, "cairo_pattern_reference",
                     (void *)cairo_pattern_reference
                     );
       f->addArg(type_cairo_pattern_t, "pattern");

    f = mod->addFunc(type_void, "cairo_pattern_destroy",
                     (void *)cairo_pattern_destroy
                     );
       f->addArg(type_cairo_pattern_t, "pattern");

    f = mod->addFunc(type_uint, "cairo_pattern_get_reference_count",
                     (void *)cairo_pattern_get_reference_count
                     );
       f->addArg(type_cairo_pattern_t, "pattern");

    f = mod->addFunc(type_uint, "cairo_pattern_status",
                     (void *)cairo_pattern_status
                     );
       f->addArg(type_cairo_pattern_t, "pattern");

    f = mod->addFunc(type_voidptr, "cairo_pattern_get_user_data",
                     (void *)cairo_pattern_get_user_data
                     );
       f->addArg(type_cairo_pattern_t, "pattern");
       f->addArg(type_cairo_user_data_key_t, "key");

    f = mod->addFunc(type_uint, "cairo_pattern_set_user_data",
                     (void *)cairo_pattern_set_user_data
                     );
       f->addArg(type_cairo_pattern_t, "pattern");
       f->addArg(type_cairo_user_data_key_t, "key");
       f->addArg(type_voidptr, "user_data");
       f->addArg(function_pvoid_c_svoidptr_q, "destroy");

    f = mod->addFunc(type_uint, "cairo_pattern_get_type",
                     (void *)cairo_pattern_get_type
                     );
       f->addArg(type_cairo_pattern_t, "pattern");

    f = mod->addFunc(type_void, "cairo_pattern_add_color_stop_rgb",
                     (void *)cairo_pattern_add_color_stop_rgb
                     );
       f->addArg(type_cairo_pattern_t, "pattern");
       f->addArg(type_float64, "offset");
       f->addArg(type_float64, "red");
       f->addArg(type_float64, "green");
       f->addArg(type_float64, "blue");

    f = mod->addFunc(type_void, "cairo_pattern_add_color_stop_rgba",
                     (void *)cairo_pattern_add_color_stop_rgba
                     );
       f->addArg(type_cairo_pattern_t, "pattern");
       f->addArg(type_float64, "offset");
       f->addArg(type_float64, "red");
       f->addArg(type_float64, "green");
       f->addArg(type_float64, "blue");
       f->addArg(type_float64, "alpha");

    f = mod->addFunc(type_void, "cairo_pattern_set_matrix",
                     (void *)cairo_pattern_set_matrix
                     );
       f->addArg(type_cairo_pattern_t, "pattern");
       f->addArg(type_cairo_matrix_t, "matrix");

    f = mod->addFunc(type_void, "cairo_pattern_get_matrix",
                     (void *)cairo_pattern_get_matrix
                     );
       f->addArg(type_cairo_pattern_t, "pattern");
       f->addArg(type_cairo_matrix_t, "matrix");

    f = mod->addFunc(type_void, "cairo_pattern_set_extend",
                     (void *)cairo_pattern_set_extend
                     );
       f->addArg(type_cairo_pattern_t, "pattern");
       f->addArg(type_uint, "extend");

    f = mod->addFunc(type_uint, "cairo_pattern_get_extend",
                     (void *)cairo_pattern_get_extend
                     );
       f->addArg(type_cairo_pattern_t, "pattern");

    f = mod->addFunc(type_void, "cairo_pattern_set_filter",
                     (void *)cairo_pattern_set_filter
                     );
       f->addArg(type_cairo_pattern_t, "pattern");
       f->addArg(type_uint, "filter");

    f = mod->addFunc(type_uint, "cairo_pattern_get_filter",
                     (void *)cairo_pattern_get_filter
                     );
       f->addArg(type_cairo_pattern_t, "pattern");

    f = mod->addFunc(type_uint, "cairo_pattern_get_rgba",
                     (void *)cairo_pattern_get_rgba
                     );
       f->addArg(type_cairo_pattern_t, "pattern");
       f->addArg(array_pfloat64_q, "red");
       f->addArg(array_pfloat64_q, "green");
       f->addArg(array_pfloat64_q, "blue");
       f->addArg(array_pfloat64_q, "alpha");

    f = mod->addFunc(type_uint, "cairo_pattern_get_surface",
                     (void *)cairo_pattern_get_surface
                     );
       f->addArg(type_cairo_pattern_t, "pattern");
       f->addArg(type_cairo_surface_t, "surface");

    f = mod->addFunc(type_uint, "cairo_pattern_get_color_stop_rgba",
                     (void *)cairo_pattern_get_color_stop_rgba
                     );
       f->addArg(type_cairo_pattern_t, "pattern");
       f->addArg(type_int, "index");
       f->addArg(array_pfloat64_q, "offset");
       f->addArg(array_pfloat64_q, "red");
       f->addArg(array_pfloat64_q, "green");
       f->addArg(array_pfloat64_q, "blue");
       f->addArg(array_pfloat64_q, "alpha");

    f = mod->addFunc(type_uint, "cairo_pattern_get_color_stop_count",
                     (void *)cairo_pattern_get_color_stop_count
                     );
       f->addArg(type_cairo_pattern_t, "pattern");
       f->addArg(array_pint_q, "count");

    f = mod->addFunc(type_uint, "cairo_pattern_get_linear_points",
                     (void *)cairo_pattern_get_linear_points
                     );
       f->addArg(type_cairo_pattern_t, "pattern");
       f->addArg(array_pfloat64_q, "x0");
       f->addArg(array_pfloat64_q, "y0");
       f->addArg(array_pfloat64_q, "x1");
       f->addArg(array_pfloat64_q, "y1");

    f = mod->addFunc(type_uint, "cairo_pattern_get_radial_circles",
                     (void *)cairo_pattern_get_radial_circles
                     );
       f->addArg(type_cairo_pattern_t, "pattern");
       f->addArg(array_pfloat64_q, "x0");
       f->addArg(array_pfloat64_q, "y0");
       f->addArg(array_pfloat64_q, "r0");
       f->addArg(array_pfloat64_q, "x1");
       f->addArg(array_pfloat64_q, "y1");
       f->addArg(array_pfloat64_q, "r1");

    f = mod->addFunc(type_void, "cairo_matrix_init",
                     (void *)cairo_matrix_init
                     );
       f->addArg(type_cairo_matrix_t, "matrix");
       f->addArg(type_float64, "xx");
       f->addArg(type_float64, "yx");
       f->addArg(type_float64, "xy");
       f->addArg(type_float64, "yy");
       f->addArg(type_float64, "x0");
       f->addArg(type_float64, "y0");

    f = mod->addFunc(type_void, "cairo_matrix_init_identity",
                     (void *)cairo_matrix_init_identity
                     );
       f->addArg(type_cairo_matrix_t, "matrix");

    f = mod->addFunc(type_void, "cairo_matrix_init_translate",
                     (void *)cairo_matrix_init_translate
                     );
       f->addArg(type_cairo_matrix_t, "matrix");
       f->addArg(type_float64, "tx");
       f->addArg(type_float64, "ty");

    f = mod->addFunc(type_void, "cairo_matrix_init_scale",
                     (void *)cairo_matrix_init_scale
                     );
       f->addArg(type_cairo_matrix_t, "matrix");
       f->addArg(type_float64, "sx");
       f->addArg(type_float64, "sy");

    f = mod->addFunc(type_void, "cairo_matrix_init_rotate",
                     (void *)cairo_matrix_init_rotate
                     );
       f->addArg(type_cairo_matrix_t, "matrix");
       f->addArg(type_float64, "radians");

    f = mod->addFunc(type_void, "cairo_matrix_translate",
                     (void *)cairo_matrix_translate
                     );
       f->addArg(type_cairo_matrix_t, "matrix");
       f->addArg(type_float64, "tx");
       f->addArg(type_float64, "ty");

    f = mod->addFunc(type_void, "cairo_matrix_scale",
                     (void *)cairo_matrix_scale
                     );
       f->addArg(type_cairo_matrix_t, "matrix");
       f->addArg(type_float64, "sx");
       f->addArg(type_float64, "sy");

    f = mod->addFunc(type_void, "cairo_matrix_rotate",
                     (void *)cairo_matrix_rotate
                     );
       f->addArg(type_cairo_matrix_t, "matrix");
       f->addArg(type_float64, "radians");

    f = mod->addFunc(type_uint, "cairo_matrix_invert",
                     (void *)cairo_matrix_invert
                     );
       f->addArg(type_cairo_matrix_t, "matrix");

    f = mod->addFunc(type_void, "cairo_matrix_multiply",
                     (void *)cairo_matrix_multiply
                     );
       f->addArg(type_cairo_matrix_t, "result");
       f->addArg(type_cairo_matrix_t, "a");
       f->addArg(type_cairo_matrix_t, "b");

    f = mod->addFunc(type_void, "cairo_matrix_transform_distance",
                     (void *)cairo_matrix_transform_distance
                     );
       f->addArg(type_cairo_matrix_t, "matrix");
       f->addArg(array_pfloat64_q, "dx");
       f->addArg(array_pfloat64_q, "dy");

    f = mod->addFunc(type_void, "cairo_matrix_transform_point",
                     (void *)cairo_matrix_transform_point
                     );
       f->addArg(type_cairo_matrix_t, "matrix");
       f->addArg(array_pfloat64_q, "x");
       f->addArg(array_pfloat64_q, "y");

    f = mod->addFunc(type_cairo_region_t, "cairo_region_create",
                     (void *)cairo_region_create
                     );

    f = mod->addFunc(type_cairo_region_t, "cairo_region_create_rectangle",
                     (void *)cairo_region_create_rectangle
                     );
       f->addArg(type_cairo_rectangle_int_t, "rectangle");

    f = mod->addFunc(type_cairo_region_t, "cairo_region_create_rectangles",
                     (void *)cairo_region_create_rectangles
                     );
       f->addArg(type_cairo_rectangle_int_t, "rects");
       f->addArg(type_int, "count");

    f = mod->addFunc(type_cairo_region_t, "cairo_region_copy",
                     (void *)cairo_region_copy
                     );
       f->addArg(type_cairo_region_t, "original");

    f = mod->addFunc(type_cairo_region_t, "cairo_region_reference",
                     (void *)cairo_region_reference
                     );
       f->addArg(type_cairo_region_t, "region");

    f = mod->addFunc(type_void, "cairo_region_destroy",
                     (void *)cairo_region_destroy
                     );
       f->addArg(type_cairo_region_t, "region");

    f = mod->addFunc(type_bool, "cairo_region_equal",
                     (void *)cairo_region_equal
                     );
       f->addArg(type_cairo_region_t, "region");
       f->addArg(type_cairo_region_t, "b");

    f = mod->addFunc(type_uint, "cairo_region_status",
                     (void *)cairo_region_status
                     );
       f->addArg(type_cairo_region_t, "region");

    f = mod->addFunc(type_void, "cairo_region_get_extents",
                     (void *)cairo_region_get_extents
                     );
       f->addArg(type_cairo_region_t, "region");
       f->addArg(type_cairo_rectangle_int_t, "extents");

    f = mod->addFunc(type_int, "cairo_region_num_rectangles",
                     (void *)cairo_region_num_rectangles
                     );
       f->addArg(type_cairo_region_t, "region");

    f = mod->addFunc(type_void, "cairo_region_get_rectangle",
                     (void *)cairo_region_get_rectangle
                     );
       f->addArg(type_cairo_region_t, "region");
       f->addArg(type_int, "nth");
       f->addArg(type_cairo_rectangle_int_t, "rectangle");

    f = mod->addFunc(type_bool, "cairo_region_is_empty",
                     (void *)cairo_region_is_empty
                     );
       f->addArg(type_cairo_region_t, "region");

    f = mod->addFunc(type_uint, "cairo_region_contains_rectangle",
                     (void *)cairo_region_contains_rectangle
                     );
       f->addArg(type_cairo_region_t, "region");
       f->addArg(type_cairo_rectangle_int_t, "rectangle");

    f = mod->addFunc(type_bool, "cairo_region_contains_point",
                     (void *)cairo_region_contains_point
                     );
       f->addArg(type_cairo_region_t, "region");
       f->addArg(type_int, "x");
       f->addArg(type_int, "y");

    f = mod->addFunc(type_void, "cairo_region_translate",
                     (void *)cairo_region_translate
                     );
       f->addArg(type_cairo_region_t, "region");
       f->addArg(type_int, "dx");
       f->addArg(type_int, "dy");

    f = mod->addFunc(type_uint, "cairo_region_subtract",
                     (void *)cairo_region_subtract
                     );
       f->addArg(type_cairo_region_t, "region");
       f->addArg(type_cairo_region_t, "other");

    f = mod->addFunc(type_uint, "cairo_region_subtract_rectangle",
                     (void *)cairo_region_subtract_rectangle
                     );
       f->addArg(type_cairo_region_t, "region");
       f->addArg(type_cairo_rectangle_int_t, "rectangle");

    f = mod->addFunc(type_uint, "cairo_region_intersect",
                     (void *)cairo_region_intersect
                     );
       f->addArg(type_cairo_region_t, "region");
       f->addArg(type_cairo_region_t, "other");

    f = mod->addFunc(type_uint, "cairo_region_intersect_rectangle",
                     (void *)cairo_region_intersect_rectangle
                     );
       f->addArg(type_cairo_region_t, "region");
       f->addArg(type_cairo_rectangle_int_t, "rectangle");

    f = mod->addFunc(type_uint, "cairo_region_union",
                     (void *)cairo_region_union
                     );
       f->addArg(type_cairo_region_t, "region");
       f->addArg(type_cairo_region_t, "other");

    f = mod->addFunc(type_uint, "cairo_region_union_rectangle",
                     (void *)cairo_region_union_rectangle
                     );
       f->addArg(type_cairo_region_t, "region");
       f->addArg(type_cairo_rectangle_int_t, "rectangle");

    f = mod->addFunc(type_uint, "cairo_region_xor",
                     (void *)cairo_region_xor
                     );
       f->addArg(type_cairo_region_t, "region");
       f->addArg(type_cairo_region_t, "other");

    f = mod->addFunc(type_uint, "cairo_region_xor_rectangle",
                     (void *)cairo_region_xor_rectangle
                     );
       f->addArg(type_cairo_region_t, "region");
       f->addArg(type_cairo_rectangle_int_t, "rectangle");

    f = mod->addFunc(type_void, "cairo_debug_reset_static_data",
                     (void *)cairo_debug_reset_static_data
                     );

    f = mod->addFunc(type_cairo_surface_t, "cairo_pdf_surface_create",
                     (void *)cairo_pdf_surface_create
                     );
       f->addArg(type_byteptr, "filename");
       f->addArg(type_float64, "width_in_points");
       f->addArg(type_float64, "height_in_points");

    f = mod->addFunc(type_cairo_surface_t, "cairo_pdf_surface_create_for_stream",
                     (void *)cairo_pdf_surface_create_for_stream
                     );
       f->addArg(function_puint_c_svoidptr_c_sbyteptr_c_suint_q, "write_func");
       f->addArg(type_voidptr, "closure");
       f->addArg(type_float64, "width_in_points");
       f->addArg(type_float64, "height_in_points");

    f = mod->addFunc(type_void, "cairo_pdf_surface_restrict_to_version",
                     (void *)cairo_pdf_surface_restrict_to_version
                     );
       f->addArg(type_cairo_surface_t, "surface");
       f->addArg(type_uint, "version");

    f = mod->addFunc(type_void, "cairo_pdf_get_versions",
                     (void *)cairo_pdf_get_versions
                     );
       f->addArg(array_puint_q, "versions");
       f->addArg(array_pint_q, "num_versions");

    f = mod->addFunc(type_byteptr, "cairo_pdf_version_to_string",
                     (void *)cairo_pdf_version_to_string
                     );
       f->addArg(type_uint, "version");

    f = mod->addFunc(type_void, "cairo_pdf_surface_set_size",
                     (void *)cairo_pdf_surface_set_size
                     );
       f->addArg(type_cairo_surface_t, "surface");
       f->addArg(type_float64, "width_in_points");
       f->addArg(type_float64, "height_in_points");

    f = mod->addFunc(type_cairo_surface_t, "cairo_svg_surface_create",
                     (void *)cairo_svg_surface_create
                     );
       f->addArg(type_byteptr, "filename");
       f->addArg(type_float64, "width_in_points");
       f->addArg(type_float64, "height_in_points");

    f = mod->addFunc(type_cairo_surface_t, "cairo_svg_surface_create_for_stream",
                     (void *)cairo_svg_surface_create_for_stream
                     );
       f->addArg(function_puint_c_svoidptr_c_sbyteptr_c_suint_q, "write_func");
       f->addArg(type_voidptr, "closure");
       f->addArg(type_float64, "width_in_points");
       f->addArg(type_float64, "height_in_points");

    f = mod->addFunc(type_void, "cairo_svg_surface_restrict_to_version",
                     (void *)cairo_svg_surface_restrict_to_version
                     );
       f->addArg(type_cairo_surface_t, "surface");
       f->addArg(type_uint, "version");

    f = mod->addFunc(type_void, "cairo_svg_get_versions",
                     (void *)cairo_svg_get_versions
                     );
       f->addArg(array_puint_q, "versions");
       f->addArg(array_pint_q, "num_versions");

    f = mod->addFunc(type_byteptr, "cairo_svg_version_to_string",
                     (void *)cairo_svg_version_to_string
                     );
       f->addArg(type_uint, "version");

    f = mod->addFunc(type_cairo_surface_t, "cairo_xlib_surface_create",
                     (void *)cairo_xlib_surface_create
                     );
       f->addArg(type_Display, "dpy");
       f->addArg(type_Drawable, "drawable");
       f->addArg(type_Visual, "visual");
       f->addArg(type_int, "width");
       f->addArg(type_int, "height");

    f = mod->addFunc(type_cairo_surface_t, "cairo_xlib_surface_create_for_bitmap",
                     (void *)cairo_xlib_surface_create_for_bitmap
                     );
       f->addArg(type_Display, "dpy");
       f->addArg(type_Pixmap, "bitmap");
       f->addArg(type_Screen, "screen");
       f->addArg(type_int, "width");
       f->addArg(type_int, "height");

    f = mod->addFunc(type_void, "cairo_xlib_surface_set_size",
                     (void *)cairo_xlib_surface_set_size
                     );
       f->addArg(type_cairo_surface_t, "surface");
       f->addArg(type_int, "width");
       f->addArg(type_int, "height");

    f = mod->addFunc(type_void, "cairo_xlib_surface_set_drawable",
                     (void *)cairo_xlib_surface_set_drawable
                     );
       f->addArg(type_cairo_surface_t, "surface");
       f->addArg(type_Drawable, "drawable");
       f->addArg(type_int, "width");
       f->addArg(type_int, "height");

    f = mod->addFunc(type_Display, "cairo_xlib_surface_get_display",
                     (void *)cairo_xlib_surface_get_display
                     );
       f->addArg(type_cairo_surface_t, "surface");

    f = mod->addFunc(type_Drawable, "cairo_xlib_surface_get_drawable",
                     (void *)cairo_xlib_surface_get_drawable
                     );
       f->addArg(type_cairo_surface_t, "surface");

    f = mod->addFunc(type_Screen, "cairo_xlib_surface_get_screen",
                     (void *)cairo_xlib_surface_get_screen
                     );
       f->addArg(type_cairo_surface_t, "surface");

    f = mod->addFunc(type_Visual, "cairo_xlib_surface_get_visual",
                     (void *)cairo_xlib_surface_get_visual
                     );
       f->addArg(type_cairo_surface_t, "surface");

    f = mod->addFunc(type_int, "cairo_xlib_surface_get_depth",
                     (void *)cairo_xlib_surface_get_depth
                     );
       f->addArg(type_cairo_surface_t, "surface");

    f = mod->addFunc(type_int, "cairo_xlib_surface_get_width",
                     (void *)cairo_xlib_surface_get_width
                     );
       f->addArg(type_cairo_surface_t, "surface");

    f = mod->addFunc(type_int, "cairo_xlib_surface_get_height",
                     (void *)cairo_xlib_surface_get_height
                     );
       f->addArg(type_cairo_surface_t, "surface");

    f = mod->addFunc(type_cairo_surface_t, "cairo_ps_surface_create",
                     (void *)cairo_ps_surface_create
                     );
       f->addArg(type_byteptr, "filename");
       f->addArg(type_float64, "width_in_points");
       f->addArg(type_float64, "height_in_points");

    f = mod->addFunc(type_cairo_surface_t, "cairo_ps_surface_create_for_stream",
                     (void *)cairo_ps_surface_create_for_stream
                     );
       f->addArg(function_puint_c_svoidptr_c_sbyteptr_c_suint_q, "write_func");
       f->addArg(type_voidptr, "closure");
       f->addArg(type_float64, "width_in_points");
       f->addArg(type_float64, "height_in_points");

    f = mod->addFunc(type_void, "cairo_ps_surface_restrict_to_level",
                     (void *)cairo_ps_surface_restrict_to_level
                     );
       f->addArg(type_cairo_surface_t, "surface");
       f->addArg(type_uint, "level");

    f = mod->addFunc(type_void, "cairo_ps_get_levels",
                     (void *)cairo_ps_get_levels
                     );
       f->addArg(array_puint_q, "levels");
       f->addArg(type_int, "num_levels");

    f = mod->addFunc(type_byteptr, "cairo_ps_level_to_string",
                     (void *)cairo_ps_level_to_string
                     );
       f->addArg(type_uint, "level");

    f = mod->addFunc(type_void, "cairo_ps_surface_set_eps",
                     (void *)cairo_ps_surface_set_eps
                     );
       f->addArg(type_cairo_surface_t, "surface");
       f->addArg(type_bool, "eps");

    f = mod->addFunc(type_bool, "cairo_ps_surface_get_eps",
                     (void *)cairo_ps_surface_get_eps
                     );
       f->addArg(type_cairo_surface_t, "surface");

    f = mod->addFunc(type_void, "cairo_ps_surface_set_size",
                     (void *)cairo_ps_surface_set_size
                     );
       f->addArg(type_cairo_surface_t, "surface");
       f->addArg(type_float64, "width_in_points");
       f->addArg(type_float64, "height_in_points");

    f = mod->addFunc(type_void, "cairo_ps_surface_dsc_comment",
                     (void *)cairo_ps_surface_dsc_comment
                     );
       f->addArg(type_cairo_surface_t, "surface");
       f->addArg(type_byteptr, "comment");

    f = mod->addFunc(type_void, "cairo_ps_surface_dsc_begin_setup",
                     (void *)cairo_ps_surface_dsc_begin_setup
                     );
       f->addArg(type_cairo_surface_t, "surface");

    f = mod->addFunc(type_void, "cairo_ps_surface_dsc_begin_page_setup",
                     (void *)cairo_ps_surface_dsc_begin_page_setup
                     );
       f->addArg(type_cairo_surface_t, "surface");


    mod->addConstant(type_uint, "CAIRO_HAS_FC_FONT",
                     static_cast<int>(CAIRO_HAS_FC_FONT)
                     );

    mod->addConstant(type_uint, "CAIRO_HAS_FT_FONT",
                     static_cast<int>(CAIRO_HAS_FT_FONT)
                     );

    mod->addConstant(type_uint, "CAIRO_HAS_GOBJECT_FUNCTIONS",
                     static_cast<int>(CAIRO_HAS_GOBJECT_FUNCTIONS)
                     );

    mod->addConstant(type_uint, "CAIRO_HAS_IMAGE_SURFACE",
                     static_cast<int>(CAIRO_HAS_IMAGE_SURFACE)
                     );

    mod->addConstant(type_uint, "CAIRO_HAS_PDF_SURFACE",
                     static_cast<int>(CAIRO_HAS_PDF_SURFACE)
                     );

    mod->addConstant(type_uint, "CAIRO_HAS_PNG_FUNCTIONS",
                     static_cast<int>(CAIRO_HAS_PNG_FUNCTIONS)
                     );

    mod->addConstant(type_uint, "CAIRO_HAS_PS_SURFACE",
                     static_cast<int>(CAIRO_HAS_PS_SURFACE)
                     );

    mod->addConstant(type_uint, "CAIRO_HAS_RECORDING_SURFACE",
                     static_cast<int>(CAIRO_HAS_RECORDING_SURFACE)
                     );

    mod->addConstant(type_uint, "CAIRO_HAS_SVG_SURFACE",
                     static_cast<int>(CAIRO_HAS_SVG_SURFACE)
                     );

    mod->addConstant(type_uint, "CAIRO_HAS_USER_FONT",
                     static_cast<int>(CAIRO_HAS_USER_FONT)
                     );

    mod->addConstant(type_uint, "CAIRO_HAS_XCB_SHM_FUNCTIONS",
                     static_cast<int>(CAIRO_HAS_XCB_SHM_FUNCTIONS)
                     );

    mod->addConstant(type_uint, "CAIRO_HAS_XCB_SURFACE",
                     static_cast<int>(CAIRO_HAS_XCB_SURFACE)
                     );

    mod->addConstant(type_uint, "CAIRO_HAS_XLIB_SURFACE",
                     static_cast<int>(CAIRO_HAS_XLIB_SURFACE)
                     );

    mod->addConstant(type_uint, "CAIRO_HAS_XLIB_XRENDER_SURFACE",
                     static_cast<int>(CAIRO_HAS_XLIB_XRENDER_SURFACE)
                     );

    mod->addConstant(type_uint, "CAIRO_STATUS_SUCCESS",
                     static_cast<int>(CAIRO_STATUS_SUCCESS)
                     );

    mod->addConstant(type_uint, "CAIRO_STATUS_NO_MEMORY",
                     static_cast<int>(CAIRO_STATUS_NO_MEMORY)
                     );

    mod->addConstant(type_uint, "CAIRO_STATUS_INVALID_RESTORE",
                     static_cast<int>(CAIRO_STATUS_INVALID_RESTORE)
                     );

    mod->addConstant(type_uint, "CAIRO_STATUS_INVALID_POP_GROUP",
                     static_cast<int>(CAIRO_STATUS_INVALID_POP_GROUP)
                     );

    mod->addConstant(type_uint, "CAIRO_STATUS_NO_CURRENT_POINT",
                     static_cast<int>(CAIRO_STATUS_NO_CURRENT_POINT)
                     );

    mod->addConstant(type_uint, "CAIRO_STATUS_INVALID_MATRIX",
                     static_cast<int>(CAIRO_STATUS_INVALID_MATRIX)
                     );

    mod->addConstant(type_uint, "CAIRO_STATUS_INVALID_STATUS",
                     static_cast<int>(CAIRO_STATUS_INVALID_STATUS)
                     );

    mod->addConstant(type_uint, "CAIRO_STATUS_NULL_POINTER",
                     static_cast<int>(CAIRO_STATUS_NULL_POINTER)
                     );

    mod->addConstant(type_uint, "CAIRO_STATUS_INVALID_STRING",
                     static_cast<int>(CAIRO_STATUS_INVALID_STRING)
                     );

    mod->addConstant(type_uint, "CAIRO_STATUS_INVALID_PATH_DATA",
                     static_cast<int>(CAIRO_STATUS_INVALID_PATH_DATA)
                     );

    mod->addConstant(type_uint, "CAIRO_STATUS_READ_ERROR",
                     static_cast<int>(CAIRO_STATUS_READ_ERROR)
                     );

    mod->addConstant(type_uint, "CAIRO_STATUS_WRITE_ERROR",
                     static_cast<int>(CAIRO_STATUS_WRITE_ERROR)
                     );

    mod->addConstant(type_uint, "CAIRO_STATUS_SURFACE_FINISHED",
                     static_cast<int>(CAIRO_STATUS_SURFACE_FINISHED)
                     );

    mod->addConstant(type_uint, "CAIRO_STATUS_SURFACE_TYPE_MISMATCH",
                     static_cast<int>(CAIRO_STATUS_SURFACE_TYPE_MISMATCH)
                     );

    mod->addConstant(type_uint, "CAIRO_STATUS_PATTERN_TYPE_MISMATCH",
                     static_cast<int>(CAIRO_STATUS_PATTERN_TYPE_MISMATCH)
                     );

    mod->addConstant(type_uint, "CAIRO_STATUS_INVALID_CONTENT",
                     static_cast<int>(CAIRO_STATUS_INVALID_CONTENT)
                     );

    mod->addConstant(type_uint, "CAIRO_STATUS_INVALID_FORMAT",
                     static_cast<int>(CAIRO_STATUS_INVALID_FORMAT)
                     );

    mod->addConstant(type_uint, "CAIRO_STATUS_INVALID_VISUAL",
                     static_cast<int>(CAIRO_STATUS_INVALID_VISUAL)
                     );

    mod->addConstant(type_uint, "CAIRO_STATUS_FILE_NOT_FOUND",
                     static_cast<int>(CAIRO_STATUS_FILE_NOT_FOUND)
                     );

    mod->addConstant(type_uint, "CAIRO_STATUS_INVALID_DASH",
                     static_cast<int>(CAIRO_STATUS_INVALID_DASH)
                     );

    mod->addConstant(type_uint, "CAIRO_STATUS_INVALID_DSC_COMMENT",
                     static_cast<int>(CAIRO_STATUS_INVALID_DSC_COMMENT)
                     );

    mod->addConstant(type_uint, "CAIRO_STATUS_INVALID_INDEX",
                     static_cast<int>(CAIRO_STATUS_INVALID_INDEX)
                     );

    mod->addConstant(type_uint, "CAIRO_STATUS_CLIP_NOT_REPRESENTABLE",
                     static_cast<int>(CAIRO_STATUS_CLIP_NOT_REPRESENTABLE)
                     );

    mod->addConstant(type_uint, "CAIRO_STATUS_TEMP_FILE_ERROR",
                     static_cast<int>(CAIRO_STATUS_TEMP_FILE_ERROR)
                     );

    mod->addConstant(type_uint, "CAIRO_STATUS_INVALID_STRIDE",
                     static_cast<int>(CAIRO_STATUS_INVALID_STRIDE)
                     );

    mod->addConstant(type_uint, "CAIRO_STATUS_FONT_TYPE_MISMATCH",
                     static_cast<int>(CAIRO_STATUS_FONT_TYPE_MISMATCH)
                     );

    mod->addConstant(type_uint, "CAIRO_STATUS_USER_FONT_IMMUTABLE",
                     static_cast<int>(CAIRO_STATUS_USER_FONT_IMMUTABLE)
                     );

    mod->addConstant(type_uint, "CAIRO_STATUS_USER_FONT_ERROR",
                     static_cast<int>(CAIRO_STATUS_USER_FONT_ERROR)
                     );

    mod->addConstant(type_uint, "CAIRO_STATUS_NEGATIVE_COUNT",
                     static_cast<int>(CAIRO_STATUS_NEGATIVE_COUNT)
                     );

    mod->addConstant(type_uint, "CAIRO_STATUS_INVALID_CLUSTERS",
                     static_cast<int>(CAIRO_STATUS_INVALID_CLUSTERS)
                     );

    mod->addConstant(type_uint, "CAIRO_STATUS_INVALID_SLANT",
                     static_cast<int>(CAIRO_STATUS_INVALID_SLANT)
                     );

    mod->addConstant(type_uint, "CAIRO_STATUS_INVALID_WEIGHT",
                     static_cast<int>(CAIRO_STATUS_INVALID_WEIGHT)
                     );

    mod->addConstant(type_uint, "CAIRO_STATUS_INVALID_SIZE",
                     static_cast<int>(CAIRO_STATUS_INVALID_SIZE)
                     );

    mod->addConstant(type_uint, "CAIRO_STATUS_USER_FONT_NOT_IMPLEMENTED",
                     static_cast<int>(CAIRO_STATUS_USER_FONT_NOT_IMPLEMENTED)
                     );

    mod->addConstant(type_uint, "CAIRO_STATUS_DEVICE_TYPE_MISMATCH",
                     static_cast<int>(CAIRO_STATUS_DEVICE_TYPE_MISMATCH)
                     );

    mod->addConstant(type_uint, "CAIRO_STATUS_DEVICE_ERROR",
                     static_cast<int>(CAIRO_STATUS_DEVICE_ERROR)
                     );

    mod->addConstant(type_uint, "CAIRO_STATUS_LAST_STATUS",
                     static_cast<int>(CAIRO_STATUS_LAST_STATUS)
                     );

    mod->addConstant(type_uint, "CAIRO_CONTENT_COLOR",
                     static_cast<int>(CAIRO_CONTENT_COLOR)
                     );

    mod->addConstant(type_uint, "CAIRO_CONTENT_ALPHA",
                     static_cast<int>(CAIRO_CONTENT_ALPHA)
                     );

    mod->addConstant(type_uint, "CAIRO_CONTENT_COLOR_ALPHA",
                     static_cast<int>(CAIRO_CONTENT_COLOR_ALPHA)
                     );

    mod->addConstant(type_uint, "CAIRO_OPERATOR_CLEAR",
                     static_cast<int>(CAIRO_OPERATOR_CLEAR)
                     );

    mod->addConstant(type_uint, "CAIRO_OPERATOR_SOURCE",
                     static_cast<int>(CAIRO_OPERATOR_SOURCE)
                     );

    mod->addConstant(type_uint, "CAIRO_OPERATOR_OVER",
                     static_cast<int>(CAIRO_OPERATOR_OVER)
                     );

    mod->addConstant(type_uint, "CAIRO_OPERATOR_IN",
                     static_cast<int>(CAIRO_OPERATOR_IN)
                     );

    mod->addConstant(type_uint, "CAIRO_OPERATOR_OUT",
                     static_cast<int>(CAIRO_OPERATOR_OUT)
                     );

    mod->addConstant(type_uint, "CAIRO_OPERATOR_ATOP",
                     static_cast<int>(CAIRO_OPERATOR_ATOP)
                     );

    mod->addConstant(type_uint, "CAIRO_OPERATOR_DEST",
                     static_cast<int>(CAIRO_OPERATOR_DEST)
                     );

    mod->addConstant(type_uint, "CAIRO_OPERATOR_DEST_OVER",
                     static_cast<int>(CAIRO_OPERATOR_DEST_OVER)
                     );

    mod->addConstant(type_uint, "CAIRO_OPERATOR_DEST_IN",
                     static_cast<int>(CAIRO_OPERATOR_DEST_IN)
                     );

    mod->addConstant(type_uint, "CAIRO_OPERATOR_DEST_OUT",
                     static_cast<int>(CAIRO_OPERATOR_DEST_OUT)
                     );

    mod->addConstant(type_uint, "CAIRO_OPERATOR_DEST_ATOP",
                     static_cast<int>(CAIRO_OPERATOR_DEST_ATOP)
                     );

    mod->addConstant(type_uint, "CAIRO_OPERATOR_XOR",
                     static_cast<int>(CAIRO_OPERATOR_XOR)
                     );

    mod->addConstant(type_uint, "CAIRO_OPERATOR_ADD",
                     static_cast<int>(CAIRO_OPERATOR_ADD)
                     );

    mod->addConstant(type_uint, "CAIRO_OPERATOR_SATURATE",
                     static_cast<int>(CAIRO_OPERATOR_SATURATE)
                     );

    mod->addConstant(type_uint, "CAIRO_OPERATOR_MULTIPLY",
                     static_cast<int>(CAIRO_OPERATOR_MULTIPLY)
                     );

    mod->addConstant(type_uint, "CAIRO_OPERATOR_SCREEN",
                     static_cast<int>(CAIRO_OPERATOR_SCREEN)
                     );

    mod->addConstant(type_uint, "CAIRO_OPERATOR_OVERLAY",
                     static_cast<int>(CAIRO_OPERATOR_OVERLAY)
                     );

    mod->addConstant(type_uint, "CAIRO_OPERATOR_DARKEN",
                     static_cast<int>(CAIRO_OPERATOR_DARKEN)
                     );

    mod->addConstant(type_uint, "CAIRO_OPERATOR_LIGHTEN",
                     static_cast<int>(CAIRO_OPERATOR_LIGHTEN)
                     );

    mod->addConstant(type_uint, "CAIRO_OPERATOR_COLOR_DODGE",
                     static_cast<int>(CAIRO_OPERATOR_COLOR_DODGE)
                     );

    mod->addConstant(type_uint, "CAIRO_OPERATOR_COLOR_BURN",
                     static_cast<int>(CAIRO_OPERATOR_COLOR_BURN)
                     );

    mod->addConstant(type_uint, "CAIRO_OPERATOR_HARD_LIGHT",
                     static_cast<int>(CAIRO_OPERATOR_HARD_LIGHT)
                     );

    mod->addConstant(type_uint, "CAIRO_OPERATOR_SOFT_LIGHT",
                     static_cast<int>(CAIRO_OPERATOR_SOFT_LIGHT)
                     );

    mod->addConstant(type_uint, "CAIRO_OPERATOR_DIFFERENCE",
                     static_cast<int>(CAIRO_OPERATOR_DIFFERENCE)
                     );

    mod->addConstant(type_uint, "CAIRO_OPERATOR_EXCLUSION",
                     static_cast<int>(CAIRO_OPERATOR_EXCLUSION)
                     );

    mod->addConstant(type_uint, "CAIRO_OPERATOR_HSL_HUE",
                     static_cast<int>(CAIRO_OPERATOR_HSL_HUE)
                     );

    mod->addConstant(type_uint, "CAIRO_OPERATOR_HSL_SATURATION",
                     static_cast<int>(CAIRO_OPERATOR_HSL_SATURATION)
                     );

    mod->addConstant(type_uint, "CAIRO_OPERATOR_HSL_COLOR",
                     static_cast<int>(CAIRO_OPERATOR_HSL_COLOR)
                     );

    mod->addConstant(type_uint, "CAIRO_OPERATOR_HSL_LUMINOSITY",
                     static_cast<int>(CAIRO_OPERATOR_HSL_LUMINOSITY)
                     );

    mod->addConstant(type_uint, "CAIRO_ANTIALIAS_DEFAULT",
                     static_cast<int>(CAIRO_ANTIALIAS_DEFAULT)
                     );

    mod->addConstant(type_uint, "CAIRO_ANTIALIAS_NONE",
                     static_cast<int>(CAIRO_ANTIALIAS_NONE)
                     );

    mod->addConstant(type_uint, "CAIRO_ANTIALIAS_GRAY",
                     static_cast<int>(CAIRO_ANTIALIAS_GRAY)
                     );

    mod->addConstant(type_uint, "CAIRO_ANTIALIAS_SUBPIXEL",
                     static_cast<int>(CAIRO_ANTIALIAS_SUBPIXEL)
                     );

    mod->addConstant(type_uint, "CAIRO_FILL_RULE_WINDING",
                     static_cast<int>(CAIRO_FILL_RULE_WINDING)
                     );

    mod->addConstant(type_uint, "CAIRO_FILL_RULE_EVEN_ODD",
                     static_cast<int>(CAIRO_FILL_RULE_EVEN_ODD)
                     );

    mod->addConstant(type_uint, "CAIRO_LINE_CAP_BUTT",
                     static_cast<int>(CAIRO_LINE_CAP_BUTT)
                     );

    mod->addConstant(type_uint, "CAIRO_LINE_CAP_ROUND",
                     static_cast<int>(CAIRO_LINE_CAP_ROUND)
                     );

    mod->addConstant(type_uint, "CAIRO_LINE_CAP_SQUARE",
                     static_cast<int>(CAIRO_LINE_CAP_SQUARE)
                     );

    mod->addConstant(type_uint, "CAIRO_LINE_JOIN_MITER",
                     static_cast<int>(CAIRO_LINE_JOIN_MITER)
                     );

    mod->addConstant(type_uint, "CAIRO_LINE_JOIN_ROUND",
                     static_cast<int>(CAIRO_LINE_JOIN_ROUND)
                     );

    mod->addConstant(type_uint, "CAIRO_LINE_JOIN_BEVEL",
                     static_cast<int>(CAIRO_LINE_JOIN_BEVEL)
                     );

    mod->addConstant(type_uint, "CAIRO_TEXT_CLUSTER_FLAG_BACKWARD",
                     static_cast<int>(CAIRO_TEXT_CLUSTER_FLAG_BACKWARD)
                     );

    mod->addConstant(type_uint, "CAIRO_FONT_SLANT_NORMAL",
                     static_cast<int>(CAIRO_FONT_SLANT_NORMAL)
                     );

    mod->addConstant(type_uint, "CAIRO_FONT_SLANT_ITALIC",
                     static_cast<int>(CAIRO_FONT_SLANT_ITALIC)
                     );

    mod->addConstant(type_uint, "CAIRO_FONT_SLANT_OBLIQUE",
                     static_cast<int>(CAIRO_FONT_SLANT_OBLIQUE)
                     );

    mod->addConstant(type_uint, "CAIRO_FONT_WEIGHT_NORMAL",
                     static_cast<int>(CAIRO_FONT_WEIGHT_NORMAL)
                     );

    mod->addConstant(type_uint, "CAIRO_FONT_WEIGHT_BOLD",
                     static_cast<int>(CAIRO_FONT_WEIGHT_BOLD)
                     );

    mod->addConstant(type_uint, "CAIRO_SUBPIXEL_ORDER_DEFAULT",
                     static_cast<int>(CAIRO_SUBPIXEL_ORDER_DEFAULT)
                     );

    mod->addConstant(type_uint, "CAIRO_SUBPIXEL_ORDER_RGB",
                     static_cast<int>(CAIRO_SUBPIXEL_ORDER_RGB)
                     );

    mod->addConstant(type_uint, "CAIRO_SUBPIXEL_ORDER_BGR",
                     static_cast<int>(CAIRO_SUBPIXEL_ORDER_BGR)
                     );

    mod->addConstant(type_uint, "CAIRO_SUBPIXEL_ORDER_VRGB",
                     static_cast<int>(CAIRO_SUBPIXEL_ORDER_VRGB)
                     );

    mod->addConstant(type_uint, "CAIRO_SUBPIXEL_ORDER_VBGR",
                     static_cast<int>(CAIRO_SUBPIXEL_ORDER_VBGR)
                     );

    mod->addConstant(type_uint, "CAIRO_HINT_STYLE_DEFAULT",
                     static_cast<int>(CAIRO_HINT_STYLE_DEFAULT)
                     );

    mod->addConstant(type_uint, "CAIRO_HINT_STYLE_NONE",
                     static_cast<int>(CAIRO_HINT_STYLE_NONE)
                     );

    mod->addConstant(type_uint, "CAIRO_HINT_STYLE_SLIGHT",
                     static_cast<int>(CAIRO_HINT_STYLE_SLIGHT)
                     );

    mod->addConstant(type_uint, "CAIRO_HINT_STYLE_MEDIUM",
                     static_cast<int>(CAIRO_HINT_STYLE_MEDIUM)
                     );

    mod->addConstant(type_uint, "CAIRO_HINT_STYLE_FULL",
                     static_cast<int>(CAIRO_HINT_STYLE_FULL)
                     );

    mod->addConstant(type_uint, "CAIRO_HINT_METRICS_DEFAULT",
                     static_cast<int>(CAIRO_HINT_METRICS_DEFAULT)
                     );

    mod->addConstant(type_uint, "CAIRO_HINT_METRICS_OFF",
                     static_cast<int>(CAIRO_HINT_METRICS_OFF)
                     );

    mod->addConstant(type_uint, "CAIRO_HINT_METRICS_ON",
                     static_cast<int>(CAIRO_HINT_METRICS_ON)
                     );

    mod->addConstant(type_uint, "CAIRO_FONT_TYPE_TOY",
                     static_cast<int>(CAIRO_FONT_TYPE_TOY)
                     );

    mod->addConstant(type_uint, "CAIRO_FONT_TYPE_FT",
                     static_cast<int>(CAIRO_FONT_TYPE_FT)
                     );

    mod->addConstant(type_uint, "CAIRO_FONT_TYPE_WIN32",
                     static_cast<int>(CAIRO_FONT_TYPE_WIN32)
                     );

    mod->addConstant(type_uint, "CAIRO_FONT_TYPE_QUARTZ",
                     static_cast<int>(CAIRO_FONT_TYPE_QUARTZ)
                     );

    mod->addConstant(type_uint, "CAIRO_FONT_TYPE_USER",
                     static_cast<int>(CAIRO_FONT_TYPE_USER)
                     );

    mod->addConstant(type_uint, "CAIRO_PATH_MOVE_TO",
                     static_cast<int>(CAIRO_PATH_MOVE_TO)
                     );

    mod->addConstant(type_uint, "CAIRO_PATH_LINE_TO",
                     static_cast<int>(CAIRO_PATH_LINE_TO)
                     );

    mod->addConstant(type_uint, "CAIRO_PATH_CURVE_TO",
                     static_cast<int>(CAIRO_PATH_CURVE_TO)
                     );

    mod->addConstant(type_uint, "CAIRO_PATH_CLOSE_PATH",
                     static_cast<int>(CAIRO_PATH_CLOSE_PATH)
                     );

    mod->addConstant(type_uint, "CAIRO_DEVICE_TYPE_DRM",
                     static_cast<int>(CAIRO_DEVICE_TYPE_DRM)
                     );

    mod->addConstant(type_uint, "CAIRO_DEVICE_TYPE_GL",
                     static_cast<int>(CAIRO_DEVICE_TYPE_GL)
                     );

    mod->addConstant(type_uint, "CAIRO_DEVICE_TYPE_SCRIPT",
                     static_cast<int>(CAIRO_DEVICE_TYPE_SCRIPT)
                     );

    mod->addConstant(type_uint, "CAIRO_DEVICE_TYPE_XCB",
                     static_cast<int>(CAIRO_DEVICE_TYPE_XCB)
                     );

    mod->addConstant(type_uint, "CAIRO_DEVICE_TYPE_XLIB",
                     static_cast<int>(CAIRO_DEVICE_TYPE_XLIB)
                     );

    mod->addConstant(type_uint, "CAIRO_DEVICE_TYPE_XML",
                     static_cast<int>(CAIRO_DEVICE_TYPE_XML)
                     );

    mod->addConstant(type_uint, "CAIRO_SURFACE_TYPE_IMAGE",
                     static_cast<int>(CAIRO_SURFACE_TYPE_IMAGE)
                     );

    mod->addConstant(type_uint, "CAIRO_SURFACE_TYPE_PDF",
                     static_cast<int>(CAIRO_SURFACE_TYPE_PDF)
                     );

    mod->addConstant(type_uint, "CAIRO_SURFACE_TYPE_PS",
                     static_cast<int>(CAIRO_SURFACE_TYPE_PS)
                     );

    mod->addConstant(type_uint, "CAIRO_SURFACE_TYPE_XLIB",
                     static_cast<int>(CAIRO_SURFACE_TYPE_XLIB)
                     );

    mod->addConstant(type_uint, "CAIRO_SURFACE_TYPE_XCB",
                     static_cast<int>(CAIRO_SURFACE_TYPE_XCB)
                     );

    mod->addConstant(type_uint, "CAIRO_SURFACE_TYPE_GLITZ",
                     static_cast<int>(CAIRO_SURFACE_TYPE_GLITZ)
                     );

    mod->addConstant(type_uint, "CAIRO_SURFACE_TYPE_QUARTZ",
                     static_cast<int>(CAIRO_SURFACE_TYPE_QUARTZ)
                     );

    mod->addConstant(type_uint, "CAIRO_SURFACE_TYPE_WIN32",
                     static_cast<int>(CAIRO_SURFACE_TYPE_WIN32)
                     );

    mod->addConstant(type_uint, "CAIRO_SURFACE_TYPE_BEOS",
                     static_cast<int>(CAIRO_SURFACE_TYPE_BEOS)
                     );

    mod->addConstant(type_uint, "CAIRO_SURFACE_TYPE_DIRECTFB",
                     static_cast<int>(CAIRO_SURFACE_TYPE_DIRECTFB)
                     );

    mod->addConstant(type_uint, "CAIRO_SURFACE_TYPE_SVG",
                     static_cast<int>(CAIRO_SURFACE_TYPE_SVG)
                     );

    mod->addConstant(type_uint, "CAIRO_SURFACE_TYPE_OS2",
                     static_cast<int>(CAIRO_SURFACE_TYPE_OS2)
                     );

    mod->addConstant(type_uint, "CAIRO_SURFACE_TYPE_WIN32_PRINTING",
                     static_cast<int>(CAIRO_SURFACE_TYPE_WIN32_PRINTING)
                     );

    mod->addConstant(type_uint, "CAIRO_SURFACE_TYPE_QUARTZ_IMAGE",
                     static_cast<int>(CAIRO_SURFACE_TYPE_QUARTZ_IMAGE)
                     );

    mod->addConstant(type_uint, "CAIRO_SURFACE_TYPE_SCRIPT",
                     static_cast<int>(CAIRO_SURFACE_TYPE_SCRIPT)
                     );

    mod->addConstant(type_uint, "CAIRO_SURFACE_TYPE_QT",
                     static_cast<int>(CAIRO_SURFACE_TYPE_QT)
                     );

    mod->addConstant(type_uint, "CAIRO_SURFACE_TYPE_RECORDING",
                     static_cast<int>(CAIRO_SURFACE_TYPE_RECORDING)
                     );

    mod->addConstant(type_uint, "CAIRO_SURFACE_TYPE_VG",
                     static_cast<int>(CAIRO_SURFACE_TYPE_VG)
                     );

    mod->addConstant(type_uint, "CAIRO_SURFACE_TYPE_GL",
                     static_cast<int>(CAIRO_SURFACE_TYPE_GL)
                     );

    mod->addConstant(type_uint, "CAIRO_SURFACE_TYPE_DRM",
                     static_cast<int>(CAIRO_SURFACE_TYPE_DRM)
                     );

    mod->addConstant(type_uint, "CAIRO_SURFACE_TYPE_TEE",
                     static_cast<int>(CAIRO_SURFACE_TYPE_TEE)
                     );

    mod->addConstant(type_uint, "CAIRO_SURFACE_TYPE_XML",
                     static_cast<int>(CAIRO_SURFACE_TYPE_XML)
                     );

    mod->addConstant(type_uint, "CAIRO_SURFACE_TYPE_SKIA",
                     static_cast<int>(CAIRO_SURFACE_TYPE_SKIA)
                     );

    mod->addConstant(type_uint, "CAIRO_SURFACE_TYPE_SUBSURFACE",
                     static_cast<int>(CAIRO_SURFACE_TYPE_SUBSURFACE)
                     );

    mod->addConstant(type_uint, "CAIRO_FORMAT_INVALID",
                     static_cast<int>(CAIRO_FORMAT_INVALID)
                     );

    mod->addConstant(type_uint, "CAIRO_FORMAT_ARGB32",
                     static_cast<int>(CAIRO_FORMAT_ARGB32)
                     );

    mod->addConstant(type_uint, "CAIRO_FORMAT_RGB24",
                     static_cast<int>(CAIRO_FORMAT_RGB24)
                     );

    mod->addConstant(type_uint, "CAIRO_FORMAT_A8",
                     static_cast<int>(CAIRO_FORMAT_A8)
                     );

    mod->addConstant(type_uint, "CAIRO_FORMAT_A1",
                     static_cast<int>(CAIRO_FORMAT_A1)
                     );

    mod->addConstant(type_uint, "CAIRO_FORMAT_RGB16_565",
                     static_cast<int>(CAIRO_FORMAT_RGB16_565)
                     );

    mod->addConstant(type_uint, "CAIRO_PATTERN_TYPE_SOLID",
                     static_cast<int>(CAIRO_PATTERN_TYPE_SOLID)
                     );

    mod->addConstant(type_uint, "CAIRO_PATTERN_TYPE_SURFACE",
                     static_cast<int>(CAIRO_PATTERN_TYPE_SURFACE)
                     );

    mod->addConstant(type_uint, "CAIRO_PATTERN_TYPE_LINEAR",
                     static_cast<int>(CAIRO_PATTERN_TYPE_LINEAR)
                     );

    mod->addConstant(type_uint, "CAIRO_PATTERN_TYPE_RADIAL",
                     static_cast<int>(CAIRO_PATTERN_TYPE_RADIAL)
                     );

    mod->addConstant(type_uint, "CAIRO_EXTEND_NONE",
                     static_cast<int>(CAIRO_EXTEND_NONE)
                     );

    mod->addConstant(type_uint, "CAIRO_EXTEND_REPEAT",
                     static_cast<int>(CAIRO_EXTEND_REPEAT)
                     );

    mod->addConstant(type_uint, "CAIRO_EXTEND_REFLECT",
                     static_cast<int>(CAIRO_EXTEND_REFLECT)
                     );

    mod->addConstant(type_uint, "CAIRO_EXTEND_PAD",
                     static_cast<int>(CAIRO_EXTEND_PAD)
                     );

    mod->addConstant(type_uint, "CAIRO_FILTER_FAST",
                     static_cast<int>(CAIRO_FILTER_FAST)
                     );

    mod->addConstant(type_uint, "CAIRO_FILTER_GOOD",
                     static_cast<int>(CAIRO_FILTER_GOOD)
                     );

    mod->addConstant(type_uint, "CAIRO_FILTER_BEST",
                     static_cast<int>(CAIRO_FILTER_BEST)
                     );

    mod->addConstant(type_uint, "CAIRO_FILTER_NEAREST",
                     static_cast<int>(CAIRO_FILTER_NEAREST)
                     );

    mod->addConstant(type_uint, "CAIRO_FILTER_BILINEAR",
                     static_cast<int>(CAIRO_FILTER_BILINEAR)
                     );

    mod->addConstant(type_uint, "CAIRO_FILTER_GAUSSIAN",
                     static_cast<int>(CAIRO_FILTER_GAUSSIAN)
                     );

    mod->addConstant(type_uint, "CAIRO_REGION_OVERLAP_IN",
                     static_cast<int>(CAIRO_REGION_OVERLAP_IN)
                     );

    mod->addConstant(type_uint, "CAIRO_REGION_OVERLAP_OUT",
                     static_cast<int>(CAIRO_REGION_OVERLAP_OUT)
                     );

    mod->addConstant(type_uint, "CAIRO_REGION_OVERLAP_PART",
                     static_cast<int>(CAIRO_REGION_OVERLAP_PART)
                     );

    mod->addConstant(type_uint, "CAIRO_PDF_VERSION_1_4",
                     static_cast<int>(CAIRO_PDF_VERSION_1_4)
                     );

    mod->addConstant(type_uint, "CAIRO_PDF_VERSION_1_5",
                     static_cast<int>(CAIRO_PDF_VERSION_1_5)
                     );

    mod->addConstant(type_uint, "CAIRO_SVG_VERSION_1_1",
                     static_cast<int>(CAIRO_SVG_VERSION_1_1)
                     );

    mod->addConstant(type_uint, "CAIRO_SVG_VERSION_1_2",
                     static_cast<int>(CAIRO_SVG_VERSION_1_2)
                     );

    mod->addConstant(type_uint, "CAIRO_PS_LEVEL_2",
                     static_cast<int>(CAIRO_PS_LEVEL_2)
                     );

    mod->addConstant(type_uint, "CAIRO_PS_LEVEL_3",
                     static_cast<int>(CAIRO_PS_LEVEL_3)
                     );
}
