#include <gtk/gtk.h>
GtkObject *GtkObject_cast(GtkWidget *widget) {
    return GTK_OBJECT(widget);
}


#include "ext/Module.h"
#include "ext/Type.h"
#include "ext/Func.h"

extern "C"
void crack_ext__gtk_init(crack::ext::Module *mod) {
    crack::ext::Func *f;
    crack::ext::Type *type_GList = mod->addType("GList");
    type_GList->finish();

    crack::ext::Type *type_GtkObject = mod->addType("GtkObject");
    type_GtkObject->finish();

    crack::ext::Type *type_GtkWidget = mod->addType("GtkWidget");
    type_GtkWidget->finish();

    f = mod->addFunc(type_GList, "g_list_append",
                     (void *)g_list_append
                     );
    f->addArg(type_GList, "list");
    f->addArg(mod->getVoidptrType(), "data");

    f = mod->addFunc(mod->getVoidType(), "g_print",
                     (void *)g_print
                     );
    f->addArg(mod->getByteptrType(), "b");

    f = mod->addFunc(mod->getVoidType(), "gtk_init",
                     (void *)gtk_init
                     );
    f->addArg(mod->getVoidptrType(), "argc");
    f->addArg(mod->getVoidptrType(), "argv");

    f = mod->addFunc(mod->getVoidType(), "gtk_widget_destroy",
                     (void *)gtk_widget_destroy
                     );
    f->addArg(type_GtkWidget, "widget");

    f = mod->addFunc(mod->getVoidType(), "gtk_widget_show",
                     (void *)gtk_widget_show
                     );
    f->addArg(type_GtkWidget, "widget");

    f = mod->addFunc(mod->getVoidType(), "gtk_object_destroy",
                     (void *)gtk_object_destroy
                     );
    f->addArg(type_GtkObject, "object");

    f = mod->addFunc(mod->getVoidType(), "gtk_main",
                     (void *)gtk_main
                     );

    f = mod->addFunc(mod->getVoidType(), "gtk_main_quit",
                     (void *)gtk_main_quit
                     );

    f = mod->addFunc(mod->getVoidType(), "g_signal_connect_data",
                     (void *)g_signal_connect_data
                     );
    f->addArg(type_GtkObject, "widget");
    f->addArg(mod->getByteptrType(), "signal");
    f->addArg(mod->getVoidptrType(), "callback");
    f->addArg(mod->getVoidptrType(), "callbackArg");
    f->addArg(mod->getVoidptrType(), "destroy_data");
    f->addArg(mod->getUintType(), "connect_flags");

    f = mod->addFunc(type_GtkWidget, "gtk_window_new",
                     (void *)gtk_window_new
                     );
    f->addArg(mod->getIntType(), "val");

    f = mod->addFunc(mod->getVoidType(), "gtk_box_pack_start",
                     (void *)gtk_box_pack_start
                     );
    f->addArg(type_GtkWidget, "box");
    f->addArg(type_GtkWidget, "child");
    f->addArg(mod->getBoolType(), "expand");
    f->addArg(mod->getBoolType(), "fill");
    f->addArg(mod->getUintType(), "padding");

    f = mod->addFunc(type_GtkWidget, "gtk_button_new_with_label",
                     (void *)gtk_button_new_with_label
                     );
    f->addArg(mod->getByteptrType(), "label");

    f = mod->addFunc(mod->getVoidType(), "gtk_container_add",
                     (void *)gtk_container_add
                     );
    f->addArg(type_GtkWidget, "container");
    f->addArg(type_GtkWidget, "widget");

    f = mod->addFunc(mod->getVoidType(), "gtk_editable_select_region",
                     (void *)gtk_editable_select_region
                     );
    f->addArg(type_GtkWidget, "entry");
    f->addArg(mod->getIntType(), "start");
    f->addArg(mod->getIntType(), "end");

    f = mod->addFunc(mod->getVoidType(), "gtk_editable_set_editable",
                     (void *)gtk_editable_set_editable
                     );
    f->addArg(type_GtkWidget, "entry");
    f->addArg(mod->getBoolType(), "editable");

    f = mod->addFunc(mod->getByteptrType(), "gtk_entry_get_text",
                     (void *)gtk_entry_get_text
                     );
    f->addArg(type_GtkWidget, "entry");

    f = mod->addFunc(type_GtkWidget, "gtk_entry_new",
                     (void *)gtk_entry_new
                     );

    f = mod->addFunc(mod->getVoidType(), "gtk_entry_set_text",
                     (void *)gtk_entry_set_text
                     );
    f->addArg(type_GtkWidget, "entry");
    f->addArg(mod->getByteptrType(), "text");

    f = mod->addFunc(mod->getVoidType(), "gtk_entry_set_visibility",
                     (void *)gtk_entry_set_visibility
                     );
    f->addArg(type_GtkWidget, "entry");
    f->addArg(mod->getBoolType(), "visible");

    f = mod->addFunc(type_GtkWidget, "gtk_hbox_new",
                     (void *)gtk_hbox_new
                     );
    f->addArg(mod->getBoolType(), "homogenous");
    f->addArg(mod->getUintType(), "spacing");

    f = mod->addFunc(type_GtkWidget, "gtk_label_new",
                     (void *)gtk_label_new
                     );
    f->addArg(mod->getByteptrType(), "text");

    f = mod->addFunc(type_GtkObject, "gtk_tooltips_new",
                     (void *)gtk_tooltips_new
                     );

    f = mod->addFunc(mod->getVoidType(), "gtk_tooltips_set_tip",
                     (void *)gtk_tooltips_set_tip
                     );
    f->addArg(type_GtkObject, "tooltips");
    f->addArg(type_GtkWidget, "widget");
    f->addArg(mod->getByteptrType(), "tip_text");
    f->addArg(mod->getByteptrType(), "tip_private");

    f = mod->addFunc(type_GtkWidget, "gtk_vbox_new",
                     (void *)gtk_vbox_new
                     );
    f->addArg(mod->getBoolType(), "homogenous");
    f->addArg(mod->getUintType(), "spacing");

    f = mod->addFunc(type_GtkObject, "GtkObject_cast",
                     (void *)GtkObject_cast
                     );
    f->addArg(type_GtkWidget, "widget");
}
