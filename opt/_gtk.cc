#include <gtk/gtk.h>
GtkObject *GtkObject_cast(GtkWidget *widget) {
    return GTK_OBJECT(widget);
}


#include "ext/Module.h"
#include "ext/Type.h"
#include "ext/Func.h"

extern "C"
void crack_ext__gtk_rinit() {
    return;
}

extern "C"
void crack_ext__gtk_cinit(crack::ext::Module *mod) {
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

    crack::ext::Type *type_GList = mod->addType("GList", sizeof(GList));
    type_GList->finish();


    crack::ext::Type *type_GtkWidget = mod->addType("GtkWidget", sizeof(GtkWidget));
    type_GtkWidget->finish();


    crack::ext::Type *type_GtkObject = mod->addType("GtkObject", sizeof(GtkObject));
    type_GtkObject->finish();


    crack::ext::Type *array = mod->getType("array");

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

    crack::ext::Type *array_parray_pbyteptr_q_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = array_pbyteptr_q;
        array_parray_pbyteptr_q_q = array->getSpecialization(params);
    }
    f = mod->addFunc(type_GList, "g_list_append",
                     (void *)g_list_append
                     );
       f->addArg(type_GList, "list");
       f->addArg(type_voidptr, "data");

    f = mod->addFunc(type_void, "g_print",
                     (void *)g_print
                     );
       f->addArg(type_byteptr, "b");

    f = mod->addFunc(type_void, "gtk_init",
                     (void *)gtk_init
                     );
       f->addArg(array_pint_q, "argc");
       f->addArg(array_parray_pbyteptr_q_q, "argv");

    f = mod->addFunc(type_void, "gtk_widget_destroy",
                     (void *)gtk_widget_destroy
                     );
       f->addArg(type_GtkWidget, "widget");

    f = mod->addFunc(type_void, "gtk_widget_show",
                     (void *)gtk_widget_show
                     );
       f->addArg(type_GtkWidget, "widget");

    f = mod->addFunc(type_void, "gtk_object_destroy",
                     (void *)gtk_object_destroy
                     );
       f->addArg(type_GtkObject, "object");

    f = mod->addFunc(type_void, "gtk_main",
                     (void *)gtk_main
                     );

    f = mod->addFunc(type_void, "gtk_main_quit",
                     (void *)gtk_main_quit
                     );

    f = mod->addFunc(type_void, "g_signal_connect_data",
                     (void *)g_signal_connect_data
                     );
       f->addArg(type_GtkObject, "widget");
       f->addArg(type_byteptr, "signal");
       f->addArg(type_voidptr, "callback");
       f->addArg(type_voidptr, "callbackArg");
       f->addArg(type_voidptr, "destroy_data");
       f->addArg(type_uint, "connect_flags");

    f = mod->addFunc(type_GtkWidget, "gtk_window_new",
                     (void *)gtk_window_new
                     );
       f->addArg(type_int, "val");

    f = mod->addFunc(type_void, "gtk_box_pack_start",
                     (void *)gtk_box_pack_start
                     );
       f->addArg(type_GtkWidget, "box");
       f->addArg(type_GtkWidget, "child");
       f->addArg(type_bool, "expand");
       f->addArg(type_bool, "fill");
       f->addArg(type_uint, "padding");

    f = mod->addFunc(type_GtkWidget, "gtk_button_new_with_label",
                     (void *)gtk_button_new_with_label
                     );
       f->addArg(type_byteptr, "label");

    f = mod->addFunc(type_void, "gtk_container_add",
                     (void *)gtk_container_add
                     );
       f->addArg(type_GtkWidget, "container");
       f->addArg(type_GtkWidget, "widget");

    f = mod->addFunc(type_void, "gtk_editable_select_region",
                     (void *)gtk_editable_select_region
                     );
       f->addArg(type_GtkWidget, "entry");
       f->addArg(type_int, "start");
       f->addArg(type_int, "end");

    f = mod->addFunc(type_void, "gtk_editable_set_editable",
                     (void *)gtk_editable_set_editable
                     );
       f->addArg(type_GtkWidget, "entry");
       f->addArg(type_bool, "editable");

    f = mod->addFunc(type_byteptr, "gtk_entry_get_text",
                     (void *)gtk_entry_get_text
                     );
       f->addArg(type_GtkWidget, "entry");

    f = mod->addFunc(type_GtkWidget, "gtk_entry_new",
                     (void *)gtk_entry_new
                     );

    f = mod->addFunc(type_void, "gtk_entry_set_text",
                     (void *)gtk_entry_set_text
                     );
       f->addArg(type_GtkWidget, "entry");
       f->addArg(type_byteptr, "text");

    f = mod->addFunc(type_void, "gtk_entry_set_visibility",
                     (void *)gtk_entry_set_visibility
                     );
       f->addArg(type_GtkWidget, "entry");
       f->addArg(type_bool, "visible");

    f = mod->addFunc(type_GtkWidget, "gtk_hbox_new",
                     (void *)gtk_hbox_new
                     );
       f->addArg(type_bool, "homogenous");
       f->addArg(type_uint, "spacing");

    f = mod->addFunc(type_GtkWidget, "gtk_label_new",
                     (void *)gtk_label_new
                     );
       f->addArg(type_byteptr, "text");

    f = mod->addFunc(type_GtkObject, "gtk_tooltips_new",
                     (void *)gtk_tooltips_new
                     );

    f = mod->addFunc(type_void, "gtk_tooltips_set_tip",
                     (void *)gtk_tooltips_set_tip
                     );
       f->addArg(type_GtkObject, "tooltips");
       f->addArg(type_GtkWidget, "widget");
       f->addArg(type_byteptr, "tip_text");
       f->addArg(type_byteptr, "tip_private");

    f = mod->addFunc(type_GtkWidget, "gtk_vbox_new",
                     (void *)gtk_vbox_new
                     );
       f->addArg(type_bool, "homogenous");
       f->addArg(type_uint, "spacing");

    f = mod->addFunc(type_GtkObject, "GtkObject_cast",
                     (void *)GtkObject_cast
                     );
       f->addArg(type_GtkWidget, "widget");

}
