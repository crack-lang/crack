#include <curl/curl.h>
typedef void * voidptr;
typedef char * byteptr;
typedef int Undef;
typedef struct curl_slist crack_slist;

int easy_setopt_long(CURL *handle, int option, int64_t parameter){
        return curl_easy_setopt(handle, (CURLoption)option, (long) parameter);
    }
int easy_setopt_offset(CURL *handle, int option, size_t parameter){
        return curl_easy_setopt(handle, (CURLoption)option, (size_t) parameter);
    }
class CURLinfoWrapper {
        public:

        CURLcode success;
        CURLINFO info;        // Code of the info requested
        long resultLong;
        double resultDouble;
        voidptr resultPtr;

        CURLinfoWrapper(CURLINFO info_in){
            info = info_in;
        };
    };

void easy_info_new(CURLinfoWrapper *ci, int info) {
        ci->info = (CURLINFO)info;
    }
int easy_info_get_long(CURL *handle, CURLinfoWrapper *result) {
        result->success = curl_easy_getinfo(handle, result->info,
                                            &(result->resultLong));
        return result->success;
    }

int easy_info_get_double(CURL *handle, CURLinfoWrapper *result) {
        result->success = curl_easy_getinfo(handle, result->info,
                                            &(result->resultDouble));
        return result->success;
    }

int easy_info_get_ptr(CURL *handle, CURLinfoWrapper *result) {

        result->success = curl_easy_getinfo(handle, result->info,
                            &(result->resultPtr));
        return result->success;
    }

int easy_info_get_slist(CURL *handle, CURLinfoWrapper *result) {
        result->success = curl_easy_getinfo(handle, result->info,
                            (crack_slist **) &(result->resultPtr));
        return result->success;
    }

void curl_slist_new(crack_slist *list, char *buffer) {
        list->data = buffer;
        list->next = NULL;
    }
crack_slist *crack_slist_cast(voidptr ptr) {
        return (crack_slist *)ptr;
    }


#include "ext/Module.h"
#include "ext/Type.h"
#include "ext/Func.h"

extern "C"
void crack_ext__curl_rinit() {
    return;
}

