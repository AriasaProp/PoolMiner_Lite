#include <cstdint>
#include <cstring>
#include <endian.h>
#include <jni.h>
#include <string>

JavaVM *global_jvm;

extern bool MinerService_OnLoad(JNIEnv*);

jint JNI_OnLoad (JavaVM *vm, void *) {
  JNIEnv *env;
  if (
      (vm->GetEnv ((void **)&env, JNI_VERSION_1_6) != JNI_OK) ||
      !MinerService_OnLoad(env)
  ) return JNI_ERR;

  global_jvm = vm;
  return JNI_VERSION_1_6;
}

extern void MinerService_OnUnload(JNIEnv*);

void JNI_OnUnload (JavaVM *vm, void *) {
  JNIEnv *env;
  if (vm->GetEnv ((void **)&env, JNI_VERSION_1_6) != JNI_OK)
  return;

  MinerService_OnUnload(env);
  global_jvm = nullptr;
}
