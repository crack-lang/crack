#include <GL/gl.h>


#include "ext/Module.h"
#include "ext/Type.h"
#include "ext/Func.h"

extern "C"
void crack_ext__gl_rinit() {
    return;
}

extern "C"
void crack_ext__gl_cinit(crack::ext::Module *mod) {
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

    crack::ext::Type *array = mod->getType("array");

    crack::ext::Type *array_pfloat64_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_float64;
        array_pfloat64_q = array->getSpecialization(params);
    }

    crack::ext::Type *array_pfloat32_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_float32;
        array_pfloat32_q = array->getSpecialization(params);
    }

    crack::ext::Type *array_pint_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_int;
        array_pint_q = array->getSpecialization(params);
    }

    crack::ext::Type *array_pbyte_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_byte;
        array_pbyte_q = array->getSpecialization(params);
    }

    crack::ext::Type *array_puint_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_uint;
        array_puint_q = array->getSpecialization(params);
    }

    crack::ext::Type *array_pfloat_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_float;
        array_pfloat_q = array->getSpecialization(params);
    }
    f = mod->addFunc(type_uint, "glGetError",
                     (void *)glGetError
                     );

    f = mod->addFunc(type_void, "glMatrixMode",
                     (void *)glMatrixMode
                     );
       f->addArg(type_uint, "mode");

    f = mod->addFunc(type_void, "glLoadIdentity",
                     (void *)glLoadIdentity
                     );

    f = mod->addFunc(type_void, "glFrustum",
                     (void *)glFrustum
                     );
       f->addArg(type_float64, "left");
       f->addArg(type_float64, "right");
       f->addArg(type_float64, "bottom");
       f->addArg(type_float64, "top");
       f->addArg(type_float64, "nearVal");
       f->addArg(type_float64, "farVal");

    f = mod->addFunc(type_void, "glFlush",
                     (void *)glFlush
                     );

    f = mod->addFunc(type_void, "glMultMatrixd",
                     (void *)glMultMatrixd
                     );
       f->addArg(array_pfloat64_q, "matrix");

    f = mod->addFunc(type_void, "glMultMatrixf",
                     (void *)glMultMatrixf
                     );
       f->addArg(array_pfloat32_q, "matrix");

    f = mod->addFunc(type_void, "glTranslated",
                     (void *)glTranslated
                     );
       f->addArg(type_float64, "x");
       f->addArg(type_float64, "y");
       f->addArg(type_float64, "z");

    f = mod->addFunc(type_void, "glTranslatef",
                     (void *)glTranslatef
                     );
       f->addArg(type_float32, "x");
       f->addArg(type_float32, "y");
       f->addArg(type_float32, "z");

    f = mod->addFunc(type_void, "glBegin",
                     (void *)glBegin
                     );
       f->addArg(type_uint, "mode");

    f = mod->addFunc(type_void, "glEnd",
                     (void *)glEnd
                     );

    f = mod->addFunc(type_void, "glVertex2d",
                     (void *)glVertex2d
                     );
       f->addArg(type_float64, "x");
       f->addArg(type_float64, "y");

    f = mod->addFunc(type_void, "glVertex2f",
                     (void *)glVertex2f
                     );
       f->addArg(type_float32, "x");
       f->addArg(type_float32, "y");

    f = mod->addFunc(type_void, "glVertex2i",
                     (void *)glVertex2i
                     );
       f->addArg(type_int, "x");
       f->addArg(type_int, "y");

    f = mod->addFunc(type_void, "glVertex3d",
                     (void *)glVertex3d
                     );
       f->addArg(type_float64, "x");
       f->addArg(type_float64, "y");
       f->addArg(type_float64, "z");

    f = mod->addFunc(type_void, "glVertex3f",
                     (void *)glVertex3f
                     );
       f->addArg(type_float32, "x");
       f->addArg(type_float32, "y");
       f->addArg(type_float32, "z");

    f = mod->addFunc(type_void, "glVertex3i",
                     (void *)glVertex3i
                     );
       f->addArg(type_int, "x");
       f->addArg(type_int, "y");
       f->addArg(type_int, "z");

    f = mod->addFunc(type_void, "glVertex4d",
                     (void *)glVertex4d
                     );
       f->addArg(type_float64, "x");
       f->addArg(type_float64, "y");
       f->addArg(type_float64, "z");
       f->addArg(type_float64, "w");

    f = mod->addFunc(type_void, "glVertex4f",
                     (void *)glVertex4f
                     );
       f->addArg(type_float32, "x");
       f->addArg(type_float32, "y");
       f->addArg(type_float32, "z");
       f->addArg(type_float32, "w");

    f = mod->addFunc(type_void, "glVertex4i",
                     (void *)glVertex4i
                     );
       f->addArg(type_int, "x");
       f->addArg(type_int, "y");
       f->addArg(type_int, "z");
       f->addArg(type_int, "w");

    f = mod->addFunc(type_void, "glVertex2dv",
                     (void *)glVertex2dv
                     );
       f->addArg(array_pfloat64_q, "v");

    f = mod->addFunc(type_void, "glVertex2fv",
                     (void *)glVertex2fv
                     );
       f->addArg(array_pfloat32_q, "v");

    f = mod->addFunc(type_void, "glVertex2iv",
                     (void *)glVertex2iv
                     );
       f->addArg(array_pint_q, "v");

    f = mod->addFunc(type_void, "glVertex3dv",
                     (void *)glVertex3dv
                     );
       f->addArg(array_pfloat64_q, "v");

    f = mod->addFunc(type_void, "glVertex3fv",
                     (void *)glVertex3fv
                     );
       f->addArg(array_pfloat32_q, "v");

    f = mod->addFunc(type_void, "glVertex3iv",
                     (void *)glVertex3iv
                     );
       f->addArg(array_pint_q, "v");

    f = mod->addFunc(type_void, "glVertex4dv",
                     (void *)glVertex4dv
                     );
       f->addArg(array_pfloat64_q, "v");

    f = mod->addFunc(type_void, "glVertex4fv",
                     (void *)glVertex4fv
                     );
       f->addArg(array_pfloat32_q, "v");

    f = mod->addFunc(type_void, "glVertex4iv",
                     (void *)glVertex4iv
                     );
       f->addArg(array_pint_q, "v");

    f = mod->addFunc(type_void, "glNormal3b",
                     (void *)glNormal3b
                     );
       f->addArg(type_byte, "nx");
       f->addArg(type_byte, "ny");
       f->addArg(type_byte, "nz");

    f = mod->addFunc(type_void, "glNormal3d",
                     (void *)glNormal3d
                     );
       f->addArg(type_float64, "nx");
       f->addArg(type_float64, "ny");
       f->addArg(type_float64, "nz");

    f = mod->addFunc(type_void, "glNormal3f",
                     (void *)glNormal3f
                     );
       f->addArg(type_float32, "nx");
       f->addArg(type_float32, "ny");
       f->addArg(type_float32, "nz");

    f = mod->addFunc(type_void, "glNormal3i",
                     (void *)glNormal3i
                     );
       f->addArg(type_int, "nx");
       f->addArg(type_int, "ny");
       f->addArg(type_int, "nz");

    f = mod->addFunc(type_void, "glNormal3bv",
                     (void *)glNormal3bv
                     );
       f->addArg(array_pbyte_q, "v");

    f = mod->addFunc(type_void, "glNormal3dv",
                     (void *)glNormal3dv
                     );
       f->addArg(array_pfloat64_q, "v");

    f = mod->addFunc(type_void, "glNormal3fv",
                     (void *)glNormal3fv
                     );
       f->addArg(array_pfloat32_q, "v");

    f = mod->addFunc(type_void, "glNormal3iv",
                     (void *)glNormal3iv
                     );
       f->addArg(array_pint_q, "v");

    f = mod->addFunc(type_void, "glColor3b",
                     (void *)glColor3b
                     );
       f->addArg(type_byte, "red");
       f->addArg(type_byte, "green");
       f->addArg(type_byte, "blue");

    f = mod->addFunc(type_void, "glColor3d",
                     (void *)glColor3d
                     );
       f->addArg(type_float64, "red");
       f->addArg(type_float64, "green");
       f->addArg(type_float64, "blue");

    f = mod->addFunc(type_void, "glColor3f",
                     (void *)glColor3f
                     );
       f->addArg(type_float32, "red");
       f->addArg(type_float32, "green");
       f->addArg(type_float32, "blue");

    f = mod->addFunc(type_void, "glColor3i",
                     (void *)glColor3i
                     );
       f->addArg(type_int, "red");
       f->addArg(type_int, "green");
       f->addArg(type_int, "blue");

    f = mod->addFunc(type_void, "glColor3ui",
                     (void *)glColor3ui
                     );
       f->addArg(type_uint, "red");
       f->addArg(type_uint, "green");
       f->addArg(type_uint, "blue");

    f = mod->addFunc(type_void, "glColor4b",
                     (void *)glColor4b
                     );
       f->addArg(type_byte, "red");
       f->addArg(type_byte, "green");
       f->addArg(type_byte, "blue");
       f->addArg(type_byte, "alpha");

    f = mod->addFunc(type_void, "glColor4d",
                     (void *)glColor4d
                     );
       f->addArg(type_float64, "red");
       f->addArg(type_float64, "green");
       f->addArg(type_float64, "blue");
       f->addArg(type_float64, "alpha");

    f = mod->addFunc(type_void, "glColor4f",
                     (void *)glColor4f
                     );
       f->addArg(type_float32, "red");
       f->addArg(type_float32, "green");
       f->addArg(type_float32, "blue");
       f->addArg(type_float32, "alpha");

    f = mod->addFunc(type_void, "glColor4i",
                     (void *)glColor4i
                     );
       f->addArg(type_int, "red");
       f->addArg(type_int, "green");
       f->addArg(type_int, "blue");
       f->addArg(type_int, "alpha");

    f = mod->addFunc(type_void, "glColor4ui",
                     (void *)glColor4ui
                     );
       f->addArg(type_uint, "red");
       f->addArg(type_uint, "green");
       f->addArg(type_uint, "blue");
       f->addArg(type_uint, "alpha");

    f = mod->addFunc(type_void, "glColor3bv",
                     (void *)glColor3bv
                     );
       f->addArg(array_pbyte_q, "v");

    f = mod->addFunc(type_void, "glColor3dv",
                     (void *)glColor3dv
                     );
       f->addArg(array_pfloat64_q, "v");

    f = mod->addFunc(type_void, "glColor3fv",
                     (void *)glColor3fv
                     );
       f->addArg(array_pfloat32_q, "v");

    f = mod->addFunc(type_void, "glColor3iv",
                     (void *)glColor3iv
                     );
       f->addArg(array_pint_q, "v");

    f = mod->addFunc(type_void, "glColor3uiv",
                     (void *)glColor3uiv
                     );
       f->addArg(array_puint_q, "v");

    f = mod->addFunc(type_void, "glColor4bv",
                     (void *)glColor4bv
                     );
       f->addArg(array_pbyte_q, "v");

    f = mod->addFunc(type_void, "glColor4dv",
                     (void *)glColor4dv
                     );
       f->addArg(array_pfloat64_q, "v");

    f = mod->addFunc(type_void, "glColor4fv",
                     (void *)glColor4fv
                     );
       f->addArg(array_pfloat32_q, "v");

    f = mod->addFunc(type_void, "glColor4iv",
                     (void *)glColor4iv
                     );
       f->addArg(array_pint_q, "v");

    f = mod->addFunc(type_void, "glColor4uiv",
                     (void *)glColor4uiv
                     );
       f->addArg(array_puint_q, "v");

    f = mod->addFunc(type_void, "glViewport",
                     (void *)glViewport
                     );
       f->addArg(type_int, "x");
       f->addArg(type_int, "y");
       f->addArg(type_int, "width");
       f->addArg(type_int, "height");

    f = mod->addFunc(type_void, "glClearColor",
                     (void *)glClearColor
                     );
       f->addArg(type_float32, "red");
       f->addArg(type_float32, "green");
       f->addArg(type_float32, "blue");
       f->addArg(type_float32, "alpha");

    f = mod->addFunc(type_void, "glClearDepth",
                     (void *)glClearDepth
                     );
       f->addArg(type_float64, "depth");

    f = mod->addFunc(type_void, "glClear",
                     (void *)glClear
                     );
       f->addArg(type_uint, "mask");

    f = mod->addFunc(type_void, "glShadeModel",
                     (void *)glShadeModel
                     );
       f->addArg(type_uint, "mode");

    f = mod->addFunc(type_void, "glPolygonMode",
                     (void *)glPolygonMode
                     );
       f->addArg(type_uint, "face");
       f->addArg(type_uint, "mode");

    f = mod->addFunc(type_void, "glLightf",
                     (void *)glLightf
                     );
       f->addArg(type_uint, "light");
       f->addArg(type_uint, "pname");
       f->addArg(type_float32, "param");

    f = mod->addFunc(type_void, "glLighti",
                     (void *)glLighti
                     );
       f->addArg(type_uint, "light");
       f->addArg(type_uint, "pname");
       f->addArg(type_int, "param");

    f = mod->addFunc(type_void, "glLightfv",
                     (void *)glLightfv
                     );
       f->addArg(type_uint, "light");
       f->addArg(type_uint, "pname");
       f->addArg(array_pfloat32_q, "params");

    f = mod->addFunc(type_void, "glLightiv",
                     (void *)glLightiv
                     );
       f->addArg(type_uint, "light");
       f->addArg(type_uint, "pname");
       f->addArg(array_pint_q, "params");

    f = mod->addFunc(type_void, "glEnable",
                     (void *)glEnable
                     );
       f->addArg(type_uint, "cap");

    f = mod->addFunc(type_void, "glDisable",
                     (void *)glDisable
                     );
       f->addArg(type_uint, "cap");

    f = mod->addFunc(type_void, "glDepthFunc",
                     (void *)glDepthFunc
                     );
       f->addArg(type_uint, "func");

    f = mod->addFunc(type_void, "glDepthMask",
                     (void *)glDepthMask
                     );
       f->addArg(type_bool, "flag");

    f = mod->addFunc(type_void, "glHint",
                     (void *)glHint
                     );
       f->addArg(type_uint, "target");
       f->addArg(type_uint, "mode");

    f = mod->addFunc(type_void, "glGenTextures",
                     (void *)glGenTextures
                     );
       f->addArg(type_int, "n");
       f->addArg(array_puint_q, "textures");

    f = mod->addFunc(type_void, "glDeleteTextures",
                     (void *)glDeleteTextures
                     );
       f->addArg(type_int, "n");
       f->addArg(array_puint_q, "textures");

    f = mod->addFunc(type_void, "glBindTexture",
                     (void *)glBindTexture
                     );
       f->addArg(type_uint, "target");
       f->addArg(type_uint, "texture");

    f = mod->addFunc(type_void, "glTexImage2D",
                     (void *)glTexImage2D
                     );
       f->addArg(type_uint, "target");
       f->addArg(type_int, "level");
       f->addArg(type_int, "internalFormat");
       f->addArg(type_int, "width");
       f->addArg(type_int, "height");
       f->addArg(type_int, "border");
       f->addArg(type_uint, "format");
       f->addArg(type_uint, "type");
       f->addArg(type_voidptr, "pixels");

    f = mod->addFunc(type_void, "glTexParameterf",
                     (void *)glTexParameterf
                     );
       f->addArg(type_uint, "target");
       f->addArg(type_uint, "pname");
       f->addArg(type_float, "param");

    f = mod->addFunc(type_void, "glTexParameteri",
                     (void *)glTexParameteri
                     );
       f->addArg(type_uint, "target");
       f->addArg(type_uint, "pname");
       f->addArg(type_int, "param");

    f = mod->addFunc(type_void, "glTexParameterfv",
                     (void *)glTexParameterfv
                     );
       f->addArg(type_uint, "target");
       f->addArg(type_uint, "pname");
       f->addArg(array_pfloat_q, "params");

    f = mod->addFunc(type_void, "glTexParameteriv",
                     (void *)glTexParameteriv
                     );
       f->addArg(type_uint, "target");
       f->addArg(type_uint, "pname");
       f->addArg(array_pint_q, "params");

    f = mod->addFunc(type_void, "glGetTexParameterfv",
                     (void *)glGetTexParameterfv
                     );
       f->addArg(type_uint, "target");
       f->addArg(type_uint, "pname");
       f->addArg(array_pfloat_q, "params");

    f = mod->addFunc(type_void, "glGetTexParameteriv",
                     (void *)glGetTexParameteriv
                     );
       f->addArg(type_uint, "target");
       f->addArg(type_uint, "pname");
       f->addArg(array_pint_q, "params");

    f = mod->addFunc(type_void, "glGetTexLevelParameterfv",
                     (void *)glGetTexLevelParameterfv
                     );
       f->addArg(type_uint, "target");
       f->addArg(type_int, "level");
       f->addArg(type_uint, "pname");
       f->addArg(array_pfloat_q, "params");

    f = mod->addFunc(type_void, "glGetTexLevelParameteriv",
                     (void *)glGetTexLevelParameteriv
                     );
       f->addArg(type_uint, "target");
       f->addArg(type_int, "level");
       f->addArg(type_uint, "pname");
       f->addArg(array_pint_q, "params");

    f = mod->addFunc(type_void, "glTexCoord1d",
                     (void *)glTexCoord1d
                     );
       f->addArg(type_float64, "s");

    f = mod->addFunc(type_void, "glTexCoord1f",
                     (void *)glTexCoord1f
                     );
       f->addArg(type_float32, "s");

    f = mod->addFunc(type_void, "glTexCoord1i",
                     (void *)glTexCoord1i
                     );
       f->addArg(type_int, "s");

    f = mod->addFunc(type_void, "glTexCoord2d",
                     (void *)glTexCoord2d
                     );
       f->addArg(type_float64, "s");
       f->addArg(type_float64, "t");

    f = mod->addFunc(type_void, "glTexCoord2f",
                     (void *)glTexCoord2f
                     );
       f->addArg(type_float32, "s");
       f->addArg(type_float32, "t");

    f = mod->addFunc(type_void, "glTexCoord2i",
                     (void *)glTexCoord2i
                     );
       f->addArg(type_int, "s");
       f->addArg(type_int, "t");

    f = mod->addFunc(type_void, "glTexCoord3d",
                     (void *)glTexCoord3d
                     );
       f->addArg(type_float64, "s");
       f->addArg(type_float64, "t");
       f->addArg(type_float64, "r");

    f = mod->addFunc(type_void, "glTexCoord3f",
                     (void *)glTexCoord3f
                     );
       f->addArg(type_float32, "s");
       f->addArg(type_float32, "t");
       f->addArg(type_float32, "r");

    f = mod->addFunc(type_void, "glTexCoord3i",
                     (void *)glTexCoord3i
                     );
       f->addArg(type_int, "s");
       f->addArg(type_int, "t");
       f->addArg(type_int, "r");

    f = mod->addFunc(type_void, "glTexCoord4d",
                     (void *)glTexCoord4d
                     );
       f->addArg(type_float64, "s");
       f->addArg(type_float64, "t");
       f->addArg(type_float64, "r");
       f->addArg(type_float64, "q");

    f = mod->addFunc(type_void, "glTexCoord4f",
                     (void *)glTexCoord4f
                     );
       f->addArg(type_float32, "s");
       f->addArg(type_float32, "t");
       f->addArg(type_float32, "r");
       f->addArg(type_float32, "q");

    f = mod->addFunc(type_void, "glTexCoord4i",
                     (void *)glTexCoord4i
                     );
       f->addArg(type_int, "s");
       f->addArg(type_int, "t");
       f->addArg(type_int, "r");
       f->addArg(type_int, "q");

    f = mod->addFunc(type_void, "glTexCoord1dv",
                     (void *)glTexCoord1dv
                     );
       f->addArg(array_pfloat64_q, "v");

    f = mod->addFunc(type_void, "glTexCoord1fv",
                     (void *)glTexCoord1fv
                     );
       f->addArg(array_pfloat32_q, "v");

    f = mod->addFunc(type_void, "glTexCoord1iv",
                     (void *)glTexCoord1iv
                     );
       f->addArg(array_pint_q, "v");

    f = mod->addFunc(type_void, "glTexCoord2dv",
                     (void *)glTexCoord2dv
                     );
       f->addArg(array_pfloat64_q, "v");

    f = mod->addFunc(type_void, "glTexCoord2fv",
                     (void *)glTexCoord2fv
                     );
       f->addArg(array_pfloat32_q, "v");

    f = mod->addFunc(type_void, "glTexCoord2iv",
                     (void *)glTexCoord2iv
                     );
       f->addArg(array_pint_q, "v");

    f = mod->addFunc(type_void, "glTexCoord3dv",
                     (void *)glTexCoord3dv
                     );
       f->addArg(array_pfloat64_q, "v");

    f = mod->addFunc(type_void, "glTexCoord3fv",
                     (void *)glTexCoord3fv
                     );
       f->addArg(array_pfloat32_q, "v");

    f = mod->addFunc(type_void, "glTexCoord3iv",
                     (void *)glTexCoord3iv
                     );
       f->addArg(array_pint_q, "v");

    f = mod->addFunc(type_void, "glTexCoord4dv",
                     (void *)glTexCoord4dv
                     );
       f->addArg(array_pfloat64_q, "v");

    f = mod->addFunc(type_void, "glTexCoord4fv",
                     (void *)glTexCoord4fv
                     );
       f->addArg(array_pfloat32_q, "v");

    f = mod->addFunc(type_void, "glTexCoord4iv",
                     (void *)glTexCoord4iv
                     );
       f->addArg(array_pint_q, "v");

    f = mod->addFunc(type_void, "glBlendFunc",
                     (void *)glBlendFunc
                     );
       f->addArg(type_uint, "sfactor");
       f->addArg(type_uint, "dfactor");


    mod->addConstant(type_uint, "GL_PROJECTION",
                     static_cast<int>(GL_PROJECTION)
                     );

    mod->addConstant(type_uint, "GL_MODELVIEW",
                     static_cast<int>(GL_MODELVIEW)
                     );

    mod->addConstant(type_uint, "GL_BYTE",
                     static_cast<int>(GL_BYTE)
                     );

    mod->addConstant(type_uint, "GL_UNSIGNED_BYTE",
                     static_cast<int>(GL_UNSIGNED_BYTE)
                     );

    mod->addConstant(type_uint, "GL_SHORT",
                     static_cast<int>(GL_SHORT)
                     );

    mod->addConstant(type_uint, "GL_UNSIGNED_SHORT",
                     static_cast<int>(GL_UNSIGNED_SHORT)
                     );

    mod->addConstant(type_uint, "GL_INT",
                     static_cast<int>(GL_INT)
                     );

    mod->addConstant(type_uint, "GL_UNSIGNED_INT",
                     static_cast<int>(GL_UNSIGNED_INT)
                     );

    mod->addConstant(type_uint, "GL_FLOAT",
                     static_cast<int>(GL_FLOAT)
                     );

    mod->addConstant(type_uint, "GL_2_BYTES",
                     static_cast<int>(GL_2_BYTES)
                     );

    mod->addConstant(type_uint, "GL_3_BYTES",
                     static_cast<int>(GL_3_BYTES)
                     );

    mod->addConstant(type_uint, "GL_4_BYTES",
                     static_cast<int>(GL_4_BYTES)
                     );

    mod->addConstant(type_uint, "GL_DOUBLE",
                     static_cast<int>(GL_DOUBLE)
                     );

    mod->addConstant(type_uint, "GL_POINTS",
                     static_cast<int>(GL_POINTS)
                     );

    mod->addConstant(type_uint, "GL_LINES",
                     static_cast<int>(GL_LINES)
                     );

    mod->addConstant(type_uint, "GL_LINE_LOOP",
                     static_cast<int>(GL_LINE_LOOP)
                     );

    mod->addConstant(type_uint, "GL_LINE_STRIP",
                     static_cast<int>(GL_LINE_STRIP)
                     );

    mod->addConstant(type_uint, "GL_TRIANGLES",
                     static_cast<int>(GL_TRIANGLES)
                     );

    mod->addConstant(type_uint, "GL_TRIANGLE_STRIP",
                     static_cast<int>(GL_TRIANGLE_STRIP)
                     );

    mod->addConstant(type_uint, "GL_TRIANGLE_FAN",
                     static_cast<int>(GL_TRIANGLE_FAN)
                     );

    mod->addConstant(type_uint, "GL_QUADS",
                     static_cast<int>(GL_QUADS)
                     );

    mod->addConstant(type_uint, "GL_QUAD_STRIP",
                     static_cast<int>(GL_QUAD_STRIP)
                     );

    mod->addConstant(type_uint, "GL_POLYGON",
                     static_cast<int>(GL_POLYGON)
                     );

    mod->addConstant(type_uint, "GL_CURRENT_BIT",
                     static_cast<int>(GL_CURRENT_BIT)
                     );

    mod->addConstant(type_uint, "GL_POINT_BIT",
                     static_cast<int>(GL_POINT_BIT)
                     );

    mod->addConstant(type_uint, "GL_LINE_BIT",
                     static_cast<int>(GL_LINE_BIT)
                     );

    mod->addConstant(type_uint, "GL_POLYGON_BIT",
                     static_cast<int>(GL_POLYGON_BIT)
                     );

    mod->addConstant(type_uint, "GL_POLYGON_STIPPLE_BIT",
                     static_cast<int>(GL_POLYGON_STIPPLE_BIT)
                     );

    mod->addConstant(type_uint, "GL_PIXEL_MODE_BIT",
                     static_cast<int>(GL_PIXEL_MODE_BIT)
                     );

    mod->addConstant(type_uint, "GL_LIGHTING_BIT",
                     static_cast<int>(GL_LIGHTING_BIT)
                     );

    mod->addConstant(type_uint, "GL_FOG_BIT",
                     static_cast<int>(GL_FOG_BIT)
                     );

    mod->addConstant(type_uint, "GL_DEPTH_BUFFER_BIT",
                     static_cast<int>(GL_DEPTH_BUFFER_BIT)
                     );

    mod->addConstant(type_uint, "GL_ACCUM_BUFFER_BIT",
                     static_cast<int>(GL_ACCUM_BUFFER_BIT)
                     );

    mod->addConstant(type_uint, "GL_STENCIL_BUFFER_BIT",
                     static_cast<int>(GL_STENCIL_BUFFER_BIT)
                     );

    mod->addConstant(type_uint, "GL_VIEWPORT_BIT",
                     static_cast<int>(GL_VIEWPORT_BIT)
                     );

    mod->addConstant(type_uint, "GL_TRANSFORM_BIT",
                     static_cast<int>(GL_TRANSFORM_BIT)
                     );

    mod->addConstant(type_uint, "GL_ENABLE_BIT",
                     static_cast<int>(GL_ENABLE_BIT)
                     );

    mod->addConstant(type_uint, "GL_COLOR_BUFFER_BIT",
                     static_cast<int>(GL_COLOR_BUFFER_BIT)
                     );

    mod->addConstant(type_uint, "GL_HINT_BIT",
                     static_cast<int>(GL_HINT_BIT)
                     );

    mod->addConstant(type_uint, "GL_EVAL_BIT",
                     static_cast<int>(GL_EVAL_BIT)
                     );

    mod->addConstant(type_uint, "GL_LIST_BIT",
                     static_cast<int>(GL_LIST_BIT)
                     );

    mod->addConstant(type_uint, "GL_TEXTURE_BIT",
                     static_cast<int>(GL_TEXTURE_BIT)
                     );

    mod->addConstant(type_uint, "GL_SCISSOR_BIT",
                     static_cast<int>(GL_SCISSOR_BIT)
                     );

    mod->addConstant(type_uint, "GL_ALL_ATTRIB_BITS",
                     static_cast<int>(GL_ALL_ATTRIB_BITS)
                     );

    mod->addConstant(type_uint, "GL_FILL",
                     static_cast<int>(GL_FILL)
                     );

    mod->addConstant(type_uint, "GL_SMOOTH",
                     static_cast<int>(GL_SMOOTH)
                     );

    mod->addConstant(type_uint, "GL_FRONT_AND_BACK",
                     static_cast<int>(GL_FRONT_AND_BACK)
                     );

    mod->addConstant(type_uint, "GL_LIGHTING",
                     static_cast<int>(GL_LIGHTING)
                     );

    mod->addConstant(type_uint, "GL_LIGHT0",
                     static_cast<int>(GL_LIGHT0)
                     );

    mod->addConstant(type_uint, "GL_LIGHT1",
                     static_cast<int>(GL_LIGHT1)
                     );

    mod->addConstant(type_uint, "GL_LIGHT2",
                     static_cast<int>(GL_LIGHT2)
                     );

    mod->addConstant(type_uint, "GL_LIGHT3",
                     static_cast<int>(GL_LIGHT3)
                     );

    mod->addConstant(type_uint, "GL_LIGHT4",
                     static_cast<int>(GL_LIGHT4)
                     );

    mod->addConstant(type_uint, "GL_LIGHT5",
                     static_cast<int>(GL_LIGHT5)
                     );

    mod->addConstant(type_uint, "GL_LIGHT6",
                     static_cast<int>(GL_LIGHT6)
                     );

    mod->addConstant(type_uint, "GL_LIGHT7",
                     static_cast<int>(GL_LIGHT7)
                     );

    mod->addConstant(type_uint, "GL_SPOT_EXPONENT",
                     static_cast<int>(GL_SPOT_EXPONENT)
                     );

    mod->addConstant(type_uint, "GL_SPOT_CUTOFF",
                     static_cast<int>(GL_SPOT_CUTOFF)
                     );

    mod->addConstant(type_uint, "GL_CONSTANT_ATTENUATION",
                     static_cast<int>(GL_CONSTANT_ATTENUATION)
                     );

    mod->addConstant(type_uint, "GL_LINEAR_ATTENUATION",
                     static_cast<int>(GL_LINEAR_ATTENUATION)
                     );

    mod->addConstant(type_uint, "GL_QUADRATIC_ATTENUATION",
                     static_cast<int>(GL_QUADRATIC_ATTENUATION)
                     );

    mod->addConstant(type_uint, "GL_AMBIENT",
                     static_cast<int>(GL_AMBIENT)
                     );

    mod->addConstant(type_uint, "GL_DIFFUSE",
                     static_cast<int>(GL_DIFFUSE)
                     );

    mod->addConstant(type_uint, "GL_SPECULAR",
                     static_cast<int>(GL_SPECULAR)
                     );

    mod->addConstant(type_uint, "GL_SHININESS",
                     static_cast<int>(GL_SHININESS)
                     );

    mod->addConstant(type_uint, "GL_EMISSION",
                     static_cast<int>(GL_EMISSION)
                     );

    mod->addConstant(type_uint, "GL_POSITION",
                     static_cast<int>(GL_POSITION)
                     );

    mod->addConstant(type_uint, "GL_SPOT_DIRECTION",
                     static_cast<int>(GL_SPOT_DIRECTION)
                     );

    mod->addConstant(type_uint, "GL_AMBIENT_AND_DIFFUSE",
                     static_cast<int>(GL_AMBIENT_AND_DIFFUSE)
                     );

    mod->addConstant(type_uint, "GL_COLOR_INDEXES",
                     static_cast<int>(GL_COLOR_INDEXES)
                     );

    mod->addConstant(type_uint, "GL_LIGHT_MODEL_TWO_SIDE",
                     static_cast<int>(GL_LIGHT_MODEL_TWO_SIDE)
                     );

    mod->addConstant(type_uint, "GL_LIGHT_MODEL_LOCAL_VIEWER",
                     static_cast<int>(GL_LIGHT_MODEL_LOCAL_VIEWER)
                     );

    mod->addConstant(type_uint, "GL_LIGHT_MODEL_AMBIENT",
                     static_cast<int>(GL_LIGHT_MODEL_AMBIENT)
                     );

    mod->addConstant(type_uint, "GL_FRONT_AND_BACK",
                     static_cast<int>(GL_FRONT_AND_BACK)
                     );

    mod->addConstant(type_uint, "GL_SHADE_MODEL",
                     static_cast<int>(GL_SHADE_MODEL)
                     );

    mod->addConstant(type_uint, "GL_FLAT",
                     static_cast<int>(GL_FLAT)
                     );

    mod->addConstant(type_uint, "GL_SMOOTH",
                     static_cast<int>(GL_SMOOTH)
                     );

    mod->addConstant(type_uint, "GL_COLOR_MATERIAL",
                     static_cast<int>(GL_COLOR_MATERIAL)
                     );

    mod->addConstant(type_uint, "GL_COLOR_MATERIAL_FACE",
                     static_cast<int>(GL_COLOR_MATERIAL_FACE)
                     );

    mod->addConstant(type_uint, "GL_COLOR_MATERIAL_PARAMETER",
                     static_cast<int>(GL_COLOR_MATERIAL_PARAMETER)
                     );

    mod->addConstant(type_uint, "GL_NORMALIZE",
                     static_cast<int>(GL_NORMALIZE)
                     );

    mod->addConstant(type_uint, "GL_DEPTH_TEST",
                     static_cast<int>(GL_DEPTH_TEST)
                     );

    mod->addConstant(type_uint, "GL_TEXTURE_2D",
                     static_cast<int>(GL_TEXTURE_2D)
                     );

    mod->addConstant(type_uint, "GL_LIGHTING",
                     static_cast<int>(GL_LIGHTING)
                     );

    mod->addConstant(type_uint, "GL_LEQUAL",
                     static_cast<int>(GL_LEQUAL)
                     );

    mod->addConstant(type_uint, "GL_PERSPECTIVE_CORRECTION_HINT",
                     static_cast<int>(GL_PERSPECTIVE_CORRECTION_HINT)
                     );

    mod->addConstant(type_uint, "GL_POINT_SMOOTH_HINT",
                     static_cast<int>(GL_POINT_SMOOTH_HINT)
                     );

    mod->addConstant(type_uint, "GL_LINE_SMOOTH_HINT",
                     static_cast<int>(GL_LINE_SMOOTH_HINT)
                     );

    mod->addConstant(type_uint, "GL_POLYGON_SMOOTH_HINT",
                     static_cast<int>(GL_POLYGON_SMOOTH_HINT)
                     );

    mod->addConstant(type_uint, "GL_FOG_HINT",
                     static_cast<int>(GL_FOG_HINT)
                     );

    mod->addConstant(type_uint, "GL_DONT_CARE",
                     static_cast<int>(GL_DONT_CARE)
                     );

    mod->addConstant(type_uint, "GL_FASTEST",
                     static_cast<int>(GL_FASTEST)
                     );

    mod->addConstant(type_uint, "GL_NICEST",
                     static_cast<int>(GL_NICEST)
                     );

    mod->addConstant(type_uint, "GL_FOG",
                     static_cast<int>(GL_FOG)
                     );

    mod->addConstant(type_uint, "GL_FOG_MODE",
                     static_cast<int>(GL_FOG_MODE)
                     );

    mod->addConstant(type_uint, "GL_FOG_DENSITY",
                     static_cast<int>(GL_FOG_DENSITY)
                     );

    mod->addConstant(type_uint, "GL_FOG_COLOR",
                     static_cast<int>(GL_FOG_COLOR)
                     );

    mod->addConstant(type_uint, "GL_FOG_INDEX",
                     static_cast<int>(GL_FOG_INDEX)
                     );

    mod->addConstant(type_uint, "GL_FOG_START",
                     static_cast<int>(GL_FOG_START)
                     );

    mod->addConstant(type_uint, "GL_FOG_END",
                     static_cast<int>(GL_FOG_END)
                     );

    mod->addConstant(type_uint, "GL_LINEAR",
                     static_cast<int>(GL_LINEAR)
                     );

    mod->addConstant(type_uint, "GL_EXP",
                     static_cast<int>(GL_EXP)
                     );

    mod->addConstant(type_uint, "GL_EXP2",
                     static_cast<int>(GL_EXP2)
                     );

    mod->addConstant(type_uint, "GL_TEXTURE_MIN_FILTER",
                     static_cast<int>(GL_TEXTURE_MIN_FILTER)
                     );

    mod->addConstant(type_uint, "GL_TEXTURE_MAG_FILTER",
                     static_cast<int>(GL_TEXTURE_MAG_FILTER)
                     );

    mod->addConstant(type_uint, "GL_RGB",
                     static_cast<int>(GL_RGB)
                     );

    mod->addConstant(type_uint, "GL_RGBA",
                     static_cast<int>(GL_RGBA)
                     );

    mod->addConstant(type_uint, "GL_RGB8",
                     static_cast<int>(GL_RGB8)
                     );

    mod->addConstant(type_uint, "GL_RGBA8",
                     static_cast<int>(GL_RGBA8)
                     );

    mod->addConstant(type_uint, "GL_BLEND",
                     static_cast<int>(GL_BLEND)
                     );

    mod->addConstant(type_uint, "GL_BLEND_SRC",
                     static_cast<int>(GL_BLEND_SRC)
                     );

    mod->addConstant(type_uint, "GL_BLEND_DST",
                     static_cast<int>(GL_BLEND_DST)
                     );

    mod->addConstant(type_uint, "GL_ZERO",
                     static_cast<int>(GL_ZERO)
                     );

    mod->addConstant(type_uint, "GL_ONE",
                     static_cast<int>(GL_ONE)
                     );

    mod->addConstant(type_uint, "GL_SRC_COLOR",
                     static_cast<int>(GL_SRC_COLOR)
                     );

    mod->addConstant(type_uint, "GL_ONE_MINUS_SRC_COLOR",
                     static_cast<int>(GL_ONE_MINUS_SRC_COLOR)
                     );

    mod->addConstant(type_uint, "GL_SRC_ALPHA",
                     static_cast<int>(GL_SRC_ALPHA)
                     );

    mod->addConstant(type_uint, "GL_ONE_MINUS_SRC_ALPHA",
                     static_cast<int>(GL_ONE_MINUS_SRC_ALPHA)
                     );

    mod->addConstant(type_uint, "GL_DST_ALPHA",
                     static_cast<int>(GL_DST_ALPHA)
                     );

    mod->addConstant(type_uint, "GL_ONE_MINUS_DST_ALPHA",
                     static_cast<int>(GL_ONE_MINUS_DST_ALPHA)
                     );

    mod->addConstant(type_uint, "GL_DST_COLOR",
                     static_cast<int>(GL_DST_COLOR)
                     );

    mod->addConstant(type_uint, "GL_ONE_MINUS_DST_COLOR",
                     static_cast<int>(GL_ONE_MINUS_DST_COLOR)
                     );

    mod->addConstant(type_uint, "GL_SRC_ALPHA_SATURATE",
                     static_cast<int>(GL_SRC_ALPHA_SATURATE)
                     );
}
