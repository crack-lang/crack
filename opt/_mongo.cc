#include <inttypes.h>
#include <float.h>
#include <stdlib.h>
#include <mongo-client/mongo.h>
typedef void * voidptr;
typedef char * byteptr;
typedef int Undef;
typedef struct { byteptr data; gint32 size; gint32 tpe;} bufStruct;

    byteptr bson_cursor_get_string_crk(bson_cursor *c) {
        const gchar *dest;
        if (bson_cursor_get_string(c, &dest)) return (byteptr)dest;
        return NULL;
    }
double bson_cursor_get_double_crk(bson_cursor *c) {
        double result;
        if(bson_cursor_get_double(c, &result)) return result;
        return 0;
    }
bson* bson_cursor_get_document_crk(bson_cursor *c) {
        bson *dest;
        if (bson_cursor_get_document(c, &dest)) return dest;
        return NULL;
    }
bson *bson_cursor_get_array_crk(bson_cursor *c) {
        bson *arr;
        if (bson_cursor_get_array(c, &arr)) return arr;
        return NULL;
    }
bufStruct* bson_cursor_get_binary_crk(bson_cursor *c) {
      bufStruct *b = (bufStruct *)calloc(1, sizeof(bufStruct));
      bool success;
      bson_binary_subtype tpe;
      success = bson_cursor_get_binary(c, &tpe, (const guint8**)&(b->data),
                                       &(b->size));
      if (!success) {
        free(b);
        b = NULL;
      } else {
        b->tpe = gint32(tpe);
      }
      return b;
    }
byteptr bson_cursor_get_oid_crk(bson_cursor *c) {
        const guint8 *oid;
        if (bson_cursor_get_oid(c, &oid)) return (byteptr)oid;
        return NULL;
     }
bool bson_cursor_get_boolean_crk(bson_cursor *c) {
        gboolean dest;
        if (bson_cursor_get_boolean(c, &dest)) return (bool)dest;
        return false;
    }
gint64 bson_cursor_get_utc_datetime_crk(bson_cursor* c){
        gint64 dest;
        if (bson_cursor_get_utc_datetime(c, &dest)) return dest;
        return 0;
    }
byteptr bson_cursor_get_javascript_crk(bson_cursor *c){
        const gchar *dest;
        if (bson_cursor_get_javascript(c, &dest)) return (byteptr)dest;
    }
byteptr bson_cursor_get_symbol_crk(bson_cursor* c){
        const gchar *dest;
        if (bson_cursor_get_symbol(c, &dest)) return (byteptr)dest;
    }
gint32 bson_cursor_get_int32_crk(bson_cursor *c){
        gint32 dest;
        if (bson_cursor_get_int32(c, &dest)) return dest;
        return 0;
     }
gint64 bson_cursor_get_timestamp_crk(bson_cursor *c){
        gint64 dest;
        if (bson_cursor_get_timestamp(c, &dest)) return dest;
        return 0;
     }
gint64 bson_cursor_get_int64_crk(bson_cursor* c){
        gint64 dest;
        if (bson_cursor_get_int64(c, &dest)) return dest;
        return 0;
     }
bool mongo_sync_cmd_insert_crk(mongo_sync_connection *conn,
                                            byteptr ns, bson *doc) {
       return mongo_sync_cmd_insert(conn, (const gchar *) ns, doc, NULL);
        
    }


#include "ext/Module.h"
#include "ext/Type.h"
#include "ext/Func.h"

extern "C"
void crack_ext__mongo_rinit() {
    return;
}

