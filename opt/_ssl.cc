#include <openssl/ssl.h>
#include <openssl/evp.h>


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

int my_EVP_CIPHER_iv_length(EVP_CIPHER *cipher) {
    return EVP_CIPHER_iv_length(cipher);
}

int my_EVP_CIPHER_key_length(EVP_CIPHER *cipher) {
    return EVP_CIPHER_key_length(cipher);
}

int my_EVP_CIPHER_block_size(EVP_CIPHER *cipher) {
    return EVP_CIPHER_block_size(cipher);
}

int my_EVP_CIPHER_mode(EVP_CIPHER *cipher) {
    return EVP_CIPHER_mode(cipher);
}

int my_EVP_CIPHER_flags(EVP_CIPHER *cipher) {
    return EVP_CIPHER_flags(cipher);
}

// Definining this here.
struct engine_st {};


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


    crack::ext::Type *type_EVPCipher = mod->addType("EVPCipher", sizeof(EVP_CIPHER));

    f = type_EVPCipher->addMethod(
        type_int,
        "getIVLength",
        (void *)my_EVP_CIPHER_iv_length
    );


    f = type_EVPCipher->addMethod(
        type_int,
        "getKeyLength",
        (void *)my_EVP_CIPHER_key_length
    );


    f = type_EVPCipher->addMethod(
        type_int,
        "getBlockSize",
        (void *)my_EVP_CIPHER_block_size
    );


    f = type_EVPCipher->addMethod(
        type_int,
        "getMode",
        (void *)my_EVP_CIPHER_mode
    );


    f = type_EVPCipher->addMethod(
        type_int,
        "getFlags",
        (void *)my_EVP_CIPHER_flags
    );

    type_EVPCipher->finish();


    crack::ext::Type *type_Engine = mod->addType("Engine", sizeof(ENGINE));
    type_Engine->finish();


    crack::ext::Type *array = mod->getType("array");

    crack::ext::Type *array_pint_q;
    {
        std::vector<crack::ext::Type *> params(1);
        params[0] = type_int;
        array_pint_q = array->getSpecialization(params);
    }

    crack::ext::Type *type_EVPCipherContext = mod->addType("EVPCipherContext", sizeof(EVP_CIPHER_CTX));
        f = type_EVPCipherContext->addConstructor("oper init",
                            (void *)EVP_CIPHER_CTX_init
                        );


    f = type_EVPCipherContext->addMethod(
        type_void,
        "cleanup",
        (void *)EVP_CIPHER_CTX_cleanup
    );


    f = type_EVPCipherContext->addMethod(
        type_int,
        "encryptInit",
        (void *)EVP_EncryptInit_ex
    );
    f->addArg(type_EVPCipher,
              "type"
              );
    f->addArg(type_Engine,
              "impl"
              );
    f->addArg(type_byteptr,
              "key"
              );
    f->addArg(type_byteptr,
              "iv"
              );


    f = type_EVPCipherContext->addMethod(
        type_int,
        "encryptUpdate",
        (void *)EVP_EncryptUpdate
    );
    f->addArg(type_byteptr,
              "out"
              );
    f->addArg(array_pint_q,
              "out1"
              );
    f->addArg(type_byteptr,
              "inp"
              );
    f->addArg(type_int,
              "inp1"
              );


    f = type_EVPCipherContext->addMethod(
        type_int,
        "encryptFinal",
        (void *)EVP_EncryptFinal
    );
    f->addArg(type_byteptr,
              "out"
              );
    f->addArg(array_pint_q,
              "out1"
              );


    f = type_EVPCipherContext->addMethod(
        type_int,
        "decryptInit",
        (void *)EVP_DecryptInit_ex
    );
    f->addArg(type_EVPCipher,
              "type"
              );
    f->addArg(type_Engine,
              "impl"
              );
    f->addArg(type_byteptr,
              "key"
              );
    f->addArg(type_byteptr,
              "iv"
              );


    f = type_EVPCipherContext->addMethod(
        type_int,
        "decryptUpdate",
        (void *)EVP_DecryptUpdate
    );
    f->addArg(type_byteptr,
              "out"
              );
    f->addArg(array_pint_q,
              "out1"
              );
    f->addArg(type_byteptr,
              "inp"
              );
    f->addArg(type_int,
              "inp1"
              );


    f = type_EVPCipherContext->addMethod(
        type_int,
        "decryptFinal",
        (void *)EVP_DecryptFinal
    );
    f->addArg(type_byteptr,
              "out"
              );
    f->addArg(array_pint_q,
              "out1"
              );

    type_EVPCipherContext->finish();

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

    f = mod->addFunc(type_EVPCipher, "EVP_enc_null",
                     (void *)EVP_enc_null
                     );

    f = mod->addFunc(type_EVPCipher, "EVP_aes_128_cbc",
                     (void *)EVP_aes_128_cbc
                     );

    f = mod->addFunc(type_EVPCipher, "EVP_aes_128_ecb",
                     (void *)EVP_aes_128_ecb
                     );

    f = mod->addFunc(type_EVPCipher, "EVP_aes_128_cfb",
                     (void *)EVP_aes_128_cfb
                     );

    f = mod->addFunc(type_EVPCipher, "EVP_aes_128_ofb",
                     (void *)EVP_aes_128_ofb
                     );

    f = mod->addFunc(type_EVPCipher, "EVP_aes_192_cbc",
                     (void *)EVP_aes_192_cbc
                     );

    f = mod->addFunc(type_EVPCipher, "EVP_aes_192_ecb",
                     (void *)EVP_aes_192_ecb
                     );

    f = mod->addFunc(type_EVPCipher, "EVP_aes_192_cfb",
                     (void *)EVP_aes_192_cfb
                     );

    f = mod->addFunc(type_EVPCipher, "EVP_aes_192_ofb",
                     (void *)EVP_aes_192_ofb
                     );

    f = mod->addFunc(type_EVPCipher, "EVP_aes_256_cbc",
                     (void *)EVP_aes_256_cbc
                     );

    f = mod->addFunc(type_EVPCipher, "EVP_aes_256_ecb",
                     (void *)EVP_aes_256_ecb
                     );

    f = mod->addFunc(type_EVPCipher, "EVP_aes_256_cfb",
                     (void *)EVP_aes_256_cfb
                     );

    f = mod->addFunc(type_EVPCipher, "EVP_aes_256_ofb",
                     (void *)EVP_aes_256_ofb
                     );

    f = mod->addFunc(type_EVPCipher, "EVP_des_cbc",
                     (void *)EVP_des_cbc
                     );

    f = mod->addFunc(type_EVPCipher, "EVP_des_ecb",
                     (void *)EVP_des_ecb
                     );

    f = mod->addFunc(type_EVPCipher, "EVP_des_cfb",
                     (void *)EVP_des_cfb
                     );

    f = mod->addFunc(type_EVPCipher, "EVP_des_ofb",
                     (void *)EVP_des_ofb
                     );

    f = mod->addFunc(type_EVPCipher, "EVP_des_ede_cbc",
                     (void *)EVP_des_ede_cbc
                     );

    f = mod->addFunc(type_EVPCipher, "EVP_des_ede",
                     (void *)EVP_des_ede
                     );

    f = mod->addFunc(type_EVPCipher, "EVP_des_ede_ofb",
                     (void *)EVP_des_ede_ofb
                     );

    f = mod->addFunc(type_EVPCipher, "EVP_des_ede_cfb",
                     (void *)EVP_des_ede_cfb
                     );

    f = mod->addFunc(type_EVPCipher, "EVP_des_ede3_cbc",
                     (void *)EVP_des_ede3_cbc
                     );

    f = mod->addFunc(type_EVPCipher, "EVP_des_ede3",
                     (void *)EVP_des_ede3
                     );

    f = mod->addFunc(type_EVPCipher, "EVP_des_ede3_ofb",
                     (void *)EVP_des_ede3_ofb
                     );

    f = mod->addFunc(type_EVPCipher, "EVP_des_ede3_cfb",
                     (void *)EVP_des_ede3_cfb
                     );

    f = mod->addFunc(type_EVPCipher, "EVP_desx_cbc",
                     (void *)EVP_desx_cbc
                     );

    f = mod->addFunc(type_EVPCipher, "EVP_rc4",
                     (void *)EVP_rc4
                     );

    f = mod->addFunc(type_EVPCipher, "EVP_rc4_40",
                     (void *)EVP_rc4_40
                     );

    f = mod->addFunc(type_EVPCipher, "EVP_rc2_cbc",
                     (void *)EVP_rc2_cbc
                     );

    f = mod->addFunc(type_EVPCipher, "EVP_rc2_ecb",
                     (void *)EVP_rc2_ecb
                     );

    f = mod->addFunc(type_EVPCipher, "EVP_rc2_cfb",
                     (void *)EVP_rc2_cfb
                     );

    f = mod->addFunc(type_EVPCipher, "EVP_rc2_ofb",
                     (void *)EVP_rc2_ofb
                     );

    f = mod->addFunc(type_EVPCipher, "EVP_rc2_40_cbc",
                     (void *)EVP_rc2_40_cbc
                     );

    f = mod->addFunc(type_EVPCipher, "EVP_rc2_64_cbc",
                     (void *)EVP_rc2_64_cbc
                     );

    f = mod->addFunc(type_EVPCipher, "EVP_bf_cbc",
                     (void *)EVP_bf_cbc
                     );

    f = mod->addFunc(type_EVPCipher, "EVP_bf_ecb",
                     (void *)EVP_bf_ecb
                     );

    f = mod->addFunc(type_EVPCipher, "EVP_bf_cfb",
                     (void *)EVP_bf_cfb
                     );

    f = mod->addFunc(type_EVPCipher, "EVP_bf_ofb",
                     (void *)EVP_bf_ofb
                     );

    f = mod->addFunc(type_EVPCipher, "EVP_cast5_cbc",
                     (void *)EVP_cast5_cbc
                     );

    f = mod->addFunc(type_EVPCipher, "EVP_cast5_ecb",
                     (void *)EVP_cast5_ecb
                     );

    f = mod->addFunc(type_EVPCipher, "EVP_cast5_cfb",
                     (void *)EVP_cast5_cfb
                     );

    f = mod->addFunc(type_EVPCipher, "EVP_cast5_ofb",
                     (void *)EVP_cast5_ofb
                     );

    f = mod->addFunc(type_EVPCipher, "EVP_aes_128_gcm",
                     (void *)EVP_aes_128_gcm
                     );

    f = mod->addFunc(type_EVPCipher, "EVP_aes_192_gcm",
                     (void *)EVP_aes_192_gcm
                     );

    f = mod->addFunc(type_EVPCipher, "EVP_aes_256_gcm",
                     (void *)EVP_aes_256_gcm
                     );

    f = mod->addFunc(type_EVPCipher, "EVP_aes_128_ccm",
                     (void *)EVP_aes_128_ccm
                     );

    f = mod->addFunc(type_EVPCipher, "EVP_aes_192_ccm",
                     (void *)EVP_aes_192_ccm
                     );

    f = mod->addFunc(type_EVPCipher, "EVP_aes_256_ccm",
                     (void *)EVP_aes_256_ccm
                     );


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

    mod->addConstant(type_int, "EVP_CIPH_STREAM_CIPHER",
                     static_cast<int>(EVP_CIPH_STREAM_CIPHER)
                     );

    mod->addConstant(type_int, "EVP_CIPH_ECB_MODE",
                     static_cast<int>(EVP_CIPH_ECB_MODE)
                     );

    mod->addConstant(type_int, "EVP_CIPH_CBC_MODE",
                     static_cast<int>(EVP_CIPH_CBC_MODE)
                     );

    mod->addConstant(type_int, "EVP_CIPH_CFB_MODE",
                     static_cast<int>(EVP_CIPH_CFB_MODE)
                     );

    mod->addConstant(type_int, "EVP_CIPH_OFB_MODE",
                     static_cast<int>(EVP_CIPH_OFB_MODE)
                     );

    mod->addConstant(type_int, "EVP_CIPH_CTR_MODE",
                     static_cast<int>(EVP_CIPH_CTR_MODE)
                     );

    mod->addConstant(type_int, "EVP_CIPH_GCM_MODE",
                     static_cast<int>(EVP_CIPH_GCM_MODE)
                     );

    mod->addConstant(type_int, "EVP_CIPH_CCM_MODE",
                     static_cast<int>(EVP_CIPH_CCM_MODE)
                     );

    mod->addConstant(type_int, "EVP_CIPH_XTS_MODE",
                     static_cast<int>(EVP_CIPH_XTS_MODE)
                     );

    mod->addConstant(type_int, "EVP_CIPH_MODE",
                     static_cast<int>(EVP_CIPH_MODE)
                     );

    mod->addConstant(type_int, "EVP_CIPH_VARIABLE_LENGTH",
                     static_cast<int>(EVP_CIPH_VARIABLE_LENGTH)
                     );

    mod->addConstant(type_int, "EVP_CIPH_CUSTOM_IV",
                     static_cast<int>(EVP_CIPH_CUSTOM_IV)
                     );

    mod->addConstant(type_int, "EVP_CIPH_ALWAYS_CALL_INIT",
                     static_cast<int>(EVP_CIPH_ALWAYS_CALL_INIT)
                     );

    mod->addConstant(type_int, "EVP_CIPH_CTRL_INIT",
                     static_cast<int>(EVP_CIPH_CTRL_INIT)
                     );

    mod->addConstant(type_int, "EVP_CIPH_CUSTOM_KEY_LENGTH",
                     static_cast<int>(EVP_CIPH_CUSTOM_KEY_LENGTH)
                     );

    mod->addConstant(type_int, "EVP_CIPH_NO_PADDING",
                     static_cast<int>(EVP_CIPH_NO_PADDING)
                     );

    mod->addConstant(type_int, "EVP_CIPH_RAND_KEY",
                     static_cast<int>(EVP_CIPH_RAND_KEY)
                     );

    mod->addConstant(type_int, "EVP_CIPH_CUSTOM_COPY",
                     static_cast<int>(EVP_CIPH_CUSTOM_COPY)
                     );

    mod->addConstant(type_int, "EVP_CIPH_FLAG_DEFAULT_ASN1",
                     static_cast<int>(EVP_CIPH_FLAG_DEFAULT_ASN1)
                     );

    mod->addConstant(type_int, "EVP_CIPH_FLAG_LENGTH_BITS",
                     static_cast<int>(EVP_CIPH_FLAG_LENGTH_BITS)
                     );

    mod->addConstant(type_int, "EVP_CIPH_FLAG_FIPS",
                     static_cast<int>(EVP_CIPH_FLAG_FIPS)
                     );

    mod->addConstant(type_int, "EVP_CIPH_FLAG_NON_FIPS_ALLOW",
                     static_cast<int>(EVP_CIPH_FLAG_NON_FIPS_ALLOW)
                     );

    mod->addConstant(type_int, "EVP_CIPH_FLAG_CUSTOM_CIPHER",
                     static_cast<int>(EVP_CIPH_FLAG_CUSTOM_CIPHER)
                     );

    mod->addConstant(type_int, "EVP_CIPH_FLAG_AEAD_CIPHER",
                     static_cast<int>(EVP_CIPH_FLAG_AEAD_CIPHER)
                     );

    mod->addConstant(type_int, "EVP_CTRL_INIT",
                     static_cast<int>(EVP_CTRL_INIT)
                     );

    mod->addConstant(type_int, "EVP_CTRL_SET_KEY_LENGTH",
                     static_cast<int>(EVP_CTRL_SET_KEY_LENGTH)
                     );

    mod->addConstant(type_int, "EVP_CTRL_GET_RC2_KEY_BITS",
                     static_cast<int>(EVP_CTRL_GET_RC2_KEY_BITS)
                     );

    mod->addConstant(type_int, "EVP_CTRL_SET_RC2_KEY_BITS",
                     static_cast<int>(EVP_CTRL_SET_RC2_KEY_BITS)
                     );

    mod->addConstant(type_int, "EVP_CTRL_GET_RC5_ROUNDS",
                     static_cast<int>(EVP_CTRL_GET_RC5_ROUNDS)
                     );

    mod->addConstant(type_int, "EVP_CTRL_SET_RC5_ROUNDS",
                     static_cast<int>(EVP_CTRL_SET_RC5_ROUNDS)
                     );

    mod->addConstant(type_int, "EVP_CTRL_RAND_KEY",
                     static_cast<int>(EVP_CTRL_RAND_KEY)
                     );

    mod->addConstant(type_int, "EVP_CTRL_PBE_PRF_NID",
                     static_cast<int>(EVP_CTRL_PBE_PRF_NID)
                     );

    mod->addConstant(type_int, "EVP_CTRL_COPY",
                     static_cast<int>(EVP_CTRL_COPY)
                     );

    mod->addConstant(type_int, "EVP_CTRL_GCM_SET_IVLEN",
                     static_cast<int>(EVP_CTRL_GCM_SET_IVLEN)
                     );

    mod->addConstant(type_int, "EVP_CTRL_GCM_GET_TAG",
                     static_cast<int>(EVP_CTRL_GCM_GET_TAG)
                     );

    mod->addConstant(type_int, "EVP_CTRL_GCM_SET_TAG",
                     static_cast<int>(EVP_CTRL_GCM_SET_TAG)
                     );

    mod->addConstant(type_int, "EVP_CTRL_GCM_SET_IV_FIXED",
                     static_cast<int>(EVP_CTRL_GCM_SET_IV_FIXED)
                     );

    mod->addConstant(type_int, "EVP_CTRL_GCM_IV_GEN",
                     static_cast<int>(EVP_CTRL_GCM_IV_GEN)
                     );

    mod->addConstant(type_int, "EVP_CTRL_CCM_SET_IVLEN",
                     static_cast<int>(EVP_CTRL_CCM_SET_IVLEN)
                     );

    mod->addConstant(type_int, "EVP_CTRL_CCM_GET_TAG",
                     static_cast<int>(EVP_CTRL_CCM_GET_TAG)
                     );

    mod->addConstant(type_int, "EVP_CTRL_CCM_SET_TAG",
                     static_cast<int>(EVP_CTRL_CCM_SET_TAG)
                     );

    mod->addConstant(type_int, "EVP_CTRL_CCM_SET_L",
                     static_cast<int>(EVP_CTRL_CCM_SET_L)
                     );

    mod->addConstant(type_int, "EVP_CTRL_CCM_SET_MSGLEN",
                     static_cast<int>(EVP_CTRL_CCM_SET_MSGLEN)
                     );

    mod->addConstant(type_int, "EVP_CTRL_AEAD_TLS1_AAD",
                     static_cast<int>(EVP_CTRL_AEAD_TLS1_AAD)
                     );
}
