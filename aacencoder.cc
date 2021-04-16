#include <sys/socket.h>
#include <stdio.h>
#include <memory.h>
#include <node_api.h>

#include "aacencoder.h"
#include <assert.h>

AACEncoder::AACEncoder(ulong sampleRate, uint numChannels)
    : sampleRate_(sampleRate), numChannels_(numChannels), env_(nullptr), wrapper_(nullptr) 
{
    outputBuffer = NULL;
}

AACEncoder::~AACEncoder()
{
    if (outputBuffer != NULL)
    {
        delete outputBuffer;
        outputBuffer = NULL;
    }
    printf("faacEncClose\n");
    faacEncClose(hEncoder);
    napi_delete_reference(env_, wrapper_);
}

void AACEncoder::Destructor(napi_env env,
                          void* nativeObject,
                          void* /*finalize_hint*/) 
{
  reinterpret_cast<AACEncoder*>(nativeObject)->~AACEncoder();
}

#define DECLARE_NAPI_METHOD(name, func)                                        \
  { name, 0, func, 0, 0, 0, napi_default, 0 }

napi_value AACEncoder::Init(napi_env env, napi_value exports)
{
    napi_status status;
    napi_property_descriptor properties[] = {
        DECLARE_NAPI_METHOD("encode", encode),
    };

    printf("AACEncoder::Init\n");

    napi_value cons;
    status = napi_define_class(
        env, "AACEncoder", NAPI_AUTO_LENGTH, New, nullptr, 1, properties, &cons);
    assert(status == napi_ok);

    printf("napi_define_class\n");

    // We will need the constructor `cons` later during the life cycle of the
    // addon, so we store a persistent reference to it as the instance data for
    // our addon. This will enable us to use `napi_get_instance_data` at any
    // point during the life cycle of our addon to retrieve it. We cannot simply
    // store it as a global static variable, because that will render our addon
    // unable to support Node.js worker threads and multiple contexts on a single
    // thread.
    //
    // The finalizer we pass as a lambda will be called when our addon is unloaded
    // and is responsible for releasing the persistent reference and freeing the
    // heap memory where we stored the persistent reference.
    napi_ref* constructor = new napi_ref;
    status = napi_create_reference(env, cons, 1, constructor);
    assert(status == napi_ok);

    printf("napi_create_reference\n");

    status = napi_set_instance_data(
        env,
        constructor,
        [](napi_env env, void* data, void* hint) {
            napi_ref* constructor = static_cast<napi_ref*>(data);
            napi_status status = napi_delete_reference(env, *constructor);
            assert(status == napi_ok);
            delete constructor;
        },
        nullptr);
    assert(status == napi_ok);

    printf("napi_set_instance_data\n");

    status = napi_set_named_property(env, exports, "AACEncoder", cons);
    assert(status == napi_ok);
    return exports;
}

napi_value AACEncoder::Constructor(napi_env env) 
{
    void* instance_data = nullptr;
    napi_status status = napi_get_instance_data(env, &instance_data);
    assert(status == napi_ok);
    napi_ref* constructor = static_cast<napi_ref*>(instance_data);

    napi_value cons;
    status = napi_get_reference_value(env, *constructor, &cons);
    assert(status == napi_ok);
    return cons;
}

napi_value AACEncoder::New(napi_env env, napi_callback_info info) 
{
    napi_status status;

    napi_value target;
    status = napi_get_new_target(env, info, &target);
    assert(status == napi_ok);
    bool is_constructor = target != nullptr;

    if (is_constructor) 
    {
        // Invoked as constructor: `new AACEncoder(...)`
        size_t argc = 2;
        napi_value args[2];
        napi_value jsthis;
        status = napi_get_cb_info(env, info, &argc, args, &jsthis, nullptr);
        assert(status == napi_ok);

        uint32_t value = 0;
        ulong sampleRate = 0;
        uint numChannels = 0;

        napi_valuetype valuetype;
        status = napi_typeof(env, args[0], &valuetype);
        assert(status == napi_ok);

        if (valuetype != napi_undefined) {
            status = napi_get_value_uint32(env, args[0], &value);
            assert(status == napi_ok);
        }
        sampleRate = value;

        status = napi_typeof(env, args[1], &valuetype);
        assert(status == napi_ok);
    
        if (valuetype != napi_undefined) {
            status = napi_get_value_uint32(env, args[1], &value);
            assert(status == napi_ok);
        }
        numChannels = value;
    
        printf("sampleRate=%lu, numChannels=%u\n", sampleRate, numChannels);
    
        AACEncoder* obj = new AACEncoder(sampleRate, numChannels);
    
        obj->hEncoder = faacEncOpen(sampleRate, numChannels, &obj->inputSamples, &obj->maxOutputBytes);
        printf("faacEncOpen  inputSamples=%lu, maxOutputBytes=%lu\n", obj->inputSamples, obj->maxOutputBytes);
        
        faacEncConfigurationPtr faacConfig = faacEncGetCurrentConfiguration(obj->hEncoder);
    
        obj->outputBuffer = new unsigned char[obj->maxOutputBytes];
        faacConfig->inputFormat = FAAC_INPUT_16BIT;
        faacConfig->bitRate = 8000;
        faacConfig->outputFormat = 1; // 0 = Raw; 1 = ADTS
    
        int result = faacEncSetConfiguration(obj->hEncoder, faacConfig);
        printf("faacEncSetConfiguration return %d\n", result);
    
        obj->env_ = env;
        status = napi_wrap(env,
                        jsthis,
                        reinterpret_cast<void*>(obj),
                        AACEncoder::Destructor,
                        nullptr,  // finalize_hint
                        &obj->wrapper_);
        assert(status == napi_ok);
    
        return jsthis;
    } 
    else 
    {
        // Invoked as plain function `AACEncoder(...)`, turn into construct call.
        size_t argc_ = 1;
        napi_value args[1];
        status = napi_get_cb_info(env, info, &argc_, args, nullptr, nullptr);
        assert(status == napi_ok);
    
        const size_t argc = 1;
        napi_value argv[argc] = {args[0]};
    
        napi_value instance;
        status = napi_new_instance(env, Constructor(env), argc, argv, &instance);
        assert(status == napi_ok);
    
        return instance;
    }
}

napi_value AACEncoder::encode(napi_env env, napi_callback_info info)
{
    napi_status status;

    size_t argc = 1;
    napi_value value;
    napi_value jsthis;
    status = napi_get_cb_info(env, info, &argc, &value, &jsthis, nullptr);
    assert(status == napi_ok);

    AACEncoder* obj;
    status = napi_unwrap(env, jsthis, reinterpret_cast<void**>(&obj));
    assert(status == napi_ok);

    size_t in_length = 0;
    unsigned char *iptr = NULL;
    status = napi_get_arraybuffer_info(env, value, (void**)&iptr, &in_length);
    assert(status == napi_ok);

    printf("Input %ld bytes (%p) to encode\n", in_length, iptr);

    int nbBytesEncoded = faacEncEncode(obj->hEncoder, (int32_t*)iptr, in_length, obj->outputBuffer, obj->maxOutputBytes);

    printf("Encoded %d bytes\n", nbBytesEncoded);

    napi_value arraybuffer;
    size_t out_length = nbBytesEncoded;
    unsigned char *dptr = NULL;
    status = napi_create_arraybuffer(env, out_length, (void**) &dptr, &arraybuffer);
    memcpy(dptr, obj->outputBuffer, out_length);

    return arraybuffer;
}

napi_value Init(napi_env env, napi_value exports) 
{
    return AACEncoder::Init(env, exports);
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