extern "C"
void crack_ext__mongo_cinit(crack::ext::Module *mod) {
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

    crack::ext::Type *type_mongo_connection = mod->addType("mongo_connection", sizeof(Undef));
    type_mongo_connection->finish();


    crack::ext::Type *type_mongo_sync_connection = mod->addType("mongo_sync_connection", sizeof(Undef));
    type_mongo_sync_connection->finish();


    crack::ext::Type *type_mongo_sync_cursor = mod->addType("mongo_sync_cursor", sizeof(Undef));
    type_mongo_sync_cursor->finish();


    crack::ext::Type *type_mongo_sync_pool = mod->addType("mongo_sync_pool", sizeof(Undef));
    type_mongo_sync_pool->finish();


    crack::ext::Type *type_mongo_sync_pool_connection = mod->addType("mongo_sync_pool_connection", sizeof(Undef));
    type_mongo_sync_pool_connection->finish();


    crack::ext::Type *type_mongo_packet = mod->addType("mongo_packet", sizeof(Undef));
    type_mongo_packet->finish();


    crack::ext::Type *type_bson = mod->addType("bson", sizeof(Undef));
    type_bson->finish();


    crack::ext::Type *type_bson_cursor = mod->addType("bson_cursor", sizeof(Undef));
    type_bson_cursor->finish();


    crack::ext::Type *type_bufStruct = mod->addType("bufStruct", sizeof(bufStruct));
        type_bufStruct->addInstVar(type_byteptr, "data",
                                CRACK_OFFSET(bufStruct, data));
        type_bufStruct->addInstVar(type_int32, "size",
                                CRACK_OFFSET(bufStruct, size));
        type_bufStruct->addInstVar(type_int32, "tpe",
                                CRACK_OFFSET(bufStruct, tpe));
    type_bufStruct->finish();

    f = mod->addFunc(type_byteptr, "bson_type_as_string",
                     (void *)bson_type_as_string
                     );
       f->addArg(type_int, "type");

    f = mod->addFunc(type_bson, "bson_new",
                     (void *)bson_new
                     );

    f = mod->addFunc(type_bson, "bson_new_sized",
                     (void *)bson_new_sized
                     );
       f->addArg(type_int32, "size");

    f = mod->addFunc(type_bson, "bson_new_from_data",
                     (void *)bson_new_from_data
                     );
       f->addArg(type_byteptr, "data");
       f->addArg(type_int32, "size");

    f = mod->addFunc(type_bool, "bson_finish",
                     (void *)bson_finish
                     );
       f->addArg(type_bson, "b");

    f = mod->addFunc(type_bool, "bson_reset",
                     (void *)bson_reset
                     );
       f->addArg(type_bson, "b");

    f = mod->addFunc(type_void, "bson_free",
                     (void *)bson_free
                     );
       f->addArg(type_bson, "b");

    f = mod->addFunc(type_int32, "bson_size",
                     (void *)bson_size
                     );
       f->addArg(type_bson, "b");

    f = mod->addFunc(type_byteptr, "bson_data",
                     (void *)bson_data
                     );
       f->addArg(type_bson, "b");

    f = mod->addFunc(type_bool, "bson_validate_key",
                     (void *)bson_validate_key
                     );
       f->addArg(type_byteptr, "key");
       f->addArg(type_bool, "forbid_dots");
       f->addArg(type_bool, "no_dollar");

    f = mod->addFunc(type_bool, "bson_append_string",
                     (void *)bson_append_string
                     );
       f->addArg(type_bson, "b");
       f->addArg(type_byteptr, "name");
       f->addArg(type_byteptr, "val");
       f->addArg(type_int32, "length");

    f = mod->addFunc(type_bool, "bson_append_double",
                     (void *)bson_append_double
                     );
       f->addArg(type_bson, "b");
       f->addArg(type_byteptr, "name");
       f->addArg(type_float64, "d");

    f = mod->addFunc(type_bool, "bson_append_document",
                     (void *)bson_append_document
                     );
       f->addArg(type_bson, "b");
       f->addArg(type_byteptr, "name");
       f->addArg(type_bson, "doc");

    f = mod->addFunc(type_bool, "bson_append_array",
                     (void *)bson_append_array
                     );
       f->addArg(type_bson, "b");
       f->addArg(type_byteptr, "name");
       f->addArg(type_bson, "arr");

    f = mod->addFunc(type_bool, "bson_append_binary",
                     (void *)bson_append_binary
                     );
       f->addArg(type_bson, "b");
       f->addArg(type_byteptr, "name");
       f->addArg(type_int, "subtype");
       f->addArg(type_byteptr, "data");
       f->addArg(type_int32, "size");

    f = mod->addFunc(type_bool, "bson_append_oid",
                     (void *)bson_append_oid
                     );
       f->addArg(type_bson, "b");
       f->addArg(type_byteptr, "name");
       f->addArg(type_byteptr, "oid");

    f = mod->addFunc(type_bool, "bson_append_boolean",
                     (void *)bson_append_boolean
                     );
       f->addArg(type_bson, "b");
       f->addArg(type_byteptr, "name");
       f->addArg(type_bool, "value");

    f = mod->addFunc(type_bool, "bson_append_utc_datetime",
                     (void *)bson_append_utc_datetime
                     );
       f->addArg(type_bson, "b");
       f->addArg(type_byteptr, "name");
       f->addArg(type_int64, "ts");

    f = mod->addFunc(type_bool, "bson_append_null",
                     (void *)bson_append_null
                     );
       f->addArg(type_bson, "b");
       f->addArg(type_byteptr, "name");

    f = mod->addFunc(type_bool, "bson_append_regex",
                     (void *)bson_append_regex
                     );
       f->addArg(type_bson, "b");
       f->addArg(type_byteptr, "name");
       f->addArg(type_byteptr, "regexp");
       f->addArg(type_byteptr, "options");

    f = mod->addFunc(type_bool, "bson_append_javascript",
                     (void *)bson_append_javascript
                     );
       f->addArg(type_bson, "b");
       f->addArg(type_byteptr, "name");
       f->addArg(type_byteptr, "js");
       f->addArg(type_int32, "len");

    f = mod->addFunc(type_bool, "bson_append_symbol",
                     (void *)bson_append_symbol
                     );
       f->addArg(type_bson, "b");
       f->addArg(type_byteptr, "name");
       f->addArg(type_byteptr, "symbol");
       f->addArg(type_int32, "len");

    f = mod->addFunc(type_bool, "bson_append_javascript_w_scope",
                     (void *)bson_append_javascript_w_scope
                     );
       f->addArg(type_bson, "b");
       f->addArg(type_byteptr, "name");
       f->addArg(type_byteptr, "js");
       f->addArg(type_int32, "len");
       f->addArg(type_bson, "scope");

    f = mod->addFunc(type_bool, "bson_append_int32",
                     (void *)bson_append_int32
                     );
       f->addArg(type_bson, "b");
       f->addArg(type_byteptr, "name");
       f->addArg(type_int32, "i");

    f = mod->addFunc(type_bool, "bson_append_timestamp",
                     (void *)bson_append_timestamp
                     );
       f->addArg(type_bson, "b");
       f->addArg(type_byteptr, "name");
       f->addArg(type_int64, "ts");

    f = mod->addFunc(type_bool, "bson_append_int64",
                     (void *)bson_append_int64
                     );
       f->addArg(type_bson, "b");
       f->addArg(type_byteptr, "name");
       f->addArg(type_int64, "i");

    f = mod->addFunc(type_bson_cursor, "bson_cursor_new",
                     (void *)bson_cursor_new
                     );
       f->addArg(type_bson, "b");

    f = mod->addFunc(type_bson_cursor, "bson_find",
                     (void *)bson_find
                     );
       f->addArg(type_bson, "b");
       f->addArg(type_byteptr, "name");

    f = mod->addFunc(type_void, "bson_cursor_free",
                     (void *)bson_cursor_free
                     );
       f->addArg(type_bson_cursor, "c");

    f = mod->addFunc(type_bool, "bson_cursor_next",
                     (void *)bson_cursor_next
                     );
       f->addArg(type_bson_cursor, "c");

    f = mod->addFunc(type_bool, "bson_cursor_find_next",
                     (void *)bson_cursor_find_next
                     );
       f->addArg(type_bson_cursor, "c");
       f->addArg(type_byteptr, "name");

    f = mod->addFunc(type_bool, "bson_cursor_find",
                     (void *)bson_cursor_find
                     );
       f->addArg(type_bson_cursor, "c");
       f->addArg(type_byteptr, "name");

    f = mod->addFunc(type_int, "bson_cursor_type",
                     (void *)bson_cursor_type
                     );
       f->addArg(type_bson_cursor, "c");

    f = mod->addFunc(type_byteptr, "bson_cursor_type_as_string",
                     (void *)bson_cursor_type_as_string
                     );
       f->addArg(type_bson_cursor, "c");

    f = mod->addFunc(type_byteptr, "bson_cursor_key",
                     (void *)bson_cursor_key
                     );
       f->addArg(type_bson_cursor, "c");

    f = mod->addFunc(type_byteptr, "bson_cursor_get_string",
                     (void *)bson_cursor_get_string_crk
                     );
       f->addArg(type_bson_cursor, "c");

    f = mod->addFunc(type_float64, "bson_cursor_get_double",
                     (void *)bson_cursor_get_double_crk
                     );
       f->addArg(type_bson_cursor, "c");

    f = mod->addFunc(type_bson, "bson_cursor_get_document",
                     (void *)bson_cursor_get_document_crk
                     );
       f->addArg(type_bson_cursor, "c");

    f = mod->addFunc(type_bson, "bson_cursor_get_array",
                     (void *)bson_cursor_get_array_crk
                     );
       f->addArg(type_bson_cursor, "c");

    f = mod->addFunc(type_bufStruct, "bson_cursor_get_binary",
                     (void *)bson_cursor_get_binary_crk
                     );
       f->addArg(type_bson_cursor, "c");

    f = mod->addFunc(type_byteptr, "bson_cursor_get_oid",
                     (void *)bson_cursor_get_oid_crk
                     );
       f->addArg(type_bson_cursor, "c");

    f = mod->addFunc(type_bool, "bson_cursor_get_boolean",
                     (void *)bson_cursor_get_boolean_crk
                     );
       f->addArg(type_bson_cursor, "c");

    f = mod->addFunc(type_int64, "bson_cursor_get_utc_datetime",
                     (void *)bson_cursor_get_utc_datetime_crk
                     );
       f->addArg(type_bson_cursor, "c");

    f = mod->addFunc(type_byteptr, "bson_cursor_get_javascript",
                     (void *)bson_cursor_get_javascript_crk
                     );
       f->addArg(type_bson_cursor, "c");

    f = mod->addFunc(type_byteptr, "bson_cursor_get_symbol",
                     (void *)bson_cursor_get_symbol_crk
                     );
       f->addArg(type_bson_cursor, "c");

    f = mod->addFunc(type_int32, "bson_cursor_get_int32",
                     (void *)bson_cursor_get_int32_crk
                     );
       f->addArg(type_bson_cursor, "c");

    f = mod->addFunc(type_int64, "bson_cursor_get_timestamp",
                     (void *)bson_cursor_get_timestamp_crk
                     );
       f->addArg(type_bson_cursor, "c");

    f = mod->addFunc(type_int64, "bson_cursor_get_int64",
                     (void *)bson_cursor_get_int64_crk
                     );
       f->addArg(type_bson_cursor, "c");

    f = mod->addFunc(type_mongo_sync_connection, "mongo_sync_connect",
                     (void *)mongo_sync_connect
                     );
       f->addArg(type_byteptr, "address");
       f->addArg(type_int, "port");
       f->addArg(type_bool, "slaveok");

    f = mod->addFunc(type_bool, "mongo_sync_conn_seed_add",
                     (void *)mongo_sync_conn_seed_add
                     );
       f->addArg(type_mongo_sync_connection, "conn");
       f->addArg(type_byteptr, "host");
       f->addArg(type_int, "port");

    f = mod->addFunc(type_mongo_sync_connection, "mongo_sync_reconnect",
                     (void *)mongo_sync_reconnect
                     );
       f->addArg(type_mongo_sync_connection, "conn");
       f->addArg(type_bool, "force_master");

    f = mod->addFunc(type_void, "mongo_sync_disconnect",
                     (void *)mongo_sync_disconnect
                     );
       f->addArg(type_mongo_sync_connection, "conn");

    f = mod->addFunc(type_bool, "mongo_sync_conn_get_slaveok",
                     (void *)mongo_sync_conn_get_slaveok
                     );
       f->addArg(type_mongo_sync_connection, "conn");

    f = mod->addFunc(type_bool, "mongo_sync_conn_set_slaveok",
                     (void *)mongo_sync_conn_set_slaveok
                     );
       f->addArg(type_mongo_sync_connection, "conn");
       f->addArg(type_bool, "slaveok");

    f = mod->addFunc(type_bool, "mongo_sync_conn_get_safe_mode",
                     (void *)mongo_sync_conn_get_safe_mode
                     );
       f->addArg(type_mongo_sync_connection, "conn");

    f = mod->addFunc(type_bool, "mongo_sync_conn_set_safe_mode",
                     (void *)mongo_sync_conn_set_safe_mode
                     );
       f->addArg(type_mongo_sync_connection, "conn");
       f->addArg(type_bool, "safe_mode");

    f = mod->addFunc(type_bool, "mongo_sync_conn_get_auto_reconnect",
                     (void *)mongo_sync_conn_get_auto_reconnect
                     );
       f->addArg(type_mongo_sync_connection, "conn");

    f = mod->addFunc(type_bool, "mongo_sync_conn_set_auto_reconnect",
                     (void *)mongo_sync_conn_set_auto_reconnect
                     );
       f->addArg(type_mongo_sync_connection, "conn");
       f->addArg(type_bool, "auto_reconnect");

    f = mod->addFunc(type_int32, "mongo_sync_conn_get_max_insert_size",
                     (void *)mongo_sync_conn_get_max_insert_size
                     );
       f->addArg(type_mongo_sync_connection, "conn");

    f = mod->addFunc(type_bool, "mongo_sync_conn_set_max_insert_size",
                     (void *)mongo_sync_conn_set_max_insert_size
                     );
       f->addArg(type_mongo_sync_connection, "conn");
       f->addArg(type_int32, "max_size");

    f = mod->addFunc(type_bool, "mongo_sync_cmd_update",
                     (void *)mongo_sync_cmd_update
                     );
       f->addArg(type_mongo_sync_connection, "conn");
       f->addArg(type_byteptr, "ns");
       f->addArg(type_int32, "flags");
       f->addArg(type_bson, "selector");
       f->addArg(type_bson, "update");

    f = mod->addFunc(type_bool, "mongo_sync_cmd_insert",
                     (void *)mongo_sync_cmd_insert_crk
                     );
       f->addArg(type_mongo_sync_connection, "conn");
       f->addArg(type_byteptr, "ns");
       f->addArg(type_bson, "doc");

    f = mod->addFunc(type_bool, "mongo_sync_cmd_insert_n",
                     (void *)mongo_sync_cmd_insert_n
                     );
       f->addArg(type_mongo_sync_connection, "conn");
       f->addArg(type_byteptr, "ns");
       f->addArg(type_int32, "n");
       f->addArg(type_bson, "docs");

    f = mod->addFunc(type_mongo_packet, "mongo_sync_cmd_query",
                     (void *)mongo_sync_cmd_query
                     );
       f->addArg(type_mongo_sync_connection, "conn");
       f->addArg(type_byteptr, "ns");
       f->addArg(type_int32, "flags");
       f->addArg(type_int32, "skip");
       f->addArg(type_int32, "ret");
       f->addArg(type_bson, "query");
       f->addArg(type_bson, "sel");

    f = mod->addFunc(type_mongo_packet, "mongo_sync_cmd_get_more",
                     (void *)mongo_sync_cmd_get_more
                     );
       f->addArg(type_mongo_sync_connection, "conn");
       f->addArg(type_byteptr, "ns");
       f->addArg(type_int32, "ret");
       f->addArg(type_int64, "cursor_id");

    f = mod->addFunc(type_bool, "mongo_sync_cmd_delete",
                     (void *)mongo_sync_cmd_delete
                     );
       f->addArg(type_mongo_sync_connection, "conn");
       f->addArg(type_byteptr, "ns");
       f->addArg(type_int32, "flags");
       f->addArg(type_bson, "sel");

    f = mod->addFunc(type_mongo_packet, "mongo_sync_cmd_custom",
                     (void *)mongo_sync_cmd_custom
                     );
       f->addArg(type_mongo_sync_connection, "conn");
       f->addArg(type_byteptr, "db");
       f->addArg(type_bson, "command");

    f = mod->addFunc(type_float64, "mongo_sync_cmd_count",
                     (void *)mongo_sync_cmd_count
                     );
       f->addArg(type_mongo_sync_connection, "conn");
       f->addArg(type_byteptr, "db");
       f->addArg(type_byteptr, "coll");
       f->addArg(type_bson, "query");

    f = mod->addFunc(type_bson, "mongo_sync_cmd_exists",
                     (void *)mongo_sync_cmd_exists
                     );
       f->addArg(type_mongo_sync_connection, "conn");
       f->addArg(type_byteptr, "db");
       f->addArg(type_byteptr, "coll");

    f = mod->addFunc(type_bool, "mongo_sync_cmd_drop",
                     (void *)mongo_sync_cmd_drop
                     );
       f->addArg(type_mongo_sync_connection, "conn");
       f->addArg(type_byteptr, "db");
       f->addArg(type_byteptr, "coll");

    f = mod->addFunc(type_bool, "mongo_sync_cmd_reset_error",
                     (void *)mongo_sync_cmd_reset_error
                     );
       f->addArg(type_mongo_sync_connection, "conn");
       f->addArg(type_byteptr, "db");

    f = mod->addFunc(type_bool, "mongo_sync_cmd_is_master",
                     (void *)mongo_sync_cmd_is_master
                     );
       f->addArg(type_mongo_sync_connection, "conn");

    f = mod->addFunc(type_bool, "mongo_sync_cmd_ping",
                     (void *)mongo_sync_cmd_ping
                     );
       f->addArg(type_mongo_sync_connection, "conn");

    f = mod->addFunc(type_bool, "mongo_sync_cmd_user_add",
                     (void *)mongo_sync_cmd_user_add
                     );
       f->addArg(type_mongo_sync_connection, "conn");
       f->addArg(type_byteptr, "db");
       f->addArg(type_byteptr, "user");
       f->addArg(type_byteptr, "pw");

    f = mod->addFunc(type_bool, "mongo_sync_cmd_user_remove",
                     (void *)mongo_sync_cmd_user_remove
                     );
       f->addArg(type_mongo_sync_connection, "conn");
       f->addArg(type_byteptr, "db");
       f->addArg(type_byteptr, "user");

    f = mod->addFunc(type_bool, "mongo_sync_cmd_authenticate",
                     (void *)mongo_sync_cmd_authenticate
                     );
       f->addArg(type_mongo_sync_connection, "conn");
       f->addArg(type_byteptr, "db");
       f->addArg(type_byteptr, "user");
       f->addArg(type_byteptr, "pw");

    f = mod->addFunc(type_bool, "mongo_sync_cmd_index_create",
                     (void *)mongo_sync_cmd_index_create
                     );
       f->addArg(type_mongo_sync_connection, "conn");
       f->addArg(type_byteptr, "ns");
       f->addArg(type_bson, "key");
       f->addArg(type_int, "options");

    f = mod->addFunc(type_bool, "mongo_sync_cmd_index_drop",
                     (void *)mongo_sync_cmd_index_drop
                     );
       f->addArg(type_mongo_sync_connection, "conn");
       f->addArg(type_byteptr, "ns");
       f->addArg(type_bson, "key");

    f = mod->addFunc(type_bool, "mongo_sync_cmd_index_drop_all",
                     (void *)mongo_sync_cmd_index_drop_all
                     );
       f->addArg(type_mongo_sync_connection, "conn");
       f->addArg(type_byteptr, "ns");

    f = mod->addFunc(type_mongo_packet, "mongo_wire_packet_new",
                     (void *)mongo_wire_packet_new
                     );

    f = mod->addFunc(type_void, "mongo_wire_packet_free",
                     (void *)mongo_wire_packet_free
                     );
       f->addArg(type_mongo_packet, "p");

    f = mod->addFunc(type_mongo_sync_cursor, "mongo_sync_cursor_new",
                     (void *)mongo_sync_cursor_new
                     );
       f->addArg(type_mongo_sync_connection, "conn");
       f->addArg(type_byteptr, "ns");
       f->addArg(type_mongo_packet, "packet");

    f = mod->addFunc(type_bool, "mongo_sync_cursor_next",
                     (void *)mongo_sync_cursor_next
                     );
       f->addArg(type_mongo_sync_cursor, "cursor");

    f = mod->addFunc(type_bson, "mongo_sync_cursor_get_data",
                     (void *)mongo_sync_cursor_get_data
                     );
       f->addArg(type_mongo_sync_cursor, "cursor");

    f = mod->addFunc(type_void, "mongo_sync_cursor_free",
                     (void *)mongo_sync_cursor_free
                     );
       f->addArg(type_mongo_sync_cursor, "cursor");

    f = mod->addFunc(type_mongo_sync_pool, "mongo_sync_pool_new",
                     (void *)mongo_sync_pool_new
                     );
       f->addArg(type_byteptr, "host");
       f->addArg(type_int, "port");
       f->addArg(type_int, "nmasters");
       f->addArg(type_int, "nslaves");

    f = mod->addFunc(type_void, "mongo_sync_pool_free",
                     (void *)mongo_sync_pool_free
                     );
       f->addArg(type_mongo_sync_pool, "pool");

    f = mod->addFunc(type_mongo_sync_pool_connection, "mongo_sync_pool_pick",
                     (void *)mongo_sync_pool_pick
                     );
       f->addArg(type_mongo_sync_pool, "pool");
       f->addArg(type_bool, "want_master");

    f = mod->addFunc(type_bool, "mongo_sync_pool_return",
                     (void *)mongo_sync_pool_return
                     );
       f->addArg(type_mongo_sync_pool, "pool");
       f->addArg(type_mongo_sync_pool_connection, "conn");

    f = mod->addFunc(type_void, "mongo_util_oid_init",
                     (void *)mongo_util_oid_init
                     );
       f->addArg(type_int32, "machine_id");

    f = mod->addFunc(type_byteptr, "mongo_util_oid_new",
                     (void *)mongo_util_oid_new
                     );
       f->addArg(type_int32, "seq");

    f = mod->addFunc(type_byteptr, "mongo_util_oid_new_with_time",
                     (void *)mongo_util_oid_new_with_time
                     );
       f->addArg(type_int32, "time");
       f->addArg(type_int32, "seq");

    f = mod->addFunc(type_byteptr, "mongo_util_oid_as_string",
                     (void *)mongo_util_oid_as_string
                     );
       f->addArg(type_byteptr, "oid");

    f = mod->addFunc(type_mongo_connection, "mongo_connect",
                     (void *)mongo_connect
                     );
       f->addArg(type_byteptr, "address");
       f->addArg(type_int, "port");

    f = mod->addFunc(type_void, "mongo_disconnect",
                     (void *)mongo_disconnect
                     );
       f->addArg(type_mongo_connection, "conn");

    f = mod->addFunc(type_int32, "mongo_connection_get_requestid",
                     (void *)mongo_connection_get_requestid
                     );
       f->addArg(type_mongo_connection, "conn");

    f = mod->addFunc(type_bool, "mongo_connection_set_timeout",
                     (void *)mongo_connection_set_timeout
                     );
       f->addArg(type_mongo_connection, "conn");
       f->addArg(type_int, "timeout");


    mod->addConstant(type_int, "BSON_BINARY_SUBTYPE_GENERIC",
                     static_cast<int>(BSON_BINARY_SUBTYPE_GENERIC)
                     );

    mod->addConstant(type_int, "BSON_BINARY_SUBTYPE_FUNCTION",
                     static_cast<int>(BSON_BINARY_SUBTYPE_FUNCTION)
                     );

    mod->addConstant(type_int, "BSON_BINARY_SUBTYPE_BINARY",
                     static_cast<int>(BSON_BINARY_SUBTYPE_BINARY)
                     );

    mod->addConstant(type_int, "BSON_BINARY_SUBTYPE_UUID",
                     static_cast<int>(BSON_BINARY_SUBTYPE_UUID)
                     );

    mod->addConstant(type_int, "BSON_BINARY_SUBTYPE_MD5",
                     static_cast<int>(BSON_BINARY_SUBTYPE_MD5)
                     );

    mod->addConstant(type_int, "BSON_BINARY_SUBTYPE_USER_DEFINED",
                     static_cast<int>(BSON_BINARY_SUBTYPE_USER_DEFINED)
                     );

    mod->addConstant(type_int, "BSON_TYPE_NONE",
                     static_cast<int>(BSON_TYPE_NONE)
                     );

    mod->addConstant(type_int, "BSON_TYPE_DOUBLE",
                     static_cast<int>(BSON_TYPE_DOUBLE)
                     );

    mod->addConstant(type_int, "BSON_TYPE_STRING",
                     static_cast<int>(BSON_TYPE_STRING)
                     );

    mod->addConstant(type_int, "BSON_TYPE_DOCUMENT",
                     static_cast<int>(BSON_TYPE_DOCUMENT)
                     );

    mod->addConstant(type_int, "BSON_TYPE_ARRAY",
                     static_cast<int>(BSON_TYPE_ARRAY)
                     );

    mod->addConstant(type_int, "BSON_TYPE_BINARY",
                     static_cast<int>(BSON_TYPE_BINARY)
                     );

    mod->addConstant(type_int, "BSON_TYPE_OID",
                     static_cast<int>(BSON_TYPE_OID)
                     );

    mod->addConstant(type_int, "BSON_TYPE_BOOLEAN",
                     static_cast<int>(BSON_TYPE_BOOLEAN)
                     );

    mod->addConstant(type_int, "BSON_TYPE_UTC_DATETIME",
                     static_cast<int>(BSON_TYPE_UTC_DATETIME)
                     );

    mod->addConstant(type_int, "BSON_TYPE_NULL",
                     static_cast<int>(BSON_TYPE_NULL)
                     );

    mod->addConstant(type_int, "BSON_TYPE_REGEXP",
                     static_cast<int>(BSON_TYPE_REGEXP)
                     );

    mod->addConstant(type_int, "BSON_TYPE_JS_CODE",
                     static_cast<int>(BSON_TYPE_JS_CODE)
                     );

    mod->addConstant(type_int, "BSON_TYPE_SYMBOL",
                     static_cast<int>(BSON_TYPE_SYMBOL)
                     );

    mod->addConstant(type_int, "BSON_TYPE_JS_CODE_W_SCOPE",
                     static_cast<int>(BSON_TYPE_JS_CODE_W_SCOPE)
                     );

    mod->addConstant(type_int, "BSON_TYPE_INT32",
                     static_cast<int>(BSON_TYPE_INT32)
                     );

    mod->addConstant(type_int, "BSON_TYPE_TIMESTAMP",
                     static_cast<int>(BSON_TYPE_TIMESTAMP)
                     );

    mod->addConstant(type_int, "BSON_TYPE_INT64",
                     static_cast<int>(BSON_TYPE_INT64)
                     );

    mod->addConstant(type_int, "MONGO_COLLECTION_DEFAULTS",
                     static_cast<int>(MONGO_COLLECTION_DEFAULTS)
                     );

    mod->addConstant(type_int, "MONGO_COLLECTION_CAPPED",
                     static_cast<int>(MONGO_COLLECTION_CAPPED)
                     );

    mod->addConstant(type_int, "MONGO_COLLECTION_CAPPED_MAX",
                     static_cast<int>(MONGO_COLLECTION_CAPPED_MAX)
                     );

    mod->addConstant(type_int, "MONGO_COLLECTION_AUTO_INDEX_ID",
                     static_cast<int>(MONGO_COLLECTION_AUTO_INDEX_ID)
                     );

    mod->addConstant(type_int, "MONGO_COLLECTION_SIZED",
                     static_cast<int>(MONGO_COLLECTION_SIZED)
                     );

    mod->addConstant(type_int, "MONGO_INDEX_UNIQUE",
                     static_cast<int>(MONGO_INDEX_UNIQUE)
                     );

    mod->addConstant(type_int, "MONGO_INDEX_DROP_DUPS",
                     static_cast<int>(MONGO_INDEX_DROP_DUPS)
                     );

    mod->addConstant(type_int, "MONGO_INDEX_BACKGROUND",
                     static_cast<int>(MONGO_INDEX_BACKGROUND)
                     );

    mod->addConstant(type_int, "MONGO_INDEX_SPARSE",
                     static_cast<int>(MONGO_INDEX_SPARSE)
                     );

    mod->addConstant(type_int, "MONGO_WIRE_FLAG_QUERY_TAILABLE_CURSOR",
                     static_cast<int>(MONGO_WIRE_FLAG_QUERY_TAILABLE_CURSOR)
                     );

    mod->addConstant(type_int, "MONGO_WIRE_FLAG_QUERY_SLAVE_OK",
                     static_cast<int>(MONGO_WIRE_FLAG_QUERY_SLAVE_OK)
                     );

    mod->addConstant(type_int, "MONGO_WIRE_FLAG_QUERY_NO_CURSOR_TIMEOUT",
                     static_cast<int>(MONGO_WIRE_FLAG_QUERY_NO_CURSOR_TIMEOUT)
                     );

    mod->addConstant(type_int, "MONGO_WIRE_FLAG_QUERY_AWAIT_DATA",
                     static_cast<int>(MONGO_WIRE_FLAG_QUERY_AWAIT_DATA)
                     );

    mod->addConstant(type_int, "MONGO_WIRE_FLAG_QUERY_EXHAUST",
                     static_cast<int>(MONGO_WIRE_FLAG_QUERY_EXHAUST)
                     );

    mod->addConstant(type_int, "MONGO_WIRE_FLAG_QUERY_PARTIAL_RESULTS",
                     static_cast<int>(MONGO_WIRE_FLAG_QUERY_PARTIAL_RESULTS)
                     );

    mod->addConstant(type_int, "MONGO_WIRE_FLAG_UPDATE_UPSERT",
                     static_cast<int>(MONGO_WIRE_FLAG_UPDATE_UPSERT)
                     );

    mod->addConstant(type_int, "MONGO_WIRE_FLAG_UPDATE_MULTI",
                     static_cast<int>(MONGO_WIRE_FLAG_UPDATE_MULTI)
                     );
}