extern "C"
void crack_ext__curl_cinit(crack::ext::Module *mod) {
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

    crack::ext::Type *type_CURL = mod->addType("CURL", sizeof(Undef));
    type_CURL->finish();


    crack::ext::Type *type_CURLinfo = mod->addType("CURLinfo", sizeof(CURLinfoWrapper));
        type_CURLinfo->addInstVar(type_int, "success",
                                CRACK_OFFSET(CURLinfoWrapper, success));
        type_CURLinfo->addInstVar(type_int, "info",
                                CRACK_OFFSET(CURLinfoWrapper, info));
        type_CURLinfo->addInstVar(type_int64, "resultLong",
                                CRACK_OFFSET(CURLinfoWrapper, resultLong));
        type_CURLinfo->addInstVar(type_float64, "resultDouble",
                                CRACK_OFFSET(CURLinfoWrapper, resultDouble));
        type_CURLinfo->addInstVar(type_voidptr, "resultPtr",
                                CRACK_OFFSET(CURLinfoWrapper, resultPtr));
        f = type_CURLinfo->addConstructor("init",
                            (void *)easy_info_new
                        );
    f->addArg(type_int, 
              "info"
              );

    type_CURLinfo->finish();


    crack::ext::Type *type_slist = mod->addForwardType("slist", sizeof(crack_slist));

    // Definition of forward type slist ---------------------------------------
        type_slist->addInstVar(type_byteptr, "data",
                                CRACK_OFFSET(crack_slist, data));
        type_slist->addInstVar(type_slist, "next",
                                CRACK_OFFSET(crack_slist, next));
        f = type_slist->addConstructor("init",
                            (void *)curl_slist_new
                        );
    f->addArg(type_byteptr, 
              "buffer"
              );


    f = type_slist->addMethod(
        type_slist, 
        "append",
        (void *)curl_slist_append
    );
    f->addArg(type_byteptr, 
              "buffer"
              );

    type_slist->finish();

    f = mod->addFunc(type_byteptr, "curl_version",
                     (void *)curl_version
                     );

    f = mod->addFunc(type_CURL, "easy_init",
                     (void *)curl_easy_init
                     );

    f = mod->addFunc(type_void, "easy_cleanup",
                     (void *)curl_easy_cleanup
                     );
       f->addArg(type_CURL, "handle");

    f = mod->addFunc(type_byteptr, "curl_escape",
                     (void *)curl_escape
                     );
       f->addArg(type_byteptr, "url");
       f->addArg(type_int, "length");

    f = mod->addFunc(type_byteptr, "curl_unescape",
                     (void *)curl_unescape
                     );
       f->addArg(type_byteptr, "url");
       f->addArg(type_int, "length");

    f = mod->addFunc(type_void, "curl_free",
                     (void *)curl_free
                     );
       f->addArg(type_byteptr, "buffer");

    f = mod->addFunc(type_int, "easy_setopt_long",
                     (void *)easy_setopt_long
                     );
       f->addArg(type_CURL, "handle");
       f->addArg(type_int, "option");
       f->addArg(type_int64, "parameter");

    f = mod->addFunc(type_int, "easy_setopt_ptr",
                     (void *)curl_easy_setopt
                     );
       f->addArg(type_CURL, "handle");
       f->addArg(type_int, "option");
       f->addArg(type_voidptr, "parameter");

    f = mod->addFunc(type_int, "easy_setopt_offset",
                     (void *)easy_setopt_offset
                     );
       f->addArg(type_CURL, "handle");
       f->addArg(type_int, "option");
       f->addArg(type_uintz, "parameter");

    f = mod->addFunc(type_int, "easy_perform",
                     (void *)curl_easy_perform
                     );
       f->addArg(type_CURL, "handle");

    f = mod->addFunc(type_int, "easy_info_get_long",
                     (void *)easy_info_get_long
                     );
       f->addArg(type_CURL, "handle");
       f->addArg(type_CURLinfo, "result");

    f = mod->addFunc(type_int, "easy_info_get_double",
                     (void *)easy_info_get_double
                     );
       f->addArg(type_CURL, "handle");
       f->addArg(type_CURLinfo, "result");

    f = mod->addFunc(type_int, "easy_info_get_ptr",
                     (void *)easy_info_get_ptr
                     );
       f->addArg(type_CURL, "handle");
       f->addArg(type_CURLinfo, "result");

    f = mod->addFunc(type_int, "easy_info_get_slist",
                     (void *)easy_info_get_slist
                     );
       f->addArg(type_CURL, "handle");
       f->addArg(type_CURLinfo, "result");

    f = mod->addFunc(type_byteptr, "easy_strerror",
                     (void *)curl_easy_strerror
                     );
       f->addArg(type_int, "code");

    f = mod->addFunc(type_int, "easy_pause",
                     (void *)curl_easy_pause
                     );
       f->addArg(type_CURL, "handle");
       f->addArg(type_int, "bitmask");

    f = mod->addFunc(type_slist, "slist_cast",
                     (void *)crack_slist_cast
                     );
       f->addArg(type_voidptr, "ptr");

    f = mod->addFunc(type_void, "slist_free_all",
                     (void *)curl_slist_free_all
                     );
       f->addArg(type_slist, "list");


    mod->addConstant(type_int64, "HTTPPOST_FILENAME",
                     static_cast<int64_t>(HTTPPOST_FILENAME)
                     );

    mod->addConstant(type_int64, "HTTPPOST_READFILE",
                     static_cast<int64_t>(HTTPPOST_READFILE)
                     );

    mod->addConstant(type_int64, "HTTPPOST_PTRNAME",
                     static_cast<int64_t>(HTTPPOST_PTRNAME)
                     );

    mod->addConstant(type_int64, "HTTPPOST_PTRCONTENTS",
                     static_cast<int64_t>(HTTPPOST_PTRCONTENTS)
                     );

    mod->addConstant(type_int64, "HTTPPOST_BUFFER",
                     static_cast<int64_t>(HTTPPOST_BUFFER)
                     );

    mod->addConstant(type_int64, "HTTPPOST_PTRBUFFER",
                     static_cast<int64_t>(HTTPPOST_PTRBUFFER)
                     );

    mod->addConstant(type_int64, "HTTPPOST_CALLBACK",
                     static_cast<int64_t>(HTTPPOST_CALLBACK)
                     );

    mod->addConstant(type_int64, "CURLAUTH_NONE",
                     static_cast<int64_t>(CURLAUTH_NONE)
                     );

    mod->addConstant(type_int64, "CURLAUTH_BASIC",
                     static_cast<int64_t>(CURLAUTH_BASIC)
                     );

    mod->addConstant(type_int64, "CURLAUTH_DIGEST",
                     static_cast<int64_t>(CURLAUTH_DIGEST)
                     );

    mod->addConstant(type_int64, "CURLAUTH_GSSNEGOTIATE",
                     static_cast<int64_t>(CURLAUTH_GSSNEGOTIATE)
                     );

    mod->addConstant(type_int64, "CURLAUTH_NTLM",
                     static_cast<int64_t>(CURLAUTH_NTLM)
                     );

    mod->addConstant(type_int64, "CURLAUTH_DIGEST_IE",
                     static_cast<int64_t>(CURLAUTH_DIGEST_IE)
                     );

    mod->addConstant(type_int64, "CURLAUTH_NTLM_WB",
                     static_cast<int64_t>(CURLAUTH_NTLM_WB)
                     );

    mod->addConstant(type_int64, "CURLAUTH_ONLY",
                     static_cast<int64_t>(CURLAUTH_ONLY)
                     );

    mod->addConstant(type_int64, "CURLAUTH_ANY",
                     static_cast<int64_t>(CURLAUTH_ANY)
                     );

    mod->addConstant(type_int64, "CURLAUTH_ANYSAFE",
                     static_cast<int64_t>(CURLAUTH_ANYSAFE)
                     );

    mod->addConstant(type_int64, "CURLSSH_AUTH_ANY",
                     static_cast<int64_t>(CURLSSH_AUTH_ANY)
                     );

    mod->addConstant(type_int64, "CURLSSH_AUTH_NONE",
                     static_cast<int64_t>(CURLSSH_AUTH_NONE)
                     );

    mod->addConstant(type_int64, "CURLSSH_AUTH_PUBLICKEY",
                     static_cast<int64_t>(CURLSSH_AUTH_PUBLICKEY)
                     );

    mod->addConstant(type_int64, "CURLSSH_AUTH_PASSWORD",
                     static_cast<int64_t>(CURLSSH_AUTH_PASSWORD)
                     );

    mod->addConstant(type_int64, "CURLSSH_AUTH_HOST",
                     static_cast<int64_t>(CURLSSH_AUTH_HOST)
                     );

    mod->addConstant(type_int64, "CURLSSH_AUTH_KEYBOARD",
                     static_cast<int64_t>(CURLSSH_AUTH_KEYBOARD)
                     );

    mod->addConstant(type_int64, "CURLSSH_AUTH_DEFAULT",
                     static_cast<int64_t>(CURLSSH_AUTH_DEFAULT)
                     );

    mod->addConstant(type_int, "CURLFILETYPE_FILE",
                     static_cast<int>(CURLFILETYPE_FILE)
                     );

    mod->addConstant(type_int, "CURLFILETYPE_DIRECTORY",
                     static_cast<int>(CURLFILETYPE_DIRECTORY)
                     );

    mod->addConstant(type_int, "CURLFILETYPE_SYMLINK",
                     static_cast<int>(CURLFILETYPE_SYMLINK)
                     );

    mod->addConstant(type_int, "CURLFILETYPE_DEVICE_BLOCK",
                     static_cast<int>(CURLFILETYPE_DEVICE_BLOCK)
                     );

    mod->addConstant(type_int, "CURLFILETYPE_DEVICE_CHAR",
                     static_cast<int>(CURLFILETYPE_DEVICE_CHAR)
                     );

    mod->addConstant(type_int, "CURLFILETYPE_NAMEDPIPE",
                     static_cast<int>(CURLFILETYPE_NAMEDPIPE)
                     );

    mod->addConstant(type_int, "CURLFILETYPE_SOCKET",
                     static_cast<int>(CURLFILETYPE_SOCKET)
                     );

    mod->addConstant(type_int, "CURLFILETYPE_DOOR",
                     static_cast<int>(CURLFILETYPE_DOOR)
                     );

    mod->addConstant(type_int, "CURLFILETYPE_UNKNOWN",
                     static_cast<int>(CURLFILETYPE_UNKNOWN)
                     );

    mod->addConstant(type_int, "CURLSOCKTYPE_IPCXN",
                     static_cast<int>(CURLSOCKTYPE_IPCXN)
                     );

    mod->addConstant(type_int, "CURL_MAX_WRITE_SIZE",
                     static_cast<int>(CURL_MAX_WRITE_SIZE)
                     );

    mod->addConstant(type_int, "CURL_MAX_HTTP_HEADER",
                     static_cast<int>(CURL_MAX_HTTP_HEADER)
                     );

    mod->addConstant(type_int, "CURL_WRITEFUNC_PAUSE",
                     static_cast<int>(CURL_WRITEFUNC_PAUSE)
                     );

    mod->addConstant(type_int, "CURLFINFOFLAG_KNOWN_FILENAME",
                     static_cast<int>(CURLFINFOFLAG_KNOWN_FILENAME)
                     );

    mod->addConstant(type_int, "CURLFINFOFLAG_KNOWN_FILETYPE",
                     static_cast<int>(CURLFINFOFLAG_KNOWN_FILETYPE)
                     );

    mod->addConstant(type_int, "CURLFINFOFLAG_KNOWN_TIME",
                     static_cast<int>(CURLFINFOFLAG_KNOWN_TIME)
                     );

    mod->addConstant(type_int, "CURLFINFOFLAG_KNOWN_PERM",
                     static_cast<int>(CURLFINFOFLAG_KNOWN_PERM)
                     );

    mod->addConstant(type_int, "CURLFINFOFLAG_KNOWN_UID",
                     static_cast<int>(CURLFINFOFLAG_KNOWN_UID)
                     );

    mod->addConstant(type_int, "CURLFINFOFLAG_KNOWN_GID",
                     static_cast<int>(CURLFINFOFLAG_KNOWN_GID)
                     );

    mod->addConstant(type_int, "CURLFINFOFLAG_KNOWN_SIZE",
                     static_cast<int>(CURLFINFOFLAG_KNOWN_SIZE)
                     );

    mod->addConstant(type_int, "CURLFINFOFLAG_KNOWN_HLINKCOUNT",
                     static_cast<int>(CURLFINFOFLAG_KNOWN_HLINKCOUNT)
                     );

    mod->addConstant(type_int, "CURL_CHUNK_BGN_FUNC_OK",
                     static_cast<int>(CURL_CHUNK_BGN_FUNC_OK)
                     );

    mod->addConstant(type_int, "CURL_CHUNK_BGN_FUNC_FAIL",
                     static_cast<int>(CURL_CHUNK_BGN_FUNC_FAIL)
                     );

    mod->addConstant(type_int, "CURL_CHUNK_BGN_FUNC_SKIP",
                     static_cast<int>(CURL_CHUNK_BGN_FUNC_SKIP)
                     );

    mod->addConstant(type_int, "CURL_CHUNK_END_FUNC_OK",
                     static_cast<int>(CURL_CHUNK_END_FUNC_OK)
                     );

    mod->addConstant(type_int, "CURL_CHUNK_END_FUNC_FAIL",
                     static_cast<int>(CURL_CHUNK_END_FUNC_FAIL)
                     );

    mod->addConstant(type_int, "CURL_FNMATCHFUNC_MATCH",
                     static_cast<int>(CURL_FNMATCHFUNC_MATCH)
                     );

    mod->addConstant(type_int, "CURL_FNMATCHFUNC_NOMATCH",
                     static_cast<int>(CURL_FNMATCHFUNC_NOMATCH)
                     );

    mod->addConstant(type_int, "CURL_FNMATCHFUNC_FAIL",
                     static_cast<int>(CURL_FNMATCHFUNC_FAIL)
                     );

    mod->addConstant(type_int, "CURL_SEEKFUNC_OK",
                     static_cast<int>(CURL_SEEKFUNC_OK)
                     );

    mod->addConstant(type_int, "CURL_SEEKFUNC_FAIL",
                     static_cast<int>(CURL_SEEKFUNC_FAIL)
                     );

    mod->addConstant(type_int, "CURL_SEEKFUNC_CANTSEEK",
                     static_cast<int>(CURL_SEEKFUNC_CANTSEEK)
                     );

    mod->addConstant(type_int, "CURL_READFUNC_ABORT",
                     static_cast<int>(CURL_READFUNC_ABORT)
                     );

    mod->addConstant(type_int, "CURL_READFUNC_PAUSE",
                     static_cast<int>(CURL_READFUNC_PAUSE)
                     );

    mod->addConstant(type_int, "CURLIOE_OK",
                     static_cast<int>(CURLIOE_OK)
                     );

    mod->addConstant(type_int, "CURLIOE_UNKNOWNCMD",
                     static_cast<int>(CURLIOE_UNKNOWNCMD)
                     );

    mod->addConstant(type_int, "CURLIOE_FAILRESTART",
                     static_cast<int>(CURLIOE_FAILRESTART)
                     );

    mod->addConstant(type_int, "CURLIOE_LAST",
                     static_cast<int>(CURLIOE_LAST)
                     );

    mod->addConstant(type_int, "CURLIOCMD_NOP",
                     static_cast<int>(CURLIOCMD_NOP)
                     );

    mod->addConstant(type_int, "CURLIOCMD_RESTARTREAD",
                     static_cast<int>(CURLIOCMD_RESTARTREAD)
                     );

    mod->addConstant(type_int, "CURLINFO_TEXT",
                     static_cast<int>(CURLINFO_TEXT)
                     );

    mod->addConstant(type_int, "CURLINFO_HEADER_IN",
                     static_cast<int>(CURLINFO_HEADER_IN)
                     );

    mod->addConstant(type_int, "CURLINFO_HEADER_OUT",
                     static_cast<int>(CURLINFO_HEADER_OUT)
                     );

    mod->addConstant(type_int, "CURLINFO_DATA_IN",
                     static_cast<int>(CURLINFO_DATA_IN)
                     );

    mod->addConstant(type_int, "CURLINFO_DATA_OUT",
                     static_cast<int>(CURLINFO_DATA_OUT)
                     );

    mod->addConstant(type_int, "CURLINFO_SSL_DATA_IN",
                     static_cast<int>(CURLINFO_SSL_DATA_IN)
                     );

    mod->addConstant(type_int, "CURLINFO_SSL_DATA_OUT",
                     static_cast<int>(CURLINFO_SSL_DATA_OUT)
                     );

    mod->addConstant(type_int, "CURLINFO_END",
                     static_cast<int>(CURLINFO_END)
                     );

    mod->addConstant(type_int, "CURLE_OK",
                     static_cast<int>(CURLE_OK)
                     );

    mod->addConstant(type_int, "CURLE_UNSUPPORTED_PROTOCOL",
                     static_cast<int>(CURLE_UNSUPPORTED_PROTOCOL)
                     );

    mod->addConstant(type_int, "CURLE_FAILED_INIT",
                     static_cast<int>(CURLE_FAILED_INIT)
                     );

    mod->addConstant(type_int, "CURLE_URL_MALFORMAT",
                     static_cast<int>(CURLE_URL_MALFORMAT)
                     );

    mod->addConstant(type_int, "CURLE_NOT_BUILT_IN",
                     static_cast<int>(CURLE_NOT_BUILT_IN)
                     );

    mod->addConstant(type_int, "CURLE_COULDNT_RESOLVE_PROXY",
                     static_cast<int>(CURLE_COULDNT_RESOLVE_PROXY)
                     );

    mod->addConstant(type_int, "CURLE_COULDNT_RESOLVE_HOST",
                     static_cast<int>(CURLE_COULDNT_RESOLVE_HOST)
                     );

    mod->addConstant(type_int, "CURLE_COULDNT_CONNECT",
                     static_cast<int>(CURLE_COULDNT_CONNECT)
                     );

    mod->addConstant(type_int, "CURLE_FTP_WEIRD_SERVER_REPLY",
                     static_cast<int>(CURLE_FTP_WEIRD_SERVER_REPLY)
                     );

    mod->addConstant(type_int, "CURLE_REMOTE_ACCESS_DENIED",
                     static_cast<int>(CURLE_REMOTE_ACCESS_DENIED)
                     );

    mod->addConstant(type_int, "CURLE_FTP_WEIRD_PASS_REPLY",
                     static_cast<int>(CURLE_FTP_WEIRD_PASS_REPLY)
                     );

    mod->addConstant(type_int, "CURLE_FTP_WEIRD_PASV_REPLY",
                     static_cast<int>(CURLE_FTP_WEIRD_PASV_REPLY)
                     );

    mod->addConstant(type_int, "CURLE_FTP_WEIRD_227_FORMAT",
                     static_cast<int>(CURLE_FTP_WEIRD_227_FORMAT)
                     );

    mod->addConstant(type_int, "CURLE_FTP_CANT_GET_HOST",
                     static_cast<int>(CURLE_FTP_CANT_GET_HOST)
                     );

    mod->addConstant(type_int, "CURLE_FTP_COULDNT_SET_TYPE",
                     static_cast<int>(CURLE_FTP_COULDNT_SET_TYPE)
                     );

    mod->addConstant(type_int, "CURLE_PARTIAL_FILE",
                     static_cast<int>(CURLE_PARTIAL_FILE)
                     );

    mod->addConstant(type_int, "CURLE_FTP_COULDNT_RETR_FILE",
                     static_cast<int>(CURLE_FTP_COULDNT_RETR_FILE)
                     );

    mod->addConstant(type_int, "CURLE_QUOTE_ERROR",
                     static_cast<int>(CURLE_QUOTE_ERROR)
                     );

    mod->addConstant(type_int, "CURLE_HTTP_RETURNED_ERROR",
                     static_cast<int>(CURLE_HTTP_RETURNED_ERROR)
                     );

    mod->addConstant(type_int, "CURLE_WRITE_ERROR",
                     static_cast<int>(CURLE_WRITE_ERROR)
                     );

    mod->addConstant(type_int, "CURLE_UPLOAD_FAILED",
                     static_cast<int>(CURLE_UPLOAD_FAILED)
                     );

    mod->addConstant(type_int, "CURLE_READ_ERROR",
                     static_cast<int>(CURLE_READ_ERROR)
                     );

    mod->addConstant(type_int, "CURLE_OUT_OF_MEMORY",
                     static_cast<int>(CURLE_OUT_OF_MEMORY)
                     );

    mod->addConstant(type_int, "CURLE_OPERATION_TIMEDOUT",
                     static_cast<int>(CURLE_OPERATION_TIMEDOUT)
                     );

    mod->addConstant(type_int, "CURLE_FTP_PORT_FAILED",
                     static_cast<int>(CURLE_FTP_PORT_FAILED)
                     );

    mod->addConstant(type_int, "CURLE_FTP_COULDNT_USE_REST",
                     static_cast<int>(CURLE_FTP_COULDNT_USE_REST)
                     );

    mod->addConstant(type_int, "CURLE_RANGE_ERROR",
                     static_cast<int>(CURLE_RANGE_ERROR)
                     );

    mod->addConstant(type_int, "CURLE_HTTP_POST_ERROR",
                     static_cast<int>(CURLE_HTTP_POST_ERROR)
                     );

    mod->addConstant(type_int, "CURLE_SSL_CONNECT_ERROR",
                     static_cast<int>(CURLE_SSL_CONNECT_ERROR)
                     );

    mod->addConstant(type_int, "CURLE_BAD_DOWNLOAD_RESUME",
                     static_cast<int>(CURLE_BAD_DOWNLOAD_RESUME)
                     );

    mod->addConstant(type_int, "CURLE_FILE_COULDNT_READ_FILE",
                     static_cast<int>(CURLE_FILE_COULDNT_READ_FILE)
                     );

    mod->addConstant(type_int, "CURLE_LDAP_CANNOT_BIND",
                     static_cast<int>(CURLE_LDAP_CANNOT_BIND)
                     );

    mod->addConstant(type_int, "CURLE_LDAP_SEARCH_FAILED",
                     static_cast<int>(CURLE_LDAP_SEARCH_FAILED)
                     );

    mod->addConstant(type_int, "CURLE_FUNCTION_NOT_FOUND",
                     static_cast<int>(CURLE_FUNCTION_NOT_FOUND)
                     );

    mod->addConstant(type_int, "CURLE_ABORTED_BY_CALLBACK",
                     static_cast<int>(CURLE_ABORTED_BY_CALLBACK)
                     );

    mod->addConstant(type_int, "CURLE_BAD_FUNCTION_ARGUMENT",
                     static_cast<int>(CURLE_BAD_FUNCTION_ARGUMENT)
                     );

    mod->addConstant(type_int, "CURLE_INTERFACE_FAILED",
                     static_cast<int>(CURLE_INTERFACE_FAILED)
                     );

    mod->addConstant(type_int, "CURLE_TOO_MANY_REDIRECTS",
                     static_cast<int>(CURLE_TOO_MANY_REDIRECTS)
                     );

    mod->addConstant(type_int, "CURLE_UNKNOWN_OPTION",
                     static_cast<int>(CURLE_UNKNOWN_OPTION)
                     );

    mod->addConstant(type_int, "CURLE_TELNET_OPTION_SYNTAX",
                     static_cast<int>(CURLE_TELNET_OPTION_SYNTAX)
                     );

    mod->addConstant(type_int, "CURLE_PEER_FAILED_VERIFICATION",
                     static_cast<int>(CURLE_PEER_FAILED_VERIFICATION)
                     );

    mod->addConstant(type_int, "CURLE_GOT_NOTHING",
                     static_cast<int>(CURLE_GOT_NOTHING)
                     );

    mod->addConstant(type_int, "CURLE_SSL_ENGINE_NOTFOUND",
                     static_cast<int>(CURLE_SSL_ENGINE_NOTFOUND)
                     );

    mod->addConstant(type_int, "CURLE_SSL_ENGINE_SETFAILED",
                     static_cast<int>(CURLE_SSL_ENGINE_SETFAILED)
                     );

    mod->addConstant(type_int, "CURLE_SEND_ERROR",
                     static_cast<int>(CURLE_SEND_ERROR)
                     );

    mod->addConstant(type_int, "CURLE_RECV_ERROR",
                     static_cast<int>(CURLE_RECV_ERROR)
                     );

    mod->addConstant(type_int, "CURLE_SSL_CERTPROBLEM",
                     static_cast<int>(CURLE_SSL_CERTPROBLEM)
                     );

    mod->addConstant(type_int, "CURLE_SSL_CIPHER",
                     static_cast<int>(CURLE_SSL_CIPHER)
                     );

    mod->addConstant(type_int, "CURLE_SSL_CACERT",
                     static_cast<int>(CURLE_SSL_CACERT)
                     );

    mod->addConstant(type_int, "CURLE_BAD_CONTENT_ENCODING",
                     static_cast<int>(CURLE_BAD_CONTENT_ENCODING)
                     );

    mod->addConstant(type_int, "CURLE_LDAP_INVALID_URL",
                     static_cast<int>(CURLE_LDAP_INVALID_URL)
                     );

    mod->addConstant(type_int, "CURLE_FILESIZE_EXCEEDED",
                     static_cast<int>(CURLE_FILESIZE_EXCEEDED)
                     );

    mod->addConstant(type_int, "CURLE_USE_SSL_FAILED",
                     static_cast<int>(CURLE_USE_SSL_FAILED)
                     );

    mod->addConstant(type_int, "CURLE_SEND_FAIL_REWIND",
                     static_cast<int>(CURLE_SEND_FAIL_REWIND)
                     );

    mod->addConstant(type_int, "CURLE_SSL_ENGINE_INITFAILED",
                     static_cast<int>(CURLE_SSL_ENGINE_INITFAILED)
                     );

    mod->addConstant(type_int, "CURLE_LOGIN_DENIED",
                     static_cast<int>(CURLE_LOGIN_DENIED)
                     );

    mod->addConstant(type_int, "CURLE_TFTP_NOTFOUND",
                     static_cast<int>(CURLE_TFTP_NOTFOUND)
                     );

    mod->addConstant(type_int, "CURLE_TFTP_PERM",
                     static_cast<int>(CURLE_TFTP_PERM)
                     );

    mod->addConstant(type_int, "CURLE_REMOTE_DISK_FULL",
                     static_cast<int>(CURLE_REMOTE_DISK_FULL)
                     );

    mod->addConstant(type_int, "CURLE_TFTP_ILLEGAL",
                     static_cast<int>(CURLE_TFTP_ILLEGAL)
                     );

    mod->addConstant(type_int, "CURLE_TFTP_UNKNOWNID",
                     static_cast<int>(CURLE_TFTP_UNKNOWNID)
                     );

    mod->addConstant(type_int, "CURLE_REMOTE_FILE_EXISTS",
                     static_cast<int>(CURLE_REMOTE_FILE_EXISTS)
                     );

    mod->addConstant(type_int, "CURLE_TFTP_NOSUCHUSER",
                     static_cast<int>(CURLE_TFTP_NOSUCHUSER)
                     );

    mod->addConstant(type_int, "CURLE_CONV_FAILED",
                     static_cast<int>(CURLE_CONV_FAILED)
                     );

    mod->addConstant(type_int, "CURLE_CONV_REQD",
                     static_cast<int>(CURLE_CONV_REQD)
                     );

    mod->addConstant(type_int, "CURLE_SSL_CACERT_BADFILE",
                     static_cast<int>(CURLE_SSL_CACERT_BADFILE)
                     );

    mod->addConstant(type_int, "CURLE_REMOTE_FILE_NOT_FOUND",
                     static_cast<int>(CURLE_REMOTE_FILE_NOT_FOUND)
                     );

    mod->addConstant(type_int, "CURLE_SSH",
                     static_cast<int>(CURLE_SSH)
                     );

    mod->addConstant(type_int, "CURLE_SSL_SHUTDOWN_FAILED",
                     static_cast<int>(CURLE_SSL_SHUTDOWN_FAILED)
                     );

    mod->addConstant(type_int, "CURLE_AGAIN",
                     static_cast<int>(CURLE_AGAIN)
                     );

    mod->addConstant(type_int, "CURLE_SSL_CRL_BADFILE",
                     static_cast<int>(CURLE_SSL_CRL_BADFILE)
                     );

    mod->addConstant(type_int, "CURLE_SSL_ISSUER_ERROR",
                     static_cast<int>(CURLE_SSL_ISSUER_ERROR)
                     );

    mod->addConstant(type_int, "CURLE_FTP_PRET_FAILED",
                     static_cast<int>(CURLE_FTP_PRET_FAILED)
                     );

    mod->addConstant(type_int, "CURLE_RTSP_CSEQ_ERROR",
                     static_cast<int>(CURLE_RTSP_CSEQ_ERROR)
                     );

    mod->addConstant(type_int, "CURLE_RTSP_SESSION_ERROR",
                     static_cast<int>(CURLE_RTSP_SESSION_ERROR)
                     );

    mod->addConstant(type_int, "CURLE_FTP_BAD_FILE_LIST",
                     static_cast<int>(CURLE_FTP_BAD_FILE_LIST)
                     );

    mod->addConstant(type_int, "CURLE_CHUNK_FAILED",
                     static_cast<int>(CURLE_CHUNK_FAILED)
                     );

    mod->addConstant(type_int, "CURLPROXY_HTTP",
                     static_cast<int>(CURLPROXY_HTTP)
                     );

    mod->addConstant(type_int, "CURLPROXY_HTTP_1_0",
                     static_cast<int>(CURLPROXY_HTTP_1_0)
                     );

    mod->addConstant(type_int, "CURLPROXY_SOCKS4",
                     static_cast<int>(CURLPROXY_SOCKS4)
                     );

    mod->addConstant(type_int, "CURLPROXY_SOCKS5",
                     static_cast<int>(CURLPROXY_SOCKS5)
                     );

    mod->addConstant(type_int, "CURLPROXY_SOCKS4A",
                     static_cast<int>(CURLPROXY_SOCKS4A)
                     );

    mod->addConstant(type_int, "CURLPROXY_SOCKS5_HOSTNAME",
                     static_cast<int>(CURLPROXY_SOCKS5_HOSTNAME)
                     );

    mod->addConstant(type_int, "CURLGSSAPI_DELEGATION_NONE",
                     static_cast<int>(CURLGSSAPI_DELEGATION_NONE)
                     );

    mod->addConstant(type_int, "CURLGSSAPI_DELEGATION_POLICY_FLAG",
                     static_cast<int>(CURLGSSAPI_DELEGATION_POLICY_FLAG)
                     );

    mod->addConstant(type_int, "CURLGSSAPI_DELEGATION_FLAG",
                     static_cast<int>(CURLGSSAPI_DELEGATION_FLAG)
                     );

    mod->addConstant(type_int, "CURL_ERROR_SIZE",
                     static_cast<int>(CURL_ERROR_SIZE)
                     );

    mod->addConstant(type_int, "CURLKHTYPE_UNKNOWN",
                     static_cast<int>(0)
                     );

    mod->addConstant(type_int, "CURLKHTYPE_RSA1",
                     static_cast<int>(1)
                     );

    mod->addConstant(type_int, "CURLKHTYPE_RSA",
                     static_cast<int>(2)
                     );

    mod->addConstant(type_int, "CURLKHTYPE_DSS",
                     static_cast<int>(3)
                     );

    mod->addConstant(type_int, "CURLKHSTAT_FINE_ADD_TO_FILE",
                     static_cast<int>(CURLKHSTAT_FINE_ADD_TO_FILE)
                     );

    mod->addConstant(type_int, "CURLKHSTAT_FINE",
                     static_cast<int>(CURLKHSTAT_FINE)
                     );

    mod->addConstant(type_int, "CURLKHSTAT_REJECT",
                     static_cast<int>(CURLKHSTAT_REJECT)
                     );

    mod->addConstant(type_int, "CURLKHSTAT_DEFER",
                     static_cast<int>(CURLKHSTAT_DEFER)
                     );

    mod->addConstant(type_int, "CURLKHSTAT_LAST",
                     static_cast<int>(CURLKHSTAT_LAST)
                     );

    mod->addConstant(type_int, "CURLKHMATCH_OK",
                     static_cast<int>(CURLKHMATCH_OK)
                     );

    mod->addConstant(type_int, "CURLKHMATCH_MISMATCH",
                     static_cast<int>(CURLKHMATCH_MISMATCH)
                     );

    mod->addConstant(type_int, "CURLKHMATCH_MISSING",
                     static_cast<int>(CURLKHMATCH_MISSING)
                     );

    mod->addConstant(type_int, "CURLUSESSL_NONE",
                     static_cast<int>(CURLUSESSL_NONE)
                     );

    mod->addConstant(type_int, "CURLUSESSL_TRY",
                     static_cast<int>(CURLUSESSL_TRY)
                     );

    mod->addConstant(type_int, "CURLUSESSL_CONTROL",
                     static_cast<int>(CURLUSESSL_CONTROL)
                     );

    mod->addConstant(type_int, "CURLUSESSL_ALL",
                     static_cast<int>(CURLUSESSL_ALL)
                     );

    mod->addConstant(type_int, "CURLUSESSL_LAST",
                     static_cast<int>(CURLUSESSL_LAST)
                     );

    mod->addConstant(type_int, "CURLFTPSSL_CCC_NONE",
                     static_cast<int>(CURLFTPSSL_CCC_NONE)
                     );

    mod->addConstant(type_int, "CURLFTPSSL_CCC_PASSIVE",
                     static_cast<int>(CURLFTPSSL_CCC_PASSIVE)
                     );

    mod->addConstant(type_int, "CURLFTPSSL_CCC_ACTIVE",
                     static_cast<int>(CURLFTPSSL_CCC_ACTIVE)
                     );

    mod->addConstant(type_int, "CURLFTPAUTH_DEFAULT",
                     static_cast<int>(CURLFTPAUTH_DEFAULT)
                     );

    mod->addConstant(type_int, "CURLFTPAUTH_SSL",
                     static_cast<int>(CURLFTPAUTH_SSL)
                     );

    mod->addConstant(type_int, "CURLFTPAUTH_TLS",
                     static_cast<int>(CURLFTPAUTH_TLS)
                     );

    mod->addConstant(type_int, "CURLFTPAUTH_LAST",
                     static_cast<int>(CURLFTPAUTH_LAST)
                     );

    mod->addConstant(type_int, "CURLFTP_CREATE_DIR_NONE",
                     static_cast<int>(CURLFTP_CREATE_DIR_NONE)
                     );

    mod->addConstant(type_int, "CURLFTP_CREATE_DIR",
                     static_cast<int>(CURLFTP_CREATE_DIR)
                     );

    mod->addConstant(type_int, "CURLFTP_CREATE_DIR_RETRY",
                     static_cast<int>(CURLFTP_CREATE_DIR_RETRY)
                     );

    mod->addConstant(type_int, "CURLFTPMETHOD_DEFAULT",
                     static_cast<int>(CURLFTPMETHOD_DEFAULT)
                     );

    mod->addConstant(type_int, "CURLFTPMETHOD_MULTICWD",
                     static_cast<int>(CURLFTPMETHOD_MULTICWD)
                     );

    mod->addConstant(type_int, "CURLFTPMETHOD_NOCWD",
                     static_cast<int>(CURLFTPMETHOD_NOCWD)
                     );

    mod->addConstant(type_int, "CURLFTPMETHOD_SINGLECWD",
                     static_cast<int>(CURLFTPMETHOD_SINGLECWD)
                     );

    mod->addConstant(type_int, "CURLPROTO_HTTP",
                     static_cast<int>(CURLPROTO_HTTP)
                     );

    mod->addConstant(type_int, "CURLPROTO_HTTPS",
                     static_cast<int>(CURLPROTO_HTTPS)
                     );

    mod->addConstant(type_int, "CURLPROTO_FTP",
                     static_cast<int>(CURLPROTO_FTP)
                     );

    mod->addConstant(type_int, "CURLPROTO_FTPS",
                     static_cast<int>(CURLPROTO_FTPS)
                     );

    mod->addConstant(type_int, "CURLPROTO_SCP",
                     static_cast<int>(CURLPROTO_SCP)
                     );

    mod->addConstant(type_int, "CURLPROTO_SFTP",
                     static_cast<int>(CURLPROTO_SFTP)
                     );

    mod->addConstant(type_int, "CURLPROTO_TELNET",
                     static_cast<int>(CURLPROTO_TELNET)
                     );

    mod->addConstant(type_int, "CURLPROTO_LDAP",
                     static_cast<int>(CURLPROTO_LDAP)
                     );

    mod->addConstant(type_int, "CURLPROTO_LDAPS",
                     static_cast<int>(CURLPROTO_LDAPS)
                     );

    mod->addConstant(type_int, "CURLPROTO_DICT",
                     static_cast<int>(CURLPROTO_DICT)
                     );

    mod->addConstant(type_int, "CURLPROTO_FILE",
                     static_cast<int>(CURLPROTO_FILE)
                     );

    mod->addConstant(type_int, "CURLPROTO_TFTP",
                     static_cast<int>(CURLPROTO_TFTP)
                     );

    mod->addConstant(type_int, "CURLPROTO_IMAP",
                     static_cast<int>(CURLPROTO_IMAP)
                     );

    mod->addConstant(type_int, "CURLPROTO_IMAPS",
                     static_cast<int>(CURLPROTO_IMAPS)
                     );

    mod->addConstant(type_int, "CURLPROTO_POP3",
                     static_cast<int>(CURLPROTO_POP3)
                     );

    mod->addConstant(type_int, "CURLPROTO_POP3S",
                     static_cast<int>(CURLPROTO_POP3S)
                     );

    mod->addConstant(type_int, "CURLPROTO_SMTP",
                     static_cast<int>(CURLPROTO_SMTP)
                     );

    mod->addConstant(type_int, "CURLPROTO_SMTPS",
                     static_cast<int>(CURLPROTO_SMTPS)
                     );

    mod->addConstant(type_int, "CURLPROTO_RTSP",
                     static_cast<int>(CURLPROTO_RTSP)
                     );

    mod->addConstant(type_int, "CURLPROTO_RTMP",
                     static_cast<int>(CURLPROTO_RTMP)
                     );

    mod->addConstant(type_int, "CURLPROTO_RTMPT",
                     static_cast<int>(CURLPROTO_RTMPT)
                     );

    mod->addConstant(type_int, "CURLPROTO_RTMPE",
                     static_cast<int>(CURLPROTO_RTMPE)
                     );

    mod->addConstant(type_int, "CURLPROTO_RTMPTE",
                     static_cast<int>(CURLPROTO_RTMPTE)
                     );

    mod->addConstant(type_int, "CURLPROTO_RTMPS",
                     static_cast<int>(CURLPROTO_RTMPS)
                     );

    mod->addConstant(type_int, "CURLPROTO_RTMPTS",
                     static_cast<int>(CURLPROTO_RTMPTS)
                     );

    mod->addConstant(type_int, "CURLPROTO_GOPHER",
                     static_cast<int>(CURLPROTO_GOPHER)
                     );

    mod->addConstant(type_int, "CURLPROTO_ALL",
                     static_cast<int>(CURLPROTO_ALL)
                     );

    mod->addConstant(type_int, "CURLOPTTYPE_LONG",
                     static_cast<int>(CURLOPTTYPE_LONG)
                     );

    mod->addConstant(type_int, "CURLOPTTYPE_OBJECTPOINT",
                     static_cast<int>(CURLOPTTYPE_OBJECTPOINT)
                     );

    mod->addConstant(type_int, "CURLOPTTYPE_FUNCTIONPOINT",
                     static_cast<int>(CURLOPTTYPE_FUNCTIONPOINT)
                     );

    mod->addConstant(type_int, "CURLOPTTYPE_OFF_T",
                     static_cast<int>(CURLOPTTYPE_OFF_T)
                     );

    mod->addConstant(type_int, "CURLOPT_FILE",
                     static_cast<int>(CURLOPT_FILE)
                     );

    mod->addConstant(type_int, "CURLOPT_URL",
                     static_cast<int>(CURLOPT_URL)
                     );

    mod->addConstant(type_int, "CURLOPT_PORT",
                     static_cast<int>(CURLOPT_PORT)
                     );

    mod->addConstant(type_int, "CURLOPT_PROXY",
                     static_cast<int>(CURLOPT_PROXY)
                     );

    mod->addConstant(type_int, "CURLOPT_USERPWD",
                     static_cast<int>(CURLOPT_USERPWD)
                     );

    mod->addConstant(type_int, "CURLOPT_PROXYUSERPWD",
                     static_cast<int>(CURLOPT_PROXYUSERPWD)
                     );

    mod->addConstant(type_int, "CURLOPT_RANGE",
                     static_cast<int>(CURLOPT_RANGE)
                     );

    mod->addConstant(type_int, "CURLOPT_INFILE",
                     static_cast<int>(CURLOPT_INFILE)
                     );

    mod->addConstant(type_int, "CURLOPT_ERRORBUFFER",
                     static_cast<int>(CURLOPT_ERRORBUFFER)
                     );

    mod->addConstant(type_int, "CURLOPT_WRITEFUNCTION",
                     static_cast<int>(CURLOPT_WRITEFUNCTION)
                     );

    mod->addConstant(type_int, "CURLOPT_READFUNCTION",
                     static_cast<int>(CURLOPT_READFUNCTION)
                     );

    mod->addConstant(type_int, "CURLOPT_TIMEOUT",
                     static_cast<int>(CURLOPT_TIMEOUT)
                     );

    mod->addConstant(type_int, "CURLOPT_INFILESIZE",
                     static_cast<int>(CURLOPT_INFILESIZE)
                     );

    mod->addConstant(type_int, "CURLOPT_POSTFIELDS",
                     static_cast<int>(CURLOPT_POSTFIELDS)
                     );

    mod->addConstant(type_int, "CURLOPT_REFERER",
                     static_cast<int>(CURLOPT_REFERER)
                     );

    mod->addConstant(type_int, "CURLOPT_FTPPORT",
                     static_cast<int>(CURLOPT_FTPPORT)
                     );

    mod->addConstant(type_int, "CURLOPT_USERAGENT",
                     static_cast<int>(CURLOPT_USERAGENT)
                     );

    mod->addConstant(type_int, "CURLOPT_LOW_SPEED_LIMIT",
                     static_cast<int>(CURLOPT_LOW_SPEED_LIMIT)
                     );

    mod->addConstant(type_int, "CURLOPT_LOW_SPEED_TIME",
                     static_cast<int>(CURLOPT_LOW_SPEED_TIME)
                     );

    mod->addConstant(type_int, "CURLOPT_RESUME_FROM",
                     static_cast<int>(CURLOPT_RESUME_FROM)
                     );

    mod->addConstant(type_int, "CURLOPT_COOKIE",
                     static_cast<int>(CURLOPT_COOKIE)
                     );

    mod->addConstant(type_int, "CURLOPT_HTTPHEADER",
                     static_cast<int>(CURLOPT_HTTPHEADER)
                     );

    mod->addConstant(type_int, "CURLOPT_HTTPPOST",
                     static_cast<int>(CURLOPT_HTTPPOST)
                     );

    mod->addConstant(type_int, "CURLOPT_SSLCERT",
                     static_cast<int>(CURLOPT_SSLCERT)
                     );

    mod->addConstant(type_int, "CURLOPT_KEYPASSWD",
                     static_cast<int>(CURLOPT_KEYPASSWD)
                     );

    mod->addConstant(type_int, "CURLOPT_CRLF",
                     static_cast<int>(CURLOPT_CRLF)
                     );

    mod->addConstant(type_int, "CURLOPT_QUOTE",
                     static_cast<int>(CURLOPT_QUOTE)
                     );

    mod->addConstant(type_int, "CURLOPT_WRITEHEADER",
                     static_cast<int>(CURLOPT_WRITEHEADER)
                     );

    mod->addConstant(type_int, "CURLOPT_COOKIEFILE",
                     static_cast<int>(CURLOPT_COOKIEFILE)
                     );

    mod->addConstant(type_int, "CURLOPT_SSLVERSION",
                     static_cast<int>(CURLOPT_SSLVERSION)
                     );

    mod->addConstant(type_int, "CURLOPT_TIMECONDITION",
                     static_cast<int>(CURLOPT_TIMECONDITION)
                     );

    mod->addConstant(type_int, "CURLOPT_TIMEVALUE",
                     static_cast<int>(CURLOPT_TIMEVALUE)
                     );

    mod->addConstant(type_int, "CURLOPT_CUSTOMREQUEST",
                     static_cast<int>(CURLOPT_CUSTOMREQUEST)
                     );

    mod->addConstant(type_int, "CURLOPT_STDERR",
                     static_cast<int>(CURLOPT_STDERR)
                     );

    mod->addConstant(type_int, "CURLOPT_POSTQUOTE",
                     static_cast<int>(CURLOPT_POSTQUOTE)
                     );

    mod->addConstant(type_int, "CURLOPT_VERBOSE",
                     static_cast<int>(CURLOPT_VERBOSE)
                     );

    mod->addConstant(type_int, "CURLOPT_HEADER",
                     static_cast<int>(CURLOPT_HEADER)
                     );

    mod->addConstant(type_int, "CURLOPT_NOPROGRESS",
                     static_cast<int>(CURLOPT_NOPROGRESS)
                     );

    mod->addConstant(type_int, "CURLOPT_NOBODY",
                     static_cast<int>(CURLOPT_NOBODY)
                     );

    mod->addConstant(type_int, "CURLOPT_FAILONERROR",
                     static_cast<int>(CURLOPT_FAILONERROR)
                     );

    mod->addConstant(type_int, "CURLOPT_UPLOAD",
                     static_cast<int>(CURLOPT_UPLOAD)
                     );

    mod->addConstant(type_int, "CURLOPT_POST",
                     static_cast<int>(CURLOPT_POST)
                     );

    mod->addConstant(type_int, "CURLOPT_DIRLISTONLY",
                     static_cast<int>(CURLOPT_DIRLISTONLY)
                     );

    mod->addConstant(type_int, "CURLOPT_APPEND",
                     static_cast<int>(CURLOPT_APPEND)
                     );

    mod->addConstant(type_int, "CURLOPT_NETRC",
                     static_cast<int>(CURLOPT_NETRC)
                     );

    mod->addConstant(type_int, "CURLOPT_FOLLOWLOCATION",
                     static_cast<int>(CURLOPT_FOLLOWLOCATION)
                     );

    mod->addConstant(type_int, "CURLOPT_TRANSFERTEXT",
                     static_cast<int>(CURLOPT_TRANSFERTEXT)
                     );

    mod->addConstant(type_int, "CURLOPT_PUT",
                     static_cast<int>(CURLOPT_PUT)
                     );

    mod->addConstant(type_int, "CURLOPT_PROGRESSFUNCTION",
                     static_cast<int>(CURLOPT_PROGRESSFUNCTION)
                     );

    mod->addConstant(type_int, "CURLOPT_PROGRESSDATA",
                     static_cast<int>(CURLOPT_PROGRESSDATA)
                     );

    mod->addConstant(type_int, "CURLOPT_AUTOREFERER",
                     static_cast<int>(CURLOPT_AUTOREFERER)
                     );

    mod->addConstant(type_int, "CURLOPT_PROXYPORT",
                     static_cast<int>(CURLOPT_PROXYPORT)
                     );

    mod->addConstant(type_int, "CURLOPT_POSTFIELDSIZE",
                     static_cast<int>(CURLOPT_POSTFIELDSIZE)
                     );

    mod->addConstant(type_int, "CURLOPT_HTTPPROXYTUNNEL",
                     static_cast<int>(CURLOPT_HTTPPROXYTUNNEL)
                     );

    mod->addConstant(type_int, "CURLOPT_INTERFACE",
                     static_cast<int>(CURLOPT_INTERFACE)
                     );

    mod->addConstant(type_int, "CURLOPT_KRBLEVEL",
                     static_cast<int>(CURLOPT_KRBLEVEL)
                     );

    mod->addConstant(type_int, "CURLOPT_SSL_VERIFYPEER",
                     static_cast<int>(CURLOPT_SSL_VERIFYPEER)
                     );

    mod->addConstant(type_int, "CURLOPT_CAINFO",
                     static_cast<int>(CURLOPT_CAINFO)
                     );

    mod->addConstant(type_int, "CURLOPT_MAXREDIRS",
                     static_cast<int>(CURLOPT_MAXREDIRS)
                     );

    mod->addConstant(type_int, "CURLOPT_FILETIME",
                     static_cast<int>(CURLOPT_FILETIME)
                     );

    mod->addConstant(type_int, "CURLOPT_TELNETOPTIONS",
                     static_cast<int>(CURLOPT_TELNETOPTIONS)
                     );

    mod->addConstant(type_int, "CURLOPT_MAXCONNECTS",
                     static_cast<int>(CURLOPT_MAXCONNECTS)
                     );

    mod->addConstant(type_int, "CURLOPT_FRESH_CONNECT",
                     static_cast<int>(CURLOPT_FRESH_CONNECT)
                     );

    mod->addConstant(type_int, "CURLOPT_FORBID_REUSE",
                     static_cast<int>(CURLOPT_FORBID_REUSE)
                     );

    mod->addConstant(type_int, "CURLOPT_RANDOM_FILE",
                     static_cast<int>(CURLOPT_RANDOM_FILE)
                     );

    mod->addConstant(type_int, "CURLOPT_EGDSOCKET",
                     static_cast<int>(CURLOPT_EGDSOCKET)
                     );

    mod->addConstant(type_int, "CURLOPT_CONNECTTIMEOUT",
                     static_cast<int>(CURLOPT_CONNECTTIMEOUT)
                     );

    mod->addConstant(type_int, "CURLOPT_HEADERFUNCTION",
                     static_cast<int>(CURLOPT_HEADERFUNCTION)
                     );

    mod->addConstant(type_int, "CURLOPT_HTTPGET",
                     static_cast<int>(CURLOPT_HTTPGET)
                     );

    mod->addConstant(type_int, "CURLOPT_SSL_VERIFYHOST",
                     static_cast<int>(CURLOPT_SSL_VERIFYHOST)
                     );

    mod->addConstant(type_int, "CURLOPT_COOKIEJAR",
                     static_cast<int>(CURLOPT_COOKIEJAR)
                     );

    mod->addConstant(type_int, "CURLOPT_SSL_CIPHER_LIST",
                     static_cast<int>(CURLOPT_SSL_CIPHER_LIST)
                     );

    mod->addConstant(type_int, "CURLOPT_HTTP_VERSION",
                     static_cast<int>(CURLOPT_HTTP_VERSION)
                     );

    mod->addConstant(type_int, "CURLOPT_FTP_USE_EPSV",
                     static_cast<int>(CURLOPT_FTP_USE_EPSV)
                     );

    mod->addConstant(type_int, "CURLOPT_SSLCERTTYPE",
                     static_cast<int>(CURLOPT_SSLCERTTYPE)
                     );

    mod->addConstant(type_int, "CURLOPT_SSLKEY",
                     static_cast<int>(CURLOPT_SSLKEY)
                     );

    mod->addConstant(type_int, "CURLOPT_SSLKEYTYPE",
                     static_cast<int>(CURLOPT_SSLKEYTYPE)
                     );

    mod->addConstant(type_int, "CURLOPT_SSLENGINE",
                     static_cast<int>(CURLOPT_SSLENGINE)
                     );

    mod->addConstant(type_int, "CURLOPT_SSLENGINE_DEFAULT",
                     static_cast<int>(CURLOPT_SSLENGINE_DEFAULT)
                     );

    mod->addConstant(type_int, "CURLOPT_DNS_CACHE_TIMEOUT",
                     static_cast<int>(CURLOPT_DNS_CACHE_TIMEOUT)
                     );

    mod->addConstant(type_int, "CURLOPT_PREQUOTE",
                     static_cast<int>(CURLOPT_PREQUOTE)
                     );

    mod->addConstant(type_int, "CURLOPT_DEBUGFUNCTION",
                     static_cast<int>(CURLOPT_DEBUGFUNCTION)
                     );

    mod->addConstant(type_int, "CURLOPT_DEBUGDATA",
                     static_cast<int>(CURLOPT_DEBUGDATA)
                     );

    mod->addConstant(type_int, "CURLOPT_COOKIESESSION",
                     static_cast<int>(CURLOPT_COOKIESESSION)
                     );

    mod->addConstant(type_int, "CURLOPT_CAPATH",
                     static_cast<int>(CURLOPT_CAPATH)
                     );

    mod->addConstant(type_int, "CURLOPT_BUFFERSIZE",
                     static_cast<int>(CURLOPT_BUFFERSIZE)
                     );

    mod->addConstant(type_int, "CURLOPT_NOSIGNAL",
                     static_cast<int>(CURLOPT_NOSIGNAL)
                     );

    mod->addConstant(type_int, "CURLOPT_SHARE",
                     static_cast<int>(CURLOPT_SHARE)
                     );

    mod->addConstant(type_int, "CURLOPT_PROXYTYPE",
                     static_cast<int>(CURLOPT_PROXYTYPE)
                     );

    mod->addConstant(type_int, "CURLOPT_ACCEPT_ENCODING",
                     static_cast<int>(CURLOPT_ACCEPT_ENCODING)
                     );

    mod->addConstant(type_int, "CURLOPT_PRIVATE",
                     static_cast<int>(CURLOPT_PRIVATE)
                     );

    mod->addConstant(type_int, "CURLOPT_HTTP200ALIASES",
                     static_cast<int>(CURLOPT_HTTP200ALIASES)
                     );

    mod->addConstant(type_int, "CURLOPT_UNRESTRICTED_AUTH",
                     static_cast<int>(CURLOPT_UNRESTRICTED_AUTH)
                     );

    mod->addConstant(type_int, "CURLOPT_FTP_USE_EPRT",
                     static_cast<int>(CURLOPT_FTP_USE_EPRT)
                     );

    mod->addConstant(type_int, "CURLOPT_HTTPAUTH",
                     static_cast<int>(CURLOPT_HTTPAUTH)
                     );

    mod->addConstant(type_int, "CURLOPT_SSL_CTX_FUNCTION",
                     static_cast<int>(CURLOPT_SSL_CTX_FUNCTION)
                     );

    mod->addConstant(type_int, "CURLOPT_SSL_CTX_DATA",
                     static_cast<int>(CURLOPT_SSL_CTX_DATA)
                     );

    mod->addConstant(type_int, "CURLOPT_FTP_CREATE_MISSING_DIRS",
                     static_cast<int>(CURLOPT_FTP_CREATE_MISSING_DIRS)
                     );

    mod->addConstant(type_int, "CURLOPT_PROXYAUTH",
                     static_cast<int>(CURLOPT_PROXYAUTH)
                     );

    mod->addConstant(type_int, "CURLOPT_FTP_RESPONSE_TIMEOUT",
                     static_cast<int>(CURLOPT_FTP_RESPONSE_TIMEOUT)
                     );

    mod->addConstant(type_int, "CURLOPT_SERVER_RESPONSE_TIMEOUT",
                     static_cast<int>(CURLOPT_SERVER_RESPONSE_TIMEOUT)
                     );

    mod->addConstant(type_int, "CURLOPT_IPRESOLVE",
                     static_cast<int>(CURLOPT_IPRESOLVE)
                     );

    mod->addConstant(type_int, "CURLOPT_MAXFILESIZE",
                     static_cast<int>(CURLOPT_MAXFILESIZE)
                     );

    mod->addConstant(type_int, "CURLOPT_INFILESIZE_LARGE",
                     static_cast<int>(CURLOPT_INFILESIZE_LARGE)
                     );

    mod->addConstant(type_int, "CURLOPT_RESUME_FROM_LARGE",
                     static_cast<int>(CURLOPT_RESUME_FROM_LARGE)
                     );

    mod->addConstant(type_int, "CURLOPT_MAXFILESIZE_LARGE",
                     static_cast<int>(CURLOPT_MAXFILESIZE_LARGE)
                     );

    mod->addConstant(type_int, "CURLOPT_NETRC_FILE",
                     static_cast<int>(CURLOPT_NETRC_FILE)
                     );

    mod->addConstant(type_int, "CURLOPT_USE_SSL",
                     static_cast<int>(CURLOPT_USE_SSL)
                     );

    mod->addConstant(type_int, "CURLOPT_POSTFIELDSIZE_LARGE",
                     static_cast<int>(CURLOPT_POSTFIELDSIZE_LARGE)
                     );

    mod->addConstant(type_int, "CURLOPT_TCP_NODELAY",
                     static_cast<int>(CURLOPT_TCP_NODELAY)
                     );

    mod->addConstant(type_int, "CURLOPT_FTPSSLAUTH",
                     static_cast<int>(CURLOPT_FTPSSLAUTH)
                     );

    mod->addConstant(type_int, "CURLOPT_IOCTLFUNCTION",
                     static_cast<int>(CURLOPT_IOCTLFUNCTION)
                     );

    mod->addConstant(type_int, "CURLOPT_IOCTLDATA",
                     static_cast<int>(CURLOPT_IOCTLDATA)
                     );

    mod->addConstant(type_int, "CURLOPT_FTP_ACCOUNT",
                     static_cast<int>(CURLOPT_FTP_ACCOUNT)
                     );

    mod->addConstant(type_int, "CURLOPT_COOKIELIST",
                     static_cast<int>(CURLOPT_COOKIELIST)
                     );

    mod->addConstant(type_int, "CURLOPT_IGNORE_CONTENT_LENGTH",
                     static_cast<int>(CURLOPT_IGNORE_CONTENT_LENGTH)
                     );

    mod->addConstant(type_int, "CURLOPT_FTP_SKIP_PASV_IP",
                     static_cast<int>(CURLOPT_FTP_SKIP_PASV_IP)
                     );

    mod->addConstant(type_int, "CURLOPT_FTP_FILEMETHOD",
                     static_cast<int>(CURLOPT_FTP_FILEMETHOD)
                     );

    mod->addConstant(type_int, "CURLOPT_LOCALPORT",
                     static_cast<int>(CURLOPT_LOCALPORT)
                     );

    mod->addConstant(type_int, "CURLOPT_LOCALPORTRANGE",
                     static_cast<int>(CURLOPT_LOCALPORTRANGE)
                     );

    mod->addConstant(type_int, "CURLOPT_CONNECT_ONLY",
                     static_cast<int>(CURLOPT_CONNECT_ONLY)
                     );

    mod->addConstant(type_int, "CURLOPT_CONV_FROM_NETWORK_FUNCTION",
                     static_cast<int>(CURLOPT_CONV_FROM_NETWORK_FUNCTION)
                     );

    mod->addConstant(type_int, "CURLOPT_CONV_TO_NETWORK_FUNCTION",
                     static_cast<int>(CURLOPT_CONV_TO_NETWORK_FUNCTION)
                     );

    mod->addConstant(type_int, "CURLOPT_CONV_FROM_UTF8_FUNCTION",
                     static_cast<int>(CURLOPT_CONV_FROM_UTF8_FUNCTION)
                     );

    mod->addConstant(type_int, "CURLOPT_MAX_SEND_SPEED_LARGE",
                     static_cast<int>(CURLOPT_MAX_SEND_SPEED_LARGE)
                     );

    mod->addConstant(type_int, "CURLOPT_MAX_RECV_SPEED_LARGE",
                     static_cast<int>(CURLOPT_MAX_RECV_SPEED_LARGE)
                     );

    mod->addConstant(type_int, "CURLOPT_FTP_ALTERNATIVE_TO_USER",
                     static_cast<int>(CURLOPT_FTP_ALTERNATIVE_TO_USER)
                     );

    mod->addConstant(type_int, "CURLOPT_SOCKOPTFUNCTION",
                     static_cast<int>(CURLOPT_SOCKOPTFUNCTION)
                     );

    mod->addConstant(type_int, "CURLOPT_SOCKOPTDATA",
                     static_cast<int>(CURLOPT_SOCKOPTDATA)
                     );

    mod->addConstant(type_int, "CURLOPT_SSL_SESSIONID_CACHE",
                     static_cast<int>(CURLOPT_SSL_SESSIONID_CACHE)
                     );

    mod->addConstant(type_int, "CURLOPT_SSH_AUTH_TYPES",
                     static_cast<int>(CURLOPT_SSH_AUTH_TYPES)
                     );

    mod->addConstant(type_int, "CURLOPT_SSH_PUBLIC_KEYFILE",
                     static_cast<int>(CURLOPT_SSH_PUBLIC_KEYFILE)
                     );

    mod->addConstant(type_int, "CURLOPT_SSH_PRIVATE_KEYFILE",
                     static_cast<int>(CURLOPT_SSH_PRIVATE_KEYFILE)
                     );

    mod->addConstant(type_int, "CURLOPT_FTP_SSL_CCC",
                     static_cast<int>(CURLOPT_FTP_SSL_CCC)
                     );

    mod->addConstant(type_int, "CURLOPT_TIMEOUT_MS",
                     static_cast<int>(CURLOPT_TIMEOUT_MS)
                     );

    mod->addConstant(type_int, "CURLOPT_CONNECTTIMEOUT_MS",
                     static_cast<int>(CURLOPT_CONNECTTIMEOUT_MS)
                     );

    mod->addConstant(type_int, "CURLOPT_HTTP_TRANSFER_DECODING",
                     static_cast<int>(CURLOPT_HTTP_TRANSFER_DECODING)
                     );

    mod->addConstant(type_int, "CURLOPT_HTTP_CONTENT_DECODING",
                     static_cast<int>(CURLOPT_HTTP_CONTENT_DECODING)
                     );

    mod->addConstant(type_int, "CURLOPT_NEW_FILE_PERMS",
                     static_cast<int>(CURLOPT_NEW_FILE_PERMS)
                     );

    mod->addConstant(type_int, "CURLOPT_NEW_DIRECTORY_PERMS",
                     static_cast<int>(CURLOPT_NEW_DIRECTORY_PERMS)
                     );

    mod->addConstant(type_int, "CURLOPT_POSTREDIR",
                     static_cast<int>(CURLOPT_POSTREDIR)
                     );

    mod->addConstant(type_int, "CURLOPT_SSH_HOST_PUBLIC_KEY_MD5",
                     static_cast<int>(CURLOPT_SSH_HOST_PUBLIC_KEY_MD5)
                     );

    mod->addConstant(type_int, "CURLOPT_OPENSOCKETFUNCTION",
                     static_cast<int>(CURLOPT_OPENSOCKETFUNCTION)
                     );

    mod->addConstant(type_int, "CURLOPT_OPENSOCKETDATA",
                     static_cast<int>(CURLOPT_OPENSOCKETDATA)
                     );

    mod->addConstant(type_int, "CURLOPT_COPYPOSTFIELDS",
                     static_cast<int>(CURLOPT_COPYPOSTFIELDS)
                     );

    mod->addConstant(type_int, "CURLOPT_PROXY_TRANSFER_MODE",
                     static_cast<int>(CURLOPT_PROXY_TRANSFER_MODE)
                     );

    mod->addConstant(type_int, "CURLOPT_SEEKFUNCTION",
                     static_cast<int>(CURLOPT_SEEKFUNCTION)
                     );

    mod->addConstant(type_int, "CURLOPT_SEEKDATA",
                     static_cast<int>(CURLOPT_SEEKDATA)
                     );

    mod->addConstant(type_int, "CURLOPT_CRLFILE",
                     static_cast<int>(CURLOPT_CRLFILE)
                     );

    mod->addConstant(type_int, "CURLOPT_ISSUERCERT",
                     static_cast<int>(CURLOPT_ISSUERCERT)
                     );

    mod->addConstant(type_int, "CURLOPT_ADDRESS_SCOPE",
                     static_cast<int>(CURLOPT_ADDRESS_SCOPE)
                     );

    mod->addConstant(type_int, "CURLOPT_CERTINFO",
                     static_cast<int>(CURLOPT_CERTINFO)
                     );

    mod->addConstant(type_int, "CURLOPT_USERNAME",
                     static_cast<int>(CURLOPT_USERNAME)
                     );

    mod->addConstant(type_int, "CURLOPT_PASSWORD",
                     static_cast<int>(CURLOPT_PASSWORD)
                     );

    mod->addConstant(type_int, "CURLOPT_PROXYUSERNAME",
                     static_cast<int>(CURLOPT_PROXYUSERNAME)
                     );

    mod->addConstant(type_int, "CURLOPT_PROXYPASSWORD",
                     static_cast<int>(CURLOPT_PROXYPASSWORD)
                     );

    mod->addConstant(type_int, "CURLOPT_NOPROXY",
                     static_cast<int>(CURLOPT_NOPROXY)
                     );

    mod->addConstant(type_int, "CURLOPT_TFTP_BLKSIZE",
                     static_cast<int>(CURLOPT_TFTP_BLKSIZE)
                     );

    mod->addConstant(type_int, "CURLOPT_SOCKS5_GSSAPI_SERVICE",
                     static_cast<int>(CURLOPT_SOCKS5_GSSAPI_SERVICE)
                     );

    mod->addConstant(type_int, "CURLOPT_SOCKS5_GSSAPI_NEC",
                     static_cast<int>(CURLOPT_SOCKS5_GSSAPI_NEC)
                     );

    mod->addConstant(type_int, "CURLOPT_PROTOCOLS",
                     static_cast<int>(CURLOPT_PROTOCOLS)
                     );

    mod->addConstant(type_int, "CURLOPT_REDIR_PROTOCOLS",
                     static_cast<int>(CURLOPT_REDIR_PROTOCOLS)
                     );

    mod->addConstant(type_int, "CURLOPT_SSH_KNOWNHOSTS",
                     static_cast<int>(CURLOPT_SSH_KNOWNHOSTS)
                     );

    mod->addConstant(type_int, "CURLOPT_SSH_KEYFUNCTION",
                     static_cast<int>(CURLOPT_SSH_KEYFUNCTION)
                     );

    mod->addConstant(type_int, "CURLOPT_SSH_KEYDATA",
                     static_cast<int>(CURLOPT_SSH_KEYDATA)
                     );

    mod->addConstant(type_int, "CURLOPT_MAIL_FROM",
                     static_cast<int>(CURLOPT_MAIL_FROM)
                     );

    mod->addConstant(type_int, "CURLOPT_MAIL_RCPT",
                     static_cast<int>(CURLOPT_MAIL_RCPT)
                     );

    mod->addConstant(type_int, "CURLOPT_FTP_USE_PRET",
                     static_cast<int>(CURLOPT_FTP_USE_PRET)
                     );

    mod->addConstant(type_int, "CURLOPT_RTSP_REQUEST",
                     static_cast<int>(CURLOPT_RTSP_REQUEST)
                     );

    mod->addConstant(type_int, "CURLOPT_RTSP_SESSION_ID",
                     static_cast<int>(CURLOPT_RTSP_SESSION_ID)
                     );

    mod->addConstant(type_int, "CURLOPT_RTSP_STREAM_URI",
                     static_cast<int>(CURLOPT_RTSP_STREAM_URI)
                     );

    mod->addConstant(type_int, "CURLOPT_RTSP_TRANSPORT",
                     static_cast<int>(CURLOPT_RTSP_TRANSPORT)
                     );

    mod->addConstant(type_int, "CURLOPT_RTSP_CLIENT_CSEQ",
                     static_cast<int>(CURLOPT_RTSP_CLIENT_CSEQ)
                     );

    mod->addConstant(type_int, "CURLOPT_RTSP_SERVER_CSEQ",
                     static_cast<int>(CURLOPT_RTSP_SERVER_CSEQ)
                     );

    mod->addConstant(type_int, "CURLOPT_INTERLEAVEDATA",
                     static_cast<int>(CURLOPT_INTERLEAVEDATA)
                     );

    mod->addConstant(type_int, "CURLOPT_INTERLEAVEFUNCTION",
                     static_cast<int>(CURLOPT_INTERLEAVEFUNCTION)
                     );

    mod->addConstant(type_int, "CURLOPT_WILDCARDMATCH",
                     static_cast<int>(CURLOPT_WILDCARDMATCH)
                     );

    mod->addConstant(type_int, "CURLOPT_CHUNK_BGN_FUNCTION",
                     static_cast<int>(CURLOPT_CHUNK_BGN_FUNCTION)
                     );

    mod->addConstant(type_int, "CURLOPT_CHUNK_END_FUNCTION",
                     static_cast<int>(CURLOPT_CHUNK_END_FUNCTION)
                     );

    mod->addConstant(type_int, "CURLOPT_FNMATCH_FUNCTION",
                     static_cast<int>(CURLOPT_FNMATCH_FUNCTION)
                     );

    mod->addConstant(type_int, "CURLOPT_CHUNK_DATA",
                     static_cast<int>(CURLOPT_CHUNK_DATA)
                     );

    mod->addConstant(type_int, "CURLOPT_FNMATCH_DATA",
                     static_cast<int>(CURLOPT_FNMATCH_DATA)
                     );

    mod->addConstant(type_int, "CURLOPT_RESOLVE",
                     static_cast<int>(CURLOPT_RESOLVE)
                     );

    mod->addConstant(type_int, "CURLOPT_TLSAUTH_USERNAME",
                     static_cast<int>(CURLOPT_TLSAUTH_USERNAME)
                     );

    mod->addConstant(type_int, "CURLOPT_TLSAUTH_PASSWORD",
                     static_cast<int>(CURLOPT_TLSAUTH_PASSWORD)
                     );

    mod->addConstant(type_int, "CURLOPT_TLSAUTH_TYPE",
                     static_cast<int>(CURLOPT_TLSAUTH_TYPE)
                     );

    mod->addConstant(type_int, "CURLOPT_TRANSFER_ENCODING",
                     static_cast<int>(CURLOPT_TRANSFER_ENCODING)
                     );

    mod->addConstant(type_int, "CURLOPT_CLOSESOCKETFUNCTION",
                     static_cast<int>(CURLOPT_CLOSESOCKETFUNCTION)
                     );

    mod->addConstant(type_int, "CURLOPT_CLOSESOCKETDATA",
                     static_cast<int>(CURLOPT_CLOSESOCKETDATA)
                     );

    mod->addConstant(type_int, "CURLOPT_GSSAPI_DELEGATION",
                     static_cast<int>(CURLOPT_GSSAPI_DELEGATION)
                     );

    mod->addConstant(type_int, "CURL_IPRESOLVE_WHATEVER",
                     static_cast<int>(CURL_IPRESOLVE_WHATEVER)
                     );

    mod->addConstant(type_int, "CURL_IPRESOLVE_V4",
                     static_cast<int>(CURL_IPRESOLVE_V4)
                     );

    mod->addConstant(type_int, "CURL_IPRESOLVE_V6",
                     static_cast<int>(CURL_IPRESOLVE_V6)
                     );

    mod->addConstant(type_int, "CURLOPT_WRITEDATA",
                     static_cast<int>(CURLOPT_WRITEDATA)
                     );

    mod->addConstant(type_int, "CURLOPT_READDATA",
                     static_cast<int>(CURLOPT_READDATA)
                     );

    mod->addConstant(type_int, "CURLOPT_HEADERDATA",
                     static_cast<int>(CURLOPT_HEADERDATA)
                     );

    mod->addConstant(type_int, "CURLOPT_RTSPHEADER",
                     static_cast<int>(CURLOPT_RTSPHEADER)
                     );

    mod->addConstant(type_int, "CURL_HTTP_VERSION_NONE",
                     static_cast<int>(CURL_HTTP_VERSION_NONE)
                     );

    mod->addConstant(type_int, "CURL_HTTP_VERSION_1_0",
                     static_cast<int>(CURL_HTTP_VERSION_1_0)
                     );

    mod->addConstant(type_int, "CURL_HTTP_VERSION_1_1",
                     static_cast<int>(CURL_HTTP_VERSION_1_1)
                     );

    mod->addConstant(type_int, "CURL_RTSPREQ_NONE",
                     static_cast<int>(CURL_RTSPREQ_NONE)
                     );

    mod->addConstant(type_int, "CURL_RTSPREQ_OPTIONS",
                     static_cast<int>(CURL_RTSPREQ_OPTIONS)
                     );

    mod->addConstant(type_int, "CURL_RTSPREQ_DESCRIBE",
                     static_cast<int>(CURL_RTSPREQ_DESCRIBE)
                     );

    mod->addConstant(type_int, "CURL_RTSPREQ_ANNOUNCE",
                     static_cast<int>(CURL_RTSPREQ_ANNOUNCE)
                     );

    mod->addConstant(type_int, "CURL_RTSPREQ_SETUP",
                     static_cast<int>(CURL_RTSPREQ_SETUP)
                     );

    mod->addConstant(type_int, "CURL_RTSPREQ_PLAY",
                     static_cast<int>(CURL_RTSPREQ_PLAY)
                     );

    mod->addConstant(type_int, "CURL_RTSPREQ_PAUSE",
                     static_cast<int>(CURL_RTSPREQ_PAUSE)
                     );

    mod->addConstant(type_int, "CURL_RTSPREQ_TEARDOWN",
                     static_cast<int>(CURL_RTSPREQ_TEARDOWN)
                     );

    mod->addConstant(type_int, "CURL_RTSPREQ_GET_PARAMETER",
                     static_cast<int>(CURL_RTSPREQ_GET_PARAMETER)
                     );

    mod->addConstant(type_int, "CURL_RTSPREQ_SET_PARAMETER",
                     static_cast<int>(CURL_RTSPREQ_SET_PARAMETER)
                     );

    mod->addConstant(type_int, "CURL_RTSPREQ_RECORD",
                     static_cast<int>(CURL_RTSPREQ_RECORD)
                     );

    mod->addConstant(type_int, "CURL_RTSPREQ_RECEIVE",
                     static_cast<int>(CURL_RTSPREQ_RECEIVE)
                     );

    mod->addConstant(type_int, "CURL_RTSPREQ_LAST",
                     static_cast<int>(CURL_RTSPREQ_LAST)
                     );

    mod->addConstant(type_int, "CURL_NETRC_IGNORED",
                     static_cast<int>(CURL_NETRC_IGNORED)
                     );

    mod->addConstant(type_int, "CURL_NETRC_OPTIONAL",
                     static_cast<int>(CURL_NETRC_OPTIONAL)
                     );

    mod->addConstant(type_int, "CURL_NETRC_REQUIRED",
                     static_cast<int>(CURL_NETRC_REQUIRED)
                     );

    mod->addConstant(type_int, "CURL_SSLVERSION_DEFAULT",
                     static_cast<int>(CURL_SSLVERSION_DEFAULT)
                     );

    mod->addConstant(type_int, "CURL_SSLVERSION_TLSv1",
                     static_cast<int>(CURL_SSLVERSION_TLSv1)
                     );

    mod->addConstant(type_int, "CURL_SSLVERSION_SSLv2",
                     static_cast<int>(CURL_SSLVERSION_SSLv2)
                     );

    mod->addConstant(type_int, "CURL_SSLVERSION_SSLv3",
                     static_cast<int>(CURL_SSLVERSION_SSLv3)
                     );

    mod->addConstant(type_int, "CURL_TLSAUTH_NONE",
                     static_cast<int>(CURL_TLSAUTH_NONE)
                     );

    mod->addConstant(type_int, "CURL_TLSAUTH_SRP",
                     static_cast<int>(CURL_TLSAUTH_SRP)
                     );

    mod->addConstant(type_int, "CURL_REDIR_GET_ALL",
                     static_cast<int>(CURL_REDIR_GET_ALL)
                     );

    mod->addConstant(type_int, "CURL_REDIR_POST_301",
                     static_cast<int>(CURL_REDIR_POST_301)
                     );

    mod->addConstant(type_int, "CURL_REDIR_POST_302",
                     static_cast<int>(CURL_REDIR_POST_302)
                     );

    mod->addConstant(type_int, "CURL_REDIR_POST_ALL",
                     static_cast<int>(CURL_REDIR_POST_ALL)
                     );

    mod->addConstant(type_int, "CURL_TIMECOND_NONE",
                     static_cast<int>(CURL_TIMECOND_NONE)
                     );

    mod->addConstant(type_int, "CURL_TIMECOND_IFMODSINCE",
                     static_cast<int>(CURL_TIMECOND_IFMODSINCE)
                     );

    mod->addConstant(type_int, "CURL_TIMECOND_IFUNMODSINCE",
                     static_cast<int>(CURL_TIMECOND_IFUNMODSINCE)
                     );

    mod->addConstant(type_int, "CURL_TIMECOND_LASTMOD",
                     static_cast<int>(CURL_TIMECOND_LASTMOD)
                     );

    mod->addConstant(type_int, "CURLFORM_COPYNAME",
                     static_cast<int>(CURLFORM_COPYNAME)
                     );

    mod->addConstant(type_int, "CURLFORM_PTRNAME",
                     static_cast<int>(CURLFORM_PTRNAME)
                     );

    mod->addConstant(type_int, "CURLFORM_NAMELENGTH",
                     static_cast<int>(CURLFORM_NAMELENGTH)
                     );

    mod->addConstant(type_int, "CURLFORM_COPYCONTENTS",
                     static_cast<int>(CURLFORM_COPYCONTENTS)
                     );

    mod->addConstant(type_int, "CURLFORM_PTRCONTENTS",
                     static_cast<int>(CURLFORM_PTRCONTENTS)
                     );

    mod->addConstant(type_int, "CURLFORM_CONTENTSLENGTH",
                     static_cast<int>(CURLFORM_CONTENTSLENGTH)
                     );

    mod->addConstant(type_int, "CURLFORM_FILECONTENT",
                     static_cast<int>(CURLFORM_FILECONTENT)
                     );

    mod->addConstant(type_int, "CURLFORM_ARRAY",
                     static_cast<int>(CURLFORM_ARRAY)
                     );

    mod->addConstant(type_int, "CURLFORM_OBSOLETE",
                     static_cast<int>(CURLFORM_OBSOLETE)
                     );

    mod->addConstant(type_int, "CURLFORM_FILE",
                     static_cast<int>(CURLFORM_FILE)
                     );

    mod->addConstant(type_int, "CURLFORM_BUFFER",
                     static_cast<int>(CURLFORM_BUFFER)
                     );

    mod->addConstant(type_int, "CURLFORM_BUFFERPTR",
                     static_cast<int>(CURLFORM_BUFFERPTR)
                     );

    mod->addConstant(type_int, "CURLFORM_BUFFERLENGTH",
                     static_cast<int>(CURLFORM_BUFFERLENGTH)
                     );

    mod->addConstant(type_int, "CURLFORM_CONTENTTYPE",
                     static_cast<int>(CURLFORM_CONTENTTYPE)
                     );

    mod->addConstant(type_int, "CURLFORM_CONTENTHEADER",
                     static_cast<int>(CURLFORM_CONTENTHEADER)
                     );

    mod->addConstant(type_int, "CURLFORM_FILENAME",
                     static_cast<int>(CURLFORM_FILENAME)
                     );

    mod->addConstant(type_int, "CURLFORM_END",
                     static_cast<int>(CURLFORM_END)
                     );

    mod->addConstant(type_int, "CURLFORM_STREAM",
                     static_cast<int>(CURLFORM_STREAM)
                     );

    mod->addConstant(type_int, "CURL_FORMADD_OK",
                     static_cast<int>(CURL_FORMADD_OK)
                     );

    mod->addConstant(type_int, "CURL_FORMADD_MEMORY",
                     static_cast<int>(CURL_FORMADD_MEMORY)
                     );

    mod->addConstant(type_int, "CURL_FORMADD_OPTION_TWICE",
                     static_cast<int>(CURL_FORMADD_OPTION_TWICE)
                     );

    mod->addConstant(type_int, "CURL_FORMADD_NULL",
                     static_cast<int>(CURL_FORMADD_NULL)
                     );

    mod->addConstant(type_int, "CURL_FORMADD_UNKNOWN_OPTION",
                     static_cast<int>(CURL_FORMADD_UNKNOWN_OPTION)
                     );

    mod->addConstant(type_int, "CURL_FORMADD_INCOMPLETE",
                     static_cast<int>(CURL_FORMADD_INCOMPLETE)
                     );

    mod->addConstant(type_int, "CURL_FORMADD_ILLEGAL_ARRAY",
                     static_cast<int>(CURL_FORMADD_ILLEGAL_ARRAY)
                     );

    mod->addConstant(type_int, "CURL_FORMADD_DISABLED",
                     static_cast<int>(CURL_FORMADD_DISABLED)
                     );

    mod->addConstant(type_int, "CURLINFO_STRING",
                     static_cast<int>(CURLINFO_STRING)
                     );

    mod->addConstant(type_int, "CURLINFO_LONG",
                     static_cast<int>(CURLINFO_LONG)
                     );

    mod->addConstant(type_int, "CURLINFO_DOUBLE",
                     static_cast<int>(CURLINFO_DOUBLE)
                     );

    mod->addConstant(type_int, "CURLINFO_SLIST",
                     static_cast<int>(CURLINFO_SLIST)
                     );

    mod->addConstant(type_int, "CURLINFO_MASK",
                     static_cast<int>(CURLINFO_MASK)
                     );

    mod->addConstant(type_int, "CURLINFO_TYPEMASK",
                     static_cast<int>(CURLINFO_TYPEMASK)
                     );

    mod->addConstant(type_int, "CURLINFO_EFFECTIVE_URL",
                     static_cast<int>(CURLINFO_EFFECTIVE_URL)
                     );

    mod->addConstant(type_int, "CURLINFO_RESPONSE_CODE",
                     static_cast<int>(CURLINFO_RESPONSE_CODE)
                     );

    mod->addConstant(type_int, "CURLINFO_TOTAL_TIME",
                     static_cast<int>(CURLINFO_TOTAL_TIME)
                     );

    mod->addConstant(type_int, "CURLINFO_NAMELOOKUP_TIME",
                     static_cast<int>(CURLINFO_NAMELOOKUP_TIME)
                     );

    mod->addConstant(type_int, "CURLINFO_CONNECT_TIME",
                     static_cast<int>(CURLINFO_CONNECT_TIME)
                     );

    mod->addConstant(type_int, "CURLINFO_PRETRANSFER_TIME",
                     static_cast<int>(CURLINFO_PRETRANSFER_TIME)
                     );

    mod->addConstant(type_int, "CURLINFO_SIZE_UPLOAD",
                     static_cast<int>(CURLINFO_SIZE_UPLOAD)
                     );

    mod->addConstant(type_int, "CURLINFO_SIZE_DOWNLOAD",
                     static_cast<int>(CURLINFO_SIZE_DOWNLOAD)
                     );

    mod->addConstant(type_int, "CURLINFO_SPEED_DOWNLOAD",
                     static_cast<int>(CURLINFO_SPEED_DOWNLOAD)
                     );

    mod->addConstant(type_int, "CURLINFO_SPEED_UPLOAD",
                     static_cast<int>(CURLINFO_SPEED_UPLOAD)
                     );

    mod->addConstant(type_int, "CURLINFO_HEADER_SIZE",
                     static_cast<int>(CURLINFO_HEADER_SIZE)
                     );

    mod->addConstant(type_int, "CURLINFO_REQUEST_SIZE",
                     static_cast<int>(CURLINFO_REQUEST_SIZE)
                     );

    mod->addConstant(type_int, "CURLINFO_SSL_VERIFYRESULT",
                     static_cast<int>(CURLINFO_SSL_VERIFYRESULT)
                     );

    mod->addConstant(type_int, "CURLINFO_FILETIME",
                     static_cast<int>(CURLINFO_FILETIME)
                     );

    mod->addConstant(type_int, "CURLINFO_CONTENT_LENGTH_DOWNLOAD",
                     static_cast<int>(CURLINFO_CONTENT_LENGTH_DOWNLOAD)
                     );

    mod->addConstant(type_int, "CURLINFO_CONTENT_LENGTH_UPLOAD",
                     static_cast<int>(CURLINFO_CONTENT_LENGTH_UPLOAD)
                     );

    mod->addConstant(type_int, "CURLINFO_STARTTRANSFER_TIME",
                     static_cast<int>(CURLINFO_STARTTRANSFER_TIME)
                     );

    mod->addConstant(type_int, "CURLINFO_CONTENT_TYPE",
                     static_cast<int>(CURLINFO_CONTENT_TYPE)
                     );

    mod->addConstant(type_int, "CURLINFO_REDIRECT_TIME",
                     static_cast<int>(CURLINFO_REDIRECT_TIME)
                     );

    mod->addConstant(type_int, "CURLINFO_REDIRECT_COUNT",
                     static_cast<int>(CURLINFO_REDIRECT_COUNT)
                     );

    mod->addConstant(type_int, "CURLINFO_PRIVATE",
                     static_cast<int>(CURLINFO_PRIVATE)
                     );

    mod->addConstant(type_int, "CURLINFO_HTTP_CONNECTCODE",
                     static_cast<int>(CURLINFO_HTTP_CONNECTCODE)
                     );

    mod->addConstant(type_int, "CURLINFO_HTTPAUTH_AVAIL",
                     static_cast<int>(CURLINFO_HTTPAUTH_AVAIL)
                     );

    mod->addConstant(type_int, "CURLINFO_PROXYAUTH_AVAIL",
                     static_cast<int>(CURLINFO_PROXYAUTH_AVAIL)
                     );

    mod->addConstant(type_int, "CURLINFO_OS_ERRNO",
                     static_cast<int>(CURLINFO_OS_ERRNO)
                     );

    mod->addConstant(type_int, "CURLINFO_NUM_CONNECTS",
                     static_cast<int>(CURLINFO_NUM_CONNECTS)
                     );

    mod->addConstant(type_int, "CURLINFO_SSL_ENGINES",
                     static_cast<int>(CURLINFO_SSL_ENGINES)
                     );

    mod->addConstant(type_int, "CURLINFO_COOKIELIST",
                     static_cast<int>(CURLINFO_COOKIELIST)
                     );

    mod->addConstant(type_int, "CURLINFO_LASTSOCKET",
                     static_cast<int>(CURLINFO_LASTSOCKET)
                     );

    mod->addConstant(type_int, "CURLINFO_FTP_ENTRY_PATH",
                     static_cast<int>(CURLINFO_FTP_ENTRY_PATH)
                     );

    mod->addConstant(type_int, "CURLINFO_REDIRECT_URL",
                     static_cast<int>(CURLINFO_REDIRECT_URL)
                     );

    mod->addConstant(type_int, "CURLINFO_PRIMARY_IP",
                     static_cast<int>(CURLINFO_PRIMARY_IP)
                     );

    mod->addConstant(type_int, "CURLINFO_APPCONNECT_TIME",
                     static_cast<int>(CURLINFO_APPCONNECT_TIME)
                     );

    mod->addConstant(type_int, "CURLINFO_CERTINFO",
                     static_cast<int>(CURLINFO_CERTINFO)
                     );

    mod->addConstant(type_int, "CURLINFO_CONDITION_UNMET",
                     static_cast<int>(CURLINFO_CONDITION_UNMET)
                     );

    mod->addConstant(type_int, "CURLINFO_RTSP_SESSION_ID",
                     static_cast<int>(CURLINFO_RTSP_SESSION_ID)
                     );

    mod->addConstant(type_int, "CURLINFO_RTSP_CLIENT_CSEQ",
                     static_cast<int>(CURLINFO_RTSP_CLIENT_CSEQ)
                     );

    mod->addConstant(type_int, "CURLINFO_RTSP_SERVER_CSEQ",
                     static_cast<int>(CURLINFO_RTSP_SERVER_CSEQ)
                     );

    mod->addConstant(type_int, "CURLINFO_RTSP_CSEQ_RECV",
                     static_cast<int>(CURLINFO_RTSP_CSEQ_RECV)
                     );

    mod->addConstant(type_int, "CURLINFO_PRIMARY_PORT",
                     static_cast<int>(CURLINFO_PRIMARY_PORT)
                     );

    mod->addConstant(type_int, "CURLINFO_LOCAL_IP",
                     static_cast<int>(CURLINFO_LOCAL_IP)
                     );

    mod->addConstant(type_int, "CURLINFO_LOCAL_PORT",
                     static_cast<int>(CURLINFO_LOCAL_PORT)
                     );

    mod->addConstant(type_int, "CURLCLOSEPOLICY_OLDEST",
                     static_cast<int>(CURLCLOSEPOLICY_OLDEST)
                     );

    mod->addConstant(type_int, "CURLCLOSEPOLICY_LEAST_RECENTLY_USED",
                     static_cast<int>(CURLCLOSEPOLICY_LEAST_RECENTLY_USED)
                     );

    mod->addConstant(type_int, "CURLCLOSEPOLICY_LEAST_TRAFFIC",
                     static_cast<int>(CURLCLOSEPOLICY_LEAST_TRAFFIC)
                     );

    mod->addConstant(type_int, "CURLCLOSEPOLICY_SLOWEST",
                     static_cast<int>(CURLCLOSEPOLICY_SLOWEST)
                     );

    mod->addConstant(type_int, "CURLCLOSEPOLICY_CALLBACK",
                     static_cast<int>(CURLCLOSEPOLICY_CALLBACK)
                     );

    mod->addConstant(type_int, "CURL_GLOBAL_SSL",
                     static_cast<int>(CURL_GLOBAL_SSL)
                     );

    mod->addConstant(type_int, "CURL_GLOBAL_WIN32",
                     static_cast<int>(CURL_GLOBAL_WIN32)
                     );

    mod->addConstant(type_int, "CURL_GLOBAL_ALL",
                     static_cast<int>(CURL_GLOBAL_ALL)
                     );

    mod->addConstant(type_int, "CURL_GLOBAL_NOTHING",
                     static_cast<int>(CURL_GLOBAL_NOTHING)
                     );

    mod->addConstant(type_int, "CURL_LOCK_DATA_NONE",
                     static_cast<int>(CURL_LOCK_DATA_NONE)
                     );

    mod->addConstant(type_int, "CURL_LOCK_DATA_SHARE",
                     static_cast<int>(CURL_LOCK_DATA_SHARE)
                     );

    mod->addConstant(type_int, "CURL_LOCK_DATA_COOKIE",
                     static_cast<int>(CURL_LOCK_DATA_COOKIE)
                     );

    mod->addConstant(type_int, "CURL_LOCK_DATA_DNS",
                     static_cast<int>(CURL_LOCK_DATA_DNS)
                     );

    mod->addConstant(type_int, "CURL_LOCK_DATA_SSL_SESSION",
                     static_cast<int>(CURL_LOCK_DATA_SSL_SESSION)
                     );

    mod->addConstant(type_int, "CURL_LOCK_DATA_CONNECT",
                     static_cast<int>(CURL_LOCK_DATA_CONNECT)
                     );

    mod->addConstant(type_int, "CURL_LOCK_ACCESS_NONE",
                     static_cast<int>(CURL_LOCK_ACCESS_NONE)
                     );

    mod->addConstant(type_int, "CURL_LOCK_ACCESS_SHARED",
                     static_cast<int>(CURL_LOCK_ACCESS_SHARED)
                     );

    mod->addConstant(type_int, "CURL_LOCK_ACCESS_SINGLE",
                     static_cast<int>(CURL_LOCK_ACCESS_SINGLE)
                     );

    mod->addConstant(type_int, "CURLVERSION_FIRST",
                     static_cast<int>(CURLVERSION_FIRST)
                     );

    mod->addConstant(type_int, "CURLVERSION_SECOND",
                     static_cast<int>(CURLVERSION_SECOND)
                     );

    mod->addConstant(type_int, "CURLVERSION_THIRD",
                     static_cast<int>(CURLVERSION_THIRD)
                     );

    mod->addConstant(type_int, "CURLVERSION_FOURTH",
                     static_cast<int>(CURLVERSION_FOURTH)
                     );

    mod->addConstant(type_int, "CURL_VERSION_IPV6",
                     static_cast<int>(CURL_VERSION_IPV6)
                     );

    mod->addConstant(type_int, "CURL_VERSION_KERBEROS4",
                     static_cast<int>(CURL_VERSION_KERBEROS4)
                     );

    mod->addConstant(type_int, "CURL_VERSION_SSL",
                     static_cast<int>(CURL_VERSION_SSL)
                     );

    mod->addConstant(type_int, "CURL_VERSION_LIBZ",
                     static_cast<int>(CURL_VERSION_LIBZ)
                     );

    mod->addConstant(type_int, "CURL_VERSION_NTLM",
                     static_cast<int>(CURL_VERSION_NTLM)
                     );

    mod->addConstant(type_int, "CURL_VERSION_GSSNEGOTIATE",
                     static_cast<int>(CURL_VERSION_GSSNEGOTIATE)
                     );

    mod->addConstant(type_int, "CURL_VERSION_DEBUG",
                     static_cast<int>(CURL_VERSION_DEBUG)
                     );

    mod->addConstant(type_int, "CURL_VERSION_ASYNCHDNS",
                     static_cast<int>(CURL_VERSION_ASYNCHDNS)
                     );

    mod->addConstant(type_int, "CURL_VERSION_SPNEGO",
                     static_cast<int>(CURL_VERSION_SPNEGO)
                     );

    mod->addConstant(type_int, "CURL_VERSION_LARGEFILE",
                     static_cast<int>(CURL_VERSION_LARGEFILE)
                     );

    mod->addConstant(type_int, "CURL_VERSION_IDN",
                     static_cast<int>(CURL_VERSION_IDN)
                     );

    mod->addConstant(type_int, "CURL_VERSION_SSPI",
                     static_cast<int>(CURL_VERSION_SSPI)
                     );

    mod->addConstant(type_int, "CURL_VERSION_CONV",
                     static_cast<int>(CURL_VERSION_CONV)
                     );

    mod->addConstant(type_int, "CURL_VERSION_CURLDEBUG",
                     static_cast<int>(CURL_VERSION_CURLDEBUG)
                     );

    mod->addConstant(type_int, "CURL_VERSION_TLSAUTH_SRP",
                     static_cast<int>(CURL_VERSION_TLSAUTH_SRP)
                     );

    mod->addConstant(type_int, "CURL_VERSION_NTLM_WB",
                     static_cast<int>(CURL_VERSION_NTLM_WB)
                     );

    mod->addConstant(type_int, "CURLPAUSE_RECV",
                     static_cast<int>(CURLPAUSE_RECV)
                     );

    mod->addConstant(type_int, "CURLPAUSE_RECV_CONT",
                     static_cast<int>(CURLPAUSE_RECV_CONT)
                     );

    mod->addConstant(type_int, "CURLPAUSE_SEND",
                     static_cast<int>(CURLPAUSE_SEND)
                     );

    mod->addConstant(type_int, "CURLPAUSE_SEND_CONT",
                     static_cast<int>(CURLPAUSE_SEND_CONT)
                     );

    mod->addConstant(type_int, "CURLPAUSE_ALL",
                     static_cast<int>(CURLPAUSE_ALL)
                     );

    mod->addConstant(type_int, "CURLPAUSE_CONT",
                     static_cast<int>(CURLPAUSE_CONT)
                     );
}
