#ifndef AAC_ENCODEROBJECT_H_
#define AAC_ENCODEROBJECT_H_

#include <node_api.h>
#include <faac.h>

class AACEncoder {
 public:
  static napi_value Init(napi_env env, napi_value exports);
  static void Destructor(napi_env env, void* nativeObject, void* finalize_hint);

 private:
  explicit AACEncoder(ulong sampleRate_ = 8000, uint numChannels_ = 2);
  ~AACEncoder();

  static napi_value New(napi_env env, napi_callback_info info);
  static napi_value encode(napi_env env, napi_callback_info info);
  static inline napi_value Constructor(napi_env env);

  ulong sampleRate_;
  uint numChannels_;
  ulong inputSamples;
  ulong maxOutputBytes;
  faacEncHandle  hEncoder;
  unsigned char *outputBuffer;

  napi_env env_;
  napi_ref wrapper_;
};

#endif  // AAC_ENCODEROBJECT_H_
