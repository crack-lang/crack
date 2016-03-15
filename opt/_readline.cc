#include <readline/readline.h>
#include <readline/history.h>


#include "ext/Module.h"
#include "ext/Type.h"
#include "ext/Func.h"

extern "C"
void crack_ext__readline_rinit() {
    return;
}

extern "C"
void crack_ext__readline_cinit(crack::ext::Module *mod) {
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

    crack::ext::Type *type_HistData = mod->addType("HistData", sizeof(histdata_t));
    type_HistData->finish();


    crack::ext::Type *type_HistEntry = mod->addType("HistEntry", sizeof(HIST_ENTRY));
        type_HistEntry->addInstVar(type_byteptr, "line",
                                CRACK_OFFSET(HIST_ENTRY, line));
        type_HistEntry->addInstVar(type_byteptr, "timestamp",
                                CRACK_OFFSET(HIST_ENTRY, timestamp));
        type_HistEntry->addInstVar(type_HistData, "data",
                                CRACK_OFFSET(HIST_ENTRY, data));
    type_HistEntry->finish();


    crack::ext::Type *array = mod->getType("array");

    crack::ext::Type *array_pHistEntry_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_HistEntry;
        array_pHistEntry_q = array->getSpecialization(params);
    }

    crack::ext::Type *type_HistoryState = mod->addType("HistoryState", sizeof(HISTORY_STATE));
        type_HistoryState->addInstVar(array_pHistEntry_q, "entries",
                                CRACK_OFFSET(HISTORY_STATE, entries));
        type_HistoryState->addInstVar(type_int, "offset",
                                CRACK_OFFSET(HISTORY_STATE, offset));
        type_HistoryState->addInstVar(type_int, "length",
                                CRACK_OFFSET(HISTORY_STATE, length));
        type_HistoryState->addInstVar(type_int, "size",
                                CRACK_OFFSET(HISTORY_STATE, size));
        type_HistoryState->addInstVar(type_int, "flags",
                                CRACK_OFFSET(HISTORY_STATE, flags));
    type_HistoryState->finish();


    crack::ext::Type *array_pbyteptr_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_byteptr;
        array_pbyteptr_q = array->getSpecialization(params);
    }

    crack::ext::Type *array_pint_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_int;
        array_pint_q = array->getSpecialization(params);
    }
    f = mod->addFunc(type_byteptr, "readline",
                     (void *)readline
                     );
       f->addArg(type_byteptr, "prompt");

    f = mod->addFunc(type_void, "using_history",
                     (void *)using_history
                     );

    f = mod->addFunc(type_HistoryState, "history_get_history_state",
                     (void *)history_get_history_state
                     );

    f = mod->addFunc(type_void, "add_history",
                     (void *)add_history
                     );
       f->addArg(type_byteptr, "string");

    f = mod->addFunc(type_void, "add_history_time",
                     (void *)add_history_time
                     );
       f->addArg(type_byteptr, "string");

    f = mod->addFunc(type_HistEntry, "remove_history",
                     (void *)remove_history
                     );
       f->addArg(type_int, "which");

    f = mod->addFunc(type_HistData, "free_history_entry",
                     (void *)free_history_entry
                     );
       f->addArg(type_HistEntry, "histent");

    f = mod->addFunc(type_HistEntry, "replace_history_entry",
                     (void *)replace_history_entry
                     );
       f->addArg(type_int, "which");
       f->addArg(type_byteptr, "line");
       f->addArg(type_HistData, "data");

    f = mod->addFunc(type_void, "clear_history",
                     (void *)clear_history
                     );

    f = mod->addFunc(type_void, "stifle_history",
                     (void *)stifle_history
                     );
       f->addArg(type_int, "max");

    f = mod->addFunc(type_int, "unstifle_history",
                     (void *)unstifle_history
                     );

    f = mod->addFunc(type_int, "history_is_stifled",
                     (void *)history_is_stifled
                     );

    f = mod->addFunc(array_pHistEntry_q, "history_list",
                     (void *)history_list
                     );

    f = mod->addFunc(type_int, "where_history",
                     (void *)where_history
                     );

    f = mod->addFunc(type_HistEntry, "current_history",
                     (void *)current_history
                     );

    f = mod->addFunc(type_HistEntry, "history_get",
                     (void *)history_get
                     );
       f->addArg(type_int, "offset");

    f = mod->addFunc(type_uint, "history_get_time",
                     (void *)history_get_time
                     );
       f->addArg(type_HistEntry, "entry");

    f = mod->addFunc(type_int, "history_total_bytes",
                     (void *)history_total_bytes
                     );

    f = mod->addFunc(type_int, "history_set_pos",
                     (void *)history_set_pos
                     );
       f->addArg(type_int, "pos");

    f = mod->addFunc(type_HistEntry, "previous_history",
                     (void *)previous_history
                     );

    f = mod->addFunc(type_HistEntry, "next_history",
                     (void *)next_history
                     );

    f = mod->addFunc(type_int, "history_search",
                     (void *)history_search
                     );
       f->addArg(type_byteptr, "string");
       f->addArg(type_int, "direction");

    f = mod->addFunc(type_int, "history_search_prefix",
                     (void *)history_search_prefix
                     );
       f->addArg(type_byteptr, "string");
       f->addArg(type_int, "direction");

    f = mod->addFunc(type_int, "history_search_pos",
                     (void *)history_search_pos
                     );
       f->addArg(type_byteptr, "string");
       f->addArg(type_int, "direction");
       f->addArg(type_int, "pos");

    f = mod->addFunc(type_int, "read_history",
                     (void *)read_history
                     );
       f->addArg(type_byteptr, "filename");

    f = mod->addFunc(type_int, "read_history_range",
                     (void *)read_history_range
                     );
       f->addArg(type_byteptr, "filename");
       f->addArg(type_int, "from");
       f->addArg(type_int, "to");

    f = mod->addFunc(type_int, "write_history",
                     (void *)write_history
                     );
       f->addArg(type_byteptr, "filename");

    f = mod->addFunc(type_int, "append_history",
                     (void *)append_history
                     );
       f->addArg(type_int, "nelements");
       f->addArg(type_byteptr, "filename");

    f = mod->addFunc(type_int, "history_truncate_file",
                     (void *)history_truncate_file
                     );
       f->addArg(type_byteptr, "filename");
       f->addArg(type_int, "nlines");

    f = mod->addFunc(type_int, "history_expand",
                     (void *)history_expand
                     );
       f->addArg(type_byteptr, "string");
       f->addArg(array_pbyteptr_q, "output");

    f = mod->addFunc(type_byteptr, "get_history_event",
                     (void *)get_history_event
                     );
       f->addArg(type_byteptr, "string");
       f->addArg(array_pint_q, "cindex");
       f->addArg(type_int, "qchar");

    f = mod->addFunc(array_pbyteptr_q, "history_tokenize",
                     (void *)history_tokenize
                     );
       f->addArg(type_byteptr, "string");

    f = mod->addFunc(type_byteptr, "history_arg_extract",
                     (void *)history_arg_extract
                     );
       f->addArg(type_int, "first");
       f->addArg(type_int, "last");
       f->addArg(type_byteptr, "string");

}
