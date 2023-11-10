#include "hashing.hpp"
#include <cstdint>
#include <cstring>
#include <endian.h>
#include <jni.h>
#include <string>

JavaVM *global_jvm;

jint JNI_OnLoad (JavaVM *vm, void *) {
  JNIEnv *env;
  if (
      (vm->GetEnv ((void **)&env, JNI_VERSION_1_6) != JNI_OK)) return JNI_ERR;

  global_jvm = vm;
  return JNI_VERSION_1_6;
}
void JNI_OnUnload (JavaVM *vm, void *) {
  JNIEnv *env;
  if (vm->GetEnv ((void **)&env, JNI_VERSION_1_6) != JNI_OK)
    return;

  global_jvm = nullptr;
}
