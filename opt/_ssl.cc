#include <openssl/ssl.h>


#include <iostream>
using namespace std;

SSL *my_SSL_new(SSL_CTX *ctx) {
    SSL *result = SSL_new(ctx);
    if (!result)
        cerr << "error is " << SSL_get_error(result, 0) <<
            " context is " << ctx << endl;
    cerr << "my ssl = " << result << endl;
    return result;
}

void my_BIO_set_blocking(BIO *bio, int blocking) {
    BIO_set_nbio(bio, !blocking);
}

void my_SSL_set_accept_state(SSL *ssl) {
    cerr << "setting accept state on " << ssl << endl;
    SSL_set_accept_state(ssl);
}



#include "ext/Module.h"
#include "ext/Type.h"
#include "ext/Func.h"

extern "C"
void crack_ext__ssl_rinit() {
    return;
}

extern "C"
void crack_ext__ssl_cinit(crack::ext::Module *mod) {
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

    crack::ext::Type *type_BIO_METHOD = mod->addType("BIO_METHOD", sizeof(BIO_METHOD));
    type_BIO_METHOD->finish();


    crack::ext::Type *type_BIO = mod->addType("BIO", sizeof(BIO));

    f = type_BIO->addMethod(
        type_int, 
        "read",
        (void *)BIO_read
    );
    f->addArg(type_byteptr, 
              "buffer"
              );
    f->addArg(type_int, 
              "len"
              );


    f = type_BIO->addMethod(
        type_int, 
        "write",
        (void *)BIO_write
    );
    f->addArg(type_byteptr, 
              "buffer"
              );
    f->addArg(type_int, 
              "len"
              );


    f = type_BIO->addMethod(
        type_void, 
        "free",
        (void *)BIO_free
    );
    f->addArg(type_BIO, 
              "bio"
              );


    f = type_BIO->addMethod(
        type_int, 
        "ctrlPending",
        (void *)BIO_ctrl_pending
    );


    f = type_BIO->addMethod(
        type_void, 
        "setBlocking",
        (void *)my_BIO_set_blocking
    );
    f->addArg(type_bool, 
              "blocking"
              );

    type_BIO->finish();


    crack::ext::Type *type_SSL_METHOD = mod->addType("SSL_METHOD", sizeof(SSL_METHOD));
    type_SSL_METHOD->finish();


    crack::ext::Type *type_SSL_CTX = mod->addType("SSL_CTX", sizeof(SSL_CTX));

    f = type_SSL_CTX->addMethod(
        type_void, 
        "free",
        (void *)SSL_CTX_free
    );


    f = type_SSL_CTX->addMethod(
        type_int, 
        "useCertificateFile",
        (void *)SSL_CTX_use_certificate_file
    );
    f->addArg(type_byteptr, 
              "filename"
              );
    f->addArg(type_int, 
              "type"
              );


    f = type_SSL_CTX->addMethod(
        type_int, 
        "usePrivateKeyFile",
        (void *)SSL_CTX_use_PrivateKey_file
    );
    f->addArg(type_byteptr, 
              "file"
              );
    f->addArg(type_int, 
              "type"
              );

    type_SSL_CTX->finish();


    crack::ext::Type *type_SSL = mod->addType("SSL", sizeof(SSL));

    f = type_SSL->addMethod(
        type_void, 
        "free",
        (void *)SSL_free
    );


    f = type_SSL->addMethod(
        type_void, 
        "setBIO",
        (void *)SSL_set_bio
    );
    f->addArg(type_BIO, 
              "rbio"
              );
    f->addArg(type_BIO, 
              "wbio"
              );


    f = type_SSL->addMethod(
        type_void, 
        "setAcceptState",
        (void *)SSL_set_accept_state
    );


    f = type_SSL->addMethod(
        type_void, 
        "setConnectState",
        (void *)SSL_set_connect_state
    );


    f = type_SSL->addMethod(
        type_int, 
        "accept",
        (void *)SSL_accept
    );


    f = type_SSL->addMethod(
        type_int, 
        "connect",
        (void *)SSL_connect
    );


    f = type_SSL->addMethod(
        type_int, 
        "shutdown",
        (void *)SSL_shutdown
    );


    f = type_SSL->addMethod(
        type_int, 
        "write",
        (void *)SSL_write
    );
    f->addArg(type_byteptr, 
              "buf"
              );
    f->addArg(type_int, 
              "size"
              );


    f = type_SSL->addMethod(
        type_int, 
        "read",
        (void *)SSL_read
    );
    f->addArg(type_byteptr, 
              "buf"
              );
    f->addArg(type_int, 
              "cap"
              );


    f = type_SSL->addMethod(
        type_int, 
        "getError",
        (void *)SSL_get_error
    );
    f->addArg(type_int, 
              "ret"
              );

    type_SSL->finish();

    f = mod->addFunc(type_BIO_METHOD, "BIO_s_mem",
                     (void *)BIO_s_mem
                     );

    f = mod->addFunc(type_BIO, "BIO_new",
                     (void *)BIO_new
                     );
       f->addArg(type_BIO_METHOD, "method");

    f = mod->addFunc(type_SSL_METHOD, "SSLv23_method",
                     (void *)SSLv23_method
                     );

    f = mod->addFunc(type_void, "SSL_library_init",
                     (void *)SSL_library_init
                     );

    f = mod->addFunc(type_SSL_CTX, "SSL_CTX_new",
                     (void *)SSL_CTX_new
                     );
       f->addArg(type_SSL_METHOD, "method");

    f = mod->addFunc(type_SSL, "SSL_new",
                     (void *)SSL_new
                     );
       f->addArg(type_SSL_CTX, "ctx");


    mod->addConstant(type_int, "SSL_FILETYPE_PEM",
                     static_cast<int>(SSL_FILETYPE_PEM)
                     );

    mod->addConstant(type_int, "SSL_ERROR_NONE",
                     static_cast<int>(SSL_ERROR_NONE)
                     );

    mod->addConstant(type_int, "SSL_ERROR_ZERO_RETURN",
                     static_cast<int>(SSL_ERROR_ZERO_RETURN)
                     );

    mod->addConstant(type_int, "SSL_ERROR_WANT_READ",
                     static_cast<int>(SSL_ERROR_WANT_READ)
                     );

    mod->addConstant(type_int, "SSL_ERROR_WANT_WRITE",
                     static_cast<int>(SSL_ERROR_WANT_WRITE)
                     );

    mod->addConstant(type_int, "SSL_ERROR_WANT_CONNECT",
                     static_cast<int>(SSL_ERROR_WANT_CONNECT)
                     );

    mod->addConstant(type_int, "SSL_ERROR_WANT_ACCEPT",
                     static_cast<int>(SSL_ERROR_WANT_ACCEPT)
                     );

    mod->addConstant(type_int, "SSL_ERROR_WANT_X509_LOOKUP",
                     static_cast<int>(SSL_ERROR_WANT_X509_LOOKUP)
                     );

    mod->addConstant(type_int, "SSL_ERROR_SYSCALL",
                     static_cast<int>(SSL_ERROR_SYSCALL)
                     );

    mod->addConstant(type_int, "SSL_ERROR_SSL",
                     static_cast<int>(SSL_ERROR_SSL)
                     );
}
