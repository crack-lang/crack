#include <cairo.h>


#include "ext/Module.h"
#include "ext/Type.h"
#include "ext/Func.h"

extern "C"
void crack_ext__cairo_init(crack::ext::Module *mod) {
    crack::ext::Func *f;
    crack::ext::Type *type_Class = mod->getClassType();

    crack::ext::Type *array = mod->getType("array");
    crack::ext::Type *type_bool = mod->getBoolType();
    crack::ext::Type *type_byte = mod->getByteType();
    crack::ext::Type *type_byteptr = mod->getByteptrType();
    crack::ext::Type *type_float = mod->getFloatType();
    crack::ext::Type *type_float32 = mod->getFloat32Type();
    crack::ext::Type *type_float64 = mod->getFloat64Type();
    crack::ext::Type *type_int = mod->getIntType();
    crack::ext::Type *type_int32 = mod->getInt32Type();
    crack::ext::Type *type_int64 = mod->getInt64Type();
    crack::ext::Type *type_uint = mod->getUintType();
    crack::ext::Type *type_uint32 = mod->getUint32Type();
    crack::ext::Type *type_uint64 = mod->getUint64Type();
    crack::ext::Type *type_void = mod->getVoidType();
    crack::ext::Type *type_voidptr = mod->getVoidptrType();

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
